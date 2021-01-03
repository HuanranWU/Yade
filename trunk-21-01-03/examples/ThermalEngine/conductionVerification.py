#*************************************************************************
#  Copyright (C) 2019 by Robert Caulk                                    *
#  rob.caulk@gmail.com                                                   *
#                                                                        *
#  This program is free software; it is licensed under the terms of the  *
#  GNU General Public License v2 or later. See file LICENSE for details. *
#*************************************************************************/
#
# Script demonstrating the use of ThermalEngine by comparing conduction
# scheme to analytical solution to Fourier (rod cooling with constant 
# boundary conditions). See details in:
#
# Caulk, R., Scholtes, L., Kraczek, M., Chareyre, B. (In Print) A 
# pore-scale Thermo-Hydro-Mechanical coupled model for particulate systems. 
# Computer Methods in Applied Mechanics and Engineering. Accepted July 2020.
#

from yade import pack
from yade import timing
import numpy as np
import shutil
timeStr = time.strftime('%m-%d-%Y')
num_spheres=1000# number of spheres
young=1e6
rad=0.003

mn,mx=Vector3(0,0,0),Vector3(1.0,0.008,0.008) # corners of the initial packing

thermalCond = 2. #W/(mK)
heatCap = 710. #J(kg K) 
t0 = 400. #K

r = rad
k = 2*2.0*r  # 2*k*r 
Cp = 710.
rho = 2600.
D = 2.*r
m = 4./3.*np.pi*r**2/rho
# macro diffusivity
thermalDiff = 6.*k/(D*np.pi*Cp*rho)

identifier = '-conductionVerification'

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

O.bodies.append(pack.regularOrtho(pack.inAlignedBox(mn,mx),radius=rad,gap=-1e-8,material='spheres'))

print('num bodies ', len(O.bodies))

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
	#triax,
	VTKRecorder(iterPeriod=500,fileName='VTK'+timeStr+identifier+'/spheres-',recorders=['spheres','thermal','intr'],dead=1,label='VTKrec'),
	newton
]

#goal = -1e5
#triax.goal1=triax.goal2=triax.goal3=goal

for b in O.bodies:
	if isinstance(b.shape, Sphere): 
		b.dynamic=False


# we only need flow engine to detect boundaries, there is no flow computed for this 
flow.dead=0

thermal.dead=0
thermal.conduction=True
thermal.thermoMech=False
thermal.advection=False
thermal.fluidThermoMech = False
thermal.solidThermoMech = False
thermal.fluidConduction= False

thermal.bndCondIsTemperature=[1,1,0,0,0,0]
thermal.thermalBndCondValue=[0,0,0,0,0,0]
thermal.tsSafetyFactor=0
thermal.particleDensity=2600
thermal.particleT0=t0
thermal.particleCp=heatCap
thermal.particleK=thermalCond
thermal.particleAlpha =11.6e-3
thermal.useKernMethod=False

timing.reset()
#ThermalEngine.dead=0

flow.updateTriangulation=True
O.dt=1.
O.dynDt=False

flow.emulateAction()
flow.dead=1

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
	print('found closest body ', cBody.id, ' at ', cBody.state.pos)
	return cBody

# solution to the heat equation for constant initial condition , BCs=0, and using series for approx
def analyticalHeatSolution(x,t,u0,L,k):
	ns = np.linspace(1,1000,1000)
	solution = 0
	for i,n in enumerate(ns):
		integral = (-2./L)*u0*L*(np.cos(n*np.pi)-1.) / (n*np.pi)
		solution += integral * np.sin(n*np.pi*x/L)*np.exp((-k*(n*np.pi/L)**2)*t)
	return solution
  
# find 10 bodies along x axis
axis = np.linspace(mn[0], mx[0], num=11)
axisBodies = [None] * len(axis)
axisTrue = np.zeros(len(axis))
for i,x in enumerate(axis):
	axisBodies[i] = bodyByPos(x, mx[1]/2, mx[2]/2)
	axisTrue[i] = axisBodies[i].state.pos[0]
np.savetxt('txt'+timeStr+identifier+'/xdata.txt',axisTrue)
print("Axis length used for analy ", max(axisTrue)-min(axisTrue))
from yade import plot

## a function saving variables
def history():
	plot.addData(
		t=O.time,
		i = O.iter,
		temp1 = axisBodies[0].state.temp,
		temp2 = axisBodies[1].state.temp,
		temp3 = axisBodies[2].state.temp,
		temp4 = axisBodies[3].state.temp,
		temp5 = axisBodies[4].state.temp,
		temp6 = axisBodies[5].state.temp,
		temp7 = axisBodies[6].state.temp,
		temp8 = axisBodies[7].state.temp,
		temp9 = axisBodies[8].state.temp,
		temp10 = axisBodies[9].state.temp,
		temp11 = axisBodies[10].state.temp,
		AnalyTemp1 = analyticalHeatSolution(0,O.time,t0,mx[0],thermalDiff),
		AnalyTemp2 = analyticalHeatSolution(axisBodies[1].state.pos[0],O.time,t0,mx[0],thermalDiff),
		AnalyTemp3 = analyticalHeatSolution(axisBodies[2].state.pos[0]-min(axisTrue),O.time,t0,max(axisTrue)-min(axisTrue),thermalDiff),
		AnalyTemp4 = analyticalHeatSolution(axisBodies[3].state.pos[0]-min(axisTrue),O.time,t0,max(axisTrue)-min(axisTrue),thermalDiff),
		#bndFlux1 = thermal.thermalBndFlux[0],
		#bndFlux2 = thermal.thermalBndFlux[1]
		AnalyTemp5 = analyticalHeatSolution(mx[0],O.time,t0,mx[0],thermalDiff)
)
	plot.saveDataTxt('txt'+timeStr+identifier+'/conductionAnalyticalComparison.txt',vars=('t','i','temp1','temp2','temp3','temp4','temp5','temp6','temp7','temp8','temp9','temp10','temp11'))
  	#plot.addData(e22=-triax.strain[1],t=O.time,s22=-triax.stress(2)[1],p=flow.MeasurePorePressure((0.5,0.5,0.5)))

O.engines=O.engines+[PyRunner(iterPeriod=500,command='history()',label='recorder')]
##make nice animations:
#O.engines=O.engines+[PyRunner(iterPeriod=200,command='flow.saveVtk(withBoundaries=False)')]
VTKrec.dead=0
from yade import plot

plot.plots={'t':(('temp4','k-'),('temp3','r-'),('AnalyTemp4','k--'),('AnalyTemp3','r--'))} #
#plot.plots={'t':(('bndFlux1','k-'),('bndFlux2','r-'))} #
plot.plot()
O.saveTmp()
O.timingEnabled=1
from yade import timing
print("starting oedometer simulation")
O.run(200,1)
timing.stats()
