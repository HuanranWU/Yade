/*************************************************************************
*  2019 Janek Kozicki                                                    *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

// This python module exposes all C++ math functions for Real and Complex type to python.
// In fact it "just duplicates" 'import math', 'import cmath' or 'import mpmath'.
// This module has following purposes:
// 1. to reliably test all C++ math functions of arbitrary Real and Complex types against mpmath.
// 2. to test Eigen NumTraits
// 3. to test CGAL NumTraits
// 4. To allow writing python math code in  a way that mirrors C++ math code in yade. As a bonus it will be faster than mpmath
//    because mpmath is a purely python library (which was one of the main difficulties when writing lib/high-precision/ToFromPythonConverter.hpp)

#include <lib/base/Logging.hpp>
#include <lib/high-precision/Constants.hpp>
#include <lib/high-precision/Real.hpp>
#include <lib/high-precision/RealHPConfig.hpp>
#include <lib/high-precision/RealIO.hpp>
#include <lib/pyutil/doc_opts.hpp>
#ifdef YADE_CGAL
#include <lib/base/AliasCGAL.hpp>
#endif

#include <Eigen/Core>
#include <Eigen/src/Core/MathFunctions.h>
#include <boost/python.hpp>
#include <iostream>
#include <limits>
#include <sstream>

#include <py/high-precision/_ExposeStorageOrdering.hpp>
#include <py/high-precision/_RealHPDiagnostics.hpp>

// testing Real type
#define BOOST_ENABLE_ASSERT_HANDLER
#include <boost/assert.hpp> // Simple boost assert a little better than standard assert: https://www.boost.org/doc/libs/master/libs/assert/doc/html/assert.html
#include <boost/concept/assert.hpp>
#include <boost/math/concepts/real_type_concept.hpp>

#include <lib/high-precision/ToFromPythonConverter.hpp>
CREATE_CPP_LOCAL_LOGGER("_math.cpp")

namespace boost {
void assertion_failed(char const* expr, char const* function, char const* /*file*/, long line)
{
	std::string msg = function + (":" + boost::lexical_cast<std::string>(line)) + ", test failed for: " + expr;
	LOG_FATAL(msg);
	throw std::runtime_error(msg.c_str());
};
}

namespace py = ::boost::python;
using ::yade::Complex;
using ::yade::ComplexHP;
using ::yade::Real;
using ::yade::RealHP;

// Converts a std::pair instance to a Python tuple.
template <typename T1, typename T2> struct std_pair_to_tuple {
	static PyObject*           convert(std::pair<T1, T2> const& p) { return boost::python::incref(boost::python::make_tuple(p.first, p.second).ptr()); }
	static PyTypeObject const* get_pytype() { return &PyTuple_Type; }
};

// Helper for convenience.
template <typename T1, typename T2> struct std_pair_to_python_converter {
	std_pair_to_python_converter()
	{
		boost::python::to_python_converter<
		        std::pair<T1, T2>,
		        std_pair_to_tuple<T1, T2>,
		        true //std_pair_to_tuple has get_pytype
		        >();
	}
};

template <int N> RealHP<N> roundTrip(const RealHP<N>& x) { return x; }

template <int N> std::pair<RealHP<N>, int> test_frexp(const RealHP<N>& x)
{
	int       i   = 0;
	RealHP<N> ret = ::yade::math::frexp(x, &i);
	return std::pair<RealHP<N>, int> { ret, i };
}

template <int N> std::pair<RealHP<N>, RealHP<N>> test_modf(const RealHP<N>& x)
{
	RealHP<N> r   = 0;
	RealHP<N> ret = ::yade::math::modf(x, &r);
	return std::pair<RealHP<N>, RealHP<N>> { ret, r };
}

template <int N> std::pair<RealHP<N>, long> test_remquo(const RealHP<N>& x, const RealHP<N>& y)
{
	int       i   = 0;
	RealHP<N> ret = ::yade::math::remquo(x, y, &i);
	return std::pair<RealHP<N>, long> { ret, i };
}


#ifdef YADE_CGAL

namespace TestCGAL {
template <int N> bool                      Is_valid(const RealHP<N>& x) { return CGAL::Is_valid<RealHP<N>>()(x); }
template <int N> RealHP<N>                 Square(const RealHP<N>& x) { return typename CGAL::Algebraic_structure_traits<RealHP<N>>::Square()(x); }
template <int N> RealHP<N>                 Sqrt(const RealHP<N>& x) { return typename CGAL::Algebraic_structure_traits<RealHP<N>>::Sqrt()(x); }
template <int N> RealHP<N>                 K_root(int k, const RealHP<N>& x) { return typename CGAL::Algebraic_structure_traits<RealHP<N>>::Kth_root()(k, x); }
template <int N> std::pair<double, double> To_interval(const RealHP<N>& x) { return typename CGAL::Real_embeddable_traits<RealHP<N>>::To_interval()(x); }
template <int N> int                       Sgn(const RealHP<N>& x) { return int(typename CGAL::Real_embeddable_traits<RealHP<N>>::Sgn()(x)); }
template <int N> bool                      Is_finite(const RealHP<N>& x) { return typename CGAL::Real_embeddable_traits<RealHP<N>>::Is_finite()(x); }
}

namespace yade {
template <int N> RealHP<N> simpleCgalNumTraitsCalculation()
{
	typename CgalHP<N>::CGALpoint  x(RealHP<N>(1), RealHP<N>(1), RealHP<N>(1));
	typename CgalHP<N>::CGALpoint  p1(RealHP<N>(0), RealHP<N>(0), RealHP<N>(0));
	typename CgalHP<N>::CGALvector v1(RealHP<N>(1), RealHP<N>(1), RealHP<N>(1));
	typename CgalHP<N>::Plane      P(p1, v1);
	RealHP<N>                      h = P.a() * x.x() + P.b() * x.y() + P.c() * x.z() + P.d();
	return ((h > 0.) - (h < 0.)) * pow(h, 2) / (typename CgalHP<N>::CGALvector(P.a(), P.b(), P.c())).squared_length();
}
}

#endif

template <int N, bool> struct Var {
	RealHP<N>    value { -71.23 };
	ComplexHP<N> valueComplex { -71.23, 33.23 };

	RealHP<N> get() const { return value; };
	void      set(RealHP<N> val) { value = val; };

	ComplexHP<N> getComplex() const { return valueComplex; };
	void         setComplex(ComplexHP<N> val) { valueComplex = val; };
};

// A note: on page 54 in book "C++ Templates", D.Vandevoorde a 'template <auto N> struct { … };' allows to have both the type and its value. But it requires C++17.
// so that I could declare UnderlyingRealHP which accepts both 'int' and 'RealHP<N>'
// template <int N> using UnderlyingRealHP_int = UnderlyingRealHP<RealHP<N>>; // this is to allow `int` template arguments

template <int N> void compareVec(const std::vector<RealHP<N>>& vec, const ::yade::math::UnderlyingRealHP<RealHP<N>>* array)
{
	for (size_t i = 0; i < vec.size(); i++) {
		if (vec[i] != array[i]) {
			std::cerr << __PRETTY_FUNCTION__ << " failed test\n";
			exit(1);
		}
	}
}

#include <boost/range/combine.hpp>
// this funcction simulates some external library which works on C-arrays.
template <int N> void multVec(::yade::math::UnderlyingRealHP<RealHP<N>>* array, const ::yade::math::UnderlyingRealHP<RealHP<N>>& fac, size_t s)
{
	for (size_t i = 0; i < s; i++)
		array[i] *= fac;
}

namespace yade {
template <int N> void testConstants()
{
	LOG_NOFILTER("Checking ConstantsHP<" << N << ">");
	BOOST_ASSERT(math::ConstantsHP<N>::PI == boost::math::constants::pi<RealHP<N>>());
	BOOST_ASSERT(math::ConstantsHP<N>::TWO_PI == boost::math::constants::two_pi<RealHP<N>>());
	BOOST_ASSERT(math::ConstantsHP<N>::HALF_PI == boost::math::constants::half_pi<RealHP<N>>());
	BOOST_ASSERT(math::ConstantsHP<N>::SQRT_TWO_PI == boost::math::constants::root_two_pi<RealHP<N>>());
	BOOST_ASSERT(math::ConstantsHP<N>::E == boost::math::constants::e<RealHP<N>>());
	BOOST_ASSERT(math::ConstantsHP<N>::I == ComplexHP<N>(0, 1));

	// these two involve division. On 32bit architecture there might be a different rounding, which may result in an error. Let's hope it's not worse on other architectures.
	BOOST_ASSERT(math::abs(boost::math::float_distance(math::ConstantsHP<N>::DEG_TO_RAD, math::ConstantsHP<N>::PI / RealHP<N>(180))) <= 1);
	BOOST_ASSERT(math::abs(boost::math::float_distance(math::ConstantsHP<N>::RAD_TO_DEG, RealHP<N>(180) / math::ConstantsHP<N>::PI)) <= 1);

	BOOST_ASSERT(math::ConstantsHP<N>::EPSILON == std::numeric_limits<RealHP<N>>::epsilon());
	BOOST_ASSERT(math::ConstantsHP<N>::MAX_REAL == std::numeric_limits<RealHP<N>>::max());
	BOOST_ASSERT(math::ConstantsHP<N>::ZERO_TOLERANCE == RealHP<N>(1e-20));
}
void                  testLoopRealHP();
template <int N> void testArray()
{
	LOG_NOFILTER(
	        "sizeof RealHP<" << N << ">" << std::setw(2) << " "
	                         << " in bytes: " << sizeof(RealHP<N>));
	LOG_NOFILTER("sizeof float       in bytes: " << sizeof(float));
	LOG_NOFILTER("sizeof double      in bytes: " << sizeof(double));
	LOG_NOFILTER("sizeof long double in bytes: " << sizeof(long double));
	std::vector<RealHP<N>> vec {};
	int                    i = 1000;
	while (i-- > 0)
		vec.push_back(math::random01HP<N>());
	compareVec<N>(vec, math::constVectorData(vec));
	auto      copy = vec;
	RealHP<N> fac  = 0.25;
	multVec<N>(math::vectorData(vec), static_cast<::yade::math::UnderlyingRealHP<RealHP<N>>>(fac), vec.size());
	for (auto a : boost::combine(copy, vec)) {
		if (a.template get<0>() * fac != a.template get<1>()) {
			std::cerr << __PRETTY_FUNCTION__ << " failed test\n";
			exit(1);
		}
	}
	LOG_NOFILTER("calling testLoopRealHP()");
	testLoopRealHP();
}
}

namespace {
template <int N> static inline RealHP<N> smallest_positive() { return std::numeric_limits<RealHP<N>>::min(); }
}

//#if not(defined(YADE_EIGEN_NUM_TRAITS_HPP) or defined(EIGEN_MPREALSUPPORT_MODULE_H))
namespace boost {
namespace multiprecision {
}
}

template <int N> struct Substitute {
	static constexpr long   get_default_prec = std::numeric_limits<RealHP<N>>::digits;
	static inline RealHP<N> highest(long = get_default_prec) { return std::numeric_limits<RealHP<N>>::max(); }
	static inline RealHP<N> lowest(long = get_default_prec) { return std::numeric_limits<RealHP<N>>::lowest(); }
	static inline RealHP<N> Pi(long = get_default_prec) { return boost::math::constants::pi<RealHP<N>>(); }
	static inline RealHP<N> Euler(long = get_default_prec) { return boost::math::constants::euler<RealHP<N>>(); }
	static inline RealHP<N> Log2(long = get_default_prec)
	{
		using namespace boost::multiprecision;
		using namespace std;
		return log(RealHP<N>(2));
	}
	static inline RealHP<N> Catalan(long = get_default_prec) { return boost::math::constants::catalan<RealHP<N>>(); }

	static inline RealHP<N> epsilon(long = get_default_prec) { return std::numeric_limits<RealHP<N>>::epsilon(); }
	static inline RealHP<N> epsilon(const RealHP<N>&) { return std::numeric_limits<RealHP<N>>::epsilon(); }
	static inline bool      isEqualFuzzy(const RealHP<N>& a, const RealHP<N>& b, const RealHP<N>& eps)
	{
		using namespace boost::multiprecision;
		using namespace std;
		return abs(a - b) <= eps;
	}
};
//#endif

// The 'if constexpr(…)' is not working for C++14. So I have to simulate it using templates. Otherwise the python 'import >>thisModule<<' is really slow.
template <int, bool> struct IfConstexprForSlowFunctions {
	static void work(const py::scope&) {};
};

template <int N> struct IfConstexprForSlowFunctions<N, true> {
	static void work(const py::scope& scopeHP)
	{
		py::scope HPn(scopeHP);
		py::def("erf",
		        static_cast<RealHP<N> (*)(const RealHP<N>&)>(&::yade::math::erf),
		        (py::arg("x")),
		        R"""(:return: ``Real`` Computes the `error function <https://en.wikipedia.org/wiki/Error_function>`__ of argument. Depending on compilation options wraps ``::boost::multiprecision::erf(…)`` or `std::erf(…) <https://en.cppreference.com/w/cpp/numeric/math/erf>`__ function.)""");
		py::def("erfc",
		        static_cast<RealHP<N> (*)(const RealHP<N>&)>(&::yade::math::erfc),
		        (py::arg("x")),
		        R"""(:return: ``Real`` Computes the `complementary error function <https://en.wikipedia.org/wiki/Error_function#Complementary_error_function>`__ of argument, that is ``1.0-erf(arg)``. Depending on compilation options wraps ``::boost::multiprecision::erfc(…)`` or `std::erfc(…) <https://en.cppreference.com/w/cpp/numeric/math/erfc>`__ function.)""");
		py::def("lgamma",
		        static_cast<RealHP<N> (*)(const RealHP<N>&)>(&::yade::math::lgamma),
		        (py::arg("x")),
		        R"""(:return: ``Real`` Computes the natural logarithm of the absolute value of the `gamma function <https://en.wikipedia.org/wiki/Gamma_function>`__ of arg. Depending on compilation options wraps ``::boost::multiprecision::lgamma(…)`` or `std::lgamma(…) <https://en.cppreference.com/w/cpp/numeric/math/lgamma>`__ function.)""");
		py::def("tgamma",
		        static_cast<RealHP<N> (*)(const RealHP<N>&)>(&::yade::math::tgamma),
		        (py::arg("x")),
		        R"""(:return: ``Real`` Computes the `gamma function <https://en.wikipedia.org/wiki/Gamma_function>`__ of arg. Depending on compilation options wraps ``::boost::multiprecision::tgamma(…)`` or `std::tgamma(…) <https://en.cppreference.com/w/cpp/numeric/math/tgamma>`__ function.)""");
	}
};

template <int, bool> struct IfConstexprForEigen;

#ifndef __clang__ // problem with clang 9.0.1-12. It tries to instatinate an explicitly illegal/disabled code. Produces errors that some types don't exist. But they shouldn't exist.
template <int N> struct IfConstexprForEigen<N, true> {
	static void work(const py::scope& scopeHP, const long& defprec)
	{
		py::scope HPn(scopeHP);
		py::def("highest",
		        Eigen::NumTraits<RealHP<N>>::highest,
		        (py::arg("Precision") = defprec),
		        R"""(:return: ``Real`` returns the largest finite value of the ``Real`` type. Wraps `std::numeric_limits<Real>::max() <https://en.cppreference.com/w/cpp/types/numeric_limits/max>`__ function.)""");
		py::def("lowest",
		        Eigen::NumTraits<RealHP<N>>::lowest,
		        (py::arg("Precision") = defprec),
		        R"""(:return: ``Real`` returns the lowest (negative) finite value of the ``Real`` type. Wraps `std::numeric_limits<Real>::lowest() <https://en.cppreference.com/w/cpp/types/numeric_limits/lowest>`__ function.)""");

		py::def("Pi",
		        Eigen::NumTraits<RealHP<N>>::Pi,
		        (py::arg("Precision") = defprec),
		        R"""(:return: ``Real`` The `π constant <https://en.wikipedia.org/wiki/Pi>`__, exposed to python for :ysrc:`testing <py/tests/testMath.py>` of :ysrc:`eigen numerical traits<lib/high-precision/EigenNumTraits.hpp>`.)""");
		py::def("Euler",
		        Eigen::NumTraits<RealHP<N>>::Euler,
		        (py::arg("Precision") = defprec),
		        R"""(:return: ``Real`` The `Euler–Mascheroni constant <https://en.wikipedia.org/wiki/Euler%E2%80%93Mascheroni_constant>`__, exposed to python for :ysrc:`testing <py/tests/testMath.py>` of :ysrc:`eigen numerical traits<lib/high-precision/EigenNumTraits.hpp>`.)""");
		py::def("Log2",
		        Eigen::NumTraits<RealHP<N>>::Log2,
		        (py::arg("Precision") = defprec),
		        R"""(:return: ``Real`` natural logarithm of 2, exposed to python for :ysrc:`testing <py/tests/testMath.py>` of :ysrc:`eigen numerical traits<lib/high-precision/EigenNumTraits.hpp>`.)""");
		py::def("Catalan",
		        Eigen::NumTraits<RealHP<N>>::Catalan,
		        (py::arg("Precision") = defprec),
		        R"""(:return: ``Real`` The `catalan constant <https://en.wikipedia.org/wiki/Catalan%27s_constant>`__, exposed to python for :ysrc:`testing <py/tests/testMath.py>` of :ysrc:`eigen numerical traits<lib/high-precision/EigenNumTraits.hpp>`.)""");

		py::def("epsilon",
		        static_cast<RealHP<N> (*)(long)>(&Eigen::NumTraits<RealHP<N>>::epsilon),
		        (py::arg("Precision") = defprec),
		        R"""(:return: ``Real`` returns the difference between ``1.0`` and the next representable value of the ``Real`` type. Wraps `std::numeric_limits<Real>::epsilon() <https://en.cppreference.com/w/cpp/types/numeric_limits/epsilon>`__ function.)""");
		py::def("epsilon",
		        static_cast<RealHP<N> (*)(const RealHP<N>&)>(&Eigen::NumTraits<RealHP<N>>::epsilon),
		        (py::arg("x")),
		        R"""(:return: ``Real`` returns the difference between ``1.0`` and the next representable value of the ``Real`` type. Wraps `std::numeric_limits<Real>::epsilon() <https://en.cppreference.com/w/cpp/types/numeric_limits/epsilon>`__ function.)""");
		py::def("isEqualFuzzy",
		        static_cast<bool (*)(const RealHP<N>&, const RealHP<N>&, const RealHP<N>&)>(&Eigen::internal::isEqualFuzzy),
		        R"""(:return: ``bool``, ``True`` if the absolute difference between two numbers is smaller than `std::numeric_limits<Real>::epsilon() <https://en.cppreference.com/w/cpp/types/numeric_limits/epsilon>`__)""");
		py::def("smallest_positive",
		        static_cast<RealHP<N> (*)()>(&Eigen::NumTraits<RealHP<N>>::smallest_positive),
		        R"""(:return: ``Real`` the smallest number greater than zero. Wraps `std::numeric_limits<Real>::min() <https://en.cppreference.com/w/cpp/types/numeric_limits/min>`__)""");
	}
};
#endif

template <int N> struct IfConstexprForEigen<N, false> {
	static void work(const py::scope& scopeHP, const long& defprec)
	{
		py::scope HPn(scopeHP);
		py::def("highest",
		        Substitute<N>::highest,
		        (py::arg("Precision") = defprec),
		        R"""(:return: ``Real`` returns the largest finite value of the ``Real`` type. Wraps `std::numeric_limits<Real>::max() <https://en.cppreference.com/w/cpp/types/numeric_limits/max>`__ function.)""");
		py::def("lowest",
		        Substitute<N>::lowest,
		        (py::arg("Precision") = defprec),
		        R"""(:return: ``Real`` returns the lowest (negative) finite value of the ``Real`` type. Wraps `std::numeric_limits<Real>::lowest() <https://en.cppreference.com/w/cpp/types/numeric_limits/lowest>`__ function.)""");

		py::def("Pi",
		        Substitute<N>::Pi,
		        (py::arg("Precision") = defprec),
		        R"""(:return: ``Real`` The `π constant <https://en.wikipedia.org/wiki/Pi>`__, exposed to python for :ysrc:`testing <py/tests/testMath.py>` of :ysrc:`eigen numerical traits<lib/high-precision/EigenNumTraits.hpp>`.)""");
		py::def("Euler",
		        Substitute<N>::Euler,
		        (py::arg("Precision") = defprec),
		        R"""(:return: ``Real`` The `Euler–Mascheroni constant <https://en.wikipedia.org/wiki/Euler%E2%80%93Mascheroni_constant>`__, exposed to python for :ysrc:`testing <py/tests/testMath.py>` of :ysrc:`eigen numerical traits<lib/high-precision/EigenNumTraits.hpp>`.)""");
		py::def("Log2",
		        Substitute<N>::Log2,
		        (py::arg("Precision") = defprec),
		        R"""(:return: ``Real`` natural logarithm of 2, exposed to python for :ysrc:`testing <py/tests/testMath.py>` of :ysrc:`eigen numerical traits<lib/high-precision/EigenNumTraits.hpp>`.)""");
		py::def("Catalan",
		        Substitute<N>::Catalan,
		        (py::arg("Precision") = defprec),
		        R"""(:return: ``Real`` The `catalan constant <https://en.wikipedia.org/wiki/Catalan%27s_constant>`__, exposed to python for :ysrc:`testing <py/tests/testMath.py>` of :ysrc:`eigen numerical traits<lib/high-precision/EigenNumTraits.hpp>`.)""");

		py::def("epsilon",
		        static_cast<RealHP<N> (*)(long)>(&Substitute<N>::epsilon),
		        (py::arg("Precision") = defprec),
		        R"""(:return: ``Real`` returns the difference between ``1.0`` and the next representable value of the ``Real`` type. Wraps `std::numeric_limits<Real>::epsilon() <https://en.cppreference.com/w/cpp/types/numeric_limits/epsilon>`__ function.)""");
		py::def("epsilon",
		        static_cast<RealHP<N> (*)(const RealHP<N>&)>(&Substitute<N>::epsilon),
		        (py::arg("x")),
		        R"""(:return: ``Real`` returns the difference between ``1.0`` and the next representable value of the ``Real`` type. Wraps `std::numeric_limits<Real>::epsilon() <https://en.cppreference.com/w/cpp/types/numeric_limits/epsilon>`__ function.)""");
		py::def("smallest_positive",
		        static_cast<RealHP<N> (*)()>(&smallest_positive<N>),
		        R"""(:return: ``Real`` the smallest number greater than zero. Wraps `std::numeric_limits<Real>::min() <https://en.cppreference.com/w/cpp/types/numeric_limits/min>`__)""");
		py::def("isEqualFuzzy",
		        Substitute<N>::isEqualFuzzy,
		        R"""(:return: ``bool``, ``True`` if the absolute difference between two numbers is smaller than `std::numeric_limits<Real>::epsilon() <https://en.cppreference.com/w/cpp/types/numeric_limits/epsilon>`__)""");
	}
};

template <int N, bool registerConverters> struct RegisterRealHPMath {
	// python 'import this_module' measured time: skipSlowFunctionsAbove_N==6 → 10min, N==5 → 3m24s, N==4 → 1m55s, N==3 → 1minute23sec
	static const constexpr int skipSlowFunctionsAbove_N = yade::math::RealHPConfig::workaroundSlowBoostBinFloat;

	static void work(const py::scope& topScope, const py::scope& scopeHP)
	{
		//LOG_NOFILTER("Registering RealHP<" << N << "> and ComplexHP<" << N << ">");

		// Very important line: Verifies that Real type satisfies all the requirements of RealTypeConcept
		BOOST_CONCEPT_ASSERT((boost::math::concepts::RealTypeConcept<RealHP<N>>));
		if (::yade::math::isHP<RealHP<N>> == false) {
			throw std::runtime_error("::yade::math::isHP<RealHP<N1>> == false, please file a bug report.");
		};
		if (::yade::math::isHP<ComplexHP<N>> == false) {
			throw std::runtime_error("::yade::math::isHP<ComplexHP<N1>> == false, please file a bug report.");
		};

		if (registerConverters) {
			py::scope top(topScope);

			// Below all functions from lib/high-precision/MathFunctions.hpp are exported for tests.
			// Some of these functions return two element tuples: frexp, modf, remquo, CGAL_To_interval
			// so the std::pair to python tuple converters must be registered
			std_pair_to_python_converter<RealHP<N>, RealHP<N>>();
			std_pair_to_python_converter<RealHP<N>, long>();
			std_pair_to_python_converter<RealHP<N>, int>();
		}

		py::scope HPn(scopeHP);

		py::class_<Var<N, registerConverters>>("Var", "The ``Var`` class is used to test to/from python converters for arbitrary precision ``Real``")
		        .add_property("val", &Var<N, registerConverters>::get, &Var<N, registerConverters>::set, "one ``Real`` variable for testing.")
		        .add_property(
		                "cpl",
		                &Var<N, registerConverters>::getComplex,
		                &Var<N, registerConverters>::setComplex,
		                "one ``Complex`` variable to test reading from and writing to it.");

		/********************************************************************************************/
		/**********************        complex trigonometric functions         **********************/
		/********************************************************************************************/
		// complex functions must be registered first, so that python will properly discover overloads
		py::def("sin",
		        static_cast<ComplexHP<N> (*)(const ComplexHP<N>&)>(&::yade::math::sin),
		        (py::arg("x")),
		        R"""(:return: ``Complex`` the sine of the ``Complex`` argument in radians. Depending on compilation options wraps ``::boost::multiprecision::sin(…)`` or `std::sin(…) <https://en.cppreference.com/w/cpp/numeric/complex/sin>`__ function.)""");
		py::def("sinh",
		        static_cast<ComplexHP<N> (*)(const ComplexHP<N>&)>(&::yade::math::sinh),
		        (py::arg("x")),
		        R"""(:return: ``Complex`` the hyperbolic sine of the ``Complex`` argument in radians. Depending on compilation options wraps ``::boost::multiprecision::sinh(…)`` or `std::sinh(…) <https://en.cppreference.com/w/cpp/numeric/complex/sinh>`__ function.)""");
		py::def("cos",
		        static_cast<ComplexHP<N> (*)(const ComplexHP<N>&)>(&::yade::math::cos),
		        (py::arg("x")),
		        R"""(:return: ``Complex`` the cosine of the ``Complex`` argument in radians. Depending on compilation options wraps ``::boost::multiprecision::cos(…)`` or `std::cos(…) <https://en.cppreference.com/w/cpp/numeric/complex/cos>`__ function.)""");
		py::def("cosh",
		        static_cast<ComplexHP<N> (*)(const ComplexHP<N>&)>(&::yade::math::cosh),
		        (py::arg("x")),
		        R"""(:return: ``Complex`` the hyperbolic cosine of the ``Complex`` argument in radians. Depending on compilation options wraps ``::boost::multiprecision::cosh(…)`` or `std::cosh(…) <https://en.cppreference.com/w/cpp/numeric/complex/cosh>`__ function.)""");
		py::def("tan",
		        static_cast<ComplexHP<N> (*)(const ComplexHP<N>&)>(&::yade::math::tan),
		        (py::arg("x")),
		        R"""(:return: ``Complex`` the tangent of the ``Complex`` argument in radians. Depending on compilation options wraps ``::boost::multiprecision::tan(…)`` or `std::tan(…) <https://en.cppreference.com/w/cpp/numeric/complex/tan>`__ function.)""");
		py::def("tanh",
		        static_cast<ComplexHP<N> (*)(const ComplexHP<N>&)>(&::yade::math::tanh),
		        (py::arg("x")),
		        R"""(:return: ``Complex`` the hyperbolic tangent of the ``Complex`` argument in radians. Depending on compilation options wraps ``::boost::multiprecision::tanh(…)`` or `std::tanh(…) <https://en.cppreference.com/w/cpp/numeric/complex/tanh>`__ function.)""");


		/********************************************************************************************/
		/**********************            trigonometric functions             **********************/
		/********************************************************************************************/
		// Real versions are registered afterwards
		py::def("sin",
		        static_cast<RealHP<N> (*)(const RealHP<N>&)>(&::yade::math::sin),
		        (py::arg("x")),
		        R"""(:return: ``Real`` the sine of the ``Real`` argument in radians. Depending on compilation options wraps ``::boost::multiprecision::sin(…)`` or `std::sin(…) <https://en.cppreference.com/w/cpp/numeric/math/sin>`__ function.)""");
		py::def("sinh",
		        static_cast<RealHP<N> (*)(const RealHP<N>&)>(&::yade::math::sinh),
		        (py::arg("x")),
		        R"""(:return: ``Real`` the hyperbolic sine of the ``Real`` argument in radians. Depending on compilation options wraps ``::boost::multiprecision::sinh(…)`` or `std::sinh(…) <https://en.cppreference.com/w/cpp/numeric/math/sinh>`__ function.)""");
		py::def("cos",
		        static_cast<RealHP<N> (*)(const RealHP<N>&)>(&::yade::math::cos),
		        (py::arg("x")),
		        R"""(:return: ``Real`` the cosine of the ``Real`` argument in radians. Depending on compilation options wraps ``::boost::multiprecision::cos(…)`` or `std::cos(…) <https://en.cppreference.com/w/cpp/numeric/math/cos>`__ function.)""");
		py::def("cosh",
		        static_cast<RealHP<N> (*)(const RealHP<N>&)>(&::yade::math::cosh),
		        (py::arg("x")),
		        R"""(:return: ``Real`` the hyperbolic cosine of the ``Real`` argument in radians. Depending on compilation options wraps ``::boost::multiprecision::cosh(…)`` or `std::cosh(…) <https://en.cppreference.com/w/cpp/numeric/math/cosh>`__ function.)""");
		py::def("tan",
		        static_cast<RealHP<N> (*)(const RealHP<N>&)>(&::yade::math::tan),
		        (py::arg("x")),
		        R"""(:return: ``Real`` the tangent of the ``Real`` argument in radians. Depending on compilation options wraps ``::boost::multiprecision::tan(…)`` or `std::tan(…) <https://en.cppreference.com/w/cpp/numeric/math/tan>`__ function.)""");
		py::def("tanh",
		        static_cast<RealHP<N> (*)(const RealHP<N>&)>(&::yade::math::tanh),
		        (py::arg("x")),
		        R"""(:return: ``Real`` the hyperbolic tangent of the ``Real`` argument in radians. Depending on compilation options wraps ``::boost::multiprecision::tanh(…)`` or `std::tanh(…) <https://en.cppreference.com/w/cpp/numeric/math/tanh>`__ function.)""");


		/********************************************************************************************/
		/**********************        inverse trigonometric functions         **********************/
		/********************************************************************************************/
		py::def("asin",
		        static_cast<RealHP<N> (*)(const RealHP<N>&)>(&::yade::math::asin),
		        (py::arg("x")),
		        R"""(:return: ``Real`` the arcus sine of the argument. Depending on compilation options wraps ``::boost::multiprecision::asin(…)`` or `std::asin(…) <https://en.cppreference.com/w/cpp/numeric/math/asin>`__ function.)""");
		py::def("asinh",
		        static_cast<RealHP<N> (*)(const RealHP<N>&)>(&::yade::math::asinh),
		        (py::arg("x")),
		        R"""(:return: ``Real`` the hyperbolic arcus sine of the argument. Depending on compilation options wraps ``::boost::multiprecision::asinh(…)`` or `std::asinh(…) <https://en.cppreference.com/w/cpp/numeric/math/asinh>`__ function.)""");
		py::def("acos",
		        static_cast<RealHP<N> (*)(const RealHP<N>&)>(&::yade::math::acos),
		        (py::arg("x")),
		        R"""(:return: ``Real`` the arcus cosine of the argument. Depending on compilation options wraps ``::boost::multiprecision::acos(…)`` or `std::acos(…) <https://en.cppreference.com/w/cpp/numeric/math/acos>`__ function.)""");
		py::def("acosh",
		        static_cast<RealHP<N> (*)(const RealHP<N>&)>(&::yade::math::acosh),
		        (py::arg("x")),
		        R"""(:return: ``Real`` the hyperbolic arcus cosine of the argument. Depending on compilation options wraps ``::boost::multiprecision::acosh(…)`` or `std::acosh(…) <https://en.cppreference.com/w/cpp/numeric/math/acosh>`__ function.)""");
		py::def("atan",
		        static_cast<RealHP<N> (*)(const RealHP<N>&)>(&::yade::math::atan),
		        (py::arg("x")),
		        R"""(:return: ``Real`` the arcus tangent of the argument. Depending on compilation options wraps ``::boost::multiprecision::atan(…)`` or `std::atan(…) <https://en.cppreference.com/w/cpp/numeric/math/atan>`__ function.)""");
		py::def("atanh",
		        static_cast<RealHP<N> (*)(const RealHP<N>&)>(&::yade::math::atanh),
		        (py::arg("x")),
		        R"""(:return: ``Real`` the hyperbolic arcus tangent of the argument. Depending on compilation options wraps ``::boost::multiprecision::atanh(…)`` or `std::atanh(…) <https://en.cppreference.com/w/cpp/numeric/math/atanh>`__ function.)""");
		py::def("atan2",
		        static_cast<RealHP<N> (*)(const RealHP<N>&, const RealHP<N>&)>(&::yade::math::atan2),
		        (py::arg("x"), "y"),
		        R"""(:return: ``Real`` the arc tangent of y/x using the signs of the arguments ``x`` and ``y`` to determine the correct quadrant. Depending on compilation options wraps ``::boost::multiprecision::atan2(…)`` or `std::atan2(…) <https://en.cppreference.com/w/cpp/numeric/math/atan2>`__ function.)""");


		/********************************************************************************************/
		/**********************        complex logarithm and exponential        *********************/
		/********************************************************************************************/
		// complex functions must be registered first, so that python will properly discover overloads
		py::def("exp",
		        static_cast<ComplexHP<N> (*)(const ComplexHP<N>&)>(&::yade::math::exp),
		        (py::arg("x")),
		        R"""(:return: the base `e` exponential of a ``Complex`` argument. Depending on compilation options wraps ``::boost::multiprecision::exp(…)`` or `std::exp(…) <https://en.cppreference.com/w/cpp/numeric/complex/exp>`__ function.)""");
		py::def("log",
		        static_cast<ComplexHP<N> (*)(const ComplexHP<N>&)>(&::yade::math::log),
		        (py::arg("x")),
		        R"""(:return: the ``Complex`` natural (base `e`) logarithm of a complex value z with a branch cut along the negative real axis. Depending on compilation options wraps ``::boost::multiprecision::log(…)`` or `std::log(…) <https://en.cppreference.com/w/cpp/numeric/complex/log>`__ function.)""");


		/********************************************************************************************/
		/**********************   logarithm, exponential and power functions   **********************/
		/********************************************************************************************/
		py::def("log",
		        static_cast<RealHP<N> (*)(const RealHP<N>&)>(&::yade::math::log),
		        (py::arg("x")),
		        R"""(:return: the ``Real`` natural (base `e`) logarithm of a real value. Depending on compilation options wraps ``::boost::multiprecision::log(…)`` or `std::log(…) <https://en.cppreference.com/w/cpp/numeric/math/log>`__ function.)""");
		py::def("log10",
		        static_cast<RealHP<N> (*)(const RealHP<N>&)>(&::yade::math::log10),
		        (py::arg("x")),
		        R"""(:return: the ``Real`` decimal (base ``10``) logarithm of a real value. Depending on compilation options wraps ``::boost::multiprecision::log10(…)`` or `std::log10(…) <https://en.cppreference.com/w/cpp/numeric/math/log10>`__ function.)""");
		py::def("log1p",
		        static_cast<RealHP<N> (*)(const RealHP<N>&)>(&::yade::math::log1p),
		        (py::arg("x")),
		        R"""(:return: the ``Real`` natural (base `e`) logarithm of ``1+argument``. Depending on compilation options wraps ``::boost::multiprecision::log1p(…)`` or `std::log1p(…) <https://en.cppreference.com/w/cpp/numeric/math/log1p>`__ function.)""");
		py::def("log2",
		        static_cast<RealHP<N> (*)(const RealHP<N>&)>(&::yade::math::log2),
		        (py::arg("x")),
		        R"""(:return: the ``Real`` binary (base ``2``) logarithm of a real value. Depending on compilation options wraps ``::boost::multiprecision::log2(…)`` or `std::log2(…) <https://en.cppreference.com/w/cpp/numeric/math/log2>`__ function.)""");
		py::def("logb",
		        static_cast<RealHP<N> (*)(const RealHP<N>&)>(&::yade::math::logb),
		        (py::arg("x")),
		        R"""(:return: Extracts the value of the unbiased radix-independent exponent from the floating-point argument arg, and returns it as a floating-point value. Depending on compilation options wraps ``::boost::multiprecision::logb(…)`` or `std::logb(…) <https://en.cppreference.com/w/cpp/numeric/math/logb>`__ function.)""");
		py::def("ilogb",
		        static_cast<RealHP<N> (*)(const RealHP<N>&)>(&::yade::math::ilogb),
		        (py::arg("x")),
		        R"""(:return: ``Real`` extracts the value of the unbiased exponent from the floating-point argument arg, and returns it as a signed integer value. Depending on compilation options wraps ``::boost::multiprecision::ilogb(…)`` or `std::ilogb(…) <https://en.cppreference.com/w/cpp/numeric/math/ilogb>`__ function.)""");
		py::def("ldexp",
		        static_cast<RealHP<N> (*)(const RealHP<N>&, int)>(&::yade::math::ldexp),
		        (py::arg("x"), "y"),
		        R"""(:return: Multiplies a floating point value ``x`` by the number 2 raised to the ``exp`` power. Depending on compilation options wraps ``::boost::multiprecision::ldexp(…)`` or `std::ldexp(…) <https://en.cppreference.com/w/cpp/numeric/math/ldexp>`__ function.)""");
		py::def("frexp",
		        test_frexp<N>,
		        (py::arg("x")),
		        R"""(:return: tuple of ``(Real,int)``, decomposes given floating point ``Real`` argument into a normalized fraction and an integral power of two. Depending on compilation options wraps ``::boost::multiprecision::frexp(…)`` or `std::frexp(…) <https://en.cppreference.com/w/cpp/numeric/math/frexp>`__ function.)""");
		py::def("exp",
		        static_cast<RealHP<N> (*)(const RealHP<N>&)>(&::yade::math::exp),
		        (py::arg("x")),
		        R"""(:return: the base `e` exponential of a ``Real`` argument. Depending on compilation options wraps ``::boost::multiprecision::exp(…)`` or `std::exp(…) <https://en.cppreference.com/w/cpp/numeric/math/exp>`__ function.)""");
		py::def("exp2",
		        static_cast<RealHP<N> (*)(const RealHP<N>&)>(&::yade::math::exp2),
		        (py::arg("x")),
		        R"""(:return: the base `2` exponential of a ``Real`` argument. Depending on compilation options wraps ``::boost::multiprecision::exp2(…)`` or `std::exp2(…) <https://en.cppreference.com/w/cpp/numeric/math/exp2>`__ function.)""");
		py::def("expm1",
		        static_cast<RealHP<N> (*)(const RealHP<N>&)>(&::yade::math::expm1),
		        (py::arg("x")),
		        R"""(:return: the base `e` exponential of a ``Real`` argument minus ``1.0``. Depending on compilation options wraps ``::boost::multiprecision::expm1(…)`` or `std::expm1(…) <https://en.cppreference.com/w/cpp/numeric/math/expm1>`__ function.)""");
		py::def("pow",
		        static_cast<RealHP<N> (*)(const RealHP<N>&, const RealHP<N>&)>(&::yade::math::pow),
		        (py::arg("x"), "y"),
		        R"""(:return: ``Real`` the value of ``base`` raised to the power ``exp``. Depending on compilation options wraps ``::boost::multiprecision::pow(…)`` or `std::pow(…) <https://en.cppreference.com/w/cpp/numeric/math/pow>`__ function.)""");
		py::def("sqrt",
		        static_cast<RealHP<N> (*)(const RealHP<N>&)>(&::yade::math::sqrt),
		        (py::arg("x")),
		        R"""(:return: ``Real`` square root of the argument. Depending on compilation options wraps ``::boost::multiprecision::sqrt(…)`` or `std::sqrt(…) <https://en.cppreference.com/w/cpp/numeric/math/sqrt>`__ function.)""");
		py::def("cbrt",
		        static_cast<RealHP<N> (*)(const RealHP<N>&)>(&::yade::math::cbrt),
		        (py::arg("x")),
		        R"""(:return: ``Real`` cubic root of the argument. Depending on compilation options wraps ``::boost::multiprecision::cbrt(…)`` or `std::cbrt(…) <https://en.cppreference.com/w/cpp/numeric/math/cbrt>`__ function.)""");
		py::def("hypot",
		        static_cast<RealHP<N> (*)(const RealHP<N>&, const RealHP<N>&)>(&::yade::math::hypot),
		        (py::arg("x"), "y"),
		        R"""(:return: ``Real`` the square root of the sum of the squares of ``x`` and ``y``, without undue overflow or underflow at intermediate stages of the computation. Depending on compilation options wraps ``::boost::multiprecision::hypot(…)`` or `std::hypot(…) <https://en.cppreference.com/w/cpp/numeric/math/hypot>`__ function.)""");


		/********************************************************************************************/
		/**********************         complex conj, abs, real, imag          *********************/
		/********************************************************************************************/
		// complex functions must be registered first, so that python will properly discover overloads
		py::def("conj",
		        static_cast<ComplexHP<N> (*)(const ComplexHP<N>&)>(&::yade::math::conj),
		        (py::arg("x")),
		        R"""(:return: the complex conjugation a ``Complex`` argument. Depending on compilation options wraps ``::boost::multiprecision::conj(…)`` or `std::conj(…) <https://en.cppreference.com/w/cpp/numeric/complex/conj>`__ function.)""");
		py::def("abs",
		        // note …<ComplexHP<N>> at the end, to help compiler in the template overload resolution.
		        static_cast<RealHP<N> (*)(const ComplexHP<N>&)>(&::yade::math::abs<ComplexHP<N>>),
		        (py::arg("x")),
		        R"""(:return: the ``Real`` absolute value of the ``Complex`` argument. Depending on compilation options wraps ``::boost::multiprecision::abs(…)`` or `std::abs(…) <https://en.cppreference.com/w/cpp/numeric/complex/abs>`__ function.)""");
		py::def("real",
		        static_cast<RealHP<N> (*)(const ComplexHP<N>&)>(&::yade::math::real),
		        (py::arg("x")),
		        R"""(:return: the real part of a ``Complex`` argument. Depending on compilation options wraps ``::boost::multiprecision::real(…)`` or `std::real(…) <https://en.cppreference.com/w/cpp/numeric/complex/real2>`__ function.)""");
		py::def("imag",
		        static_cast<RealHP<N> (*)(const ComplexHP<N>&)>(&::yade::math::imag),
		        (py::arg("x")),
		        R"""(:return: the imag part of a ``Complex`` argument. Depending on compilation options wraps ``::boost::multiprecision::imag(…)`` or `std::imag(…) <https://en.cppreference.com/w/cpp/numeric/complex/imag2>`__ function.)""");


		/********************************************************************************************/
		/**********************    min, max, abs, sign, floor, ceil, etc...    **********************/
		/********************************************************************************************/
		py::def("abs",
		        static_cast<RealHP<N> (*)(const RealHP<N>&)>(&::yade::math::abs),
		        //XXX note for YADE_REAL_BIT <= 64 different signature was used: static_cast<RealHP<N> (*)(Real)>(&::yade::math::abs); Is not needed anymore(?)
		        (py::arg("x")),
		        R"""(:return: the ``Real`` absolute value of the ``Real`` argument. Depending on compilation options wraps ``::boost::multiprecision::abs(…)`` or `std::abs(…) <https://en.cppreference.com/w/cpp/numeric/math/abs>`__ function.)""");
		py::def("fabs",
		        static_cast<RealHP<N> (*)(const RealHP<N>&)>(&::yade::math::fabs),
		        //XXX note for YADE_REAL_BIT <= 64 different signature was used: static_cast<RealHP<N> (*)(Real)>(&::yade::math::fabs; Is not needed anymore(?)
		        (py::arg("x")),
		        R"""(:return: the ``Real`` absolute value of the argument. Depending on compilation options wraps ``::boost::multiprecision::abs(…)`` or `std::abs(…) <https://en.cppreference.com/w/cpp/numeric/math/fabs>`__ function.)""");
		py::def("max",
		        static_cast<const RealHP<N>& (*)(const RealHP<N>&, const RealHP<N>&)>(&::yade::math::max),
		        (py::arg("x"), "y"),
		        py::return_value_policy<py::copy_const_reference>(),
		        R"""(:return: ``Real`` larger of the two arguments. Depending on compilation options wraps ``::boost::multiprecision::max(…)`` or `std::max(…) <https://en.cppreference.com/w/cpp/numeric/math/max>`__ function.)""");
		py::def("min",
		        static_cast<const RealHP<N>& (*)(const RealHP<N>&, const RealHP<N>&)>(&::yade::math::min),
		        (py::arg("x"), "y"),
		        py::return_value_policy<py::copy_const_reference>(),
		        R"""(:return: ``Real`` smaller of the two arguments. Depending on compilation options wraps ``::boost::multiprecision::min(…)`` or `std::min(…) <https://en.cppreference.com/w/cpp/numeric/math/min>`__ function.)""");
		py::def("sgn",
		        static_cast<int (*)(const RealHP<N>&)>(&::yade::math::sgn),
		        (py::arg("x")),
		        R"""(:return: ``int`` the sign of the argument: ``-1``, ``0`` or ``1``.)""");
		py::def("sign",
		        static_cast<int (*)(const RealHP<N>&)>(&::yade::math::sign),
		        (py::arg("x")),
		        R"""(:return: ``int`` the sign of the argument: ``-1``, ``0`` or ``1``.)""");
		py::def("floor",
		        static_cast<RealHP<N> (*)(const RealHP<N>&)>(&::yade::math::floor),
		        (py::arg("x")),
		        R"""(:return: ``Real`` Computes the largest integer value not greater than arg. Depending on compilation options wraps ``::boost::multiprecision::floor(…)`` or `std::floor(…) <https://en.cppreference.com/w/cpp/numeric/math/floor>`__ function.)""");
		py::def("ceil",
		        static_cast<RealHP<N> (*)(const RealHP<N>&)>(&::yade::math::ceil),
		        (py::arg("x")),
		        R"""(:return: ``Real`` Computes the smallest integer value not less than arg. Depending on compilation options wraps ``::boost::multiprecision::ceil(…)`` or `std::ceil(…) <https://en.cppreference.com/w/cpp/numeric/math/ceil>`__ function.)""");
		py::def("round",
		        static_cast<RealHP<N> (*)(const RealHP<N>&)>(&::yade::math::round),
		        (py::arg("x")),
		        R"""(:return: ``Real`` the nearest integer value to arg (in floating-point format), rounding halfway cases away from zero, regardless of the current rounding mode.. Depending on compilation options wraps ``::boost::multiprecision::round(…)`` or `std::round(…) <https://en.cppreference.com/w/cpp/numeric/math/round>`__ function.)""");
		py::def("rint",
		        static_cast<RealHP<N> (*)(const RealHP<N>&)>(&::yade::math::rint),
		        (py::arg("x")),
		        R"""(:return: Rounds the floating-point argument arg to an integer value (in floating-point format), using the `current rounding mode <https://en.cppreference.com/w/cpp/numeric/fenv/FE_round>`__. Depending on compilation options wraps ``::boost::multiprecision::rint(…)`` or `std::rint(…) <https://en.cppreference.com/w/cpp/numeric/math/rint>`__ function.)""");
		py::def("trunc",
		        static_cast<RealHP<N> (*)(const RealHP<N>&)>(&::yade::math::trunc),
		        (py::arg("x")),
		        R"""(:return: ``Real`` the nearest integer not greater in magnitude than arg. Depending on compilation options wraps ``::boost::multiprecision::trunc(…)`` or `std::trunc(…) <https://en.cppreference.com/w/cpp/numeric/math/trunc>`__ function.)""");
#ifndef YADE_IGNORE_IEEE_INFINITY_NAN
		py::def("isnan",
		        static_cast<bool (*)(const RealHP<N>&)>(&::yade::math::isnan),
		        (py::arg("x")),
		        R"""(:return: ``bool`` indicating if the ``Real`` argument is NaN. Depending on compilation options wraps ``::boost::multiprecision::isnan(…)`` or `std::isnan(…) <https://en.cppreference.com/w/cpp/numeric/math/isnan>`__ function.)""");
		py::def("isinf",
		        static_cast<bool (*)(const RealHP<N>&)>(&::yade::math::isinf),
		        (py::arg("x")),
		        R"""(:return: ``bool`` indicating if the ``Real`` argument is Inf. Depending on compilation options wraps ``::boost::multiprecision::isinf(…)`` or `std::isinf(…) <https://en.cppreference.com/w/cpp/numeric/math/isinf>`__ function.)""");
		py::def("isfinite",
		        static_cast<bool (*)(const RealHP<N>&)>(&::yade::math::isfinite),
		        (py::arg("x")),
		        R"""(:return: ``bool`` indicating if the ``Real`` argument is Inf. Depending on compilation options wraps ``::boost::multiprecision::isfinite(…)`` or `std::isfinite(…) <https://en.cppreference.com/w/cpp/numeric/math/isfinite>`__ function.)""");
		py::scope().attr("hasInfinityNan") = true;
#else
		py::scope().attr("hasInfinityNan") = false;
#endif


		/********************************************************************************************/
		/**********************        integer division and remainder          **********************/
		/********************************************************************************************/
		py::def("fmod",
		        static_cast<RealHP<N> (*)(const RealHP<N>&, const RealHP<N>&)>(&::yade::math::fmod),
		        (py::arg("x"), "y"),
		        R"""(:return: ``Real`` the floating-point remainder of the division operation ``x/y`` of the arguments ``x`` and ``y``. Depending on compilation options wraps ``::boost::multiprecision::fmod(…)`` or `std::fmod(…) <https://en.cppreference.com/w/cpp/numeric/math/fmod>`__ function.)""");
		py::def("remainder",
		        static_cast<RealHP<N> (*)(const RealHP<N>&, const RealHP<N>&)>(&::yade::math::remainder),
		        (py::arg("x"), "y"),
		        R"""(:return: ``Real`` the IEEE remainder of the floating point division operation ``x/y``. Depending on compilation options wraps ``::boost::multiprecision::remainder(…)`` or `std::remainder(…) <https://en.cppreference.com/w/cpp/numeric/math/remainder>`__ function.)""");
		py::def("modf",
		        test_modf<N>,
		        (py::arg("x")),
		        R"""(:return: tuple of ``(Real,Real)``, decomposes given floating point ``Real`` into integral and fractional parts, each having the same type and sign as x. Depending on compilation options wraps ``::boost::multiprecision::modf(…)`` or `std::modf(…) <https://en.cppreference.com/w/cpp/numeric/math/modf>`__ function.)""");
		py::def("fma",
		        static_cast<RealHP<N> (*)(const RealHP<N>&, const RealHP<N>&, const RealHP<N>&)>(&::yade::math::fma),
		        (py::arg("x"), "y", "z"),
		        R"""(:return: ``Real`` - computes ``(x*y) + z`` as if to infinite precision and rounded only once to fit the result type. Depending on compilation options wraps ``::boost::multiprecision::fma(…)`` or `std::fma(…) <https://en.cppreference.com/w/cpp/numeric/math/fma>`__ function.)""");
		py::def("remquo",
		        test_remquo<N>,
		        (py::arg("x"), "y"),
		        R"""(:return: tuple of ``(Real,long)``, the floating-point remainder of the division operation ``x/y`` as the std::remainder() function does. Additionally, the sign and at least the three of the last bits of ``x/y`` are returned, sufficient to determine the octant of the result within a period. Depending on compilation options wraps ``::boost::multiprecision::remquo(…)`` or `std::remquo(…) <https://en.cppreference.com/w/cpp/numeric/math/remquo>`__ function.)""");


		/********************************************************************************************/
		/**********************         special mathematical functions         **********************/
		/********************************************************************************************/
		// remember that complex functions must be registered first.
		// The 'if constexpr(…)' is not working for C++14. So I have to simulate it using templates. Otherwise the python 'import >>thisModule<<' is really slow.
		IfConstexprForSlowFunctions<N, (N <= skipSlowFunctionsAbove_N)>::work(scopeHP);

		/********************************************************************************************/
		/**********************        extract C-array from std::vector        **********************/
		/********************************************************************************************/
		py::def("testArray",
		        ::yade::testArray<N>,
		        R"""(This function tests call to ``std::vector::data(…)`` function in order to extract the array.)""");

		/********************************************************************************************/
		/**********************              other C++ side tests              **********************/
		/********************************************************************************************/
		py::def("testConstants",
		        ::yade::testConstants<N>,
		        R"""(This function tests lib/high-precision/Constants.hpp, the yade::math::ConstantsHP<N>, former yade::Mathr constants.)""");


		/********************************************************************************************/
		/**********************                     random                     **********************/
		/********************************************************************************************/
		// the random functions are exported for tests as a part of Eigen numerical traits, see below


		/********************************************************************************************/
		/**********************            Eigen numerical traits              **********************/
		/********************************************************************************************/
		long defprec  = std::numeric_limits<RealHP<N>>::digits;
		long max_exp2 = std::numeric_limits<RealHP<N>>::max_exponent;

		py::scope().attr("defprec")  = defprec;
		py::scope().attr("max_exp2") = max_exp2;

		// it would work if this enum had a name.
		//py::enum_<Eigen::NumTraits<RealHP<N>>::EnumName>("traits").value("IsInteger",Eigen::NumTraits<Real>::IsInteger).export_values();

		py::scope().attr("IsInteger")             = int(Eigen::NumTraits<RealHP<N>>::IsInteger);
		py::scope().attr("IsSigned")              = int(Eigen::NumTraits<RealHP<N>>::IsSigned);
		py::scope().attr("IsComplex")             = int(Eigen::NumTraits<RealHP<N>>::IsComplex);
		py::scope().attr("RequireInitialization") = int(Eigen::NumTraits<RealHP<N>>::RequireInitialization);
		py::scope().attr("ReadCost")              = int(Eigen::NumTraits<RealHP<N>>::ReadCost);
		py::scope().attr("AddCost")               = int(Eigen::NumTraits<RealHP<N>>::AddCost);
		py::scope().attr("MulCost")               = int(Eigen::NumTraits<RealHP<N>>::MulCost);
		py::scope().attr("ComplexReadCost")       = int(Eigen::NumTraits<ComplexHP<N>>::ReadCost);
		py::scope().attr("ComplexAddCost")        = int(Eigen::NumTraits<ComplexHP<N>>::AddCost);
		py::scope().attr("ComplexMulCost")        = int(Eigen::NumTraits<ComplexHP<N>>::MulCost);

		//#if defined(YADE_EIGEN_NUM_TRAITS_HPP) or defined(EIGEN_MPREALSUPPORT_MODULE_H)
		// The 'if constexpr(…)' is not working for C++14. So I have to simulate it using templates. Otherwise compiler will try to access non-existing types.
		IfConstexprForEigen<N, (std::numeric_limits<RealHP<N>>::digits >= std::numeric_limits<long double>::digits)>::work(scopeHP, defprec);

		py::def("dummy_precision",
		        Eigen::NumTraits<RealHP<N>>::dummy_precision,
		        R"""(:return: similar to the function ``epsilon``, but assumes that last 10% of bits contain the numerical error only. This is sometimes used by Eigen when calling ``isEqualFuzzy`` to determine if values differ a lot or if they are vaguely close to each other.)""");

		py::def("random",
		        static_cast<RealHP<N> (*)()>(&Eigen::internal::random<RealHP<N>>),
		        R"""(:return: ``Real`` a symmetric random number in interval ``(-1,1)``. Used by Eigen.)""");
		py::def("random",
		        static_cast<RealHP<N> (*)(const RealHP<N>&, const RealHP<N>&)>(&Eigen::internal::random<RealHP<N>>),
		        (py::arg("a"), "b"),
		        R"""(:return: ``Real`` a random number in interval ``(a,b)``. Used by Eigen.)""");

#if EIGEN_VERSION_AT_LEAST(3, 3, 0)
		py::def("isMuchSmallerThan",
		        static_cast<bool (*)(const RealHP<N>&, const RealHP<N>&, const RealHP<N>&)>(&Eigen::internal::isMuchSmallerThan),
		        (py::arg("a"), "b", "eps"),
		        R"""(:return: ``bool``, True if ``a`` is less than ``b`` with provided ``eps``, see also `here <https://stackoverflow.com/questions/15051367/how-to-compare-vectors-approximately-in-eigen>`__)""");
		py::def("isApprox",
		        static_cast<bool (*)(const RealHP<N>&, const RealHP<N>&, const RealHP<N>&)>(&Eigen::internal::isApprox),
		        (py::arg("a"), "b", "eps"),
		        R"""(:return: ``bool``, True if ``a`` is approximately equal ``b`` with provided ``eps``, see also `here <https://stackoverflow.com/questions/15051367/how-to-compare-vectors-approximately-in-eigen>`__)""");
		py::def("isApproxOrLessThan",
		        static_cast<bool (*)(const RealHP<N>&, const RealHP<N>&, const RealHP<N>&)>(&Eigen::internal::isApproxOrLessThan),
		        (py::arg("a"), "b", "eps"),
		        R"""(:return: ``bool``, True if ``a`` is approximately less than or equal ``b`` with provided ``eps``, see also `here <https://stackoverflow.com/questions/15051367/how-to-compare-vectors-approximately-in-eigen>`__)""");
#else
		// older eigen 3.2 didn't use `const Real&` but was copying third argument by value `Real`
		py::def("isMuchSmallerThan",
		        static_cast<bool (*)(const RealHP<N>&, const RealHP<N>&, RealHP<N>)>(&Eigen::internal::isMuchSmallerThan<RealHP<N>, RealHP<N>>),
		        (py::arg("a"), "b", "eps"),
		        R"""(:return: ``bool``, True if ``a`` is less than ``b`` with provided ``eps``, see also `here <https://stackoverflow.com/questions/15051367/how-to-compare-vectors-approximately-in-eigen>`__)""");
		py::def("isApprox",
		        static_cast<bool (*)(const RealHP<N>&, const RealHP<N>&, RealHP<N>)>(&Eigen::internal::isApprox<RealHP<N>>),
		        (py::arg("a"), "b", "eps"),
		        R"""(:return: ``bool``, True if ``a`` is approximately equal ``b`` with provided ``eps``, see also `here <https://stackoverflow.com/questions/15051367/how-to-compare-vectors-approximately-in-eigen>`__)""");
		py::def("isApproxOrLessThan",
		        static_cast<bool (*)(const RealHP<N>&, const RealHP<N>&, RealHP<N>)>(&Eigen::internal::isApproxOrLessThan<RealHP<N>>),
		        (py::arg("a"), "b", "eps"),
		        R"""(:return: ``bool``, True if ``a`` is approximately less than or equal ``b`` with provided ``eps``, see also `here <https://stackoverflow.com/questions/15051367/how-to-compare-vectors-approximately-in-eigen>`__)""");
#endif

		py::def("toLongDouble",
		        static_cast<long double (*)(const RealHP<N>&)>(&Eigen::internal::cast<RealHP<N>, long double>),
		        (py::arg("x")),
		        R"""(:return: ``float`` converts ``Real`` type to ``long double`` and returns a native python ``float``.)""");
		py::def("toDouble",
		        static_cast<double (*)(const RealHP<N>&)>(&Eigen::internal::cast<RealHP<N>, double>),
		        (py::arg("x")),
		        R"""(:return: ``float`` converts ``Real`` type to ``double`` and returns a native python ``float``.)""");
		py::def("toLong",
		        static_cast<long (*)(const RealHP<N>&)>(&Eigen::internal::cast<RealHP<N>, long>),
		        (py::arg("x")),
		        R"""(:return: ``int`` converts ``Real`` type to ``long int`` and returns a native python ``int``.)""");
		py::def("toInt",
		        static_cast<int (*)(const RealHP<N>&)>(&Eigen::internal::cast<RealHP<N>, int>),
		        (py::arg("x")),
		        R"""(:return: ``int`` converts ``Real`` type to ``int`` and returns a native python ``int``.)""");
		py::def("roundTrip",
		        roundTrip<N>,
		        (py::arg("x")),
		        R"""(:return: ``Real`` returns the argument ``x``. Can be used to convert type to native RealHP<N> accuracy.)""");


		/********************************************************************************************/
		/**********************             CGAL numerical traits              **********************/
		/********************************************************************************************/
#ifdef YADE_CGAL
		py::scope().attr("testCgalNumTraits") = true;
		// https://doc.cgal.org/latest/Algebraic_foundations/group__PkgAlgebraicFoundationsRef.html
		py::def("CGAL_Is_valid",
		        TestCGAL::Is_valid<N>,
		        (py::arg("x")),
		        R"""(
CGAL's function ``Is_valid``, as described in `CGAL algebraic <https://doc.cgal.org/latest/Algebraic_foundations/index.html>`__
`foundations <https://doc.cgal.org/latest/Algebraic_foundations/group__PkgAlgebraicFoundationsRef.html>`__ :ysrc:`exposed<lib/high-precision/CgalNumTraits.hpp>`
to python for :ysrccommit:`testing<ff600a80018d21c03626c720cda08967b043c1c8/py/tests/testMath.py#L207>` of CGAL numerical traits.

:return: ``bool`` indicating if the ``Real`` argument is valid. Checks are performed against NaN and Inf.
)""");
		// AlgebraicStructureTraits
		py::def("CGAL_Square",
		        TestCGAL::Square<N>,
		        (py::arg("x")),
		        R"""(
CGAL's function ``Square``, as described in `CGAL algebraic <https://doc.cgal.org/latest/Algebraic_foundations/index.html>`__
`foundations <https://doc.cgal.org/latest/Algebraic_foundations/group__PkgAlgebraicFoundationsRef.html>`__ :ysrc:`exposed<lib/high-precision/CgalNumTraits.hpp>`
to python for :ysrccommit:`testing<ff600a80018d21c03626c720cda08967b043c1c8/py/tests/testMath.py#L207>` of CGAL numerical traits.

:return: ``Real`` the argument squared.
)""");
		py::def("CGAL_Sqrt",
		        TestCGAL::Sqrt<N>,
		        (py::arg("x")),
		        R"""(
CGAL's function ``Sqrt``, as described in `CGAL algebraic <https://doc.cgal.org/latest/Algebraic_foundations/index.html>`__
`foundations <https://doc.cgal.org/latest/Algebraic_foundations/group__PkgAlgebraicFoundationsRef.html>`__ :ysrc:`exposed<lib/high-precision/CgalNumTraits.hpp>`
to python for :ysrccommit:`testing<ff600a80018d21c03626c720cda08967b043c1c8/py/tests/testMath.py#L207>` of CGAL numerical traits.

:return: ``Real`` the square root of argument.
)""");
		py::def("CGAL_Kth_root",
		        TestCGAL::K_root<N>,
		        (py::arg("x")),
		        R"""(
CGAL's function ``Kth_root``, as described in `CGAL algebraic <https://doc.cgal.org/latest/Algebraic_foundations/index.html>`__
`foundations <https://doc.cgal.org/latest/Algebraic_foundations/group__PkgAlgebraicFoundationsRef.html>`__ :ysrc:`exposed<lib/high-precision/CgalNumTraits.hpp>`
to python for :ysrccommit:`testing<ff600a80018d21c03626c720cda08967b043c1c8/py/tests/testMath.py#L207>` of CGAL numerical traits.

:return: ``Real`` the k-th root of argument.
)""");
		// RealEmbeddableTraits
		py::def("CGAL_To_interval",
		        TestCGAL::To_interval<N>,
		        (py::arg("x")),
		        R"""(
CGAL's function ``To_interval``, as described in `CGAL algebraic <https://doc.cgal.org/latest/Algebraic_foundations/index.html>`__
`foundations <https://doc.cgal.org/latest/Algebraic_foundations/group__PkgAlgebraicFoundationsRef.html>`__ :ysrc:`exposed<lib/high-precision/CgalNumTraits.hpp>`
to python for :ysrccommit:`testing<ff600a80018d21c03626c720cda08967b043c1c8/py/tests/testMath.py#L207>` of CGAL numerical traits.

:return: ``(double,double)`` tuple inside which the high-precision ``Real`` argument resides.
)""");
		py::def("CGAL_Sgn",
		        TestCGAL::Sgn<N>,
		        (py::arg("x")),
		        R"""(
CGAL's function ``Sgn``, as described in `CGAL algebraic <https://doc.cgal.org/latest/Algebraic_foundations/index.html>`__
`foundations <https://doc.cgal.org/latest/Algebraic_foundations/group__PkgAlgebraicFoundationsRef.html>`__ :ysrc:`exposed<lib/high-precision/CgalNumTraits.hpp>`
to python for :ysrccommit:`testing<ff600a80018d21c03626c720cda08967b043c1c8/py/tests/testMath.py#L207>` of CGAL numerical traits.

:return: sign of the argument, can be ``-1``, ``0`` or ``1``. Not very useful in python. In C++ it is useful to obtain a sign of an expression with exact accuracy, CGAL starts using MPFR internally for this when the approximate interval contains zero inside it.
)""");
		py::def("CGAL_Is_finite",
		        TestCGAL::Is_finite<N>,
		        (py::arg("x")),
		        R"""(
CGAL's function ``Is_finite``, as described in `CGAL algebraic <https://doc.cgal.org/latest/Algebraic_foundations/index.html>`__
`foundations <https://doc.cgal.org/latest/Algebraic_foundations/group__PkgAlgebraicFoundationsRef.html>`__ :ysrc:`exposed<lib/high-precision/CgalNumTraits.hpp>`
to python for :ysrccommit:`testing<ff600a80018d21c03626c720cda08967b043c1c8/py/tests/testMath.py#L207>` of CGAL numerical traits.

:return: ``bool`` indicating if the ``Real`` argument is finite.
)""");
		py::def("CGAL_simpleTest",
		        ::yade::simpleCgalNumTraitsCalculation<N>,
		        R"""(
Tests a simple CGAL calculation. Distance between plane and point, uses CGAL's sqrt and pow.

:return: 3.0
)""");
#else
		py::scope().attr("testCgalNumTraits") = false;
#endif
	}
};

BOOST_PYTHON_MODULE(_math)
try {
	YADE_SET_DOCSTRING_OPTS;

	if ((::yade::math::RealHPConfig::getDigits10(1) >= 18) or (::yade::math::levelOrZero<double> == 0)) {
		std_pair_to_python_converter<double, double>();
	}
	// this loop registers all python functions from range defined in YADE_EIGENCGAL_HP, file lib/high-precision/RealHPEigenCgal.hpp
	// Some functions for large N are extremely slow during python 'import yade.math', so they are not registered, see struct IfConstexprForSlowFunctions
	::yade::math::detail::registerLoopForHPn<::yade::math::RealHPConfig::SupportedByMinieigen, RegisterRealHPMath>();

	expose_storage_ordering();
	exposeRealHPDiagnostics();

	::yade::math::RealHPConfig::pyRegister();

} catch (...) {
	LOG_FATAL("Importing this module caused an exception and this module is in an inconsistent state now.");
	PyErr_Print();
	PyErr_SetString(PyExc_SystemError, __FILE__); // raising anything other than SystemError is not possible
	boost::python::handle_exception();
	throw;
}

// The header UpconversionOfBasicOperatorsHP has to be tested, but it must be included below everything, because otherwise it would pollute other code. So test it here.
#include <lib/high-precision/UpconversionOfBasicOperatorsHP.hpp>
#include <boost/core/demangle.hpp>
#include <boost/lexical_cast.hpp>

namespace yade {

template <int N1> struct TestRealHP2 {
	template <typename N2mpl> void operator()(N2mpl)
	{
		constexpr int N2   = N2mpl::value;
		constexpr int N    = std::max(N1, N2);
		std::string   info = " N1=" + boost::lexical_cast<std::string>(N1) + " N2=" + boost::lexical_cast<std::string>(N2) + " "
		        + boost::core::demangle(typeid(RealHP<N1>).name()) + " " + boost::core::demangle(typeid(RealHP<N2>).name()) + " "
		        + boost::core::demangle(typeid(ComplexHP<N1>).name()) + " " + boost::core::demangle(typeid(ComplexHP<N2>).name());
		//LOG_NOFILTER("TestRealHP info:" << info);
		{
			RealHP<N1> a  = static_cast<RealHP<N1>>(-1.25);
			RealHP<N2> b  = static_cast<RealHP<N2>>(2.5);
			auto       c1 = a + b;
			auto       c2 = a - b;
			auto       c3 = a * b;
			auto       c4 = a / b;
			if (c1 != RealHP<N>(1.25)) // NOTE: might want later to replace these if with BOOST_ASSERT
				throw std::runtime_error(("TestRealHP error: Fatal r1" + info).c_str());
			if (c2 != RealHP<N>(-3.75))
				throw std::runtime_error(("TestRealHP error: Fatal r2" + info).c_str());
			if (c3 != RealHP<N>(-3.125))
				throw std::runtime_error(("TestRealHP error: Fatal r3" + info).c_str());
			if (c4 != RealHP<N>(-0.5))
				throw std::runtime_error(("TestRealHP error: Fatal r4" + info).c_str());
			auto       d1 = a;
			auto       d2 = b;
			RealHP<N2> d3 = RealHP<N2>(a);
			RealHP<N1> d4 = RealHP<N1>(b);
			auto       d5 = d1 + d2 + d3 + d4;
			if (d5 != RealHP<N>(2.5))
				throw std::runtime_error(("TestRealHP error: Fatal r5" + info).c_str());
		}
		{
			ComplexHP<N1> a  = ComplexHP<N1>(-1.25, 0.5);
			ComplexHP<N2> b  = ComplexHP<N2>(1.0, 1.0);
			auto          c1 = a + b;
			auto          c2 = a - b;
			auto          c3 = a * b;
			auto          c4 = a / b;
			if (c1 != ComplexHP<N>(-0.25, 1.5))
				throw std::runtime_error(("TestRealHP error: Fatal c1" + info).c_str());
			if (c2 != ComplexHP<N>(-2.25, -0.5))
				throw std::runtime_error(("TestRealHP error: Fatal c2" + info).c_str());
			if (c3 != ComplexHP<N>(-1.75, -0.75))
				throw std::runtime_error(("TestRealHP error: Fatal c3" + info).c_str());
			if (c4 != ComplexHP<N>(-0.375, 0.875))
				throw std::runtime_error(("TestRealHP error: Fatal c4" + info).c_str());
			auto d1 = a;
			auto d2 = b;
			// down-converting requires extra casting
			ComplexHP<N2> d3 = ComplexHP<N2>(RealHP<N2>(a.real()), RealHP<N2>(a.imag()));
			ComplexHP<N1> d4 = ComplexHP<N1>(RealHP<N1>(b.real()), RealHP<N1>(b.imag()));
			auto          d5 = d1 + d2 + d3 + d4;
			if (d5 != ComplexHP<N>(-0.5, 3.0))
				throw std::runtime_error(("TestRealHP error: Fatal c5" + info).c_str());
		}
		{
			ComplexHP<N1> a  = ComplexHP<N1>(-1.25, 0.5);
			RealHP<N2>    b  = RealHP<N2>(1.0);
			auto          c1 = a + b;
			auto          c2 = a - b;
			auto          c3 = a * b;
			auto          c4 = a / b;
			auto          c5 = b + a;
			auto          c6 = b - a;
			auto          c7 = b * a;
			auto          c8 = b / a;
			if (c1 != ComplexHP<N>(-0.25, 0.5))
				throw std::runtime_error(("TestRealHP error: Fatal cr1" + info).c_str());
			if (c2 != ComplexHP<N>(-2.25, 0.5))
				throw std::runtime_error(("TestRealHP error: Fatal cr2" + info).c_str());
			if (c3 != ComplexHP<N>(-1.25, 0.5))
				throw std::runtime_error(("TestRealHP error: Fatal cr3" + info).c_str());
			if (c4 != ComplexHP<N>(-1.25, 0.5))
				throw std::runtime_error(("TestRealHP error: Fatal cr4" + info).c_str());
			if (c5 != ComplexHP<N>(-0.25, 0.5))
				throw std::runtime_error(("TestRealHP error: Fatal cr5" + info).c_str());
			if (c6 != ComplexHP<N>(2.25, -0.5))
				throw std::runtime_error(("TestRealHP error: Fatal cr6" + info).c_str());
			if (c7 != ComplexHP<N>(-1.25, 0.5))
				throw std::runtime_error(("TestRealHP error: Fatal cr7" + info).c_str());
			if (::yade::math::abs(c8 - ComplexHP<N>(-0.68965517241379310345, -0.27586206896551724138)) > 0.1)
				throw std::runtime_error(("TestRealHP error: Fatal cr8" + info).c_str());
			auto d1 = a;
			auto d2 = b;
			// down-converting requires extra casting
			ComplexHP<N2> d3 = ComplexHP<N2>(RealHP<N2>(a.real()), RealHP<N2>(a.imag()));
			RealHP<N1>    d4 = RealHP<N1>(b);
			auto          d5 = d1 + d2 + d4 + d3;
			if (d5 != ComplexHP<N>(-0.5, 1.0))
				throw std::runtime_error(("TestRealHP error: Fatal cr9" + info).c_str());
			auto d6 = d1 + d2 + d3 + d4;
			if (d6 != ComplexHP<N>(-0.5, 1.0))
				throw std::runtime_error(("TestRealHP error: Fatal cr10" + info).c_str());

			static_assert(std::is_same<ComplexHP<N>, decltype(c1)>::value, "Assert error c1");
			static_assert(std::is_same<ComplexHP<N>, decltype(c2)>::value, "Assert error c2");
			static_assert(std::is_same<ComplexHP<N>, decltype(c3)>::value, "Assert error c3");
			static_assert(std::is_same<ComplexHP<N>, decltype(c4)>::value, "Assert error c4");
			static_assert(std::is_same<ComplexHP<N>, decltype(c5)>::value, "Assert error c5");
			static_assert(std::is_same<ComplexHP<N>, decltype(c6)>::value, "Assert error c6");
			static_assert(std::is_same<ComplexHP<N>, decltype(c7)>::value, "Assert error c7");
			static_assert(std::is_same<ComplexHP<N>, decltype(c8)>::value, "Assert error c8");
			static_assert(std::is_same<ComplexHP<N1>, decltype(d1)>::value, "Assert error d1");
			static_assert(std::is_same<decltype(b), decltype(d2)>::value, "Assert error d2");
			static_assert(std::is_same<ComplexHP<N2>, decltype(d3)>::value, "Assert error d3");
			static_assert(std::is_same<RealHP<N1>, decltype(d4)>::value, "Assert error d4");
			static_assert(std::is_same<ComplexHP<N>, decltype(d5)>::value, "Assert error d5");
			static_assert(std::is_same<ComplexHP<N>, decltype(d6)>::value, "Assert error d6");
		}
	}
};

struct TestRealHP1 {
	template <typename N1> void operator()(N1)
	{
		boost::mpl::for_each<::yade::math::RealHPConfig::SupportedByEigenCgal>(TestRealHP2<N1::value>());
		testConstants<N1::value>();
	}
};

void testLoopRealHP() { boost::mpl::for_each<::yade::math::RealHPConfig::SupportedByEigenCgal>(TestRealHP1()); }

} // namespace yade
