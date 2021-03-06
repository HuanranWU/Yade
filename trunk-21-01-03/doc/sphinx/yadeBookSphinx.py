#!/usr/bin/python
# encoding: utf-8
#
# module documentation
#

from builtins import range
import sys,os,os.path

outDir=sys.argv[2] if len(sys.argv)>2 else 'book/_build/'
for d in (outDir,outDir+'/latex',outDir+'/html',outDir+'/html/_static'):
	if not os.path.exists(d):
		os.mkdir(d)
		
#We assume that modules have been documented by a previous run of yadeSphinx
#def moduleDoc(module,submodules=[]):
	#f=open('yade.'+module+'.rst','w')
	#modFmt=""".. automodule:: yade.%s
	#:members:
	#:undoc-members:
	#:inherited-members:

#"""
	#f.write(""".. _yade.%s:

#yade.%s module
#==========================================

#"""%(module,module))
	##f.write(".. module:: yade.%s\n\n"%module+modFmt%module)
	#f.write(modFmt%module)
	#for sub in submodules:
		##f.write(".. submodule:: yade.%s\n\n"%sub+modFmt%sub)
		#f.write(".. currentmodule:: yade.%s\n\n%s\n\n"%(module,modFmt%sub))
	#f.close()
##
## put all yade modules here, as {module:[submodules]}
##
## don't forget to put the module in index.rst as well!
##
#mods={'export':[],'post2d':[],'pack':['_packSpheres','_packPredicates','_packObb'],'plot':[],'timing':[],'utils':['_utils'],'polyhedra_utils':['_polyhedra_utils'],'ymport':[],'geom':[],'bodiesHandling':[],'qt':['qt._GLViewer'],'linterpolation':[]}
##
## generate documentation, in alphabetical order
#mm=mods.keys(); mm.sort()
#for m in mm: moduleDoc(m,mods[m])

#with open('modules.rst','w') as f:
	#f.write("Yade modules reference\n=============\n\n.. toctree::\n\t:maxdepth: 2\n\n")
	#for m in mm: f.write('\tyade.%s.rst\n\n'%m)



#import sys
#sys.exit(0)

#
# wrapper documentation
#

# classes already documented
docClasses=set()

def autodocClass(klass):
	global docClasses
	docClasses.add(klass)
	return ".. autoclass:: %s\n\t:members:\n\t:undoc-members:\n\t:inherited-members:\n\n"%(klass) # \t:inherited-members:\n # \n\t:show-inheritance:
def autodocDerived(klass,doSections=True):
	global docClasses
	ret=''
	if doSections: ret+='%s\n'%klass+'^'*100+'\n\n'
	#ret+='.. inheritance-diagram:: %s\n\n'%klass
	ret+=inheritanceDiagram(klass)
	ret+=autodocClass(klass)
	childs=list(yade.system.childClasses(klass));
	childs.sort();
	for k in childs:
		if not k in docClasses: ret+=autodocClass(k)
	return ret
def inheritanceDiagram(klass):
	"""Generate simple inheritance graph for given classname *klass* using dot (http://www.graphviz.org) syntax.
	Classes in the docClasses set (already documented) are framed differently and the inheritance tree stops there.
	"""
	global docClasses
	def mkUrl(c):
		global writer
		# useless, doesn't work in LaTeX anyway...
		return ('#yade.wrapper.%s'%c if writer=='html' else '%%yade.wrapper#yade.wrapper.%s'%c) # HTML/LaTeX
	def mkNode(c,style='solid',fillcolor=None): return '\t\t"%s" [shape="box",fontsize=8,style="setlinewidth(0.5),%s",%sheight=0.2,URL="yade.wrapper.html#yade.wrapper.%s"];\n'%(c,style,'fillcolor=%s,'%fillcolor if fillcolor else '',c)
	ret=".. graphviz::\n\n\tdigraph %s {\n\t\trankdir=RL;\n\t\tmargin=.2;\n"%klass+mkNode(klass)
	childs=yade.system.childClasses(klass)
	if len(childs)==0: return ''
	for c in childs:
		try:
			base=eval(c).__bases__[0].__name__
			if base!=klass and base in docClasses:
				continue # skip classes deriving from classes that are already documented
			if c not in docClasses: ret+=mkNode(c)
			else: # classes of which childs are documented elsewhere are marked specially
				ret+=mkNode(c,style='filled,dashed',fillcolor='grey')
			ret+='\t\t"%s" -> "%s" [arrowsize=0.5,style="setlinewidth(0.5)"];\n'%(c,base)
		except NameError:
			pass
			#print 'WARN: unable to find class object for',c
	return ret+'\t}\n\n'


def sect(title,text,tops,reverse=False):
	subsects=[autodocDerived(top,doSections=(len(tops)>1)) for top in tops]
	if reverse: subsects.reverse()
	return title+'\n'+100*'-'+'\n\n'+text+'\n\n'+'\n\n'.join(subsects)+'\n'


def genWrapperRst():
	global docClasses
	docClasses=set() # reset globals
	wrapper=file('yade.wrapper.rst','w')
	wrapper.write(""".. _yade.wrapper::

Yade wrapper class reference
============================

.. toctree::
  :maxdepth: 2


.. currentmodule:: yade.wrapper
.. autosummary::

"""+
	sect('Bodies','',['Body','Shape','State','Material','Bound'])+
	sect('Interactions','',['Interaction','IGeom','IPhys'])+
	sect('Global engines','',['FieldApplier','Collider','BoundaryController','GlobalEngine'],reverse=True)+
	sect('Partial engines','',['PartialEngine'])+
	sect('Bounding volume creation','',['BoundFunctor','BoundDispatcher'])+
	sect('Interaction Geometry creation','',['IGeomFunctor','IGeomDispatcher'])+
	sect('Interaction Physics creation','',['IPhysFunctor','IPhysDispatcher'])+
	sect('Constitutive laws','',['LawFunctor','LawDispatcher'])+
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
	for l in open(f):
		if writer=='html':
			if not '(</big><em>inherits ' in l:
				out.append(l)
				continue
			m=re.match(r'(^.*\(</big><em>inherits )([a-zA-Z0-9_ →]*)(</em><big>\).*$)',l)
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
			if not (r'\pysiglinewithargsret{\sphinxstrong{class }\code{yade.wrapper.}\bfcode{' in l and r'\emph{inherits' in l):
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
		ff=open(f,'w')
		for l in out:
			ff.write(l)
		ff.close()
	
def processTemplate(f1,f2):
	"Copy file f1 to f2, evaluating all occurences of @...@ with eval(); they should expand to a string and can contain arbitrary python expressions."
	import re
	def doEval(match):
		import bib2rst
		return str(eval(match.group(1)))
	ff2=open(f2,'w')
	for l in open(f1):
		ff2.write(re.sub(r'@([^@]*)@',doEval,l))

def genReferences():
	import sys
	processTemplate('book/fullBibliography.rst.in','fullPublications.rst')

import sphinx,sys,shutil
sys.path.append('.') # for bib2rst

genReferences()
for bib in ('references','yade-articles','yade-theses','yade-conferences','citing_yade'):
	shutil.copyfile('../%s.bib'%bib,outDir+'/latex/%s.bib'%bib)

global writer
writer=None
#writers=['html','latex','epub']
writers=['latex']

volumes=['Book','Reference','Manual','Theory'] #would work if yade was exiting correctly (it seems)
for vol in volumes:
	# no way to use custom file name for conf.py, instead we replace it by the variants
	shutil.copy('book/conf'+vol+'.py','book/conf.py')
	for writer in writers:
		#genWrapperRst()
		# HACK: must rewrite sys.argv, since reference generator in conf.py determines if we output latex/html by inspecting it
		sys.argv=['sphinx-build','-a','-c','book','-E','-b','%s'%writer,'-d',outDir+'/doctrees','.',outDir+'/%s'%writer]
		try:
			sphinx.main(sys.argv)
		except SystemExit:
			pass
		import glob
		texDocs=glob.glob(outDir+'latex/*.tex')
		if writer=='html':
			makeBaseClassesClickable((outDir+'/html/yade.wrapper.html'),writer)
		elif writer=='latex':
			for texdoc in texDocs: makeBaseClassesClickable(texdoc,writer)
		if (os.path.exists('/usr/share/javascript/jquery/jquery.js')): #Check, whether jquery.js installed in system
			if os.path.isfile(outDir+'/html/_static/jquery.js'): os.system('rm '+ outDir+'/html/_static/jquery.js')
			os.system('ln -s /usr/share/javascript/jquery/jquery.js '+ outDir+'/html/_static/jquery.js')

		# HACK!!!!==========================================================================
		# New sphinx-python versions (hopefully) are producing empty "verbatim"-environments.
		# That is why xelatex crashes. 
		# The following "script" removes all empty environments. Needs to be fixed in python-sphinx.
		if (writer=='latex'):
			for texdoc in texDocs: 
				infile = open(texdoc,"r")
				lines = infile.readlines()
				infile.close()
				
				out=[]
				for i in range(0,len(lines)):
					if (i!=len(lines) and
							lines[i].strip()=="\\begin{Verbatim}[commandchars=\\\\\{\\}]" and
							lines[i+1].strip()=="\\end{Verbatim}"):
							lines[i]=''; lines[i+1]=''
					else:
						out.append(lines[i])
				file(texdoc,'w').write('')
				for i in out:
					file(texdoc,'a').write(i)
# HACK!!!!==========================================================================
sys.exit()
