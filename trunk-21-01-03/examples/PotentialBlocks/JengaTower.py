#!/usr/bin/python3
# 2020 (c) Vasileios Angelidakis <v.angelidakis2@ncl.ac.uk>

# Collapse of a zero-porosity packing of cuboidal blocks in a Jenga-like tower, using the Potential Blocks.

recordVTK=False
# ────────────────────────────────────────────────────────────────────────────────────────────────────────── #
if recordVTK:
	# Create vtk directory
	import os
	import errno
	try:
		os.mkdir('./vtk/')
	except OSError as exc:
		if exc.errno != errno.EEXIST:
			raise
		pass

# ────────────────────────────────────────────────────────────────────────────────────────────────────────── #
# Contact stiffnesses and Engines
kn=1e7
ks=kn/1.5
kn_i=5*kn
ks_i=5*ks

init=1e-6 # initial overlap distance

O.engines=[
	ForceResetter(),
	InsertionSortCollider([PotentialBlock2AABB()],verletDist=0.00), #0.01
	InteractionLoop(
		[Ig2_PB_PB_ScGeom(calContactArea=True)],
		[Ip2_FrictMat_FrictMat_KnKsPBPhys(kn_i=kn_i,ks_i=ks_i,Knormal=kn,Kshear=ks,viscousDamping=0.00)],
		[Law2_SCG_KnKsPBPhys_KnKsPBLaw(initialOverlapDistance=init,allowViscousAttraction=False,label='law')]
	),
	NewtonIntegrator(damping=0.1,exactAsphericalRot=True,gravity=[0,0,-9.81],label='newton'),
]

# ────────────────────────────────────────────────────────────────────────────────────────────────────────── #
# Material
O.materials.append(FrictMat(young=-1,poisson=-1,frictionAngle=radians(10.0),density=400,label='frictional')) 

# ────────────────────────────────────────────────────────────────────────────────────────────────────────── #
# Generate building blocks
length = 0.075 #m = 7.5cm
width  = 0.025 #m = 2.5cm
height = 0.015 #m = 1.5cm

# Piece blocks into a tower
for i in range(0,18): #18 layers
	for j in range(0,3): #3 pieces per layer
		r=0.5*width/2.
		b=Body()
		b.aspherical=True
		color=Vector3(random.random(),random.random(),random.random())
		b.shape=PotentialBlock(r=r, R=0.0, a=[1,-1,0,0,0,0], b=[0,0,1,-1,0,0], c=[0,0,0,0,1,-1], d=[length/2.-r+init,length/2.-r+init,width/2.-r+init,width/2.-r+init,height/2.-r+init,height/2.-r+init], isBoundary=False, color=color, AabbMinMax=True, fixedNormal=False)
		utils._commonBodySetup(b, b.shape.volume, b.shape.inertia, material='frictional',pos=[0,0,0], fixed=False)
		if (i % 2) == 0:
			b.state.pos = [0, width*j-width, height/2.+height*i]
			b.state.ori = b.shape.orientation
		else:
			b.state.pos = [width*j-width, 0, height/2.+height*i]
			b.state.ori = Quaternion((0,0,1),radians(90))
		O.bodies.append(b)

# ────────────────────────────────────────────────────────────────────────────────────────────────────────── #
# Move block 10 on top
O.bodies[10].state.pos=[0, width*2-width, height/2.+height*18]
O.bodies[10].state.ori = O.bodies[10].shape.orientation

# ────────────────────────────────────────────────────────────────────────────────────────────────────────── #
# Generate fixed plate
width = 5 * length #half width
thickness = 0.3 * length
r = 0.5 * thickness

bbb=Body()
bbb.shape=PotentialBlock(r=r, R=0.0, a=[1,-1,0,0,0,0], b=[0,0,1,-1,0,0], c=[0,0,0,0,1,-1], d=[width/2.-r,width/2.-r,width/2.-r,width/2.-r,thickness/2.-r,thickness/2.-r], id=len(O.bodies), isBoundary=True, color=[0,0.5,1], AabbMinMax=True, wire=False, fixedNormal=True, boundaryNormal=[0,0,1])
utils._commonBodySetup(bbb, bbb.shape.volume, bbb.shape.inertia, material='frictional', pos=[0,0,-thickness/2.], fixed=True)
O.bodies.append(bbb)

# ────────────────────────────────────────────────────────────────────────────────────────────────────────── #
# Visualisation
from yade import qt
qt.Controller()
v=qt.View()

v.eyePosition=Vector3(1.3*width,-1.3*width,1.5*length)
v.upVector=Vector3(0,0,1)
v.viewDir=Vector3(-sqrt(2)/2,sqrt(2)/2,0)

v.ortho=False
v.sceneRadius=width

rndr=yade.qt.Renderer()
rndr.lightPos*=-1

# ────────────────────────────────────────────────────────────────────────────────────────────────────────── #
# Timestep
#O.dt = sqrt(0.3*O.bodies[0].state.mass/kn)
O.dt=5e-5

# ────────────────────────────────────────────────────────────────────────────────────────────────────────── #
# Pull block 11 along its long axis
O.engines=O.engines+[TranslationEngine(ids=[11],translationAxis=[0,1,0],velocity=5,label='tEngine')]

# Bring block 11 on top, once removed
def bringOnTop():
	if O.bodies[11].state.pos[1]>length:
		tEngine.dead=True
		O.bodies[11].state.vel=[0,0,0]
		O.bodies[11].state.pos=[0, width*1-width, height/2.+height*18]
		O.bodies[11].state.ori = O.bodies[11].shape.orientation
		bOT.dead=True
		eFP.dead=False

O.engines=O.engines+[PyRunner(iterPeriod=1,command='bringOnTop()',label='bOT')]


def eraseFallenParticles():
	for b in O.bodies:
		if sqrt(b.state.pos[0]**2+b.state.pos[1]**2)>2*length:
			O.bodies.erase(b.id)
O.engines=O.engines+[PyRunner(iterPeriod=500,command='eraseFallenParticles()',dead=True,label='eFP')]

# ────────────────────────────────────────────────────────────────────────────────────────────────────────── #
if recordVTK:
	# Take snapshots
#	O.engines=O.engines+[SnapshotEngine(iterPeriod=100,fileBase='vtk/jenga-',firstIterRun=10,label='snapshooter')]

	# Export VTK results
	from yade import export
	vtkExporter = export.VTKExporter('./vtk/jengaTowerExample')

	def vtkExport():
		vtkExporter.exportPotentialBlocks(what=dict(n='b.id'))

	O.engines=O.engines+[PyRunner(iterPeriod=500,command='vtkExport()')]

# ────────────────────────────────────────────────────────────────────────────────────────────────────────── #
O.saveTmp()
O.run()

