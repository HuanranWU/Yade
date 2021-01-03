/*************************************************************************
*  2020 Janek Kozicki                                                    *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#include <lib/high-precision/RealHPConfig.hpp>
#include <lib/high-precision/RealIO.hpp>

namespace forCtags {
struct RealHPConfig { // these are directly assigned below in pyRegister(), they are not C++ variables. So let ctags find them here, struct forCtags::RealHPConfig is not used anywhere (except for ctags).
	bool isFloat128Broken;
	bool isEnabledRealHP;
}; // for ctags
}

namespace yade {
namespace math {

	int RealHPConfig::extraStringDigits10 { 4 };
	// https://stackoverflow.com/questions/14395967/proper-initialization-of-static-constexpr-array-in-class-template
	// should not have a (duplicate) initializer in its namespace scope definition
	const constexpr size_t                                       RealHPConfig::sizeEigenCgal;
	const constexpr size_t                                       RealHPConfig::sizeMinieigen;
	const constexpr std::array<int, RealHPConfig::sizeEigenCgal> RealHPConfig::supportedByEigenCgal;
	const constexpr std::array<int, RealHPConfig::sizeMinieigen> RealHPConfig::supportedByMinieigen;

	void RealHPConfig::pyRegister()
	{
		namespace py = ::boost::python;
		py::scope cl = py::class_<RealHPConfig>(
		                       "RealHPConfig",
		                       // The docstrings can use syntax
		                       //  :param ……: ………
		                       //  :return: …….
		                       // For details see https://thomas-cokelaer.info/tutorials/sphinx/docstring_python.html
		                       // docstrings for static properties are forbidden in python. The solution is to put it into __doc__
		                       // https://stackoverflow.com/questions/25386370/docstrings-for-static-properties-in-boostpython
		                       R"""(
``RealHPConfig`` class provides information about RealHP<N> type.

:cvar extraStringDigits10: this static variable allows to control how many extra digits to use when converting to decimal strings. Assign a different value to it to affect the string conversion done in :ysrccommit:`C++ ↔ python conversions <26bffeb7ef4fd0d15e4faa025f68f97381621f04/lib/high-precision/ToFromPythonConverter.hpp#L37>` as well as in :ysrccommit:`all other conversions<26bffeb7ef4fd0d15e4faa025f68f97381621f04/lib/high-precision/RealIO.hpp#L34>`. Be careful, because values smaller than 3 can fail the round trip conversion test.

:cvar isFloat128Broken: provides runtime information if Yade was compiled with g++ version < 9.2.1 and thus ``boost::multiprecision::float128`` cannot work.

:cvar isEnabledRealHP:  provides runtime information ``RealHP<N>`` is available for N higher than 1.

:cvar workaroundSlowBoostBinFloat: ``boost::multiprecision::cpp_bin_float`` has some problem that importing it in python is very slow when these functions are exported: erf, erfc, lgamma, tgamma. In such case the python ``import yade.math`` can take more than minute. The workaround is to make them unavailable in python for higher N values. See invocation of IfConstexprForSlowFunctions in :ysrccommit:`_math.cpp<61fc7f208027344e27dc832052b3f8c911a5909e/py/high-precision/_math.cpp#L672>`. This variable contains the highest N in which these functions are available. It equals to highest N when ``boost::multiprecision::cpp_bin_float`` is not used.

)""")
		                       .add_static_property(
		                               "extraStringDigits10",
		                               py::make_getter(&RealHPConfig::extraStringDigits10, py::return_value_policy<py::return_by_value>()),
		                               py::make_setter(&RealHPConfig::extraStringDigits10, py::return_value_policy<py::return_by_value>())
		                               // python docstrings for static variables have to be written inside :cvar ………: in __doc__ of a class. See above.
		                       );
		py::def("getSupportedByEigenCgal",
		        getSupportedByEigenCgal,
		        R"""(:return: the ``tuple`` containing ``N`` from ``RealHP<N>`` precisions supported by Eigen and CGAL)""");
		py::def("getSupportedByMinieigen",
		        getSupportedByMinieigen,
		        R"""(:return: the ``tuple`` containing ``N`` from ``RealHP<N>`` precisions supported by minieigenHP)""");
		py::def("getDigits10", getDigits10, (py::arg("N")), R"""(
This is a yade.math.RealHPConfig diagnostic function.

:param N: ``int`` - the value of ``N`` in ``RealHP<N>``.
:return: the ``int`` representing ``std::numeric_limits<RealHP<N>>::digits10``)""");
		py::def("getDigits2", getDigits2, (py::arg("N")), R"""(
This is a yade.math.RealHPConfig diagnostic function.

:param N: ``int`` - the value of ``N`` in ``RealHP<N>``.
:return: the ``int`` representing ``std::numeric_limits<RealHP<N>>::digits``, which corresponds to the number of significand bits used by this type.)""");
#if (__GNUC__ < 9) // It should be checking  if (ver < 9.2.1) in fact. But cmake does the job. So here it's only to catch 'larger' mistakes.
#ifndef YADE_DISABLE_REAL_MULTI_HP
#warning "RealHP<…> won't work on this system, cmake sets YADE_DISABLE_REAL_MULTI_HP to use RealHP<1> for all precisions RealHP<N>. Also you can try -O0 flag."
// see file lib/high-precision/RealHP.hpp line: 'template <int Level> using RealHP    = Real;'
#endif
		// When using gcc older than 9.2.1 it is not possible for RealHP<N> to work. Without optimizations -O0 it can work, except for float128.
		// If YADE_DISABLE_REAL_MULTI_HP is set, then RealHP<1> is used in place of all possible precisions RealHP<N> : see file RealHP.hpp for this setting.
		// So this is for local testing only. With flag -O0 most of RealHP<…> works, except for float128 which is always segfaulting.
		py::scope().attr("isFloat128Broken") = true;
#else
		py::scope().attr("isFloat128Broken") = false;
#endif
#ifndef YADE_DISABLE_REAL_MULTI_HP
		py::scope().attr("isEnabledRealHP") = true;
#else
		py::scope().attr("isEnabledRealHP")  = false;
#endif
		py::scope().attr("workaroundSlowBoostBinFloat") = int(workaroundSlowBoostBinFloat);
	}

} // namespace math
} // namespace yade

/* A bit more about extraStringDigits10:

The extraStringDigits10 is to make sure that there are no conversion errors in the last bit.
here is a quick python example which shows the 'cutting' of last digits.

# This one demonstrates that `double` used by python has just 53 bits of precision:

for a in range(128): print(str(a).rjust(3,' ')+": "+str(1+1./pow(2.,a)))

# This one shows the 'true' values:

import mpmath; mpmath.mp.dps=200;
for a in range(128): print(str(a).rjust(3,' ')+": "+str(mpmath.mpf(1)+mpmath.mpf(1)/pow(mpmath.mpf(2),mpmath.mpf(a))))

# This one shows the actual 'Real' precision used in yade. To achieve this the mth.roundTrip(…) is called to force the numbers
# to pass through C++, instead of letting mpmath to calculate this, so for example we can see that float128 has 113 bits.
# Also this test was used to verify the value for extraStringDigits10 as well as the formula given
# in IEEE Std 754-2019 Standard for Floating-Point Arithmetic: Pmin (bf) = 1 + ceiling( p × log10(2)), where p is the number of significant bits in bf

from yade import math as mth
for a in range(128): print(str(a).rjust(3,' ')+": "+str(mth.roundTrip(mth.roundTrip(1)+mth.roundTrip(1)/mth.pow(mth.roundTrip(2),a))))

# But also we can now check the precision directly by calling

yade.math.RealHPConfig.getDigits2(N) # for any N of RealHP<N>

# FIXED: Hmm maybe turn this into an external parameter? Configurable from python? And write in help "use 1 to 'just' extract results and avoid fake sense of more precision,
# use 4 or more to have numbers which will convert exactly in both directions mpmath ↔ string ↔ C++.".
# For now it is in yade.math.RealHPConfig.extraStringDigits10
*/
