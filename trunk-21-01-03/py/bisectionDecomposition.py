'''
Module for domain decomposition based on the Orthogonal Recursive bisection Algorithm, see [1] and [2] for full details. 
Working : The master processor assigns subdomains determines the min and max bounds from all the bodies (excluding wall, subdomain and box), 
This space is the subdivided into regions based on a splitting axis. The axis of split is along the largest dimension and each split results into 
2 subregions** until a certain 'depth' based on the number of workers is reached.
The maximum depth/number of levels is defined as log2(N_{w}), where N_{w} is the number of workers. The algorithm can be used for any number of workers.

(c) Deepak Kunhappan, deepak.kn1990@gmail.com, deepak.kunhappan@3sr-grenoble.fr 

'''

from math import ceil, log10, log
from yade.wrapper import * 
from yade.minieigenHP import Vector3
# Note: importing mpi4py will scan environment variable for MPI flags (e.g. OMPI_MCA), so because of this import here bisectionDecomposition can't be imported before mpy.configure(), which may change env variables
from mpi4py import MPI 
import sys

sys.stderr.write=sys.stdout.write
minVal =  -1e+50
maxVal =  1e+50
maxDim = 3 
_TAG_DATA_=999
_TOL_ = 1e-08


# for coloring bodies. 
import yade.coloring as col

class decompBodiesSerial: 

	
	def __init__(self, mpiComm): 
		
		self.comm = mpiComm
		self.numWorkers = self.comm.Get_size() - 1
		# for coloring bodies 
		self.colorscale = col.getCol(self.numWorkers)
		

	def isEqual(self, a,b): 
		'''function to check if 2 floats are equal'''
		
		return abs(a-b) < _TOL_ 
	
	
	def ifPw2(self, numThreads):
		''' Determine if the number of workers is a power of 2 '''
		 
		if numThreads == 0 : return False 
		return numThreads & (numThreads-1) == 0
	
	def numThreadsinPw2(self, numThreads): 
		''' Express the number of workers as sum in poowers of 2, e.g: 11 = 2^{3} + 2^{1} + 2^{0} '''
		parts = [] 
		i = 1 
		while i <= numThreads: 
			if i & numThreads : parts.append(i) 
			i <<= 1 
		return parts 
	      
	def setPartList(self, lst, numThreads): 
		
		''' Used when the number of workers are not a power of 2. Bisectioning divides the space into 2 regions, hence equal divisions are created when the number of workers are in power of 2. If not
		one or more workers end up having more number of bodies compared to other workers, leading to a load imbalance. In-order to overcome this we first express the numW in terms sums of 2^{n}, with each 2^{n} defining 
		a sub group in which bisectioning can be done. 
		For example : we have 11 workers, so : 11 = 2^{0} + 2^{1} + 2^{3} = [1, 2, 8] 
		with 1 worker from 1 level of bisectioning,(a part of the domain is assigned to 1 worker) 
		2 workers from 1 level/depth of bisectioning
		8 workers from 3 levels/depth of bisectioning
		'''
		
		nb = len(lst)//numThreads 
		parts = self.numThreadsinPw2(numThreads) 
		wIds = [[] for x in range(len(parts))]
		for i in range(len(parts)-1): 
			splitAxis = self.domainPreProc(lst)
			lst = sorted(lst, key=lambda x:x[0][splitAxis])
			splitPt = len(lst)-(nb*parts[i])
			wIds[i] = lst[splitPt:]; lst = lst[:splitPt]
		wIds[len(parts)-1] = lst 
		return parts, wIds 
	
	def _partitionDomain(self, bList, numW, strideRank): 
	  
		''' The partitioning algorithm, sorts the list of bodies in increasing order based on a predetermined axis, (split axis)
		for each level. With one half allocated to one worker and the other to another
		at the final level. '''
		
		workerIds = [[] for x in range(numW)]
		workerIds[0] = bList 
		wProcs = [n for n in range(numW)] 
		numLevels = int(log(numW,2)) 
		nlevel = 0; depth = 0
		while (nlevel <= numLevels): 
			lpW2 = 1 << nlevel 
			lpW2_1 = 1 << (nlevel+1)
			sIndxs = [x for x in wProcs if x < lpW2] 
			rIndxs = [x for x in wProcs if x >= lpW2 and x < lpW2_1]
			for s,r in zip(sIndxs, rIndxs): 
				workerIds[s], workerIds[r] = self.partData(workerIds[s], nlevel)
			nlevel += 1 
		
		for i,x in enumerate(workerIds): 
			workerIds[i] = [bId[1] for bId in x]
		
		for i, blst in enumerate(workerIds): 
			for x in blst: 
				O.bodies[x].subdomain = (i+strideRank)+1
				O.bodies[x].shape.color = self.colorscale[(i+strideRank)-1] 

	
	def partData(self, bList, nlevel): 
	
		''' function to sort data and divide into 2 parts. The data is a list containing a tuple of (b.state.pos, b.id) '''
		
		splitAxis = self.domainPreProc(bList) 
		sortD = sorted(bList, key=lambda x:x[0][splitAxis]) 
		splitPt = len(sortD)//2 
		return sortD[:splitPt], sortD[splitPt:]
	
	
	def domainPreProc(self, data): 
	  
		''' determines the max extends of the domain, the splitting axis is based on the axis of the largest extend. Also used to determine the dimensionality of the 
		configuration, eg: 2D or 3D. 
		'''
		
		domainMin = Vector3(maxVal, maxVal, maxVal)
		domainMax = Vector3(minVal, minVal, minVal) 
		ndim = -1; symIndex = -1; domain2D = False 
		splitPlaneIndex = -1; maxDim = 3 
		
		for x in data: 
			pos = x[0] 
			
			domainMin[0] = min(domainMin[0], pos[0]) 
			domainMin[1] = min(domainMin[1], pos[1]) 
			domainMin[2] = min(domainMin[2], pos[2]) 
			
			domainMax[0] = max(domainMax[0], pos[0]) 
			domainMax[1] = max(domainMax[1], pos[1]) 
			domainMax[2] = max(domainMax[2], pos[2]) 
		
		for j in range(maxDim): 
			if (self.isEqual(domainMin[j], domainMax[j])): 
				domain2D = True
				symIndex = j 
				ndim = 2
			else : ndim = 3 
			
		domainAxisL = [abs(domainMax[0]-domainMin[0])] + [abs(domainMax[1]-domainMin[1])] + [abs(domainMax[2]-domainMin[2])]
		splitPlaneIndex = domainAxisL.index(max(domainAxisL))
		
		#TODO: find minimum of intersection areas between subdomains, the idea is to use this as a condition to determine if subdomains have to be reset. 
		interSectionAreas = [domainAxisL[1]*domainAxisL[2]]+[domainAxisL[0]*domainAxisL[2]]+[domainAxisL[0]*domainAxisL[1]]
		areaMin = None 
		if ndim == 3: 
			areaMin = interSectionAreas.index(min(interSectionAreas))
		
		else : 
			if symIndex == 0 : areaMin = min(domainAxisL[1], domainAxisL[2])
			if symIndex == 1 : areaMin = min(domainAxisL[2], domainAxisL[0])
			if symIndex == 2 : areaMin = min(domainAxisL[0], domainAxisL[1]) 
			
		
		return splitPlaneIndex
		


	def checkIfMasterBody(self,b): 
		''' check if the body is a boundary body or bodies that would be typically be in master. '''
		if (isinstance(b.shape, Wall)) : return True 
		elif (isinstance(b.shape, Box)) : return True 
		elif (isinstance(b.shape, FluidDomainBbox)) : return True 
		elif (isinstance(b.shape, Facet)) : return True 
		elif (isinstance(b.shape, Subdomain)) : return True 
		else : return False 

      
	def partitionDomain(self, fibreList = None): 
		''' The main driver function  '''
		
		blst = []
		if not fibreList: 
			blst = [(b.state.pos, b.id) for b in O.bodies if not self.checkIfMasterBody(b)]
		else : 
			blst = [(O.bodies[j.cntrId].state.pos, j.cntrId) for j in fibreList]
	
		if self.ifPw2(self.numWorkers) : 
			self._partitionDomain(blst, self.numWorkers, strideRank = 0)
		else : 
			parts, wIds = self.setPartList(blst, self.numWorkers)
			nr = self.numWorkers+1
			for i in range(len(parts)): 
				nr = nr - parts[i]
				self._partitionDomain(wIds[i], parts[i], nr-1)
				
				
		if fibreList : 
			for fib in fibreList: 
				for j in fib.nodes:
					O.bodies[j].subdomain = O.bodies[fib.cntrId].subdomain 
					O.bodies[j].shape.color = O.bodies[fib.cntrId].shape.color
					for intr in O.bodies[j].intrs(): 
						O.bodies[intr.geom.connectionBody.id].subdomain = O.bodies[fib.cntrId].subdomain 
						O.bodies[intr.geom.connectionBody.id].shape.color = O.bodies[fib.cntrId].shape.color
				

  


