
/*CWBoon 2015 */
#ifdef YADE_POTENTIAL_BLOCKS
#include "PotentialBlock2AABB.hpp"
#include <pkg/common/Aabb.hpp>
#include <pkg/dem/PotentialBlock.hpp>

namespace yade { // Cannot have #include directive inside.

void PotentialBlock2AABB::go(const shared_ptr<Shape>& cm, shared_ptr<Bound>& bv, const Se3r& se3, const Body*)
{
	PotentialBlock* pp = static_cast<PotentialBlock*>(cm.get());
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

YADE_PLUGIN((PotentialBlock2AABB));

} // namespace yade

#endif // YADE_POTENTIAL_BLOCKS
