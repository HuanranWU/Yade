# encoding: utf-8
from __future__ import print_function

from yade import pack,export,plot
import math,os,sys
print('checkColliderConstantness for InsertionSortCollider')

#### This is useful for printing the linenumber in the script
# import inspect
# print(inspect.currentframe().f_lineno)

if((opts.threads != None and opts.threads != 1) or (opts.cores != None and opts.cores != '1')):
	raise YadeCheckError("This test will only work on single core, because it must be fully reproducible, but -j "+str(opts.threads)+" or --cores "+str(opts.cores)+" is used.")

from yade import pack

# I had a third O.run( 500, True); and so there was
# [None,None,None] below, but I decided that it is too much testing.
results={True:[None,None],False:[None,None]}

#checksPath="." # this line was used for working on this script locally.

for usePeriod in [True,False]:
	O.periodic=usePeriod
	length=1.0
	height=1.0
	width=1.0
	thickness=0.1
	
	if(usePeriod):
		O.cell.hSize=Matrix3(length, 0, 0, 0 ,3.*height, 0, 0, 0, width)
	
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
		InsertionSortCollider([Bo1_Box_Aabb(),Bo1_Sphere_Aabb()],allowBiggerThanPeriod=True,targetInterv=50),
		InteractionLoop(
			[Ig2_Sphere_Sphere_ScGeom(),Ig2_Box_Sphere_ScGeom()],
			[Ip2_FrictMat_FrictMat_FrictPhys()],
			[Law2_ScGeom_FrictPhys_CundallStrack()]
		),
		newton
	]
	
	testedCollider=typedEngine("InsertionSortCollider")
	
	O.run( 500, True); results[usePeriod][0]=testedCollider.dumpBounds()
	O.run(1000, True); results[usePeriod][1]=testedCollider.dumpBounds()
	#O.run( 500, True); results[usePeriod][2]=testedCollider.dumpBounds()
	O.reset()


#### these text files have too high precision, and get too big. I think that 8 decimal places should be good to avoid any numerical errors arising on different architectures.
# textFile=open("Output123___n.txt", "w");textFile.write(str([results[False][0],results[False][1],results[False][2]]));textFile.close()
# textFile=open("Output123___p.txt", "w");textFile.write(str([results[True ][0],results[True ][1],results[True ][2]]));textFile.close()

resultFile=None
# careful, I used this loop to save the reference results in git revision 2bc5ac90b. When doing tests it must be readonly, and loading=True
loading=True
if(loading):
	resultFile=open( checksPath+'/data/checkColider.txt', "r" )
else:
	resultFile=open( checksPath+'/data/checkColider.txt', "w" )
lineCount=0
for per in sorted(results):
	for result in results[per]:
		for record in result:
			for tupl in record:
				# contents of this tuple is explained in file InsertionSortCollider.cpp line 518, function boost::python::tuple InsertionSortCollider::dumpBounds();
				for number in tupl:
					lineCount+=1
					if(loading):
						line = resultFile.readline()
						tmp = float(line)
						if(abs(tmp - number) > 1e-8):
							if(yade.math.needsMpmathAtN(1) and (type(number) == int) and (abs(tmp - number)==100)):
								# with high precision sometimes the periodic spheres are sorted a little differently (their positions are equal since that's the same sphere with different period):
								# sphere id numbers -88 and -188 or -55 and -155 are swapped.
								pass
							else:
								raise YadeCheckError("InsertionSortCollider check failed in file scripts/checks-and-tests/checks/data/checkColider.txt line: %d"%lineCount)
					else:
						if(type(number) == int):
							resultFile.write(str(number)+'\n')
						else:
							resultFile.write("%.8f"%number+'\n')


