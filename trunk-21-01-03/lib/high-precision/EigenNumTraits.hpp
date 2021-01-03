/*************************************************************************
*  2019 Janek Kozicki                                                    *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#ifndef YADE_REAL_MATH_NAMESPACE
#error "This file cannot be included alone, include Real.hpp instead"
#endif

#ifndef YADE_EIGEN_NUM_TRAITS_HPP
#define YADE_EIGEN_NUM_TRAITS_HPP

// compare this file with /usr/include/eigen3/Eigen/src/Core/NumTraits.h
// http://eigen.tuxfamily.org/dox/TopicCustomizing_CustomScalar.html
// http://eigen.tuxfamily.org/dox-3.2/TopicCustomizingEigen.html

#include <Eigen/Core>
#include <boost/math/constants/constants.hpp>

/*************************************************************************/
/*************************        Real          **************************/
/*************************************************************************/

namespace Eigen {

template <typename> struct EigenCostRealHP {
	enum { ReadCost = Eigen::HugeCost, AddCost = Eigen::HugeCost, MulCost = Eigen::HugeCost };
};

template <> struct EigenCostRealHP<boost::float_fast80_t> {
	enum { ReadCost = 1, AddCost = 1, MulCost = 1 };
};
#ifdef BOOST_MP_FLOAT128_HPP
template <> struct EigenCostRealHP<boost::multiprecision::float128> {
	enum { ReadCost = 1, AddCost = 2, MulCost = 2 };
};
#endif

// signature of general template from Eigen headers.
template <class> struct NumTraits;

template <int levelHP> struct NumTraitsRealHP : GenericNumTraits<::yade::RealHP<levelHP>> {
	typedef ::yade::RealHP<levelHP>              Real;
	typedef ::yade::math::UnderlyingRealHP<Real> Underlying;
	typedef ::yade::RealHP<levelHP>              NonInteger;
	typedef ::yade::RealHP<levelHP>              Nested;

	enum {
		IsInteger             = 0,
		IsSigned              = 1,
		IsComplex             = 0,
		RequireInitialization = 1,
		ReadCost              = EigenCostRealHP<Underlying>::ReadCost,
		AddCost               = EigenCostRealHP<Underlying>::AddCost,
		MulCost               = EigenCostRealHP<Underlying>::MulCost
	};

	static constexpr long get_default_prec = std::numeric_limits<Real>::digits;

	// http://cs.swan.ac.uk/~csoliver/ok-sat-library/internet_html/doc/doc/Mpfr/3.0.0/mpfr.html/Exception-Related-Functions.html
	// https://tspiteri.gitlab.io/gmp-mpfr-sys/mpfr/MPFR-Interface.html
	static inline Real highest(long = get_default_prec) { return std::numeric_limits<Real>::max(); }
	static inline Real lowest(long = get_default_prec) { return std::numeric_limits<Real>::lowest(); }

	// Constants
	static inline Real Pi(long = get_default_prec) { return boost::math::constants::pi<Real>(); }
	static inline Real Euler(long = get_default_prec) { return boost::math::constants::euler<Real>(); }
	static inline Real Log2(long = get_default_prec) { return ::yade::math::log(Real(2)); }
	static inline Real Catalan(long = get_default_prec) { return boost::math::constants::catalan<Real>(); }

	static inline Real epsilon(long = get_default_prec) { return std::numeric_limits<Real>::epsilon(); }
	static inline Real epsilon(const Real&) { return std::numeric_limits<Real>::epsilon(); }
	static inline Real smallest_positive() { return std::numeric_limits<Real>::min(); }
	static inline int  digits10(long = get_default_prec) { return std::numeric_limits<Real>::digits10; }
	static inline int  digits10(const Real&) { return std::numeric_limits<Real>::digits10; }
	static inline Real dummy_precision() { return epsilon() * ::yade::math::pow(Real(10), digits10() / Real(10)); }
};

namespace internal {
	// TODO: - make sure that all these functions and these below in macro YADE_EIGEN_SUPPORT_REAL_HP, are properly recognized and used.
	//         I am not 100% sure that <int levelHP> is properly resolved.
	// other ideas:
	//         template <typename Rr> inline typename boost::enable_if_c<::yade::math::isHP<Rr>, bool>::type
	//         template <typename Rr> inline typename boost::enable_if_c<::yade::math::isHP<Rr>, Rr>::type random<Rr>() { return ::yade::math::randomHP<::yade::math::levelOfRealHP<Rr>>(); }
	//         template <int levelHP>       inline bool isEqualFuzzy(const ::yade::math::RealHP<levelHP>& a, const ::yade::math::RealHP<levelHP>& b, const ::yade::math::RealHP<levelHP>& eps)
	// TODO: - or just put them all into YADE_EIGEN_SUPPORT_REAL_HP, but have all working tests first.
	// NOTE: it passes all the tests so far. So if a mistake is discovered here, then please make sure to add that case to the tests.
	template <int levelHP> inline ::yade::math::RealHP<levelHP> random() { return ::yade::math::randomHP<levelHP>(); }
	template <int levelHP> inline ::yade::math::RealHP<levelHP> random(const ::yade::math::RealHP<levelHP>& a, const ::yade::math::RealHP<levelHP>& b)
	{
		return a + (b - a) * ::yade::math::random01HP<levelHP>();
	}
	template <int levelHP>
	inline bool isMuchSmallerThan(const ::yade::math::RealHP<levelHP>& a, const ::yade::math::RealHP<levelHP>& b, const ::yade::math::RealHP<levelHP>& eps)
	{
		return ::yade::math::abs(a) <= ::yade::math::abs(b) * eps;
	}
	template <typename Rr> inline typename boost::enable_if_c<::yade::math::isHP<Rr>, bool>::type isEqualFuzzy(const Rr& a, const Rr& b, const Rr& eps)
	{
		return ::yade::math::abs(a - b) <= eps;
	}
	template <int levelHP>
	inline bool isApprox(const ::yade::math::RealHP<levelHP>& a, const ::yade::math::RealHP<levelHP>& b, const ::yade::math::RealHP<levelHP>& eps)
	{
		return isEqualFuzzy(a, b, eps);
	}
	template <int levelHP>
	inline bool isApproxOrLessThan(const ::yade::math::RealHP<levelHP>& a, const ::yade::math::RealHP<levelHP>& b, const ::yade::math::RealHP<levelHP>& eps)
	{
		return a <= b || isEqualFuzzy(a, b, eps);
	}
} // end namespace internal

/*************************************************************************/
/*************************       Complex        **************************/
/*************************************************************************/

template <int levelHP> struct NumTraitsComplexHP : GenericNumTraits<::yade::ComplexHP<levelHP>> {
	typedef typename ::yade::ComplexHP<levelHP>::value_type Real;
	typedef ::yade::math::UnderlyingRealHP<Real>            Underlying;
	typedef ::yade::ComplexHP<levelHP>                      Complex;
	typedef ::yade::ComplexHP<levelHP>                      NonInteger;
	typedef ::yade::ComplexHP<levelHP>                      Nested;

	enum {
		IsInteger             = 0,
		IsSigned              = 1,
		IsComplex             = 1,
		RequireInitialization = 1,
		ReadCost              = 2 * EigenCostRealHP<Underlying>::ReadCost,
		AddCost               = 2 * EigenCostRealHP<Underlying>::AddCost,
		MulCost               = 4 * EigenCostRealHP<Underlying>::MulCost + 2 * EigenCostRealHP<Underlying>::AddCost
	};

	static constexpr long get_default_prec = std::numeric_limits<Real>::digits;

	static inline Complex highest(long = get_default_prec) { return std::numeric_limits<Real>::max(); }
	static inline Complex lowest(long = get_default_prec) { return std::numeric_limits<Real>::lowest(); }

	// Constants
	static inline Complex Pi(long = get_default_prec) { return boost::math::constants::pi<Real>(); }
	static inline Complex Euler(long = get_default_prec) { return boost::math::constants::euler<Real>(); }
	static inline Complex Log2(long = get_default_prec) { return ::yade::math::log(Real(2)); }
	static inline Complex Catalan(long = get_default_prec) { return boost::math::constants::catalan<Real>(); }

	static inline Complex epsilon(long = get_default_prec) { return std::numeric_limits<Real>::epsilon(); }
	static inline Complex epsilon(const Complex&) { return std::numeric_limits<Real>::epsilon(); }
	static inline Complex smallest_positive() { return std::numeric_limits<Real>::min(); }
	static inline int     digits10(long = get_default_prec) { return std::numeric_limits<Real>::digits10; }
	static inline int     digits10(const Complex&) { return std::numeric_limits<Real>::digits10; }
	static inline Complex dummy_precision() { return epsilon() * ::yade::math::pow(Real(10), digits10() / Real(10)); }
};

/*
namespace internal {
	template <> inline ::yade::math::Complex random<::yade::math::Complex>() { return ::yade::random(); }
	template <> inline ::yade::math::Complex random<::yade::math::Complex>(const ::yade::math::Complex& a, const ::yade::math::Complex& b)
	{
		return a + (b - a) * ::yade::random01();
	}
	inline bool isMuchSmallerThan(const ::yade::math::Complex& a, const ::yade::math::Complex& b, const ::yade::math::Complex& eps)
	{
		return ::yade::abs(a) <= ::yade::abs(b) * eps;
	}
	inline bool isEqualFuzzy(const ::yade::math::Complex& a, const ::yade::math::Complex& b, const ::yade::math::Complex& eps) { return ::yade::abs(a - b) <= eps; }
	inline bool isApprox(const ::yade::math::Complex& a, const ::yade::math::Complex& b, const ::yade::math::Complex& eps) { return isEqualFuzzy(a, b, eps); }
	inline bool isApproxOrLessThan(const ::yade::math::Complex& a, const ::yade::math::Complex& b, const ::yade::math::Complex& eps)
	{
		return a <= b || isEqualFuzzy(a, b, eps);
	}
	template <> inline long double cast<::yade::math::Complex, long double>(const ::yade::math::Complex& x) { return (long double)(x); }
	template <> inline double      cast<::yade::math::Complex, double>(const ::yade::math::Complex& x) { return double(x); }
	template <> inline long        cast<::yade::math::Complex, long>(const ::yade::math::Complex& x) { return long(x); }
	template <> inline int         cast<::yade::math::Complex, int>(const ::yade::math::Complex& x) { return int(x); }
} // end namespace internal
*/

/*
//namespace yade::Math { // maybe add this later

inline const ::yade::math::Real& conj(const ::yade::math::Real& x) { return x; }
inline const ::yade::math::Real& real(const ::yade::math::Real& x) { return x; }
inline ::yade::math::Real        imag(const ::yade::math::Real&) { return 0.; }
inline ::yade::math::Real        abs(const ::yade::math::Real& x)
{
	return ::yade::abs(x);
}
inline ::yade::math::Real abs2(const ::yade::math::Real& x) { return x * x; }

//}
*/

// There are two ways to avoid this macro (hint: the best is to use C++20). See file lib/high-precision/RealHPEigenCgal.hpp for details.
#define YADE_EIGEN_SUPPORT_REAL_HP(levelHP)                                                                                                                    \
	template <> struct NumTraits<::yade::RealHP<levelHP>> : public NumTraitsRealHP<levelHP> {                                                              \
	};                                                                                                                                                     \
	template <> struct NumTraits<::yade::ComplexHP<levelHP>> : public NumTraitsComplexHP<levelHP> {                                                        \
	};                                                                                                                                                     \
	namespace internal {                                                                                                                                   \
		template <> inline long double cast<typename ::yade::math::RealHP<levelHP>, long double>(const ::yade::math::RealHP<levelHP>& x)               \
		{                                                                                                                                              \
			return (long double)(x);                                                                                                               \
		}                                                                                                                                              \
		template <> inline double cast<typename ::yade::math::RealHP<levelHP>, double>(const ::yade::math::RealHP<levelHP>& x) { return double(x); }   \
		template <> inline long   cast<typename ::yade::math::RealHP<levelHP>, long>(const ::yade::math::RealHP<levelHP>& x) { return long(x); }       \
		template <> inline int    cast<typename ::yade::math::RealHP<levelHP>, int>(const ::yade::math::RealHP<levelHP>& x) { return int(x); }         \
	}


} // namespace Eigen

#endif
