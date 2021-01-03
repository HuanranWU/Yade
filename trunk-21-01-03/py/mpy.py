# 2018 © Bruno Chareyre <bruno.chareyre@grenoble-inp.fr> 

'''
This module defines mpirun(), a parallel implementation of run() using a distributed memory approach. Message passing is done with mpi4py mainly, however some messages are also handled in c++ (with openmpi).

.. note:: Many internals of the mpy module listed on this page are not helpful to the user. Instead, please find :ref:`introductory material on mpy module<mpy>` in user manual.


Logic:
------

The logic for an initially centralized scene is as follows:

1. Instanciate a complete, ordinary, yade scene
2. Insert subdomains as special yade bodies. This is somehow similar to adding a clump body on the top of clump members
3. Broadcast this scene to all workers. In the initialization phase the workers will:

	- define the bounding box of their assigned bodies and return it to other workers
	- detect which assigned bodies are virtually in interaction with other domains (based on their bounding boxes) and communicate the lists to the relevant workers
	- erase the bodies which are neither assigned nor virtually interacting with the subdomain

4. Run a number of 'regular' iterations without re-running collision detection (verlet dist mechanism). In each regular iteration the workers will:

	- calculate internal and cross-domains interactions
	- execute Newton on assigned bodies (modified Newton skips other domains)
	- send updated positions to other workers and partial force on floor to master
	
5. When one worker triggers collision detection all workers will follow. It will result in updating the intersections between subdomains.

6. If enabled, bodies may be re-allocated to different domains just after a collision detection, based on a filter. Custom filters are possible. One is predidefined here (medianFilter)

Rules:
------

	#- intersections[0] has 0-bodies (to which we need to send force)
	#- intersections[thisDomain] has ids of the other domains overlapping the current ones
	#- intersections[otherDomain] has ids of bodies in _current_ domain which are overlapping with other domain (for which we need to send updated pos/vel)

Hints:
------

	#- handle subD.intersections with care (same for mirrorIntersections). subD.intersections.append() will not reach the c++ object. subD.intersections can only be assigned (a list of list of int)

'''

import sys,os,inspect
import time
import numpy as np
import yade.runtime
this = sys.modules[__name__]


#from yade import *
from yade.utils import *
from yade.wrapper import *
#import yade.runtime
from yade import timing; timing.mpi={} #prepare a dictionnary for mpi-related stats

# for coloring bodies
import colorsys

## Config flags

ACCUMULATE_FORCES=True #control force summation on master's body. FIXME: if false master goes out of sync since nothing is blocking rank=0 thread
VERBOSE_OUTPUT=False
NO_OUTPUT=False
COLOR_OUTPUT=True # color mprint output per rank / turn off here for sphinx build mainly
MAX_RANK_OUTPUT=5 #: larger ranks will be skipped in mprint
SEND_SHAPES=False #if false only bodies' states are communicated between threads, else shapes as well (to be implemented)
ERASE_REMOTE = True # True is MANDATORY. Erase bodies not interacting wit a given subdomain? else keep dead clones of all bodies in each scene
ERASE_REMOTE_MASTER = True # erase remotes on master or keep them for fast merge (updating just b.state)
OPTIMIZE_COM=True
USE_CPP_MPI=True and OPTIMIZE_COM
YADE_TIMING=False #report timing.stats()?
MERGE_SPLIT = False
MERGE_W_INTERACTIONS = True
COPY_MIRROR_BODIES_WHEN_COLLIDE = True  # True is MANDATORY 
RESET_SUBDOMAINS_WHEN_COLLIDE = False
DOMAIN_DECOMPOSITION = False
NUM_MERGES = 0
SEND_BYTEARRAYS = True
ENABLE_PFACETS = False    #PFacets need special (and expensive) tricks, if PFacets are not used skip the tricks
DISTRIBUTED_INSERT = False  #if True each worker is supposed to "O.bodies.insertAtId" its own bodies
REALLOCATE_FREQUENCY = 0  # if >0 checkAndCollide() will automatically reallocate bodies to subdomains, if =1 realloc. happens each time collider is triggered, if >1 it happens every N trigger
REALLOCATE_FILTER = None # pointer to filtering function, will be set to 'medianFilter' hereafter, could point to other ones if implemented
AUTO_COLOR = True
MINIMAL_INTERSECTIONS = False # Reduces the size of position/velocity comms (at the end of the colliding phase, we can exclude those bodies with no interactions besides body<->subdomain from intersections). 
REALLOCATE_MINIMAL = False # if true, intersections are minimized before reallocations, hence minimizing the number of reallocated bodies
fibreList = []
FLUID_COUPLING = False
fluidBodies = [] 
USE_CPP_REALLOC = True
USE_CPP_INTERS = False #sending intersections using mpi4py sometimes fails (dependent on mpi4py version, needs confirmation) (ERR : MPI_ERR_TRUNCATE)


### Internals

comm = None
comm_slave = None # will be MPI.Comm.Get_parent() at runtime, until we disconnect 
rank = None
numThreads = None
MPI = None # will be mpi4py.MPI after configure()

waitingCommands=False		# are workers currently interactive?
userScriptInCheckList=""	# detect if mpy is executed by checkList.py
caller_name=""			# name of the executed script (typically user script, alternatively yadeSphinx.py or checkList.py)
_REALLOC_COUNT=0
colorScale=None
# for coloring processes outputs differently
bcolors=['\x1b[95m','\x1b[94m','\x1b[93m','\x1b[92m','\x1b[91m','\x1b[90m','\x1b[95m','\x1b[93m','\x1b[91m','\x1b[1m','\x1b[4m','\x1b[0m']

#tags for mpi messages
_SCENE_=11
_SUBDOMAINSIZE_=12
_INTERSECTION_=13
_ID_STATE_SHAPE_=14
_FORCES_=15
_MIRROR_INTERSECTIONS_ = 16
_POS_VEL_ = 17
_BOUNDS_ = 18
_MASTER_COMMAND_ = 19
_RETURN_VALUE_ = 20
_ASSIGNED_IDS_ = 21
_GET_CONNEXION_= 22



	
def makeColorScale(n=numThreads):
	scale= [(0.3+random.random())*Vector3(colorsys.hsv_to_rgb(value*1.0/n, 1, 1)) for value in range(0, n)]
	from random import shuffle
	random.seed(1)
	shuffle(scale)
	return scale

def mprint(*args,force=False): #this one will print regardless of VERBOSE_OUTPUT
	"""
	Print with rank-reflecting color regardless of mpy.VERBOSE_OUTPUT, still limited to rank<=mpy.MAX_RANK_OUTPUT
	"""
	if (NO_OUTPUT or rank>MAX_RANK_OUTPUT) and not force: return
	m=bcolors[min(rank,len(bcolors)-2)]  if COLOR_OUTPUT else ''
	resetFont='\x1b[0m' if COLOR_OUTPUT else ''
	if rank==0:
		m+='Master: '
	else:
		m+='Worker'+str(rank)+": "
	for a in args:
		m+=str(a)+" "
	print (m+resetFont)
	# sys.stdout.flush() # maybe?

def wprint(*args):
	"""
	Print with rank-reflecting color, *only if* mpy.VERBOSE_OUTPUT=True (else see :yref:`yade.mpy.mprint`), limited to rank<=mpy.MAX_RANK_OUTPUT
	"""
	if not VERBOSE_OUTPUT: return
	mprint(*args)
	
### Here we detect current MPI context *only if needed*, i.e. importing mpy without actually using it will not call configure() (then no MPI related warnings)
#if yade.runtime.opts.mpi_mode:
	#configure()

def colorDomains():
	'''
	Apply color to body to reflect their subdomain idx
	'''
	global colorScale
	if len(colorScale)<numThreads:
		colorScale = makeColorScale(numThreads)
	for b in O.bodies:
		b.shape.color=colorScale[b.subdomain]
		
## Initialization

def configure(): # calling this function will import mpi4py.MPI, 
	'''
	Import MPI and define context, configure will no spawn workers by itself, that is done by initialize()
	openmpi environment variables needs to be set before calling configure()
	'''
	global comm, comm_slave, rank, numThreads, colorScale, MPI
	#os.environ["OMPI_MCA_rmaps_base_oversubscribe"]="1" # needed here, after importing MPI is too late (or there is a way to update flags before the spawn?)
	from mpi4py import MPI
	worldComm = MPI.COMM_WORLD
	color = 3; key =0;
	comm = worldComm.Split(color, key)  # if OFOAM coupled, split communicator

	comm_slave = comm.Get_parent()
	if comm_slave!=MPI.COMM_NULL: 	# if executor is a spawned worker merge comm with master 
		comm=comm_slave.Merge()

	rank = comm.Get_rank()		# set rank and numThreads
	if rank>0:
		sys.stderr.write=sys.stdout.write # so we see error messages from workers in terminal
	numThreads = comm.Get_size()
	colorScale= makeColorScale(numThreads)
	
configure() # needed at import time, else we can't check rank or numThreads right after import 

def disconnect():
	'''
	Kill all mpi processes, leaving python interpreter to rank 0 as in single-threaded execution.
	The scenes in workers are lost since further reconnexion to mpi will just spawn new processes.
	The scene in master thread is left unchanged. 
	'''
	
	global comm,comm_slave
	if rank==0: # exit the interactive mode on master _after_ telling workers to exit
		wprint("sending exit command")
		sendCommand(executors="slaves",command="exit",wait=False)
	if comm:
		wprint("disconnecting")
		 # should be a Disconnect(), no a Free(), but we have an issue with openmpi it seems
		 # https://bitbucket.org/mpi4py/mpi4py/issues/176/disconnect-hangs-with-openmp31-python-38
		comm.Free() 
		if comm_slave:
			comm_slave.Disconnect()
		else:
			mprint("please report bug in mpy.disconnect()")
		if rank>0: # kill workers
			exit
	else:
		mprint("mpy already disconnected")
	comm=None
	comm_slave=None
	yade.runtime.opts.mpi_mode=False

def initialize(np):
	global comm,comm_slave,rank,numThreads,userScriptInCheckList,colorScale
	if comm==None: configure() # should only happen after a despawn
	process_count = comm.Get_size()

	if(process_count<np):
		#TODO: if process_count>numThreads, free some workers
		# HACK 1: this is to detect when we run regression tests and prevent mpirun to enter a parallel testing spree
		if(("--check" in sys.yade_argv) or ("--checkall" in sys.yade_argv)):
			userScriptInCheckList=inspect.stack()[2][1]
		# END HACK 1 (continued below)
		
		# HACK 2:
		yadeArgv = sys.yade_argv.copy()
		if yadeArgv.count("yadeSphinx.py"):
			# yadeSphinx.py itself must not run parallel (only examples in it may run parallel), remove it from argv so workers will not run it
			# they will wait after being initialized, responding to messages sent trhough ipython directives within *.rst source
			yadeArgv.remove("yadeSphinx.py")
			# suppress (harmless?) messages on newer versions of linux (docker specific) - https://github.com/open-mpi/ompi/issues/4948
			#os.environ['OMPI_MCA_btl_vader_single_copy_mechanism']='none'
		# END HACK 2
		
		numThreads=np
		colorScale= makeColorScale(numThreads)
		
		if not yade.runtime.opts.mpi_mode: #MASTER only, the workers will be already in mpi_mode
			mprint("will spawn ",numThreads-process_count," workers")
			if (userScriptInCheckList==""): #normal case
				comm_slave = MPI.COMM_WORLD.Spawn(yadeArgv[0], args=yadeArgv[1:],maxprocs=numThreads-process_count)
			else: #HACK 1 (continued) handle execution by checkList.py, otherwise would we run checkList.py in parallel
				#os.environ['OMPI_MCA_btl_vader_single_copy_mechanism']='none' # suppress (harmless?) messages on newer versions of linux (docker specific)
				comm_slave = MPI.COMM_WORLD.Spawn(sys.yade_argv[0], args=[userScriptInCheckList],maxprocs=numThreads-process_count)
			comm = comm_slave.Merge()
			yade.runtime.opts.mpi_mode=True
			rank=0
		else:	#WORKERS
			mprint("spawned")
		#initialize subdomains. For Master it will be used storage and comm only, for workers it will be over-written in the split operation
		O.subD=Subdomain() #for storage and comm only, this one will not be used beyond that 
		O.subD.comm = comm
	else:
		if rank==0: yade.runtime.opts.mpi_mode=True #since if we started without mpiexec it is initialized to False
	return rank,numThreads

def spawnedProcessWaitCommand():
	global waitingCommands
	if waitingCommands: return
	if comm==None: configure()
	wprint("I'm now waiting")
	waitingCommands = True
	s=MPI.Status()
	while 1:
		while not comm.Iprobe(source=MPI.ANY_SOURCE, tag=_MASTER_COMMAND_, status=s):
			time.sleep(0.001)
		command = comm.recv(source=s.source,tag=_MASTER_COMMAND_)
		if command=="exit": #this is to terminate the waiting loop remotely
			comm.send(0,dest=s.source,tag=_RETURN_VALUE_)
			break
		wprint("will now execute ",command)
		try:
			exec(command)
		except:
			comm.send(None,dest=s.source,tag=_RETURN_VALUE_)
			mprint(sys.exc_info(),force=True)
			raise
	waitingCommands = False
	disconnect() # this will kill the workers
		
def sendCommand(executors,command,wait=True,workerToWorker=False):
	'''
	Send a command to a worker (or list of) from master or from another worker. Accepted executors are "i", "[i,j,k]", "slaves", "all" (then even master will execute the command).
	'''
	start=time.time()
	if (rank>0 and not workerToWorker): wprint("sendCommand ignored by worker", rank, ", pass workerToWorker=True to force it"); return
	if (executors=="all"): executors=list(range(numThreads))
	if (executors=="slaves"): executors=list(range(1,numThreads))
	argIsList=isinstance(executors,list)
	toMaster = (argIsList and 0 in executors) or executors==0
	if (toMaster and rank>0): mprint("workers cannot sendCommand to master (only master to itself)")
	
	if not argIsList: executors = [executors]
	#if 0 in executors: mprint("master does not accept mpi commands"); return
	if len(executors)>numThreads: mprint("executors > numThreads"); return
	
	if wait and not command=="exit":#trick command to make it return a result by mpi
		commandSent="resCommand="+command+";comm.send(resCommand,dest="+str(rank)+",tag=_RETURN_VALUE_)"
	else: commandSent=command
	
	reqs=[]
	for w in executors:
		#note: if the return from this isend() is not appended to a list we have random deadlock
		if (w>0): reqs.append(comm.isend(commandSent,dest=w,tag=_MASTER_COMMAND_) )
	
	resCommand=[]
	if toMaster:#eval command on master since it wasn't done yet
		try: resCommand = [eval(command)]
		except:
			resCommand = [None]
			mprint(sys.exc_info(),force=True)
			raise
	
	if wait:
		resCommand=resCommand+ [comm.recv(source=w,tag=_RETURN_VALUE_) for w in executors if w>0]
		wprint("sendCommand returned in "+str(time.time()-start)+" s")
		return (resCommand if argIsList else resCommand[0])
	else:
		return None


def probeRecvMessage(source, tag):
	msgStat = MPI.Status() 
	comm.Probe(source=source, tag=tag, status=msgStat)
	if msgStat.tag == tag : print("message size recvd")
	data = bytearray(msgStat.Get_count(MPI.BYTE))  
	comm.Recv([data, MPI.BYTE], source=source, tag=tag)
	return data

###  TIMING ###

def recordMpiTiming(name,val):
	'''
	append val to a list of values defined by 'name' in the dictionnary timing.mpi
	'''
	if not name in yade.timing.mpi.keys(): yade.timing.mpi[name]=[]
	yade.timing.mpi[name].append(val)
	
def mpiStats():
	if (waitingCommands and rank==0): sendCommand(list(range(1,numThreads)),"mpiStats()")
	dat = yade.timing.mpi
	dat = comm.gather(dat,root=0)
	return dat
yade.timing.mpiStats=mpiStats

class Timing_comm():
	def __init__(self):
		self.timings={}
		
	def clear(self):
		self.timings={}
	
	def print_all(self):
		time.sleep((numThreads-rank)*0.001)
		message = "COMMUNICATION TIMINGS:\n" 
		max_string_len = len(max(self.timings.keys(),key=len))
		for k,v in sorted(self.timings.items(), key=lambda x: x[1][1], reverse=True):
			message += ("{:<"+str(max_string_len)+"}").format(k) + " " + str(v) + "\n"
		mprint(message)
	
	def enable_timing(comm_function):
		def wrapper(self,timing_name, *args, **kwargs):
			#pre-exec
			ti=time.time()
			#exec
			rvalue=comm_function(self, *args, **kwargs)
			#post-exec
			if(not timing_name in self.timings.keys()):
				self.timings[timing_name]=[0,0]
			self.timings[timing_name][0]+=1
			self.timings[timing_name][1]+=time.time()-ti
			return rvalue
		return wrapper
	
	@enable_timing
	def send(self, *args, **kwargs):
		return comm.send(*args,**kwargs)
	
	@enable_timing
	def recv(self, *args, **kwargs):
		return comm.recv(*args,**kwargs)
	
	@enable_timing
	def bcast(self, *args, **kwargs):
		return comm.bcast(*args,**kwargs)
	
	@enable_timing
	def allreduce(self, *args, **kwargs):
		return comm.allreduce(*args,**kwargs)
	
	@enable_timing
	def Gather(self, *args, **kwargs):
		return comm.Gather(*args, **kwargs)
	
	@enable_timing
	def Gatherv(self, *args, **kwargs):
		return comm.Gatherv(*args, **kwargs)
	
	@enable_timing
	def Allgather(self, *args, **kwargs):
		return comm.Allgather(*args, **kwargs)
	 #this is to time the cpp messages
	@enable_timing
	def mpiWaitReceived(self,*args, **kwargs):
		return O.subD.mpiWaitReceived(*args, **kwargs)
	
	@enable_timing
	def mpiSendStates(self,*args, **kwargs):
		return O.subD.mpiSendStates(*args, **kwargs)

timing_comm = Timing_comm()

def receiveForces(subdomains):
	'''
	Accumulate forces from subdomains (only executed by master process), should happen after ForceResetter but before Newton and before any other force-dependent engine (e.g. StressController), could be inserted via yade's pyRunner.
	'''
	if 1: #non-blocking:
		reqForces=[]
		#for sd in subdomains:
		for sd in O.subD.intersections[0]:
			#wprint( "master getting forces from "+str(b.subdomain)+"(id="+str(b.id)+")")		
			reqForces.append(comm.irecv(None,sd, tag=_FORCES_))
			#wprint( "master got forces from "+str(b.subdomain)+": "+str(forces))		
		for r in reqForces:
			forces=r.wait()
			for ft in forces:
				#wprint(  "adding force "+str(ft[1])+" to body "+str(ft[0]))
				O.forces.addF(ft[0],ft[1])
				O.forces.addT(ft[0],ft[2])
	else:
		for sd in subdomains:
			forces=timing_comm.recv("isendRecvForces",source=sd, tag=_FORCES_)
			#wprint( "master got forces from "+str(sd)+": "+str(forces)+" iter="+str(O.iter)+" dt="+str(O.dt))
			for ft in forces:
				#wprint(  "adding force "+str(ft[1])+" to body "+str(ft[0]))
				O.forces.addF(ft[0],ft[1])
				O.forces.addT(ft[0],ft[2])

def shrinkIntersections():
	'''
	Reduce intersections and mirrorIntersections to bodies effectively interacting with another statefull body form current subdomain
	This will reduce the number of updates in sendRecvStates
	Initial lists are backed-up and need to be restored (and all states updated) before collision detection (see checkAndCollide())
	'''
	O.subD.fullIntersections = O.subD.intersections
	O.subD.fullMirrorIntersections = O.subD.mirrorIntersections
	if (rank==0): return 0,0
	res=O.subD.filterIntersections()
	oriLen=sum([len(c) for c in O.subD.intersections])
	reqs=[]
	for other in O.subD.intersections[rank]:
			if other==0: continue
			reqs.append([other,comm.irecv(None, other, tag=_MIRROR_INTERSECTIONS_)])
			comm.send(O.subD.intersections[other],dest=other,tag=_MIRROR_INTERSECTIONS_)
	ints = O.subD.mirrorIntersections
	
	for r in reqs:
		ints[r[0]]=r[1].wait()
		if ints[r[0]]!=O.subD.mirrorIntersections[r[0]]:
			wprint("inconsistency in the filtering of intersections[",r[0],"]:",len(ints[r[0]]),"received vs.",len(O.subD.mirrorIntersections[r[0]]))
	O.subD.mirrorIntersections = ints #that's because python wrapping only enable assignment
	return res,oriLen

def checkAndCollide():
	'''
	return true if collision detection needs activation in at least one SD, else false. If COPY_MIRROR_BODIES_WHEN_COLLIDE run collider when needed, and in that case return False.
	'''
	global _REALLOC_COUNT
	needsCollide = int(typedEngine("InsertionSortCollider").isActivated())
	if(needsCollide!=0):
		wprint("triggers collider at iter "+str(O.iter))
	needsCollide = timing_comm.allreduce("checkcollider",needsCollide,op=MPI.SUM)
	if needsCollide:
		if(COPY_MIRROR_BODIES_WHEN_COLLIDE):
			if MINIMAL_INTERSECTIONS:
				if hasattr(O.subD,"fullIntersections"): # if we have tricked intersections in previous steps we set them backto full content in order to update all positions before colliding
					O.subD.intersections = O.subD.fullIntersections
					O.subD.mirrorIntersections = O.subD.fullMirrorIntersections
				#else: mprint("fullIntersections not initialized (first iteration or rank=0)")
					sendRecvStates() # triggers comm
			
			# parallel collision detection (incl. insertion of newly intersecting bodies)
			parallelCollide()
			# reallocate and/or minimize intersections
			if REALLOCATE_FREQUENCY>0:
				_REALLOC_COUNT+=1
				if _REALLOC_COUNT>=REALLOCATE_FREQUENCY:
					#comm.barrier() #we will modify intersections while they can still be accessed by calls to mpi in parallelCollide()
					if (MINIMAL_INTERSECTIONS and REALLOCATE_MINIMAL):
						r=shrinkIntersections() #if we filter before reallocation we minimize the reallocations
						#mprint("filtered out (1)",r[0],"of",r[1])
					reallocateBodiesToSubdomains(REALLOCATE_FILTER)
					_REALLOC_COUNT=0
			if (MINIMAL_INTERSECTIONS): #filter here, even if already done before, since realloc updated intersections
				#if rank>0:
				r=shrinkIntersections()
				recordMpiTiming("filteredInts",r[0]); recordMpiTiming("totInts",r[1]); recordMpiTiming("interactionsInts",len(O.interactions)); recordMpiTiming("iterInts",O.iter)
				#mprint("filtered out (2)",r[0],"of",r[1])
			return False
		else: return True
	return False

def unboundRemoteBodies():
	'''
	Turn bounding boxes on/off depending on rank
	'''
	for b in O.bodies:# unbound the bodies assigned to workers (not interacting directly with other bodies in master scene)
		if not b.isSubdomain and b.subdomain!=rank:
			b.bounded=False
			
def reboundRemoteBodies(ids):
	'''
	update states of bodies handled by other workers, argument 'states' is a list of [id,state] (or [id,state,shape] conditionnaly)
	'''
	if isinstance(ids,list):
		for id in ids:
			b = O.bodies[id]
			if b and not isinstance(b.shape,GridNode): b.bounded=True
	else: #when passing numpy array we need to convert 'np.int32' to 'int'
		for id in ids:
			b = O.bodies[id.item()] 
			if b and not isinstance(b.shape,GridNode): b.bounded=True 

def updateDomainBounds(subdomains): #subdomains is the list of subdomains by body ids
	'''
	Update bounds of current subdomain, broadcast, and receive updated bounds from other subdomains
	Precondition: collider.boundDispatcher.__call__() 
	'''
	wprint( "Updating bounds: "+str(subdomains))
	if(rank==0):
		send_buff=np.zeros(6)*np.nan
	else:
		subD=O.bodies[subdomains[rank-1]].shape #shorthand to shape of current subdomain
		send_buff=np.append(subD.boundsMin,subD.boundsMax)
	recv_buff = np.empty(6*numThreads)
	timing_comm.Allgather("updateDomainBounds",send_buff,recv_buff)
	
	for r in range(1,numThreads):
		O.bodies[subdomains[r-1]].shape.boundsMin = recv_buff[6*r:6*r+3]
		O.bodies[subdomains[r-1]].shape.boundsMax = recv_buff[3+6*r:6+6*r]
		#if(VERBOSE_OUTPUT):#condition here to avoid concatenation overhead
			#mprint("Updated ", O.bodies[subdomains[r-1]].subdomain, " with min=", O.bodies[subdomains[r-1]].shape.boundsMin," and max=", O.bodies[subdomains[r-1]].shape.boundsMax)

            
def maskedPFacet(pf, boolArray):
	'''
	List bodies within a facet selectively, the ones marked 'True' in boolArray (i.e. already selected from another facet) are discarded
	'''
	l=[]
	for id in [pf.node1.id, pf.node2.id, pf.node3.id, pf.conn1.id, pf.conn2.id, pf.conn3.id]:
		if not boolArray[id]:
			l.append(id)
			boolArray[id]=True

def maskedPFacet(b, boolArray):
	'''
	List bodies within a facet selectively, the ones marked 'True' in boolArray (i.e. already selected from another facet) are discarded
	'''
	l=[]
	pf=b.shape
	for id in [b.id,pf.node1.id, pf.node2.id, pf.node3.id, pf.conn1.id, pf.conn2.id, pf.conn3.id]:
		if not boolArray[id]:
			l.append(id)
			boolArray[id]=True
	return l

def maskedConnection(b, boolArray):
	'''
	List bodies within a facet selectively, the ones marked 'True' in boolArray (i.e. already selected from another facet) are discarded
	'''
	l=[]
	pf=b.shape
	for id in [b.id,pf.node1.id, pf.node2.id]:
		if not boolArray[id]:
			l.append(id)
			boolArray[id]=True
	return l

def genLocalIntersections(subdomains):
	'''
	Defines sets of bodies within current domain overlapping with other domains.
	
	The structure of the data for domain 'k' is:
	[[id1, id2, ...],  <----------- intersections[0] = ids of bodies in domain k interacting with master domain (subdomain k itself excluded)
	[id3, id4, ...],  <----------- intersections[1] = ids of bodies in domain k interacting with domain rank=1 (subdomain k itself excluded)
	...
	[domain1, domain2, domain3, ...], <---------- intersections[k] = ranks (not ids!) of external domains interacting with domain k
	...
	]
	'''
	intersections=[[] for n in range(numThreads)]
	for sdId in subdomains:
		#grid nodes or grid connections could be appended twice or more, as they can participate in multiple pfacets and connexions
		#this bool list is used to append only once
		if (ENABLE_PFACETS):
			if rank==0: print("this one makes mpi inneficient when ENABLE_PFACETS, contact yade devs if you are interested in developping MPI for PFacets")
			appended = np.repeat([False],len(O.bodies))
		subdIdx=O.bodies[sdId].subdomain
		intrs=O.interactions.withBodyAll(sdId)
		#special case when we get interactions with current domain, only used to define interactions with master, otherwise some intersections would appear twice
		if subdIdx==rank:
			for i in intrs:
				otherId=i.id1 if i.id2==sdId else i.id2
				b=O.bodies[otherId]
				if not b:continue #in case the body was deleted
				if b.subdomain==0:
					if isinstance(b.shape,PFacet):
						intersections[0]+= maskedPFacet(b, appended); continue
					if isinstance(b.shape,GridConnection):
						intersections[0]+=maskedConnection(b, appended); continue
					#else (standalone body, normal case)
					intersections[0].append(otherId)
			if len(intersections[0])>0: intersections[subdIdx].append(0)
			continue
		# normal case
		for i in intrs:
			otherId=i.id1 if i.id2==sdId else i.id2
			b=O.bodies[otherId]
			if not b:continue #in case the body was deleted
			if b.subdomain!=rank: continue
			if b.isSubdomain : intersections[rank].append(subdIdx) #intersecting subdomain (will need to receive updated positions from there)
			else:
				if isinstance(b.shape,PFacet):
						intersections[subdIdx]+= maskedPFacet(b, appended); continue
				if isinstance(b.shape,GridConnection):
						intersections[subdIdx]+=maskedConnection(b, appended); continue
				#else (standalone body, normal case)
				intersections[subdIdx].append(otherId)
		#for master domain set list of interacting subdomains (could be handled above but for the sake of clarity complex if-else-if are avoided for now)
		if rank==0 and len(intersections[subdIdx])>0:
			intersections[0].append(subdIdx)
	#wprint( "found "+str(len(intrs))+" intersections"+str(intersections))
	return intersections

def updateRemoteStates(states, setBounded=False):
	'''
	update states of bodies handled by other workers, argument 'states' is a list of [id,state] (or [id,state,shape] conditionnaly)
	'''
	ids=[]
	for bst in states:
		#print bst[0],O.bodies[bst[0]]
		ids.append(bst[0])
		b=O.bodies[bst[0]]
		b.state=bst[1]
		#if SEND_SHAPES: b.shape=bst[2]
		if setBounded and not isinstance(b.shape,GridNode): b.bounded=True 
	return ids

def genUpdatedStates(b_ids):
	'''
	return list of [id,state] (or [id,state,shape] conditionnaly) to be sent to other workers
	'''
	return [[id,O.bodies[id].state] for id in b_ids] if not SEND_SHAPES else [[id,O.bodies[id].state,O.bodies[id].shape] for id in b_ids]



#############   COMMUNICATIONS   ################"

statesCommTime=0

def sendRecvStates():
	global statesCommTime
	start=time.time()
	#____1. get ready to receive positions from other subdomains
	pstates = []
	buf = [] #heuristic guess, assuming number of intersecting is ~linear in the number of rows, needs
		
	if rank!=0: #the master process never receive updated states (except when gathering)
		for otherDomain in O.subD.intersections[rank]:
			if len(O.subD.mirrorIntersections[otherDomain])==0:
				continue #can happen if MINIMAL_INTERSECTIONS
			if not USE_CPP_MPI:
				buf.append(bytearray(1<<22)) #FIXME: smarter size? this is for a few thousands states max (empirical); bytearray(1<<24) = 128 MB 
				pstates.append( comm.irecv(buf[-1],otherDomain, tag=_ID_STATE_SHAPE_))  #warning leaving buffer size undefined crash for large subdomains (MPI_ERR_TRUNCATE: message truncated)
			else:
				O.subD.mpiIrecvStates(otherDomain) #use yade's messages (coded in cpp)
	#____2. broadcast new positions (should be non-blocking if n>2, else lock) - this includes subdomain bodies intersecting the current one	
	reqs=[]
	for k in O.subD.intersections[rank]:
		if k==rank or k==0: continue #don't broadcast to itself... OTOH this list intersections[rank] will be used to receive
		if len(O.subD.intersections[k])==0:
			continue #can happen if MINIMAL_INTERSECTIONS
		#if len(b_ids)>0:#skip empty intersections, it means even the bounding boxes of the corresponding subdomains do not overlap
		wprint("sending "+str(len(O.subD.intersections[k]))+" states to "+str(k))
		if not OPTIMIZE_COM:
			timing_comm.send("sendRecvStates",genUpdatedStates(O.subD.intersections[k]), dest=k, tag=_ID_STATE_SHAPE_) #should be non-blocking if n>2, else lock?
		else:
			if not USE_CPP_MPI:
				reqs.append(comm.isend(O.subD.getStateValues(k), dest=k, tag=_ID_STATE_SHAPE_)) #should be non-blocking if n>2, else lock?
			else:
				timing_comm.mpiSendStates("mpiSendStates",k)
	for r in reqs: r.wait() #empty if USE_CPP_MPI
		
	#____3. receive positions and update bodies
	
	if rank==0: return #positions sent from master, done. Will receive forces instead of states
	if not USE_CPP_MPI:
		nn=0	
		for ss in pstates:
			states=ss.wait()
			if not OPTIMIZE_COM:
				updateRemoteStates(states)
			else:
				O.subD.setStateValuesFromIds(O.subD.mirrorIntersections[O.subD.intersections[rank][nn]],states)
				nn+=1
	else:
		
		for otherDomain in O.subD.intersections[rank]:
			if len(O.subD.mirrorIntersections[otherDomain])==0: continue #can happen if MINIMAL_INTERSECTIONS
			timing_comm.mpiWaitReceived("mpiWaitReceived(States)",otherDomain)
			O.subD.setStateValuesFromBuffer(otherDomain)
	statesCommTime+=(time.time()-start)

def isendRecvForces():
	'''
	Communicate forces from subdomain to master
	Warning: the sending sides (everyone but master) must wait() the returned list of requests
	'''	
	O.freqs=[] #keep that one defined even if empty, it is accessed in other functions
	#TDOD: FORCES FROM FLUID DOMAIN BOXES!!!!
	if ACCUMULATE_FORCES:
		if rank!=0:
			if not 0 in O.subD.intersections[rank]: return
			if FLUID_COUPLING: 
				forces0 = []
				for id in O.subD.mirrorIntersections[0]: 
					if not isinstance(O.bodies[id].shape, FluidDomainBbox): 
						forces0.append([id, O.forces.f(id), O.forces.t(id)])
			else:
				forces0=[[id,O.forces.f(id),O.forces.t(id)] for id in  O.subD.mirrorIntersections[0]]
			#wprint ("worker "+str(rank)+": sending "+str(len(forces0))+" "+str("forces to 0 "))
			#O.freqs.append(comm.isend(forces0, dest=0, tag=_FORCES_))
			timing_comm.send("isendRecvForces",forces0, dest=0, tag=_FORCES_)
		else: #master
			receiveForces(O.subD.intersections[0])

def waitForces():
	'''
	wait until all forces are sent to master. 
	O.freqs is empty for master, and for all threads if not ACCUMULATE_FORCES
	'''
	for r in O.freqs: r.wait()


##### INITIALIZE MPI #########

# Flag used after import of this module, turned True after scene is distributed
O.splitted=False
O.splittedOnce=False #after the first split we have additional bodies (Subdomains) and engines in the merged scene, use this flag to know

def mergeScene():
	
	if(rank==0 and waitingCommands):
		sendCommand("slaves","mergeScene()",False)
	if O.splitted:
		if MERGE_W_INTERACTIONS or ERASE_REMOTE_MASTER or DISTRIBUTED_INSERT:
			O.subD.mergeOp()
			sendRecvStatesRunner.dead = isendRecvForcesRunner.dead = waitForcesRunner.dead = collisionChecker.dead = True
			O.splitted=False
			collider.doSort = True
			if (AUTO_COLOR): colorDomains()
			global NUM_MERGES; NUM_MERGES +=1; 
		else:
			if rank>0:
				# Workers
				send_buff=np.asarray(O.subD.getStateBoundsValuesFromIds([b.id for b in O.bodies if b.subdomain==rank]))
				size=np.array(len(send_buff),dtype=int)
			else:
				#Master
				send_buff=np.array([0])
				size=np.array(0,dtype=int)
			sizes=np.empty(numThreads,dtype=int)
			# Master get sizes from all workers
			timing_comm.Gather("mergeScene_sizes",size,sizes,root=0)
			
			
			if(rank==0):
				# MASTER
				# Alloc sizes for workers 
				dat=np.ones(sizes.sum(),dtype=np.float64)
				# Displacement indexes where data should be stored/received in targeted array
				# dspl should be visible by everyone
				dspl=np.empty(numThreads, dtype=int)
				dspl[0] = 0
				for i in range(1, len(sizes)):
					dspl[i] = dspl[i-1] + sizes[i-1];
			else:
				dspl=None
				dat=None
			# data sent = [data, size of data] (for each worker)
			# data recv = [allocated target_array, array of different sizes, displacement, data type]
			timing_comm.Gatherv("mergeScene_data",[send_buff, size], [dat, sizes, dspl, MPI.DOUBLE], root=0)
			if(rank==0): #master
				for worker_id in range(1, numThreads):
					# generate corresponding ids (order is the same for both master and worker)
					#ids = [b.id for b in O.bodies if b.subdomain==worker_id]
					ids = O.bodies[O.subD.subdomains[worker_id-1]].shape.ids+[O.subD.subdomains[worker_id-1]] #faster than looping on all bodies
					#if (O.bodies[O.subD.subdomains[worker_id-1]].shape.ids+[O.subD.subdomains[worker_id-1]] != ids):
						#print("______________INCONSISTENCY!______________",ids," vs. ",O.bodies[O.subD.subdomains[worker_id-1]].shape.ids+[O.subD.subdomains[worker_id-1]])
					shift = dspl[worker_id];
					if (worker_id != numThreads-1):
						shift_plus_one = dspl[worker_id+1];
					else:		
						shift_plus_one = len(dat);
					O.subD.setStateBoundsValuesFromIds(ids,dat[shift: shift_plus_one]);
					reboundRemoteBodies(ids)
			# turn mpi engines off
			sendRecvStatesRunner.dead = isendRecvForcesRunner.dead = waitForcesRunner.dead = collisionChecker.dead = True
			O.splitted=False
			collider.doSort = True
		
		if (AUTO_COLOR): colorDomains()
		if rank==0: O.engines = O.initialEngines


def splitScene(): 
	'''
	Split a monolithic scene into distributed scenes on threads.
	
	Precondition: the bodies have subdomain no. set in user script
	'''
	if not COPY_MIRROR_BODIES_WHEN_COLLIDE: mprint("COPY_MIRROR_BODIES_WHEN_COLLIDE=False is not supported")
	#if not ERASE_REMOTE: mprint("ERASE_REMOTE=False is not supported")
	if not O.splittedOnce:
		O.initialEngines = O.engines
		if DOMAIN_DECOMPOSITION: #if not already partitionned by the user we partition here
			if rank == 0:
				import yade.bisectionDecomposition as dd
				decomposition = dd.decompBodiesSerial(comm) 
				decomposition.partitionDomain(fibreList) 
		maxid = len(O.bodies)-1
		if DISTRIBUTED_INSERT: #find max id before inserting subdomains
			maxid = timing_comm.allreduce("splitScene",maxid,op=MPI.MAX)
			wprint("Splitting with maxId=",maxid)
		if rank == 0 or DISTRIBUTED_INSERT:
			subdomains=[] #list subdomains by body ids
			#insert "meta"-bodies
			for k in range(1,numThreads):
				domainBody=Body(shape=Subdomain(ids=[b.id for b in O.bodies if b.subdomain==k]),subdomain=k) #note: not clear yet how shape.subDomainIndex and body.subdomain should interact, currently equal values
				domainBody.isSubdomain=True
				if rank==k: O._sceneObj.subD=domainBody.shape
				subdomains.append(O.bodies.insertAtId(domainBody,maxid+k))
				
			if rank==0:  O._sceneObj.subD = Subdomain()  # make sure it's initialized here
			O.subD = O._sceneObj.subD
			O.subD.subdomains = subdomains
			subD= O.subD #alias
			subD.comm=comm #make sure the c++ uses the merged intracommunicator
			
			masterBodies = [b.id for b in O.bodies if b.subdomain==0] #for VTKRecorderParallel, easier to loop through the owned bodies. 
			subD.setIDstoSubdomain(masterBodies)
			
			
			#tell the collider how to handle this new thing
			collider = typedEngine("InsertionSortCollider")
			if FLUID_COUPLING: 
				collider.boundDispatcher.functors = collider.boundDispatcher.functors+[Bo1_FluidDomainBbox_Aabb()]
			collider.boundDispatcher.functors=collider.boundDispatcher.functors+[Bo1_Subdomain_Aabb()]
			collider.targetInterv=0
			collider.keepListsShort=True # probably not needed, O.bodies.insertAtId should turn it on automaticaly 
			O.bodies.useRedirection=True # idem
			O.bodies.allowRedirection=False
			
			#BEGIN Garbage (should go to some init(), usually done in collider.__call__() but in the mpi case we want to collider.boundDispatcher.__call__() before collider.__call__()
			collider.boundDispatcher.sweepDist=collider.verletDist;
			collider.boundDispatcher.minSweepDistFactor=collider.minSweepDistFactor;
			collider.boundDispatcher.targetInterv=collider.targetInterv;
			collider.boundDispatcher.updatingDispFactor=collider.updatingDispFactor;
			#END Garbage
		if not DISTRIBUTED_INSERT: #we send scene from master to all workers
			sceneAsString= O.sceneToString() if rank==0 else None
			sceneAsString=timing_comm.bcast("splitScene",sceneAsString,root=0)
			if rank > 0: 
				O.stringToScene(sceneAsString) #receive a scene pre-processed by master (i.e. with appropriate body.subdomain's)  
				# as long as subD.subdomains isn't serialized we need to rebuild it here since it's lost
				domainBody=None
				subdomains=[] #list of subdomains by body id
				for b in O.bodies:
					if b.isSubdomain:
						subdomains.append(b.id)
						if b.subdomain==rank: domainBody=b
				if domainBody==None: wprint("SUBDOMAIN NOT FOUND FOR RANK=",rank)
				O._sceneObj.subD = domainBody.shape
				O.subD = O._sceneObj.subD
				O.subD.subdomains = subdomains
				subD = O.subD

		if FLUID_COUPLING: 
			fluidCoupling = typedEngine("FoamCoupling")
			fluidCoupling.comm = comm
			fluidCoupling.setIdList(fluidBodies)
			fluidCoupling.couplingModeParallel = True


		O._sceneObj.subdomain = rank
		O.subD.comm=comm #make sure the c++ uses the merged intracommunicator
		
		O.subD.init() 
		wprint("to parallel collide")
		parallelCollide()
		wprint("end parallel collide")
		
		# insert states communicator after newton 
		idx = O.engines.index(typedEngine("NewtonIntegrator"))
		O.engines=O.engines[:idx+1]+[PyRunner(iterPeriod=1,initRun=True,command="sys.modules['yade.mpy'].sendRecvStates();  ",label="sendRecvStatesRunner")]+O.engines[idx+1:]
		
		# insert force communicator before Newton
		O.engines=O.engines[:idx]+[PyRunner(iterPeriod=1,initRun=True,command="sys.modules['yade.mpy'].isendRecvForces()",label="isendRecvForcesRunner")]+O.engines[idx:]

		# append engine waiting until forces are effectively sent to master
		O.engines=O.engines+[PyRunner(iterPeriod=1,initRun=True,command="pass",label="waitForcesRunner")]
		O.engines=O.engines+[PyRunner(iterPeriod=1,initRun=True,command="if sys.modules['yade.mpy'].checkAndCollide(): O.pause();",label="collisionChecker")]

		O.splittedOnce = True
		O.splittedEngines = O.engines
	else: 
		if (DOMAIN_DECOMPOSITION and RESET_SUBDOMAINS_WHEN_COLLIDE):
			if rank == 0:
				import yade.bisectionDecomposition as dd
				decomposition = dd.decompBodiesSerial(comm) 
				decomposition.partitionDomain() 
			O.subD.splitBodiesToWorkers(RESET_SUBDOMAINS_WHEN_COLLIDE)
			parallelCollide()
		if rank == 0 :
			O.engines = O.splittedEngines
			O.interactions.clear()
			unboundRemoteBodies()
			if (ERASE_REMOTE and ERASE_REMOTE_MASTER): eraseRemote()
		sendRecvStatesRunner.dead = isendRecvForcesRunner.dead = waitForcesRunner.dead = collisionChecker.dead = False
	
	O.splitted = True
		
def updateAllIntersections():
	subD=O.subD
	subD.intersections=genLocalIntersections(subD.subdomains)
	if USE_CPP_INTERS: #to be used in case of  possible issue with older mpi4py versions, this is slightly faster.  
		subD.getMirrorIntrs() 
		
	else: 
	#update mirror intersections so we know message sizes in advance
		subD.mirrorIntersections=[[] for n in range(numThreads)]
		if rank==0:#master domain
			for worker in range(1,numThreads):#FIXME: we actually don't need so much data since at this stage the states are unchanged and the list is used to re-bound intersecting bodies, this is only done in the initialization phase, though
				#wprint("sending mirror intersections to "+str(worker)+" ("+str(len(subD.intersections[worker]))+" bodies), "+str(subD.intersections[worker]))
				m = O.intrsctToBytes(subD,worker,False) if SEND_BYTEARRAYS else  subD.intersections[worker];
				timing_comm.send("sendIntersections",m, dest=worker, tag=_MIRROR_INTERSECTIONS_)
		else:
			# from master
			b_ids=comm.recv(source=0, tag=_MIRROR_INTERSECTIONS_)
			wprint("Received mirrors from master: ",b_ids)
			#FIXME: we are assuming that Body::id_t is 4 bytes here, not that portable...
			numInts0= int(len(b_ids)/4) if SEND_BYTEARRAYS else len(b_ids)  #ints = 4 bytes
			
			if numInts0>0:
				if SEND_BYTEARRAYS:
					O.bufferFromIntrsct(subD,0,numInts0,True)[:]=b_ids
					b_ids=np.frombuffer(b_ids,dtype=np.int32)
				else:
					subD.mirrorIntersections= [b_ids]+subD.mirrorIntersections[1:]

				#reboundRemoteBodies(b_ids)
				# since interaction with 0-bodies couldn't be detected before, mirror intersections from master will
				# tell if we need to wait messages from master (and this is declared via intersections) 
				if not 0 in subD.intersections[rank]:
					temp=subD.intersections[rank]
					temp+=[0]
					subD.intersections=subD.intersections[:rank]+[temp]+subD.intersections[rank+1:]
				else:
					if not O.splittedOnce: mprint("0 already in intersections (should not happen)")
			reqs=[]
			
			#from workers
			for worker in subD.intersections[rank]:
				if worker==0: continue #already received above
				#wprint("subD.intersections["+str(rank)+"]: "+str(subD.intersections[rank]))
				#buf = bytearray(1<<22) #CRITICAL
				#FIXME : using 'None' as a buffer
				reqs.append([worker,comm.irecv(None, worker, tag=_MIRROR_INTERSECTIONS_)])

			for worker in subD.intersections[rank]:
				if worker==0: continue #we do not send positions to master, only forces
				#wprint("sending "+str(len(subD.intersections[worker]))+" states to "+str(worker))
				wprint("Send mirrors to: ", worker)
				m = O.intrsctToBytes(subD,worker,False) if SEND_BYTEARRAYS else  subD.intersections[worker];
				timing_comm.send("splitScene_intersections", m, dest=worker, tag=_MIRROR_INTERSECTIONS_)
			for req in reqs:
				intrs=req[1].wait()
				if SEND_BYTEARRAYS:
					wprint("Received mirrors from: ", req[0], " : ",np.frombuffer(intrs,dtype=np.int32))
					O.bufferFromIntrsct(subD,req[0],int(len(intrs)/4),True)[:]=intrs
					intrs=np.frombuffer(intrs,dtype=np.int32)
				else:
					subD.mirrorIntersections = subD.mirrorIntersections[0:req[0]]+[intrs]+subD.mirrorIntersections[req[0]+1:]
				#reboundRemoteBodies(intrs)

bodiesToImport=[]

def parallelCollide():
	global bodiesToImport
	subD=O.subD
	start = time.time()
	if (not O.splitted):
		unboundRemoteBodies()
		eraseRemote()
		
	collider.boundDispatcher.__call__()
	updateDomainBounds(subD.subdomains) #triggers communications
	collider.__call__() #see [1]
	unboundRemoteBodies() #in splitted stage we exploit bounds to detect bodies which are no longer part of intersections (they will be left with no bounds after what follows)
	updateAllIntersections()  #triggers communications
	if rank!=0:
		for l in subD.mirrorIntersections:
			if len(l)>0:
				reboundRemoteBodies(l)
	if(ERASE_REMOTE): eraseRemote() # erase the bodies which still have no bounds
		
	"""
	" NOTE: FK, what to do here:
	" 1- all threads loop on reqs, i.e the intersecting subdomains of the current subdomain.
	" 2- during this loop, check whether the current subdomain needs bodies from the intersecting ones
	" 3- in all cases, isend the ids needed, (if no ids needed send empty array)
	" 3.1- build a ranks list "requestedSomethingFrom" to loop later on to receive data
	" 4- loop again on reqs to get the ids needed by other subdomains (with blocking recv as we used isend)
	" 5- if the data recved is empty (nothing requested), do nothing. Else isend the bodies (c++)
	" 6- loop on "requestedSomethingFrom" ranks and recv the bodies (blocking, c++, using MPI_Probe to know the message size)
	" 7- comm.barrier(), just in case
	"""
	if(COPY_MIRROR_BODIES_WHEN_COLLIDE or MERGE_W_INTERACTIONS):
		requestedSomethingFrom=[]
		bodiesToImport=[[] for worker in range(numThreads)]
		sent=[]
		if rank>0: #master doesn't need bodies
			#reqs=subD.intersections[rank]
			for worker in subD.intersections[rank]:
				#worker=req[0]
				#if(worker==0):continue
				for mirrorBodyId in subD.mirrorIntersections[worker]:
					if O.bodies[mirrorBodyId]==None:
						bodiesToImport[worker]+=[mirrorBodyId]
				if(len(bodiesToImport[worker])>0):
					requestedSomethingFrom.append(worker)
				wprint("I request ids: ",len(bodiesToImport[worker]), " from ",worker)
				sent.append(comm.isend(bodiesToImport[worker], worker, tag=_MIRROR_INTERSECTIONS_))
		wprint("will wait requests from ",subD.intersections[rank])
		for worker in subD.intersections[rank]:
			if worker!=0:
				wprint("waiting requests from ",worker)
				requestedIds=timing_comm.recv("parallelCollide",source=worker, tag=_MIRROR_INTERSECTIONS_)
				wprint("requested: ",len(requestedIds),"from ",worker)
				if(len(requestedIds)>0):
					wprint("will now send ",len(requestedIds)," to ",worker)
					subD.sendBodies(worker,requestedIds)
		for worker in requestedSomethingFrom:
			subD.receiveBodies(worker)
		for s in sent:
			s.wait()
		subD.completeSendBodies();
		
	if not collider.keepListsShort: collider.doSort = True
	collider.__call__()
	collider.execTime+=int((time.time()-start)*1e9)
	collider.execCount+=1
	try:
		collisionChecker.execTime-=int((time.time()-start)*1e9)
	except:
		pass

	#maxVelocitySq is normally reset in NewtonIntegrator in the same iteration as bound dispatching, since Newton will not run before next iter in our case we force that value to avoid another collision detection at next step
	typedEngine("NewtonIntegrator").maxVelocitySq=0.5


def eraseRemote(): 
	if rank>0 or ERASE_REMOTE_MASTER: # suppress external bodies from scene
		#numBodies = len(O.bodies)
		#for id in range(numBodies):
		#mprint("will erase ",[b.id for b in O.bodies if (not b.bounded and b.subdomain!=rank)])
		for b in O.bodies:
			if not b.bounded and b.subdomain != rank:
				connected = False #a gridNode could be needed as part of interacting facet/connection even if not overlaping a specific subdomain. Assume connections are always bounded for now, we thus only check nodes.
				if isinstance(b.shape,GridNode):
					for f in b.shape.getPFacets():
						if f.bounded: connected = True
					for c in b.shape.getConnections():
						if c.bounded: connected = True
				if not connected:
					O.bodies.erase(b.id)

##### RUN MPI #########
def mpirun(nSteps,np=None,withMerge=False):
	'''
	Parallel version of O.run() using MPI domain decomposition.
	
	Parameters
	
	nSteps : The numer of steps to compute
	np :  number of mpi workers (master+subdomains), if=1 the function fallback to O.run()
	withMerge : wether subdomains should be merged into master at the end of the run (default False). If True the scene in the master process is exactly in the same state as after O.run(nSteps,True). The merge can be time consumming, it is recommended to activate only if post-processing or other similar tasks require it.
	'''
	
	if comm==None: configure()
	if np==None: np=numThreads
	if(np==1):
		mprint("single-core, fall back to O.run()")
		O.run(nSteps,True)
		return
	
	# DIRTY HACK: this is to detect when we run regression tests and prevent mpirun to enter a parallel testing spree
	#stack=inspect.stack()
	#global userScriptInCheckList
	#if len(stack[3][1])>12 and stack[3][1][-12:]=="checkList.py":
		#userScriptInCheckList=stack[1][1]
	caller_name = inspect.stack()[2][3]
	#mprint("stack", stack)
	
	if (np>numThreads):  
		if numThreads==1: initialize(np) #this will set numThreads
		else: mprint("number of mpy cores can't be increased when already initialized")
	if(rank==0 and not caller_name=='execfile'): #if the caller is the user's script, everyone already calls mpirun and the workers are not waiting for a command.
		waitingCommands=True
		for w in range(1,numThreads):
			comm.send("yade.mpy.mpirun(nSteps="+str(nSteps)+",withMerge="+str(withMerge)+")",dest=w,tag=_MASTER_COMMAND_)
			wprint("Command sent to ",w)
	
	
	if FLUID_COUPLING:
		fluidCoupling = typedEngine("FoamCoupling") 
		fluidCoupling.comm = comm 
		fluidCoupling.getFluidDomainBbox() #triggers the communication between yade procs and Yales2/openfoam procs, get's fluid domain bounding boxes from all fluid procs. 
	
	# split if needed
	initStep = O.iter
	if not O.splitted:
		wprint("splitting")
		splitScene()
		wprint("splitted")
		
	O.timingEnabled=YADE_TIMING #turn it ON/OFF
		
	# run iterations
	if not (MERGE_SPLIT):
		O.run(nSteps,True) 
		if withMerge: mergeScene() #will be useful to see evolution in QGLViewer, for instance
	else: #merge/split or body_copy for each collider update
		collisionChecker.dead=True
		while (O.iter-initStep)<nSteps:
			O.step()
			if checkAndCollide():
				mergeScene()
				splitScene()
		mergeScene()
	
	# report performance
	if YADE_TIMING and (rank<=MAX_RANK_OUTPUT or rank>=(numThreads-MAX_RANK_OUTPUT)):
		timing_comm.print_all()
		from yade import timing
		time.sleep((numThreads-rank)*0.002) #avoid mixing the final output, timing.stats() is independent of the sleep
		mprint( "#####  Worker "+str(rank)+"  ######")
		timing.stats() #specific numbers for -n4 and gabion.py

#######################################
#######  Bodies re-allocation  ########
#######################################

def runOnSynchronouslPairs(workers,command):
	'''
	Locally (from one worker POV), this function runs interactive mpi tasks defined by 'command' on a list of other workers (typically the list of interacting subdomains).
	Overall, peer-to-peer connexions are established so so that 'command' is executed symmetrically and simultaneously on both sides of each worker pair. I.e. if worker "i" executes "command" with argument "j" (index of another worker), then by design "j" will execute the same thing with argument "i" *simultaneously*.
	
	In many cases a similar series of data exchanges can be obtained more simply (and fastly) with asynchronous irecv+send like below.

	for w in workers:
		m=comm.irecv(w)
		comm.send(data,dest=w)
	
	The above only works if the messages are all known in advance locally, before any communication. If the interaction with workers[1] depends on the result of a previous interaction with workers[0] OTOH, it needs synchronous execution, hence this function. Synchronicity is also required if more than one blocking call is present in 'command', else an obvious deadlock as if 'irecv' was replaced by 'recv' in that naive loop.
	Both cases occur with the 'medianFilter' algorithm, hence why we need this synchronous method.
	
	In this function pair connexions are established by the workers in a non-supervized and non-deterministic manner. Each time an interactive communication (i,j) is established 'command' is executed simultaneously by i and j. It is guaranted that all possible pairs are visited.
	
	The function can be used for all-to-all operations (N^2 pairs), but more interestingly it works with workers=intersections[rank] (O(N) pairs). It can be tested with the dummy funtion 'pairOp': runOnSynchronouslPairs(range(numThreads),pairOp)
	
	command:
		a function taking index of another worker as argument, can include blocking communications with the other worker since runOnSynchronouslPairs guarantee that the other worker will be running the command symmetrically. 
	'''
	global t1
	t1=time.time()
	workersTemp = list(workers) #we will remove elements from this copy as we proceed
	if rank in workersTemp: workersTemp.remove(rank) #don't talk to yourself
	#skip master (master is useless typically if we are reallocating bodies, we skip it here although the function would work with master to)
	if True:
		if 0 in workersTemp: workersTemp.remove(0) #don't talk to master
		if rank==0: return #don't talk if you are master
	
	# now call the workers one by one until one of them reacts
	s=MPI.Status();
	sentTo=-1  #last worker to which connexion request was sent
	while len(workersTemp)>0:
		other=workersTemp[-1]
		connected=False
		if rank>other and other!=sentTo:
			rs = comm.issend(None,dest=other,tag=_GET_CONNEXION_)
			sentTo = other
		
		while not (connected):
			if comm.Iprobe(source=MPI.ANY_SOURCE, status=s,tag=_GET_CONNEXION_):
				data = timing_comm.recv("runOnSynchronouslPairs",None,s.source,tag=_GET_CONNEXION_)
				talkTo = s.source
				connected = True
			elif rank>other:
				if rs.test()[0]:
					talkTo = other
					rs.wait()
					connected=True
		
		command(talkTo)
		workersTemp.remove(talkTo)
	#mprint("TOTAL TIME(",rank,"):",time.time()-t1)

# a dummy test function for runOnSynchronouslPairs(range(numThreads,pairOp)
def pairOp(talkTo):
	global t1
	# send/recv data 
	message="haha"
	comm.isend(message,talkTo,tag=_PAIR_OP_)
	feedback = timming_comm.recv("pair_op",None,talkTo,tag=_PAIR_OP_)
	# crunch feedback and numbers...
	time.sleep(0.01)
	# send/recv result
	comm.isend(message,talkTo,tag=_PAIR_OP_)
	feedback = timing_comm.recv("pair_op",None,talkTo,tag=_PAIR_OP_)
	print("(",rank,talkTo,") done in",time.time()-t1,"s")

def migrateBodies(ids,origin,destination):
	'''
	Reassign bodies from origin to destination. The function has to be called by both origin (send) and destination (recv).
	Note: subD.completeSendBodies() will have to be called after a series of reassignement since subD.sendBodies() is non-blocking
	'''
	
	ts = time.time() 
	
	if rank==origin:
		if USE_CPP_REALLOC: 
			O.subD.migrateBodiesSend(ids, destination)
	  
		else:
			thisSubD = O.subD.subdomains[rank-1]
			for id in ids:
				if not O.bodies[id]: mprint("reassignBodies failed,",id," is not in subdomain ",rank)
				O.bodies[id].subdomain = destination
				createInteraction(thisSubD,id,virtualI=True) # link translated body to subdomain, since there is initially no interaction with local bodies
				#for k in O.subD.intersections[rank]:
					#if k==0: continue
					#if id in O.subD.intersections[k]:
						#O.subD.intersections[k].remove(id) # so we don't send the same body to multiple domains...
			O.subD.sendBodies(destination,ids)
	elif rank==destination:
		O.subD.receiveBodies(origin)
	te = time.time() 
	
	#mprint("time in migrateBodies-->  ", te-ts, "  rank = ", rank)
	

def projectedBounds(i,j):
	'''
	Returns sorted list of projections of bounds on a given axis, with bounds taken in i->j and j->i intersections
	'''
	useAABB=False #using center of subdomain AABB is a bit unstable since the movement of on single body can change it greatly
	if (useAABB):
		pt1 = 0.5*(O.bodies[O.subD.subdomains[i-1]].bound.min+O.bodies[O.subD.subdomains[i-1]].bound.max)
		pt2 = 0.5*(O.bodies[O.subD.subdomains[j-1]].bound.min+O.bodies[O.subD.subdomains[j-1]].bound.max)
	else: #use center of mass
		pt1 = O.subD._centers_of_mass[i]
		pt2 = O.subD._centers_of_mass[j]
	axis=pt2-pt1
	axis.normalize()
	pos = [[O.subD.boundOnAxis(O.bodies[k].bound,axis,True),i,k] for k in O.subD.intersections[j]]+[[O.subD.boundOnAxis(O.bodies[k].bound,axis,False),j,k] for k in O.subD.mirrorIntersections[j]]
	pos.sort(key= lambda x: x[0])
	return pos

def medianFilter(i,j):
	'''
	Returns bodies in "i" to be assigned to "j" based on median split between the center points of subdomain's AABBs
	'''
	bodiesToSend=[]
	bodiesToRecv=[]
	
	ts = time.time()
	
	if USE_CPP_REALLOC: 
		useAABB = False; 
		otherSubDCM = O.subD._centers_of_mass[j]
		subDCM = O.subD._centers_of_mass[i]
		bodiesToSend= O.subD.medianFilterCPP(bodiesToRecv,j, otherSubDCM, subDCM, useAABB)
		
	else:
		pos = projectedBounds(i,j)
		# we will start from first and last elements and converge to middle, to check possible inversions of bboxes along the axis
		xminus=0; xplus=len(pos)-1
		while (xminus<xplus):
			while (pos[xminus][1]==i and xminus<xplus): xminus+=1
			while (pos[xplus][1]==j and xminus<xplus): xplus-=1
			if xminus<xplus:
				bodiesToSend.append(pos[xplus][2])
				bodiesToRecv.append(pos[xminus][2])
				pos[xminus][1]=i
				pos[xplus][1]=j
				xminus+=1; xplus-=1
	#if len(bodiesToSend)>0: mprint("will send ",len(bodiesToSend)," to ",j," (and recv ",len(bodiesToRecv),")")
	te = time.time() 
	
	#mprint("time in median filter -->  ", te-ts, "  rank = ", rank)
	
	return bodiesToSend,bodiesToRecv

REALLOCATE_FILTER=medianFilter #that's currently default and only option

def reallocateBodiesToSubdomains(_filter=medianFilter,blocking=True):
	'''
	Re-assign bodies to subdomains based on '_filter' argument.
	Requirement: '_filter' is a function taking ranks of origin and destination and returning the list of bodies (by index) to be moved. That's where the decomposition strategy is defined. See example medianFilter (used by default).
	This function must be called in parallel, hence if ran interactively the command needs to be sent explicitely:
	mp.sendCommand("all","reallocateBodiesToSubdomains(medianFilter)",True)
	'''
	
	O.subD._centers_of_mass=[Vector3(0,0,0) for k in range(numThreads)]
	O.subD._centers_of_mass[rank]=O.subD.centerOfMass()
	
	if blocking: # the filter will be applied sequentially to each other domain. It can include blocking communications in subdomain pairs
		_functor = lambda x : reallocateBodiesPairWiseBlocking(medianFilter,x)
		runOnSynchronouslPairs(O.subD.intersections[rank],_functor)
	else: 	# non-blocking method, migrated bodies are decided unilateraly by each subdomain
		# if using medianFilter it leads to non-constant bodies per subdomain
		if rank>0:
			for worker in O.subD.intersections[rank]:
				if worker==0: continue
				candidates = _filter(rank,worker)
				wprint("sending to ",worker,": ",len(candidates))
				migrateBodies(candidates,rank,worker) #send
				migrateBodies(None,worker,rank)       #recv
				
	O.subD.completeSendBodies()
	
	ts = time.time()
	if USE_CPP_REALLOC:
		O.subD.updateLocalIds(ERASE_REMOTE_MASTER)
		if not ERASE_REMOTE_MASTER:
			if rank == 0 : 
				if (AUTO_COLOR) : colorDomains()
			updateAllIntersections()
			
	else:
		O.subD.ids = [b.id for b in O.bodies if (b.subdomain==rank and not b.isSubdomain)] #update local ids

		if not ERASE_REMOTE_MASTER:
			# update remote ids in master
			if rank>0: req = comm.isend(O.subD.ids,dest=0,tag=_ASSIGNED_IDS_)
			else: #master will update subdomains for correct display (besides, keeping 'ids' updated for remote subdomains may not be a strict requirement)
				for k in range(1,numThreads):
					ids=comm.recv(source=k,tag=_ASSIGNED_IDS_)
					O.bodies[O.subD.subdomains[k-1]].shape.ids=ids
					for i in ids: O.bodies[i].subdomain=k
				if (AUTO_COLOR): colorDomains()
			# update intersections and mirror
			updateAllIntersections() #triggers communication
			if rank>0: req.wait()


def reallocateBodiesPairWiseBlocking(_filter,otherDomain):
	'''
	Re-assign bodies from/to otherDomain based on '_filter' argument.
	Requirement: '_filter' is a function taking ranks of origin and destination and returning the list of bodies (by index) to be moved. That's where the decomposition strategy is defined. See example medianFilter (used by default).
	'''
	#if rank==0: return
	ts = time.time()
	if True: #clean intersections, remove bodies already moved to other domain
	  
		if USE_CPP_REALLOC: 
			O.subD.cleanIntersections(otherDomain)
		else: 
			ints = [ii for ii in O.subD.intersections[otherDomain] if O.bodies[ii].subdomain==rank] #make sure we don't send ids of already moved bodies
			O.subD.intersections=O.subD.intersections[:otherDomain]+[ints]+O.subD.intersections[otherDomain+1:]
	
	te = time.time() 
	
	#mprint("time in clear intrs -->  ", te-ts, "  rank = ", rank)
	
	req = comm.irecv(None,otherDomain,tag=_MIRROR_INTERSECTIONS_)
	timing_comm.send("reallocateBodiesPairWiseBlocking",[O.subD.intersections[otherDomain],O.subD._centers_of_mass[rank]],dest=otherDomain,tag=_MIRROR_INTERSECTIONS_)
	newMirror = req.wait()
	
	ts = time.time() 
	
	if USE_CPP_REALLOC:
		O.subD.updateNewMirrorIntrs(otherDomain, newMirror[0])
	else:
		O.subD.mirrorIntersections=O.subD.mirrorIntersections[:otherDomain]+[newMirror[0]]+O.subD.mirrorIntersections[otherDomain+1:]
	te = time.time() 
	
	#mprint("time in mirrorUpdate  ", te-ts, "  rank = ", rank)
	
	O.subD._centers_of_mass[otherDomain]=newMirror[1]
	
	candidates,mirror = _filter(rank,otherDomain)
	
	#mprint("Will send ",candidates)
	#req2=comm.irecv(None,otherDomain,tag=_MIRROR_INTERSECTIONS_)
	#comm.send(candidates,dest=otherDomain,tag=_MIRROR_INTERSECTIONS_)
	#mirrorCandidates2 = req2.wait()
	#req2.wait()
	##mirrorCandidates2=req2.wait()
	#mprint("mirrorCandidates2 ",mirrorCandidates2)
	#if mirrorCandidates2==None: mirrorCandidates2=[] #sending empty list results in 'None' received... fix it here
	#if len(mirrorCandidates2)!=len(candidates):
		#mprint("UNBALANCED: will send",len(candidates)," and receive ",len(mirrorCandidates2))
	#if mirrorCandidates2!=mirror:
		#mprint("different contents:", mirror," ", mirrorCandidates2)
	
	#mprint("sending to ",otherDomain,": ",len(candidates))
	migrateBodies(candidates,rank,otherDomain) #send
	migrateBodies(None,otherDomain,rank)       #recv
	
	
