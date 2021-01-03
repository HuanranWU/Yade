/*************************************************************************
*  2020 Janek Kozicki                                                    *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#include "_RealHPDiagnostics.hpp"
#include <lib/base/Logging.hpp>
#include <lib/high-precision/Real.hpp>
#include <lib/high-precision/RealHPConfig.hpp>
#include <lib/high-precision/RealIO.hpp>
#include <algorithm>
#include <bitset>
#include <boost/core/demangle.hpp>
#include <boost/math/special_functions/next.hpp>
#include <boost/mpl/reverse.hpp>
#include <boost/predef/other/endian.h>
#include <boost/python.hpp>
#include <boost/python/stl_iterator.hpp>
#include <boost/range/adaptor/reversed.hpp>
#include <map>
#include <set>

#include <lib/high-precision/ToFromPythonConverter.hpp>

CREATE_CPP_LOCAL_LOGGER("_RealHPDiagnostics.cpp")

namespace py = ::boost::python;

namespace yade {
// DecomposedReal contains an arbitrary floating point type decomposed into bits and other useful debug information.
// It is written to allow direct bit-per-bit comparison of two numbers to find out how many of the least significant bits are different.
// Maybe this class will find more use in the future and get moved to some .hpp
class DecomposedReal {
private:
	int                        sig;
	int                        exp;
	std::vector<unsigned char> bits;
	bool                       bad { true };
	bool                       fpInf { false };
	bool                       fpNan { false };
	bool                       fpNormal { false };
	bool                       fpSubNormal { false };
	bool                       fpZero { false };
	int                        fpClassify { 0 }; // https://en.cppreference.com/w/cpp/numeric/math/fpclassify
	int                        levelOfHP { -1 };
	std::string                demangledType {};
	std::string                whatEntered {};

	// this function is only because I discovered a bug in frexp function for multiprecision MPFR
	template <typename Rr> inline void checkNormExp(Rr& norm, int& ex, const Rr& x)
	{
		if (((norm < 0.5) or (norm >= 1)) and (not fpZero) and (norm != 0) and math::isfinite(norm)) {
			// TODO: check if latest MPFR version fixed bug in frexp for arguments such as:
			// -0.999999999999999999999999778370088952043740344504867972909872539207515981285392968364222963125347205961928119872988781594713043198125331291
			// and if not then file a bug report.
			static bool warnOnce = true;
			if (warnOnce) {
				LOG_WARN(
				        "frexp(…) documentation says that the result is always >= 0.5 and < 1.0. But here frexp("
				        << math::toStringHP(x) << ",&ex) == " << math::toStringHP(norm) << ", please file a bug report against the "
				        << demangledType << "\n Not sure if this bug is in multiprecision or in MPFR, or somewhere else.");
				LOG_WARN("Trying to fix this.");
				LOG_WARN("Will warn about this problem only once.");
				warnOnce = false;
			}
			if ((norm > 0.25) and (norm < 0.5)) { // if this bug becomes more annoying this fix might need to get moved into MathFunctions.hpp
				norm *= 2;
				ex -= 1;
			} else {
				bad = true;
				LOG_FATAL("Failed to fix frexp function, norm = " << math::toStringHP(norm) << " ex = " << ex);
				throw std::runtime_error("Failed to fix frexp; got norm = " + math::toStringHP(norm));
			}
		}
	}

public:
	DecomposedReal()
	        : bad(true)
	{
	}
	bool wrong() const { return (bits.empty() or abs(sig) > 1 or bad); }
	// don't call rebuild() when wrong() == true
	template <typename Rr> Rr rebuild() const
	{
		if (wrong())
			throw std::runtime_error("DecomposedReal::rebuild got wrong() data.");
		return rebuild<Rr>(this->bits, this->exp, this->sig);
	}
	template <typename Rr> static Rr rebuild(const std::vector<unsigned char>& bits, const int& exp, const int& sig)
	{
		Rr  ret = 0;
		int i   = 0;
		for (auto c : bits) {
			if (c == 1) {
				ret += pow(static_cast<Rr>(2), static_cast<Rr>(exp - i));
			} else if (c != 0) {
				throw std::runtime_error("error: value different than '0' or '1' encountered.");
			}
			++i;
		}
		return ret * static_cast<Rr>(sig);
	}

	template <typename Rr> DecomposedReal(const Rr& x)
	{
		this->levelOfHP = math::levelOfHP<Rr>; // this line will fail if Rr is not a RealHP type.
		fpInf           = math::isinf(x);
		fpNan           = math::isnan(x);
		fpClassify      = math::fpclassify(x);
		fpNormal        = (fpClassify == FP_NORMAL);
		fpSubNormal     = (fpClassify == FP_SUBNORMAL);
		fpZero          = (fpClassify == FP_ZERO);
		if ((fpInf != (fpClassify == FP_INFINITE)) or (fpNan != (fpClassify == FP_NAN))) {
			bad = true;
		}
		demangledType = boost::core::demangle(typeid(Rr).name());
		whatEntered   = math::toStringHP(x);
		bad           = (not math::isfinite(x)); // NaN or Inf
		int ex        = 0;
		Rr  norm      = math::frexp(math::abs(x), &ex);
		checkNormExp(norm, ex, x); // may set bad=true;
		if (bad)
			return;
		sig     = math::sign(x);
		exp     = ex - 1;
		ex      = 0;
		int pos = 0;
		bits.resize(std::numeric_limits<Rr>::digits, 0);
		while (norm != 0) {
			pos -= ex;
			if ((ex <= 0) and (pos < int(bits.size())) and (pos >= 0) and (not bad)) {
				bits[pos] = 1;
				norm -= static_cast<Rr>(0.5);
				norm = math::frexp(norm, &ex);
				checkNormExp(norm, ex, x);
			} else {
				LOG_FATAL("DecomposedReal construction error, x=" << x);
				bad = true;
				return;
			}
		};
		bad = false;
		if (x != rebuild<Rr>()) {
			throw std::runtime_error("DecomposedReal rebuild error.");
		}
	}
	template <typename Rr = double> void     print() const { std::cout << py::extract<std::string>(py::str(getDict<Rr>()))() << "\n"; }
	template <typename Rr = double> py::dict getDict() const
	{
		py::dict          ret {};
		std::stringstream out {};
		for (auto c : bits)
			out << int(c);
		if (not wrong()) {
			ret["reconstructed"] = rebuild<Rr>();
		} else { // create None when reconstrucion is not possible, https://www.boost.org/doc/libs/1_42_0/libs/python/doc/v2/object.html#object-spec-ctors
			ret["reconstructed"] = py::object();
		}
		ret["sign"]        = sig;
		ret["exp"]         = exp;
		ret["bits"]        = out.str();
		ret["wrong"]       = wrong();
		ret["fpInf"]       = fpInf;
		ret["fpNan"]       = fpNan;
		ret["fpNormal"]    = fpNormal;
		ret["fpSubNormal"] = fpSubNormal;
		ret["fpZero"]      = fpZero;
		ret["fpClassify"]  = fpClassify;
		ret["type"]        = demangledType;
		ret["input"]       = whatEntered;
		return ret;
	}

	// this function checks that the two arguments are "very equal" to each other.
	// Meaning that: all bits are equal and all remaining bits are zero.
	// NOTE - this function is a bit excessive. A simple static_cast would suffice. It's just to be sure. Might get removed later.
	bool veryEqual(const DecomposedReal& maxPrec) const
	{
		if (this->bits.size() > maxPrec.bits.size())
			throw std::runtime_error("DecomposedReal::veryEqual - the argument maxPrec should have higher precision");
		if ((this->sig != maxPrec.sig) or (this->exp != maxPrec.exp))
			return false;
		size_t i = 0;
		for (; i < maxPrec.bits.size(); ++i) {
			if ((i < this->bits.size()) and (this->bits[i] != maxPrec.bits[i]))
				return false;
			if ((i >= this->bits.size()) and (maxPrec.bits[i] != 0))
				return false;
		}
		return true;
	}
	template <typename A, typename B> static void veryEqual(const A& a, const B& b)
	{
		if (not DecomposedReal(a).veryEqual(DecomposedReal(b)))
			throw std::runtime_error("DecomposedReal::veryEqual error " + math::toStringHP(a) + " vs. " = math::toStringHP(b));
	}
};

bool isThisSystemLittleEndian()
{
#if BOOST_ENDIAN_BIG_BYTE
	return false;
#elif BOOST_ENDIAN_LITTLE_BYTE
	return true;
#else
	throw std::runtime_error("Unknown endian type, maybe BOOST_ENDIAN_BIG_WORD or BOOST_ENDIAN_LITTLE_WORD ? See file boost/predef/other/endian.h");
#endif
}

template <int N> std::string      getDemangledName() { return boost::core::demangle(typeid(RealHP<N>).name()); };
template <int N> py::dict         getDecomposedReal(const RealHP<N>& arg) { return DecomposedReal(arg).getDict<RealHP<N>>(); }
template <int N, int M> RealHP<M> toHP(const RealHP<N>& arg) { return static_cast<RealHP<M>>(arg); };

template <int N, int M> void registerHPnHPm()
{
	std::string strN = boost::lexical_cast<std::string>(N);
	std::string strM = boost::lexical_cast<std::string>(M);
	py::def(("toHP" + strM).c_str(),
	        static_cast<RealHP<M> (*)(const RealHP<N>&)>(&toHP<N, M>),
	        (py::arg("x")),
	        (":return: ``RealHP<" + strM + ">`` converted from argument ``RealHP<" + strN + ">`` as a result of ``static_cast<RealHP<" + strM
	         + ">>(arg)``.")
	                .c_str());
};

template <int N> std::string getRawBits(const RealHP<N>& arg)
{
	if (math::detail::isNthLevel<RealHP<N>>) {
		LOG_WARN("Warning: RealHP<N> is a non POD type, the raw bits might be a pointer instead of a floating point number.");
	}
	unsigned char buf[sizeof(decltype(arg))];
	std::memcpy(&buf, &arg, sizeof(decltype(arg)));
	std::stringstream out {};
	if (isThisSystemLittleEndian()) {
		// reverse for little endian architecture.
		// https://www.boost.org/doc/libs/1_73_0/libs/endian/doc/html/endian.html
		for (auto c : boost::adaptors::reverse(buf))
			out << std::bitset<8>(c);
	} else {
		for (auto c : buf)
			out << std::bitset<8>(c);
	}
	return out.str();
}

template <int N>::yade::RealHP<N> fromBits(const std::string& str, int exp, int sign)
{
	std::vector<unsigned char> bits {};
	auto                       it = str.begin();
	std::generate_n(std::back_inserter(bits), str.size(), [&it]() -> unsigned char { return static_cast<unsigned char>(*(it++) - '0'); });
	return DecomposedReal::rebuild<RealHP<N>>(bits, exp, sign);
}

namespace math { // a couple extra functions to simplify code below for testing +, -, /, *
	template <typename Rr, int Level = levelOfRealHP<Rr>> inline Rr basicAdd(const Rr& a, const Rr& b)
	{
		return static_cast<const UnderlyingHP<Rr>&>(a) + static_cast<const UnderlyingHP<Rr>&>(b);
	}
	template <typename Rr, int Level = levelOfRealHP<Rr>> inline Rr basicSub(const Rr& a, const Rr& b)
	{
		return static_cast<const UnderlyingHP<Rr>&>(a) - static_cast<const UnderlyingHP<Rr>&>(b);
	}
	template <typename Rr, int Level = levelOfRealHP<Rr>> inline Rr basicDiv(const Rr& a, const Rr& b)
	{
		return static_cast<const UnderlyingHP<Rr>&>(a) / static_cast<const UnderlyingHP<Rr>&>(b);
	}
	template <typename Rr, int Level = levelOfRealHP<Rr>> inline Rr basicMult(const Rr& a, const Rr& b)
	{
		return static_cast<const UnderlyingHP<Rr>&>(a) * static_cast<const UnderlyingHP<Rr>&>(b);
	}
}


template <int N, bool /*registerConverters*/> struct RegisterRealBitDebug {
	static void work(const py::scope& /*topScope*/, const py::scope& scopeHP)
	{
		py::scope HPn(scopeHP);

		py::def("getDemangledName",
		        static_cast<std::string (*)()>(&getDemangledName<N>),
		        R"""(:return: ``string`` - the demangled C++ typnename of ``RealHP<N>``.)""");

		py::def("getDecomposedReal",
		        static_cast<py::dict (*)(const RealHP<N>&)>(&getDecomposedReal<N>),
		        (py::arg("x")),
		        R"""(:return: ``dict`` - the dictionary with the debug information how the DecomposedReal class sees this type. This is for debugging purposes, rather slow. Includes result from `fpclassify <https://en.cppreference.com/w/cpp/numeric/math/fpclassify>`__ function call, a binary representation and other useful info. See also :yref:`fromBits<yade._math.fromBits>`.)""");

// This also could be a boost::mpl::for_each loop (like in registerLoopForHPn) with templates and a helper struct. Not sure which approach is more readable.
#define YADE_REGISTER_MNE_LEVELS(name) BOOST_PP_SEQ_FOR_EACH(YADE_HP_PARSE_ONE, name, YADE_MINIEIGEN_HP) // it just creates: name(1) name(2) name(3) ....
#define REGISTER_CONVERTER_HPn_TO_HPm(levelHP) registerHPnHPm<N, levelHP>();
		YADE_REGISTER_MNE_LEVELS(REGISTER_CONVERTER_HPn_TO_HPm)
#undef REGISTER_CONVERTER_HPn_TO_HPm
#undef YADE_REGISTER_MNE_LEVELS
		py::def("getRawBits",
		        static_cast<std::string (*)(const RealHP<N>&)>(&getRawBits<N>),
		        (py::arg("x")),
		        R"""(:return: ``string`` - the raw bits in memory representing this type. Be careful: it only checks the system endianness and either prints bytes in reverse order or not. Does not make any attempts to further interpret the bits of: sign, exponent or significand (on a typical x86 processor they are printed in that order), and different processors might store them differently. It is not useful for types which internally use a pointer because for them this function prints not the floating point number but a pointer. This is for debugging purposes.)""");

		py::def("getFloatDistanceULP",
		        static_cast<RealHP<N> (*)(const RealHP<N>&, const RealHP<N>&)>(&boost::math::float_distance),
		        R"""(:return: an integer value stored in ``RealHP<N>``, the `ULP distance <https://en.wikipedia.org/wiki/Unit_in_the_last_place>`__ calculated by `boost::math::float_distance <https://www.boost.org/doc/libs/1_73_0/libs/math/doc/html/math_toolkit/next_float/float_distance.html>`__, also see `Floating-point Comparison <https://www.boost.org/doc/libs/1_73_0/libs/math/doc/html/math_toolkit/float_comparison.html>`__ and `Prof. Kahan paper about this topic <https://people.eecs.berkeley.edu/~wkahan/Mindless.pdf>`__.

.. warning::
	The returned value is the **directed distance** between two arguments, this means that it can be negative.
)""");

		py::def("fromBits",
		        static_cast<RealHP<N> (*)(const std::string&, int, int)>(&fromBits<N>),
		        (py::arg("bits"), py::arg("exp") = 0, py::arg("sign") = 1),
		        R"""(
:param bits: ``str`` - a string containing '0', '1' characters.
:param exp:  ``int`` - the binary exponent which shifts the bits.
:param sign: ``int`` - the sign, should be -1 or +1, but it is not checked. It multiplies the result when construction from bits is finished.
:return: ``RealHP<N>`` constructed from string containing '0', '1' bits. This is for debugging purposes, rather slow.
)""");
	}
};

template <int minHP> class TestBits { // minHP is because the bits absent in lower precision should be zero to avoid ambiguity.
private:
	static const constexpr auto maxN = boost::mpl::back<math::RealHPConfig::SupportedByEigenCgal>::type::value; // the absolutely maximum precision.
	static const constexpr auto maxP = boost::mpl::back<math::RealHPConfig::SupportedByMinieigen>::type::value; // this one gets exported to python.
	using Rnd        = std::array<RealHP<minHP>, 3>; // minHP is because the bits absent in lower precision should be zero to avoid ambiguity.
	using Error      = std::pair<std::vector<std::array<RealHP<maxP>, 3>> /* arguments */, RealHP<maxP> /* ULP error */>;
	using FuncErrors = std::map<int /* N, the level of HP */, Error>;
	int                                 testCount;
	const RealHP<minHP>                 minX;
	const RealHP<minHP>                 maxX;
	const std::set<int>&                testSet;
	bool                                first { true };
	bool                                collectArgs { false };
	bool                                extraChecks { false };
	FuncErrors                          empty;
	std::map<std::string, FuncErrors>   results;
	std::map<std::string, RealHP<maxN>> reference;
	Rnd                                 minHPArgs;
	bool                                useRandomArgs;
	// When not useRandomArgs then a linear scan is performed. The scan starts from minX but to make the range to be "in the middle" between minX and maxX,
	// the start has to shifted by half { 0.5 } upwards from minX. Of course this arbitrary choice will be nullified by any non-symmetrical choice of minX…maxX.
	RealHP<minHP> count { 0.5 };

	enum struct Domain { All, PlusMinus1, AboveMinus1, Above1, Above0, NonZero };
	template <int testN> auto applyDomain(const RealHP<testN>& x, const Domain& d)
	{
		switch (d) {
			case Domain::All: return x;
			case Domain::PlusMinus1: return math::fmod(math::abs(x), static_cast<RealHP<testN>>(2)) - 1;
			case Domain::AboveMinus1: return math::abs(x) - 1;
			case Domain::Above1: return math::abs(x) + 1;
			case Domain::Above0: return math::abs(x);
			case Domain::NonZero: return (x == 0) ? 1 : x; // extremely rare
			default: throw std::runtime_error("applyDomain : unrecognized domain selected to use.");
		};
	}

public:
	TestBits(const int& testCount_, const Real& minX_, const Real& maxX_, bool useRnd, const std::set<int>& testSet_, bool collectArgs_, bool extraChecks_)
	        : testCount(testCount_)
	        , minX(minX_)
	        , maxX(maxX_)
	        , testSet(testSet_)
	        , collectArgs(collectArgs_)
	        , extraChecks(extraChecks_)
	        , minHPArgs { 0, 0, 0 } // up to 3 arguments are needed. Though most of the time only the first one is used.
	        , useRandomArgs { useRnd }
	{
		for (auto N : testSet)
			if (N != *testSet.rbegin())
				empty[N] = Error { {}, 0 };
	}
	void prepare()
	{
		first = true;
		for (auto& a : minHPArgs)
			a = Eigen::internal::random<minHP>(minX, maxX);
		if (not useRandomArgs) {
			// To make sure that these equidistant points are not overlapping when math::abs(…) or math::fmod(…) are used I add a small random variation to them.
			minHPArgs[0] = Eigen::internal::random<minHP>(
			        static_cast<RealHP<minHP>>(-0.4999999999999999), static_cast<RealHP<minHP>>(0.4999999999999999));
			// The points will be still equidistant on average ↓← here the random variation is used.
			minHPArgs[0] = minX + (maxX - minX) * (count + minHPArgs[0]) / RealHP<minHP>(testCount);
			count += 1;
			//LOG_NOFILTER(minHPArgs[0]);
			if (extraChecks) {
				static RealHP<minHP> prev = minX - 1;
				if ((prev > minHPArgs[0]) and (math::abs(minHPArgs[0] - maxX) > 2)) {
					LOG_FATAL("Error:\nprev=" << prev << "\nnow =" << minHPArgs[0]);
					throw std::runtime_error(
					        "prepare() : point was generated in wrong interval, please report a bug, prev=" + math::toStringHP(prev)
					        + " arg=" + math::toStringHP(minHPArgs[0]));
				}
				prev = minHPArgs[0];
			}
		}
	}
	template <int testN>
	void amend(const std::string& funcName, const RealHP<testN>& funcValue, const std::vector<Domain>& domains, const std::array<RealHP<testN>, 3>& args)
	{
		if (results.count(funcName) == 0)
			results[funcName] = empty;
		if (first) { // store results for the highest N
			reference[funcName] = static_cast<RealHP<maxN>>(funcValue);
		} else if (math::isfinite(funcValue) and math::isfinite(reference[funcName])) {
			auto ulpError
			        = static_cast<RealHP<maxP>>(math::abs(boost::math::float_distance(static_cast<RealHP<testN>>(reference[funcName]), funcValue)));
			if (ulpError > results[funcName][testN].second) {
				std::array<RealHP<maxP>, 3> usedArgs { 0, 0, 0 };
				for (size_t i = 0; i < 3; ++i)
					if (i < domains.size())
						usedArgs[i] = static_cast<RealHP<maxP>>(applyDomain<testN>(args[i], domains[i]));
				if (collectArgs) {
					results[funcName][testN].first.push_back(usedArgs);
					results[funcName][testN].second = ulpError;
				} else {
					results[funcName][testN] = Error { { usedArgs }, ulpError };
				}
			}
		}
	}
	template <int testN>
	void amend(const std::string& funcName, const ComplexHP<testN>& funcValue, const std::vector<Domain>& dom, const std::array<RealHP<testN>, 3>& args)
	{
		amend<testN>("complex " + funcName + " real", funcValue.real(), dom, args);
		amend<testN>("complex " + funcName + " imag", funcValue.imag(), dom, args);
	}
	template <int testN> void checkFunctions()
	{
		if (testSet.find(testN) == testSet.end()) // skip N that were not requested to be tested.
			return;
		std::array<RealHP<testN>, 3> args {};
		for (int i = 0; i < 3; ++i) {
			args[i] = static_cast<RealHP<testN>>(minHPArgs[i]); // Prepare the random numbers with correct precision.
			if (extraChecks)                                    // Will throw in case of error. Just an extra check.
				DecomposedReal::veryEqual(minHPArgs[i], args[i]);
		}
		// clang-format off
// Add here tests of more functions as necessary. Make sure to use proper domain.
//                ↓ R - Real, C - Complex
#define CHECK_FUN_R_1(f, domain)     amend<testN>(#f, math::f(                 applyDomain<testN>(args[0], domain)), { domain }, args)
#define CHECK_FUN_R_2(f, d1, d2)     amend<testN>(#f, math::f(                 applyDomain<testN>(args[0], d1), applyDomain<testN>(args[1], d2) ), { d1, d2 }, args)
#define CHECK_FUN_R_3(f, d1, d2, d3) amend<testN>(#f, math::f(                 applyDomain<testN>(args[0], d1), applyDomain<testN>(args[1], d2)  , applyDomain<testN>(args[2], d3)), { d1, d2, d3 }, args)
#define CHECK_FUN_C_2(f, d1, d2)     amend<testN>(#f, math::f(ComplexHP<testN>(applyDomain<testN>(args[0], d1), applyDomain<testN>(args[1], d2))), { d1, d2 }, args)
		CHECK_FUN_R_2(basicAdd , Domain::All, Domain::All);        // all
		CHECK_FUN_R_2(basicSub , Domain::All, Domain::All);        // all
		CHECK_FUN_R_2(basicDiv , Domain::All, Domain::NonZero);    // all
		CHECK_FUN_R_2(basicMult, Domain::All, Domain::All);        // all

		CHECK_FUN_R_1(sin  , Domain::All);                         // all
		CHECK_FUN_R_1(cos  , Domain::All);                         // all
		CHECK_FUN_R_1(tan  , Domain::All);                         // all
		CHECK_FUN_R_1(sinh , Domain::All);                         // all
		CHECK_FUN_R_1(cosh , Domain::All);                         // all
		CHECK_FUN_R_1(tanh , Domain::All);                         // all

		CHECK_FUN_R_1(asin , Domain::PlusMinus1);                  // -1,1
		CHECK_FUN_R_1(acos , Domain::PlusMinus1);                  // -1,1
		CHECK_FUN_R_1(atan , Domain::All);                         // all
		CHECK_FUN_R_1(asinh, Domain::All);                         // all
		CHECK_FUN_R_1(acosh, Domain::Above1);                      // x>1
		CHECK_FUN_R_1(atanh, Domain::PlusMinus1);                  // -1,1
		CHECK_FUN_R_2(atan2, Domain::All, Domain::All);            // all

		// FIXME: skip complex checks until I update ComplexHP<N> to take into account suggestions from https://github.com/boostorg/multiprecision/issues/262
		//CHECK_FUN_C_2(sin  , Domain::All, Domain::All);          // all
		//CHECK_FUN_C_2(cos  , Domain::All, Domain::All);          // all
		//CHECK_FUN_C_2(tan  , Domain::All, Domain::All);          // all
		//CHECK_FUN_C_2(sinh , Domain::All, Domain::All);          // all
		//CHECK_FUN_C_2(cosh , Domain::All, Domain::All);          // all
		//CHECK_FUN_C_2(tanh , Domain::All, Domain::All);          // all

		CHECK_FUN_R_1(log  , Domain::Above0);                      // x>0
		CHECK_FUN_R_1(log10, Domain::Above0);                      // x>0
		CHECK_FUN_R_1(log1p, Domain::AboveMinus1);                 // x>-1
		CHECK_FUN_R_1(log2 , Domain::Above0);                      // x>0
		CHECK_FUN_R_1(logb , Domain::NonZero);                     // x≠0
		//CHECK_FUN_R_1(ilogb);                                    // x≠0 // If the correct result is greater than INT_MAX or smaller than INT_MIN, then return value is unspecified.

		//CHECK_FUNC(ldexp);
		//CHECK_FUNC(frexp);
		CHECK_FUN_R_1(exp  , Domain::All);                         // all
		CHECK_FUN_R_1(exp2 , Domain::All);                         // all
		CHECK_FUN_R_1(expm1, Domain::All);                         // all

		CHECK_FUN_R_2(pow  , Domain::Above0, Domain::All);         // x>0 (no complex here) , all y
		CHECK_FUN_R_1(sqrt , Domain::Above0);                      // x>0
		CHECK_FUN_R_1(cbrt , Domain::All);                         // all
		CHECK_FUN_R_2(hypot, Domain::All, Domain::All);            // all

		CHECK_FUN_R_1(erf   , Domain::All);                        // all
		CHECK_FUN_R_1(erfc  , Domain::All);                        // all
		CHECK_FUN_R_1(lgamma, Domain::Above0);                     // x>0
		CHECK_FUN_R_1(tgamma, Domain::All);                        // all, except pole errors

		//CHECK_FUNC(modf);
		CHECK_FUN_R_2(fmod     , Domain::All, Domain::NonZero);    // y≠0
		CHECK_FUN_R_2(remainder, Domain::All, Domain::NonZero);    // y≠0
		//CHECK_FUNC(remquo);
		CHECK_FUN_R_3(fma, Domain::All, Domain::All, Domain::All); // all
#undef CHECK_FUN_R_1
#undef CHECK_FUN_R_2
#undef CHECK_FUN_R_3
#undef CHECK_FUN_C_2
		// clang-format on
		first = false;
	}
	py::dict getResult()
	{
		py::dict ret {};
		for (const auto& a : results) {
			py::dict badBits {};
			for (const auto& funcErrors : a.second) {
				const Error&                                    er   = funcErrors.second;
				const std::vector<std::array<RealHP<maxP>, 3>>& args = er.first;
				py::list                                        pyArgs {};
				for (const auto& arg : args) {
					pyArgs.append(py::make_tuple(arg[0], arg[1], arg[2]));
				}
				badBits[math::RealHPConfig::getDigits2(funcErrors.first)] = py::make_tuple(pyArgs, er.second);
			}
			ret[a.first] = badBits;
		}
		return ret;
	}
};

template <int minHP> struct TestLoop {
	TestBits<minHP>& tb;
	TestLoop(TestBits<minHP>& t)
	        : tb(t) {};
	template <typename Nmpl> void operator()(Nmpl) { tb.template checkFunctions<Nmpl::value>(); }
};

template <int minHP>
py::dict
runTest(int                  testCount,
        const Real&          minX,
        const Real&          maxX,
        bool                 useRandomArgs,
        int                  printEveryNth,
        const std::set<int>& testSet,
        bool                 collectArgs,
        bool                 extraChecks)
{
	TestBits<minHP> testHelper(testCount, minX, maxX, useRandomArgs, testSet, collectArgs, extraChecks);
	TestLoop<minHP> testLoop(testHelper);
	while (testCount-- > 0) {
		testHelper.prepare();
		boost::mpl::for_each<boost::mpl::reverse<math::RealHPConfig::SupportedByMinieigen>::type>(testLoop);
		if (((testCount % printEveryNth) == 0) and (testCount != 0))
			LOG_INFO("minHP = " << minHP << ", testCount = " << testCount << "\n" << py::extract<std::string>(py::str(testHelper.getResult()))());
	}
	return testHelper.getResult();
}

py::dict
getRealHPErrors(const py::list& testLevelsHP, int testCount, Real minX, Real maxX, bool useRandomArgs, int printEveryNth, bool collectArgs, bool extraChecks)
{
	TRVAR1(py::extract<std::string>(py::str(testLevelsHP))());
	TRVAR5(testCount, minX, maxX, useRandomArgs, printEveryNth);
	TRVAR2(collectArgs, extraChecks);
	std::set<int> testSet { py::stl_input_iterator<int>(testLevelsHP), py::stl_input_iterator<int>() };
	if (testSet.size() < 2)
		throw std::runtime_error("The testLevelsHP is too small, there must be a higher precision to test against.");
	if (not std::includes(math::RealHPConfig::supportedByMinieigen.begin(), math::RealHPConfig::supportedByMinieigen.end(), testSet.begin(), testSet.end()))
		throw std::runtime_error("testLevelsHP contains N not present in yade.math.RealHPConfig.getSupportedByMinieigen()");
	int smallestTestedHPn = *testSet.begin();
	// Go from runtime parameter to a constexpr template parameter. This allows for greater precision in entire test.
	if (smallestTestedHPn == 1)
		return runTest<1>(testCount, minX, maxX, useRandomArgs, printEveryNth, testSet, collectArgs, extraChecks);
	else
		return runTest<2>(testCount, minX, maxX, useRandomArgs, printEveryNth, testSet, collectArgs, extraChecks);

	// This uses a switch instead and produces all possible variants, but is compiling longer.
	/* switch (smallestTestedHPn) {
	#define CASE_LEVEL_HP_TEST(levelHP)                                                                                                                            \
	case levelHP: return runTest<levelHP>(testCount, minX, maxX, printEveryNth, testSet);
		YADE_REGISTER_HP_LEVELS(CASE_LEVEL_HP_TEST)
	#undef CASE_LEVEL_HP_TEST
	}*/
}

} // namespace yade

// https://www.boost.org/doc/libs/1_73_0/libs/math/doc/html/math_toolkit/float_comparison.html
// https://bitbashing.io/comparing-floats.html
// https://github.com/boostorg/multiprecision/pull/249
// nice example of losing precision due to catastrophic cancellation:
// https://en.wikipedia.org/wiki/Loss_of_significance
// https://people.eecs.berkeley.edu/~wkahan/Qdrtcs.pdf
// https://people.eecs.berkeley.edu/~wkahan/Mindless.pdf

void exposeRealHPDiagnostics()
{
	// this loop registers diagnostic functions for all HPn types.
	::yade::math::detail::registerLoopForHPn<::yade::math::RealHPConfig::SupportedByMinieigen, ::yade::RegisterRealBitDebug>();

	py::def("isThisSystemLittleEndian",
	        ::yade::isThisSystemLittleEndian,
	        R"""(
:return: ``True`` if this system uses little endian architecture, ``False`` otherwise.
	)""");

	py::def("getRealHPErrors",
	        yade::getRealHPErrors,
	        (py::arg("testLevelsHP"),
	         py::arg("testCount")     = 10,
	         py::arg("minX")          = yade::Real { -10 },
	         py::arg("maxX")          = yade::Real { 10 },
	         py::arg("useRandomArgs") = false,
	         py::arg("printEveryNth") = 1000,
	         py::arg("collectArgs")   = false,
	         py::arg("extraChecks")   = false),
	        R"""(
Tests mathematical functions against the highest precision in argument ``testLevelsHP`` and returns the largest `ULP distance <https://en.wikipedia.org/wiki/Unit_in_the_last_place>`__ `found <https://www.boost.org/doc/libs/1_73_0/libs/math/doc/html/math_toolkit/float_comparison.html>`__ with :yref:`getFloatDistanceULP<yade._math.getFloatDistanceULP>`. A ``testCount`` randomized tries with function arguments in range ``minX ... maxX`` are performed on the ``RealHP<N>`` types where ``N`` is from the list provided in ``testLevelsHP`` argument.

:param testLevelsHP: a ``list`` of ``int`` values consisting of high precision levels ``N`` (in ``RealHP<N>``) for which the tests should be done. Must consist at least of two elements so that there is a higher precision type available against which to perform the tests.
:param testCount: ``int`` - specifies how many randomized tests of each function to perform.
:param minX: ``Real`` - start of the range in which the random arguments are generated.
:param maxX: ``Real`` - end of that range.
:param useRandomArgs: If ``False`` (default) then ``minX ... maxX`` is divided into ``testCount`` equidistant points. If ``True`` then each call is a random number. This applies only to the first argument of a function, if a function takes more than one argument, then remaining arguments are random - 2D scans are not performed.
:param printEveryNth: will :ref:`print using<logging>` ``LOG_INFO`` the progress information every Nth step in the ``testCount`` loop. To see it e.g. start using ``yade -f6``, also see :ref:`logger documentation<logging>`.
:param collectArgs: if ``True`` then in returned results will be a longer list of arguments that produce incorrect results.
:param extraChecks: will perform extra checks while executing this funcion. Useful only for debugging of :yref:`getRealHPErrors<yade._math.getRealHPErrors>`.
:return: A python dictionary with the largest ULP distance to the correct function value. For each function name there is a dictionary consisting of: how many binary digits (bits) are in the tested ``RealHP<N>`` type, the worst arguments for this function, and the ULP distance to the reference value.

.. hint::
	The returned ULP error is an absolute value, as opposed to :yref:`getFloatDistanceULP<yade._math.getFloatDistanceULP>` which is signed.

	)""");
}
