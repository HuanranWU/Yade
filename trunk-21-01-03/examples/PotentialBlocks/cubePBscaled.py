#!/usr/local/bin/yade-trunk -x
# -*- encoding=utf-8 -*-
# CWBoon 2015

# Uses the following algorithm:
# CW Boon, GT Houlsby, S Utili (2012).  A new algorithm for contact detection between convex polygonal and polyhedral particles in the discrete element method.  Computers and Geotechnics 44, 73-82. 

#Display is saved to a vtk file in the "vtk folder" and the user is required to load it using ParaView.  Control the frequency of printing a vtk file using vtkRecorder.iterPeriod in python

#To use this script:
#Compile with
#ENABLE_POTENTIAL_BLOCKS=ON, and add  sudo apt-get install
#coinor-clp, 
#coinor-libclp-dev, 
#coinor-libclp1, 
#coinor-libosi1v5


from yade import pack
import math

import os
import errno
try:
	os.mkdir('./vtk/')
except OSError as exc:
	if exc.errno != errno.EEXIST:
		raise
	pass


O.engines=[
	ForceResetter(),
	InsertionSortCollider([PotentialBlock2AABB()],verletDist=0.01, avoidSelfInteractionMask=2),
	InteractionLoop(
		[Ig2_PB_PB_ScGeom(twoDimension=False, unitWidth2D=1.0, calContactArea=True)],
		[Ip2_FrictMat_FrictMat_KnKsPBPhys(kn_i=1e8, ks_i=1e7, Knormal=1e8, Kshear=1e7, useFaceProperties=False, viscousDamping=0.2)],
		[Law2_SCG_KnKsPBPhys_KnKsPBLaw(label='law',neverErase=False, allowViscousAttraction=True, traceEnergy=False)]
	),
	#GlobalStiffnessTimeStepper(),
	NewtonIntegrator(damping=0.0,exactAsphericalRot=True,gravity=[0,-9.81,0]),
	PotentialBlockVTKRecorder(fileName='./vtk/cubePBscaled',iterPeriod=10000,twoDimension=False,sampleX=50,sampleY=50,sampleZ=50,maxDimension=0.2,label='vtkRecorder')
]


powderDensity = 2000
distanceToCentre = 0.5
meanSize = 1.0
wallThickness = 0.5*meanSize
O.materials.append(FrictMat(young=-1,poisson=-1,frictionAngle=radians(0.0),density=powderDensity,label='frictionless')) #The normal and shear stifness values are determined in the IPhys functor, thus the young, poisson parameters of the FrictMat are not used.
lengthOfBase = 9.0*meanSize
heightOfBase = 14.0*meanSize
sp=pack.SpherePack()
mn,mx=Vector3(-0.5*(lengthOfBase-wallThickness),0.5*meanSize,-0.5*(lengthOfBase-wallThickness)),Vector3(0.5*(lengthOfBase-wallThickness),7.0*heightOfBase,0.5*(lengthOfBase-wallThickness))
R=sqrt(3.0)*distanceToCentre
sp.makeCloud(mn,mx,R,0,100,False)


r=0.01*meanSize
for s in sp:
	b=Body()
	b.mask=1
	b.aspherical=True
	wire=False
	color=Vector3(random.random(),random.random(),random.random())
	highlight=False
	b.shape=PotentialBlock(k=0.0, r=r, R=0.0, a=[1,-1,0,0,0,0], b=[0,0,1,-1,0,0], c=[0,0,0,0,1,-1], 
d=[distanceToCentre-r,distanceToCentre-r,distanceToCentre-r,distanceToCentre-r,distanceToCentre-r,distanceToCentre-r], isBoundary=False, color=color,
wire=wire, highlight=highlight, minAabb=sqrt(3)*Vector3(distanceToCentre,distanceToCentre,distanceToCentre), maxAabb=sqrt(3)*Vector3(distanceToCentre,distanceToCentre,distanceToCentre), AabbMinMax=True, fixedNormal=False, id=len(O.bodies))
	utils._commonBodySetup(b, b.shape.volume, b.shape.inertia, material='frictionless',pos=s[0], fixed=False)
	b.state.pos = s[0] #s[0] stores center
	b.state.ori = Quaternion((random.random(),random.random(),random.random()),random.random()) #s[2]
	O.bodies.append(b)


#Bottom faces of the box
r=0.1*wallThickness
bbb=Body()
bbb.mask=3
wire=False
color=[0,0.5,1]
highlight=False
bbb.shape=PotentialBlock(k=0.0, r=r, R=0.0, a=[1,-1,0,0,0,0], b=[0,0,1,-1,0,0], c=[0,0,0,0,1,-1], d=[lengthOfBase/6.0-r,lengthOfBase/6.0-r,0.5*wallThickness-r,0.5*wallThickness-r,lengthOfBase/6.0-r,lengthOfBase/6.0-r], id=len(O.bodies), isBoundary=True, color=color, wire=wire, highlight=highlight, AabbMinMax=True, minAabb=1.05*Vector3(lengthOfBase/6.0,0.5*wallThickness,lengthOfBase/6.0), maxAabb=1.05*Vector3(lengthOfBase/6.0,0.5*wallThickness,lengthOfBase/6.0), fixedNormal=False)
utils._commonBodySetup(bbb, bbb.shape.volume, bbb.shape.inertia, material='frictionless', pos=[0,0,0], fixed=True)
bbb.state.pos = [0,0,0]
lidID = O.bodies.append(bbb)


b1=Body()
b1.mask=3
wire=False
color=[0,0.5,1]
highlight=False
b1.shape=PotentialBlock(k=0.0, r=r, R=0.0, a=[1,-1,0,0,0,0], b=[0,0,1,-1,0,0], c=[0,0,0,0,1,-1], d=[lengthOfBase/6.0-r,lengthOfBase/6.0-r,0.5*wallThickness-r,0.5*wallThickness-r,lengthOfBase/6.0-r,lengthOfBase/6.0-r], id=len(O.bodies), isBoundary=True, color=color, wire=wire, highlight=highlight, AabbMinMax=True, minAabb=1.05*Vector3(lengthOfBase/6.0,0.5*wallThickness,lengthOfBase/6.0), maxAabb=1.05*Vector3(lengthOfBase/6.0,0.5*wallThickness,lengthOfBase/6.0), fixedNormal=False)
utils._commonBodySetup(b1, b1.shape.volume, b1.shape.inertia, material='frictionless', pos=[0,0,0], fixed=True)
b1.state.pos = [lengthOfBase/3.0,0,lengthOfBase/3.0]
O.bodies.append(b1)


b2=Body()
b2.mask=3
wire=False
color=[0,0.5,1]
highlight=False
b2.shape=PotentialBlock(k=0.0, r=r, R=0.0, a=[1,-1,0,0,0,0], b=[0,0,1,-1,0,0], c=[0,0,0,0,1,-1], d=[lengthOfBase/6.0-r,lengthOfBase/6.0-r,0.5*wallThickness-r,0.5*wallThickness-r,lengthOfBase/6.0-r,lengthOfBase/6.0-r], id=len(O.bodies), isBoundary=True, color=color, wire=wire, highlight=highlight, AabbMinMax=True, minAabb=1.05*Vector3(lengthOfBase/6.0,0.5*wallThickness,lengthOfBase/6.0), maxAabb=1.05*Vector3(lengthOfBase/6.0,0.5*wallThickness,lengthOfBase/6.0), fixedNormal=False)
utils._commonBodySetup(b2, b2.shape.volume, b2.shape.inertia, material='frictionless', pos=[0,0,0], fixed=True)
b2.state.pos = [-lengthOfBase/3.0,0,lengthOfBase/3.0]
O.bodies.append(b2)


b3=Body()
b3.mask=3
wire=False
color=[0,0.5,1]
highlight=False
b3.shape=PotentialBlock(k=0.0, r=r, R=0.0, a=[1,-1,0,0,0,0], b=[0,0,1,-1,0,0], c=[0,0,0,0,1,-1], d=[lengthOfBase/6.0-r,lengthOfBase/6.0-r,0.5*wallThickness-r,0.5*wallThickness-r,lengthOfBase/6.0-r,lengthOfBase/6.0-r], id=len(O.bodies), isBoundary=True, color=color, wire=wire, highlight=highlight, AabbMinMax=True, minAabb=1.05*Vector3(lengthOfBase/6.0,0.5*wallThickness,lengthOfBase/6.0), maxAabb=1.05*Vector3(lengthOfBase/6.0,0.5*wallThickness,lengthOfBase/6.0), fixedNormal=False)
utils._commonBodySetup(b3, b3.shape.volume, b3.shape.inertia, material='frictionless', pos=[0,0,0], fixed=True)
b3.state.pos = [0,0,lengthOfBase/3.0]
O.bodies.append(b3)


b4=Body()
b4.mask=3
wire=False
color=[0,0.5,1]
highlight=False
b4.shape=PotentialBlock(k=0.0, r=r, R=0.0, a=[1,-1,0,0,0,0], b=[0,0,1,-1,0,0], c=[0,0,0,0,1,-1], d=[lengthOfBase/6.0-r,lengthOfBase/6.0-r,0.5*wallThickness-r,0.5*wallThickness-r,lengthOfBase/6.0-r,lengthOfBase/6.0-r], id=len(O.bodies), isBoundary=True, color=color, wire=wire, highlight=highlight, AabbMinMax=True, minAabb=1.05*Vector3(lengthOfBase/6.0,0.5*wallThickness,lengthOfBase/6.0), maxAabb=1.05*Vector3(lengthOfBase/6.0,0.5*wallThickness,lengthOfBase/6.0), fixedNormal=False)
utils._commonBodySetup(b4, b4.shape.volume, b4.shape.inertia, material='frictionless', pos=[0,0,0], fixed=True)
b4.state.pos = [lengthOfBase/3.0,0,-lengthOfBase/3.0]
O.bodies.append(b4)


b5=Body()
b5.mask=3
wire=False
color=[0,0.5,1]
highlight=False
b5.shape=PotentialBlock(k=0.0, r=r, R=0.0, a=[1,-1,0,0,0,0], b=[0,0,1,-1,0,0], c=[0,0,0,0,1,-1], d=[lengthOfBase/6.0-r,lengthOfBase/6.0-r,0.5*wallThickness-r,0.5*wallThickness-r,lengthOfBase/6.0-r,lengthOfBase/6.0-r], id=len(O.bodies), isBoundary=True, color=color, wire=wire, highlight=highlight, AabbMinMax=True, minAabb=1.05*Vector3(lengthOfBase/6.0,0.5*wallThickness,lengthOfBase/6.0), maxAabb=1.05*Vector3(lengthOfBase/6.0,0.5*wallThickness,lengthOfBase/6.0), fixedNormal=False)
utils._commonBodySetup(b5, b5.shape.volume, b5.shape.inertia, material='frictionless', pos=[0,0,0], fixed=True)
b5.state.pos = [0,0,-lengthOfBase/3.0]
O.bodies.append(b5)


b6=Body()
b6.mask=3
wire=False
color=[0,0.5,1]
highlight=False
b6.shape=PotentialBlock(k=0.0, r=r, R=0.0, a=[1,-1,0,0,0,0], b=[0,0,1,-1,0,0], c=[0,0,0,0,1,-1], d=[lengthOfBase/6.0-r,lengthOfBase/6.0-r,0.5*wallThickness-r,0.5*wallThickness-r,lengthOfBase/6.0-r,lengthOfBase/6.0-r], id=len(O.bodies), isBoundary=True, color=color, wire=wire, highlight=highlight, AabbMinMax=True, minAabb=1.05*Vector3(lengthOfBase/6.0,0.5*wallThickness,lengthOfBase/6.0), maxAabb=1.05*Vector3(lengthOfBase/6.0,0.5*wallThickness,lengthOfBase/6.0), fixedNormal=False)
utils._commonBodySetup(b6, b6.shape.volume, b6.shape.inertia, material='frictionless', pos=[0,0,0], fixed=True)
b6.state.pos = [-lengthOfBase/3.0,0,-lengthOfBase/3.0]
O.bodies.append(b6)


b7=Body()
b7.mask=3
wire=False
color=[0,0.5,1]
highlight=False
b7.shape=PotentialBlock(k=0.0, r=r, R=0.0, a=[1,-1,0,0,0,0], b=[0,0,1,-1,0,0], c=[0,0,0,0,1,-1], d=[lengthOfBase/6.0-r,lengthOfBase/6.0-r,0.5*wallThickness-r,0.5*wallThickness-r,lengthOfBase/6.0-r,lengthOfBase/6.0-r], id=len(O.bodies), isBoundary=True, color=color, wire=wire, highlight=highlight, AabbMinMax=True, minAabb=1.05*Vector3(lengthOfBase/6.0,0.5*wallThickness,lengthOfBase/6.0), maxAabb=1.05*Vector3(lengthOfBase/6.0,0.5*wallThickness,lengthOfBase/6.0), fixedNormal=False)
utils._commonBodySetup(b7, b7.shape.volume, b7.shape.inertia, material='frictionless', pos=[0,0,0], fixed=True)
b7.state.pos = [-lengthOfBase/3.0,0,0]
O.bodies.append(b7)


b8=Body()
b8.mask=3
wire=False
color=[0,0.5,1]
highlight=False
b8.shape=PotentialBlock(k=0.0, r=r, R=0.0, a=[1,-1,0,0,0,0], b=[0,0,1,-1,0,0], c=[0,0,0,0,1,-1], d=[lengthOfBase/6.0-r,lengthOfBase/6.0-r,0.5*wallThickness-r,0.5*wallThickness-r,lengthOfBase/6.0-r,lengthOfBase/6.0-r], id=len(O.bodies), isBoundary=True, color=color, wire=wire, highlight=highlight, AabbMinMax=True, minAabb=1.05*Vector3(lengthOfBase/6.0,0.5*wallThickness,lengthOfBase/6.0), maxAabb=1.05*Vector3(lengthOfBase/6.0,0.5*wallThickness,lengthOfBase/6.0), fixedNormal=False)
utils._commonBodySetup(b8, b8.shape.volume, b8.shape.inertia, material='frictionless', pos=[0,0,0], fixed=True)
b8.state.pos = [lengthOfBase/3.0,0,0]
O.bodies.append(b8)


# Vertical faces A-B-C-D of the box
bA=Body()
bA.mask=3
wire=False
color=[0,0.5,1]
highlight=False
bA.shape=PotentialBlock(k=0.0, r=r, R=0.0, a=[1,-1,0,0,0,0], b=[0,0,1,-1,0,0], c=[0,0,0,0,1,-1], d=[0.5*wallThickness-r,0.5*wallThickness-r,0.5*heightOfBase-r,0.5*heightOfBase-r,0.5*lengthOfBase-r,0.5*lengthOfBase-r], id=len(O.bodies), isBoundary=True, color=color, wire=wire, highlight=highlight, AabbMinMax=True, minAabb=1.05*Vector3(0.4*wallThickness,0.5*heightOfBase,0.5*lengthOfBase), maxAabb=1.05*Vector3(0.4*wallThickness,0.5*heightOfBase,0.5*lengthOfBase), fixedNormal=False)
utils._commonBodySetup(bA, bA.shape.volume, bA.shape.inertia, material='frictionless', pos=[0,0,0], fixed=True)
bA.state.pos = [0.5*lengthOfBase,0.5*heightOfBase,0]
O.bodies.append(bA)


bB=Body()
bB.mask=3
wire=False
color=[0,0.5,1]
highlight=False
bB.shape=PotentialBlock(k=0.0, r=r, R=0.0, a=[1,-1,0,0,0,0], b=[0,0,1,-1,0,0], c=[0,0,0,0,1,-1], d=[0.5*wallThickness-r,0.5*wallThickness-r,0.5*heightOfBase-r,0.5*heightOfBase-r,0.5*lengthOfBase-r,0.5*lengthOfBase-r], id=len(O.bodies), isBoundary=True, color=color, wire=wire, highlight=highlight, AabbMinMax=True, minAabb=1.05*Vector3(0.4*wallThickness,0.5*heightOfBase,0.5*lengthOfBase), maxAabb=1.05*Vector3(0.4*wallThickness,0.5*heightOfBase,0.5*lengthOfBase), fixedNormal=False)
utils._commonBodySetup(bB, bB.shape.volume, bB.shape.inertia, material='frictionless', pos=[0,0,0], fixed=True)
bB.state.pos = [-0.5*lengthOfBase,0.5*heightOfBase,0]
O.bodies.append(bB)


bC=Body()
bC.mask=3
wire=True
color=[0,0.5,1]
highlight=False
bC.shape=PotentialBlock(k=0.0, r=r, R=0.0, a=[1,-1,0,0,0,0], b=[0,0,1,-1,0,0], c=[0,0,0,0,1,-1], d=[0.5*lengthOfBase-r,0.5*lengthOfBase-r,0.5*heightOfBase-r,0.5*heightOfBase-r,0.5*wallThickness-r,0.5*wallThickness-r], id=len(O.bodies), isBoundary=True, color=color, wire=wire, highlight=highlight, AabbMinMax=True, minAabb=1.05*Vector3(0.5*lengthOfBase,0.5*heightOfBase,0.4*wallThickness), maxAabb=1.05*Vector3(0.5*lengthOfBase,0.5*heightOfBase,0.4*wallThickness), fixedNormal=False)
utils._commonBodySetup(bC, bC.shape.volume, bC.shape.inertia, material='frictionless', pos=[0,0,0], fixed=True)
bC.state.pos = [0,0.5*heightOfBase,0.5*lengthOfBase]
O.bodies.append(bC)


bD=Body()
bD.mask=3
wire=False
color=[0,0.5,1]
highlight=False
bD.shape=PotentialBlock(k=0.0, r=r, R=0.0, a=[1,-1,0,0,0,0], b=[0,0,1,-1,0,0], c=[0,0,0,0,1,-1], d=[0.5*lengthOfBase-r,0.5*lengthOfBase-r,0.5*heightOfBase-r,0.5*heightOfBase-r,0.5*wallThickness-r,0.5*wallThickness-r], id=len(O.bodies), isBoundary=True, color=color, wire=wire, highlight=highlight, AabbMinMax=True, minAabb=1.05*Vector3(0.5*lengthOfBase,0.5*heightOfBase,0.4*wallThickness), maxAabb=1.05*Vector3(0.5*lengthOfBase,0.5*heightOfBase,0.4*wallThickness), fixedNormal=False)
utils._commonBodySetup(bD, bD.shape.volume, bD.shape.inertia, material='frictionless', pos=[0,0,0], fixed=True)
bD.state.pos = [0,0.5*heightOfBase,-0.5*lengthOfBase]
O.bodies.append(bD)


escapeNo=0
def myAddPlotData():
	global escapeNo
	global wallThickness
	global meanSize
	uf=utils.unbalancedForce()
	if isnan(uf):
		uf = 1.0
	KE = utils.kineticEnergy()

	for b in O.bodies:
		if b.state.pos[1] < -5.0*meanSize and len(b.state.blockedDOFs)==0: #i.e. fixed==False
			escapeNo = escapeNo+1
			O.bodies.erase(b.id)
	if O.iter>25000:
		removeLid()
	plot.addData(timeStep1=O.iter,timeStep2=O.iter,timeStep3=O.iter,timeStep4=O.iter,time=O.time,unbalancedForce=uf,kineticEn=KE,outsideNo=escapeNo)


from yade import plot
plot.plots={'timeStep1':('unbalancedForce'),'timeStep2':('kineticEn'),'time':('outsideNo')}
#plot.plot() #Uncomment to view plots
O.engines=O.engines+[PyRunner(iterPeriod=10,command='myAddPlotData()')]

def removeLid():
	global lidID	
	if (O.bodies[lidID]):
		O.bodies.erase(lidID)	


O.dt = 0.2*sqrt(0.3*O.bodies[0].state.mass/1.0e8)

from yade import qt
qt.Controller()
v=qt.View()

O.saveTmp()

