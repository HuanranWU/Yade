/*CWBoon 2015 */
#ifdef YADE_POTENTIAL_BLOCKS
#pragma once

#include <lib/compatibility/LapackCompatibility.hpp>
#include <core/Shape.hpp>
#include <Eigen/Core>
#include <Eigen/LU>
#include <Eigen/QR>
#include <vector>
//#include <lib/base/openmp-accu.hpp>

namespace yade { // Cannot have #include directive inside.


class PotentialBlock : public Shape {
public:
	struct Planes {
		vector<int> vertexID;
	};
	//		struct Vertices{ vector<int> edgeID;  vector<int> planeID; };
	//		struct Edges{ vector<int> vertexID; };

	void addPlaneStruct();
	//		void addVertexStruct();
	//		void addEdgeStruct();

	vector<Planes> planeStruct;
	//		vector<Vertices> vertexStruct;
	//		vector<Edges> edgeStruct;

	//		MatrixXr Amatrix;
	//		MatrixXr Dmatrix;
	virtual ~PotentialBlock();
	void postLoad(PotentialBlock&);

	Real getDet(const MatrixXr A);
	Real getSignedArea(const Vector3r pt1, const Vector3r pt2, const Vector3r pt3);
	void calculateVertices();
	void calculateInertia(Vector3r& centroid, Real& Ixx, Real& Iyy, Real& Izz, Real& Ixy, Real& Ixz, Real& Iyz);

	// clang-format off
	YADE_CLASS_BASE_DOC_ATTRS_CTOR(PotentialBlock,Shape,"Geometry of PotentialBlock.",
		((bool, isLining, false,, "Whether particle is part of tunnel lining (used in the RockLining.cpp script)"))
		((Real, liningStiffness, pow(10.0,8),, "Lining stiffness"))
		((Real, liningFriction, 20.0,, "Lining friction"))
		((Real, liningLength, 0.0,, "Lining spacing between nodes"))
		((Real, liningTensionGap, 0.0,, "Numerical gap between lining and block to allowing tension to be calculated"))
		((Vector3r, liningNormalPressure, Vector3r(0,0,0),, "Normal pressure acting on lining"))
		((Vector3r, liningTotalPressure, Vector3r(0,0,0),, "Total pressure acting on lining"))
		((bool, isBoundary, false,, "Whether the particle is part of a boundary block"))
//			((bool, isEastBoundary, false,Attr::hidden, "Boolean used for a case history"))
		((bool, isBolt, false,, "Whether a block is part of a bolt (used in the Rockbolt.cpp script)"))
		((bool, fixedNormal, false,, "Whether to fix the contact normal at a boundary, using boundaryNormal"))
		((Vector3r, boundaryNormal, Vector3r::Zero(),,"Normal direction of boundary if fixedNormal=True"))
			((bool, AabbMinMax, false,, "Whether the exact Aabb should be calculated. If false, an approximate cubic Aabb is defined with edges of ``2R``"))
		((Vector3r, minAabb, Vector3r::Zero(),,"Min from box centre: Used for visualisation in vtk"))
		((Vector3r, maxAabb, Vector3r::Zero(),,"Max from box centre: Used for visualisation in vtk"))
//			((Vector3r, minAabbRotated, Vector3r::Zero(),,"(not used: Actually used in the BlockGen, but not needed!) min from box centre"))
//			((Vector3r, maxAabbRotated, Vector3r::Zero(),,"(not used: Actually used in the BlockGen, but not needed!) max from box centre"))
//			((Vector3r, halfSize, Vector3r::Zero(),Attr::hidden,"max from box centre"))
//			((vector<Vector3r>, node, ,Attr::hidden, "(not used)"))
//			((Real, gridVol, ,Attr::hidden, "(not used)"))
//			((Quaternionr, oriAabb, Quaternionr::Identity(),, "(not used) original aabb "))
		((Real, r, 0.0,, "r in Potential Particles"))
		((Real, R, 0.0,, "R in Potential Particles. If left zero, a default value is calculated as half the distance of the farthest vertices"))
		((Real, k, 0.0,, "k in Potential Particles (not used)"))
		((Real, volume, ,, "Volume |yupdate|"))
		((Vector3r, inertia, Vector3r::Zero(),, "Principal inertia tensor |yupdate|"))
		((Vector3r, position, Vector3r::Zero(),, "Initial position of the particle, if initially defined eccentrically to the centroid |yupdate|"))
		((Quaternionr, orientation, Quaternionr::Identity(),, "Principal orientation"))
		((int, id, -1,, "Particle id (for graphics in vtk output)")) //TODO: Check if we can use the body id instead in all instances and delete this attribute
		((bool, erase, false,, "Parameter to mark particles to be removed (for excavation)"))
		((vector<bool>, intactRock, false,, "Property for plane")) //TODO: Provide more info on the functionality of this attribute
//			((vector<bool>, isBoundaryPlane, ,Attr::hidden, "Property for plane (not used)"))
//			((vector<Real>, hwater, ,Attr::hidden, "Property for plane (not used), height of pore water"))
//			((vector<Real>, JRC, ,Attr::hidden, "Property for plane (not used), rock joint"))
//			((vector<Real>, JCS, ,Attr::hidden, "Property for plane (not used), rock joint"))
//			((vector<Real>, asperity, ,Attr::hidden, "Property for plane (not used), rock joint"))
//			((vector<Real>, sigmaC, ,Attr::hidden,"Property for plane (not used), rock joint"))
		((vector<Real>, phi_b, ,, "Basic friction angle of each face (property for plane, rock joint)"))
		((vector<Real>, phi_r, ,, "Residual friction angle of each face (property for plane, rock joint)"))
		((vector<Real>, cohesion, ,, "Cohesion (stress) of each face (property for plane, rock joint)"))
		((vector<Real>, tension, ,, "Tension (stress) of each face (property for plane, rock joint)"))
		((vector<int>, jointType, ,, "jointType"))
//			((vector<Real>, lambda0, ,Attr::hidden, "Property for plane (not used), heat"))
//			((vector<Real>, kn, ,Attr::hidden, "Property for plane, rock joint (not used: would be used in that each face could have it's own stiffness properties (not developed))"))
//			((vector<Real>, ks, ,Attr::hidden, "Property for plane, rock joint (not used: would be used in that each face could have it's own stiffness properties (not developed))"))
//			((vector<Real>, heatCapacity, ,Attr::hidden, "Property for plane, rock joint"))
//			((vector<Real>, rFactor, ,Attr::hidden, "(not used), individual factor for r"))
		((vector<Vector3r>, vertices, ,(Attr::readonly),"Vertices |yupdate|"))
		//((MatrixXr , Amatrix, ,, "a "))
		//((MatrixXr , Dmatrix, ,, "b "))
//			((Real, waterVolume, ,, "volume of body submerged in water"))
//			((vector<Vector3r>, verticesCD, ,, "vertices"))
		((vector<Real>, a, ,, "List of a coefficients of plane normals"))
		((vector<Real>, b, ,, "List of b coefficients of plane normals"))
		((vector<Real>, c, ,, "List of c coefficients of plane normals"))
		((vector<Real>, d, ,, "List of d coefficients of plane equations"))
		((vector<vector<int> >, connectivity, ,(Attr::readonly), "Connectivity of vertices for each plane |yupdate|"))
		, /*ctor*/
		createIndex();
		#if 0
		for (int i=0; i<a.size(); i++){
			Amatrix(i,0) = a[i]; Amatrix(i,1)=b[i]; Amatrix(i,2)=c[i];
			Dmatrix(i,0) = d[i] + r;
	 	}
		#endif
	);
	// clang-format on
	//#endif
	REGISTER_CLASS_INDEX(PotentialBlock, Shape);
};

REGISTER_SERIALIZABLE(PotentialBlock);

} // namespace yade

#endif // YADE_POTENTIAL_BLOCKS
