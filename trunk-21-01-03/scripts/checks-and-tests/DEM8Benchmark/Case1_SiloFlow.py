#!/usr/bin/python
# 2020 © Vasileios Angelidakis <v.angelidakis2@ncl.ac.uk>
# 2020 © Bruno Chareyre <bruno.chareyre@grenoble-inp.fr> 
#from builtins import range
#import time

numMPIThreads=1

if numMPIThreads > 1:
    from yade import mpy as mp
    mp.initialize(numMPIThreads)

# Benchmark of basic performance of open-source DEM simulation systems
# Case 1: Silo flow

# The stl file is in mm -> We can keep the units below or scale the stl file to meters (ymport.stl does not have a scale parameter, but I can edit the .stl file manually)

# -------------------------------------------------------------------- #
# Units: N/mm/MPa/ton
# 	force:   1 N
# 	length:  1 mm = 1e-3 m
# 	stress:  1 MPa = 1 N/mm^2 = 1e6 Pa = 1e-3 GPa
#	mass:    1 ton = 1e3 kg
#	density: 1 ton/mm^3 = 1e-12 kg/m^3

# -------------------------------------------------------------------- #
# Input Data -> Define Material and Orifice size. Uncomment the prefered choice

granularMaterial='M1'
#granularMaterial='M2'

fileName='SiloLargeOrifice'
#fileName='SiloSmallOrifice'

# -------------------------------------------------------------------- #
# End of input data. The rest of the script should be automated for all different scenarios. Bruno, Robert, let me know if this is not the case/I have missed something.



# -------------------------------------------------------------------- #
# Materials
Steel = O.materials.append(FrictMat(young=210e-3,poisson=0.2,density=7200e-12,label='Steel'))

# -------------------------------------------------------------------- #
# Asign values based on the Material and Orifice size
# Coeff of restitution (e) / Coeff of friction (f)
e_M1_M2=0.45;	f_M1_M2=0.2
e_M1_M1=0.5;	f_M1_M1=0.3
e_M1_St=0.4;	f_M1_St=0.2
e_M2_M2=0.4;	f_M2_M2=0.4
e_M2_St=0.4;	f_M2_St=0.2

if granularMaterial=='M1':
	M1=O.materials.append(FrictMat(young=1.0e-3,poisson=0.2,density=2500e-12,label='M1'))
	e_gg=e_M1_M1	# Coefficient of restitution (e) between granular material (g) and granular material (g)
	f_gg=f_M1_M1	# Coefficient of friction (f)...

	e_gs=e_M1_St	# Coefficient of restitution (e) between granular material (g) and steel (s)
	f_gs=f_M1_St	# Coefficient of friction (f)...
elif granularMaterial=='M2':
	M2=O.materials.append(FrictMat(young=0.5e-3,poisson=0.2,density=2000e-12,label='M2'))
	e_gg=e_M2_M2
	f_gg=f_M2_M2

	e_gs=e_M2_St
	f_gs=f_M2_St

F_gg=atan(f_gg) # Friction Angle between granular material (g) and granular material (g)
F_gs=atan(f_gs) # Friction Angle between granular material (g) and steel (s)


if fileName=='SiloLargeOrifice':
	z=70	# This is the height of the lowest point of the funnel (at the orifice), measuring from the lowest cylindrical cross section of the silo
elif fileName=='SiloSmallOrifice':
	z=80

# -------------------------------------------------------------------- #
## Engines 
O.engines=[
	ForceResetter(),
	InsertionSortCollider([Bo1_Sphere_Aabb(),Bo1_Facet_Aabb()],label="collider"),
	InteractionLoop(
		[Ig2_Sphere_Sphere_ScGeom(), Ig2_Facet_Sphere_ScGeom()],
		[Ip2_FrictMat_FrictMat_MindlinPhys(
			frictAngle = MatchMaker(matches=((1,1,F_gg),(0,1,F_gs))), # 0 being the id of Steel and
			en         = MatchMaker(matches=((1,1,e_gg),(0,1,e_gs)))  # 1 being the id of granularMaterial
		)],
		[Law2_ScGeom_MindlinPhys_Mindlin()],
	),
	NewtonIntegrator(damping=0,gravity=[0,0,-9810],label="newton"),
	#GlobalStiffnessTimeStepper(active=1,timestepSafetyCoefficient=0.8, timeStepUpdateInterval=100, parallelMode=False, label = "ts",defaultDt=PWaveTimeStep()), #FIXME Remember to reinstate parallelMode=True when we use MPI
	#VTKRecorder(virtPeriod=0.04,fileName='/tmp/Silo-',recorders=['spheres','facets']),
]



# -------------------------------------------------------------------- #
# Generate initial packing. Choose among regularOrtho, regularHexa or randomDensePack (which I think is best).

## Using regularOrtho
#sp=pack.regularOrtho(pack.inCylinder(Vector3(0,0,0),Vector3(0,0,305),radius),radius=2,gap=r*1/10.,material=granularMaterial)

## Using regularHexa
#sp=pack.regularHexa(pack.inCylinder(Vector3(0,0,0),Vector3(0,0,215),radius),radius=2,gap=r*1/10.,material=granularMaterial)  

# Using randomDensePack
sp=pack.randomDensePack(pack.inCylinder((0,0,0),(0,0,254),100),radius=4,spheresInCell=500,returnSpherePack=False,material=granularMaterial,seed=1) # FIXME: We can maybe increase spheresInCell to 5k or 10k. I used 500 to have a fast generation of the sample at this stage. If you prefer, we can load the packing from a db using memoizeDb, to achieve a faster sample generation.

# -------------------------------------------------------------------- #
# Sort packing in ascending Z coordinates and delete excess particles to achieve sample size of 122k
zValues=[]
for s in sp:
	zValues.append(s.state.pos[2])

from operator import itemgetter
indices, zValues_sorted = zip(*sorted(enumerate(zValues), key=itemgetter(1)))
list(zValues)
list(indices)

sp_new=[]
for i in range(0,len(sp)):
	sp_new.append(sp[indices[i]])

Nspheres=122000
sp_new=sp_new[0:Nspheres]

from yade import ymport
facets = ymport.stl(fileName+'.stl',color=(0,1,0),material=Steel)
fctIds = range(len(facets))

NSTEPS = 1000

if numMPIThreads > 1:
    mp.mprint("appending bodies, rank", mp.rank)

    if mp.rank==0:
        O.bodies.append(facets)
        mp.mprint("master has",len(O.bodies), "facets")
    else:
        import numpy as np
        layers = np.array_split(sp_new,numMPIThreads-1)
        mp.mprint("layers",[len(l) for l in layers])
        layerNo = mp.rank-1 #rank zero is for facets
        nextId = int(len(facets) + np.sum([len(x) for x in layers[:layerNo]]))
        mp.mprint("s",s,"startId",nextId)
        for s in layers[layerNo]:
            s.subdomain = mp.rank
            O.bodies.insertAtId(s,nextId)
            nextId += 1
        mp.mprint("s",s,"lastId",nextId-1)
        
        # tune mpi
    mp.VERBOSE_OUTPUT=False
    mp.DISTRIBUTED_INSERT=True
    mp.REALLOCATE_FREQUENCY=4
    mp.ACCUMULATE_FORCES=False
    mp.MAX_RANK_OUTPUT=4
    

else:
    O.bodies.append(facets)
    O.bodies.append(sp_new)
    


collider.verletDist = 0.5
O.dt=1e-10
O.dynDt=False

if numMPIThreads>1:
    mp.mpirun(1,numMPIThreads,False) #this is to eliminate initialization overhead in Cundall number and timings
    mp.YADE_TIMING=True
    t1=time.time()
    mp.mpirun(NSTEPS,withMerge=False)
    t2=time.time()
    mp.mprint("num. bodies:",len([b for b in O.bodies])," ",len(O.bodies))
    if mp.rank==0:
        mp.mprint("CPU wall time for ",NSTEPS," iterations:",t2-t1,"; Cundall number = TODO")
    #mp.mergeScene()

else: 
    O.run(1,True)
    t1=time.time()
    O.run(NSTEPS,True)
    t2=time.time()
    print("num. bodies:",len([b for b in O.bodies])," ",len(O.bodies))
    print("CPU wall time for ",NSTEPS," iterations:",t2-t1,"; Cundall number = TODO")

# -------------------------------------------------------------------- #
# Record time-dependent number of retained particles

#def recordRetained():
#	retained=0
#	retained=sum(1 for i in indSpheres if O.bodies[i].state.pos[2]>-z) # z=-70mm for the silo with LargeOrifice and z=-80 for the siilo with SmallOrifice
##	print(retained)
#	t=O.time
#	return t,retained

#from yade import plot
#def addPlotData(): 
##	try:
	#retained=sum(1 for i in indSpheres if O.bodies[i].state.pos[2]>-z)  
	#eraseEscapedParticles()
	#plot.addData(retained=retained, time1=O.time)
	#if retained==0:
		#finish()
##	except:
##		pass

#O.engines=O.engines+[PyRunner(iterPeriod=100,command='addPlotData()')] # FIXME: Remember to revisit the iterPeriod

#plot.plots={'time1':'retained'}
#plot.plot(noShow=False)


## -------------------------------------------------------------------- #
## Erase particles flowing out of the silo
#def eraseEscapedParticles():
	#for i in indSpheres:
		#if O.bodies[i].state.pos[2]<-z-20: # I do not delete the particles right after they pass the orifice, to disturb the simulation as little as possible
			#indSpheres.remove(i)
			#O.bodies.erase(i)

#def finish():
	#plot.saveDataTxt(fileName+'_'+granularMaterial+'.txt',vars=('time1','retained'))
	#O.pause()

## -------------------------------------------------------------------- #
## GUI
#if opts.nogui==False:
	#from yade import qt
	#v=qt.View()

	#v.eyePosition = Vector3(0,-600,100)
	#v.upVector    = Vector3(0,0,1)
	#v.viewDir     = Vector3(0,1,0)
##	v.grid=(False,True,False)

	#rndr=yade.qt.Renderer()
	##rndr.shape=False
	##rndr.bound=True

##O.saveTmp()
##O.run()

