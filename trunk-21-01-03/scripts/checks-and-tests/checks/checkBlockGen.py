# encoding: utf-8
# 2019 © Vasileios Angelidakis <v.angelidakis2@ncl.ac.uk>
# A script to check the block generation algorithm, which subdivides Potential Blocks by a set of predefined discontinuities, creating zero-porosity assemblies of polyhedral particles.

# The BlockGen engine uses the following algorithms:
# CW Boon, GT Houlsby, S Utili (2012).  A new algorithm for contact detection between convex polygonal and polyhedral particles in the discrete element method.  Computers and Geotechnics 44, 73-82. 
# CW Boon, GT Houlsby, S Utili (2015).  A new rock slicing method based on linear programming.  Computers and Geotechnics 65, 12-29. 

if ('PotentialBlocks' in features):

	errors=0
	errMsg=""

	# ----------------------------------------------------------------------------------------------------------------------------------------------- #
	# Set up BlockGen parameters
	p=BlockGen()
	p.maxRatio = 1000.0;
	p.minSize = 7.0;
	p.density = 2650 #kg/m3
	p.dampingMomentum = 0.0
	p.viscousDamping = 0.0
	p.Kn = 2.0e9  	#2 GPa/m
	p.Ks = 0.1e9 	#0.1 GPa/m
	p.frictionDeg = 50.0 #degrees
	p.traceEnergy = False
	p.defaultDt = 1e-4
	p.rForPP = 0.04 #0.04 #0.4
	p.kForPP = 0.0
	p.RForPP = 1800.0 #1800.0
	p.gravity = [0,0,9.81]
	#p.inertiaFactor = 1.0
	#p.initialOverlap = 1e-6
	p.exactRotation = True
	p.shrinkFactor = 1.0
	p.boundarySizeXmin = 20.0; #South
	p.boundarySizeXmax = 20.0; #North
	p.boundarySizeYmin = 20.0; #West
	p.boundarySizeYmax = 20.0; #East
	p.boundarySizeZmin =  0.0; #Up
	p.boundarySizeZmax = 40.0; #Down
	p.persistentPlanes = False
	p.jointProbabilistic = True
	p.opening = False
	p.boundaries = True
	p.slopeFace = False
	p.twoDimension = False
#	p.unitWidth2D = 9.0
	p.calContactArea = True
	p.intactRockDegradation = True
	#p.useFaceProperties = False
	p.neverErase = False # Must be used when tension is on
	p.sliceBoundaries = False
	p.filenamePersistentPlanes = ''
	p.filenameProbabilistic = checksPath+'/data/joints.csv'
	p.filenameOpening = '' #'./joints/opening.csv'
	p.filenameBoundaries = ''
	p.filenameSlopeFace =''
	p.filenameSliceBoundaries = ''
	p.directionA=Vector3(1,0,0)
	p.directionB=Vector3(0,1,0)
	p.directionC=Vector3(0,0,1)

	p.saveBlockGenData=True
	p.outputFile=""

	p.load()
	O.engines[2].lawDispatcher.functors[0].initialOverlapDistance = 0 # = p.initialOverlap - 1e-6
	O.engines[2].lawDispatcher.functors[0].allowBreakage = False

	O.engines[2].physDispatcher.functors[0].kn_i = 2.0e9
	O.engines[2].physDispatcher.functors[0].ks_i = 0.1e9

	# ----------------------------------------------------------------------------------------------------------------------------------------------- #
	# Checks on the generated rockmass
	# ----------------------------------------------------------------------------------------------------------------------------------------------- #
	# Check number of generated blocks
	if len(O.bodies)!=6: #6 bodies
		errMsg+=str( "Wrong number of generated blocks: " + str(len(O.bodies))+" , instead of: "+ str(6) +"\n" )
		errors += 1

	# ----------------------------------------------------------------------------------------------------------------------------------------------- #
	# Check number of vertices of all the generated blocks
	sumVertices = sum( len(b.shape.vertices) for b in O.bodies ) #38 vertices in total
	if sumVertices!=38:
		errMsg+=str( "Wrong number of vertices of all the generated blocks: " + str(sumVertices)+" , instead of: "+ str(38) +"\n" )
		errors += 1

	# ----------------------------------------------------------------------------------------------------------------------------------------------- #
	# Check number of faces of all the generated blocks
	a = sum(len(b.shape.a) for b in O.bodies )
	b = sum(len(b.shape.b) for b in O.bodies )
	c = sum(len(b.shape.c) for b in O.bodies )
	d = sum(len(b.shape.d) for b in O.bodies )

	average=(a+b+c+d)/4.
	if a!=average or b!=average or c!=average or c!=average or a!=32: #32 faces of all bodies
		errMsg+=str( "Wrong calculation of faces of all the generated blocks: " + str(average)+" , instead of: "+ str(32) +"\n" ) #The planes should have the same size of a,b,c,d vectors
		errors += 1

	# ----------------------------------------------------------------------------------------------------------------------------------------------- #
	# Check volume of the rockmass
	tol=1e-6
	sumVolume = sum( b.shape.volume for b in O.bodies )
	target = (p.boundarySizeXmin + p.boundarySizeXmax) * (p.boundarySizeYmin + p.boundarySizeYmax) * (p.boundarySizeZmin + p.boundarySizeZmax); V=target
	if (abs(sumVolume-target)/abs(target)>tol):
		errMsg+=str( "Wrong volume of rockmass: " + str(sumVolume)+" , instead of: "+ str(target) +"\n" )
		errors += 1

	# ----------------------------------------------------------------------------------------------------------------------------------------------- #
	# Check mass of the rockmass
	tol=1e-6
	sumMass = sum (b.state.mass for b in O.bodies)
	target = V*p.density
	if (abs(sumMass-target)/abs(target)>tol):
		errMsg+=str( "Wrong mass of rockmass: " + str(sumMass)+" , instead of: "+ str(target) +"\n" )
		errors += 1

	# ----------------------------------------------------------------------------------------------------------------------------------------------- #
	# Check centroid of the generated rockmass
	tol=1e-6
	target=Vector3.Zero
	target = Vector3( 0.5*(p.boundarySizeXmax-p.boundarySizeXmin), 0.5*(p.boundarySizeYmax-p.boundarySizeYmin), 0.5*(p.boundarySizeZmax-p.boundarySizeZmin) ); centroid=target;
	x = sum( b.state.pos[0]*b.shape.volume for b in O.bodies )/sumVolume
	y = sum( b.state.pos[1]*b.shape.volume for b in O.bodies )/sumVolume
	z = sum( b.state.pos[2]*b.shape.volume for b in O.bodies )/sumVolume

	if (abs( x - target[0] )/abs( 0.5*(p.boundarySizeXmax + p.boundarySizeXmin) ) > tol  or 
	    abs( y - target[1] )/abs( 0.5*(p.boundarySizeYmax + p.boundarySizeYmin) ) > tol  or
	    abs( z - target[2] )/abs( 0.5*(p.boundarySizeZmax + p.boundarySizeZmin) ) > tol  ):
		errMsg+=str( "Wrong centroid of rockmass: " + str(Vector3(x,y,z))+" , instead of: "+ str(target) +"\n" )
		errors += 1

	# ----------------------------------------------------------------------------------------------------------------------------------------------- #
	# Check inertia of the generated rockmass
	tol=1e-6
	target=Vector3.Zero
	target[0] = 1/12. * V * ( (p.boundarySizeYmax + p.boundarySizeYmin)**2 + (p.boundarySizeZmax + p.boundarySizeZmin)**2 )
	target[1] = 1/12. * V * ( (p.boundarySizeXmax + p.boundarySizeXmin)**2 + (p.boundarySizeZmax + p.boundarySizeZmin)**2 )
	target[2] = 1/12. * V * ( (p.boundarySizeXmax + p.boundarySizeXmin)**2 + (p.boundarySizeYmax + p.boundarySizeYmin)**2 )

	inertiaGlobal=Matrix3.Zero
	for b in O.bodies:
		inertiaDiagonal =  b.state.ori.toRotationMatrix() * b.shape.inertia.asDiagonal() * b.state.ori.conjugate().toRotationMatrix()

		relPos = (b.state.pos - centroid)
		dx = relPos[0]
		dy = relPos[1]
		dz = relPos[2]

		Ixx = inertiaDiagonal[0][0] + b.shape.volume * ( dy**2 + dz**2 )
		Iyy = inertiaDiagonal[1][1] + b.shape.volume * ( dx**2 + dz**2 )
		Izz = inertiaDiagonal[2][2] + b.shape.volume * ( dx**2 + dy**2 )
		Ixy = inertiaDiagonal[0][1] - b.shape.volume * ( dx*dy )
		Ixz = inertiaDiagonal[0][2] - b.shape.volume * ( dx*dz )
		Iyz = inertiaDiagonal[1][2] - b.shape.volume * ( dy*dz )

		inertiaDiagonal = Matrix3(Ixx, Ixy, Ixz, Ixy, Iyy, Iyz, Ixz, Iyz, Izz)
		inertiaGlobal = inertiaGlobal + inertiaDiagonal

	if (abs( inertiaGlobal[0][0] - target[0])/abs(target[0]) > tol  or 
	    abs( inertiaGlobal[1][1] - target[1])/abs(target[1]) > tol  or
	    abs( inertiaGlobal[2][2] - target[2])/abs(target[2]) > tol  or

	  ( abs( inertiaGlobal[0][1] ) + abs( inertiaGlobal[1][0] ) + 
	    abs( inertiaGlobal[0][2] ) + abs( inertiaGlobal[2][0] ) + 
	    abs( inertiaGlobal[1][2] ) + abs( inertiaGlobal[2][1] ) 
	  )/abs( inertiaGlobal.trace() ) > tol ):
		errMsg+=str( "Wrong inertia of rockmass: " + str(inertiaGlobal)+" , instead of: "+ str(target.asDiagonal()) +"\n" )
		errors += 1

	# ----------------------------------------------------------------------------------------------------------------------------------------------- #
	if (errors):
		raise YadeCheckError(errMsg)	#Test has failed

else:
	print("skip BlockGen check, PotentialBlocks not available")

