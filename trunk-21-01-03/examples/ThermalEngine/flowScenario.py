#*************************************************************************
#  Copyright (C) 2019 by Robert Caulk                                    *
#  rob.caulk@gmail.com                                                   *
#                                                                        *
#  This program is free software; it is licensed under the terms of the  *
#  GNU General Public License v2 or later. See file LICENSE for details. *
#*************************************************************************/
#
# Script demonstrating the use of ThermalEngine by permeating warm fluid
# through a cold packing. Also serves as a validation script for comparison
# with ANSYS CFD. See details in 
# Caulk, R., Scholtes, L., Kraczek, M., Chareyre, B. (In Print) A 
# pore-scale Thermo-Hydro-Mechanical coupled model for particulate systems.
# Computer Methods in Applied Mechanics and Engineering. Accepted July 2020.
#
# note: warnings for inifiniteK and Reynolds numbers = nan for boundary
# cells in regular packings are expected. It does not interfere with the 
# physics


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

identifier = '-flowScenario'

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
	FlowEngine(dead=1,label="flow",multithread=False),
	ThermalEngine,	GlobalStiffnessTimeStepper(active=1,timeStepUpdateInterval=100,timestepSafetyCoefficient=0.8),
	triax,
	VTKRecorder(iterPeriod=500,fileName='VTK'+timeStr+identifier+'/spheres-',recorders=['spheres','thermal','intr'],dead=1,label='VTKrec'),
	newton
]

#goal = -1e5
#triax.goal1=triax.goal2=triax.goal3=goal

for b in O.bodies:
	if isinstance(b.shape, Sphere): 
		b.dynamic=False # mechanically static

flow.dead=0
flow.defTolerance=-1 #0.3
flow.meshUpdateInterval=-1
flow.useSolver=4
flow.permeabilityFactor= 1
flow.viscosity= 0.001
flow.bndCondIsPressure=[1,1,0,0,0,0]
flow.bndCondValue=[10,0,0,0,0,0]
flow.thermalEngine=True
flow.debug=False
flow.fluidRho = 997
flow.fluidCp = 4181.7
flow.getCHOLMODPerfTimings=True
flow.bndCondIsTemperature=[1,0,0,0,0,0]
flow.thermalEngine=True
flow.thermalBndCondValue=[343.15,0,0,0,0,0]
flow.tZero=t0
flow.pZero=0
flow.maxKdivKmean=1
flow.minKdivmean=0.0001;


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
thermal.fluidK = 0.6069 #0.650
thermal.fluidConductionAreaFactor=1.
thermal.particleT0 = t0
thermal.particleDensity=2600.
thermal.particleK = thermalCond
thermal.particleCp = heatCap
thermal.useKernMethod=True
#thermal.useHertzMethod=False
timing.reset()

O.dt=0.1e-3
O.dynDt=False

O.run(1,1)
flow.dead=0

def bodyByPos(x,y,z):
	cBody = O.bodies[1]
	cDist = Vector3(100,100,100)
	for b in O.bodies:
		if isinstance(b.shape, Sphere):
			dist = b.state.pos - Vector3(x,y,z)
			if np.linalg.norm(dist) < np.linalg.norm(cDist):
				cDist = dist
				cBody = b
	print('found closest body ', cBody.id, ' at ', cBody.state.pos)
	return cBody

#bodyOfInterest = bodyByPos(15.998e-3,0.0230911,19.5934e-3)
bodyOfInterest = bodyByPos(0.025,0.025,0.025)

# find 10 bodies along x axis
axis = np.linspace(mn[0], mx[0], num=5)
axisBodies = [None] * len(axis)
axisTrue = np.zeros(len(axis))
for i,x in enumerate(axis):
	axisBodies[i] = bodyByPos(x, mx[1]/2, mx[2]/2)
	axisTrue[i] = axisBodies[i].state.pos[0]

print("found body of interest at", bodyOfInterest.state.pos)

from yade import plot

## a function saving variables
def history():
	plot.addData(
		ftemp1=flow.getPoreTemperature((0.024,0.023,0.02545)),
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

def pressureField():
	flow.saveVtk('VTK'+timeStr+identifier+'/',withBoundaries=False)
O.engines=O.engines+[PyRunner(iterPeriod=2000,command='pressureField()')]

def endFlux():
	if O.time >= 30:
		flux = 0
		n=utils.porosity()
		for i in flow.getBoundaryVel(1):
			flux +=i[0]*i[3]/n  # area * velocity / porosity  (dividing by porosity because flow engine is computing the darcy velocity)
		massFlux = flux * 997

		K = abs(flow.getBoundaryFlux(1))*(flow.viscosity*0.5)/(0.5**2.*(10.-0))
		d=8e-3 # sphere diameter
		Kc = d**2/180. * (n**3.)/(1.-n)**2

		print('Permeability', K, 'kozeny', Kc)
		print('outlet flux(with vels only):',massFlux,'compared to CFD = 0.004724 kg/s')
		print('sim paused')
		O.pause()

O.engines=O.engines+[PyRunner(iterPeriod=10,command='endFlux()')]
VTKrec.dead=0
from yade import plot

plot.plots={'t':(('ftemp1','k-'),('bodyOfIntTemp','r-'))} #
plot.plot()
O.saveTmp()

print("starting thermal sim")
O.run()

