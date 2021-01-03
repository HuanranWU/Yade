/*************************************************************************
*  2010 Václav Šmilauer                                                  *
*  2012 Anton Gladky                                                     *
*  2019 Janek Kozicki                                                    *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#pragma once

#include <lib/high-precision/Real.hpp>

#include <cassert>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <boost/lexical_cast.hpp>
#include <boost/numeric/conversion/bounds.hpp>
#include <boost/preprocessor.hpp>
#include <boost/python.hpp>
#include <boost/python/class.hpp>
#include <boost/python/module.hpp>
#include <boost/python/object.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/type_traits.hpp>


#ifndef FOREACH
#define FOREACH(i_, arr_) for (i_ : arr_)
#endif


// TODO: they no longer expand to dynamic/static pointer casts depending on DEBUG=ON/OFF build. They are in wrong file. Think about either fixing this or removing them, https://gitlab.com/yade-dev/trunk/issues/97
#ifndef YADE_PTR_CAST
#define YADE_PTR_CAST ::boost::static_pointer_cast
#endif

#ifndef YADE_CAST
#define YADE_CAST static_cast
#endif

#ifndef YADE_PTR_DYN_CAST
#define YADE_PTR_DYN_CAST ::boost::dynamic_pointer_cast
#endif

#ifdef YADE_MASK_ARBITRARY
#include <bitset>
#endif

#include <Eigen/Core>
#include <Eigen/Eigenvalues>
#include <Eigen/Geometry>

#include <boost/serialization/is_bitwise_serializable.hpp>
#include <boost/serialization/nvp.hpp>

// https://en.cppreference.com/w/cpp/language/unqualified_lookup
// https://en.cppreference.com/w/cpp/language/qualified_lookup
// https://en.cppreference.com/w/cpp/language/namespace

namespace yade { // Cannot have #include directive inside.


using boost::shared_ptr;
using std::cerr;
using std::cout;
using std::endl;
using std::invalid_argument;
using std::ios;
using std::ios_base;
using std::logic_error;
using std::make_pair;
using std::pair;
using std::runtime_error;
using std::setfill;
using std::setprecision;
using std::setw;
using std::string;
using std::swap;
using std::type_info;
using std::vector;

// in some cases, we want to initialize types that have no default constructor (OpenMPAccumulator, for instance)
// template specialization will help us here
template <typename EigenMatrix> EigenMatrix ZeroInitializer() { return EigenMatrix::Zero(); };
template <> int                             ZeroInitializer<int>();
template <> Real                            ZeroInitializer<Real>();

/* this was removed in eigen3, see http://forum.kde.org/viewtopic.php?f=74&t=90914 */
template <typename MatrixT> void Matrix_computeUnitaryPositive(const MatrixT& in, MatrixT* unitary, MatrixT* positive)
{
	assert(unitary);
	assert(positive);
	//Eigen::JacobiSVD<MatrixT> svd(in, Eigen::ComputeThinU | Eigen::ComputeThinV);
	Eigen::JacobiSVD<MatrixT> svd(in, Eigen::ComputeFullU | Eigen::ComputeFullV);
	MatrixT                   mU, mV, mS;
	mU = svd.matrixU();
	mV = svd.matrixV();
	mS = svd.singularValues().asDiagonal();

	*unitary  = mU * mV.adjoint();
	*positive = mV * mS * mV.adjoint();
}

template <typename MatrixT> void matrixEigenDecomposition(const MatrixT& m, MatrixT& mRot, MatrixT& mDiag)
{
	//assert(mRot); assert(mDiag);
	Eigen::SelfAdjointEigenSolver<MatrixT> a(m);
	mRot  = a.eigenvectors();
	mDiag = a.eigenvalues().asDiagonal();
}

/* convert Vector6r in the Voigt notation to corresponding 2nd order symmetric tensor (stored as Matrix3r)
	if strain is true, then multiply non-diagonal parts by .5
*/
template <typename Scalar> Matrix3<Scalar> voigt_toSymmTensor(const Vector6<Scalar>& v, bool strain = false)
{
	Real            k = (strain ? .5 : 1.);
	Matrix3<Scalar> ret;
	ret << v[0], k * v[5], k * v[4], k * v[5], v[1], k * v[3], k * v[4], k * v[3], v[2];
	return ret;
}
/* convert 2nd order tensor to 6-vector (Voigt notation), symmetrizing the tensor;
	if strain is true, multiply non-diagonal components by 2.
*/
template <typename Scalar> Vector6<Scalar> tensor_toVoigt(const Matrix3<Scalar>& m, bool strain = false)
{
	int             k = (strain ? 2 : 1);
	Vector6<Scalar> ret;
	ret << m(0, 0), m(1, 1), m(2, 2), k * .5 * (m(1, 2) + m(2, 1)), k * .5 * (m(2, 0) + m(0, 2)), k * .5 * (m(0, 1) + m(1, 0));
	return ret;
}

const Real
        NaN(std::numeric_limits<Real>::has_signaling_NaN
                    ? std::numeric_limits<Real>::signaling_NaN()
                    : (std::numeric_limits<Real>::has_quiet_NaN ? std::numeric_limits<Real>::quiet_NaN() : math::abs(Real(0) / Real(0))));

// void quaternionToEulerAngles (const Quaternionr& q, Vector3r& eulerAngles,Real threshold=1e-6f);
template <typename Scalar> void quaterniontoGLMatrix(const Eigen::Quaternion<Scalar>& q, Scalar m[16])
{
	Scalar w2 = 2. * q.w(), x2 = 2. * q.x(), y2 = 2. * q.y(), z2 = 2. * q.z();
	Scalar x2w = w2 * q.w(), y2w = y2 * q.w(), z2w = z2 * q.w();
	Scalar x2x = x2 * q.x(), y2x = y2 * q.x(), z2x = z2 * q.x();
	Scalar x2y = y2 * q.y(), y2y = y2 * q.y(), z2y = z2 * q.y();
	Scalar x2z = z2 * q.z(), y2z = y2 * q.z(), z2z = z2 * q.z();
	m[0]  = 1. - (y2y + z2z);
	m[4]  = y2x - z2w;
	m[8]  = z2x + y2w;
	m[12] = 0;
	m[1]  = y2x + z2w;
	m[5]  = 1. - (x2x + z2z);
	m[9]  = z2y - x2w;
	m[13] = 0;
	m[2]  = z2x - y2w;
	m[6]  = z2y + x2w;
	m[10] = 1. - (x2x + y2y);
	m[14] = 0;
	m[3]  = 0.;
	m[7]  = 0.;
	m[11] = 0.;
	m[15] = 1.;
}

// functions
template <typename Scalar> Scalar unitVectorsAngle(const Vector3<Scalar>& a, const Vector3<Scalar>& b) { return acos(a.dot(b)); }
// operators


/*
 * Mask
 */
#ifdef YADE_MASK_ARBITRARY
using mask_t = std::bitset<YADE_MASK_ARBITRARY_SIZE>;
bool   operator==(const mask_t& g, int i);
bool   operator==(int i, const mask_t& g);
bool   operator!=(const mask_t& g, int i);
bool   operator!=(int i, const mask_t& g);
mask_t operator&(const mask_t& g, int i);
mask_t operator&(int i, const mask_t& g);
mask_t operator|(const mask_t& g, int i);
mask_t operator|(int i, const mask_t& g);
bool   operator||(const mask_t& g, bool b);
bool   operator||(bool b, const mask_t& g);
bool   operator&&(const mask_t& g, bool b);
bool   operator&&(bool b, const mask_t& g);
#else
using mask_t = int;
#endif


} // namespace yade

#include <lib/high-precision/MathSerialization.hpp>
