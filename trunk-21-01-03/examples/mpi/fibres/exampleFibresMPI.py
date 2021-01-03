#Example showing how to run 'fiber' (gridNode+girdConnection) in Yade-MPI

from yade import utils
from yade.gridpfacet import *
from yade import mpy as mp


# create a Fiber class with attributes of the nodes it consists, the node which corresponds to the centre of mass, and a tuple 'segs' which consists the node pair forming a segment.
class Fibre : 
    def __init__(self): 
        self.numseg = 0 
        self.cntrId = -1 
        self.nodes = []
        self.segs = []
        
        
    
# The usual engines, without the timestepper.  (NOTE: will be fixed soon.)   
O.engines = [ForceResetter(), 
             InsertionSortCollider([Bo1_GridConnection_Aabb(),Bo1_PFacet_Aabb(), Bo1_Subdomain_Aabb()]), 
             InteractionLoop(
                 [Ig2_GridNode_GridNode_GridNodeGeom6D(),
                  Ig2_GridConnection_GridConnection_GridCoGridCoGeom(),
                  Ig2_GridConnection_PFacet_ScGeom(),
                  Ig2_Sphere_PFacet_ScGridCoGeom()], 
                 [Ip2_CohFrictMat_CohFrictMat_CohFrictPhys(setCohesionNow=True,setCohesionOnNewContacts=False),Ip2_FrictMat_FrictMat_FrictPhys()], 
                 [Law2_ScGeom6D_CohFrictPhys_CohesionMoment(label='fib_self'),
                  Law2_ScGeom_FrictPhys_CundallStrack(),
                  Law2_GridCoGridCoGeom_FrictPhys_CundallStrack()]
                 ), 
             NewtonIntegrator(damping = 0.0, label='newton', gravity = (0, -10, 0))
            ]

#materials as usual. 
young = 2e5
O.materials.append(CohFrictMat(young=young,poisson=0.3,density=1050,frictionAngle=radians(20),normalCohesion=1e100,shearCohesion=1e100,momentRotationLaw=True,label='spheremat'))
O.materials.append(FrictMat(young=young,poisson=0.3,density=1050,frictionAngle=radians(20),label='frictmat'))                 
O.materials.append(FrictMat(young=young,poisson=0.3,density=5000,frictionAngle=radians(30),label='facetmat'))
O.materials.append(CohFrictMat(young=young,poisson=0.3,density=5000,frictionAngle=radians(30),normalCohesion=1e100,shearCohesion=1e100,momentRotationLaw=True,label='facetnode'))


r = 2e-04       #fibre radius
ntot = 150     # total number of fibers 
nNodes = 11     # number of nodes per fiber
numseg = nNodes-1 #number of segments per fiber. 
l = 2e-02         # fiber length. 
nodes = []        #ids of nodes. 

#the file fibnodes.dat consists of the coordinates of the nodes, generated from fibgen.f90 (python version soon!)
nlist = open('fibreNodes.dat', 'r')
#insert the gridNodes to the bodyContainer.
for l in nlist:
    for i in range(0, nNodes):
        nodes.append(O.bodies.append(gridNode([float(l.split()[3*i]), 
                                           float(l.split()[3*i+1]), 
                                           float(l.split()[3*i+2])], 
                                           r, material = 'spheremat', 
                                           fixed=False)))
nlist.close()    

#insert the gridConnection in the bodyContainer
for i in range(0, ntot): 
    for j in range(0,nNodes-1): 
        O.bodies.append(gridConnection(nNodes*i+j, nNodes*i+j+1,r, material='frictmat'))
        


#---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------#

# a list of fibers. This is needed for domain decomposition.
fibreList = [Fibre() for i in range(ntot)]

for i in range(ntot):
    for j in range(nNodes): 
        fibreList[i].nodes.append(nNodes*i+j)
        
for fib in fibreList: 
    for i in range(len(fib.nodes)-1): 
        fib.segs.append( (fib.nodes[i],fib.nodes[i+1]) )
    fib.cntrId = fib.nodes[len(fib.nodes)//2]
    
    
        
#---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------#

#pfacet walls. These bodies are not included in the worker subdomains (neither pfacet nodes, pfcet connections) 
ymax = 0.25
xmax = 0.225
zmax = 0.2225
xmin = 1e-10
ymin = -0.05
zmin = 1e-10


color = [255./255.,102./255.,0./255.]

wall1=[]
wall2=[]

O.bodies.append(gridNode([xmin,ymin,zmin],r,fixed=True,material='facetnode'))
wall1.append(O.bodies[-1].id)

O.bodies.append(gridNode([xmax,ymin,zmin],r,fixed=True,material='facetnode'))
wall1.append(O.bodies[-1].id)

O.bodies.append(gridNode([xmin,ymin,zmax],r,fixed=True,material='facetnode'))
wall1.append(O.bodies[-1].id)

O.bodies.append(gridNode([xmax,ymin,zmax],r,fixed=True,material='facetnode'))
wall1.append(O.bodies[-1].id)


color = [255./255.,102./255.,0./255.]
O.bodies.append( gridConnection(wall1[0],wall1[1],r,material='facetmat') )
O.bodies.append( gridConnection(wall1[2],wall1[3],r,material='facetmat') )
O.bodies.append( gridConnection(wall1[2],wall1[1],r,material='facetmat') )
O.bodies.append( gridConnection(wall1[2],wall1[0],r,material='facetmat') )
O.bodies.append( gridConnection(wall1[3],wall1[1],r,material='facetmat') )

O.bodies.append( pfacet(wall1[2],wall1[3],wall1[1],color=color,material='facetmat') )
O.bodies.append( pfacet(wall1[2],wall1[0],wall1[1],color=color,material='facetmat') )


O.bodies.append(gridNode([xmin,ymax,zmin],r,fixed=True,material='facetnode'))
wall2.append(O.bodies[-1].id)

O.bodies.append(gridNode([xmax,ymax,zmin],r,fixed=True,material='facetnode'))
wall2.append(O.bodies[-1].id)

O.bodies.append(gridNode([xmin,ymax,zmax],r,fixed=True,material='facetnode'))
wall2.append(O.bodies[-1].id)

O.bodies.append(gridNode([xmax,ymax,zmax],r,fixed=True,material='facetnode'))
wall2.append(O.bodies[-1].id)


O.bodies.append( gridConnection(wall2[0],wall2[1],r,material='facetmat') )
O.bodies.append( gridConnection(wall2[2],wall2[3],r,material='facetmat') )
O.bodies.append( gridConnection(wall2[2],wall2[1],r,material='facetmat') )
O.bodies.append( gridConnection(wall2[2],wall2[0],r,material='facetmat') )
O.bodies.append( gridConnection(wall2[3],wall2[1],r,material='facetmat') )

O.bodies.append( pfacet(wall2[2],wall2[3],wall2[1],color=color,material='facetmat') )
O.bodies.append( pfacet(wall2[2],wall2[0],wall2[1],color=color,material='facetmat') )

#set a timestep.
O.dt = 1e-05
numThreads = 5
mp.DOMAIN_DECOMPOSITION = True 
mp.fibreList = fibreList
NSTEPS = 500
mp.mpirun(NSTEPS, numThreads)
mp.mergeScene() 
if mp.rank == 0: O.save('fibres.yade')

#---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------#
