/*************************************************************************
*  2020 Janek Kozicki                                                    *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#ifndef YADE_REAL_HP_EIGEN_CGAL_MINIEIGEN_HPP
#define YADE_REAL_HP_EIGEN_CGAL_MINIEIGEN_HPP

// Select which RealHP<N> are going to work with Eigen, CGAL, minieigenHP, by selecting N values in YADE_EIGENCGAL_HP, YADE_MINIEIGEN_HP
#include <lib/high-precision/RealHPConfig.hpp>
#include <boost/preprocessor.hpp>

/*

This file provides explicit instantinations for Vector3rHP<N> family of types. Before we switch to C++20 this file is unfortunately necessary.

One cannot "just use Vector3rHP<10>", and be happy, because Eigen and CGAL are not flexible enough in template specialization mechanisms.
The RealHP<10> just works, but for Vector3rHP<10> or CgalHP<N> (file lib/base/AliasCGAL.hpp) this file is necessary.

The current solution to this problem is to set the list of supported numbers in RealHPConfig.hpp (separately for C++ and Python):
	C++	: YADE_EIGENCGAL_HP  ↔ The numbers listed here will work in C++ for RealHP<N> in CGAL and Eigen. Rather cheap in compilation time.
	Python	: YADE_MINIEIGEN_HP  ↔ These are exported to python. Expensive: each one makes compilation longer by 1 minute.

Caution: trying to use an unregistered for python Vector3rHP<N> type in YADE_CLASS_BASE_DOC_ATTRS_* to export value to Python will cause problems.
         however it is safe (and intended) to use them in the C++ calculations in critical sections of code, without exporting them to python.
*/

#if ((not defined(YADE_EIGENCGAL_HP)) or (not defined(YADE_MINIEIGEN_HP)))
#error "YADE_EIGENCGAL_HP and YADE_MINIEIGEN_HP should be defined in file RealHPConfig.hpp"
#endif

/*
There are two ways to avoid these macros:

1) modify Eigen library to change the main template declaration from

	template<typename>
	struct NumTraits;

   into

	template<typename, typename=void>
	struct NumTraits;

   then we could use the second typename to enable/disable our template specialization, by writing:

	template <typename Rr>
	struct NumTraits <Rr, typename std::enable_if<::yade::math::isHP<Rr>>::type> : public NumTraitsHP<::yade::math::levelOfRealHP<Rr>> {};

   Do the same with CGAL.
   I checked locally that modification of Eigen library fixes the template specializations and removes all these macros.

2) wait until we move yade to C++20 and then use the https://en.cppreference.com/w/cpp/language/constraints to enable/disable our
   template specializations of NumTraitsRealHP (which can then be renamed to Eigen::NumTraits) using the `requires` and `using`
   C++20 keywords, explained in section "Type requirements". The classes can then be renamed back to original names:

	NumTraitsRealHP				→ Eigen::NumTraits
	RealHP_Is_valid				→ CGAL::Is_valid
	RealHP_Algebraic_structure_traits	→ CGAL::Algebraic_structure_traits 
	RealHP_embeddable_traits		→ CGAL::Real_embeddable_traits

*/

#if ((not defined(YADE_REAL_MATH_NAMESPACE)) or (not defined(YADE_EIGEN_SUPPORT_REAL_HP))                                                                      \
     or (defined(YADE_CGAL) and (not defined(YADE_DISABLE_REAL_MULTI_HP)) and (not defined(YADE_CGAL_SUPPORT_REAL_HP))))
#error "This file cannot be included alone, include Real.hpp instead"
#endif

namespace forCtags {
struct RealHPEigenCgal {
}; // for ctags
}

/*
 * Macro YADE_HP_PYTHON_REGISTER generates an assembly code for a template instantination. It is used in files:
 *
 * py/high-precision/_ExposeBoxes.cpp        py/high-precision/_ExposeConverters.cpp    py/high-precision/_ExposeQuaternion.cpp
 * py/high-precision/_ExposeComplex1.cpp     py/high-precision/_ExposeMatrices1.cpp     py/high-precision/_ExposeVectors1.cpp
 * py/high-precision/_ExposeComplex2.cpp     py/high-precision/_ExposeMatrices2.cpp     py/high-precision/_ExposeVectors2.cpp
 *
 * Each number in YADE_MINIEIGEN_HP, defined above,  makes compilation longer by 1 minute. So put there only the ones which are really needed to be accessed from python.
 */
#define YADE_HP_PY_EIGEN(r, name, levelHP) template void name<levelHP>(bool, const py::scope&);
#define YADE_HP_PYTHON_REGISTER(name) BOOST_PP_SEQ_FOR_EACH(YADE_HP_PY_EIGEN, name, YADE_MINIEIGEN_HP) // instatinate templates for name<1>, name<2>, etc …

/*
 * The float and double are supported by default in Eigen and CGAL, so they have to be skipped.
 * This macro FOR loop 'BOOST_PP_SEQ_FOR_EACH' skips registration of those that are already registered.
 */
#define YADE_SKIP_ARG(arg)
#define YADE_HP_PARSE_SEQUENCE(r, data, levelHP)                                                                                                               \
	BOOST_PP_IF(                                                                                                                                           \
	        BOOST_PP_GREATER_EQUAL(levelHP, /* skip 'below' this number */ BOOST_PP_SEQ_ELEM(1, data)),                                                    \
	        /* execute macro with this 'name'   */ BOOST_PP_SEQ_ELEM(0, data),                                                                             \
	        /* skip the already registered one  */ YADE_SKIP_ARG)                                                                                          \
	(levelHP)
#define YADE_REGISTER_HP_LEVELS_FROM(name, below)                                                                                                              \
	BOOST_PP_SEQ_FOR_EACH(                                                                                                                                 \
	        YADE_HP_PARSE_SEQUENCE, (/* the macro 'name' to be executed */ name)(/* skip numbers 'below' this value */ below), YADE_EIGENCGAL_HP)

#if (YADE_REAL_BIT >= 80)
#define YADE_REGISTER_SELECTED_HP_LEVELS(name) YADE_REGISTER_HP_LEVELS(name) //         it creates: name(1) name(2) name(3) .... using YADE_EIGENCGAL_HP
#elif (YADE_REAL_BIT >= 64)
#define YADE_REGISTER_SELECTED_HP_LEVELS(name) YADE_REGISTER_HP_LEVELS_FROM(name, 2) // it creates:         name(2) name(3) ....
#elif (YADE_REAL_BIT >= 32)
#define YADE_REGISTER_SELECTED_HP_LEVELS(name) YADE_REGISTER_HP_LEVELS_FROM(name, 3) // it creates:                 name(3) ....
#endif

namespace Eigen {

// The YADE_EIGEN_SUPPORT_REAL_HP is provided from file lib/high-precision/EigenNumTraits.hpp
YADE_REGISTER_SELECTED_HP_LEVELS(YADE_EIGEN_SUPPORT_REAL_HP)
#undef YADE_EIGEN_SUPPORT_REAL_HP

} // namespace Eigen

#ifdef YADE_CGAL_SUPPORT_REAL_HP
namespace CGAL {

// The YADE_CGAL_SUPPORT_REAL_HP is provided from file lib/high-precision/CgalNumTraits.hpp
YADE_REGISTER_SELECTED_HP_LEVELS(YADE_CGAL_SUPPORT_REAL_HP)
#undef YADE_CGAL_SUPPORT_REAL_HP

} // namespace CGAL
#endif // YADE_CGAL

#undef YADE_REGISTER_SELECTED_HP_LEVELS

#endif
