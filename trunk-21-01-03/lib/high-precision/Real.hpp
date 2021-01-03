/*************************************************************************
*  2019 Janek Kozicki                                                    *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#ifndef YADE_REAL_DETECTION_HPP
#define YADE_REAL_DETECTION_HPP

/* This file takes following defines as inputs:
 *
 * #define YADE_REAL_BIT  int(number of bits)
 * #define YADE_REAL_DEC  int(number of decimal places)
 * #define YADE_REAL_MPFR    // defined when Real is expressed using mpfr (requires mpfr)
 * #define YADE_REAL_BBFLOAT // defined when Real is expressed using boost::multiprecision::cpp_bin_float
 *
 * A hardware-accelerated numerical type is used when available, otherwise mpfr or boost::cpp_bin_float.hpp is used for arbitrary precision. The supported types are following:
 *
 *     type                   bits     decimal places         notes
 *     ---------------------------------------------------------------------------
 *     float,float32_t        32       6                      hardware accelerated
 *     double,float64_t       64       15                     hardware accelerated
 *     long double,float80_t  80       18                     hardware accelerated
 *     float128_t             128      33                     depending on processor type it can be hardware accelerated
 *     mpfr                   Nbit     Nbit/(log(2)/log(10))
 *     boost::cpp_bin_float   Nbit     Nbit/(log(2)/log(10))
 *
 *
 * header <boost/cstdfloat.hpp> guarantees standardized floating-point typedefs having specified widths
 * https://www.boost.org/doc/libs/1_71_0/libs/math/doc/html/math_toolkit/specified_typedefs.html
 *
 * Depending on platform the fastest one is chosen.
 * https://www.boost.org/doc/libs/1_71_0/libs/math/doc/html/math_toolkit/fastest_typdefs.html
 *
 * TODO:
 * It is possible to extend RealHP<N> with a type trait. A Tag class in definition of UnderlyingReal or Real or RealHP to select which of these extensions are selected to be used:
 *   → regular RealHP<N>
 *   → selecting a different rounding mode policy
 *       An important and easy test of numerical stability is to run the same calculations with different rounding modes:
 *       page 54 "How Futile are Mindless Assessments of Roundoff in Floating-Point Computation ?" in https://people.eecs.berkeley.edu/~wkahan/Mindless.pdf
 *       https://www.boost.org/doc/libs/1_73_0/libs/numeric/interval/doc/rounding.htm
 *   → MPFI, Interval Arithmetic (IA) types:
 *       see chapter about IA and conclusions in https://people.eecs.berkeley.edu/~wkahan/Mindless.pdf "How Futile are Mindless Assessments of Roundoff in Floating-Point Computation ?"
 *       https://www.boost.org/doc/libs/1_71_0/libs/multiprecision/doc/html/boost_multiprecision/tut/interval/mpfi.html
 *       "Choosing Your Own Interval Type" https://www.boost.org/doc/libs/1_73_0/libs/numeric/interval/doc/guide.htm
 *   → Arb
 *   → flint library
 *   → Automatic differentiation:
 *       https://www.boost.org/doc/libs/master/libs/math/doc/html/math_toolkit/autodiff.html #include <boost/math/differentiation/autodiff.hpp>
 *       https://github.com/boostorg/math/pull/176#issuecomment-454174960
 *       https://github.com/vincent-picaud/MissionImpossible
 *       Eigen::AutoDiffScalar  A scalar type replacement with automatic differentation capability. https://eigen.tuxfamily.org/dox/unsupported/classEigen_1_1AutoDiffScalar.html
 *   → ………
 *
 * Some hints how to use float128
 *   https://www.boost.org/doc/libs/1_71_0/libs/math/doc/html/math_toolkit/float128_hints.html
 *   https://www.boost.org/doc/libs/1_71_0/libs/multiprecision/doc/html/boost_multiprecision/tut/floats/float128.html
 *   e.g.: "Make sure std lib functions are called unqualified so that the correct overload is found via Argument Dependent Lookup (ADL). So write sqrt(variable) and not std::sqrt(variable)."
 *   Eigen author recommends using boost::multiprecision::float128 ← https://forum.kde.org/viewtopic.php?f=74&t=140253
 *
 * Some hints how to use mpfr
 *   https://www.boost.org/doc/libs/1_71_0/libs/math/doc/html/index.html
 *   https://www.boost.org/doc/libs/1_71_0/boost/math/tools/precision.hpp
 *   https://www.boost.org/doc/libs/1_71_0/libs/multiprecision/doc/html/index.html
 *   allocate_stack → faster calculations - allocates numbers on stack instead of on heap, this works only with relatively small precisions.
 *   et_on          → faster calculations, slower compilation → FIXME - compile error in Eigen::abs2(…)
 *   et_off         → slower calculations, faster compilation
 */

#include <boost/config.hpp>
#include <boost/version.hpp>
#if (((__GNUC__ > 7) or (not BOOST_GCC)) and (BOOST_VERSION >= 106700))
#define BOOST_CSTDFLOAT_NO_LIBQUADMATH_COMPLEX // see issue https://github.com/boostorg/math/issues/350
#include <boost/cstdfloat.hpp>
#else
namespace boost {
// the cstdfloat.hpp makes sure these are fastest types for each particular platform. But including this file for g++ version <=7 does not work.
using float_fast32_t = float;
using float_fast64_t = double;
using float_fast80_t = long double;
}
#endif
#include <cmath>
#include <complex>
#include <limits>

#define EIGEN_DONT_PARALLELIZE

#include <Eigen/Core>

/*************************************************************************/
/*************************    float 32 bits     **************************/
/*************************************************************************/
#if YADE_REAL_BIT <= 32
namespace yade {
namespace math {
	using UnderlyingReal = boost::float_fast32_t;
}
}

/*************************************************************************/
/*************************   double 64 bits     **************************/
/*************************************************************************/
#elif YADE_REAL_BIT <= 64
namespace yade {
namespace math {
	using UnderlyingReal = boost::float_fast64_t;
}
}

/*************************************************************************/
/************************* long double 80 bits  **************************/
/*************************************************************************/
#elif YADE_REAL_BIT <= 80
namespace yade {
namespace math {
	using UnderlyingReal = boost::float_fast80_t;
}
}

/*************************************************************************/
/*************************  float128 128 bits   **************************/
/*************************************************************************/
#elif YADE_REAL_BIT <= 128
#include <boost/multiprecision/float128.hpp>
// TODO: boost 1.68 has #include <boost/multiprecision/complex128.hpp>, which would simplify some things
#include <quadmath.h>
namespace yade {
namespace math {
	using UnderlyingReal = boost::multiprecision::float128;
}
}

/*************************************************************************/
/*************************         MPFR         **************************/
/*************************************************************************/
#elif defined(YADE_REAL_MPFR)
#include <boost/multiprecision/mpfr.hpp>
namespace yade {
namespace math {
	template <unsigned int DecimalPlaces>
	using UnderlyingRealBackend = boost::multiprecision::mpfr_float_backend<DecimalPlaces, boost::multiprecision::allocate_stack>;
	using UnderlyingReal        = boost::multiprecision::number<UnderlyingRealBackend<YADE_REAL_DEC>, boost::multiprecision::et_off>;
}
}

/*************************************************************************/
/************************* boost::cpp_bin_float **************************/
/*************************************************************************/
#elif defined(YADE_REAL_BBFLOAT)
#include <boost/multiprecision/cpp_bin_float.hpp>
namespace yade {
namespace math {
	template <unsigned int DecimalPlaces> using UnderlyingRealBackend = boost::multiprecision::cpp_bin_float<DecimalPlaces>;
	using UnderlyingReal = boost::multiprecision::number<UnderlyingRealBackend<YADE_REAL_DEC>, boost::multiprecision::et_off>;
}
}

/*************************************************************************/
#else
#error "Real precision is unspecified, there must be a mistake in CMakeLists.txt, the requested #defines should have been provided."
#endif

/*************************************************************************/
/*********************** YADE_REAL_MATH_NAMESPACE ************************/
/*************************************************************************/
// The YADE_REAL_MATH_NAMESPACE macro makes sure that proper namespace is used for Real and eventual RealHP<…> types.
// On older systems RealHP<…> can't work, so we need to check if it was explicitly disabled during cmake autodetection.
#if ((YADE_REAL_BIT <= 80) and defined(YADE_DISABLE_REAL_MULTI_HP))
#define YADE_REAL_MATH_NAMESPACE ::std
#else
#define YADE_REAL_MATH_NAMESPACE ::boost::multiprecision
#endif

/*************************************************************************/
/*************************     AND  FINALLY     **************************/
/*************************     declare Real     **************************/
/*************************************************************************/

#if (YADE_REAL_BIT <= 80) and (YADE_REAL_BIT > 64)
// `long double` needs special consideration to workaround boost::python losing 3 digits precision
#include "ThinComplexWrapper.hpp"
#include "ThinRealWrapper.hpp"

namespace yade {
namespace math {
	using Real    = ThinRealWrapper<UnderlyingReal>;
	using Complex = ThinComplexWrapper<std::complex<UnderlyingReal>>;
}
}

#include "NumericLimits.hpp"
#else

namespace yade {
namespace math {
	using Real    = UnderlyingReal;
	using Complex = std::complex<UnderlyingReal>;
}
}

#endif

namespace yade {
using Complex = math::Complex;
using Real    = math::Real;
static_assert(sizeof(Real) == sizeof(math::UnderlyingReal), "This compiler introduced padding, which breaks binary compatibility");
static_assert(sizeof(Complex) == sizeof(std::complex<math::UnderlyingReal>), "This compiler introduced padding, which breaks binary compatibility");
}

/*************************************************************************/
/*************************      RealHP<…>       **************************/
/*************************************************************************/
#include "RealHP.hpp"

/*************************************************************************/
/*************************    Math functions    **************************/
/*************************************************************************/
#include "MathFunctions.hpp"

/*************************************************************************/
/*************************   Eigen  NumTraits   **************************/
/*************************************************************************/
#include "EigenNumTraits.hpp"

/*************************************************************************/
/*************************    CGAL NumTraits    **************************/
/*************************************************************************/
#ifdef YADE_CGAL
#include "CgalNumTraits.hpp"
#endif

/*************************************************************************/
/************************* Vector3 Matrix3 etc  **************************/
/*************************************************************************/
#include "MathEigenTypes.hpp"

/*************************************************************************/
/************************* RealHP<…> Eigen,CGAL **************************/
/*************************************************************************/
// Select which RealHP<N> are going to work with Eigen, CGAL, minieigenHP:
#include "RealHPConfig.hpp"
// Make them work:
#include "RealHPEigenCgal.hpp"

#undef YADE_REAL_MATH_NAMESPACE

/*************************************************************************/
/*************************  Some sanity checks  **************************/
/*************************************************************************/
#if defined(YADE_REAL_BBFLOAT) and defined(YADE_REAL_MPFR)
#error "Specify either YADE_REAL_MPFR or YADE_REAL_BBFLOAT"
#endif
#if defined(__INTEL_COMPILER) and (YADE_REAL_BIT > 80) and (YADE_REAL_BIT <= 128)
#warning "Intel compiler notes, see: https://www.boost.org/doc/libs/1_71_0/libs/multiprecision/doc/html/boost_multiprecision/tut/floats/float128.html"
#warning "Intel compiler notes: about using flags -Qoption,cpp,--extended_float_type"
#endif

/*************************************************************************/
/**  workaround odeint bug https://github.com/boostorg/odeint/issues/40 **/
/*************************************************************************/
#if BOOST_VERSION >= 106800
#include "WorkaroundBoostOdeIntBug.hpp"
#endif

#endif
