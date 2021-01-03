/*************************************************************************
*  2020 Janek Kozicki                                                    *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#ifndef YADE_UPCONVERSION_OF_BASIC_OPERAORS_FOR_HIGH_PRECISION_HPP
#define YADE_UPCONVERSION_OF_BASIC_OPERAORS_FOR_HIGH_PRECISION_HPP

#include <lib/high-precision/Real.hpp>

/*

If you are using RealHP<1> (which is just Real) along with RealHP<10> and then you write this:

  Real       a = 1;
  RealHP<10> b = 2;
  RealHP<10> c = a+b;

you will get a compilation error, because YADE is very strict in general about type conversions. This is to avoid an unintended error.
It is better to have a direct control over the types, to make sure that no precision is lost anywhere. This involves a bit longer
code to write. And instead of the third line above (which generates an error) you can write this:

  auto       c = static_cast<decltype(b)>(a)+b;

However sometimes this is too inconvenient to write these static_casts everywhere. In that case you can include this header.

This header provides operator+ , operator- , operator* , operator/   which will accept any pair of RealHP<N1> and RealHP<N2> (for N1 ≠ N2)
and return an up-converted precision type: RealHP<max(N1,N2)>, using a helper SelectHigherHP<N1,N2> for this.
This also works for ComplexHP<…> mixed with RealHP<…>.

This is done with maximum possible efficiency: the conversions are performed only when necessary, otherwise the const reference is used.

*/

/*************************************************************************/
/*************************   operator + - * /   **************************/
/*************************************************************************/

// Simplify some basic arithmetic operations between different RealHP<…> , ComplexHP<…> types. Explicit casting by hand is sometimes too tedious.
// Using SFINAE technique the is_convertible and enable_if_t inside levelOfRealHP make sure that these templates apply only when A,B are RealHP<…> types.

namespace yade {
namespace math {
	namespace detail {

		// These are helper types for producing faster code, static_cast<…> will produce faster code when casting to const reference.
		// Because in such case no casting occurs at all. The CastA and CastB make sure to use this whenever possible.
		template <typename A, typename B, int LevelA = levelOfHP<A>, int LevelB = levelOfHP<B>>
		using CastA = typename std::conditional<
		        ((LevelA > LevelB) and (boost::is_complex<A>::value == boost::is_complex<SelectHigherHP<A, B>>::value)),
		        const SelectHigherHP<A, B>&,
		        const SelectHigherHP<A, B>>::type;
		template <typename A, typename B, int LevelA = levelOfHP<A>, int LevelB = levelOfHP<B>>
		using CastB = typename std::conditional<
		        ((LevelA < LevelB) and (boost::is_complex<B>::value == boost::is_complex<SelectHigherHP<A, B>>::value)),
		        const SelectHigherHP<A, B>&,
		        const SelectHigherHP<A, B>>::type;

		// This template makes sure that if any of the two arguments is ComplexHP<…>, then the return type is also complex.
		template <typename A, typename B, int Level>
		using SelectMaybeComplexHP =
		        typename std::conditional<(boost::is_complex<A>::value or boost::is_complex<B>::value), ComplexHP<Level>, RealHP<Level>>::type;

		template <typename A, typename B, int LevelA = levelOfHP<A>, int LevelB = levelOfHP<B>>
		using MaybeComplexA = typename std::conditional<
		        ((LevelA > LevelB) and (boost::is_complex<A>::value == boost::is_complex<SelectMaybeComplexHP<A, B, LevelA>>::value)),
		        const SelectMaybeComplexHP<A, B, LevelA>&,
		        const SelectMaybeComplexHP<A, B, LevelA>>::type;

		template <typename A, typename B, int LevelA = levelOfHP<A>, int LevelB = levelOfHP<B>>
		using MaybeComplexB = typename std::conditional<
		        ((LevelA < LevelB) and (boost::is_complex<B>::value == boost::is_complex<SelectMaybeComplexHP<A, B, LevelB>>::value)),
		        const SelectMaybeComplexHP<A, B, LevelB>&,
		        const SelectMaybeComplexHP<A, B, LevelB>>::type;

		// Older compilers do not have std::void_t, this small helper type is used by HasPlus, HasMinus, etc, below.
		template <typename... Ts> struct make_void {
			typedef void type;
		};
		template <typename... Ts> using void_type = typename make_void<Ts...>::type;

		// These templates are used to find out if there exists an operator+ between types A and B. So basically it is designed to catch the example written at the start of this file:
		//
		//  Real       a = 1;
		//  RealHP<10> b = 2;
		//  RealHP<10> c = a+b;
		//
		// If operator+ does not exist, and compiler is about to raise a compilation error, then the template HasPlus::value will return false. This allows to avoid that compilation error.
		template <typename A, typename B, class = void> struct HasPlus : std::false_type {
		};
		//                                                                        std::declval<A>()            + std::declval<B>() // also works.
		template <typename A, typename B> struct HasPlus<A, B, void_type<decltype(std::declval<A, B>().operator+())>> : std::true_type {
		};
		template <typename A, typename B, class = void> struct HasMinus : std::false_type {
		};
		template <typename A, typename B> struct HasMinus<A, B, void_type<decltype(std::declval<A, B>().operator-())>> : std::true_type {
		};
		template <typename A, typename B, class = void> struct HasMult : std::false_type {
		};
		template <typename A, typename B> struct HasMult<A, B, void_type<decltype(std::declval<A, B>().operator*())>> : std::true_type {
		};
		template <typename A, typename B, class = void> struct HasDiv : std::false_type {
		};
		template <typename A, typename B> struct HasDiv<A, B, void_type<decltype(std::declval<A, B>().operator/())>> : std::true_type {
		};

		// Enable all of this only when the operators+-*/ are not provided and both types are of the HP kind. Either RealHP or ComplexHP.
		// These templates help o determine if these operators are needed by the compiler.
		template <typename A, typename B> struct NeedsBasicPlusOperatorHP {
			static const constexpr bool value = (((not HasPlus<A, B>::value)) and ::yade::math::isHP<A> and ::yade::math::isHP<B>);
		};
		template <typename A, typename B> struct NeedsBasicMinusOperatorHP {
			static const constexpr bool value = (((not HasMinus<A, B>::value)) and ::yade::math::isHP<A> and ::yade::math::isHP<B>);
		};
		template <typename A, typename B> struct NeedsBasicMultOperatorHP {
			static const constexpr bool value = (((not HasMult<A, B>::value)) and ::yade::math::isHP<A> and ::yade::math::isHP<B>);
		};
		template <typename A, typename B> struct NeedsBasicDivOperatorHP {
			static const constexpr bool value = (((not HasDiv<A, B>::value)) and ::yade::math::isHP<A> and ::yade::math::isHP<B>);
		};
	} // namespace detail
} // namespace math

// Finally provide the promised operators.
//
// The nested static_casts are compiled away when unnecessary: (eg. RealHP<…> → RealHP<…>). Otherwise they are needed to work in two stages:
//
//   first promote RealHP   <N1> → ComplexHP<N1>
//   then  promote ComplexHP<N1> → ComplexHP<N2>

template <typename A, typename B>
typename boost::enable_if<math::detail::NeedsBasicPlusOperatorHP<A, B>, math::SelectHigherHP<A, B>>::type operator+(const A& a, const B& b)
{
	return static_cast<math::detail::CastA<A, B>>(static_cast<math::detail::MaybeComplexA<A, B>>(a))
	        + static_cast<math::detail::CastB<A, B>>(static_cast<math::detail::MaybeComplexB<A, B>>(b));
}

template <typename A, typename B>
typename boost::enable_if<math::detail::NeedsBasicMinusOperatorHP<A, B>, math::SelectHigherHP<A, B>>::type operator-(const A& a, const B& b)
{
	return static_cast<math::detail::CastA<A, B>>(static_cast<math::detail::MaybeComplexA<A, B>>(a))
	        - static_cast<math::detail::CastB<A, B>>(static_cast<math::detail::MaybeComplexB<A, B>>(b));
}

template <typename A, typename B>
typename boost::enable_if<math::detail::NeedsBasicMultOperatorHP<A, B>, math::SelectHigherHP<A, B>>::type operator*(const A& a, const B& b)
{
	return static_cast<math::detail::CastA<A, B>>(static_cast<math::detail::MaybeComplexA<A, B>>(a))
	        * static_cast<math::detail::CastB<A, B>>(static_cast<math::detail::MaybeComplexB<A, B>>(b));
}

template <typename A, typename B>
typename boost::enable_if<math::detail::NeedsBasicDivOperatorHP<A, B>, math::SelectHigherHP<A, B>>::type operator/(const A& a, const B& b)
{
	return static_cast<math::detail::CastA<A, B>>(static_cast<math::detail::MaybeComplexA<A, B>>(a))
	        / static_cast<math::detail::CastB<A, B>>(static_cast<math::detail::MaybeComplexB<A, B>>(b));
}

} // namespace yade

#endif
