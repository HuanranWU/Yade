
'''
(c) 2018 Bruno Chareyre <bruno.chareyre@grenoble-inp.fr>
# MPI execution of gravity deposition of 3D stacked blocked
# Possible executions of this script
# yade-mpi -x script.py (interactive, the default numThreads is used)
# yade-mpi script.py (passive, the default numThreads is used)
# mpiexec -n N yade-mpi script.py (semi-interactive, N defines numThreads)
# mpiexec -n N yade-mpi -x script.py (passive, N defines numThreads)

'''

from yade import mpy as mp
numThreads = mp.numThreads # 13 is the default, 'mpirun -n' overrides
np = numThreads-1  #remember to set odd number of cores to make the number of domains even

NSTEPS=1000 #turn it >0 to see time iterations, else only initilization TODO!HACK
L=10; M=10; N=10; #default size per thread
if len(sys.argv)>1: #we then assume L,M,N are provided as as cmd line arguments
	L=int(sys.argv[1]); N=int(sys.argv[3]); M=int(sys.argv[2])

Nx=Ny=Nz=1 #the number of subD in each direction of space

# Here we try and find a smart way to pile np blocks, expending in all three directions
# stack max. 3 domains vertically, if possible
if np%3==0: Ny=3; np=np/3
elif np%2==0: Ny=2; np=np/2
else: Ny=1
# now determine the how many domains along x
while np%4==0: 
	Nx*=2; Nz*=2; np/=4
if np%2==0: 
	Nx*=2; np/=2
Nz*=np

#add spheres
subdNo=0
import itertools
_id = 0 #will be used to count total number of bodies regardless of subdomain attribute, so that same ids are not reused for different bodies
for x,y,z in itertools.product(range(int(Nx)),range(int(Ny)),range(int(Nz))):
	subdNo+=1
	if mp.rank!=subdNo: continue
	ids=[]
	for i in range(L):#(numThreads-1) x N x M x L spheres, one thread is for master and will keep only the wall, others handle spheres
		for j in range(M):
			for k in range(N):
				dxOndy = 1/15.; dzOndy=1/15. # to make the columns inclined
				px= x*L+i+(y*M+j)*dxOndy; pz= z*N+k+(y*M+j)*dzOndy; py = (y*M+j)*(1 -dxOndy**2 -dzOndy**2)**0.5
				id = O.bodies.insertAtId(sphere((px,py,pz),0.500),_id+(N*M*L*(subdNo-1))) #a small shift in x-positions of the rows to break symmetry
				_id+=1
				ids.append(id)
	for id in ids: O.bodies[id].subdomain = subdNo
	
if mp.rank==0: #the wall belongs to master
	WALL_ID=O.bodies.insertAtId(box(center=(Nx*L/2,-0.5,Nz*N/2),extents=(2*Nx*L,0,2*Nz*N),fixed=True),(N*M*L*(numThreads-1)))

collider.verletDist = 0.25
newton.gravity=(0,-10,0) #else nothing would move
O.dt=0.001
O.dynDt=False
#import yade's mpi module
#from yade import mpy as mp
# customize
mp.VERBOSE_OUTPUT=False
mp.DISTRIBUTED_INSERT=True
mp.REALLOCATE_FREQUENCY=4
mp.MAX_RANK_OUTPUT=6
mp.mpirun(1,numThreads,False) #this is to eliminate initialization overhead in Cundall number and timings
mp.YADE_TIMING=True
t1=time.time()
mp.mpirun(NSTEPS,withMerge=False)
t2=time.time()
mp.mprint("num. bodies:",len([b for b in O.bodies if b.subdomain==mp.rank]),"/",len(O.bodies))

def collectTiming(Cundall):
	created = os.path.isfile("collect3D.dat")
	f=open('collect3D.dat','a')
	if not created: f.write("CuNumber numThreads mpi omp Nspheres L M N runtime \n")
	from yade import timing
	f.write(str(Cundall)+" "+str(numThreads*int(os.getenv('OMP_NUM_THREADS')))+" "+str(os.getenv('OMPI_COMM_WORLD_SIZE'))+" "+os.getenv('OMP_NUM_THREADS')+" "+str(N*M*L*(numThreads-1))+" "+str(L)+" "+str(M)+" "+str(N)+" "+str(timing.runtime())+"\n")
	f.close()

if mp.rank==0:
	CuN=N*M*L*(numThreads-1)*NSTEPS/(t2-t1)
	mp.mprint( "Total force on floor="+str(O.forces.f(WALL_ID)[1]))
	mp.mprint("CPU wall time for ",NSTEPS," iterations:",t2-t1,"; Cundall number = ",CuN)
	collectTiming(CuN)
#mp.mergeScene()
