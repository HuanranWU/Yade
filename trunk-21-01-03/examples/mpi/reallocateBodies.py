

# Possible executions of this script
# yadempi reallocateBodies.py

NSTEPS=100 #turn it >0 to see time iterations, else only initilization TODO!HACK
N=5; M=8; #(columns, rows) per thread

import os
from yade import mpy as mp
numThreads = 4

# sequential grain colors
import colorsys
colorScale = [Vector3(colorsys.hsv_to_rgb(value*1.0/numThreads, 1, 1)) for value in range(0, numThreads)]

#add spheres
for sd in range(0,numThreads-1):
	col = colorScale[sd+1]
	ids=[]
	for i in range(N):#(numThreads-1) x N x M spheres, one thread is for master and will keep only the wall, others handle spheres
		for j in range(M):
			id = O.bodies.append(sphere((sd*N+i+j/2.,j,0),0.500,color=col)) #a small shift in x-positions of the rows to break symmetry
			ids.append(id)
	for id in ids: O.bodies[id].subdomain = sd+1

WALL_ID=O.bodies.append(box(center=(numThreads*N*0.5,-0.5,0),extents=(2*numThreads*N,0,2),fixed=True))

collider.verletDist = 0.1
collider.targetInterv=0
newton.gravity=(0,-10,0) #else nothing would move
newton.damping=0.01 #let them bump
tsIdx=O.engines.index(timeStepper) #remove the automatic timestepper. Very important: we don't want subdomains to use many different timesteps...
O.engines=O.engines[0:tsIdx]+O.engines[tsIdx+1:]
O.dt=0.001
#O.dt=0.1*PWaveTimeStep() #very important, we don't want subdomains to use many different timesteps...

# customize mpy
mp.ERASE_REMOTE_MASTER=False
mp.MERGE_W_INTERACTIONS=False
mp.VERBOSE_OUTPUT=False
mp.YADE_TIMING=False
mp.REALLOCATE_FREQUENCY=2
mp.mpirun(NSTEPS+1,4,True) #+1 in order to be consistent with other example scripts

#if mp.rank==0: #the interactive commands below should be typed in the python prompt, if inserted in the script they need to be guarded 
	#mp.sendCommand("all","reallocateBodiesToSubdomains(medianFilter)",True)

	###coloring is not automatic, we re-do it here
	#colors=[(1,0,0),(0,1,0),(0,0,1)]
	#for b in O.bodies: b.shape.color=colors[b.subdomain-1]
	
def animate(N,nsteps):
	for i in range(N):
		mp.mpirun(nsteps,4,True)
		for b in O.bodies: b.shape.color=colorScale[b.subdomain]
		time.sleep(0.1)