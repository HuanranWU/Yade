// © 2013 Jan Elias, http://www.fce.vutbr.cz/STM/elias.j/, elias.j@fce.vutbr.cz
// https://www.vutbr.cz/www_base/gigadisk.php?i=95194aa9a

#ifdef YADE_CGAL
#include "Polyhedra.hpp"

namespace yade { // Cannot have #include directive inside.

//***************************************************************************
/*! Create Polyhedra (collision geometry) from colliding Polyhedras. */
class Ig2_Polyhedra_Polyhedra_PolyhedraGeom : public IGeomFunctor {
public:
	virtual ~Ig2_Polyhedra_Polyhedra_PolyhedraGeom() {};
	virtual bool
	             go(const shared_ptr<Shape>&       shape1,
	                const shared_ptr<Shape>&       shape2,
	                const State&                   state1,
	                const State&                   state2,
	                const Vector3r&                shift2,
	                const bool&                    force,
	                const shared_ptr<Interaction>& c);
	virtual bool goReverse(
	        const shared_ptr<Shape>&       shape1,
	        const shared_ptr<Shape>&       shape2,
	        const State&                   state1,
	        const State&                   state2,
	        const Vector3r&                shift2,
	        const bool&                    force,
	        const shared_ptr<Interaction>& c);
	FUNCTOR2D(Polyhedra, Polyhedra);
	DEFINE_FUNCTOR_ORDER_2D(Polyhedra, Polyhedra);
	// clang-format off
		YADE_CLASS_BASE_DOC_ATTRS(Ig2_Polyhedra_Polyhedra_PolyhedraGeom,IGeomFunctor,"Create/update geometry of collision between 2 Polyhedras",
			((Real,interactionDetectionFactor,1,,"see :yref:`Ig2_Sphere_Sphere_ScGeom.interactionDetectionFactor`"))
		);
	// clang-format on
	DECLARE_LOGGER;
};
REGISTER_SERIALIZABLE(Ig2_Polyhedra_Polyhedra_PolyhedraGeom);

//***************************************************************************
/*! Create Polyhedra (collision geometry) from colliding Wall & Polyhedra. */
class Ig2_Wall_Polyhedra_PolyhedraGeom : public IGeomFunctor {
public:
	virtual ~Ig2_Wall_Polyhedra_PolyhedraGeom() {};
	virtual bool
	go(const shared_ptr<Shape>&       shape1,
	   const shared_ptr<Shape>&       shape2,
	   const State&                   state1,
	   const State&                   state2,
	   const Vector3r&                shift2,
	   const bool&                    force,
	   const shared_ptr<Interaction>& c);
	FUNCTOR2D(Wall, Polyhedra);
	DEFINE_FUNCTOR_ORDER_2D(Wall, Polyhedra);
	// clang-format off
		YADE_CLASS_BASE_DOC(Ig2_Wall_Polyhedra_PolyhedraGeom,IGeomFunctor,"Create/update geometry of collision between Wall and Polyhedra");
	// clang-format on
	DECLARE_LOGGER;
};
REGISTER_SERIALIZABLE(Ig2_Wall_Polyhedra_PolyhedraGeom);

//***************************************************************************
/*! Create Polyhedra (collision geometry) from colliding Facet & Polyhedra. */
class Ig2_Facet_Polyhedra_PolyhedraGeom : public IGeomFunctor {
public:
	virtual ~Ig2_Facet_Polyhedra_PolyhedraGeom() {};
	virtual bool
	go(const shared_ptr<Shape>&       shape1,
	   const shared_ptr<Shape>&       shape2,
	   const State&                   state1,
	   const State&                   state2,
	   const Vector3r&                shift2,
	   const bool&                    force,
	   const shared_ptr<Interaction>& c);
	FUNCTOR2D(Facet, Polyhedra);
	DEFINE_FUNCTOR_ORDER_2D(Facet, Polyhedra);
	// clang-format off
		YADE_CLASS_BASE_DOC(Ig2_Facet_Polyhedra_PolyhedraGeom,IGeomFunctor,"Create/update geometry of collision between Facet and Polyhedra");
	// clang-format on
	DECLARE_LOGGER;
};
REGISTER_SERIALIZABLE(Ig2_Facet_Polyhedra_PolyhedraGeom);

//***************************************************************************
/*! Create Polyhedra (collision geometry) from colliding Sphere & Polyhedra. */
class Ig2_Sphere_Polyhedra_ScGeom : public IGeomFunctor {
public:
	enum PointTriangleRelation { inside, edge, vertex, none };
	virtual ~Ig2_Sphere_Polyhedra_ScGeom() {};
	virtual bool
	go(const shared_ptr<Shape>&       shape1,
	   const shared_ptr<Shape>&       shape2,
	   const State&                   state1,
	   const State&                   state2,
	   const Vector3r&                shift2,
	   const bool&                    force,
	   const shared_ptr<Interaction>& c);
	FUNCTOR2D(Sphere, Polyhedra);
	DEFINE_FUNCTOR_ORDER_2D(Sphere, Polyhedra);
	// clang-format off
		YADE_CLASS_BASE_DOC_ATTRS(Ig2_Sphere_Polyhedra_ScGeom,IGeomFunctor,"Create/update geometry of collision between Sphere and Polyhedra",
			((Real,edgeCoeff,1.0,,"multiplier of penetrationDepth when sphere contacts edge (simulating smaller volume of actual intersection or when several polyhedrons has common edge)"))
			((Real,vertexCoeff,1.0,,"multiplier of penetrationDepth when sphere contacts vertex (simulating smaller volume of actual intersection or when several polyhedrons has common vertex)"))
		);
	// clang-format on
	DECLARE_LOGGER;
};
REGISTER_SERIALIZABLE(Ig2_Sphere_Polyhedra_ScGeom);


//***************************************************************************
/*! Plyhedra -> ScGeom. */
class Ig2_Polyhedra_Polyhedra_ScGeom : public IGeomFunctor {
public:
	virtual ~Ig2_Polyhedra_Polyhedra_ScGeom() {};
	virtual bool
	             go(const shared_ptr<Shape>&       shape1,
	                const shared_ptr<Shape>&       shape2,
	                const State&                   state1,
	                const State&                   state2,
	                const Vector3r&                shift2,
	                const bool&                    force,
	                const shared_ptr<Interaction>& c);
	virtual bool goReverse(
	        const shared_ptr<Shape>&       shape1,
	        const shared_ptr<Shape>&       shape2,
	        const State&                   state1,
	        const State&                   state2,
	        const Vector3r&                shift2,
	        const bool&                    force,
	        const shared_ptr<Interaction>& c);
	FUNCTOR2D(Polyhedra, Polyhedra);
	DEFINE_FUNCTOR_ORDER_2D(Polyhedra, Polyhedra);
	// clang-format off
		YADE_CLASS_BASE_DOC_ATTRS(Ig2_Polyhedra_Polyhedra_ScGeom,IGeomFunctor,"EXPERIMENTAL. Ig2 functor creating ScGeom from two Polyhedra shapes. The radii are computed as a distance of contact point (computed using Ig2_Polyhedra_Polyhedra_PolyhedraGeom) and center of particle. Tested only for face-face contacts (like brick wall).",
			((Real,interactionDetectionFactor,1,,"see Ig2_Sphere_Sphere_ScGeom.interactionDetectionFactor"))
		);
	// clang-format on
	DECLARE_LOGGER;
};
REGISTER_SERIALIZABLE(Ig2_Polyhedra_Polyhedra_ScGeom);

class Ig2_Polyhedra_Polyhedra_PolyhedraGeomOrScGeom : public IGeomFunctor {
public:
	virtual ~Ig2_Polyhedra_Polyhedra_PolyhedraGeomOrScGeom() {};
	virtual bool
	             go(const shared_ptr<Shape>&       shape1,
	                const shared_ptr<Shape>&       shape2,
	                const State&                   state1,
	                const State&                   state2,
	                const Vector3r&                shift2,
	                const bool&                    force,
	                const shared_ptr<Interaction>& c);
	virtual bool goReverse(
	        const shared_ptr<Shape>&       shape1,
	        const shared_ptr<Shape>&       shape2,
	        const State&                   state1,
	        const State&                   state2,
	        const Vector3r&                shift2,
	        const bool&                    force,
	        const shared_ptr<Interaction>& c);
	FUNCTOR2D(Polyhedra, Polyhedra);
	DEFINE_FUNCTOR_ORDER_2D(Polyhedra, Polyhedra);
	// clang-format off
		YADE_CLASS_BASE_DOC_ATTRS(Ig2_Polyhedra_Polyhedra_PolyhedraGeomOrScGeom,IGeomFunctor,"EXPERIMENTAL. A hacky helper Ig2 functor combining two Polyhedra shapes to give, according to the settings, either :yref:`ScGeom` or :yref:`PolyhedraGeom`, through appropriate use of either :yref:`Ig2_Polyhedra_Polyhedra_ScGeom` (through :yref:`ig2scGeom<Ig2_Polyhedra_Polyhedra_PolyhedraGeomOrScGeom.ig2scGeom>` attribute) or :yref:`Ig2_Polyhedra_Polyhedra_PolyhedraGeom` (:yref:`ig2polyhedraGeom<Ig2_Polyhedra_Polyhedra_PolyhedraGeomOrScGeom.ig2polyhedraGeom>` attribute).",
			((bool,createScGeom,true,,"When true (resp. false), new contacts' :yref:`IGeom` are created as :yref:`ScGeom` (resp. :yref:`PolyhedraGeom`). Existing contacts are dealt with according to their present IGeom instance."))
			((shared_ptr<Ig2_Polyhedra_Polyhedra_PolyhedraGeom>,ig2polyhedraGeom,new Ig2_Polyhedra_Polyhedra_PolyhedraGeom,,"Helper Ig2 functor responsible for handling :yref:`PolyhedraGeom`."))
			((shared_ptr<Ig2_Polyhedra_Polyhedra_ScGeom>,ig2scGeom,new Ig2_Polyhedra_Polyhedra_ScGeom,,"Helper Ig2 functor responsible for handling :yref:`ScGeom`."))
		);
	// clang-format on
	DECLARE_LOGGER;
};
REGISTER_SERIALIZABLE(Ig2_Polyhedra_Polyhedra_PolyhedraGeomOrScGeom);

//***************************************************************************

} // namespace yade

#endif // YADE_CGAL
