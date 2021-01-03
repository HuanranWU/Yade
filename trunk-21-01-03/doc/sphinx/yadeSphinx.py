#!/usr/bin/python
# encoding: utf-8
#
# module documentation
#

from builtins import range
import sys,os,os.path
try: #for python 3.4+
	from importlib import reload
except:
	pass
reload(sys)
try: #for python2
	sys.setdefaultencoding('utf8')
except:
	pass

writer=sys.argv[1]
print("Running yadeSphinx.py with writer ==",writer)

outDir=sys.argv[2] if len(sys.argv)>2 else os.getcwd()+'/_build'
for d in (outDir,outDir+'/latex',outDir+'/html'):
    if not os.path.exists(d):
        os.mkdir(d)

def moduleDoc(module,submodules=[]):
    f=open('yade.'+module+'.rst','w',encoding="utf8")
    modFmt=""".. automodule:: yade.%s
    :members:
    :undoc-members:
    :inherited-members:

"""
    f.write(""".. _yade.%s:

yade.%s module
==========================================

"""%(module,module))
    #f.write(".. module:: yade.%s\n\n"%module+modFmt%module)
    f.write(modFmt%module)
    for sub in submodules:
        #f.write(".. submodule:: yade.%s\n\n"%sub+modFmt%sub)
        f.write(".. currentmodule:: yade.%s\n\n%s\n\n"%(module,modFmt%sub))
    f.close()
#
# put all yade modules here, as {module:[submodules]}
#
# NOTE: in file doc/sphinx/conf.py there is a moduleMap={……} variable which must reflect what is written below.
# hint: follow changes in d067b0696a8 to add new modules.
mods={
          'libVersions'     : ['_libVersions']
        , 'log'             : ['_log']
        , 'math'            : ['_math']
        , 'minieigenHP'     : ['_minieigenHP']
        , 'export'          : []
        , 'post2d'          : []
        , 'pack'            : ['_packSpheres','_packPredicates','_packObb']
        , 'plot'            : []
        , 'timing'          : []
        , 'utils'           : ['_utils']
        , 'polyhedra_utils' : ['_polyhedra_utils']
        , 'ymport'          : []
        , 'geom'            : []
        , 'bodiesHandling'  : []
        , 'qt'              : ['qt._GLViewer']
        , 'linterpolation'  : []
        , 'gridpfacet'      : []
        , 'mpy'             : []
    }
#
# generate documentation, in alphabetical order
mm=list(mods.keys()); mm.sort()
for m in mm: moduleDoc(m,mods[m])

with open('modules.rst','w',encoding="utf8") as f:
    f.write("Yade modules reference\n======================\n\n.. toctree::\n\t:maxdepth: 2\n\n")
    for m in mm: f.write('\tyade.%s.rst\n\n'%m)



#import sys
#sys.exit(0)

#
# wrapper documentation
#

# classes already documented
docClasses=set()

def childSet(klasses):
    ret=set()
    for klass in klasses:
        ret.add(klass)
        for child in yade.system.childClasses(klass):
            ret.add(child)
    return ret

def autodocClass(klass):
    global docClasses
    docClasses.add(klass)
    return ".. autoclass:: %s\n\t:members:\n\t:undoc-members:\n\t:inherited-members:\n\n"%(klass) # \t:inherited-members:\n # \n\t:show-inheritance:
def autodocDerived(klass,doSections=True,willBeLater=set()):
    global docClasses
    ret=''
    if doSections: ret+='%s\n'%klass+'^'*100+'\n\n'
    #the hyperlinks to those graphs work like: :ref:`inheritanceGraphBoundFunctor`  or  :ref:`BoundFunctor<inheritanceGraphBoundFunctor>`
    ret+=".. _inheritanceGraph%s:\n\n"%klass
    ret+=inheritanceDiagram(klass,willBeLater)
    ret+=autodocClass(klass)
    childs=list(yade.system.childClasses(klass));
    childs.sort();
    for k in childs:
        if not k in (docClasses|willBeLater): ret+=autodocClass(k)
    return ret
def inheritanceDiagram(klass,willBeLater):
    """Generate simple inheritance graph for given classname *klass* using dot (http://www.graphviz.org) syntax.
    Classes in the docClasses set (already documented) are framed differently and the inheritance tree stops there.
    """
    global docClasses
    global writer
    def linkGraphOrClass(klassToLink, forceYrefClassOnly=False):
        if(forceYrefClassOnly or len(childSet([klassToLink]))==1):
            return ":yref:`"+klassToLink+"`"
        else:
            return ":ref:`"+klassToLink+"<inheritanceGraph"+klassToLink+">"+"`"
    def mkUrl(c):
        global writer
        # useless, doesn't work in LaTeX anyway...
        return ('#yade.wrapper.%s'%c if writer=='html' else '%%yade.wrapper#yade.wrapper.%s'%c) # HTML/LaTeX
    def mkNode(c,style='solid',fillcolor=None,isElsewhere=False): return '\t\t"%s" [shape="box",fontsize=%i,style="setlinewidth(0.5),%s",%sheight=0.2,URL="yade.wrapper.html#%s"];\n'%(c,8 if writer=='html' else 20,style,'fillcolor=%s,'%fillcolor if fillcolor else '', "inheritancegraph"+c.lower() if (isElsewhere and len(childSet([c])) != 1) else "yade.wrapper."+c)
    childs=yade.system.childClasses(klass)
    maxDepth=1
    def countUp(c , klass):
        ret , base = 2 , eval(c).__bases__[0].__name__
        while(base != klass):
            ret += 1
            base = eval(base).__bases__[0].__name__
        return ret
    for c in childs:
        try:
            if c not in (docClasses|willBeLater):
                maxDepth=max(maxDepth,countUp(c,klass))
        except NameError:
            pass
    # https://www.graphviz.org/doc/info/attrs.html , http://www.sphinx-doc.org/en/master/usage/extensions/graphviz.html , http://www.markusz.io/posts/drafts/graphviz-sphinx/
    # margin size is in inches. The text area on page in .pdf is 6.3in by 9.8in. I'll use a default that each class uses one fifth of page width. If maxDepth>=6 then the image just gets smaller.
    pageWidth=6.3
    pageFraction=5
    fixPdfMargin=(pageWidth/pageFraction)*max(0,pageFraction-maxDepth)/2
    ret=""
    extraCaption=["",0]
    extraPdfCaptionSet=set()
    if len(childs)==0: return ''
    for c in childs:
        try:
            base=eval(c).__bases__[0].__name__
            if base!=klass and base in (docClasses|willBeLater):
                continue # skip classes deriving from classes that are already documented
            if c not in (docClasses|willBeLater):
                ret+=mkNode(c)
                extraPdfCaptionSet.add(c) # I use set() to make sure they are sorted alphabetically
            else: # classes of which childs are documented elsewhere are marked specially
                ret+=mkNode(c,style='filled,dashed',fillcolor='grey',isElsewhere=True)
                extraCaption[0] += " "+linkGraphOrClass(c)+","
                extraCaption[1] += 1
            ret+='\t\t"%s" -> "%s" [arrowsize=0.5,style="setlinewidth(0.5)"];\n'%(c,base)
        except NameError:
            pass
            #print 'WARN: unable to find class object for',c
    if(extraCaption[1] == 0):  extraCaption[0] = "."
    # the [:-1] is to cut off the last comma after the last :yref:
    if(extraCaption[1] == 1):  extraCaption[0] = ", the gray dashed class is discussed in a separate section: " + extraCaption[0][:-1] + "."
    if(extraCaption[1] >= 2):  extraCaption[0] = ", gray dashed classes are discussed in their own sections: "  + extraCaption[0][:-1] + "."

    # sphinx+graphviz does not support URL directive inside the graph. The bug is in sphinx, because it can generate only .png and .svg. And from sphinx the .png goes to latex,
    # while graphviz suppports URLs directives in latex via .ps format which is not supported by sphinx. So I will add the URLs in the caption.
    # https://www.graphviz.org/pdf/dotguide.pdf
    extraPdfCaption=["",len(extraPdfCaptionSet)]
    for c in sorted(extraPdfCaptionSet): extraPdfCaption[0] += " "+linkGraphOrClass(c,True)+","
    if(extraPdfCaption[1] == 0):  extraPdfCaption[0] = ""
    if(extraPdfCaption[1] == 1):  extraPdfCaption[0] = " See also: "+extraPdfCaption[0][:-1]+"."
    if(extraPdfCaption[1] >= 2):  extraPdfCaption[0] = " See also: "+extraPdfCaption[0][:-1]+"."
    if(writer=='html'): extraPdfCaption[0] = ""

    head=".. graphviz::"+("\n\t:caption: Inheritance graph of %s"%(klass))+extraCaption[0]+extraPdfCaption[0]+"\n\n\tdigraph %s {"%klass + "\n\t\trankdir=RL;\n\t\tmargin="+("\"%0.1f,0.05\""%(0.2 if writer=='html' else fixPdfMargin))+";\n"+('\t\tsize="'+str(pageWidth-2*fixPdfMargin)+',999!";\n' if writer!='html' else "")+mkNode(klass)
    return head+ret+'\t}\n\n'


def sect(title,text,tops,reverse=False,willBeLater=set()):
    subsects=[autodocDerived(top,doSections=(len(tops)>1),willBeLater=willBeLater) for top in tops]
    if reverse: subsects.reverse()
    return title+'\n'+100*'-'+'\n\n'+text+'\n\n'+'\n\n'.join(subsects)+'\n'

def genWrapperRst():
    global docClasses
    docClasses=set() # reset globals
    wrapper=open('yade.wrapper.rst','w',encoding="utf8")
    wrapper.write(""".. _yade.wrapper::

Yade wrapper class reference
============================

.. toctree::
  :maxdepth: 2


.. currentmodule:: yade.wrapper

"""+
    sect('Bodies','',['Body','Shape','State','Material','Bound'])+
    sect('Interactions','',['Interaction','IGeom','IPhys'])+
    sect('Global engines','',['FieldApplier','Collider','BoundaryController','PeriodicEngine','GlobalEngine'],reverse=True)+
    sect('Partial engines','',['PartialEngine'])+
    sect('Dispatchers','',['Dispatcher'],willBeLater=childSet(['BoundDispatcher', 'IGeomDispatcher', 'IPhysDispatcher', 'LawDispatcher', 'InternalForceDispatcher']))+
    sect('Functors','',['Functor'],willBeLater=childSet(['IPhysFunctor', 'GlStateFunctor', 'GlIGeomFunctor', 'IGeomFunctor', 'LawFunctor', 'GlBoundFunctor', 'GlIPhysFunctor', 'GlShapeFunctor', 'BoundFunctor', 'InternalForceFunctor']))+
    sect('Bounding volume creation','',['BoundFunctor','BoundDispatcher'])+
    sect('Interaction Geometry creation','',['IGeomFunctor','IGeomDispatcher'])+
    sect('Interaction Physics creation','',['IPhysFunctor','IPhysDispatcher'])+
    sect('Constitutive laws','',['LawFunctor','LawDispatcher'])+
    sect('Internal forces','',['InternalForceFunctor', 'InternalForceDispatcher'])+
    sect('Callbacks','',[#'BodyCallback',
        'IntrCallback'])+
    sect('Preprocessors','',['FileGenerator'])+
    sect('Rendering','',['OpenGLRenderer','GlShapeFunctor','GlStateFunctor','GlBoundFunctor','GlIGeomFunctor','GlIPhysFunctor'])+ # ,'GlShapeDispatcher','GlStateDispatcher','GlBoundDispatcher','GlIGeomDispatcher','GlIPhysDispatcher'])+
    sect('Simulation data','',['Omega','BodyContainer','InteractionContainer','ForceContainer','MaterialContainer','Scene','Cell'])
    +"""
Other classes
---------------

"""
    +''.join([autodocClass(k) for k in (yade.system.childClasses('Serializable')-docClasses)|set(['Serializable','TimingDeltas','Cell'])])
    )

    wrapper.close()

def makeBaseClassesClickable(f,writer):
    out=[]
    import re,shutil
    changed=False
    for l in open(f,encoding="utf8"):
        if writer=='html':
            if not '(</span><em>inherits ' in l:
                out.append(l)
                continue
            m=re.match(r'(^.*\(</span><em>inherits )([a-zA-Z0-9_ →]*)(</em><span class="sig-paren">\).*$)',l)
            if not m:
                out.append(l)
                continue
                #raise RuntimeError("Line %s unmatched?"%l)
            bases=m.group(2)
            bb=bases.split(' → ')
            #print bases,'@',bb
            bbb=' → '.join(['<a class="reference external" href="yade.wrapper.html#yade.wrapper.%s">%s</a>'%(b,b) for b in bb])
            out.append(m.group(1)+bbb+m.group(3))
        elif writer=='latex':
            if(     (not (r'\pysiglinewithargsret{\sphinxstrong{class }\sphinxcode{yade.wrapper.}\sphinxbfcode{' in l and r'\emph{inherits' in l)) # debian stretch, devuan ascii
                and (not (r'\pysiglinewithargsret{\strong{class }\code{yade.wrapper.}\bfcode{' in l and r'\emph{inherits' in l)) # ubuntu xenial 16.04
                and (not (r'\pysiglinewithargsret{\sphinxbfcode{class }\sphinxcode{yade.wrapper.}\sphinxbfcode{' in l and r'\emph{inherits' in l)) # ubuntu bionic 18.04
                and (not (r'\pysiglinewithargsret{\sphinxbfcode{\sphinxupquote{class }}\sphinxcode{\sphinxupquote{yade.wrapper.}}\sphinxbfcode{\sphinxupquote{' in l and r'\emph{inherits' in l)) #  debian buster, ubuntu cosmic 18.10, ubuntu disco, debian sid
            ):
                out.append(l)
                continue
            #print l
            m=re.match(r'(^.*}}{\\emph{inherits )([a-zA-Z0-9_\\() -]*)(}}.*$)',l)
            if not m:
                #print 'WARN: Line %s unmatched?'%l
                continue
            bases=m.group(2)
            bb=bases.split(r' \(\rightarrow\) ')
            bbb=r' \(\rightarrow\) '.join([r'\hyperref[yade.wrapper:yade.wrapper.%s]{%s}'%(b.replace(r'\_',r'_'),b) for b in bb])
            out.append(m.group(1)+bbb+m.group(3)+'\n')
        changed=True
    if changed:
        shutil.move(f,f+'_')
        ff=open(f,'w',encoding="utf8")
        for l in out:
            ff.write(l)
        ff.close()

def processTemplate(f1,f2):
    "Copy file f1 to f2, evaluating all occurences of @...@ with eval(); they should expand to a string and can contain arbitrary python expressions."
    import re
    def doEval(match):
        import bib2rst
        return str(eval(match.group(1)))
    ff2=open(f2,'w',encoding="utf8")
    for l in open(f1,encoding="utf8"):
        ff2.write(re.sub(r'@([^@]*)@',doEval,l))

def genReferences():
    import sys
    processTemplate('references.rst.in','references.rst')
    processTemplate('publications.rst.in','publications.rst')

import sphinx,sys,shutil
sys.path.append('.') # for bib2rst

genReferences()
for bib in ('references','yade-articles','yade-theses','yade-conferences','citing_yade'):
    shutil.copyfile('../%s.bib'%bib,outDir+'/latex/%s.bib'%bib)

# NOTE: for some unexplained reason, all code after the sphinx.main() invocation will not be executed (python exits) on docker images (gitlab), despite the try: except: statements.
# Workaround: launch this script as many times as needed (3 times for doc compilations and 1 time for post workarounds), see CMakeLists.txt.
if(writer != "workarounds"):
	genWrapperRst()
	runBuildParams=['sphinx-build','-a','-v','-T','-P','-E','-b','%s'%writer,'-d',outDir+'/doctrees','.',outDir+'/%s'%writer]

	if (sphinx.version_info[0] == 1):
		# In newer sphinx version sphinx.main was deprecated. Keep for backcompatability
		sphinx.main(runBuildParams)
	else:
		import sphinx.cmd.build
		sphinx.cmd.build.main(runBuildParams[1:])
else:
	#HTML FIXES:
	makeBaseClassesClickable((outDir+'/html/yade.wrapper.html'),"html")
	if (os.path.exists('/usr/share/javascript/jquery/jquery.js')): #Check, whether jquery.js installed in system
	    os.system('rm '+ outDir+'/html/_static/jquery.js')
	    os.system('cp /usr/share/javascript/jquery/jquery.js '+ outDir+'/html/_static/jquery.js')

	#LATEX FIXES:
	makeBaseClassesClickable((outDir+'/latex/Yade.tex'),"latex")
	###HACK: sphinx sometimes produces lots of backslashes in tex source on ipython outs (with are '\PYGZbs{}' in the source) -> remove them all.
	import re
	find_tex_backslashes=re.compile(r'^(\\PYG{g\+go}{)(\\PYGZbs{})*')
	with open(outDir+'/latex/Yade.tex','r',encoding="utf8") as f:
		lines=f.readlines()
	with open(outDir+'/latex/Yade.tex','w',encoding="utf8") as f:
		for l in lines:
			f.write(find_tex_backslashes.sub(r'\1',l))
	###HACK
sys.exit()
