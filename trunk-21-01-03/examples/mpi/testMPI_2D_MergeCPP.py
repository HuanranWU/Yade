# Possible executions of this script
### Parallel:
# mpiexec -n 4 yade-mpi -n -x testMPIxNxM.py
# mpiexec -n 4 yade-mpi  -n -x testMPIxN.py N M # (n-1) subdomains with NxM spheres each
### Monolithic:
# yade-mpi -n -x testMPIxN.py 
# yade-mpi -n -x testMPIxN.py N M
# yade-mpi -n -x testMPIxN.py N M n
# in last line the optional argument 'n' has the same meaning as with mpiexec, i.e. total number of bodies will be (n-1)*N*M but on single core
### Openmp:
# yade-mpi -j4 -n -x testMPIxN.py N M n
### Nexted MPI * OpenMP
# needs testing...
'''
This script simulates spheres falling on a plate using a distributed memory approach based on mpy module
The number of spheres assigned to one particular process (aka 'worker') is N*M, they form a regular patern.
The master process (rank=0) has no spheres assigned; it is in charge of getting the total force on the plate
The number of subdomains depends on argument 'n' of mpiexec. Since rank=0 is not assigned a regular subdomain the total number of spheres is (n-1)*N*M

'''

NSTEPS=2000#turn it >0 to see time iterations, else only initilization TODO!HACK
#NSTEPS=50 #turn it >0 to see time iterations, else only initilization
N=100; M=100; #(columns, rows) per thread

#################
# Check MPI world
# This is to know if it was run with or without mpiexec (see preamble of this script)
import os
rank = os.getenv('OMPI_COMM_WORLD_RANK')
if rank is not None: #mpiexec was used
	rank=int(rank)
	numThreads=int(os.getenv('OMPI_COMM_WORLD_SIZE'))
else: #non-mpi execution, numThreads will still be used as multiplier for the problem size (2 => multiplier is 1)
	numThreads=2 if len(sys.argv)<4 else (int(sys.argv[3]))
	print ("numThreads",numThreads)
	
if len(sys.argv)>1: #we then assume N,M are provided as 1st and 2nd cmd line arguments
	N=int(sys.argv[1]); M=int(sys.argv[2])

############  Build a scene (we use Yade's pre-filled scene)  ############

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
	if rank is not None:# assigning subdomain!=0 in single thread would freeze the particles (Newton skips them)
		for id in ids: O.bodies[id].subdomain = sd+1

WALL_ID=O.bodies.append(box(center=(numThreads*N*0.5,-0.5,0),extents=(2*numThreads*N,0,2),fixed=True))

#collider.verletDist = 0.7 TODO!HACK
collider.verletDist = 0.5
collider.targetInterv=200
newton.gravity=(0,-10,0) #else nothing would move
tsIdx=O.engines.index(timeStepper) #remove the automatic timestepper. Very important: we don't want subdomains to use many different timesteps...
O.engines=O.engines[0:tsIdx]+O.engines[tsIdx+1:]
#O.dt=0.00002 #this very small timestep will make it possible to run 2000 iter without merging
O.dt=0.0005#very important, we don't want subdomains to use many different timesteps... 
print("num bodies = ", len(O.bodies))


#########  RUN  ##########
def collectTiming():
	created = os.path.isfile("collect.dat")
	f=open('collect.dat','a')
	if not created: f.write("numThreads mpi omp Nspheres N M runtime \n")
	from yade import timing
	f.write(str(numThreads)+" "+str(os.getenv('OMPI_COMM_WORLD_SIZE'))+" "+os.getenv('OMP_NUM_THREADS')+" "+str(N*M*(numThreads-1))+" "+str(N)+" "+str(M)+" "+str(timing.runtime())+"\n")
	f.close()



def collectMergeInfo(numbodies, numIntersWall, numMerges , floorForce, numRealIntrs):
	created = os.path.isfile("collectMerge.dat")
	f=open('collectMerge.dat','a')
	if not created: f.write("numbodies numIntersWall numMerges floorForce \n")
	f.write(str(numbodies)+"    "+str(numIntersWall)+"    "+str(numMerges)+"    "+str(floorForce)+ "    "+ str(numRealIntrs) + "\n")
	f.close()





if rank is None: #######  Single-core  ######
	O.timingEnabled=True
	O.run(NSTEPS,True)
	#print "num bodies:",len(O.bodies)
	from yade import timing
	timing.stats()
	collectTiming()
	print ("num. bodies:",len([b for b in O.bodies]),len(O.bodies))
	print ("Total force on floor=",O.forces.f(WALL_ID)[1] )
        print ("num interactions = ", len(O.interactions)) 
        fl = open('intrs_serial.txt', 'w')
        for i in O.interactions:
	  a = i.id1; b = i.id2
	  if a > b : a,b = b,a
	  fl.write('%s %s\n' % (a,b))
	fl.close()
	print( "Total force on floor at 2 =",O.forces.f(WALL_ID)[1] )
	O.save('mergeSerial.yade')
	b = O.bodies[WALL_ID]
        print("num real interactions serial : " , O.interactions.countReal())
else: #######  MPI  ######
	#import yade's mpi module
	#from yade import mpy as mp TODO!HACK
	from yade import mpy as mp
	# customize
	mp.ACCUMULATE_FORCES=True  #trigger force summation on master's body (here WALL_ID)
	mp.VERBOSE_OUTPUT=False
	mp.ERASE_REMOTE=True #erase bodies not interacting wit a given subdomain?
	mp.OPTIMIZE_COM=True #L1-optimization: pass a list of double instead of a list of states
	mp.USE_CPP_MPI=True and mp.OPTIMIZE_COM #L2-optimization: workaround python by passing a vector<double> at the c++ level
	mp.MERGE_W_INTERACTIONS=True
        mp.MERGE_SPLIT=True 
        mp.COPY_MIRROR_BODIES_WHEN_COLLIDE = False 
        mp.WALL_ID = WALL_ID
	mp.mpirun(NSTEPS)
	print("num. bodies:",len([b for b in O.bodies]),len(O.bodies))
	if rank==0:
		fl = open('intrs_parallel.txt', 'w')
		for i in O.interactions:
		  a = i.id1; b = i.id2
		  if a > b : a,b = b,a
		  fl.write('%s %s\n' % (a,b))
		fl.close()
		collectTiming() 
		mp.mprint( "Total force on floor based on FORCES RECVD FROM WORKERS  ="+str(O.forces.f(WALL_ID)[1]))

		
	else: mp.mprint( "Partial force on floor="+str(O.forces.f(WALL_ID)[1]))
	mp.mergeScene()
	if rank==0: 
            # just for saving and checking the state after merge.
            #
	    O.save('mergedScene.yade')
            print ("force recieved from workers  = ", O.forces.f(WALL_ID)[1])
            #O.forces.reset()
            #collider.__call__()
            ##print "num interactions = " , len(O.interactions) 
            #O.step()
 	    #mp.mprint( "Total force on floor based on inters ="+str(O.forces.f(WALL_ID)[1]))
            spIds = [b.id for b in O.bodies if type(b.shape)==Sphere]; 
	    b = O.bodies[WALL_ID]; 
	    mp.mprint ("len of intrs of  WALL_ID ---> id = ", b.id, "  num inters =  ", len(b.intrs())); 
            mp.mprint ("num merges = ", mp.NUM_MERGES)
            collectMergeInfo(len(spIds), len(b.intrs()),mp.NUM_MERGES,O.forces.f(WALL_ID)[1],O.interactions.countReal()) 
            print("num interactions real = ", O.interactions.countReal())

	mp.MPI.Finalize()
#exit()
