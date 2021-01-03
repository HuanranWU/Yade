# encoding: utf-8
# 2020 © Vasileios Angelidakis <v.angelidakis2@ncl.ac.uk>
# A script to check the PotentialBlockVTKRecorder and PotentialParticleVTKRecorder engines.
# This script was written based on the existing check-script: checkVTKRecorder.py.

from __future__ import print_function
import os, tempfile

if((opts.threads != None and opts.threads != 1) or (opts.cores != None and opts.cores != '1')):
	raise YadeCheckError("This test will only work on single core, because it must be fully reproducible, but -j "+str(opts.threads)+" or --cores "+str(opts.cores)+" is used.")

if ('VTK' in features):
	tmpSaveDir = tempfile.mkdtemp()
#	checksPath='.' # Uncomment to run this script locally.
#	tmpSaveDir='.' # Uncomment to run this script locally.
	vtkSaveDir = tmpSaveDir+'/potentialVTKRecorders/'
	def checkVTK(prefix):
		vtkVer=yade.libVersions.getVersion('vtk')
		if(vtkVer[0]==8 and vtkVer[1]==2):
			extraPath = 'ver8.2/'
		else:
			extraPath = 'ver6-8.1/'
		toSkip=[] # Here we can put sections to ignore, if too sensitive
		section=""
		skippedLines=0
		fileList=[prefix+'contactPoint.10.vtu',prefix+'Id.10.vtu',prefix+'-pb.10.vtp',prefix+'vel.10.vtu']
		for fname in fileList:
			print("checking file: ",vtkSaveDir+fname)
			referenceFile = open( checksPath+'/data/potentialVTKRecorders/'+extraPath+fname, "r" )
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
#						print("checking section: ", section, ("(non-matching lines allowed)" if (section in toSkip) else ""))
					else:
						raise YadeCheckError("checkPotentialVTKRecorders cannot determine section name in file "+fname+" line: "+str(lineCount)+" with lines: \n"+line1+"\nvs.\n"+line2)
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
							raise YadeCheckError("checkPotentialVTKRecorders failed in file "+fname+" line: "+str(lineCount)+", because the lines have different elements:\n"+line1+"\nvs.\n"+line2)
						for s1, s2 in zip(sp1,sp2):
							try:
								if( abs( float(s1) - float(s2) ) > 5e-6 ):
									raise YadeCheckError("checkPotentialVTKRecorders failed float comparison in file "+fname+" line: "+str(lineCount)+" with inputs: '"+str(s1)+ "' vs. '"+str(s2)+"'")
							except ValueError:
								if(s1=='>' and s2=='/>'):
									pass
								elif(s1=='Int64' and s2=='Int32'):
									pass
								elif(s1 != s2):
									raise YadeCheckError("checkPotentialVTKRecorders failed string comparison in file "+fname+" line: "+str(lineCount)+" with inputs: '"+str(s1)+ "' vs. '"+str(s2)+"'")
			
		print("non-matching lines: ",skippedLines)

		if(skippedLines > 100):
			raise YadeCheckError("checkPotentialVTKRecorders failed at the end because there were over 100 non-matching lines in sections where non-matching lines were allowed.")

# check PotentialParticleVTKRecorder
if ('PotentialParticles' in features) and ('VTK' in features):
	if not os.path.exists(vtkSaveDir):
		os.makedirs(vtkSaveDir);
	# ----------------------------------------------------------------------------------------------------------------------------------------------- #
	# Engines
	O.engines=[
		ForceResetter(),
		InsertionSortCollider([PotentialParticle2AABB()],verletDist=0.01),
		InteractionLoop(
			[Ig2_PP_PP_ScGeom(twoDimension=False, calContactArea=True, areaStep=5)],
			[Ip2_FrictMat_FrictMat_KnKsPhys(kn_i=5e8, ks_i=5e7, Knormal=5e8, Kshear=5e7, useFaceProperties=False, viscousDamping=0.1)], 
			[Law2_SCG_KnKsPhys_KnKsLaw(label='PPlaw',neverErase=False)]
		),
		NewtonIntegrator(damping=0.0,exactAsphericalRot=True,gravity=[0,0,0]), # Here we deactivate gravity,
		PotentialParticleVTKRecorder(fileName=vtkSaveDir+'pp', firstIterRun=10, iterPeriod=2000, twoDimension=False, sampleX=10, sampleY=10, sampleZ=10, maxDimension=0.2, REC_INTERACTION=True, REC_COLORS=True, REC_VELOCITY=True, REC_ID=True, label='vtkRecorder')
	]

	# ----------------------------------------------------------------------------------------------------------------------------------------------- #
	# Materials
	O.materials.append(FrictMat(young=-1,poisson=-1,frictionAngle=radians(30.0),density=2000,label='frictional'))

	# ----------------------------------------------------------------------------------------------------------------------------------------------- #
	# Bodies
	edge=0.10
	k=0.8
	r=0.1*edge
	R=edge/2.

	# Body pp1=O.bodies[0]: Green
	pp1=Body()
	pp1.aspherical=True
	pp1.shape=PotentialParticle(
	k=k, r=r, R=R, a=[1, -1, 0, 0, 0, 0], b=[0, 0, 1, -1, 0, 0], c=[0, 0, 0, 0, 1,-1], d=[edge/2.-r]*6,
	id=len(O.bodies), isBoundary=False, color=[0, 1, .2], wire=False, highlight=False, AabbMinMax=True, fixedNormal=False,
	minAabb=Vector3(edge/2., edge/2., edge/2.)*2, minAabbRotated=Vector3(edge/2., edge/2., edge/2.),
	maxAabb=Vector3(edge/2., edge/2., edge/2.)*2, maxAabbRotated=Vector3(edge/2., edge/2., edge/2.))
	V=edge**3
	geomInertia=1/6.*V*edge**2.
	utils._commonBodySetup(pp1,V,Vector3(geomInertia, geomInertia, geomInertia), material='frictional', pos=[0,0,0], fixed=False)
	pp1.state.pos = [0,0,0]
	O.bodies.append(pp1)

	# Body pp2=O.bodies[1]: Red
	pp2=Body()
	pp2.aspherical=True
	pp2.shape=PotentialParticle(
	k=k, r=r, R=R, a=[1, -1, 0, 0, 0, 0], b=[0, 0, 1, -1, 0, 0], c=[0, 0, 0, 0, 1, -1], d=[edge/2.-r]*6,
	id=len(O.bodies), isBoundary=False, color=[1, .2, 0], wire=False, highlight=False, AabbMinMax=True, fixedNormal=False,
	minAabb=Vector3(edge/2., edge/2., edge/2.)*2, minAabbRotated=Vector3(edge/2., edge/2., edge/2.),
	maxAabb=Vector3(edge/2., edge/2., edge/2.)*2, maxAabbRotated=Vector3(edge/2., edge/2., edge/2.))
	V=edge**3
	geomInertia=1/6.*V*edge**2.
	utils._commonBodySetup(pp2,V,Vector3(geomInertia, geomInertia, geomInertia), material='frictional', pos=[0,0,0], fixed=False)
	pp2.state.pos = [edge/1.5,0,0]
	O.bodies.append(pp2)

	O.run(20, True);
	checkVTK('pp')
else:
	print("skip PotentialParticleVTKRecorder check, PotentialParticles or VTK not available")

# check PotentialBlockVTKRecorder
if ('PotentialBlocks' in features) and ('VTK' in features):
	O.resetThisScene()
	if not os.path.exists(vtkSaveDir):
		os.makedirs(vtkSaveDir);
	# ----------------------------------------------------------------------------------------------------------------------------------------------- #
	# Engines
	O.engines=[
		ForceResetter(),
		InsertionSortCollider([PotentialBlock2AABB()],verletDist=0.00),
		InteractionLoop(
			[Ig2_PB_PB_ScGeom(twoDimension=False, unitWidth2D=1.0, calContactArea=True)],
			[Ip2_FrictMat_FrictMat_KnKsPBPhys(kn_i=5e8, ks_i=5e7, Knormal=5e8, Kshear=5e7, useFaceProperties=False, viscousDamping=0.1)], 
			[Law2_SCG_KnKsPBPhys_KnKsPBLaw(label='PBlaw',neverErase=False, allowViscousAttraction=True)] # In this example, we do NOT use Talesnick
		),
		NewtonIntegrator(damping=0.0,exactAsphericalRot=True,gravity=[0,0,0]), # Here we deactivate gravity
		PotentialBlockVTKRecorder(fileName=vtkSaveDir+'pb', firstIterRun=10, iterPeriod=2000, twoDimension=False, sampleX=10, sampleY=10, sampleZ=10, maxDimension=0.2, REC_INTERACTION=True, REC_COLORS=True, REC_VELOCITY=True, REC_ID=True, label='vtkRecorder')
	]

	# ----------------------------------------------------------------------------------------------------------------------------------------------- #
	# Materials
	O.materials.append(FrictMat(young=-1,poisson=-1,frictionAngle=radians(30.0),density=2000,label='frictional'))

	# ----------------------------------------------------------------------------------------------------------------------------------------------- #
	# Bodies
	edge=0.10
	r=0.001*edge

	# Body pb1=O.bodies[0]: Green
	pb1=Body()
	pb1.aspherical=True
	pb1.shape=PotentialBlock(
	k=0.0, r=r, R=0.0, a=[1, -1, 0, 0, 0, 0], b=[0, 0, 1, -1, 0, 0], c=[0, 0, 0, 0, 1,-1], d=[edge/2.-r]*6,
	id=len(O.bodies), isBoundary=False, color=[0, 1, .2], wire=False, highlight=False, AabbMinMax=True, fixedNormal=False,
	minAabb=1.2*Vector3(edge/2., edge/2., edge/2.), maxAabb=1.2*Vector3(edge/2., edge/2., edge/2.))
	utils._commonBodySetup(pb1,pb1.shape.volume,pb1.shape.inertia, material='frictional', pos=[0,0,0], fixed=False)
	pb1.state.pos = [0,0,0]
	O.bodies.append(pb1)

	# Body pb2=O.bodies[1]: Red
	pb2=Body()
	pb2.aspherical=True
	pb2.shape=PotentialBlock(
	k=0.0, r=r, R=0.0, a=[1, -1, 0, 0, 0, 0], b=[0, 0, 1, -1, 0, 0], c=[0, 0, 0, 0, 1, -1], d=[edge/2.-r]*6,
	id=len(O.bodies), isBoundary=False, color=[1, .2, 0], wire=False, highlight=False, AabbMinMax=True, fixedNormal=False)
	utils._commonBodySetup(pb2,pb2.shape.volume,pb2.shape.inertia, material='frictional', pos=[0,0,0], fixed=False)
	pb2.state.pos = [edge/1.5,0,0]
	O.bodies.append(pb2)

	O.run(20, True);
	checkVTK('pb')
else:
	print("skip PotentialBlockVTKRecorder check, PotentialBlocks or VTK not available")



