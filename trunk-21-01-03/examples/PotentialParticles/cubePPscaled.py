#!/usr/local/bin/yade-trunk -x
# -*- encoding=utf-8 -*-
# CWBoon 2015

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


Gl1_PotentialParticle().store=True

O.engines=[
	ForceResetter(),
	InsertionSortCollider([PotentialParticle2AABB()],verletDist=0.01, avoidSelfInteractionMask=2),
	InteractionLoop(
		[Ig2_PP_PP_ScGeom(twoDimension=False, unitWidth2D=1.0, calContactArea=True, areaStep=5)],
		[Ip2_FrictMat_FrictMat_KnKsPhys(kn_i=1e8, ks_i=1e7, Knormal = 1e8, Kshear = 1e7, useFaceProperties=False, viscousDamping=0.05)],
		[Law2_SCG_KnKsPhys_KnKsLaw(label='law',neverErase=False)]
	),
	NewtonIntegrator(damping=0.0,exactAsphericalRot=True,gravity=[0,-9.81,0]),
	PotentialParticleVTKRecorder(fileName='./vtk/cubePPscaled',label='vtkRecorder',twoDimension=False,iterPeriod=5000,sampleX=50,sampleY=50,sampleZ=50,maxDimension=0.2)
]


powderDensity = 2000
distanceToCentre= 0.5
meanSize = 1.0
wallThickness = 0.5*meanSize
O.materials.append(FrictMat(young=-1,poisson=-1,frictionAngle=radians(0.0),density=powderDensity,label='frictionless')) #The normal and shear stifness values are determined in the IPhys functor, thus the young, poisson parameters of the FrictMat are not used.
lengthOfBase = 9.0*meanSize
heightOfBase = 14.0*meanSize
sp=pack.SpherePack()
mn,mx=Vector3(-0.5*(lengthOfBase-wallThickness),0.5*meanSize,-0.5*(lengthOfBase-wallThickness)),Vector3(0.5*(lengthOfBase-wallThickness),7.0*heightOfBase,0.5*(lengthOfBase-wallThickness))
R=sqrt(3.0)*distanceToCentre
sp.makeCloud(mn,mx,R,0,100,False)


count= 0
r=0.05*meanSize
for s in sp:
	b=Body()
	b.mask=1
	b.aspherical=True
	wire=False
	color=Vector3(random.random(),random.random(),random.random())
	highlight=False
	b.shape=PotentialParticle(k=0.2, r=r, R=R, a=[1,-1,0,0,0,0], b=[0,0,1,-1,0,0], c=[0,0,0,0,1,-1], d=[distanceToCentre-r,distanceToCentre-r,distanceToCentre-r,distanceToCentre-r,distanceToCentre-r,distanceToCentre-r], isBoundary=False, color=color, wire=wire, highlight=highlight, minAabb=sqrt(3)*Vector3(distanceToCentre,distanceToCentre,distanceToCentre), maxAabb=sqrt(3)*Vector3(distanceToCentre,distanceToCentre,distanceToCentre), maxAabbRotated=1.02*Vector3(distanceToCentre,distanceToCentre,distanceToCentre), minAabbRotated=1.02*Vector3(distanceToCentre,distanceToCentre,distanceToCentre), AabbMinMax=True, id=count)
	V=(2*distanceToCentre)**3 # (approximate) Volume of cuboid
	geomInert=(1./6.)*V*(2*distanceToCentre)**2 # (approximate) Principal inertia of cuboid to its centroid
	utils._commonBodySetup(b, V, Vector3(geomInert,geomInert,geomInert), material='frictionless', pos=[0,0,0], fixed=False)
	b.state.pos = s[0] #s[0] stores center
	b.state.ori = Quaternion((random.random(),random.random(),random.random()),random.random()) #s[2]
	O.bodies.append(b)
	count=count+1


#Bottom faces of the box
r=0.1*wallThickness
bbb=Body()
bbb.mask=3
wire=False
color=[0,0.5,1]
highlight=False
bbb.shape=PotentialParticle(k=0.1, r=0.1*wallThickness, R=0.2*lengthOfBase,a=[1,-1,0,0,0,0], b=[0,0,1,-1,0,0], c=[0,0,0,0,1,-1], d=[lengthOfBase/6.0-r,lengthOfBase/6.0-r,0.5*wallThickness-r,0.5*wallThickness-r,lengthOfBase/6.0-r,lengthOfBase/6.0-r], id=count,isBoundary=True,color=color,wire=wire,highlight=highlight,AabbMinMax=True, minAabb=1.02*Vector3(lengthOfBase/6.0,0.5*wallThickness,lengthOfBase/6.0),maxAabb=1.02*Vector3(lengthOfBase/6.0,0.5*wallThickness,lengthOfBase/6.0),maxAabbRotated=1.02*Vector3(lengthOfBase/6.0,0.5*wallThickness,lengthOfBase/6.0),minAabbRotated=1.02*Vector3(lengthOfBase/6.0,0.5*wallThickness,lengthOfBase/6.0),fixedNormal=False)
length=lengthOfBase/3.
V=length*length*wallThickness
geomInertX=(1./12.)*V*(length**2+wallThickness**2)
geomInertY=(1./12.)*V*(length**2+length**2)
geomInertZ=(1./12.)*V*(length**2+wallThickness**2)
utils._commonBodySetup(bbb, V, Vector3(geomInertX,geomInertY,geomInertZ), material='frictionless', pos=[0,0,0], fixed=True)
bbb.state.pos = [0,0,0]
lidID = O.bodies.append(bbb)
count=count+1


b1=Body()
b1.mask=3
wire=False
color=[0,0.5,1]
highlight=False
b1.shape=PotentialParticle(k=0.1, r=0.1*wallThickness, R=0.2*lengthOfBase,a=[1,-1,0,0,0,0], b=[0,0,1,-1,0,0], c=[0,0,0,0,1,-1], d=[lengthOfBase/6.0-r,lengthOfBase/6.0-r,0.5*wallThickness-r,0.5*wallThickness-r,lengthOfBase/6.0-r,lengthOfBase/6.0-r], id=count,isBoundary=True,color=color,wire=wire,highlight=highlight,AabbMinMax=True, minAabb=1.02*Vector3(lengthOfBase/6.0,0.4*wallThickness,lengthOfBase/6.0),maxAabb=1.02*Vector3(lengthOfBase/6.0,0.4*wallThickness,lengthOfBase/6.0),maxAabbRotated=1.02*Vector3(lengthOfBase/6.0,0.5*wallThickness,lengthOfBase/6.0),minAabbRotated=1.02*Vector3(lengthOfBase/6.0,0.5*wallThickness,lengthOfBase/6.0),fixedNormal=False)
length=lengthOfBase/3.
V=length*length*wallThickness
geomInertX=(1./12.)*V*(length**2+wallThickness**2)
geomInertY=(1./12.)*V*(length**2+length**2)
geomInertZ=(1./12.)*V*(length**2+wallThickness**2)
utils._commonBodySetup(b1, V, Vector3(geomInertX,geomInertY,geomInertZ), material='frictionless', pos=[0,0,0], fixed=True)
b1.state.pos = [lengthOfBase/3.0,0,lengthOfBase/3.0]
O.bodies.append(b1)
count=count+1


b2=Body()
b2.mask=3
wire=False
color=[0,0.5,1]
highlight=False
b2.shape=PotentialParticle(k=0.1, r=0.1*wallThickness, R=0.2*lengthOfBase,a=[1,-1,0,0,0,0], b=[0,0,1,-1,0,0], c=[0,0,0,0,1,-1], d=[lengthOfBase/6.0-r,lengthOfBase/6.0-r,0.5*wallThickness-r,0.5*wallThickness-r,lengthOfBase/6.0-r,lengthOfBase/6.0-r], id=count,isBoundary=True,color=color,wire=wire,highlight=highlight,AabbMinMax=True, minAabb=1.02*Vector3(lengthOfBase/6.0,0.4*wallThickness,lengthOfBase/6.0),maxAabb=1.02*Vector3(lengthOfBase/6.0,0.4*wallThickness,lengthOfBase/6.0),maxAabbRotated=1.02*Vector3(lengthOfBase/6.0,0.5*wallThickness,lengthOfBase/6.0),minAabbRotated=1.02*Vector3(lengthOfBase/6.0,0.5*wallThickness,lengthOfBase/6.0),fixedNormal=False)
length=lengthOfBase/3.
V=length*length*wallThickness
geomInertX=(1./12.)*V*(length**2+wallThickness**2)
geomInertY=(1./12.)*V*(length**2+length**2)
geomInertZ=(1./12.)*V*(length**2+wallThickness**2)
utils._commonBodySetup(b2, V, Vector3(geomInertX,geomInertY,geomInertZ), material='frictionless', pos=[0,0,0], fixed=True)
b2.state.pos = [-lengthOfBase/3.0,0,lengthOfBase/3.0]
O.bodies.append(b2)
count=count+1


b3=Body()
b3.mask=3
wire=False
color=[0,0.5,1]
highlight=False
b3.shape=PotentialParticle(k=0.1, r=0.1*wallThickness, R=0.2*lengthOfBase,a=[1,-1,0,0,0,0], b=[0,0,1,-1,0,0], c=[0,0,0,0,1,-1], d=[lengthOfBase/6.0-r,lengthOfBase/6.0-r,0.5*wallThickness-r,0.5*wallThickness-r,lengthOfBase/6.0-r,lengthOfBase/6.0-r], id=count,isBoundary=True,color=color,wire=wire,highlight=highlight,AabbMinMax=True, minAabb=1.02*Vector3(lengthOfBase/6.0,0.4*wallThickness,lengthOfBase/6.0),maxAabb=1.02*Vector3(lengthOfBase/6.0,0.4*wallThickness,lengthOfBase/6.0),maxAabbRotated=1.02*Vector3(lengthOfBase/6.0,0.5*wallThickness,lengthOfBase/6.0),minAabbRotated=1.02*Vector3(lengthOfBase/6.0,0.5*wallThickness,lengthOfBase/6.0),fixedNormal=False)
length=lengthOfBase/3.
V=length*length*wallThickness
geomInertX=(1./12.)*V*(length**2+wallThickness**2)
geomInertY=(1./12.)*V*(length**2+length**2)
geomInertZ=(1./12.)*V*(length**2+wallThickness**2)
utils._commonBodySetup(b3, V, Vector3(geomInertX,geomInertY,geomInertZ), material='frictionless', pos=[0,0,0], fixed=True)
b3.state.pos = [0,0,lengthOfBase/3.0]
O.bodies.append(b3)
count=count+1


b4=Body()
b4.mask=3
wire=False
color=[0,0.5,1]
highlight=False
b4.shape=PotentialParticle(k=0.1, r=0.1*wallThickness, R=0.2*lengthOfBase,a=[1,-1,0,0,0,0], b=[0,0,1,-1,0,0], c=[0,0,0,0,1,-1], d=[lengthOfBase/6.0-r,lengthOfBase/6.0-r,0.5*wallThickness-r,0.5*wallThickness-r,lengthOfBase/6.0-r,lengthOfBase/6.0-r], id=count,isBoundary=True,color=color,wire=wire,highlight=highlight,AabbMinMax=True, minAabb=1.02*Vector3(lengthOfBase/6.0,0.4*wallThickness,lengthOfBase/6.0),maxAabb=1.02*Vector3(lengthOfBase/6.0,0.4*wallThickness,lengthOfBase/6.0),maxAabbRotated=1.02*Vector3(lengthOfBase/6.0,0.5*wallThickness,lengthOfBase/6.0),minAabbRotated=1.02*Vector3(lengthOfBase/6.0,0.5*wallThickness,lengthOfBase/6.0),fixedNormal=False)
length=lengthOfBase/3.
V=length*length*wallThickness
geomInertX=(1./12.)*V*(length**2+wallThickness**2)
geomInertY=(1./12.)*V*(length**2+length**2)
geomInertZ=(1./12.)*V*(length**2+wallThickness**2)
utils._commonBodySetup(b4, V, Vector3(geomInertX,geomInertY,geomInertZ), material='frictionless', pos=[0,0,0], fixed=True)
b4.state.pos = [lengthOfBase/3.0,0,-lengthOfBase/3.0]
O.bodies.append(b4)
count=count+1


b5=Body()
b5.mask=3
wire=False
color=[0,0.5,1]
highlight=False
b5.shape=PotentialParticle(k=0.1, r=0.1*wallThickness, R=0.2*lengthOfBase,a=[1,-1,0,0,0,0], b=[0,0,1,-1,0,0], c=[0,0,0,0,1,-1], d=[lengthOfBase/6.0-r,lengthOfBase/6.0-r,0.5*wallThickness-r,0.5*wallThickness-r,lengthOfBase/6.0-r,lengthOfBase/6.0-r], id=count,isBoundary=True,color=color,wire=wire,highlight=highlight,AabbMinMax=True, minAabb=1.02*Vector3(lengthOfBase/6.0,0.4*wallThickness,lengthOfBase/6.0),maxAabb=1.02*Vector3(lengthOfBase/6.0,0.4*wallThickness,lengthOfBase/6.0),maxAabbRotated=1.02*Vector3(lengthOfBase/6.0,0.5*wallThickness,lengthOfBase/6.0),minAabbRotated=1.02*Vector3(lengthOfBase/6.0,0.5*wallThickness,lengthOfBase/6.0),fixedNormal=False)
length=lengthOfBase/3.
V=length*length*wallThickness
geomInertX=(1./12.)*V*(length**2+wallThickness**2)
geomInertY=(1./12.)*V*(length**2+length**2)
geomInertZ=(1./12.)*V*(length**2+wallThickness**2)
utils._commonBodySetup(b5, V, Vector3(geomInertX,geomInertY,geomInertZ), material='frictionless', pos=[0,0,0], fixed=True)
b5.state.pos = [0,0,-lengthOfBase/3.0]
O.bodies.append(b5)
count=count+1


b6=Body()
b6.mask=3
wire=False
color=[0,0.5,1]
highlight=False
b6.shape=PotentialParticle(k=0.1, r=0.1*wallThickness, R=0.2*lengthOfBase,a=[1,-1,0,0,0,0], b=[0,0,1,-1,0,0], c=[0,0,0,0,1,-1], d=[lengthOfBase/6.0-r,lengthOfBase/6.0-r,0.5*wallThickness-r,0.5*wallThickness-r,lengthOfBase/6.0-r,lengthOfBase/6.0-r], id=count,isBoundary=True,color=color,wire=wire,highlight=highlight,AabbMinMax=True, minAabb=1.02*Vector3(lengthOfBase/6.0,0.4*wallThickness,lengthOfBase/6.0),maxAabb=1.02*Vector3(lengthOfBase/6.0,0.4*wallThickness,lengthOfBase/6.0),maxAabbRotated=1.02*Vector3(lengthOfBase/6.0,0.5*wallThickness,lengthOfBase/6.0),minAabbRotated=1.02*Vector3(lengthOfBase/6.0,0.5*wallThickness,lengthOfBase/6.0),fixedNormal=False)
length=lengthOfBase/3.
V=length*length*wallThickness
geomInertX=(1./12.)*V*(length**2+wallThickness**2)
geomInertY=(1./12.)*V*(length**2+length**2)
geomInertZ=(1./12.)*V*(length**2+wallThickness**2)
utils._commonBodySetup(b6, V, Vector3(geomInertX,geomInertY,geomInertZ), material='frictionless', pos=[0,0,0], fixed=True)
b6.state.pos = [-lengthOfBase/3.0,0,-lengthOfBase/3.0]
O.bodies.append(b6)
count=count+1


b7=Body()
b7.mask=3
wire=False
color=[0,0.5,1]
highlight=False
b7.shape=PotentialParticle(k=0.1, r=0.1*wallThickness, R=0.2*lengthOfBase,a=[1,-1,0,0,0,0], b=[0,0,1,-1,0,0], c=[0,0,0,0,1,-1], d=[lengthOfBase/6.0-r,lengthOfBase/6.0-r,0.5*wallThickness-r,0.5*wallThickness-r,lengthOfBase/6.0-r,lengthOfBase/6.0-r], id=count,isBoundary=True,color=color,wire=wire,highlight=highlight,AabbMinMax=True, minAabb=1.02*Vector3(lengthOfBase/6.0,0.4*wallThickness,lengthOfBase/6.0),maxAabb=1.02*Vector3(lengthOfBase/6.0,0.4*wallThickness,lengthOfBase/6.0),maxAabbRotated=1.02*Vector3(lengthOfBase/6.0,0.5*wallThickness,lengthOfBase/6.0),minAabbRotated=1.02*Vector3(lengthOfBase/6.0,0.5*wallThickness,lengthOfBase/6.0),fixedNormal=False)
length=lengthOfBase/3.
V=length*length*wallThickness
geomInertX=(1./12.)*V*(length**2+wallThickness**2)
geomInertY=(1./12.)*V*(length**2+length**2)
geomInertZ=(1./12.)*V*(length**2+wallThickness**2)
utils._commonBodySetup(b7, V, Vector3(geomInertX,geomInertY,geomInertZ), material='frictionless', pos=[0,0,0], fixed=True)
b7.state.pos = [-lengthOfBase/3.0,0,0]
O.bodies.append(b7)
count=count+1


b8=Body()
b8.mask=3
wire=False
color=[0,0.5,1]
highlight=False
b8.shape=PotentialParticle(k=0.1, r=0.1*wallThickness, R=0.2*lengthOfBase,a=[1,-1,0,0,0,0], b=[0,0,1,-1,0,0], c=[0,0,0,0,1,-1], d=[lengthOfBase/6.0-r,lengthOfBase/6.0-r,0.5*wallThickness-r,0.5*wallThickness-r,lengthOfBase/6.0-r,lengthOfBase/6.0-r], id=count,isBoundary=True,color=color,wire=wire,highlight=highlight,AabbMinMax=True, minAabb=1.02*Vector3(lengthOfBase/6.0,0.4*wallThickness,lengthOfBase/6.0),maxAabb=1.02*Vector3(lengthOfBase/6.0,0.4*wallThickness,lengthOfBase/6.0),maxAabbRotated=1.02*Vector3(lengthOfBase/6.0,0.5*wallThickness,lengthOfBase/6.0),minAabbRotated=1.02*Vector3(lengthOfBase/6.0,0.5*wallThickness,lengthOfBase/6.0),fixedNormal=False)
length=lengthOfBase/3.
V=length*length*wallThickness
geomInertX=(1./12.)*V*(length**2+wallThickness**2)
geomInertY=(1./12.)*V*(length**2+length**2)
geomInertZ=(1./12.)*V*(length**2+wallThickness**2)
utils._commonBodySetup(b8, V, Vector3(geomInertX,geomInertY,geomInertZ), material='frictionless', pos=[0,0,0], fixed=True)
b8.state.pos = [lengthOfBase/3.0,0,0]
O.bodies.append(b8)
count=count+1


#Vertical faces A-B-C-D of the box
bA=Body()
bA.mask=3
wire=False
color=[0,0.5,1]
highlight=False
bA.shape=PotentialParticle(k=0.1, r=0.1*wallThickness, R=0.5*heightOfBase,a=[1,-1,0,0,0,0], b=[0,0,1,-1,0,0], c=[0,0,0,0,1,-1], d=[0.5*wallThickness-r,0.5*wallThickness-r,0.5*heightOfBase-r,0.5*heightOfBase-r,0.5*lengthOfBase-r,0.5*lengthOfBase-r], id=count,isBoundary=True,color=color,wire=wire,highlight=highlight,AabbMinMax=True, minAabb=1.02*Vector3(0.4*wallThickness,0.5*heightOfBase,0.5*lengthOfBase),maxAabb=1.02*Vector3(0.4*wallThickness,0.5*heightOfBase,0.5*lengthOfBase),maxAabbRotated=1.02*Vector3(0.5*wallThickness,0.5*heightOfBase,0.5*lengthOfBase),minAabbRotated=1.02*Vector3(0.5*wallThickness,0.5*heightOfBase,0.5*lengthOfBase),fixedNormal=False)
#length=lengthOfBase
V=lengthOfBase*heightOfBase*wallThickness
geomInertX=(1./12.)*V*(heightOfBase**2  + lengthOfBase**2)
geomInertY=(1./12.)*V*(wallThickness**2 + lengthOfBase**2)
geomInertZ=(1./12.)*V*(wallThickness**2 + heightOfBase**2)
utils._commonBodySetup(bA, V, Vector3(geomInertX,geomInertY,geomInertZ), material='frictionless', pos=[0,0,0], fixed=True)
bA.state.pos = [0.5*lengthOfBase,0.5*heightOfBase,0]
O.bodies.append(bA)
count=count+1


bB=Body()
bB.mask=3
wire=False
color=[0,0.5,1]
highlight=False
bB.shape=PotentialParticle(k=0.1, r=0.1*wallThickness, R=0.5*heightOfBase,a=[1,-1,0,0,0,0], b=[0,0,1,-1,0,0], c=[0,0,0,0,1,-1], d=[0.5*wallThickness-r,0.5*wallThickness-r,0.5*heightOfBase-r,0.5*heightOfBase-r,0.5*lengthOfBase-r,0.5*lengthOfBase-r], id=count,isBoundary=True,color=color,wire=wire,highlight=highlight,AabbMinMax=True, minAabb=1.02*Vector3(0.4*wallThickness,0.5*heightOfBase,0.5*lengthOfBase),maxAabb=1.02*Vector3(0.4*wallThickness,0.5*heightOfBase,0.5*lengthOfBase),maxAabbRotated=1.02*Vector3(0.5*wallThickness,0.5*heightOfBase,0.5*lengthOfBase),minAabbRotated=1.02*Vector3(0.5*wallThickness,0.5*heightOfBase,0.5*lengthOfBase),fixedNormal=False)
#length=lengthOfBase
V=lengthOfBase*heightOfBase*wallThickness
geomInertX=(1./12.)*V*(heightOfBase**2  + lengthOfBase**2)
geomInertY=(1./12.)*V*(wallThickness**2 + lengthOfBase**2)
geomInertZ=(1./12.)*V*(wallThickness**2 + heightOfBase**2)
utils._commonBodySetup(bB, V, Vector3(geomInertX,geomInertY,geomInertZ), material='frictionless', pos=[0,0,0], fixed=True)
bB.state.pos = [-0.5*lengthOfBase,0.5*heightOfBase,0]
O.bodies.append(bB)
count=count+1


bC=Body()
bC.mask=3
wire=False
color=[0,0.5,1]
highlight=False
bC.shape=PotentialParticle(k=0.1, r=0.1*wallThickness, R=0.5*heightOfBase,a=[1,-1,0,0,0,0], b=[0,0,1,-1,0,0], c=[0,0,0,0,1,-1], d=[0.5*lengthOfBase-r,0.5*lengthOfBase-r,0.5*heightOfBase-r,0.5*heightOfBase-r,0.5*wallThickness-r,0.5*wallThickness-r], id=count,isBoundary=True,color=color,wire=wire,highlight=highlight,AabbMinMax=True, minAabb=1.02*Vector3(0.5*lengthOfBase,0.5*heightOfBase,0.4*wallThickness),maxAabb=1.02*Vector3(0.5*lengthOfBase,0.5*heightOfBase,0.4*wallThickness),maxAabbRotated=1.02*Vector3(0.5*lengthOfBase,0.5*heightOfBase,0.5*wallThickness),minAabbRotated=1.02*Vector3(0.5*lengthOfBase,0.5*heightOfBase,0.5*wallThickness),fixedNormal=False)
#length=lengthOfBase
V=lengthOfBase*heightOfBase*wallThickness
geomInertX=(1./12.)*V*(heightOfBase**2  + lengthOfBase**2)
geomInertY=(1./12.)*V*(wallThickness**2 + lengthOfBase**2)
geomInertZ=(1./12.)*V*(wallThickness**2 + heightOfBase**2)
utils._commonBodySetup(bC, V, Vector3(geomInertX,geomInertY,geomInertZ), material='frictionless', pos=[0,0,0], fixed=True)
bC.state.pos = [0,0.5*heightOfBase,0.5*lengthOfBase]
O.bodies.append(bC)
count=count+1


bD=Body()
bD.mask=3
wire=False
color=[0,0.5,1]
highlight=False
bD.shape=PotentialParticle(k=0.1, r=0.1*wallThickness, R=0.5*heightOfBase,a=[1,-1,0,0,0,0], b=[0,0,1,-1,0,0], c=[0,0,0,0,1,-1], d=[0.5*lengthOfBase-r,0.5*lengthOfBase-r,0.5*heightOfBase-r,0.5*heightOfBase-r,0.5*wallThickness-r,0.5*wallThickness-r], id=count,isBoundary=True,color=color,wire=wire,highlight=highlight,AabbMinMax=True, minAabb=1.02*Vector3(0.5*lengthOfBase,0.5*heightOfBase,0.4*wallThickness),maxAabb=1.02*Vector3(0.5*lengthOfBase,0.5*heightOfBase,0.4*wallThickness),maxAabbRotated=1.02*Vector3(0.5*lengthOfBase,0.5*heightOfBase,0.5*wallThickness),minAabbRotated=1.02*Vector3(0.5*lengthOfBase,0.5*heightOfBase,0.5*wallThickness),fixedNormal=False)
#length=lengthOfBase
V=lengthOfBase*heightOfBase*wallThickness
geomInertX=(1./12.)*V*(heightOfBase**2  + lengthOfBase**2)
geomInertY=(1./12.)*V*(wallThickness**2 + lengthOfBase**2)
geomInertZ=(1./12.)*V*(wallThickness**2 + heightOfBase**2)
utils._commonBodySetup(bD, V, Vector3(geomInertX,geomInertY,geomInertZ), material='frictionless', pos=[0,0,0], fixed=True)
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

##Control the rendering quality
#Gl1_PotentialParticle.aabbEnlargeFactor=1.1
#Gl1_PotentialParticle.sizeX=30
#Gl1_PotentialParticle.sizeY=30
#Gl1_PotentialParticle.sizeZ=30

from yade import qt
qt.Controller()
v=qt.View()

O.saveTmp()

