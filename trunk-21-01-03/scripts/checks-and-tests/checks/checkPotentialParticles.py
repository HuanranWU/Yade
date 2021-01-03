# encoding: utf-8
# 2019 © Vasileios Angelidakis <v.angelidakis2@ncl.ac.uk>
# A script to check the geometric and interaction properties of two identical overlapping cubic Potential Blocks.

# Uses the following algorithm:
# CW Boon, GT Houlsby, S Utili (2013) A new contact detection algorithm for three dimensional non-spherical particles. Powder Technology 248, S.I. on DEM, 94-102.

if ('PotentialParticles' in features):
	errors=0
	errMsg=""
	# ----------------------------------------------------------------------------------------------------------------------------------------------- #
	# Engines
	Kn=5e8
	Ks=5e7
	verletDistance=0.0001

	O.engines=[
		ForceResetter(),
		InsertionSortCollider([PotentialParticle2AABB()],verletDist=verletDistance),
		InteractionLoop(
			[Ig2_PP_PP_ScGeom(twoDimension=False, calContactArea=True, areaStep=5)],
			[Ip2_FrictMat_FrictMat_KnKsPhys( kn_i=Kn, ks_i=Ks, Knormal=Kn, Kshear=Ks, useFaceProperties=False, viscousDamping=0.1)], 
			[Law2_SCG_KnKsPhys_KnKsLaw(label='law',neverErase=False)]
		),
		NewtonIntegrator(damping=0.0,exactAsphericalRot=True,gravity=[0,0,0]), # Here we deactivate gravity
	]

	# ----------------------------------------------------------------------------------------------------------------------------------------------- #
	# Materials
	O.materials.append(FrictMat(young=-1,poisson=-1,frictionAngle=radians(30.0),density=2000,label='frictional'))

	# ----------------------------------------------------------------------------------------------------------------------------------------------- #
	# Bodies
	edge=0.10
	k=0.8
	r=0.1*edge
	R=edge/2.

	# Body pb1=O.bodies[0]: Green
	pb1=Body()
	pb1.aspherical=True
	pb1.shape=PotentialParticle(
	k=k, r=r, R=R, a=[1, -1, 0, 0, 0, 0], b=[0, 0, 1, -1, 0, 0], c=[0, 0, 0, 0, 1,-1], d=[edge/2.-r]*6,
	id=len(O.bodies), isBoundary=False, color=[0, 1, .2], wire=False, highlight=False, AabbMinMax=True, fixedNormal=False,
	minAabb=Vector3(edge/2., edge/2., edge/2.)*2, minAabbRotated=Vector3(edge/2., edge/2., edge/2.),
	maxAabb=Vector3(edge/2., edge/2., edge/2.)*2, maxAabbRotated=Vector3(edge/2., edge/2., edge/2.) )
	V=edge**3
	geomInertia=1/6.*V*edge**2.
	utils._commonBodySetup(pb1,V,Vector3(geomInertia, geomInertia, geomInertia), material='frictional', pos=[0,0,0], fixed=False)
	pb1.state.pos = [0,0,0]
	O.bodies.append(pb1)

	# Body pb2=O.bodies[1]: Red
	pb2=Body()
	pb2.aspherical=True
	pb2.shape=PotentialParticle(
	k=k, r=r, R=R, a=[1, -1, 0, 0, 0, 0], b=[0, 0, 1, -1, 0, 0], c=[0, 0, 0, 0, 1, -1], d=[edge/2.-r]*6,
	id=len(O.bodies), isBoundary=False, color=[1, .2, 0], wire=False, highlight=False, AabbMinMax=True, fixedNormal=False,
	minAabb=Vector3(edge/2., edge/2., edge/2.)*2, minAabbRotated=Vector3(edge/2., edge/2., edge/2.),
	maxAabb=Vector3(edge/2., edge/2., edge/2.)*2, maxAabbRotated=Vector3(edge/2., edge/2., edge/2.) )
	V=edge**3
	geomInertia=1/6.*V*edge**2.
	utils._commonBodySetup(pb2,V,Vector3(geomInertia, geomInertia, geomInertia), material='frictional', pos=[0,0,0], fixed=False)
	pb2.state.pos = [edge/2.,0,0]
	O.bodies.append(pb2)

	# ----------------------------------------------------------------------------------------------------------------------------------------------- #
	# Timestep
	O.dt = 1e-15 # A very small timestep

	# ----------------------------------------------------------------------------------------------------------------------------------------------- #

	O.run(10,True)
	c=O.interactions[0,1]

#	# ----------------------------------------------------------------------------------------------------------------------------------------------- #
#	# Check the vertices of the rounded cuboidal-like particles  #TODO: Will check these after an automatic calculation is developed

#	# ----------------------------------------------------------------------------------------------------------------------------------------------- #
#	# Check the volume & inertia of the rounded cuboidal-like particles #TODO: Will check these after an automatic calculation is developed
#	# ----------------------------------------------------------------------------------------------------------------------------------------------- #

	# Check several known values per functor: Bound-IGeom-IPhys
	# ----------------------------------------------------------------------------------------------------------------------------------------------- #
	# Bound functor checks for PotentialParticle2AABB
	# ----------------------------------------------------------------------------------------------------------------------------------------------- #
	aabbDimensions=Vector3(edge/2.,edge/2.,edge/2.) + Vector3(verletDistance,verletDistance,verletDistance)
	tol=1e-5

	for b in O.bodies:
		targetMin=b.state.pos-aabbDimensions
		targetMax=b.state.pos+aabbDimensions
		if (abs(b.bound.min[0] - targetMin[0])/abs(targetMin[0]) > tol  or 
		    abs(b.bound.min[1] - targetMin[1])/abs(targetMin[1]) > tol  or
		    abs(b.bound.min[2] - targetMin[2])/abs(targetMin[2]) > tol  or

		    abs(b.bound.max[0] - targetMax[0])/abs(targetMax[0]) > tol  or 
		    abs(b.bound.max[1] - targetMax[1])/abs(targetMax[1]) > tol  or
		    abs(b.bound.max[2] - targetMax[2])/abs(targetMax[2]) > tol  ):
			errMsg+=str( "Wrong bounding box for O.bodies[" + str(b.shape.id)+"]: "+ str(b.bound.min) +" , "+ str(b.bound.max) +" , instead of: "+ str(
targetMin) +" , "+str(targetMax) +"\n" )
			errors += 1

	# ----------------------------------------------------------------------------------------------------------------------------------------------- #
	# IGeom functor checks for Ig2_PP_PP_ScGeom
	# ----------------------------------------------------------------------------------------------------------------------------------------------- #
	# Check contact point
	target=(pb2.state.pos-pb1.state.pos)/2.
	tol=1e-6
	if (abs(c.geom.contactPoint[0]-target[0])/target.norm()>tol or abs(c.geom.contactPoint[1]-target[1])/target.norm()>tol or abs(c.geom.contactPoint[2]-target[2])/target.norm()>tol):
		errMsg+=str( "Wrong contact point: "+ str(c.geom.contactPoint) +" , instead of: "+ str(target) +"\n" )
		errors += 1

	# Check penetration depth (overlap distance)
	target=edge/2.
	tol=1e-6
	if (abs(c.geom.penetrationDepth-target)/abs(target)>tol):
		errMsg+=str( "Wrong penetration depth: "+ str(c.geom.penetrationDepth) +" , instead of: "+ str(target) +"\n" )
		errors += 1

	# ----------------------------------------------------------------------------------------------------------------------------------------------- #
	# IPhys and Law2 functor checks for Ip2_FrictMat_FrictMat_KnKsPhys & Law2_SCG_KnKsPhys_KnKsLaw
	# ----------------------------------------------------------------------------------------------------------------------------------------------- #
	# Check contact area
	target=0.007397007611569197
	tol=1e-6
	if (abs(c.phys.contactArea-target)/abs(target)>tol):
		errMsg+=str( "Wrong contact area: "+ str(c.phys.contactArea) +" , instead of: "+ str(target) +"\n" )
		errors += 1

	# Check normal contact force
	target=c.phys.kn*c.geom.penetrationDepth
	tol=1e-6
	if (abs((c.phys.normalForce).norm()-target)/abs(target)>tol):
		errMsg+=str( "Wrong normal contact force: "+ str((c.phys.normalForce).norm()) +" , instead of: "+ str(target) +"\n" )
		errors += 1

	# Check individual contact points on the particles (used to calculate the penetrationDepth)
	target=edge/2.
	tol=1e-6
	if (abs(c.phys.ptOnP1[0]-target)/abs(target)>tol):
		errMsg+=str( "Wrong ptOnP1: "+ str(c.phys.ptOnP1[0]) +" , instead of: "+ str(target) +"\n" )
		errors += 1

	if (abs(c.phys.ptOnP2[0])>tol):
		errMsg+=str( "Wrong ptOnP2: "+ str(c.phys.ptOnP2[0]) +" , instead of: "+ str(0) +"\n" )
		errors += 1

	# ----------------------------------------------------------------------------------------------------------------------------------------------- #
	#TODO: FUTURE checks for the Potential Particles:
	#TODO: We need to check the normal contact viscous damping force as well
	#TODO: We need to check the shear contact force calculation as well
	#TODO: We need to check the contact behaviour of Potential Particles where "shape.isBoundary=True"
	#TODO: We need to check the bounding box when the particle is rotated

	#TODO: We need to create a simple gravity deposition, where the vertical normal force should be equal to the weight of the particles
	#TODO: We need to check the energies present in a simulation during a short collision
	#TODO: We need to check the collision among Potential Particles in a periodic box

	#TODO: We need to check: vertices/volume/inertia of the Potential Particles once an automatic calculation is developed for them
	#TODO: We need to check the correct definition of a PotentialParticle which is not defined centered to its centroid or aligned to its principal axes, once an automatic calculation is developed
	#TODO: We need to create a similar script for 2D contacts as well, with calContactArea=True

	if ('GUI' in features) or ('GUI-Qt5' in features): #refine visualisation of particle shape if gui is enabled
		Gl1_PotentialParticle.sizeX=50
		Gl1_PotentialParticle.sizeY=50
		Gl1_PotentialParticle.sizeZ=50

	# ----------------------------------------------------------------------------------------------------------------------------------------------- #
	if (errors):
		raise YadeCheckError(errMsg)	#Test has failed

else:
	print("skip PotentialParticles check, PotentialParticles not available")


