#*************************************************************************
#  Copyright (C) 2019 by Robert Caulk                                    *
#  rob.caulk@gmail.com                                                   *
#                                                                        *
#  This program is free software; it is licensed under the terms of the  *
#  GNU General Public License v2 or later. See file LICENSE for details. *
#*************************************************************************/
#
# Script demonstrating the use of full Thermo-Hydro-Mechanical coupling
# in ThermalEngine. Methods published in: 
#Robert Caulk, Luc Scholtès, Marek Krzaczek, Bruno Chareyre,
#A pore-scale thermo–hydro-mechanical model for particulate systems,
#Computer Methods in Applied Mechanics and Engineering,
#Volume 372,
#2020,
#113292,
#ISSN 0045-7825,
#https://doi.org/10.1016/j.cma.2020.113292.
#http://www.sciencedirect.com/science/article/pii/S0045782520304771

from yade import pack, ymport, plot, utils, export, timing
import numpy as np

young=5e6

mn,mx=Vector3(0,0,0),Vector3(0.05,0.05,0.05)

identifier = '-thm_coupling'

if not os.path.exists('VTK'+identifier):
	os.mkdir('VTK'+identifier)

if not os.path.exists('txt'+identifier):
	os.mkdir('txt'+identifier)


O.materials.append(FrictMat(young=young*100,poisson=0.5,frictionAngle=0,density=2600e10,label='walls'))
O.materials.append(FrictMat(young=young,poisson=0.5,frictionAngle=radians(30),density=2600e10,label='spheres'))


walls=aabbWalls([mn,mx],thickness=0,material='walls')
wallIds=O.bodies.append(walls)

sp=pack.SpherePack()
sp.makeCloud(mn,mx,rMean=0.0015,rRelFuzz=0.333,num=100,seed=1) 
sp.toSimulation(color=(0.752, 0.752, 0.752),material='spheres')

triax=TriaxialStressController(
 maxMultiplier=1.+2e4/young,
 finalMaxMultiplier=1.+2e3/young,
 thickness = 0,
 stressMask = 7,
 internalCompaction=True,
)

O.engines=[
 ForceResetter(),
 InsertionSortCollider([Bo1_Sphere_Aabb(aabbEnlargeFactor=1,label='is2aabb'),Bo1_Box_Aabb()]),
 InteractionLoop(
  [Ig2_Sphere_Sphere_ScGeom(interactionDetectionFactor=1,label='ss2sc'),Ig2_Box_Sphere_ScGeom()],
  [Ip2_FrictMat_FrictMat_FrictPhys()],
  [Law2_ScGeom_FrictPhys_CundallStrack()],label="iloop"
 ),
 GlobalStiffnessTimeStepper(active=1,timeStepUpdateInterval=100,timestepSafetyCoefficient=0.5),
 triax,
 FlowEngine(dead=1,label="flow",multithread=False),
 ThermalEngine(dead=1,label='thermal'),
 VTKRecorder(iterPeriod=500,fileName='VTK'+identifier+'/spheres-',recorders=['spheres','thermal','intr'],dead=1,label='VTKrec'),
 NewtonIntegrator(damping=0.5)
]

O.step()
ss2sc.interactionDetectionFactor=-1
is2aabb.aabbEnlargeFactor=-1

tri_pressure = 1000
triax.goal1=triax.goal2=triax.goal3=-tri_pressure
triax.stressMask=7
while 1:
  O.run(1000, True)
  unb=unbalancedForce()
  print('unbalanced force:',unb,' mean stress: ',triax.meanStress)
  if unb<0.1 and abs(-tri_pressure-triax.meanStress)/tri_pressure<0.001:
    break

triax.internalCompaction=False

flow.debug=False
# add flow
flow.permeabilityMap = False
flow.pZero = 10
flow.meshUpdateInterval=2
flow.fluidBulkModulus=2.2e9
flow.useSolver=4
flow.permeabilityFactor=-1e-5
flow.viscosity= 0.001
flow.decoupleForces =  False
flow.bndCondIsPressure=[0,0,0,0,1,1]
flow.bndCondValue=[0,0,0,0,10,10]

## Thermal Stuff
flow.bndCondIsTemperature=[0,0,0,0,0,0] 
flow.thermalEngine=True
flow.thermalBndCondValue=[0,0,0,0,0,0] 
flow.tZero=25

flow.dead=0
thermal.dead=1

thermal.conduction=True
thermal.fluidConduction=True
thermal.debug=0
thermal.thermoMech=True
thermal.solidThermoMech = True
thermal.fluidThermoMech = True
thermal.advection=True
thermal.useKernMethod=False
thermal.bndCondIsTemperature=[0,0,0,0,0,1]
thermal.thermalBndCondValue=[0,0,0,0,0,45]
thermal.fluidK = 0.650
thermal.fluidBeta = 2e-5 # 0.0002
thermal.particleT0 = 25
thermal.particleK = 2.0
thermal.particleCp = 710
thermal.particleAlpha = 3.0e-5
thermal.particleDensity = 2700
thermal.tsSafetyFactor = 0 #0.01
thermal.uniformReynolds =10
thermal.minimumThermalCondDist=0


timing.reset()
O.dt=1e-4
O.dynDt=False
thermal.dead=0
flow.emulateAction()

def bodyByPos(x,y,z):
 cBody = O.bodies[1]
 cDist = Vector3(100,100,100)
 for b in O.bodies:
  if isinstance(b.shape, Sphere):
   dist = b.state.pos - Vector3(x,y,z)
   if np.linalg.norm(dist) < np.linalg.norm(cDist):
    cDist = dist
    cBody = b
 return cBody

bodyOfInterest = bodyByPos(0.025,0.025,0.025)

from yade import plot

def history():
 plot.addData(
  ftemp1=flow.getPoreTemperature((0.025,0.025,0.025)),
  p=flow.getPorePressure((0.025,0.025,0.025)),
  t=O.time,
  i = O.iter,
  bodyOfIntTemp = O.bodies[bodyOfInterest.id].state.temp,
)
 #plot.saveDataTxt('Record.txt',vars=('t','i','p','ftemp1','bodyOfIntTemp'))

O.engines=O.engines+[PyRunner(iterPeriod=200,command='history()',label='recorder')]

def pressureField():
	flow.saveVtk('VTK'+identifier+'/',withBoundaries=False)

O.engines=O.engines+[PyRunner(iterPeriod=2000,command='pressureField()')]

plot.plots={'t':(('ftemp1','k-'),('bodyOfIntTemp','r-'))} 

plot.plot(subPlots=False)
O.run()
