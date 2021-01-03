

# Possible executions of this script
# ./yadempi script.py #interactive will spawn additional workers
# mpiexec -n 4 ./yadempi script.py #non interactive

NSTEPS=100 #turn it >0 to see time iterations, else only initilization TODO!HACK

import os
from yade import mpy as mp
numThreads = 6


#add spheres
young = 5e6
compFricDegree = 0.0
O.materials.append(FrictMat(young=young, poisson=0.5, frictionAngle = radians(compFricDegree), density= 2600, label='sphereMat'))
O.materials.append(FrictMat(young=young*100, poisson = 0.5, frictionAngle = compFricDegree, density =2600, label='wallMat'))

mn,mx=Vector3(0,0,0),Vector3(150,150,100)
pred = pack.inAlignedBox(mn,mx)
O.bodies.append(pack.regularHexa(pred,radius=3,gap=0, material='sphereMat'))


walls=aabbWalls([Vector3(-mx[0]*2,-1,-mx[2]*2),Vector3(mx[0]*3,mx[1],mx[2]*3)], oversizeFactor=1, material='wallMat',wire=False)
for w in walls: w.shape.wire=False
O.bodies.append(walls[:3]+walls[4:]) #don't insert top wall

collider.verletDist = 2
newton.gravity=(0.05,-0.5,0.05) #else nothing would move
tsIdx=O.engines.index(timeStepper) #remove the automatic timestepper. Very important: we don't want subdomains to use many different timesteps...
O.engines=O.engines[0:tsIdx]+O.engines[tsIdx+1:]
O.dt=0.01

#########  RUN  ##########

def collectTiming():
	created = os.path.isfile("collect.dat")
	f=open('collect.dat','a')
	if not created: f.write("numThreads mpi omp Nspheres N M runtime \n")
	from yade import timing
	f.write(str(numThreads)+" "+str(os.getenv('OMPI_COMM_WORLD_SIZE'))+" "+os.getenv('OMP_NUM_THREADS')+" "+str(N*M*(numThreads-1))+" "+str(N)+" "+str(M)+" "+str(timing.runtime())+"\n")
	f.close()

# customize mpy

mp.VERBOSE_OUTPUT=False
mp.YADE_TIMING=False
mp.DOMAIN_DECOMPOSITION= True
#mp.MERGE_W_INTERACTIONS=True
#mp.ERASE_REMOTE_MASTER=True
mp.REALLOCATE_FREQUENCY=2
mp.mpirun(NSTEPS,numThreads,True)

#def animate():
	#for k in range(600):


# single-thread vtk output from merged scene
#if mp.rank == 0:
	#from yade import export
	#v=export.VTKExporter("mpi3d")
	
#for k in range(600):
	#mp.mpirun(15,4,True)
	#if mp.rank == 0:
		#v.exportSpheres(what=dict(subdomain='b.subdomain'))

