from yade import plot,pack,timing,export,ymport
import time, sys, os, copy
import numpy as np

"""
Check test for JCFpm acoustic emission module

"""

if((opts.threads != None and opts.threads != 1) or (opts.cores != None and opts.cores != '1')):
	raise YadeCheckError("This test will only work on single core, because it must be fully reproducible, but -j "+str(opts.threads)+" or --cores "+str(opts.cores)+" is used.")

if os.path.exists('cracks_.txt'):
    os.remove('cracks_.txt')

if os.path.exists('moments_.txt'):
	os.remove('moments_.txt')

jCFmat = O.materials.append(JCFpmMat(young=65e9, cohesion=40e6, density=5000, frictionAngle=radians(19), tensileStrength=6e6, poisson=0.3, label='JCFmat'))

sp = O.bodies.append(ymport.textExt(checksPath+'/data/checkJCFpm.spheres','x_y_z_r',material='JCFmat'))

bb=uniaxialTestFeatures()
negIds,posIds,axis,crossSectionArea=bb['negIds'],bb['posIds'],bb['axis'],bb['area']
O.dt=0.7*PWaveTimeStep()

mm,mx=[pt[axis] for pt in aabbExtrema()]
coord_25,coord_50,coord_75=mm+.25*(mx-mm),mm+.5*(mx-mm),mm+.75*(mx-mm)
area_25,area_50,area_75=approxSectionArea(coord_25,axis),approxSectionArea(coord_50,axis),approxSectionArea(coord_75,axis)

O.engines=[
    ForceResetter(),
    InsertionSortCollider([Bo1_Sphere_Aabb(aabbEnlargeFactor=1.329,label='is2aabb'),],verletDist=.05*0.001),
    InteractionLoop(
        [Ig2_Sphere_Sphere_ScGeom6D(interactionDetectionFactor=1.329,label='ss2sc')],
        [Ip2_FrictMat_FrictMat_FrictPhys(),Ip2_JCFpmMat_JCFpmMat_JCFpmPhys(cohesiveTresholdIteration=1, label='jcf')],
        [Law2_ScGeom_JCFpmPhys_JointedCohesiveFrictionalPM(smoothJoint=False,neverErase=True, recordCracks=True, recordMoments=True ),Law2_ScGeom_FrictPhys_CundallStrack()],
    ),
    NewtonIntegrator(damping=0.7), UniaxialStrainer(strainRate=-0.5,axis=axis,asymmetry=0,posIds=posIds,negIds=negIds,crossSectionArea=crossSectionArea,blockDisplacements=False,blockRotations=False,setSpeeds=True) 
]

O.step(); 
ss2sc.interactionDetectionFactor=1.
is2aabb.aabbEnlargeFactor=1.
O.run(5000,1)
AEdata = np.loadtxt('moments_.txt',skiprows=1)
cracksdata = np.loadtxt('cracks_.txt',skiprows=1)

if not len(cracksdata)==sum(AEdata[:,5]):  # number of cracks should be equivalent to sum of cluster counts
	raise YadeCheckError('JCFpm checktest: cracks or AE clustering algorithm incorrect: len(cracksdata)=' + str(len(cracksdata)) + ', sum(AEdata[:,5])='+str(sum(AEdata[:,5])))
if not ((len(AEdata)==26) or (len(AEdata)==27) or (len(AEdata)==28)):
	raise YadeCheckError('JCFpm checktest: number of acoustic emission events incorrect = ' + str(len(AEdata)))

os.remove('moments_.txt')
os.remove('cracks_.txt')


