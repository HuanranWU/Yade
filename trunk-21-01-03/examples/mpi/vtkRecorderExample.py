# Possible executions of this script
# ./yadempi script.py #interactive will spawn 3 additional workers
# mpiexec -n 4 ./yadempi script.py #non interactive

NSTEPS=5000 #turn it >0 to see time iterations, else only initilization TODO!HACK
#NSTEPS=50 #turn it >0 to see time iterations, else only initilization

import os
from yade import mpy as mp

#import yade.log
#yade.log.setLevel('VTKRecorderParallel', 6)
#numThreads = 4

#materials 
young = 5e6
compFricDegree = 0.0
O.materials.append(FrictMat(young=young, poisson=0.5, frictionAngle = radians(compFricDegree), density= 2600, label='sphereMat'))
O.materials.append(FrictMat(young=young*100, poisson = 0.5, frictionAngle = compFricDegree, density =2600, label='wallMat'))


#add spheres
mn,mx=Vector3(0,0,0),Vector3(90,180,90)
pred = pack.inAlignedBox(mn,mx)
O.bodies.append(pack.regularHexa(pred,radius=2.80,gap=0, material='sphereMat'))

#walls 
wallIds=aabbWalls([Vector3(-360,-1,-360),Vector3(360,360,360)],thickness=10.0, material='wallMat')
O.bodies.append(wallIds)

#engines 
O.engines=[
	ForceResetter(),
	InsertionSortCollider([
		Bo1_Sphere_Aabb(),
		Bo1_Box_Aabb()], label = 'collider'), # always add labels. 
	InteractionLoop(
		[Ig2_Sphere_Sphere_ScGeom(),Ig2_Box_Sphere_ScGeom()],
		[Ip2_FrictMat_FrictMat_FrictPhys()],
		[Law2_ScGeom_FrictPhys_CundallStrack()], 
		label="interactionLoop"
	),
	GlobalStiffnessTimeStepper(timestepSafetyCoefficient=0.3,  timeStepUpdateInterval=100, parallelMode=True, label = 'timeStepper'), 
	NewtonIntegrator(damping=0.1,gravity = (0, -0.1, 0), label='newton'), 
	VTKRecorder(fileName='spheres/3d-vtk-', recorders=['spheres', 'intr', 'boxes'], parallelMode=True,iterPeriod=500), #use .pvtu to open spheres, .pvtp for ints, and .vtu for boxes.
]


collider.verletDist = 1.5

#########  RUN  ##########
# customize mpy
mp.ERASE_REMOTE_MASTER = True
mp.USE_CPP_REALLOC = True
mp.USE_CPP_INTERS = True 
mp.DOMAIN_DECOMPOSITION= True
mp.mpirun(NSTEPS)
mp.mergeScene() 
if mp.rank == 0: O.save('mergedScene.yade')

#demonstrate getting stuff from workers
if mp.rank==0:
	print("kinetic energy from workers: "+str(mp.sendCommand([1,2],"kineticEnergy()",True)))
