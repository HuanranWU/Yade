#*************************************************************************
#  Copyright (C) 2019 by Robert Caulk                                    *
#  rob.caulk@gmail.com                                                   *
#                                                                        *
#  This program is free software; it is licensed under the terms of the  *
#  GNU General Public License v2 or later. See file LICENSE for details. *
#*************************************************************************/
#
# Script demonstrating the use of ThermalEngine by monitoring still fluid
# temperature changes in a sphere packing.
# Also serves as a validation script for comparison
# with ANSYS CFD. See details in 
# Caulk, R., Scholtes, L., Kraczek, M., Chareyre, B. (In Print) A 
# pore-scale Thermo-Hydro-Mechanical coupled model for particulate systems.
# Computer Methods in Applied Mechanics and Engineering. Accepted July 2020.
#
# note: warnings for inifiniteK and Reynolds numbers = nan for boundary
# cells in regular packings are expected. It does not interfere with the 
# physics, these are warnings NOT errors. 

from yade import pack, ymport
from yade import timing
import numpy as np
import shutil
timeStr = time.strftime('%m-%d-%Y')
num_spheres=1000# number of spheres
young=1e9
rad=0.003

mn,mx=Vector3(0,0,0),Vector3(0.05,0.05,0.05) # corners of the initial packing

thermalCond = 2. #W/(mK)
heatCap = 710. #J(kg K) 
t0 = 333.15 #K

# micro properties
r = rad
k = 2.0   # 2*k*r 
Cp = 710.
rho = 2600.
D = 2.*r
m = 4./3.*np.pi*r**2/rho
# macro diffusivity

identifier = '-noFlowScenario'

if not os.path.exists('VTK'+timeStr+identifier):
	os.mkdir('VTK'+timeStr+identifier)
else:
	shutil.rmtree('VTK'+timeStr+identifier)
	os.mkdir('VTK'+timeStr+identifier)

if not os.path.exists('txt'+timeStr+identifier):
	os.mkdir('txt'+timeStr+identifier)
else:
	shutil.rmtree('txt'+timeStr+identifier)
	os.mkdir('txt'+timeStr+identifier)

shutil.copyfile(sys.argv[0],'txt'+timeStr+identifier+'/'+sys.argv[0])

O.materials.append(FrictMat(young=young,poisson=0.5,frictionAngle=radians(3),density=2600,label='spheres'))
O.materials.append(FrictMat(young=young,poisson=0.5,frictionAngle=0,density=0,label='walls'))
walls=aabbWalls([mn,mx],thickness=0,material='walls')
wallIds=O.bodies.append(walls)

sp = O.bodies.append(ymport.textExt('5cmEdge_1mm.spheres', 'x_y_z_r',color=(0.1,0.1,0.9), material='spheres'))

print('num bodies ', len(O.bodies))

triax=TriaxialStressController(
	maxMultiplier=1.+2e4/young, 
	finalMaxMultiplier=1.+2e3/young, 
	thickness = 0,
	stressMask = 7,
	internalCompaction=True,
)

ThermalEngine = ThermalEngine(dead=1,label='thermal');

newton=NewtonIntegrator(damping=0.2)
intRadius = 1
O.engines=[
	ForceResetter(),
	InsertionSortCollider([Bo1_Sphere_Aabb(aabbEnlargeFactor=intRadius),Bo1_Box_Aabb()]),
	InteractionLoop(
		[Ig2_Sphere_Sphere_ScGeom(interactionDetectionFactor=intRadius),Ig2_Box_Sphere_ScGeom()],
		[Ip2_FrictMat_FrictMat_FrictPhys()],
		[Law2_ScGeom_FrictPhys_CundallStrack()],label="iloop"
	),
	FlowEngine(dead=1,label="flow",multithread=False),#introduced as a dead engine for the moment, see 2nd section
	ThermalEngine,	GlobalStiffnessTimeStepper(active=1,timeStepUpdateInterval=100,timestepSafetyCoefficient=0.8),
	triax,
	VTKRecorder(iterPeriod=2000,fileName='VTK'+timeStr+identifier+'/spheres-',recorders=['spheres','thermal','intr'],dead=1,label='VTKrec'),
	newton
]

for b in O.bodies:
	if isinstance(b.shape, Sphere): 
		b.dynamic=False # mechanically static


flow.dead=0
flow.defTolerance=-1 
flow.meshUpdateInterval=-1
flow.useSolver=4
flow.permeabilityFactor= 1
flow.viscosity= 0.001 
flow.bndCondIsPressure=[0,0,0,0,0,0]
flow.bndCondValue=[0,0,0,0,0,0]
flow.thermalEngine=True
flow.debug=False
flow.fluidRho = 997
flow.fluidCp = 4181.7 
flow.bndCondIsTemperature=[0,0,0,0,0,0]
flow.thermalEngine=True
flow.thermalBndCondValue=[0,0,0,0,0,0]
flow.tZero=343.15
flow.pZero=0

thermal.dead=0
thermal.debug=False
thermal.fluidConduction=True
thermal.ignoreFictiousConduction=True
thermal.conduction=True
thermal.thermoMech=False
thermal.solidThermoMech = False
thermal.fluidThermoMech = False
thermal.advection=True
thermal.bndCondIsTemperature=[0,0,0,0,0,0]
thermal.thermalBndCondValue=[0,0,0,0,0,0]
thermal.fluidK = 0.6069 
thermal.fluidConductionAreaFactor=1.
thermal.uniformReynolds=10 # using an extremely low reynolds number to maintain a heat transfer coefficient
thermal.particleT0 = 333.15
thermal.particleDensity=2600.
thermal.particleK = 2. #thermalCond
thermal.particleCp = 710.
thermal.tsSafetyFactor=0
thermal.useKernMethod=True
thermal.useHertzMethod=False
timing.reset()
#ThermalEngine.dead=0

O.dt=0.1e-4
O.dynDt=False


O.run(1,1)
flow.dead=0

#triax.goal2=-11000

def bodyByPos(x,y,z):
	cBody = O.bodies[1]
	cDist = Vector3(100,100,100)
	for b in O.bodies:
		if isinstance(b.shape, Sphere):
			dist = b.state.pos - Vector3(x,y,z)
			if np.linalg.norm(dist) < np.linalg.norm(cDist):
				cDist = dist
				cBody = b
#	print 'found closest body ', cBody.id, ' at ', cBody.state.pos
	return cBody

#bodyOfInterest = bodyByPos(15.998e-3,0.0230911,19.5934e-3)
bodyOfInterest = bodyByPos(0.025,0.025,0.025)

#print "found body of interest at", bodyOfInterest.state.pos
  
# find 10 bodies along x axis
axis = np.linspace(mn[0], mx[0], num=5)
axisBodies = [None] * len(axis)
axisTrue = np.zeros(len(axis))
for i,x in enumerate(axis):
	axisBodies[i] = bodyByPos(x, mx[1]/2, mx[2]/2)
	axisTrue[i] = axisBodies[i].state.pos[0]

from yade import plot

def history():
	plot.addData(
		ftemp1=flow.getPoreTemperature((0.025,0.025,0.025)),
		#ftemp2=flow.getPoreTemperature((0.5,0.1,0.5)),
		#ftemp3=flow.getPoreTemperature((0.5,0.9,0.5)),
		p=flow.getPorePressure((0.025,0.025,0.025)),
		t=O.time,
		i = O.iter,
		temp1 = axisBodies[0].state.temp,
		temp2 = axisBodies[1].state.temp,
		temp3 = axisBodies[2].state.temp,
		temp4 = axisBodies[3].state.temp,
		temp5 = axisBodies[4].state.temp,
		bodyOfIntTemp = O.bodies[bodyOfInterest.id].state.temp

)
	plot.saveDataTxt('txt'+timeStr+identifier+'/temps'+identifier+'.txt',vars=('t','i','p','ftemp1', 'temp1','temp2','temp3','bodyOfIntTemp'))

O.engines=O.engines+[PyRunner(iterPeriod=500,command='history()',label='recorder')]
##make nice animations:
def pressureField():
	flow.saveVtk('VTK'+timeStr+identifier+'/',withBoundaries=False)
O.engines=O.engines+[PyRunner(iterPeriod=2000,command='pressureField()')]

def endFlux():
	if O.time >= 30:
		O.pause()
O.engines=O.engines+[PyRunner(iterPeriod=10,command='endFlux()')]
VTKrec.dead=0
from yade import plot

plot.plots={'t':(('ftemp1','k-'),('bodyOfIntTemp','r-'))} #
plot.plot()
O.saveTmp()

#print "starting thermal sim"
O.run()

