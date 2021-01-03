# encoding: utf-8
from __future__ import print_function

from yade import pack,export,plot
import math,os,sys,shutil,subprocess,tempfile
print('checkVTKRecorder')

#### This is useful for printing the linenumber in the script
# import inspect
# print(inspect.currentframe().f_lineno)

if((opts.threads != None and opts.threads != 1) or (opts.cores != None and opts.cores != '1')):
	raise YadeCheckError("This test will only work on single core, because it must be fully reproducible, but -j "+str(opts.threads)+" or --cores "+str(opts.cores)+" is used.")

from yade import pack
import yade.libVersions

if ('VTK' in features):
#checksPath="." # this line was used for working on this script locally.
	tmpSaveDir = tempfile.mkdtemp()
	vtkSaveDir = tmpSaveDir+'/vtk_testing/'
	if not os.path.exists(vtkSaveDir):
		os.makedirs(vtkSaveDir);

	O.periodic=False
	length=1.0
	height=1.0
	width=1.0
	thickness=0.1

	O.materials.append(FrictMat(density=1,young=1e5,poisson=0.3,frictionAngle=radians(30),label='boxMat'))
	lowBox = box( center=(length/2.0,thickness*0.6,width/2.0), extents=(length*2.0,thickness/2.0,width*2.0) ,fixed=True,wire=False)
	O.bodies.append(lowBox)

	radius=0.01
	O.materials.append(FrictMat(density=1000,young=1e4,poisson=0.3,frictionAngle=radians(30),label='sphereMat'))
	sp=pack.SpherePack()
#sp.makeCloud((0.*length,height+1.2*radius,0.25*width),(0.5*length,2*height-1.2*radius,0.75*width),-1,.2,2000,periodic=True)
	sp.load(checksPath+'/data/100spheres')
# 100 was not enough to have reasonable number of collisions, so I put 200 spheres.
	O.bodies.append([sphere(s[0]+Vector3(0.0,0.2,0.0),s[1]) for s in sp])
	O.bodies.append([sphere(s[0]+Vector3(0.1,0.3,0.0),s[1]) for s in sp])

	O.dt=5e-4
	O.usesTimeStepper=False
	newton=NewtonIntegrator(damping=0.6,gravity=(0,-10,0))

	O.engines=[
		ForceResetter(),
		#(1) This is where we allow big bodies, else it would crash due to the very large bottom box:
		InsertionSortCollider([Bo1_Box_Aabb(),Bo1_Sphere_Aabb()],allowBiggerThanPeriod=True),
		InteractionLoop(
			[Ig2_Sphere_Sphere_ScGeom(),Ig2_Box_Sphere_ScGeom()],
			[Ip2_FrictMat_FrictMat_FrictPhys()],
			[Law2_ScGeom_FrictPhys_CundallStrack()]
		),
		VTKRecorder(fileName=vtkSaveDir,recorders=['all'], firstIterRun=10, iterPeriod=2000 ,label="VtkRecorder", ascii=True, multiblock=True),
		newton
	]

	for b in O.bodies:
		b.shape.color=Vector3(b.id%8/8.0,b.id%8/8.0,0.5)

	O.run( 20, True);

	toSkip=['dirI','dirII','dirIII'] # these values are too sensitive so ignore them.
	section=""
	skippedLines=0
	vtkVer=yade.libVersions.getVersion('vtk')
	if(vtkVer[0] not in [6,8]):
		raise YadeCheckError("checkVTKRecorder does not have reference results for VTK version "+str(vtkVer)+", check files in "+vtkSaveDir+", if they are correct add them to: scripts/checks-and-tests/checks/data/vtk_reference_"+str(vtkVer[0])+'/')
	if(vtkVer[0]==6): fileList=['10.vtm','10/10_0.vtu','10/10_1.vtu','10/10_2.vtp']
	if(vtkVer[0]==8): fileList=['10.vtm','10/10_0.vtu','10/10_2.vtp']
	verName = str(vtkVer[0])
	if(vtkVer[0] == 8):
		if(vtkVer[1] < 2):
			verName += '.1'
		else:
			verName += '.2' # let's hope that higher versions will produce output like ver 8.2
	for fname in fileList:
		print("checking file: ",vtkSaveDir+fname)
		referenceFile = open( checksPath+'/data/vtk_reference_'+verName+'/'+fname, "r" )
		testedFile    = open( vtkSaveDir+fname, "r" )
		lineCount=0
		for line1, line2 in zip(referenceFile, testedFile):
			lineCount+=1
			t1 = line1.split('"')
			t2 = line2.split('"')
			isHeader=False
			if(t1[0]=='        <DataArray type=' and t2[0]=='        <DataArray type='):
				if(t1[3]==t2[3]):
					section=t1[3]
					print("checking section: ", section, ("(non-matching lines allowed)" if (section in toSkip) else ""))
				else:
					raise YadeCheckError("checkVTKRecorder cannot determine section name in file "+fname+" line: "+str(lineCount)+" with lines: \n"+line1+"\nvs.\n"+line2)
			if(t1[0]=='<VTKFile type='): isHeader=True # various VTK versions have different headers.
			if((line1 != line2) and (not isHeader)): # we have some differences, check if they are acceptable
				# flatten the list of lists. First they are split by space, then they are split by '"'
				sp1 = [val for sublist in [i.split('"') for i in line1.split()] for val in sublist]
				sp2 = [val for sublist in [i.split('"') for i in line2.split()] for val in sublist]
				if(section in toSkip):
					skippedLines+=1
					#print("skipping line: ",line1)
				else:
					if(len(sp1)!=len(sp2)):
						raise YadeCheckError("checkVTKRecorder failed in file "+fname+" line: "+str(lineCount)+", because the lines have different elements:\n"+line1+"\nvs.\n"+line2)
					for s1, s2 in zip(sp1,sp2):
						try:
							if( abs( float(s1) - float(s2) ) > 1e-8 ):
								raise YadeCheckError("checkVTKRecorder failed float comparison in file "+fname+" line: "+str(lineCount)+" with inputs: '"+str(s1)+ "' vs. '"+str(s2)+"'")
						except ValueError:
							if(s1=='>' and s2=='/>'):
								pass
							elif(s1=='Int64' and s2=='Int32'):
								pass
							elif(s1 != s2):
								raise YadeCheckError("checkVTKRecorder failed string comparison in file "+fname+" line: "+str(lineCount)+" with inputs: '"+str(s1)+ "' vs. '"+str(s2)+"'")
		
	print("non-matching lines: ",skippedLines)

	if(skippedLines > 100):
		raise YadeCheckError("checkVTKRecorder failed at the end because there were over 100 non-matching lines in sections where non-matching lines were allowed.")

else:
	print("skip VTKRecorder check, VTK is not available")

