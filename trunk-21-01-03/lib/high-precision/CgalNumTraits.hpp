/*************************************************************************
*  2019 Janek Kozicki                                                    *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#if (((not defined(YADE_DISABLE_REAL_MULTI_HP)) or (YADE_REAL_BIT > 64)) and defined(YADE_CGAL) and (not defined(CGAL_NUM_TRAITS_HPP)))
#define CGAL_NUM_TRAITS_HPP

#ifndef YADE_REAL_MATH_NAMESPACE
#error "This file cannot be included alone, include Real.hpp instead"
#endif

#include <CGAL/Interval_nt.h>
#include <CGAL/NT_converter.h>
#include <CGAL/number_type_basic.h>
// cmake already checks for GMP presence when checking for CGAL
#include <CGAL/gmpxx.h>
// newer CGAL uses boost multiprecision
#include <boost/multiprecision/gmp.hpp>

// The traits are those listed in documentation:
//    https://doc.cgal.org/latest/Algebraic_foundations/index.html
//    https://doc.cgal.org/latest/Algebraic_foundations/group__PkgAlgebraicFoundationsRef.html
//
// The functions used by CGAL are on top of https://doc.cgal.org/latest/Algebraic_foundations/group__PkgAlgebraicFoundationsRef.html
// And are implemented below. If some more are necessary they can be added by redirecting to ::yade::math.
// But then a check for them must be also added to:
//    py/high-precision/_math.cpp
//    py/tests/testMath.py
// To make sure they work correctly for all precision types represented by Real.
//
// There is some extra check #ifdef CGAL_CFG_IEEE_754_BUG for long double case in /usr/include/CGAL/long_double.h.
// So #if defined(CGAL_CFG_IEEE_754_BUG) and (Real == long double) then it falls back to CGAL's long double specific functions.

namespace CGAL {

template <int levelHP> class RealHP_Is_valid : public CGAL::cpp98::unary_function<::yade::RealHP<levelHP>, bool> {
public:
	bool operator()(const ::yade::RealHP<levelHP>& x) const
	{
// When CGAL detects IEEE bug, then long double needs special treatment:
#ifdef CGAL_CFG_IEEE_754_BUG // and (YADE_REAL_BIT == 80)
		if (std::is_same<::yade::RealHP<levelHP>, long double>::value) {
			return Is_valid<long double>()(static_cast<long double>(x));
		}
#endif
		return not ::yade::math::isnan(x);
	}
};

template <int levelHP> class RealHP_Algebraic_structure_traits : public Algebraic_structure_traits_base<::yade::RealHP<levelHP>, Field_with_kth_root_tag> {
public:
	typedef Tag_false               Is_exact;
	typedef Tag_true                Is_numerical_sensitive;
	typedef ::yade::RealHP<levelHP> Type;

	/* if they become necessary add tests in py/tests/testMath.py, py/high-precision/_math.cpp
	class IsZero : public CGAL::cpp98::unary_function<Type, bool> {
	public:
		bool operator()(const Type& x) const { return x == 0; }
	};

	class IsOne : public CGAL::cpp98::unary_function<Type, bool> {
	public:
		bool operator()(const Type& x) const { return x == 1; }
	};
*/

	class Square : public CGAL::cpp98::unary_function<Type, Type> {
	public:
		Type operator()(const Type& x) const { return ::yade::math::pow(x, 2); }
	};

	class Sqrt : public CGAL::cpp98::unary_function<Type, Type> {
	public:
		Type operator()(const Type& x) const
		{
			return ::yade::math::sqrt(x);
			// note: that's what would be called for long double case:
			// return Algebraic_structure_traits<long double>::Sqrt()(static_cast<long double>(x));
		}
	};

	class Kth_root : public CGAL::cpp98::binary_function<int, Type, Type> {
	public:
		Type operator()(int k, const Type& x) const
		{
			CGAL_precondition_msg(k > 0, "'k' must be positive for k-th roots");
			return ::yade::math::pow(x, static_cast<::yade::RealHP<levelHP>>(1.0) / static_cast<::yade::RealHP<levelHP>>(k));
			// note: that's what would be called for long double case:
			// return Algebraic_structure_traits<long double>::Kth_root()(k, static_cast<long double>(x));
		};
	};
};


template <int levelHP> class RealHP_embeddable_traits : public INTERN_RET::Real_embeddable_traits_base<::yade::RealHP<levelHP>, CGAL::Tag_true> {
public:
	typedef ::yade::RealHP<levelHP> Type;
	class To_interval : public CGAL::cpp98::unary_function<Type, std::pair<double, double>> {
	public:
		std::pair<double, double> operator()(const Type& x) const { return (Interval_nt<>(static_cast<double>(x)) + Interval_nt<>::smallest()).pair(); }
	};

	/* if they become necessary add tests in py/tests/testMath.py, py/high-precision/_math.cpp
	class Abs : public CGAL::cpp98::unary_function<Type, Type> {
	public:
		Type operator()(const Type& x) const { return ::yade::math::abs(x); }
	};
*/

	class Sgn : public CGAL::cpp98::unary_function<Type, CGAL::Sign> {
	public:
		CGAL::Sign operator()(const Type& x) const
		{
			// see /usr/include/CGAL/enum.h
			return CGAL::Sign(::yade::math::sign(x));
		}
	};

	class Is_finite : public CGAL::cpp98::unary_function<Type, bool> {
	public:
		bool operator()(const Type& x) const
		{
#if defined(CGAL_CFG_IEEE_754_BUG) and (YADE_REAL_BIT == 80)
			// use CGAL workaround for long double if there is an IEEE compiler bug
			return Real_embeddable_traits<long double>::Is_finite()(static_cast<long double>(x));
#else
			return ::yade::math::isfinite(x);
#endif
		}
	};
};

// There are two ways to avoid this macro (hint: the best is to use C++20). See file lib/high-precision/RealHPEigenCgal.hpp for details.
#define YADE_CGAL_SUPPORT_REAL_HP(levelHP)                                                                                                                     \
	template <> struct Is_valid<::yade::RealHP<levelHP>> : public RealHP_Is_valid<levelHP> {                                                               \
	};                                                                                                                                                     \
	template <> struct Algebraic_structure_traits<::yade::RealHP<levelHP>> : public RealHP_Algebraic_structure_traits<levelHP> {                           \
	};                                                                                                                                                     \
	template <> struct Real_embeddable_traits<::yade::RealHP<levelHP>> : public RealHP_embeddable_traits<levelHP> {                                        \
	};

// When faster CGAL computations are needed, we might want to use and specialize converter for /usr/include/CGAL/Lazy_exact_nt.h

// TODO: - make sure that RealHP<…> works correctly with all possible GMP mpq_rational combinations.
//         Before yade switches to C++20 it will requiree the same explicit template specializations macros as for the others in file RealHPEigenCgal.hpp
// NOTE: it passes all the tests so far. So if a mistake is discovered here, then please make sure to add that case to the tests.
template <typename GMP1, typename GMP2>
struct NT_converter<::yade::Real, __gmp_expr<GMP1, GMP2>>
        : public CGAL::cpp98::unary_function<::yade::Real, NT_converter<::yade::Real, __gmp_expr<GMP1, GMP2>>> {
	__gmp_expr<GMP1, GMP2> operator()(const ::yade::Real& a) const
	{
		mpq_t ret;
		mpq_init(ret);
		mpq_set(ret, boost::multiprecision::mpq_rational(static_cast<::yade::math::UnderlyingReal>(a)).backend().data());
		return __gmp_expr<GMP1, GMP2>(ret);
	}
};

template <>
struct NT_converter<::yade::Real, boost::multiprecision::mpq_rational>
        : public CGAL::cpp98::unary_function<::yade::Real, NT_converter<::yade::Real, boost::multiprecision::mpq_rational>> {
	boost::multiprecision::mpq_rational operator()(const ::yade::Real& a) const
	{
		return boost::multiprecision::mpq_rational(static_cast<::yade::math::UnderlyingReal>(a));
	}
};

} // namespace CGAL

namespace std {

// BUG in CGAL: file /usr/include/CGAL/PCA_util_Eigen.h is calling std::sqrt !
// They should call functions without qualification to let ADL take care of that, like boost does this:
//  → https://www.boost.org/doc/libs/1_72_0/libs/math/doc/html/math_toolkit/float128_hints.html
//  → https://www.boost.org/doc/libs/1_60_0/libs/multiprecision/doc/html/boost_multiprecision/tut/floats/fp_eg/jel.html
//  → https://www.boost.org/doc/libs/1_72_0/libs/math/doc/html/math_toolkit/real_concepts.html
// search for string 'unqualified'. Perhaps we should file a bugreport to CGAL.

// OK, bug reported to  CGAL: https://github.com/CGAL/cgal/issues/4527
//                     Eigen: https://gitlab.com/libeigen/eigen/issues/1823
// we can disable this later by #ifdef-ing a CGAL version.
#if CGAL_VERSION_NR < CGAL_VERSION_NUMBER(5, 1, 1)
using ::yade::math::pow;
using ::yade::math::sqrt;
#endif
}

#endif // CGAL_NUM_TRAITS_HPP
