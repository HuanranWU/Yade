/*************************************************************************
*  2019 Janek Kozicki                                                    *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#pragma once

// This file is to collect all CGAL aliases inside yade sub-namespace

#ifdef YADE_CGAL

#include <lib/high-precision/Real.hpp>
#include <CGAL/AABB_traits.h>
#include <CGAL/AABB_tree.h>
#include <CGAL/AABB_triangle_primitive.h>
#include <CGAL/Delaunay_triangulation_3.h>
#include <CGAL/Filtered_kernel.h>
#include <CGAL/Polyhedron_3.h>
#include <CGAL/Polyhedron_items_with_id_3.h>
#include <CGAL/Simple_cartesian.h>
#include <CGAL/Surface_mesh.h>
#include <CGAL/Tetrahedron_3.h>
#include <CGAL/Triangulation_data_structure_3.h>
#include <CGAL/Triangulation_structural_filtering_traits.h>
#include <CGAL/convex_hull_3.h>
#include <CGAL/linear_least_squares_fitting_3.h>
#include <CGAL/squared_distance_3.h>

namespace CGAL {
// The Exact_predicates_inexact_constructions_kernel used `double`. Make corresponding typedef for Real type.
template <int levelHP> class ERealHP;

// There are two ways to avoid this macro (hint: the best is to use C++20). See file lib/high-precision/RealHPEigenCgal.hpp for details.
#ifdef CGAL_NO_STATIC_FILTERS
#define YADE_REAL_HP_CGAL_KERNEL_ADAPTOR(levelHP)                                                                                                              \
	template <>                                                                                                                                            \
	class ERealHP<levelHP> : public Filtered_kernel_adaptor<                                                                                               \
	                                 Type_equality_wrapper<Simple_cartesian<::yade::RealHP<levelHP>>::Base<ERealHP<levelHP>>::Type, ERealHP<levelHP>>,     \
	                                 false> {                                                                                                              \
	};
#else
#define YADE_REAL_HP_CGAL_KERNEL_ADAPTOR(levelHP)                                                                                                              \
	template <>                                                                                                                                            \
	class ERealHP<levelHP> : public Filtered_kernel_adaptor<                                                                                               \
	                                 Type_equality_wrapper<Simple_cartesian<::yade::RealHP<levelHP>>::Base<ERealHP<levelHP>>::Type, ERealHP<levelHP>>,     \
	                                 true> {                                                                                                               \
	};
#endif

YADE_REGISTER_HP_LEVELS(YADE_REAL_HP_CGAL_KERNEL_ADAPTOR)
#undef YADE_REAL_HP_CGAL_KERNEL_ADAPTOR

using Exact_Real_predicates_inexact_constructions_kernel                          = ERealHP<1>;
template <int levelHP> using Exact_RealHP_predicates_inexact_constructions_kernel = ERealHP<levelHP>;

// There are two ways to avoid this macro (hint: the best is to use C++20). See file lib/high-precision/RealHPEigenCgal.hpp for details.
#define YADE_REAL_HP_TRIANGULATION(levelHP)                                                                                                                    \
	template <> struct Triangulation_structural_filtering_traits<ERealHP<levelHP>> {                                                                       \
		typedef Tag_true Use_structural_filtering_tag;                                                                                                 \
	};

YADE_REGISTER_HP_LEVELS(YADE_REAL_HP_TRIANGULATION)
#undef YADE_REAL_HP_TRIANGULATION

} // namespace CGAL

namespace yade {

// CGAL definitions - does not work with another kernel!! Why???
//   / Answer: because to use exact kernel yade must link with mpfr / Janek.
//   / to do this have a look at CMakeLists 'Real precision' section

// These are taken from files pkg/dem/Polyhedra.hpp and /pkg/dem/Gl1_PotentialBlock.hpp
// let's call this CgalHP struct. It uses inexact kernel. If exact kernel is necessary and ENABLE_MPFR used, we can add similar aliases in other namespace for the exact kernel.

template <int levelHP> struct CgalHP {
	using K              = CGAL::Exact_RealHP_predicates_inexact_constructions_kernel<levelHP>;
	using Polyhedron     = CGAL::Polyhedron_3<K>;
	using Mesh           = CGAL::Surface_mesh<typename K::Point_3>;
	using Triangulation  = CGAL::Delaunay_triangulation_3<K>;
	using CGALpoint      = typename K::Point_3;
	using CGALtriangle   = typename K::Triangle_3;
	using CGALvector     = typename K::Vector_3;
	using Transformation = CGAL::Aff_transformation_3<K>;
	using Segment        = typename K::Segment_3;
	using Triangle       = CGAL::Triangle_3<K>;
	using Plane          = CGAL::Plane_3<K>;
	using Line           = CGAL::Line_3<K>;
	using CGAL_ORIGIN    = CGAL::Origin;
	using CGAL_AABB_tree = CGAL::AABB_tree<CGAL::AABB_traits<K, CGAL::AABB_triangle_primitive<K, typename std::vector<Triangle>::iterator>>>;
};

// the top-level scope refers to RealHP<1>
using K              = CgalHP<1>::K;
using Polyhedron     = CgalHP<1>::Polyhedron;
using Mesh           = CgalHP<1>::Mesh;
using Triangulation  = CgalHP<1>::Triangulation;
using CGALpoint      = CgalHP<1>::CGALpoint;
using CGALtriangle   = CgalHP<1>::CGALtriangle;
using CGALvector     = CgalHP<1>::CGALvector;
using Transformation = CgalHP<1>::Transformation;
using Segment        = CgalHP<1>::Segment;
using Triangle       = CgalHP<1>::Triangle;
using Plane          = CgalHP<1>::Plane;
using Line           = CgalHP<1>::Line;
using CGAL_ORIGIN    = CgalHP<1>::CGAL_ORIGIN;
using CGAL_AABB_tree = CgalHP<1>::CGAL_AABB_tree;

// add ……HP<N> name convention if someone wishes to use it. Similar to Vector3rHP<N>
template <int N> using KHP              = typename CgalHP<N>::K;
template <int N> using PolyhedronHP     = typename CgalHP<N>::Polyhedron;
template <int N> using MeshHP           = typename CgalHP<N>::Mesh;
template <int N> using TriangulationHP  = typename CgalHP<N>::Triangulation;
template <int N> using CGALpointHP      = typename CgalHP<N>::CGALpoint;
template <int N> using CGALtriangleHP   = typename CgalHP<N>::CGALtriangle;
template <int N> using CGALvectorHP     = typename CgalHP<N>::CGALvector;
template <int N> using TransformationHP = typename CgalHP<N>::Transformation;
template <int N> using SegmentHP        = typename CgalHP<N>::Segment;
template <int N> using TriangleHP       = typename CgalHP<N>::Triangle;
template <int N> using PlaneHP          = typename CgalHP<N>::Plane;
template <int N> using LineHP           = typename CgalHP<N>::Line;
template <int N> using CGAL_ORIGINHP    = typename CgalHP<N>::CGAL_ORIGIN;
template <int N> using CGAL_AABB_treeHP = typename CgalHP<N>::CGAL_AABB_tree;

// It would be perhaps useful to collect here similar stuff from other files.
} // namespace yade

#endif
