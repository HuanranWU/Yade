from __future__ import print_function
from yade import plot,pack
"""
A simple script of a Brazilian splitting test.
A cylinder with x axis form 0 to specimenLength.
Load in z direction by z-perpendicular walls.

Code strongly inspired by uniax.py
"""

# default parameters or from table
readParamsFromTable(noTableOk=True, # unknownOk=True,
	young = 24e9,
	poisson = .2,

	sigmaT = 3.5e6,
	frictionAngle = atan(0.8),
	epsCrackOnset = 1e-4,
	relDuctility = 30,

	intRadius = 1.5,
	dtSafety = .8,
	strainRate = 1,

	specimenLength = .15,
	specimenRadius = .05,
	sphereRadius = 3.5e-3,
)
from yade.params.table import *

# material
concreteId = O.materials.append(CpmMat(
	young = young,
	poisson = poisson,
	frictionAngle = frictionAngle,
	epsCrackOnset = epsCrackOnset,
	relDuctility = relDuctility,
	sigmaT = sigmaT,
))

# spheres
sp=pack.randomDensePack(
	pack.inCylinder((0,0,0),(specimenLength,0,0),specimenRadius),
	radius = sphereRadius,
	spheresInCell = 500,
	memoizeDb = '/tmp/packing-brazilian.db',
	returnSpherePack = True
)
sp.toSimulation()

# walls
zMin,zMax=[pt[2] for pt in aabbExtrema()]
wallIDs = O.bodies.append([wall((0,0,z),2) for z in (zMin,zMax)])
walls = wallMin,wallMax = [O.bodies[i] for i in wallIDs]
v = strainRate * 2*specimenRadius
wallMin.state.vel = (0,0,+v)
wallMax.state.vel = (0,0,-v)

# engines
O.engines=[
	ForceResetter(),
	InsertionSortCollider([Bo1_Sphere_Aabb(aabbEnlargeFactor=intRadius,label='is2aabb'),Bo1_Wall_Aabb()]),
	InteractionLoop(
		[Ig2_Sphere_Sphere_ScGeom(interactionDetectionFactor=intRadius,label='ss2sc'),Ig2_Wall_Sphere_ScGeom()],
		[Ip2_CpmMat_CpmMat_CpmPhys()],
		[Law2_ScGeom_CpmPhys_Cpm()],
	),
	NewtonIntegrator(damping=.2),
	CpmStateUpdater(realPeriod=.5),
	PyRunner(iterPeriod=100,command='addPlotData()',initRun=True),
	PyRunner(iterPeriod=100,command='stopIfDamaged()'),
]

# stop condition
def stopIfDamaged():
	if O.iter < 1000: # do nothing at the beginning
		return
	fMax = max(plot.data["f"])
	f = plot.data["f"][-1]
	if f/fMax < .6:
		print("Damaged, stopping.")
		print("ft = ",max(plot.data["stress"]))
		O.pause()

# plot stuff
def addPlotData():
	# forces of walls. f1 is "down", f2 is "up" (f1 needs to be negated for evlauation)
	f1,f2 = [O.forces.f(i)[2] for i in wallIDs]
	f1 *= -1
	# average force
	f = .5*(f1+f2)
	# displacement (2 times each wall)
	wall = O.bodies[wallIDs[0]]
	dspl = 2*wall.state.displ()[2]
	# stress (according to standard brazilian test evaluation formula)
	stress = f/(pi*specimenRadius*specimenLength)
	# store values
	yade.plot.addData(
		t = O.time,
		i = O.iter,
		dspl = dspl,
		f1 = f1,
		f2 = f2,
		f = f,
		stress = stress,
	)
# plot dspl on x axis, stress on y1 axis and f,f1,f2 in y2 axis
plot.plots={'dspl':('stress',None,'f1','f2','f')}

O.dt = 0.
O.step(); # to create initial contacts
# now reset the interaction radius and go ahead
ss2sc.interactionDetectionFactor=1.
is2aabb.aabbEnlargeFactor=1.

# time step
O.dt = dtSafety*PWaveTimeStep()

# run simulation
plot.plot()
#O.run()
