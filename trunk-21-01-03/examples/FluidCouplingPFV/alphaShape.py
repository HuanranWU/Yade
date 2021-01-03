#*************************************************************************
#  Copyright (C) 2019 by Robert Caulk and Bruno Chareyre                 *
#  rob.caulk@gmail.com                                                   *
#                                                                        *
#  This program is free software; it is licensed under the terms of the  *
#  GNU General Public License v2 or later. See file LICENSE for details. *
#*************************************************************************/

## Example script showing the use of alpha boundary for flowengine applications ## 

from yade import pack, ymport,export
from yade import timing
import numpy as np
import shutil
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
timeStr = time.strftime('%m-%d-%Y')

identifier='testingAlpha'

if not os.path.exists('VTK'+timeStr+identifier):
	os.mkdir('VTK'+timeStr+identifier)
else:
	shutil.rmtree('VTK'+timeStr+identifier)
	os.mkdir('VTK'+timeStr+identifier)

mx = Vector3(0.9639118565206523,0.9324110610237126,0.8800533096533549)
mn = Vector3(-0.9518976876470108,-0.9685168132064216,-0.9451956043076566)

walls=aabbWalls([mn,mx],thickness=0)
wallIds=O.bodies.append(walls)

pred = pack.inSphere(Vector3(0,0,0),1)
sp=pack.randomDensePack(pred,radius=0.1,rRelFuzz=0.1,returnSpherePack=False,seed=1)
O.bodies.append(sp)

O.engines=O.engines[0:3]+[FlowEngine(dead=0,label="flow",multithread=False)]+O.engines[3:5]

# visualization of the alpha boundary: filter cells by "alpha" in paraview
def pressureField():
	flow.saveVtk('VTK'+timeStr+identifier+'/',withBoundaries=False)
O.engines=O.engines+[PyRunner(iterPeriod=100,command='pressureField()',dead=0)] 

for b in O.bodies:
	if isinstance(b.shape, Sphere):
		b.dynamic=False

flow.defTolerance = -1 
flow.meshUpdateInterval= -1
flow.useSolver=4
flow.permeabilityFactor=-1e-1
flow.alphaBound=0.3  # using manual alpha. Selecting a value of 0 will let CGAL find the minimum alpha necessary for obtaining a solid. Any positive value will be used as a manual alpha.
flow.alphaBoundValue=0  # the pressure to assign to the boundary
flow.fixedAlpha=0 # if false, use logic to improve concavity cell volume.
flow.imposePressure((0,0,0),1)

O.dt=1e-6
O.dynDt=False

O.run(101,1)

flow.printVertices()

dat = np.loadtxt('vertices.txt',skiprows=1)
xs = dat[7:,1]
ys = dat[7:,2]
zs = dat[7:,3]

radii = dat[7:,4]
alpha = dat[7:,5]

fig = plt.figure()
ax = fig.add_subplot(111, projection='3d')

ax.scatter(xs, ys, zs, c=alpha) # plot the alpha vertices as yellow

plt.show()


