/*************************************************************************
*  2019 Janek Kozicki                                                    *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#ifndef REAL_EXPOSED_TYPES_HPP
#define REAL_EXPOSED_TYPES_HPP

#include <Eigen/Core>
#include <complex>
#include <unsupported/Eigen/AlignedVector3>

#ifndef YADE_REAL_MATH_NAMESPACE
#error "This file cannot be included alone, include Real.hpp instead"
#endif

namespace yade {

template <typename Scalar> using Vector2 = Eigen::Matrix<Scalar, 2, 1>;
template <typename Scalar> using Vector3 = Eigen::Matrix<Scalar, 3, 1>;
template <typename Scalar> using Vector4 = Eigen::Matrix<Scalar, 4, 1>;
template <typename Scalar> using Vector6 = Eigen::Matrix<Scalar, 6, 1>;

template <typename Scalar> using Matrix2 = Eigen::Matrix<Scalar, 2, 2>;
template <typename Scalar> using Matrix3 = Eigen::Matrix<Scalar, 3, 3>;
template <typename Scalar> using Matrix4 = Eigen::Matrix<Scalar, 4, 4>;
template <typename Scalar> using Matrix6 = Eigen::Matrix<Scalar, 6, 6>;

/*************************************************************************/
/*************************      Integer         **************************/
/*************************************************************************/
// integral type for indices, to avoid compiler warnings with int
using Index = Eigen::Matrix<int, 1, 1>::Index;

using Vector2i = Vector2<int>;
using Vector3i = Vector3<int>;
using Vector4i = Vector4<int>;
using Vector6i = Vector6<int>;
using VectorXi = Eigen::Matrix<int, Eigen::Dynamic, 1>;

using Matrix2i = Matrix2<int>;
using Matrix3i = Matrix3<int>;
using Matrix4i = Matrix4<int>;
using Matrix6i = Matrix6<int>;
using MatrixXi = Eigen::Matrix<int, Eigen::Dynamic, Eigen::Dynamic>;

/*************************************************************************/
/*************************        Real          **************************/
/*************************************************************************/

// First declare for RealHP<n>

#ifdef EIGEN_DONT_ALIGN
template <int N> using Vector3rHP = Vector3<RealHP<N>>;
#else
// For start let's try SSE vectorization without AlignedVector3.
// Later we can improve performance by using AlignedVector3 instead of Vector3r.
// AlignedVector3 will need a few tweaks to make it right.
// using Vector3rHP = Eigen::AlignedVector3<RealHP<N>>;
template <int N> using Vector3rHP = Vector3<RealHP<N>>;
#endif

template <int N> using Vector2rHP  = Vector2<RealHP<N>>;
template <int N> using Vector3raHP = Eigen::AlignedVector3<RealHP<N>>;
template <int N> using Vector4rHP  = Vector4<RealHP<N>>;
template <int N> using Vector6rHP  = Vector6<RealHP<N>>;
template <int N> using VectorXrHP  = Eigen::Matrix<RealHP<N>, Eigen::Dynamic, 1>;

template <int N> using Matrix2rHP = Matrix2<RealHP<N>>;
template <int N> using Matrix3rHP = Matrix3<RealHP<N>>;
template <int N> using Matrix4rHP = Matrix4<RealHP<N>>;
template <int N> using Matrix6rHP = Matrix6<RealHP<N>>;
template <int N> using MatrixXrHP = Eigen::Matrix<RealHP<N>, Eigen::Dynamic, Eigen::Dynamic>;

template <int N> using QuaternionrHP  = Eigen::Quaternion<RealHP<N>>;
template <int N> using AngleAxisrHP   = Eigen::AngleAxis<RealHP<N>>;
template <int N> using AlignedBox3rHP = Eigen::AlignedBox<RealHP<N>, 3>;
template <int N> using AlignedBox2rHP = Eigen::AlignedBox<RealHP<N>, 2>;

// Then declare specialization for n==1 with the originally used names.

using Vector3r = Vector3rHP<1>;

using Vector2r  = Vector2rHP<1>;
using Vector3ra = Vector3raHP<1>;
using Vector4r  = Vector4rHP<1>;
using Vector6r  = Vector6rHP<1>;
using VectorXr  = VectorXrHP<1>;

using Matrix2r = Matrix2rHP<1>;
using Matrix3r = Matrix3rHP<1>;
using Matrix4r = Matrix4rHP<1>;
using Matrix6r = Matrix6rHP<1>;
using MatrixXr = MatrixXrHP<1>;

using Quaternionr  = QuaternionrHP<1>;
using AngleAxisr   = AngleAxisrHP<1>;
using AlignedBox3r = AlignedBox3rHP<1>;
using AlignedBox2r = AlignedBox2rHP<1>;

/*************************************************************************/
/*************************       Complex        **************************/
/*************************************************************************/

template <int N> using Vector2crHP = Vector2<ComplexHP<N>>;
template <int N> using Vector3crHP = Vector3<ComplexHP<N>>;
template <int N> using Vector4crHP = Vector4<ComplexHP<N>>;
template <int N> using Vector6crHP = Vector6<ComplexHP<N>>;
template <int N> using VectorXcrHP = Eigen::Matrix<ComplexHP<N>, Eigen::Dynamic, 1>;

template <int N> using Matrix2crHP = Matrix2<ComplexHP<N>>;
template <int N> using Matrix3crHP = Matrix3<ComplexHP<N>>;
template <int N> using Matrix4crHP = Matrix4<ComplexHP<N>>;
template <int N> using Matrix6crHP = Matrix6<ComplexHP<N>>;
template <int N> using MatrixXcrHP = Eigen::Matrix<ComplexHP<N>, Eigen::Dynamic, Eigen::Dynamic>;

using Vector2cr = Vector2crHP<1>;
using Vector3cr = Vector3crHP<1>;
using Vector4cr = Vector4crHP<1>;
using Vector6cr = Vector6crHP<1>;
using VectorXcr = VectorXcrHP<1>;

using Matrix2cr = Matrix2crHP<1>;
using Matrix3cr = Matrix3crHP<1>;
using Matrix4cr = Matrix4crHP<1>;
using Matrix6cr = Matrix6crHP<1>;
using MatrixXcr = MatrixXcrHP<1>;

/*************************************************************************/
/*************************         Se3          **************************/
/*************************************************************************/

template <class Scalar> class Se3 {
public:
	Vector3<Scalar>           position;
	Eigen::Quaternion<Scalar> orientation;
	Se3() {};
	Se3(Vector3<Scalar> rkP, Eigen::Quaternion<Scalar> qR)
	{
		position    = rkP;
		orientation = qR;
	}
	Se3(Se3<Scalar>& a, Se3<Scalar>& b)
	{
		position    = b.orientation.inverse() * (a.position - b.position);
		orientation = b.orientation.inverse() * a.orientation;
	}
	Se3<Scalar> inverse() { return Se3(-(orientation.inverse() * position), orientation.inverse()); }
	void        toGLMatrix(float m[16])
	{
		orientation.toGLMatrix(m);
		m[12] = position[0];
		m[13] = position[1];
		m[14] = position[2];
	}
	Vector3<Scalar> operator*(const Vector3<Scalar>& b) { return orientation * b + position; }
	Se3<Scalar>     operator*(const Eigen::Quaternion<Scalar>& b) { return Se3<Scalar>(position, orientation * b); }
	Se3<Scalar>     operator*(const Se3<Scalar>& b) { return Se3<Scalar>(orientation * b.position + position, orientation * b.orientation); }
};

template <int N> using Se3rHP = Se3<RealHP<N>>;
using Se3r                    = Se3rHP<1>;

/*************************************************************************/
/*************************   for external use   **************************/
/*************************************************************************/

// This is for external applications, shouldn't be normally used.
// Use `using namespace ::yade::AllMathTypes;` only inside a .cpp file! Otherwise the types will leak outside which will cause compilation errors due to ambiguity.
namespace MathEigenTypes {
	// integer types
	using ::yade::Index;

	using ::yade::Vector2i;
	using ::yade::Vector3i;
	using ::yade::Vector4i;
	using ::yade::Vector6i;
	using ::yade::VectorXi;

	using ::yade::Matrix2i;
	using ::yade::Matrix3i;
	using ::yade::Matrix4i;
	using ::yade::Matrix6i;
	using ::yade::MatrixXi;

	// using all of the RealHP<N> kind.
	using ::yade::RealHP;

	using ::yade::ComplexHP;

	using ::yade::Vector2rHP;
	using ::yade::Vector3raHP;
	using ::yade::Vector3rHP;
	using ::yade::Vector4rHP;
	using ::yade::Vector6rHP;
	using ::yade::VectorXrHP;

	using ::yade::Matrix2rHP;
	using ::yade::Matrix3rHP;
	using ::yade::Matrix4rHP;
	using ::yade::Matrix6rHP;
	using ::yade::MatrixXrHP;

	using ::yade::AlignedBox2rHP;
	using ::yade::AlignedBox3rHP;
	using ::yade::AngleAxisrHP;
	using ::yade::QuaternionrHP;

	using ::yade::Vector2crHP;
	using ::yade::Vector3crHP;
	using ::yade::Vector4crHP;
	using ::yade::Vector6crHP;
	using ::yade::VectorXcrHP;

	using ::yade::Matrix2crHP;
	using ::yade::Matrix3crHP;
	using ::yade::Matrix4crHP;
	using ::yade::Matrix6crHP;
	using ::yade::MatrixXcrHP;

	using ::yade::Se3rHP;

	// Then use the specialization for N==1 with the original names.
	using ::yade::Real;

	using ::yade::Complex;

	using ::yade::Vector2r;
	using ::yade::Vector3r;
	using ::yade::Vector3ra;
	using ::yade::Vector4r;
	using ::yade::Vector6r;
	using ::yade::VectorXr;

	using ::yade::Matrix2r;
	using ::yade::Matrix3r;
	using ::yade::Matrix4r;
	using ::yade::Matrix6r;
	using ::yade::MatrixXr;

	using ::yade::AlignedBox2r;
	using ::yade::AlignedBox3r;
	using ::yade::AngleAxisr;
	using ::yade::Quaternionr;

	using ::yade::Vector2cr;
	using ::yade::Vector3cr;
	using ::yade::Vector4cr;
	using ::yade::Vector6cr;
	using ::yade::VectorXcr;

	using ::yade::Matrix2cr;
	using ::yade::Matrix3cr;
	using ::yade::Matrix4cr;
	using ::yade::Matrix6cr;
	using ::yade::MatrixXcr;

	using ::yade::Se3r;
}

}

#endif
