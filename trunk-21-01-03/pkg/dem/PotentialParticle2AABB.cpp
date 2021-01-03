
/*CWBoon 2015 */

#ifdef YADE_POTENTIAL_PARTICLES
#include "PotentialParticle2AABB.hpp"
#include <pkg/common/Aabb.hpp>
#include <pkg/dem/PotentialParticle.hpp>

namespace yade { // Cannot have #include directive inside.

void PotentialParticle2AABB::go(const shared_ptr<Shape>& cm, shared_ptr<Bound>& bv, const Se3r& se3, const Body*)
{
	PotentialParticle* pp = static_cast<PotentialParticle*>(cm.get());
	if (!bv) {
		bv = shared_ptr<Bound>(new Aabb);
	}
	Aabb* aabb = static_cast<Aabb*>(bv.get());

	if (pp->AabbMinMax == false) {
		Real     distFromCentre = 1.0 * pp->R;
		Vector3r halfSize       = (aabbEnlargeFactor > 0 ? aabbEnlargeFactor : 1.) * Vector3r(distFromCentre, distFromCentre, distFromCentre);
		aabb->min               = se3.position - halfSize;
		aabb->max               = se3.position + halfSize;
		return;
	} else {
		//		Matrix3r r=se3.orientation.toRotationMatrix();
		if (pp->vertices.empty()) { // Until a calculation of the vertices of a PP is developed, write the initial Aabb corners into the vertices
			pp->vertices.push_back(Vector3r(pp->maxAabbRotated[0], pp->maxAabbRotated[1], pp->maxAabbRotated[2]));
			pp->vertices.push_back(Vector3r(pp->maxAabbRotated[0], pp->maxAabbRotated[1], -pp->minAabbRotated[2]));
			pp->vertices.push_back(Vector3r(-pp->minAabbRotated[0], -pp->minAabbRotated[1], pp->maxAabbRotated[2]));
			pp->vertices.push_back(Vector3r(-pp->minAabbRotated[0], -pp->minAabbRotated[1], -pp->minAabbRotated[2]));
			pp->vertices.push_back(Vector3r(-pp->minAabbRotated[0], pp->maxAabbRotated[1], pp->maxAabbRotated[2]));
			pp->vertices.push_back(Vector3r(-pp->minAabbRotated[0], pp->maxAabbRotated[1], -pp->minAabbRotated[2]));
			pp->vertices.push_back(Vector3r(pp->maxAabbRotated[0], -pp->minAabbRotated[1], pp->maxAabbRotated[2]));
			pp->vertices.push_back(Vector3r(pp->maxAabbRotated[0], -pp->minAabbRotated[1], -pp->minAabbRotated[2]));
		}
		Vector3r vertex, aabbMin(0, 0, 0), aabbMax(0, 0, 0);
		for (unsigned int i = 0; i < pp->vertices.size(); i++) {
			vertex  = se3.orientation * pp->vertices[i]; // vertices in global coordinates
			aabbMin = aabbMin.cwiseMin(vertex);
			aabbMax = aabbMax.cwiseMax(vertex);
		}
		if (aabbEnlargeFactor > 0) {
			aabbMin *= aabbEnlargeFactor;
			aabbMax *= aabbEnlargeFactor;
		}
		aabb->min = se3.position + aabbMin;
		aabb->max = se3.position + aabbMax;
		return;
	}
}

YADE_PLUGIN((PotentialParticle2AABB));

} // namespace yade

#endif // YADE_POTENTIAL_PARTICLES
