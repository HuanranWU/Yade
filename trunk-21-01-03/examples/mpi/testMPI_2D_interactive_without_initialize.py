

# Possible executions of this script
# ./yadempi script.py #interactive will spawn 3 additional workers
# mpiexec -n 4 ./yadempi script.py #non interactive

NSTEPS=1000 #turn it >0 to see time iterations, else only initilization TODO!HACK
#NSTEPS=50 #turn it >0 to see time iterations, else only initilization
N=50; M=50; #(columns, rows) per thread

import os
from yade import mpy as mp
numThreads = 4

# sequential grain colors
import colorsys
colorScale = (Vector3(colorsys.hsv_to_rgb(value*1.0/numThreads, 1, 1)) for value in range(0, numThreads))

#add spheres
for sd in range(0,numThreads-1):
	col = next(colorScale)
	ids=[]
	for i in range(N):#(numThreads-1) x N x M spheres, one thread is for master and will keep only the wall, others handle spheres
		for j in range(M):
			id = O.bodies.append(sphere((sd*N+i+j/30.,j,0),0.500,color=col)) #a small shift in x-positions of the rows to break symmetry
			ids.append(id)
	for id in ids: O.bodies[id].subdomain = sd+1

WALL_ID=O.bodies.append(box(center=(numThreads*N*0.5,-0.5,0),extents=(2*numThreads*N,0,2),fixed=True))

collider.verletDist = 0.5
collider.targetInterv=0
newton.gravity=(0,-10,0) #else nothing would move
tsIdx=O.engines.index(timeStepper) #remove the automatic timestepper. Very important: we don't want subdomains to use many different timesteps...
O.engines=O.engines[0:tsIdx]+O.engines[tsIdx+1:]
O.dt=0.001 #this very small timestep will make it possible to run 2000 iter without merging
#O.dt=0.1*PWaveTimeStep() #very important, we don't want subdomains to use many different timesteps...


#########  RUN  ##########
def collectTiming():
	created = os.path.isfile("collect.dat")
	f=open('collect.dat','a')
	if not created: f.write("numThreads mpi omp Nspheres N M runtime \n")
	from yade import timing
	f.write(str(numThreads)+" "+str(os.getenv('OMPI_COMM_WORLD_SIZE'))+" "+os.getenv('OMP_NUM_THREADS')+" "+str(N*M*(numThreads-1))+" "+str(N)+" "+str(M)+" "+str(timing.runtime())+"\n")
	f.close()

# customize mpy
mp.MERGE_W_INTERACTIONS=False
mp.ERASE_REMOTE_MASTER=False

mp.mpirun(NSTEPS+1,4,True) #+1 in order to be consistent with other example scripts
mp.mprint( "num. bodies:",len([b for b in O.bodies]),len(O.bodies))
mp.mprint( "Partial force on floor="+str(O.forces.f(WALL_ID)[1]))

#demonstrate getting stuff from workers
if mp.rank==0:
	print("kinetic energy from workers: "+str(mp.sendCommand([1,2],"kineticEnergy()",True)))
