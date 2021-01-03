/*************************************************************************
*  2019 Janek Kozicki                                                    *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

// This is an almost exact copy for ThinRealWrapper. I had to make it because the typedef 'using value_type = ThinRealWrapper<NonComplex>'
// has to be defined only for Complex type. And it's not possible to conditionally declare or not declare a typedef using SFINAE techniques.
// Also I did a unified version of the two files using https://en.wikipedia.org/wiki/Curiously_recurring_template_pattern
// but the code only became much less readable. And not much shorter, so I decided to use the two very similar files instead.
//
// As a nice side effect the ThinRealWrapper got a little shorter, since it has nothing to do with complex type now.

#ifndef YADE_THIN_COMPLEX_WRAPPER_HPP
#define YADE_THIN_COMPLEX_WRAPPER_HPP

#include "ThinRealWrapper.hpp"
#include <boost/type_traits/is_complex.hpp>

// it is possible to #define YADE_IGNORE_IEEE_INFINITY_NAN  ← about that see https://www.boost.org/doc/libs/1_71_0/libs/utility/operators.htm#ordering

// ComplexHP<…> note:
//   If ComplexHP<N> is supported then ThinComplexWrapper needs explicit conversion operators to all other higher precision types, so provide them here.
//   These include headers are necessary to make it work.
//   If other types (e.g. boost quad_double) appear, they will have to be added here.
#ifndef YADE_DISABLE_REAL_MULTI_HP
#ifdef YADE_MPFR
#include <boost/multiprecision/mpfr.hpp>
#else
#include <boost/multiprecision/cpp_bin_float.hpp>
#endif
#include <boost/multiprecision/float128.hpp>
#endif

namespace yade {
namespace math {

	template <typename T> struct RealPart {
		typedef T type;
	};
	template <typename T> struct RealPart<std::complex<T>> {
		typedef T type;
	};

	template <typename WrappedComplex>
#ifdef YADE_IGNORE_IEEE_INFINITY_NAN
	class ThinComplexWrapper : boost::ordered_field_operators1<
	                                   ThinComplexWrapper<WrappedComplex>,
	                                   boost::ordered_field_operators2<ThinComplexWrapper<WrappedComplex>, WrappedComplex>> {
#else
	class ThinComplexWrapper : boost::partially_ordered1<
	                                   ThinComplexWrapper<WrappedComplex>,
	                                   boost::partially_ordered2<
	                                           ThinComplexWrapper<WrappedComplex>,
	                                           WrappedComplex,
	                                           boost::field_operators1<ThinComplexWrapper<WrappedComplex>>>> {
#endif

	private:
		WrappedComplex val;

		// detect types which are convertible to WrappedComplex
		using NonComplex                                       = typename RealPart<WrappedComplex>::type;
		template <typename OtherType> using ComplexConvert     = std::is_convertible<OtherType, WrappedComplex>;
		template <typename OtherType> using NonComplexConvert  = std::is_convertible<OtherType, NonComplex>;
		template <typename OtherType> using EnableIfIsComplex  = std::enable_if_t<boost::is_complex<OtherType>::value>;
		template <typename OtherType> using EnableIfNonComplex = std::enable_if_t<(not boost::is_complex<OtherType>::value)>;
		template <typename OtherType>
		using EnableIfConvertible = std::enable_if_t<(ComplexConvert<OtherType>::value) or (NonComplexConvert<OtherType>::value)>;
		template <typename OtherType> using EnableIfNonComplexConvertible = std::enable_if_t<NonComplexConvert<OtherType>::value>;

		static_assert(boost::is_complex<WrappedComplex>::value == true, "WrappedComplex cannot be real");

	public:
		// this typedef is the only reason for this class. It's not possible to conditionally declare or not declare a typedef using SFINAE techniques.
		using value_type = ThinRealWrapper<NonComplex>;

	public:
		// default constructor
		ThinComplexWrapper() BOOST_NOEXCEPT_IF(boost::has_nothrow_default_constructor<WrappedComplex>::value) = default;
		// copy constructor
		ThinComplexWrapper(const ThinComplexWrapper& initVal) BOOST_NOEXCEPT_IF(boost::has_nothrow_copy_constructor<WrappedComplex>::value) = default;
		// copy assignment operator
		ThinComplexWrapper& operator=(const ThinComplexWrapper& rhs) BOOST_NOEXCEPT_IF(boost::has_nothrow_assign<WrappedComplex>::value) = default;
		// move constructor
		ThinComplexWrapper(ThinComplexWrapper&& moveVal) BOOST_NOEXCEPT_IF(boost::has_nothrow_move<WrappedComplex>::value) = default;
		// move assignment operator
		ThinComplexWrapper& operator=(ThinComplexWrapper&& moveVal) BOOST_NOEXCEPT_IF(boost::has_nothrow_move<WrappedComplex>::value) = default;
		// destructor
		~ThinComplexWrapper() noexcept = default;

		// construct from Real
		// copy constructor from Real
		constexpr ThinComplexWrapper(const ThinRealWrapper<NonComplex>& initVal)
		        BOOST_NOEXCEPT_IF(boost::has_nothrow_copy_constructor<WrappedComplex>::value)
		        : val(static_cast<NonComplex>(initVal)) {};
		// copy assignment operator from Real
		ThinComplexWrapper& operator=(const ThinRealWrapper<NonComplex>& rhs) BOOST_NOEXCEPT_IF(boost::has_nothrow_assign<WrappedComplex>::value)
		{
			val = static_cast<NonComplex>(rhs);
			return *this;
		};
		// move constructor from Real
		constexpr ThinComplexWrapper(ThinRealWrapper<NonComplex>&& moveVal) BOOST_NOEXCEPT_IF(boost::has_nothrow_move<WrappedComplex>::value)
		        : val(static_cast<NonComplex>(moveVal)) {};
		// move assignment operator Real
		ThinComplexWrapper& operator=(ThinRealWrapper<NonComplex>&& moveVal) BOOST_NOEXCEPT_IF(boost::has_nothrow_move<WrappedComplex>::value)
		{
			val = static_cast<NonComplex>(moveVal);
			return *this;
		};
		// constructor from two Real arguments
		template <typename OtherType, typename = EnableIfNonComplexConvertible<OtherType>>
		constexpr ThinComplexWrapper(const ThinRealWrapper<OtherType>& v1, const ThinRealWrapper<OtherType>& v2)
		        BOOST_NOEXCEPT_IF(boost::has_nothrow_move<WrappedComplex>::value)
		        : val(static_cast<OtherType>(v1), static_cast<OtherType>(v2))
		{
		}
		// move/copy constructor from two Real arguments
		template <typename OtherType, typename = EnableIfNonComplexConvertible<OtherType>>
		constexpr ThinComplexWrapper(OtherType&& moveVal_1, OtherType&& moveVal_2) BOOST_NOEXCEPT_IF(boost::has_nothrow_move<WrappedComplex>::value)
		        : val(std::forward<OtherType>(moveVal_1), std::forward<OtherType>(moveVal_2))
		{
		}

		// NOTE: copy and assignment constructors are implemened below as templated move/copy constructors.

		// move/copy constructor from OtherType which is_convertible to WrappedComplex
		template <typename OtherType, typename = EnableIfConvertible<OtherType>>
		constexpr ThinComplexWrapper(OtherType&& moveVal) BOOST_NOEXCEPT_IF(boost::has_nothrow_move<WrappedComplex>::value)
		        : val(static_cast<WrappedComplex>(std::forward<OtherType>(moveVal)))
		{
		}

		// move/copy assignment from OtherType which is_convertible to WrappedComplex
		template <typename OtherType, typename = EnableIfConvertible<OtherType>>
		ThinComplexWrapper& operator=(OtherType&& moveVal) BOOST_NOEXCEPT_IF(boost::has_nothrow_move<WrappedComplex>::value)
		{
			val = std::forward<OtherType>(moveVal);
			return *this;
		}

		// conversion operator to other types
		template <typename OtherType, typename = EnableIfConvertible<OtherType>> explicit operator OtherType() const
		{
			return static_cast<OtherType>(val);
		}
// If ComplexHP<N> is supported then ThinComplexWrapper needs explicit conversion operators to all other higher precision types, so provide them here.
#ifndef YADE_DISABLE_REAL_MULTI_HP
#ifdef YADE_MPFR
		template <unsigned int Dec> using HPBackend = boost::multiprecision::mpfr_float_backend<Dec, boost::multiprecision::allocate_stack>;
#else
		template <unsigned int Dec> using HPBackend = boost::multiprecision::cpp_bin_float<Dec>;
#endif
		template <unsigned int Num>
		explicit operator ::std::complex<::boost::multiprecision::number<HPBackend<Num>, boost::multiprecision::et_off>>() const
		{
			return static_cast<::std::complex<::boost::multiprecision::number<HPBackend<Num>, boost::multiprecision::et_off>>>(val);
		}
		explicit operator ::std::complex<::boost::multiprecision::float128>() const
		{
			return static_cast<::std::complex<::boost::multiprecision::float128>>(val);
		}
#endif
		explicit operator const WrappedComplex&() const { return val; }
		explicit operator WrappedComplex&() { return val; }

		// https://en.cppreference.com/w/cpp/language/cast_operator
		explicit operator WrappedComplex*() { return &val; };
		explicit operator const WrappedComplex*() const { return &val; };

		// field operators
		ThinComplexWrapper& operator+=(const ThinComplexWrapper& x)
		{
			val += x.val;
			return *this;
		}
		ThinComplexWrapper& operator-=(const ThinComplexWrapper& x)
		{
			val -= x.val;
			return *this;
		}
		ThinComplexWrapper& operator*=(const ThinComplexWrapper& x)
		{
			val *= x.val;
			return *this;
		}
		ThinComplexWrapper& operator/=(const ThinComplexWrapper& x)
		{
			val /= x.val;
			return *this;
		}
		const ThinComplexWrapper  operator-() const { return -val; }
		const ThinComplexWrapper& operator+() const { return *this; }

		// ordering operators
		bool operator<(const ThinComplexWrapper& rhs) const { return val < rhs.val; }
#ifdef YADE_IGNORE_IEEE_INFINITY_NAN
		bool operator==(const ThinComplexWrapper& rhs) const { return val == rhs.val; }
#else
		bool operator==(const ThinComplexWrapper& rhs) const
		{
			check(rhs);
			return val == rhs.val;
		}
		bool operator!=(const ThinComplexWrapper& rhs) const
		{
			check(rhs);
			return val != rhs.val;
		}

		template <typename OtherType, typename = EnableIfConvertible<OtherType>>
		friend inline bool operator==(OtherType rhs, const ThinComplexWrapper& th)
		{
			th.check(rhs);
			return th.val == rhs;
		}
		template <typename OtherType, typename = EnableIfConvertible<OtherType>>
		friend inline bool operator!=(OtherType rhs, const ThinComplexWrapper& th)
		{
			th.check(rhs);
			return th.val != rhs;
		}
#ifdef YADE_WRAPPER_THROW_ON_NAN_INF_COMPLEX
		// this can be useful sometimes for debugging when calculations go all wrong.
		void check(const ThinComplexWrapper& rhs) const
		{
			if (std::isnan(rhs.val.real()) or std::isnan(val.real()) or std::isinf(rhs.val.real()) or std::isinf(val.real())
			    or std::isnan(rhs.val.imag()) or std::isnan(val.imag()) or std::isinf(rhs.val.imag()) or std::isinf(val.imag())) {
				throw std::runtime_error("cannot compare NaN, Inf numbers.");
			}
		}
#else
		// this one is optimized away
		void check(const ThinComplexWrapper&) const {};
#endif
#endif

		// Output/ Input
		friend inline std::ostream& operator<<(std::ostream& os, const ThinComplexWrapper& v) { return os << v.val; }
		friend inline std::istream& operator>>(std::istream& is, ThinComplexWrapper& v)
		{
			is >> v.val;
			return is;
		}

		// member functions specific to Complex type, as in https://en.cppreference.com/w/cpp/numeric/complex
		template <typename OtherType = WrappedComplex, typename = EnableIfIsComplex<OtherType>> value_type real() const { return val.real(); };
		template <typename OtherType = WrappedComplex, typename = EnableIfIsComplex<OtherType>> value_type imag() const { return val.imag(); };
	};

} // namespace math
} // namespace yade

// properly inform boost::is_complex about ThinComplexWrapper
namespace boost {
template <class T> struct is_complex<::yade::math::ThinComplexWrapper<std::complex<T>>> : public true_type {
};
}

static_assert(
        boost::is_complex<::yade::math::ThinComplexWrapper<std::complex<::yade::math::UnderlyingReal>>>::value == true,
        "ThinComplexWrapper<std::complex<UnderlyingReal>> is not recognized by boost::is_complex, please report a bug.");

#endif
