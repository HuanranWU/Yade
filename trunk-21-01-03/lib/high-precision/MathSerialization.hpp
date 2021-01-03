/*************************************************************************
*  2010 Václav Šmilauer                                                  *
*  2012 Anton Gladky                                                     *
*  2019 Janek Kozicki                                                    *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

/*
 * Serialization of math classes
 */

#ifndef ALL_MATH_TYPES_SERIALIZATION_HPP
#define ALL_MATH_TYPES_SERIALIZATION_HPP

#include <lib/high-precision/RealIO.hpp>

#if (YADE_REAL_BIT > 80)
#include <boost/serialization/split_free.hpp>
BOOST_SERIALIZATION_SPLIT_FREE(::yade::math::Real);
BOOST_IS_BITWISE_SERIALIZABLE(::yade::math::Real);
#endif
#if (YADE_REAL_BIT == 80)
BOOST_IS_BITWISE_SERIALIZABLE(::yade::math::Real);
#endif

// fast serialization (no version info and no tracking) for basic math types
// http://www.boost.org/doc/libs/1_42_0/libs/serialization/doc/traits.html#bitwise

BOOST_IS_BITWISE_SERIALIZABLE(yade::Vector2r);
BOOST_IS_BITWISE_SERIALIZABLE(yade::Vector2i);
BOOST_IS_BITWISE_SERIALIZABLE(yade::Vector3r);
BOOST_IS_BITWISE_SERIALIZABLE(yade::Vector3i);
BOOST_IS_BITWISE_SERIALIZABLE(yade::Vector6r);
BOOST_IS_BITWISE_SERIALIZABLE(yade::Vector6i);
BOOST_IS_BITWISE_SERIALIZABLE(yade::Quaternionr);
BOOST_IS_BITWISE_SERIALIZABLE(yade::Se3r);
BOOST_IS_BITWISE_SERIALIZABLE(yade::Matrix3r);
BOOST_IS_BITWISE_SERIALIZABLE(yade::Matrix6r);


namespace boost {
namespace serialization {

#if (YADE_REAL_BIT > 80)
	template <class Archive> void save(Archive& ar, const ::yade::math::Real& a, unsigned int)
	{
		// TODO: maybe we can find a faster method for float128
		std::string v = ::yade::math::toString(a);
		ar&         BOOST_SERIALIZATION_NVP(v);
	}
	template <class Archive> void load(Archive& ar, ::yade::math::Real& a, unsigned int)
	{
		std::string v {};
		ar&         BOOST_SERIALIZATION_NVP(v);
		a = ::yade::math::fromStringReal(v);
	}
#endif
#if (YADE_REAL_BIT == 80)
	template <class Archive> void serialize(Archive& ar, ::yade::math::Real& a, unsigned int)
	{
		::yade::math::UnderlyingReal& v = a.operator ::yade::math::UnderlyingReal&();
		ar&                                 BOOST_SERIALIZATION_NVP(v);
	}
#endif

	template <class Archive> void serialize(Archive& ar, yade::Vector2r& g, const unsigned int /*version*/)
	{
		::yade::Real &x = g[0], &y = g[1];
		ar&           BOOST_SERIALIZATION_NVP(x) & BOOST_SERIALIZATION_NVP(y);
	}

	template <class Archive> void serialize(Archive& ar, yade::Vector2i& g, const unsigned int /*version*/)
	{
		int &x = g[0], &y = g[1];
		ar&  BOOST_SERIALIZATION_NVP(x) & BOOST_SERIALIZATION_NVP(y);
	}

	template <class Archive> void serialize(Archive& ar, yade::Vector3r& g, const unsigned int /*version*/)
	{
		::yade::Real &x = g[0], &y = g[1], &z = g[2];
		ar&           BOOST_SERIALIZATION_NVP(x) & BOOST_SERIALIZATION_NVP(y) & BOOST_SERIALIZATION_NVP(z);
	}

	template <class Archive> void serialize(Archive& ar, yade::Vector3i& g, const unsigned int /*version*/)
	{
		int &x = g[0], &y = g[1], &z = g[2];
		ar&  BOOST_SERIALIZATION_NVP(x) & BOOST_SERIALIZATION_NVP(y) & BOOST_SERIALIZATION_NVP(z);
	}

	template <class Archive> void serialize(Archive& ar, yade::Vector6r& g, const unsigned int /*version*/)
	{
		::yade::Real &v0 = g[0], &v1 = g[1], &v2 = g[2], &v3 = g[3], &v4 = g[4], &v5 = g[5];
		ar&           BOOST_SERIALIZATION_NVP(v0) & BOOST_SERIALIZATION_NVP(v1) & BOOST_SERIALIZATION_NVP(v2) & BOOST_SERIALIZATION_NVP(v3)
		        & BOOST_SERIALIZATION_NVP(v4) & BOOST_SERIALIZATION_NVP(v5);
	}

	template <class Archive> void serialize(Archive& ar, yade::Vector6i& g, const unsigned int /*version*/)
	{
		int &v0 = g[0], &v1 = g[1], &v2 = g[2], &v3 = g[3], &v4 = g[4], &v5 = g[5];
		ar&  BOOST_SERIALIZATION_NVP(v0) & BOOST_SERIALIZATION_NVP(v1) & BOOST_SERIALIZATION_NVP(v2) & BOOST_SERIALIZATION_NVP(v3)
		        & BOOST_SERIALIZATION_NVP(v4) & BOOST_SERIALIZATION_NVP(v5);
	}

	template <class Archive> void serialize(Archive& ar, yade::Quaternionr& g, const unsigned int /*version*/)
	{
		::yade::Real &w = g.w(), &x = g.x(), &y = g.y(), &z = g.z();
		ar&           BOOST_SERIALIZATION_NVP(w) & BOOST_SERIALIZATION_NVP(x) & BOOST_SERIALIZATION_NVP(y) & BOOST_SERIALIZATION_NVP(z);
	}

	template <class Archive> void serialize(Archive& ar, yade::Se3r& g, const unsigned int /*version*/)
	{
		yade::Vector3r&    position    = g.position;
		yade::Quaternionr& orientation = g.orientation;
		ar&                BOOST_SERIALIZATION_NVP(position) & BOOST_SERIALIZATION_NVP(orientation);
	}

	template <class Archive> void serialize(Archive& ar, yade::Matrix3r& m, const unsigned int /*version*/)
	{
		::yade::Real &m00 = m(0, 0), &m01 = m(0, 1), &m02 = m(0, 2), &m10 = m(1, 0), &m11 = m(1, 1), &m12 = m(1, 2), &m20 = m(2, 0), &m21 = m(2, 1),
		             &m22 = m(2, 2);
		ar& BOOST_SERIALIZATION_NVP(m00) & BOOST_SERIALIZATION_NVP(m01) & BOOST_SERIALIZATION_NVP(m02) & BOOST_SERIALIZATION_NVP(m10)
		        & BOOST_SERIALIZATION_NVP(m11) & BOOST_SERIALIZATION_NVP(m12) & BOOST_SERIALIZATION_NVP(m20) & BOOST_SERIALIZATION_NVP(m21)
		        & BOOST_SERIALIZATION_NVP(m22);
	}

	template <class Archive> void serialize(Archive& ar, yade::Matrix6r& m, const unsigned int /*version*/)
	{
		::yade::Real &m00 = m(0, 0), &m01 = m(0, 1), &m02 = m(0, 2), &m03 = m(0, 3), &m04 = m(0, 4), &m05 = m(0, 5);
		::yade::Real &m10 = m(1, 0), &m11 = m(1, 1), &m12 = m(1, 2), &m13 = m(1, 3), &m14 = m(1, 4), &m15 = m(1, 5);
		::yade::Real &m20 = m(2, 0), &m21 = m(2, 1), &m22 = m(2, 2), &m23 = m(2, 3), &m24 = m(2, 4), &m25 = m(2, 5);
		::yade::Real &m30 = m(3, 0), &m31 = m(3, 1), &m32 = m(3, 2), &m33 = m(3, 3), &m34 = m(3, 4), &m35 = m(3, 5);
		::yade::Real &m40 = m(4, 0), &m41 = m(4, 1), &m42 = m(4, 2), &m43 = m(4, 3), &m44 = m(4, 4), &m45 = m(4, 5);
		::yade::Real &m50 = m(5, 0), &m51 = m(5, 1), &m52 = m(5, 2), &m53 = m(5, 3), &m54 = m(5, 4), &m55 = m(5, 5);
		ar&           BOOST_SERIALIZATION_NVP(m00) & BOOST_SERIALIZATION_NVP(m01) & BOOST_SERIALIZATION_NVP(m02) & BOOST_SERIALIZATION_NVP(m03)
		        & BOOST_SERIALIZATION_NVP(m04) & BOOST_SERIALIZATION_NVP(m05) & BOOST_SERIALIZATION_NVP(m10) & BOOST_SERIALIZATION_NVP(m11)
		        & BOOST_SERIALIZATION_NVP(m12) & BOOST_SERIALIZATION_NVP(m13) & BOOST_SERIALIZATION_NVP(m14) & BOOST_SERIALIZATION_NVP(m15)
		        & BOOST_SERIALIZATION_NVP(m20) & BOOST_SERIALIZATION_NVP(m21) & BOOST_SERIALIZATION_NVP(m22) & BOOST_SERIALIZATION_NVP(m23)
		        & BOOST_SERIALIZATION_NVP(m24) & BOOST_SERIALIZATION_NVP(m25) & BOOST_SERIALIZATION_NVP(m30) & BOOST_SERIALIZATION_NVP(m31)
		        & BOOST_SERIALIZATION_NVP(m32) & BOOST_SERIALIZATION_NVP(m33) & BOOST_SERIALIZATION_NVP(m34) & BOOST_SERIALIZATION_NVP(m35)
		        & BOOST_SERIALIZATION_NVP(m40) & BOOST_SERIALIZATION_NVP(m41) & BOOST_SERIALIZATION_NVP(m42) & BOOST_SERIALIZATION_NVP(m43)
		        & BOOST_SERIALIZATION_NVP(m44) & BOOST_SERIALIZATION_NVP(m45) & BOOST_SERIALIZATION_NVP(m50) & BOOST_SERIALIZATION_NVP(m51)
		        & BOOST_SERIALIZATION_NVP(m52) & BOOST_SERIALIZATION_NVP(m53) & BOOST_SERIALIZATION_NVP(m54) & BOOST_SERIALIZATION_NVP(m55);
	}

#ifdef YADE_MASK_ARBITRARY
	template <class Archive> void serialize(Archive& ar, yade::mask_t& v, const unsigned int /*version*/)
	{
		std::string str = v.to_string();
		ar&         BOOST_SERIALIZATION_NVP(str);
		v = yade::mask_t(str);
	}
#endif

} // namespace serialization
} // namespace boost

#endif
