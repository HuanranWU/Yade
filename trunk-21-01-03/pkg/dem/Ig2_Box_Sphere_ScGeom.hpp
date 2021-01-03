/*************************************************************************
*  Copyright (C) 2004 by Olivier Galizzi                                 *
*  olivier.galizzi@imag.fr                                               *
*  Copyright (C) 2004 by Janek Kozicki                                   *
*  cosurgi@berlios.de                                                    *
*  Copyright (C) 2006 by Bruno Chareyre                                  *
*  bruno.chareyre@grenoble-inp.fr                                            *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#pragma once
#include <pkg/common/Box.hpp>
#include <pkg/common/Dispatching.hpp>
#include <pkg/common/Sphere.hpp>

namespace yade { // Cannot have #include directive inside.

class Ig2_Box_Sphere_ScGeom : public IGeomFunctor {
public:
	virtual bool
	go(const shared_ptr<Shape>&       cm1,
	   const shared_ptr<Shape>&       cm2,
	   const State&                   state1,
	   const State&                   state2,
	   const Vector3r&                shift2,
	   const bool&                    force,
	   const shared_ptr<Interaction>& c);

	virtual bool goReverse(
	        const shared_ptr<Shape>&       cm1,
	        const shared_ptr<Shape>&       cm2,
	        const State&                   state1,
	        const State&                   state2,
	        const Vector3r&                shift2,
	        const bool&                    force,
	        const shared_ptr<Interaction>& c);

	// clang-format off
	YADE_CLASS_BASE_DOC_ATTRS(Ig2_Box_Sphere_ScGeom,IGeomFunctor,"Create an interaction geometry :yref:`ScGeom` from :yref:`Box` and :yref:`Sphere`, representing the box with a projected virtual sphere of same radius.",
    ((Real,interactionDetectionFactor,1,,"Enlarge sphere radii by this factor (if >1), to permit creation of distant interactions.\n\nInteractionGeometry will be computed when interactionDetectionFactor*(rad) > distance.\n\n.. note::\n\t This parameter is functionally coupled with :yref:`Bo1_Sphere_Aabb::aabbEnlargeFactor`, which will create larger bounding boxes and should be of the same value.")))
	// clang-format on
	FUNCTOR2D(Box, Sphere);
	DEFINE_FUNCTOR_ORDER_2D(Box, Sphere);
};
REGISTER_SERIALIZABLE(Ig2_Box_Sphere_ScGeom);

class Ig2_Box_Sphere_ScGeom6D : public Ig2_Box_Sphere_ScGeom {
public:
	virtual bool
	go(const shared_ptr<Shape>&       cm1,
	   const shared_ptr<Shape>&       cm2,
	   const State&                   state1,
	   const State&                   state2,
	   const Vector3r&                shift2,
	   const bool&                    force,
	   const shared_ptr<Interaction>& c);

	virtual bool goReverse(
	        const shared_ptr<Shape>&       cm1,
	        const shared_ptr<Shape>&       cm2,
	        const State&                   state1,
	        const State&                   state2,
	        const Vector3r&                shift2,
	        const bool&                    force,
	        const shared_ptr<Interaction>& c);

	// clang-format off
	YADE_CLASS_BASE_DOC(Ig2_Box_Sphere_ScGeom6D,Ig2_Box_Sphere_ScGeom,"Create an interaction geometry :yref:`ScGeom6D` from :yref:`Box` and :yref:`Sphere`, representing the box with a projected virtual sphere of same radius.")
	// clang-format on
	FUNCTOR2D(Box, Sphere);
	DEFINE_FUNCTOR_ORDER_2D(Box, Sphere);
};
REGISTER_SERIALIZABLE(Ig2_Box_Sphere_ScGeom6D);

} // namespace yade
