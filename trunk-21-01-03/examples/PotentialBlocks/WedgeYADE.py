from __future__ import print_function
#CW BOON 2018
# Use the following algorithms:
# CW Boon, GT Houlsby, S Utili (2012).  A new algorithm for contact detection between convex polygonal and polyhedral particles in the discrete element method.  Computers and Geotechnics 44, 73-82. 
# CW Boon, GT Houlsby, S Utili (2015).  A new rock slicing method based on linear programming.  Computers and Geotechnics 65, 12-29. 

#The code runs some steps to stabilise.  After that a vertical cut is carried out.  And the simulations shows the failure mechanism of the slope.
#Display is saved to a vtk file in the "vtk" folder and the user is required to load it using ParaView.  Control the frequency of printing a vtk file using vtkRecorder.iterPeriod in python

#Disclaimer: This script is just for illustration to demonstrate the function of Block Gen and PotentialBlock Code, and not for other purpose outside for this intended use

#To use this script:
#Compile with
#ENABLE_POTENTIAL_BLOCKS=ON, and add  sudo apt-get install
#coinor-clp, 
#coinor-libclp-dev, 
#coinor-libclp1, 
#coinor-libosi1v5

import os
import errno
try:
   os.mkdir('./vtk/')
except OSError as exc:
   if exc.errno != errno.EEXIST:
      raise
   pass

name ='BlockGeneration'

p=BlockGen()
p.maxRatio = 10000.0;
p.minSize = 7.0;
p.density = 2700 # 23.6/9.81*1000.0 	#kg/m3
p.dampingMomentum = 0.8
p.viscousDamping = 0.0
p.Kn = 2.0e8  	#
p.Ks = 0.1e8 	#
p.frictionDeg = 50.0 #degrees
p.traceEnergy = False
p.defaultDt = 1e-4
p.rForPP = 0.2 #0.04
p.kForPP = 0.0
p.RForPP = 1800.0
p.gravity = [0,0,9.81]
#p.inertiaFactor = 1.0
p.initialOverlap = 1e-6 
p.exactRotation = True
#p.shrinkFactor = 1.0
p.boundarySizeXmin = 20.0; #South
p.boundarySizeXmax = 20.0; #North
p.boundarySizeYmin = 20.0; #West
p.boundarySizeYmax = 20; #East 4100
p.boundarySizeZmin = 0.0; #Up
p.boundarySizeZmax = 40.0; #Down
p.persistentPlanes = False
p.jointProbabilistic = True
p.opening = False
p.boundaries = True
p.slopeFace = False
p.calContactArea=True
p.twoDimension = False
p.unitWidth2D = 9.0
p.intactRockDegradation = True
#p.useFaceProperties = False
p.neverErase = False # Must be used when tension is on
p.sliceBoundaries = True
p.filenamePersistentPlanes = ''
p.filenameProbabilistic = './joints/jointC.csv'  #  PLEASE CHEK THIS  
p.filenameOpening = '' #'./joints/opening.csv'
p.filenameBoundaries = ''
p.filenameSlopeFace =''
p.filenameSliceBoundaries = ''
p.directionA=Vector3(1,0,0)
p.directionB=Vector3(0,1,0)
p.directionC=Vector3(0,0,1)

p.saveBlockGenData=True #if True, data of the BlockGen are written in a file; if False, they are displayed on the terminal
p.outputFile="BlockGen_Output.txt" #if empty, the block generation data are not written at all

p.load()
O.engines[1].avoidSelfInteractionMask=2
O.engines[2].lawDispatcher.functors[0].initialOverlapDistance = p.initialOverlap - 1e-6
O.engines[2].lawDispatcher.functors[0].allowBreakage = False
O.engines[2].lawDispatcher.functors[0].allowViscousAttraction=True
O.engines[2].lawDispatcher.functors[0].traceEnergy=False

O.engines[2].physDispatcher.functors[0].kn_i = 2e9
O.engines[2].physDispatcher.functors[0].ks_i = 0.1e9

from yade import plot

rockFriction = 40.0
boundaryFriction = 40.0
targetFriction = 40.0
waterHeight = 460.0  # PLEASE CHEK THIS 460.0 # 
startCountingBrokenBonds = False
minTimeStep = 1000000.0
westBodyId=10 #PLEASE CHEK THIS  
midBodyId =10 #PLEASE CHEK THIS  
eastBodyId =10 #PLEASE CHEK THIS  
originalPositionW = O.bodies[westBodyId].state.pos
originalPositionE = O.bodies[eastBodyId].state.pos
originalPositionM = O.bodies[midBodyId].state.pos
velocityDependency = False


# ----------------------------------------------------------------------------------------------------------------------------------------------- #
# Engines
O.engines[3].label='integration'
O.dt = 10.0e-5 #10e-4
O.engines = O.engines + [PotentialBlockVTKRecorder(fileName='./vtk/Wedge', iterPeriod=2000, twoDimension=False, sampleX=70, sampleY=70, sampleZ=70, maxDimension=100.0, REC_INTERACTION=True, REC_VELOCITY=True, label='vtkRecorder')]
O.engines = O.engines+ [PyRunner(iterPeriod=2000,command='calTimeStep()')] 
O.engines=O.engines+[PyRunner(iterPeriod=200,command='myAddPlotData()')]
O.engines = O.engines+ [PyRunner(iterPeriod=200,command='goToNextStage2()',label='dispChecker')] 


# ----------------------------------------------------------------------------------------------------------------------------------------------- #
# Material
O.materials.append(FrictMat(young=-1,poisson=-1,density=p.density,frictionAngle=radians(0.0), label='frictionless'))


# ----------------------------------------------------------------------------------------------------------------------------------------------- #
# Boundary Plates
thickness = 0.1*p.boundarySizeZmax
wire=False
highlight=False
kPP = p.kForPP
rPP = p.rForPP

# ----------------------------------------------------------------------------------------------------------------------------------------------- #
# Bottom plate
bbb = Body()
bbb.mask=3
color=[0,0.5,1]
aPP = [1,-1,0,0,0,0]
bPP = [0,0,1,-1,0,0]
cPP = [0,0,0,0,1,-1]
dPP = [p.boundarySizeXmax-rPP , p.boundarySizeXmax-rPP, p.boundarySizeYmax-rPP, p.boundarySizeYmax-rPP, thickness-rPP, thickness-rPP]
minmaxAabb = 1.05*Vector3( dPP[0], dPP[2], dPP[4] )
bbb.shape=PotentialBlock(k=kPP, r=rPP, R=0.0, a=aPP, b=bPP, c=cPP, d=dPP, id=len(O.bodies), isBoundary=True, color=color, wire=wire, highlight=highlight, AabbMinMax=True, minAabb=minmaxAabb, maxAabb=minmaxAabb, fixedNormal=True, boundaryNormal=Vector3(0,0,-1))
utils._commonBodySetup(bbb,bbb.shape.volume,bbb.shape.inertia,material='frictionless',pos= [0,0,p.boundarySizeZmax+thickness], fixed=True) 
bbb.state.ori = bbb.shape.orientation
O.bodies.append(bbb)


# ----------------------------------------------------------------------------------------------------------------------------------------------- #
# Lateral plate A
bA = Body()
bA.mask=3
color=[0,0.5,1]
aPP = [1,-1,0,0,0,0]
bPP = [0,0,1,-1,0,0]
cPP = [0,0,0,0,1,-1]
dPP = [p.boundarySizeXmax-rPP , p.boundarySizeXmax-rPP, thickness-rPP, thickness-rPP, 0.5*p.boundarySizeZmax-rPP, 0.5*p.boundarySizeZmax-rPP]
minmaxAabb = 1.05*Vector3( dPP[0], dPP[2], dPP[4] )
bA.shape=PotentialBlock(k=kPP, r=rPP, R=0.0, a=aPP, b=bPP, c=cPP, d=dPP, id=len(O.bodies), isBoundary=True, color=color, wire=wire, highlight=highlight, AabbMinMax=True, minAabb=minmaxAabb, maxAabb=minmaxAabb, fixedNormal=True, boundaryNormal=Vector3(0,-1,0))
utils._commonBodySetup(bA,bA.shape.volume,bA.shape.inertia,material='frictionless',pos= [0,p.boundarySizeYmax+thickness,0.5*p.boundarySizeZmax], fixed=True) 
bA.state.ori = bA.shape.orientation
O.bodies.append(bA)


# ----------------------------------------------------------------------------------------------------------------------------------------------- #
# Lateral plate B
bB = Body()
bB.mask=3
color=[0,0.5,1]
aPP = [1,-1,0,0,0,0]
bPP = [0,0,1,-1,0,0]
cPP = [0,0,0,0,1,-1]
dPP = [p.boundarySizeXmax-rPP, p.boundarySizeXmax-rPP, thickness-rPP, thickness-rPP, 0.5*p.boundarySizeZmax-rPP, 0.5*p.boundarySizeZmax-rPP]
minmaxAabb = 1.05*Vector3( dPP[0], dPP[2], dPP[4] )
bB.shape=PotentialBlock(k=kPP, r=rPP, R=0.0, a=aPP, b=bPP, c=cPP, d=dPP, id=len(O.bodies), isBoundary=True, color=color, wire=wire, highlight=highlight, AabbMinMax=True, minAabb=minmaxAabb, maxAabb=minmaxAabb, fixedNormal=True, boundaryNormal=Vector3(0,1,0))
utils._commonBodySetup(bB,bB.shape.volume,bB.shape.inertia,material='frictionless',pos= [0,-p.boundarySizeYmax-thickness,0.5*p.boundarySizeZmax], fixed=True) 
bB.state.ori = bB.shape.orientation
O.bodies.append(bB)


# ----------------------------------------------------------------------------------------------------------------------------------------------- #
# Lateral plate C
bC = Body()
bC.mask=3
color=[0,0.5,1]
aPP = [1,-1,0,0,0,0]
bPP = [0,0,1,-1,0,0]
cPP = [0,0,0,0,1,-1]
dPP = [thickness-rPP, thickness-rPP, p.boundarySizeYmax-rPP, p.boundarySizeYmax-rPP, 0.5*p.boundarySizeZmax-rPP, 0.5*p.boundarySizeZmax-rPP]
minmaxAabb = 1.05*Vector3( dPP[0], dPP[2], dPP[4] )
bC.shape=PotentialBlock(k=kPP, r=rPP, R=0.0, a=aPP, b=bPP, c=cPP, d=dPP, id=len(O.bodies), isBoundary=True, color=color, wire=wire, highlight=highlight, AabbMinMax=True, minAabb=minmaxAabb, maxAabb=minmaxAabb, fixedNormal=True, boundaryNormal=Vector3(1,0,0))
utils._commonBodySetup(bC,bC.shape.volume,bC.shape.inertia,material='frictionless',pos= [-p.boundarySizeXmax-thickness,0,0.5*p.boundarySizeZmax], fixed=True) 
bC.state.ori = bC.shape.orientation
O.bodies.append(bC)


# ----------------------------------------------------------------------------------------------------------------------------------------------- #
# Lateral plate D
bD = Body()
bD.mask=3
color=[0,0.5,1]
aPP = [1,-1,0,0,0,0]
bPP = [0,0,1,-1,0,0]
cPP = [0,0,0,0,1,-1]
dPP = [thickness-rPP, thickness-rPP, p.boundarySizeYmax-rPP, p.boundarySizeYmax-rPP, 0.5*p.boundarySizeZmax-rPP, 0.5*p.boundarySizeZmax-rPP]
minmaxAabb = 1.05*Vector3( dPP[0], dPP[2], dPP[4] )
bD.shape=PotentialBlock(k=kPP, r=rPP, R=0.0, a=aPP, b=bPP, c=cPP, d=dPP, id=len(O.bodies), isBoundary=True, color=color, wire=wire, highlight=highlight, AabbMinMax=True, minAabb=minmaxAabb, maxAabb=minmaxAabb, fixedNormal=True, boundaryNormal=Vector3(-1,0,0))
utils._commonBodySetup(bD,bD.shape.volume,bD.shape.inertia,material='frictionless',pos= [p.boundarySizeXmax+thickness,0,0.5*p.boundarySizeZmax], fixed=True) 
bD.state.ori = bD.shape.orientation
O.bodies.append(bD)


# ----------------------------------------------------------------------------------------------------------------------------------------------- #
def calTimeStep():
	global minTimeStep
	mkratio = 99999999.9
	maxK = 0.0
	minMass = 1.0e15
	for i in O.interactions:
		if i.isReal==True:
			dt1 = O.bodies[i.id1].state.mass/i.phys.kn
			dt2 = O.bodies[i.id2].state.mass/i.phys.kn
			if dt1 < dt2:
				presentDt = 0.15*sqrt(dt1)
				if minTimeStep > presentDt:
					minTimeStep = presentDt
					O.dt = minTimeStep
			else:
				presentDt = 0.15*sqrt(dt2)
				if minTimeStep > presentDt:
					minTimeStep = presentDt
					O.dt = minTimeStep


# ----------------------------------------------------------------------------------------------------------------------------------------------- #
def excavate():
	for b in O.bodies:
		if NorthWall(b.state.pos[0],b.state.pos[1],b.state.pos[2]) <0.001:
			if b.isClumpMember == True and b.shape.isBoundary==False:
				O.bodies.deleteClumpMember(O.bodies[b.clumpId], b)
			elif b.isClump == False and b.shape.isBoundary==False:
				O.bodies.erase(b.id)
				continue
	O.bodies.erase(bA.id)
	O.bodies.erase(bC.id)


prevDistance = O.bodies[westBodyId].state.pos[0]
tolDistance = 0.003 #0.1
tolDistance2 = 0.05
SRinProgress = False
SRcounter = 0
checkIter = 0
prevKE = 0.0
currentKE = 0.0
tolKE =10e10
initBondedContacts = 0
initDispRate = -1.0
prevDispRate = 0


# ----------------------------------------------------------------------------------------------------------------------------------------------- #
def changeKE(newKE):
	global tolKE
	tolKE = newKE


# ----------------------------------------------------------------------------------------------------------------------------------------------- #
def changeTolDist(newTol):
	global tolDistance
	tolDistance = newTol


# ----------------------------------------------------------------------------------------------------------------------------------------------- #
removeDampingBool = False
def goToNextStage2():
	global startCountingBrokenBonds
	global velocityDependency
	global waterHeight
	global boundaryFriction
	global rockFriction
	global targetFriction
	global prevDistance
	global originalPositionW
	global tolDistance
	global tolDistance2
	global checkIter
	global SRinProgress
	global SRcounter
	global prevKE
	global currentKE
	global tolKE
	global initDispRate
	global prevDispRate
	global removeDampingBool
	prevKE = currentKE
	KE = utils.kineticEnergy()
	currentKE = KE
	uf = utils.unbalancedForce()
	if O.iter>500 and removeDampingBool == False:
		removeDamping()
		removeDampingBool=True
	if O.iter>2000 and SRcounter == 0: # and uf<0.005:
		print(O.iter)
		O.pause()
		vtkRecorder.iterPeriod=1
		for b in O.bodies:
			b.state.vel = Vector3(0.0,0.0,0.0)
			b.state.angVel = Vector3(0.0,0.0,0.0)
		calTimeStep()
		excavate()
		dispChecker.iterPeriod=1
		SRcounter = 1
		O.step()
		vtkRecorder.iterPeriod=500
		O.run(20000)
		return


# ----------------------------------------------------------------------------------------------------------------------------------------------- #
def SouthWall(x,y,z):
	Xcentre1 = 0.0 	
	Ycentre1 = 0.0
	Zcentre1 = -0.0
	dip = 90.0
	dipdir = 315.0
	dipdirN = 0.0
	dipN = 90.0-dip
	if dipdir > 180.0:
		dipdirN = dipdir - 180.0
	else:
		dipdirN = dipdir + 180.0
	dipRad = dipN/180.0*pi
	dipdirRad = dipdirN/180.0*pi
	a = cos(dipdirRad)*cos(dipRad)
	b = sin(dipdirRad)*cos(dipRad)
	c = sin(dipRad)
	l = sqrt(a*a + b*b +c*c)
	a = a/l
	b = b/l
	c = c/l
	d = a*Xcentre1 + b*Ycentre1 + c*Zcentre1
	plane = a*x+ b*y +c*z - d
	if plane < 0.0:
		plane = 0.0
	return plane


# ----------------------------------------------------------------------------------------------------------------------------------------------- #
def NorthWall(x,y,z):
	Xcentre1 = 0.0 	
	Ycentre1 = 0.0
	Zcentre1 = -0.0
	dip = 90.0
	dipdir = 315.0
	dipdirN = 0.0
	dipN = 90.0-dip
	if dipdir > 180.0:
		dipdirN = dipdir - 180.0
	else:
		dipdirN = dipdir + 180.0
	dipRad = dipN/180.0*pi
	dipdirRad = dipdirN/180.0*pi
	a = cos(dipdirRad)*cos(dipRad)
	b = sin(dipdirRad)*cos(dipRad)
	c = sin(dipRad)
	l = sqrt(a*a + b*b +c*c)
	a = -a/l
	b = -b/l
	c = -c/l
	d = a*Xcentre1 + b*Ycentre1 + c*Zcentre1
	plane = a*x+ b*y +c*z - d
	if plane < 0.0:
		plane = 0.0
	return plane


# ----------------------------------------------------------------------------------------------------------------------------------------------- #
def removeDamping():
	for i in O.interactions:
		i.phys.viscousDamping = 0.5
		i.phys.cumulative_us = 0.0
	O.engines[2].physDispatcher.functors[0].viscousDamping = 0.5
	integration.damping= 0.0


# ----------------------------------------------------------------------------------------------------------------------------------------------- #
def myAddPlotData():
	global westBodyId
	global midBodyId
	global eastBodyId
	global originalPositionW
	global originalPositionM
	global originalPositionE
	global boundaryFriction
	KE = utils.kineticEnergy()
	uf = utils.unbalancedForce()
	displacementWx = O.bodies[westBodyId].state.pos[0] - originalPositionW[0]
	displacementW = (O.bodies[westBodyId].state.pos - originalPositionW).norm()
	displacementMx = O.bodies[midBodyId].state.pos[0] - originalPositionM[0]
	displacementM = (O.bodies[midBodyId].state.pos - originalPositionM).norm()
	displacementEx = O.bodies[eastBodyId].state.pos[0] - originalPositionE[0]
	displacementE = (O.bodies[eastBodyId].state.pos - originalPositionE).norm()
	plot.addData(timeStep=O.iter,timeStep1=O.iter,timeStep2=O.iter,timeStep3=O.iter,timeStep4=O.iter,timeStep5=O.iter,kineticEn=KE,unbalancedForce=uf, waterLevel=waterHeight,boundary_phi=boundaryFriction,displacement=displacementW,displacementWest=displacementW,dispWx=displacementWx,displacementMid=displacementM, dispMx=displacementMx,displacementEast=displacementE,dispEx=displacementEx)

plot.plots={'timeStep2':('kineticEn'),'timeStep3':(('displacementWest','ro-'),('dispWx','go-')),'timeStep1':(('displacementMid','ro-'),('dispMx','go-')),'timeStep5':(('displacementEast','ro-'),('dispEx','go-')),'timeStep4':('unbalancedForce')} #PLEASE CHECK
plot.plot() #Uncomment to view plots


# ----------------------------------------------------------------------------------------------------------------------------------------------- #
from yade import qt
try: 
	v=qt.View()
	vaxes=True
	v.viewDir=Vector3(0,-1,0)
	v.eyePosition=Vector3(0,200,0)

	v.eyePosition=Vector3(-77.42657409847706,84.2619166834641,-17.41745783023423)
	v.upVector=Vector3(0.1913254208509842,-0.25732151742252396,-0.9471959776136951)
	v.viewDir=Vector3(0.6412359985709529,-0.697845344639707,0.3191054200439123)
except: pass



O.step()
O.step()
#excavate()
O.step()
calTimeStep()
#O.run(20000)

