/*************************************************************************
*  2020 Bronek Kozicki                                                   *
*  2020 Janek Kozicki                                                    *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#pragma once
#include <lib/high-precision/Real.hpp>

/*
 * In this file the constants are declared. To help the compiler with optimizations during compilation they are declared as constexpr.
 *
 * The problem however is that they have to be declared four times, because:
 * → When they are constexpr they must be initialized inside a class.
 * → To generate symbols for linker they must be declared second time outside of class, and initialized there if not constexpr.
 * → But they can't be constexpr if Real type has more than 33 decimal places precision.
 *
 * Hence above are three IF conditions. There are two possible solutions:
 * 1. Declare/initialize each variable in four places. This is prone to mistakes, so be careful.
 *    We will use this approach for now, because this code is more often read than written. But be careful when adding more Constants
 *
 * 2. Use BOOST_PP_IF condition for preprocessor. This involves a macro, so be careful, but each variable is declared only once.
 *    You can examine the "nice" macro solution in 'git show 549774eeca2f63a877' or in https://gitlab.com/yade-dev/trunk/-/blob/549774eeca2f63a877/lib/high-precision/Constants.hpp#L77
 *
 * Switching between the two solutions is rather straightforward (if needed). The commit 549774eeca2f63a877 has been heavily tested.
 *    Each variable in there is initialized in single place: in the macro Y_DECLARE_CONSTANTS.
 *    To examine what the compiler actually sees invoke this command:
 *      g++ -E -P lib/high-precision/Constants.hpp  -I ./ -I /usr/include/eigen3 -I /usr/include/python3.7m > /tmp/Out.hpp
 *
 *
 * Here the non-macro approach is used. We can quickly switch to macro solution if needed, use commit 549774eeca2f63a877 for that.
 *
 *
 * About initializng the values like epsilon() or max() see:
 *      → https://en.cppreference.com/w/cpp/types/numeric_limits
 *      → https://en.cppreference.com/w/cpp/types/numeric_limits/epsilon
 *      → https://en.cppreference.com/w/cpp/types/numeric_limits/max
 */

// TODO: stop using Mathr::ZERO_TOLERANCE in files pkg/dem/ViscoelasticPM.cpp , pkg/dem/Shop_02.cpp , pkg/dem/STLImporter.cpp and remove this arbitrary constant.

namespace forCtags {
struct Constants { // for ctags
};
}

namespace yade {
namespace math {
	template <typename Rr> const constexpr bool useConstexprConstants = (std::numeric_limits<Rr>::digits10 <= 33);

	// constexpr whenever possible
	template <int N> struct ConstexprConstantsHP {
		static constexpr RealHP<N>    PI          = boost::math::constants::pi<RealHP<N>>();
		static constexpr RealHP<N>    TWO_PI      = boost::math::constants::two_pi<RealHP<N>>();
		static constexpr RealHP<N>    HALF_PI     = boost::math::constants::half_pi<RealHP<N>>();
		static constexpr RealHP<N>    SQRT_TWO_PI = boost::math::constants::root_two_pi<RealHP<N>>();
		static constexpr RealHP<N>    E           = boost::math::constants::e<RealHP<N>>();
		static constexpr ComplexHP<N> I           = ComplexHP<N>(0, 1);
		static const RealHP<N>        DEG_TO_RAD;
		static const RealHP<N>        RAD_TO_DEG;
		static const RealHP<N>        EPSILON;
		static const RealHP<N>        MAX_REAL;
		static constexpr RealHP<N>    ZERO_TOLERANCE = RealHP<N>(1e-20);
	};
	template <int N> constexpr RealHP<N>    ConstexprConstantsHP<N>::PI;
	template <int N> constexpr RealHP<N>    ConstexprConstantsHP<N>::TWO_PI;
	template <int N> constexpr RealHP<N>    ConstexprConstantsHP<N>::HALF_PI;
	template <int N> constexpr RealHP<N>    ConstexprConstantsHP<N>::SQRT_TWO_PI;
	template <int N> constexpr RealHP<N>    ConstexprConstantsHP<N>::E;
	template <int N> constexpr ComplexHP<N> ConstexprConstantsHP<N>::I;
	template <int N> const RealHP<N>        ConstexprConstantsHP<N>::DEG_TO_RAD = ConstexprConstantsHP<N>::PI / RealHP<N>(180);
	template <int N> const RealHP<N>        ConstexprConstantsHP<N>::RAD_TO_DEG = RealHP<N>(180) / ConstexprConstantsHP<N>::PI;
	template <int N> const RealHP<N>        ConstexprConstantsHP<N>::EPSILON    = std::numeric_limits<RealHP<N>>::epsilon();
	template <int N> const RealHP<N>        ConstexprConstantsHP<N>::MAX_REAL   = std::numeric_limits<RealHP<N>>::max();
	template <int N> constexpr RealHP<N>    ConstexprConstantsHP<N>::ZERO_TOLERANCE;

	// const everywhere
	template <int N> struct ConstConstantsHP {
		static const RealHP<N>    PI;
		static const RealHP<N>    TWO_PI;
		static const RealHP<N>    HALF_PI;
		static const RealHP<N>    SQRT_TWO_PI;
		static const RealHP<N>    E;
		static const ComplexHP<N> I;
		static const RealHP<N>    DEG_TO_RAD;
		static const RealHP<N>    RAD_TO_DEG;
		static const RealHP<N>    EPSILON;
		static const RealHP<N>    MAX_REAL;
		static const RealHP<N>    ZERO_TOLERANCE;
	};
	template <int N> const RealHP<N>    ConstConstantsHP<N>::PI             = boost::math::constants::pi<RealHP<N>>();
	template <int N> const RealHP<N>    ConstConstantsHP<N>::TWO_PI         = boost::math::constants::two_pi<RealHP<N>>();
	template <int N> const RealHP<N>    ConstConstantsHP<N>::HALF_PI        = boost::math::constants::half_pi<RealHP<N>>();
	template <int N> const RealHP<N>    ConstConstantsHP<N>::SQRT_TWO_PI    = boost::math::constants::root_two_pi<RealHP<N>>();
	template <int N> const RealHP<N>    ConstConstantsHP<N>::E              = boost::math::constants::e<RealHP<N>>();
	template <int N> const ComplexHP<N> ConstConstantsHP<N>::I              = ComplexHP<N>(0, 1);
	template <int N> const RealHP<N>    ConstConstantsHP<N>::DEG_TO_RAD     = ConstConstantsHP<N>::PI / RealHP<N>(180);
	template <int N> const RealHP<N>    ConstConstantsHP<N>::RAD_TO_DEG     = RealHP<N>(180) / ConstConstantsHP<N>::PI;
	template <int N> const RealHP<N>    ConstConstantsHP<N>::EPSILON        = std::numeric_limits<RealHP<N>>::epsilon();
	template <int N> const RealHP<N>    ConstConstantsHP<N>::MAX_REAL       = std::numeric_limits<RealHP<N>>::max();
	template <int N> const RealHP<N>    ConstConstantsHP<N>::ZERO_TOLERANCE = RealHP<N>(1e-20);

	template <int N> using ConstantsHP = typename std::conditional<useConstexprConstants<RealHP<N>>, ConstexprConstantsHP<N>, ConstConstantsHP<N>>::type;

} // namespace math

using Mathr = math::ConstantsHP<1>;

} // namespace yade
