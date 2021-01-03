#!/usr/bin/env python
# encoding: utf-8

# Script generates clumps and saves them
# Then it tries to load the saved simulation
# See (old site, fixed bug) https://bugs.launchpad.net/bugs/1560171
# Thanks to Bettina Suhr for providing the
# minimal test script.

from __future__ import print_function

from yade import pack
import time
from tempfile import TemporaryDirectory

#define material for all bodies:
id_Mat=O.materials.append(FrictMat(young=1e6,poisson=0.3,density=1000,frictionAngle=1))
partType='clumps'

#define engines:
O.engines=[
        ForceResetter(),
        InsertionSortCollider([Bo1_Sphere_Aabb(),Bo1_Facet_Aabb()]),
        InteractionLoop(
                [Ig2_Sphere_Sphere_ScGeom(),Ig2_Facet_Sphere_ScGeom()],
                [Ip2_FrictMat_FrictMat_FrictPhys()],
                [Law2_ScGeom_FrictPhys_CundallStrack()]
        ),
        NewtonIntegrator(damping=0.7,gravity=[0,0,-9.81])
]

#create a box:
O.bodies.append(geom.facetBox(center=(0.0,0.0,0.25),extents = (0.125,0.125,0.25), wallMask=31))

#add particles eiter spheres or clumps
if partType=='spheres':
  sp=pack.SpherePack()
  sp.makeCloud( (-0.125,-0.125,0), ( 0.125,0.125,0.5), rMean=37.6e-3/2.,rRelFuzz=0.0,num=100)
  sp.toSimulation()
  O.bodies.updateClumpProperties(discretization=10)# correct mass, volume, inertia!!
elif partType=='clumps':
  sp=pack.SpherePack()
  c1=pack.SpherePack([ ((0.,0.,0.),37.6e-3/2.), ((37.6e-3/2.,0.,0.),25e-3/2.) ])# overlap between both spheres
  sp.makeClumpCloud( (-0.125,-0.125,0), ( 0.125,0.125,0.5), [c1],num=80)
  sp.toSimulation()
  O.bodies.updateClumpProperties(discretization=10)# correct mass, volume, inertia!!
else:
  print("ERROR! choose either spheres or clumps for partType!")

O.dt=1e-6

# We check the kinetic energy to be sure that the particles are not overlapping and do not explode
def checkKineticEnergy(lowerBound,upperBound):
  Ekin=kineticEnergy()
  if ( Ekin > upperBound or Ekin < lowerBound ):
    O.pause()
    raise YadeCheckError("Kinetic energy %E is not within bounds from %E to %E! Error!"%( Ekin, lowerBound , upperBound ))
  else:
    print ("Kinetic energy OK %E"%(Ekin))

#write some restart files
with TemporaryDirectory() as tmp_dir:
  O.save(tmp_dir + '/restartMinWorkEx_'+partType+'_Initial')

  # Run simulation to check the kinetic energy
  O.run(10, True)
  checkKineticEnergy(0.6e-9,1.4e-9)

  # Let the object settle
  O.run(100000,True)
  curIter = O.iter
  checkKineticEnergy(0.07,0.16)

  # Save simulation
  O.save(tmp_dir + '/restartMinWorkEx_%s_%d'%(partType,curIter))

  time.sleep(1)
  O.reset()
  time.sleep(1)

  # Load simulation
  O.load(tmp_dir + '/restartMinWorkEx_%s_%d'%(partType,curIter))
  O.run(10,True)
  checkKineticEnergy(0.07,0.16)
  O.run(1000, True)
  checkKineticEnergy(0.07,0.16)
