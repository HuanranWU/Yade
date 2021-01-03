/*CWBoon 2015 */

#pragma once
#ifdef YADE_POTENTIAL_PARTICLES

#include <lib/base/openmp-accu.hpp>
#include <lib/compatibility/LapackCompatibility.hpp>
#include <core/Shape.hpp>
#include <Eigen/Core>
#include <Eigen/LU>
#include <Eigen/QR>
#include <vector>
//#include <lib/base/openmp-accu.hpp>


namespace yade { // Cannot have #include directive inside.

class PotentialParticle : public Shape {
public:
	virtual ~PotentialParticle();

	// clang-format off
		YADE_CLASS_BASE_DOC_ATTRS_CTOR(PotentialParticle,Shape,"EXPERIMENTAL. Geometry of PotentialParticle.",
			((int, id, 1,, "Particle id (for graphics in vtk output)")) //TODO: Check if we can use the body id instead in all instances and delete this attribute
			((bool, isBoundary, false,, "Whether the particle is part of a boundary particle"))
			((bool, fixedNormal, false,, "Whether to fix the contact normal at a boundary, using boundaryNormal"))
			((Vector3r, boundaryNormal, Vector3r::Zero(),,"Normal direction of boundary if fixedNormal=True"))
			((bool, AabbMinMax, false,, "Whether the exact Aabb should be calculated. If false, an approximate cubic Aabb is defined with edges of ``2R``"))
			((Vector3r, minAabb, Vector3r::Zero(),,"Min from box centre: Used for visualisation in vtk and qt"))
			((Vector3r, maxAabb, Vector3r::Zero(),,"Max from box centre: Used for visualisation in vtk and qt"))
			((Vector3r, minAabbRotated, Vector3r::Zero(),,"Min from box centre: Used for primary contact detection"))
			((Vector3r, maxAabbRotated, Vector3r::Zero(),,"Max from box centre: Used for primary contact detection"))
//			((Vector3r, halfSize, Vector3r::Zero(),,"max from box centre"))
//			((Quaternionr , oriAabb, Quaternionr::Identity(),, "r "))
			((Real , r, 0.1,, "r in Potential Particles"))
			((Real , R, 1.0,, "R in Potential Particles"))
			((Real , k, 0.1,, "k in Potential Particles"))
			((vector<Vector3r>, vertices,,,"Vertices"))
//			((vector<bool> , isBoundaryPlane, ,, "whether it is a boundaryPlane "))
			((vector<Real> , a, ,, "List of a coefficients of plane normals"))
			((vector<Real> , b, ,, "List of b coefficients of plane normals"))
			((vector<Real> , c, ,, "List of c coefficients of plane normals"))
			((vector<Real> , d, ,, "List of d coefficients of plane normals"))
			, /*ctor*/
			createIndex();
#if 0
		for (int i=0; i<a.size(); i++) {
		Amatrix(i,0) = a[i];
			Amatrix(i,1)=b[i];
			Amatrix(i,2)=c[i];
			Dmatrix(i,0) = d[i] + r;
		}
#endif
		);
	// clang-format on
	//#endif
	REGISTER_CLASS_INDEX(PotentialParticle, Shape);
};

REGISTER_SERIALIZABLE(PotentialParticle);

}; // namespace yade

#endif // YADE_POTENTIAL_PARTICLES
