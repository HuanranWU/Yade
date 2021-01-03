# encoding: utf-8
# 2019 © Vasileios Angelidakis <v.angelidakis2@ncl.ac.uk>
# A script to check the geometric and interaction properties of two identical overlapping cubic Potential Blocks.

# Uses the following algorithm:
# CW Boon, GT Houlsby, S Utili (2012). A new algorithm for contact detection between convex polygonal and polyhedral particles in the discrete element method. Computers and Geotechnics 44, 73-82. 

if ('PotentialBlocks' in features):
	errors=0
	errMsg=""
	# ----------------------------------------------------------------------------------------------------------------------------------------------- #
	# Engines
	Kn=5e8
	Ks=5e7
	verletDistance=0.0001

	O.engines=[
		ForceResetter(),
		InsertionSortCollider([PotentialBlock2AABB()],verletDist=verletDistance),
		InteractionLoop(
			[Ig2_PB_PB_ScGeom(twoDimension=False, unitWidth2D=1.0, calContactArea=True)],
			[Ip2_FrictMat_FrictMat_KnKsPBPhys( kn_i=Kn, ks_i=Ks, Knormal=Kn, Kshear=Ks, useFaceProperties=False, viscousDamping=0.1)], 
			[Law2_SCG_KnKsPBPhys_KnKsPBLaw(label='law',neverErase=False, allowViscousAttraction=True)] # In this example, we do NOT use Talesnick
		),
		NewtonIntegrator(damping=0.0,exactAsphericalRot=True,gravity=[0,0,0]), # Here we deactivate gravity
	]

	# ----------------------------------------------------------------------------------------------------------------------------------------------- #
	# Materials
	O.materials.append(FrictMat(young=-1,poisson=-1,frictionAngle=radians(30.0),density=2000,label='frictional'))

	# ----------------------------------------------------------------------------------------------------------------------------------------------- #
	# Bodies
	edge=0.10
	r=0.001*edge

	# Body pb1=O.bodies[0]: Green
	pb1=Body()
	pb1.aspherical=True
	pb1.shape=PotentialBlock(
	k=0.0, r=r, R=0.0, a=[1, -1, 0, 0, 0, 0], b=[0, 0, 1, -1, 0, 0], c=[0, 0, 0, 0, 1,-1], d=[edge/2.-r]*6,
	id=len(O.bodies), isBoundary=False, color=[0, 1, .2], wire=False, highlight=False, AabbMinMax=True, fixedNormal=False)
	utils._commonBodySetup(pb1,pb1.shape.volume,pb1.shape.inertia, material='frictional', pos=[0,0,0], fixed=False)
	pb1.state.pos = [0,0,0]
	O.bodies.append(pb1)

	# Body pb2=O.bodies[1]: Red
	pb2=Body()
	pb2.aspherical=True
	pb2.shape=PotentialBlock(
	k=0.0, r=r, R=0.0, a=[1, -1, 0, 0, 0, 0], b=[0, 0, 1, -1, 0, 0], c=[0, 0, 0, 0, 1, -1], d=[edge/2.-r]*6,
	id=len(O.bodies), isBoundary=False, color=[1, .2, 0], wire=False, highlight=False, AabbMinMax=True, fixedNormal=False)
	utils._commonBodySetup(pb2,pb2.shape.volume,pb2.shape.inertia, material='frictional', pos=[0,0,0], fixed=False)
	pb2.state.pos = [edge/2.,0,0]
	O.bodies.append(pb2)

	# ----------------------------------------------------------------------------------------------------------------------------------------------- #
	# Timestep
	O.dt = 1e-15 # A very small timestep

	# ----------------------------------------------------------------------------------------------------------------------------------------------- #

	O.run(10,True)
	c=O.interactions[0,1]

	# ----------------------------------------------------------------------------------------------------------------------------------------------- #
	# Check the vertices of the cubic particles, calculated from the intersection of the planes defining the particle faces
	dim=edge/2.
	for b in O.bodies:
		vertices_sum = (b.shape.vertices.count( Vector3( dim, dim, dim) ) +
				b.shape.vertices.count( Vector3( dim, dim,-dim) ) +
				b.shape.vertices.count( Vector3(-dim,-dim, dim) ) +
				b.shape.vertices.count( Vector3(-dim, dim, dim) ) +
				b.shape.vertices.count( Vector3(-dim, dim,-dim) ) +
				b.shape.vertices.count( Vector3(-dim,-dim,-dim) ) +
				b.shape.vertices.count( Vector3( dim,-dim,-dim) ) +
				b.shape.vertices.count( Vector3( dim,-dim, dim) ) 
		)
		if vertices_sum!=len(b.shape.vertices) or vertices_sum!=8:
			errMsg+=str( "Wrong vertices for body id="+ str(b.shape.id) + ", vertices_sum="+str(vertices_sum)+" len(b.shape.vertices)="+str(len(b.shape.vertices))+"\n" )
			errors += 1

	# ----------------------------------------------------------------------------------------------------------------------------------------------- #
	# Check the volume & inertia of the cubic particles, calculated using the particle vertices & faces
	# ----------------------------------------------------------------------------------------------------------------------------------------------- #
	for b in O.bodies:
		# Check volume
		target=edge**3
		tol=1e-6
		if (abs(b.shape.volume-target)/target>tol):
			errMsg+=str( "Wrong volume for O.bodies[" + str(b.shape.id)+"]: "+ str(b.shape.volume) +" , instead of: "+ str(target) +"\n" )
			errors += 1

		# Check inertia
		target=((1./6.)*target*edge**2)
		tol=1e-6
		if (abs(b.shape.inertia[0] - target)/abs(target) > tol  or 
		    abs(b.shape.inertia[1] - target)/abs(target) > tol  or
		    abs(b.shape.inertia[2] - target)/abs(target) > tol):
			errMsg+=str( "Wrong inertia for O.bodies[" + str(b.shape.id)+"]: "+ str(b.shape.inertia) +" , instead of: "+ str(Vector3(target,target,target)) +"\n" )
			errors += 1

	# Check several known values per functor: Bound-IGeom-IPhys
	# ----------------------------------------------------------------------------------------------------------------------------------------------- #
	# Bound functor checks for PotentialBlock2AABB
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
	# IGeom functor checks for Ig2_PB_PB_ScGeom
	# ----------------------------------------------------------------------------------------------------------------------------------------------- #
	# Check contact point
	target=(pb2.state.pos-pb1.state.pos)/2.
	tol=1e-6
	if (abs(c.geom.contactPoint[0]-target[0])/target.norm()>tol or c.geom.contactPoint[1]!=0 or c.geom.contactPoint[2]!=0):
		errMsg+=str( "Wrong contact point: "+ str(c.geom.contactPoint) +" , instead of: "+ str(target) +"\n" )
		errors += 1

	# Check penetration depth (overlap distance)
	target=edge/2.
	tol=1e-6
	if (abs(c.geom.penetrationDepth-target)/abs(target)>tol):
		errMsg+=str( "Wrong penetration depth: "+ str(c.geom.penetrationDepth) +" , instead of: "+ str(target) +"\n" )
		errors += 1

	# ----------------------------------------------------------------------------------------------------------------------------------------------- #
	# IPhys and Law2 functor checks for Ip2_FrictMat_FrictMat_KnKsPBPhys & Law2_SCG_KnKsPBPhys_KnKsPBLaw
	# ----------------------------------------------------------------------------------------------------------------------------------------------- #
	# Check contact area
	target=edge**2
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
		errMsg+=str( "Wrong Wrong ptOnP1: "+ str(c.phys.ptOnP1[0]) +" , instead of: "+ str(target) +"\n" )
		errors += 1

	if (abs(c.phys.ptOnP2[0])>tol):
		errMsg+=str( "Wrong Wrong ptOnP2: "+ str(c.phys.ptOnP2[0]) +" , instead of: "+ str(0) +"\n" )
		errors += 1

	# ----------------------------------------------------------------------------------------------------------------------------------------------- #
	#TODO: FUTURE checks for the Potential Blocks:
	#TODO: We need to check the normal contact viscous damping force as well
	#TODO: We need to check the shear contact force calculation as well
	#TODO: We need to check the contact behaviour of PotentialBlocks where "shape.isBoundary=True"
	#TODO: We need to check particle vertices & bounding box when it is rotated
	#TODO: We need to create a similar script for 2D contacts as well, with calContactArea=True
	#TODO: We need to create a simple gravity deposition, where the vertical normal force should be equal to the weight of the particles
	#TODO: We need to check the energies present in a simulation during a short collision
	#TODO: We need to check the collision among Potential Blocks in a periodic box
	#TODO: We need to check the correct definition of a PotentialBlock which is not defined centered to its centroid or aligned to its principal axes
	#TODO: We need to create a check script for the RockLining and RockBolt codes

	# ----------------------------------------------------------------------------------------------------------------------------------------------- #
	if (errors):
		raise YadeCheckError(errMsg)	#Test has failed

else:
	print("skip PotentialBlocks check, PotentialBlocks not available")


