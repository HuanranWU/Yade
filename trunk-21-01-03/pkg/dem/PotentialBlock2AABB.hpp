/*CWBoon 2015 */
#ifdef YADE_POTENTIAL_BLOCKS
#pragma once

#include <pkg/common/Dispatching.hpp>
#include <pkg/dem/PotentialBlock.hpp>

namespace yade { // Cannot have #include directive inside.

class PotentialBlock2AABB : public BoundFunctor {
public:
	void go(const shared_ptr<Shape>& cm, shared_ptr<Bound>& bv, const Se3r&, const Body*);

	FUNCTOR1D(PotentialBlock);
	// clang-format off
	YADE_CLASS_BASE_DOC_ATTRS(PotentialBlock2AABB,BoundFunctor,"Functor creating :yref:`Aabb` from :yref:`PotentialBlock`.",
		((Real,aabbEnlargeFactor,((void)"deactivated",-1),,"Relative enlargement of the bounding box; deactivated if negative.\n\n.. note::\n\tThis attribute is used to create distant interaction, but is only meaningful with an :yref:`IGeomFunctor` which will not simply discard such interactions: :yref:`Ig2_Sphere_Sphere_ScGeom::interactionDetectionFactor` should have the same value as :yref:`aabbEnlargeFactor<Bo1_Sphere_Aabb::aabbEnlargeFactor>`."))
	);
	// clang-format on
};

REGISTER_SERIALIZABLE(PotentialBlock2AABB);

} // namespace yade

#endif // YADE_POTENTIAL_BLOCKS
