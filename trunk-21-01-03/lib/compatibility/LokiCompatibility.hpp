/*************************************************************************
*  2019 Janek Kozicki                                                    *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

// after 16 years we stop using Loki library, because it does not compile with C++17
// this header is to make the transition easier.

#pragma once

#include <boost/mpl/at.hpp>
#include <boost/mpl/vector.hpp>

// compat with former yade's local Loki

#define TYPELIST_1(T1) ::boost::mpl::vector<T1>
#define TYPELIST_2(T1, T2) ::boost::mpl::vector<T1, T2>
#define TYPELIST_3(T1, T2, T3) ::boost::mpl::vector<T1, T2, T3>
#define TYPELIST_4(T1, T2, T3, T4) ::boost::mpl::vector<T1, T2, T3, T4>
#define TYPELIST_5(T1, T2, T3, T4, T5) ::boost::mpl::vector<T1, T2, T3, T4, T5>
#define TYPELIST_7(T1, T2, T3, T4, T5, T6, T7) ::boost::mpl::vector<T1, T2, T3, T4, T5, T6, T7>
