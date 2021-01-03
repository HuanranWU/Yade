#*************************************************************************
#  Copyright (C) 2019 by Robert Caulk                                    *
#  rob.caulk@gmail.com                                                   *
#                                                                        *
#  This program is free software; it is licensed under the terms of the  *
#  GNU General Public License v2 or later. See file LICENSE for details. *
#*************************************************************************/
#
# Script demonstrating the three point bending test used to generate
# acoustic emissions during tensile fracture in the following paper:
# Caulk, Robert A. Modeling acoustic emissions in heterogeneous rocks 
# during tensile fracture with the Discrete Element Method.
# Open Geomechanics, Volume 2 (2020) , article no. 2, 19 p. doi : 
# 10.5802/ogeo.5. 
# https://opengeomechanics.centre-mersenne.org/item/OGEO_2020__2__A2_0/


from yade import ymport, utils, pack, export
from yade import plot
from pylab import *
import os
import math
import numpy as np
import numpy.linalg as la
import time
timeStr = time.strftime('%m-%d-%Y')

readParamsFromTable(noTableOk=True,
	conf = 0,
	weibullCutOffMax=10,
	weibullCutOffMin=0.1,
	xSectionShape = 4,
	xSectionScale = 0,

)

from yade.params.table import *

mn,mx=Vector3(-0.04, -.140,-0.020),Vector3(0.04, .140, 0.020)

young = 30e9
poisson = 0.3
targetPorosity = 0.55
density = 5000
relDuctility=30
finalFricDegree = 19 
intRadius= 1.329
sigmaT = 10e6
iterper=1000
cohesion=40e6
dispVel = -0.02 # m/s
highFric = 45

identifier='shape-'+str(xSectionShape)+'-'+timeStr
outputDir='out_'+identifier
if not os.path.exists(outputDir):
	os.mkdir(outputDir)
if not os.path.exists('txt'):
	os.mkdir('txt')
	
output = './'+outputDir+'/'+identifier

wallMat = O.materials.append(FrictMat(young=80e9,poisson=.45,frictionAngle=radians(highFric),density=7000,label='frictionlessWalls'))

JCFmat = O.materials.append(JCFpmMat(young=young, cohesion=cohesion, density=density, frictionAngle=radians(finalFricDegree), tensileStrength=sigmaT, poisson=poisson, label='JCFmat'))

#### preprocessing to get dimensions
numSpheres=10000

pred = pack.inAlignedBox(mn, mx) #- pack.inCylinder((0, -.01, 0), (0, .01, 0), 0.04)
sp = pack.randomDensePack(pred, radius=0.0015, spheresInCell=numSpheres, rRelFuzz=0.25, returnSpherePack=True)
sp.toSimulation()
export.textExt('240x80mmBeam_1.5mmrad.spheres','x_y_z_r')

dim=utils.aabbExtrema()
xinf=dim[0][0]
xsup=dim[1][0]
X=xsup-xinf
yinf=dim[0][1]
ysup=dim[1][1]
Y=ysup-yinf
zinf=dim[0][2]
zsup=dim[1][2]
Z=zsup-zinf

r=X/15.

O.reset() 

wallMat = O.materials.append(FrictMat(young=80e9,poisson=.45,frictionAngle=radians(highFric),density=7000,label='frictionlessWalls'))

# Rigid blocks
block1 = O.bodies.append(geom.facetCylinder(center=(xinf-0.971*r,yinf+2*r,0),radius=r,height=Z,orientation=Quaternion((0, 0, 0), 90),segmentsNumber=20,wire=False,material=wallMat)) 

block2 = O.bodies.append(geom.facetCylinder(center=(xinf-0.915*r,ysup-2*r,0),radius=r,height=Z,orientation=Quaternion((0, 0, 0), 90),segmentsNumber=20,wire=False,material=wallMat)) 

# Loading piston
piston=O.bodies.append(geom.facetCylinder(center=(xsup+0.943*r,0,0),radius=r,height=Z,dynamic=False,orientation=Quaternion((1,0, 0),0),segmentsNumber=20,wire=False,material=wallMat))

p0=O.bodies[piston[0]].state.pos[1]

JCFmat = O.materials.append(JCFpmMat(young=young, cohesion=cohesion, density=density, frictionAngle=radians(finalFricDegree), tensileStrength=sigmaT, poisson=poisson, label='JCFmat'))

# Specimen
beam = O.bodies.append(ymport.textExt('240x80mmBeam_1.5mmrad.spheres', 'x_y_z_r',color=(0,0.2,0.7), material='JCFmat'))

AEfile = 'AEcounts_'+identifier+'.txt'

f = open('txt/'+AEfile, 'w')
f.write('time iteration AEcount P deflection pistonDisp\n')
f.close

# remove rigid block DOFs
for i in block1:
	O.bodies[i].state.blockedDOFs='xyzXYZ'	

for i in block2:
	O.bodies[i].state.blockedDOFs='xyzXYZ'

# set engine list
O.engines=[
        ForceResetter(),
        InsertionSortCollider([Bo1_Box_Aabb(),Bo1_Sphere_Aabb(aabbEnlargeFactor=intRadius,label='Saabb'),Bo1_Facet_Aabb()]),
 InteractionLoop(
  [Ig2_Sphere_Sphere_ScGeom(interactionDetectionFactor=intRadius,label='SSgeom'),Ig2_Facet_Sphere_ScGeom()],
  [Ip2_FrictMat_FrictMat_FrictPhys(),Ip2_JCFpmMat_JCFpmMat_JCFpmPhys(cohesiveTresholdIteration=10, label='jcf', xSectionWeibullShapeParameter=xSectionShape,weibullCutOffMin=weibullCutOffMin, weibullCutOffMax=weibullCutOffMax)],
  [Law2_ScGeom_JCFpmPhys_JointedCohesiveFrictionalPM(recordCracks=True, recordMoments=True,Key=identifier,label='interactionLaw'),Law2_ScGeom_FrictPhys_CundallStrack()]
 ),

        GlobalStiffnessTimeStepper(active=1,timeStepUpdateInterval=100,timestepSafetyCoefficient=0.4, defaultDt=0.1*utils.PWaveTimeStep()),
	TranslationEngine(ids=piston,translationAxis=(1,0,0), velocity=dispVel, label='pistonEngine'),
	TranslationEngine(ids=block1,translationAxis=(1,0,0), velocity=0, label='block1Engine'),
	TranslationEngine(ids=block2,translationAxis=(1,0,0), velocity=0, label='block2Engine'),
        VTKRecorder(dead=0,iterPeriod=iterper*2,initRun=True,fileName=(output+'-'),recorders=['jcfpm','cracks','facets','moments','intr'],Key=identifier,label='vtk'),
	NewtonIntegrator(damping=0.2)
]

# options for AE model
interactionLaw.momentRadiusFactor=3
interactionLaw.clusterMoments=True
interactionLaw.neverErase=True
interactionLaw.useIncrementalForm=True
interactionLaw.useStrainEnergy = True

O.dt = 0.004 * utils.PWaveTimeStep()

# collect data for active plotting and post processing
from yade import plot
O.engines=O.engines[0:8]+[PyRunner(dead=0,iterPeriod=int(iterper/4),command='stressStrainHist()',label='dataCollector')]+O.engines[8:9]

O.engines=O.engines[0:9]+[PyRunner(dead=0,iterPeriod=iterper,command='stopifDamaged()',label='damageCheck')]+O.engines[9:10]

# engage blocks to ensure force balance and remove dynamics of system
O.engines=O.engines[0:10]+[PyRunner(dead=0, iterPeriod=1,command='ensureBlockEngagement()',label='blockEngagement')]+O.engines[10:11]

def ensureBlockEngagement():
	Pblock1, Pblock2 = checkBlockEngagement()
	P = getPistonForce()
	maxVel = 0.01
	if Pblock1 != P/2.:
		multiplier = (P/2. - Pblock1)/(P/2.)
		block1Engine.velocity = multiplier*maxVel
	if Pblock2 != P/2.:
		multiplier = (P/2. - Pblock2)/(P/2.)
		block2Engine.velocity = multiplier*maxVel

def getPistonForce():
	P=0
	for i in piston:
		P += la.norm(O.forces.f(i))
	return P

def getPistonDisp():
	pistonDisp = O.bodies[piston[0]].state.displ().norm()
	block1Disp = O.bodies[block1[0]].state.displ().norm()
	block2Disp = O.bodies[block2[0]].state.displ().norm()
	return pistonDisp + (block1Disp + block2Disp)/2.

def checkBlockEngagement():
	Pblock1 = Pblock2 = 0
	for i in block1:
		Pblock1+= la.norm(O.forces.f(i))
	for i in block2:
		Pblock2+= la.norm(O.forces.f(i))
	return Pblock1, Pblock2


def stressStrainHist():
	plot.addData(
		i=O.iter,
		time = O.time,
		disp = -O.time*dispVel,
		P = getPistonForce(),
		pistonVel = pistonEngine.velocity,
		PistonDisp = getPistonDisp(),
		Pblock1 = checkBlockEngagement()[0],
		Pblock2 = checkBlockEngagement()[1])
	plot.saveDataTxt('txt/data'+identifier+'.txt',vars= ('i', 'disp','P', 'pistonVel','PistonDisp'))
	momentFile = 'moments_'+identifier+'.txt'

	AEcount=0
	if os.path.isfile(momentFile):
		AEcount = sum(1 for line in open(momentFile))

	f = open('txt/'+AEfile, 'a')
	P, pistonDisp = plot.data['P'],plot.data['PistonDisp']
	f.write('%g %g %g %g %g\n' % (O.time, O.iter, AEcount, P[-1], pistonDisp[-1]))
	f.close

def stopifDamaged():
	P=plot.data['P']
	if O.iter > 10000:
		if  max(P)>2000 and P[-1] < 0.5*max(P):
			print('failure reached')
			sigma_t = 3*max(P)*(2*(ysup-2*r))/(2*Z*X**2)
			print("Tensile strength=",sigma_t/1e6)
			O.pause()

plot.plots={'PistonDisp':('P'), 'i':('Pblock1','Pblock2')}
plot.plot(subPlots=True)

O.step()
### initializes the interaction detection factor
SSgeom.interactionDetectionFactor=-1.
Saabb.aabbEnlargeFactor=-1.

O.run()

waitIfBatch()
