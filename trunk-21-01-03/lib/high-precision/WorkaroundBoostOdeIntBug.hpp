/*************************************************************************
*  2020 Janek Kozicki                                                    *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

// workaround odeint bug https://github.com/boostorg/odeint/issues/40

#ifndef YADE_WORKAROUND_ODEINT_HPP
#define YADE_WORKAROUND_ODEINT_HPP

// https://github.com/boostorg/odeint/issues/40#issuecomment-537537305
// bug was introduced in boost ver. 1.68. When the bug is removed, modify this #ifdef approprately. Check Real.hpp file too.
#if BOOST_VERSION >= 106800

#include <boost/numeric/odeint/algebra/detail/extract_value_type.hpp>

namespace boost {
namespace numeric {
	namespace odeint {
		namespace detail {
			template <> struct extract_value_type<::yade::math::Real> {
				// no value_type defined, return Real
				using type = ::yade::math::Real;
			};
			template <> struct extract_value_type<::yade::math::Complex> {
				// it is Complex, use the value_type as per standard implementation of complex type.
				using type = ::yade::math::Complex::value_type;
			};
		}
	}
}
}

#endif
#endif
