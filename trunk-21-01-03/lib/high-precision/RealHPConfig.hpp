/*************************************************************************
*  2020 Janek Kozicki                                                    *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#ifndef YADE_REAL_HP_CONFIG_HPP
#define YADE_REAL_HP_CONFIG_HPP

#ifndef YADE_DISABLE_REAL_MULTI_HP
// Declare which precisions will be used in YADE for Eigen, CGAL and for minieigenHP (see file lib/high-precision/RealHPEigenCgal.hpp for details):
//	C++	: YADE_EIGENCGAL_HP  ↔ The numbers listed here will work in C++ for RealHP<N> in CGAL and Eigen. Rather cheap in compilation time.
//	Python	: YADE_MINIEIGEN_HP  ↔ These are exported to python. Expensive: each one makes compilation longer by 1 minute.
#define YADE_EIGENCGAL_HP (1)(2)(3)(4)(8)(10)(20)
#define YADE_MINIEIGEN_HP (1)(2)

// If you are doing some debugging and need more precisions or to access them from minieigenHP, then use e.g. this:
//#define YADE_EIGENCGAL_HP (1)(2)(3)(4)(5)(6)(7)(8)(9)(10)(20)
//#define YADE_MINIEIGEN_HP YADE_EIGENCGAL_HP

#else // if cmake detects problems that RealHP<…> can't work, e.g. with gcc older than v9.2.1
#define YADE_EIGENCGAL_HP (1)
#define YADE_MINIEIGEN_HP (1)
#endif

#define YADE_HP_PARSE_ONE(r, name, levelHP) name(levelHP)
#define YADE_REGISTER_HP_LEVELS(name) BOOST_PP_SEQ_FOR_EACH(YADE_HP_PARSE_ONE, name, YADE_EIGENCGAL_HP) // it just creates: name(1) name(2) name(3) ....
// This macro ↑ is also used in lib/base/AliasCGAL.hpp, that code could be put here, but it would make compilation unnecessarily longer.

#include <lib/high-precision/Real.hpp>
#include <array>
#include <boost/mpl/vector_c.hpp>
#include <boost/preprocessor.hpp>
#include <boost/python.hpp>

namespace yade {
namespace math {
	// contains number of decimal and binary digits during compile-time N of RealHP<N>
	template <int N> struct DigitsHP10 { // must be a struct, so that getDigits<…>(N) will accept it as an argument.
		static inline int value() { return std::numeric_limits<RealHP<N>>::digits10; }
	};
	template <int N> struct DigitsHP2 {
		static inline int value() { return std::numeric_limits<RealHP<N>>::digits; }
	};

	struct RealHPConfig {
		// set how many RealHP<N> types are provided for Eigen, CGAL and Minieigen in file lib/high-precision/RealHPEigenCgal.hpp by YADE_EIGENCGAL_HP , YADE_MINIEIGEN_HP:
		static const constexpr size_t sizeEigenCgal = BOOST_PP_SEQ_SIZE(YADE_EIGENCGAL_HP);
		static const constexpr size_t sizeMinieigen = BOOST_PP_SEQ_SIZE(YADE_MINIEIGEN_HP);

		// make a runtime iterable array (runtime variable names start with lowerCase)
		static const constexpr std::array<int, sizeEigenCgal> supportedByEigenCgal { BOOST_PP_SEQ_ENUM(YADE_EIGENCGAL_HP) };
		static const constexpr std::array<int, sizeMinieigen> supportedByMinieigen { BOOST_PP_SEQ_ENUM(YADE_MINIEIGEN_HP) };

		// make a compile-time iterable mpl::vector_c (type names start with UpperCase)
		using SupportedByEigenCgal = boost::mpl::vector_c<int, BOOST_PP_SEQ_ENUM(YADE_EIGENCGAL_HP)>;
		using SupportedByMinieigen = boost::mpl::vector_c<int, BOOST_PP_SEQ_ENUM(YADE_MINIEIGEN_HP)>;

		// export to python
		static inline boost::python::tuple getSupportedByEigenCgal() { return boost::python::make_tuple(BOOST_PP_SEQ_ENUM(YADE_EIGENCGAL_HP)); }
		static inline boost::python::tuple getSupportedByMinieigen() { return boost::python::make_tuple(BOOST_PP_SEQ_ENUM(YADE_MINIEIGEN_HP)); }

		// returns number of decimal and binary digits for runtime N of RealHP<N>
		template <template <int> class dig> static inline int getDigits(int N)
		{
			switch (N) {
#define CASE_LEVEL_HP(levelHP)                                                                                                                                 \
	case levelHP: return dig<levelHP>::value();
				YADE_REGISTER_HP_LEVELS(CASE_LEVEL_HP)
#undef CASE_LEVEL_HP
				default: return dig<1>::value() * N; // this formula is used by NthLevel in lib/high-precision/RealHP.hpp
			}
		}

		static inline int getDigits10(int N) { return getDigits<DigitsHP10>(N); };
		static inline int getDigits2(int N) { return getDigits<DigitsHP2>(N); };

		// how many extra digits to use when converting to decimal strings
		static int extraStringDigits10;

#ifdef YADE_MPFR
#define YADE_EIGENCGAL_HP_REVERSE BOOST_PP_SEQ_REVERSE(YADE_EIGENCGAL_HP)
		static const constexpr auto workaroundSlowBoostBinFloat = BOOST_PP_SEQ_HEAD(YADE_EIGENCGAL_HP_REVERSE);
#undef YADE_EIGENCGAL_HP_REVERSE
#else
		// boost cpp_bin_float has some problem that importing it in python is very slow when these functions are exported: erf, erfc, lgamma, tgamma
		// python 'import this_module' measured time: workaroundSlowBoostBinFloat==6 → 10min, N==5 → 3m24s, N==4 → 1m55s, N==3 → 1minute23sec
		// the workaround is to make them unavailable in python for higher N values. See invocation of IfConstexprForSlowFunctions in py/high-precision/_math.cpp
#ifndef YADE_DISABLE_REAL_MULTI_HP
		static const constexpr auto workaroundSlowBoostBinFloat = 2;
#else
		static const constexpr auto workaroundSlowBoostBinFloat = 1;
#endif
#endif

		// register this class to python
		static void pyRegister();
	};
} // namespace math
} // namespace yade

#endif
