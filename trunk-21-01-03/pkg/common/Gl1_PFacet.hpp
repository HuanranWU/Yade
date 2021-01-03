#pragma once
#include <core/Shape.hpp>
#include <pkg/common/Facet.hpp>
#include <pkg/common/GLDrawFunctors.hpp>
#include <pkg/common/PFacet.hpp>

namespace yade { // Cannot have #include directive inside.

#ifdef YADE_OPENGL
class Gl1_PFacet : public GlShapeFunctor {
public:
	virtual void go(const shared_ptr<Shape>&, const shared_ptr<State>&, bool, const GLViewInfo&);
	RENDERS(PFacet);
	// clang-format off
	YADE_CLASS_BASE_DOC_STATICATTRS(Gl1_PFacet,GlShapeFunctor,"Renders :yref:`Facet` object",
	  ((bool,wire,false,,"Only show wireframe (controlled by ``glutSlices`` and ``glutStacks``."))
	);
	// clang-format on
};

REGISTER_SERIALIZABLE(Gl1_PFacet);
#endif

} // namespace yade
