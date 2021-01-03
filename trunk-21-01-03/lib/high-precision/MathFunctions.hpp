/*************************************************************************
*  2019 Janek Kozicki                                                    *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

// NOTE: add more functions as necessary, but remember to add them in py/high-precision/_math.cpp, py/tests/testMath.py and py/tests/testMathHelper.py

// This file contains mathematical functions available in standard library and boost library.
//     https://en.cppreference.com/w/cpp/numeric/math
//     https://en.cppreference.com/w/cpp/numeric/special_functions
// They have to be provided here as inline redirections towards the correct implementation, depending on what precision type yade is being compiled with.
// This is the only way to make sure that ::std, ::boost::math, ::boost::multiprecision are all called correctly.

// TODO: Boost documentation recommends to link with tr1: -lboost_math_tr1 as it provides significant speedup. For example replace boost::math::acosh(x) ↔ boost::math::tr1::acosh(x)
//     https://www.boost.org/doc/libs/1_71_0/libs/math/doc/html/math_toolkit/overview_tr1.html
//#include <boost/math/tr1.hpp>

#ifndef YADE_THIN_REAL_WRAPPER_MATH_FUNCIONS_HPP
#define YADE_THIN_REAL_WRAPPER_MATH_FUNCIONS_HPP

#include <boost/config.hpp>
#include <boost/math/complex.hpp>
#include <boost/math/constants/constants.hpp>
#include <boost/math/special_functions.hpp>
#include <boost/math/tools/config.hpp>
#include <boost/random.hpp>
#include <cmath>
#include <complex>
#include <cstdlib>
#include <limits>
#include <utility>

#ifndef YADE_REAL_MATH_NAMESPACE
#error "This file cannot be included alone, include Real.hpp instead"
#endif

namespace forCtags {
struct MathFunctions {
}; // for ctags
}

// If cmake forgot to set YADE_IGNORE_IEEE_INFINITY_NAN along with -Ofast then set it here. It is to make sure that Infinity and NaN are supported only when available.
// It's is here for safety only, because if someone used -Ofast compilation flag (currently not available in CMakeLists.txt), then NaN Inf will not work unless
// these flags are used: -Ofast -fno-associative-math -fno-finite-math-only -fsigned-zeros (and maybe -march=native -mtune=native)
// The #if defined(__FAST_MATH__) detects if compiler -ffinite-math-only flag was passed, it is turned on by -Ofast.
// Comment this out, if some brave runtime speed -Ofast tests are to be performed.
#if ((not(defined(YADE_IGNORE_IEEE_INFINITY_NAN))) and (defined(__FAST_MATH__)))
#define YADE_IGNORE_IEEE_INFINITY_NAN
#endif

namespace yade {
namespace math {
	/********************************************************************************************/
	/**********************     Real or Complex trigonometric functions    **********************/
	/********************************************************************************************/
	// typename RC is a type which can be Real or Complex, Rr → only Real, Cc → only Complex.
	// The check involving int Level = levelOfHP<RC> is necessary to make sure that function is called only with yade supported HP types.
	// int Level is the N in RealHP<N>.
	template <typename RC, int Level = levelOfHP<RC>> inline RC sin(const RC& a)
	{
		using ::std::sin;
		using YADE_REAL_MATH_NAMESPACE::sin;
		return sin(static_cast<const UnderlyingHP<RC>&>(a));
	}
	template <typename RC, int Level = levelOfHP<RC>> inline RC sinh(const RC& a)
	{
		using ::std::sinh;
		using YADE_REAL_MATH_NAMESPACE::sinh;
		return sinh(static_cast<const UnderlyingHP<RC>&>(a));
	}
	template <typename RC, int Level = levelOfHP<RC>> inline RC cos(const RC& a)
	{
		using ::std::cos;
		using YADE_REAL_MATH_NAMESPACE::cos;
		return cos(static_cast<const UnderlyingHP<RC>&>(a));
	}
	template <typename RC, int Level = levelOfHP<RC>> inline RC cosh(const RC& a)
	{
		using ::std::cosh;
		using YADE_REAL_MATH_NAMESPACE::cosh;
		return cosh(static_cast<const UnderlyingHP<RC>&>(a));
	}
	template <typename RC, int Level = levelOfHP<RC>> inline RC tan(const RC& a)
	{
		using ::std::tan;
		using YADE_REAL_MATH_NAMESPACE::tan;
		return tan(static_cast<const UnderlyingHP<RC>&>(a));
	}
	template <typename RC, int Level = levelOfHP<RC>> inline RC tanh(const RC& a)
	{
		using ::std::tanh;
		using YADE_REAL_MATH_NAMESPACE::tanh;
		return tanh(static_cast<const UnderlyingHP<RC>&>(a));
	}
	// add more Real or Complex functions as necessary, but remember to add them in py/high-precision/_math.cpp, py/tests/testMath.py and py/tests/testMathHelper.py

	/********************************************************************************************/
	/**********************      Real inverse trigonometric functions      **********************/
	/********************************************************************************************/
	// The check involving int Level = levelOfRealHP<Rr> is necessary to make sure that function is called only with yade Real supported HP types.
	template <typename Rr, int Level = levelOfRealHP<Rr>> inline Rr asin(const Rr& a)
	{
		using ::std::asin;
		using YADE_REAL_MATH_NAMESPACE::asin;
		return asin(static_cast<const UnderlyingHP<Rr>&>(a));
	}
	template <typename Rr, int Level = levelOfRealHP<Rr>> inline Rr asinh(const Rr& a)
	{
		using ::std::asinh;
		using YADE_REAL_MATH_NAMESPACE::asinh;
		return asinh(static_cast<const UnderlyingHP<Rr>&>(a));
	}
	template <typename Rr, int Level = levelOfRealHP<Rr>> inline Rr acos(const Rr& a)
	{
		using ::std::acos;
		using YADE_REAL_MATH_NAMESPACE::acos;
		return acos(static_cast<const UnderlyingHP<Rr>&>(a));
	}
	template <typename Rr, int Level = levelOfRealHP<Rr>> inline Rr acosh(const Rr& a)
	{
		using ::std::acosh;
		using YADE_REAL_MATH_NAMESPACE::acosh;
		return acosh(static_cast<const UnderlyingHP<Rr>&>(a));
	}
	template <typename Rr, int Level = levelOfRealHP<Rr>> inline Rr atan(const Rr& a)
	{
		using ::std::atan;
		using YADE_REAL_MATH_NAMESPACE::atan;
		return atan(static_cast<const UnderlyingHP<Rr>&>(a));
	}
	template <typename Rr, int Level = levelOfRealHP<Rr>> inline Rr atanh(const Rr& a)
	{
		using ::std::atanh;
		using YADE_REAL_MATH_NAMESPACE::atanh;
		return atanh(static_cast<const UnderlyingHP<Rr>&>(a));
	}
	template <typename Rr, int Level = levelOfRealHP<Rr>> inline Rr atan2(const Rr& a, const Rr& b)
	{
		using ::std::atan2;
		using YADE_REAL_MATH_NAMESPACE::atan2;
		return atan2(static_cast<const UnderlyingHP<Rr>&>(a), static_cast<const UnderlyingHP<Rr>&>(b));
	}

	/********************************************************************************************/
	/**********************   logarithm, exponential and power functions   **********************/
	/********************************************************************************************/
	// Add more functions as necessary, but remember to add them in py/high-precision/_math.cpp, py/tests/testMath.py and py/tests/testMathHelper.py
	// They can be converted to accept complex by changing levelOfRealHP<> → levelOfHP<>, provided that a complex version exists.
	// But remember to add tests for complex versions in py/high-precision/_math.cpp, py/tests/testMath.py and py/tests/testMathHelper.py
	template <typename RC, int Level = levelOfHPAllow<RC, int>> inline PromoteHP<RC> log(const RC& a)
	{
		using ::std::log;
		using YADE_REAL_MATH_NAMESPACE::log;
		return log(static_cast<const UnderlyingHP<RC>&>(a));
	}
	template <typename Rr, int Level = levelOfRealHP<Rr>> inline Rr log10(const Rr& a)
	{
		using ::std::log10;
		using YADE_REAL_MATH_NAMESPACE::log10;
		return log10(static_cast<const UnderlyingHP<Rr>&>(a));
	}
	template <typename Rr, int Level = levelOfRealHP<Rr>> inline Rr log1p(const Rr& a)
	{
		using ::std::log1p;
		using YADE_REAL_MATH_NAMESPACE::log1p;
		return log1p(static_cast<const UnderlyingHP<Rr>&>(a));
	}
	template <typename Rr, int Level = levelOfRealHP<Rr>> inline Rr log2(const Rr& a)
	{
		using ::std::log2;
		using YADE_REAL_MATH_NAMESPACE::log2;
		return log2(static_cast<const UnderlyingHP<Rr>&>(a));
	}
	template <typename Rr, int Level = levelOfRealHP<Rr>> inline Rr logb(const Rr& a)
	{
		using ::std::logb;
		using YADE_REAL_MATH_NAMESPACE::logb;
		return logb(static_cast<const UnderlyingHP<Rr>&>(a));
	}
	template <typename Rr, int Level = levelOfRealHP<Rr>> inline Rr ilogb(const Rr& a)
	{
		using ::std::ilogb;
		using YADE_REAL_MATH_NAMESPACE::ilogb;
		return ilogb(static_cast<const UnderlyingHP<Rr>&>(a));
	}
	template <typename Rr, int Level = levelOfRealHP<Rr>> inline Rr ldexp(const Rr& a, int b)
	{
		using ::std::ldexp;
		using YADE_REAL_MATH_NAMESPACE::ldexp;
		return ldexp(static_cast<const UnderlyingHP<Rr>&>(a), b);
	}
	// that's original C signature of this function
	template <typename Rr, int Level = levelOfRealHP<Rr>> inline Rr frexp(const Rr& a, int* b)
	{
		using ::std::frexp;
		using YADE_REAL_MATH_NAMESPACE::frexp;
		return frexp(static_cast<const UnderlyingHP<Rr>&>(a), b);
	}
	template <typename RC, int Level = levelOfHP<RC>> inline RC exp(const RC& a)
	{
		using ::std::exp;
		using YADE_REAL_MATH_NAMESPACE::exp;
		return exp(static_cast<const UnderlyingHP<RC>&>(a));
	}
	template <typename Rr, int Level = levelOfRealHP<Rr>> inline Rr exp2(const Rr& a)
	{
		using ::std::exp2;
		using YADE_REAL_MATH_NAMESPACE::exp2;
		return exp2(static_cast<const UnderlyingHP<Rr>&>(a));
	}
	template <typename Rr, int Level = levelOfRealHP<Rr>> inline Rr expm1(const Rr& a)
	{
		using ::std::expm1;
		using YADE_REAL_MATH_NAMESPACE::expm1;
		return expm1(static_cast<const UnderlyingHP<Rr>&>(a));
	}
	template <typename A, typename B, int Level = levelOfRealHPAllow<A, int>, typename Rr = PromoteHP<A>>
	inline typename boost::enable_if<std::is_convertible<typename std::decay<B>::type, Rr>, Rr>::type pow(const A& a, const B& b)
	{
		using ::std::pow;
		using YADE_REAL_MATH_NAMESPACE::pow;
		return pow(static_cast<const UnderlyingHP<Rr>&>(a), static_cast<const UnderlyingHP<Rr>&>(b));
	}
	template <typename Rr, int Level = levelOfRealHP<Rr>> inline Rr sqrt(const Rr& a)
	{
		using ::std::sqrt;
		using YADE_REAL_MATH_NAMESPACE::sqrt;
		return sqrt(static_cast<const UnderlyingHP<Rr>&>(a));
	}
	template <typename Rr, int Level = levelOfRealHP<Rr>> inline Rr cbrt(const Rr& a)
	{
		using ::std::cbrt;
		using YADE_REAL_MATH_NAMESPACE::cbrt;
		return cbrt(static_cast<const UnderlyingHP<Rr>&>(a));
	}
	template <typename Rr, int Level = levelOfRealHP<Rr>> inline Rr hypot(const Rr& a, const Rr& b)
	{
		using ::std::hypot;
		using YADE_REAL_MATH_NAMESPACE::hypot;
		return hypot(static_cast<const UnderlyingHP<Rr>&>(a), static_cast<const UnderlyingHP<Rr>&>(b));
	}
	//YADE_WRAP_FUNC_3(hypot) // since C++17, could be very useful for us

	/********************************************************************************************/
	/**********************    min, max, abs, sign, floor, ceil, etc...    **********************/
	/********************************************************************************************/

	// Both must be found by automatic lookup: the ones from ::std and the ones that accept non-double Real types.
	using ::std::abs;
	using ::std::fabs;
	using ::std::max; // this is inside ::yade::math namespace. It is not found by ADL in ::yade namespace when applied to int type or other non-Real type.
	using ::std::min;
	// It turns out that getting min, max to work properly is more tricky than it is for other math functions: https://svn.boost.org/trac10/ticket/11149
	using YADE_REAL_MATH_NAMESPACE::max; // this refers to boost::multiprecision (or eventually to ::mpfr)
	using YADE_REAL_MATH_NAMESPACE::min;
	// make sure that min max can accept (double,Real) argument pairs such as: max(r,0.5);
	template <typename Rr, int Level = levelOfRealHPExcept<Rr, double>> inline Rr max(const double& a, const Rr& b)
	{
		return max(static_cast<const UnderlyingHP<Rr>&>(a), static_cast<const UnderlyingHP<Rr>&>(b));
	}
	template <typename Rr, int Level = levelOfRealHPExcept<Rr, double>> inline Rr min(const double& a, const Rr& b)
	{
		return min(static_cast<const UnderlyingHP<Rr>&>(a), static_cast<const UnderlyingHP<Rr>&>(b));
	}
	template <typename Rr, int Level = levelOfRealHPExcept<Rr, double>> inline Rr max(const Rr& a, const double& b)
	{
		return max(static_cast<const UnderlyingHP<Rr>&>(a), static_cast<const UnderlyingHP<Rr>&>(b));
	}
	template <typename Rr, int Level = levelOfRealHPExcept<Rr, double>> inline Rr min(const Rr& a, const double& b)
	{
		return min(static_cast<const UnderlyingHP<Rr>&>(a), static_cast<const UnderlyingHP<Rr>&>(b));
	}
	// allow here Complex also
	template <typename RC, int Level = levelOfHP<RC>> inline RealHP<Level> abs(const RC& a)
	{
		using ::std::abs;
		using YADE_REAL_MATH_NAMESPACE::abs;
		return abs(static_cast<const UnderlyingHP<RC>&>(a));
	}
	template <typename Rr, int Level = levelOfRealHP<Rr>> inline Rr fabs(const Rr& a) { return ::yade::math::abs(a); }

	template <typename T> int sgn(T val) { return (T(0) < val) - (val < T(0)); }
	template <typename T> int sign(T val) { return (T(0) < val) - (val < T(0)); }

	template <typename Rr, int Level = levelOfRealHP<Rr>> inline Rr floor(const Rr& a)
	{
		using ::std::floor;
		using YADE_REAL_MATH_NAMESPACE::floor;
		return floor(static_cast<const UnderlyingHP<Rr>&>(a));
	}
	template <typename Rr, int Level = levelOfRealHP<Rr>> inline Rr ceil(const Rr& a)
	{
		using ::std::ceil;
		using YADE_REAL_MATH_NAMESPACE::ceil;
		return ceil(static_cast<const UnderlyingHP<Rr>&>(a));
	}
	template <typename Rr, int Level = levelOfRealHPAllow<Rr, double>> inline Rr round(const Rr& a)
	{
		using ::std::round;
		using YADE_REAL_MATH_NAMESPACE::round;
		return round(static_cast<const UnderlyingHP<Rr>&>(a));
	}
	template <typename Rr, int Level = levelOfRealHP<Rr>> inline Rr rint(const Rr& a)
	{
		using ::std::rint;
		using YADE_REAL_MATH_NAMESPACE::rint;
		return rint(static_cast<const UnderlyingHP<Rr>&>(a));
	}
	template <typename Rr, int Level = levelOfRealHP<Rr>> inline Rr trunc(const Rr& a)
	{
		using ::std::trunc;
		using YADE_REAL_MATH_NAMESPACE::trunc;
		return trunc(static_cast<const UnderlyingHP<Rr>&>(a));
	}

#ifndef YADE_IGNORE_IEEE_INFINITY_NAN
	template <typename Rr, int Level = levelOfRealHP<Rr>> inline bool isnan(const Rr& a)
	{
		using ::std::isnan;
		using YADE_REAL_MATH_NAMESPACE::isnan;
		return isnan(static_cast<const UnderlyingHP<Rr>&>(a));
	}
	template <typename Rr, int Level = levelOfRealHP<Rr>> inline bool isinf(const Rr& a)
	{
		using ::std::isinf;
		using YADE_REAL_MATH_NAMESPACE::isinf;
		return isinf(static_cast<const UnderlyingHP<Rr>&>(a));
	}
	template <typename Rr, int Level = levelOfRealHP<Rr>> inline bool isfinite(const Rr& a)
	{
		using ::std::isfinite;
		using YADE_REAL_MATH_NAMESPACE::isfinite;
		return isfinite(static_cast<const UnderlyingHP<Rr>&>(a));
	}
	template <typename Rr, int Level = levelOfRealHP<Rr>> inline int fpclassify(const Rr& a)
	{
		using ::std::fpclassify;
		using YADE_REAL_MATH_NAMESPACE::fpclassify;
		return fpclassify(static_cast<const UnderlyingHP<Rr>&>(a));
	}
#endif

	/********************************************************************************************/
	/**********************            Complex conj, real, imag             *********************/
	/********************************************************************************************/
	// Add more complex functions as necessary, but remember to add them in py/high-precision/_math.cpp and py/tests/testMath.py
	// Note: most of the functions above can be converted to accept complex by changing levelOfRealHP<> → levelOfHP<>, provided that a complex version exists.
	// The check involving int Level = levelOfComplexHP<Cc> is necessary to make sure that function is called only with yade Complex supported HP types.
	template <typename Cc, int Level = levelOfComplexHP<Cc>> inline Cc conj(const Cc& a) { return ::std::conj(static_cast<const UnderlyingHP<Cc>&>(a)); }

	template <typename Cc, int Level = levelOfComplexHP<Cc>> inline RealHP<Level> real(const Cc& a)
	{
		return ::std::real(static_cast<const UnderlyingHP<Cc>&>(a));
	}
	template <typename Cc, int Level = levelOfComplexHP<Cc>> inline RealHP<Level> imag(const Cc& a)
	{
		return ::std::imag(static_cast<const UnderlyingHP<Cc>&>(a));
	}

	/********************************************************************************************/
	/**********************        integer division and remainder          **********************/
	/********************************************************************************************/
	template <typename Rr, int Level = levelOfRealHP<Rr>> inline Rr fmod(const Rr& a, const Rr& b)
	{
		using ::std::fmod;
		using YADE_REAL_MATH_NAMESPACE::fmod;
		return fmod(static_cast<const UnderlyingHP<Rr>&>(a), static_cast<const UnderlyingHP<Rr>&>(b));
	}
	template <typename Rr, int Level = levelOfRealHP<Rr>> inline Rr remainder(const Rr& a, const Rr& b)
	{
		using ::std::remainder;
		using YADE_REAL_MATH_NAMESPACE::remainder;
		return remainder(static_cast<const UnderlyingHP<Rr>&>(a), static_cast<const UnderlyingHP<Rr>&>(b));
	}
	template <typename Rr, int Level = levelOfRealHP<Rr>, typename boost::enable_if_c<IsWrapped<Rr>, int>::type = 0> inline Rr modf(const Rr& a, Rr* b)
	{
		using ::std::modf;
		using YADE_REAL_MATH_NAMESPACE::modf;
		return modf(static_cast<const UnderlyingRealHP<Rr>&>(a), b->operator UnderlyingRealHP<Rr>*());
	}
	template <typename Rr, int Level = levelOfRealHP<Rr>, typename boost::disable_if_c<IsWrapped<Rr>, int>::type = 0> inline Rr modf(const Rr& a, Rr* b)
	{
		using ::std::modf;
		using YADE_REAL_MATH_NAMESPACE::modf;
		return modf(static_cast<const UnderlyingRealHP<Rr>&>(a), b);
	}
	template <typename Rr, int Level = levelOfRealHP<Rr>> inline Rr fma(const Rr& a, const Rr& b, const Rr& c)
	{
		using ::std::fma;
		using YADE_REAL_MATH_NAMESPACE::fma;
		return fma(static_cast<const UnderlyingHP<Rr>&>(a), static_cast<const UnderlyingHP<Rr>&>(b), static_cast<const UnderlyingHP<Rr>&>(c));
	}
	template <typename Rr, int Level = levelOfRealHP<Rr>> inline Rr remquo(const Rr& a, const Rr& b, int* c)
	{
		using ::std::remquo;
		using YADE_REAL_MATH_NAMESPACE::remquo;
		return remquo(static_cast<const UnderlyingHP<Rr>&>(a), static_cast<const UnderlyingHP<Rr>&>(b), c);
	}

	/********************************************************************************************/
	/**********************         special mathematical functions         **********************/
	/********************************************************************************************/
	template <typename Rr, int Level = levelOfRealHP<Rr>> inline Rr erf(const Rr& a)
	{
		using ::std::erf;
		using YADE_REAL_MATH_NAMESPACE::erf;
		return erf(static_cast<const UnderlyingHP<Rr>&>(a));
	}
	template <typename Rr, int Level = levelOfRealHP<Rr>> inline Rr erfc(const Rr& a)
	{
		using ::std::erfc;
		using YADE_REAL_MATH_NAMESPACE::erfc;
		return erfc(static_cast<const UnderlyingHP<Rr>&>(a));
	}
	template <typename Rr, int Level = levelOfRealHP<Rr>> inline Rr lgamma(const Rr& a)
	{
		using ::std::lgamma;
		using YADE_REAL_MATH_NAMESPACE::lgamma;
		return lgamma(static_cast<const UnderlyingHP<Rr>&>(a));
	}

	// These will be available in C++17, we could use the ones from boost, if they become necessary.
	//YADE_WRAP_FUNC_1(riemann_zeta)
	//YADE_WRAP_FUNC_2(beta)
	//YADE_WRAP_FUNC_2(cyl_bessel_i)
	//YADE_WRAP_FUNC_2(cyl_bessel_j)
	//YADE_WRAP_FUNC_2(cyl_bessel_k)
	//YADE_WRAP_FUNC_2(sph_bessel, unsigned)


	// workaround broken tgamma for boost::float128, see https://github.com/boostorg/math/issues/307
	template <typename Rr, int Level = levelOfRealHP<Rr>, typename boost::enable_if_c<isFloat128<Rr>, int>::type = 0> inline Rr tgamma(const Rr& a)
	{
		using ::std::tgamma;
		using YADE_REAL_MATH_NAMESPACE::tgamma;
		if (a >= 0) {
			return tgamma(static_cast<UnderlyingRealHP<Rr>>(a));
		} else {
			return abs(tgamma(static_cast<UnderlyingRealHP<Rr>>(a))) * ((static_cast<unsigned long long>(floor(abs(a))) % 2 == 0) ? -1 : 1);
		}
	}
	template <typename Rr, int Level = levelOfRealHP<Rr>, typename boost::disable_if_c<isFloat128<Rr>, int>::type = 0> inline Rr tgamma(const Rr& a)
	{
		using ::std::tgamma;
		using YADE_REAL_MATH_NAMESPACE::tgamma;
		return tgamma(static_cast<const UnderlyingRealHP<Rr>&>(a));
	}

	/********************************************************************************************/
	/**********************        extract C-array from std::vector        **********************/
	/********************************************************************************************/

	// Some old C library functions need pointer to C-array, this is for compatibility between ThinRealWrapper and UnderlyingReal
	static_assert(sizeof(Real) == sizeof(UnderlyingReal), "This compiler introduced padding. This breaks binary compatibility");
	static_assert(sizeof(Complex) == sizeof(std::complex<UnderlyingReal>), "This compiler introduced padding, which breaks binary compatibility");

	template <typename Rr, int Level = levelOfRealHP<Rr>, typename boost::enable_if_c<IsWrapped<Rr>, int>::type = 0>
	static inline const UnderlyingRealHP<Rr>* constVectorData(const std::vector<Rr>& v)
	{
		return v.data()->operator const UnderlyingRealHP<Rr>*();
	}
	template <typename Rr, int Level = levelOfRealHP<Rr>, typename boost::enable_if_c<IsWrapped<Rr>, int>::type = 0>
	static inline UnderlyingRealHP<Rr>* vectorData(std::vector<Rr>& v)
	{
		return v.data()->operator UnderlyingRealHP<Rr>*();
	}
	template <typename Rr, int Level = levelOfRealHP<Rr>, typename boost::disable_if_c<IsWrapped<Rr>, int>::type = 0>
	static inline const UnderlyingRealHP<Rr>* constVectorData(const std::vector<Rr>& v)
	{
		return v.data();
	}
	template <typename Rr, int Level = levelOfRealHP<Rr>, typename boost::disable_if_c<IsWrapped<Rr>, int>::type = 0>
	static inline UnderlyingRealHP<Rr>* vectorData(std::vector<Rr>& v)
	{
		return v.data();
	}

	/********************************************************************************************/
	/**********************                     random                     **********************/
	/********************************************************************************************/

	// These random functions are necessary for Eigen library to for example write in python: Vector3.Random()
	// generate random number [0,1)
	template <int N> static inline RealHP<N> random01HP()
	{
		static ::boost::random::mt19937 gen;
		return ::boost::random::generate_canonical<::yade::math::RealHP<N>, std::numeric_limits<::yade::math::RealHP<N>>::digits>(gen);
	}

	// Convenience functions for Real == RealHP<1>
	static inline Real random01() { return random01HP<1>(); }
	static inline Real unitRandom() { return random01(); }
	static inline Real random() { return random01() * 2 - 1; }
	static inline Real symmetricRandom() { return random(); }

	// EigenNumTraits support for RealHP<N> requires random to work with RealHP<N>
	template <int N> static inline RealHP<N> unitRandomHP() { return random01HP<N>(); }
	template <int N> static inline RealHP<N> randomHP() { return random01HP<N>() * 2 - 1; }
	template <int N> static inline RealHP<N> symmetricRandomHP() { return randomHP<N>(); }

} // namespace math
} // namespace yade

#endif
