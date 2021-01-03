// many thanks to http://codesuppository.blogspot.com/2006_06_01_archive.html
// the code written after http://www.amillionpixels.us/bestfitobb.cpp
// which is MIT-licensed

#include <lib/base/Logging.hpp>
#include <lib/base/Math.hpp>
#include <lib/high-precision/Constants.hpp>
#include <lib/pyutil/doc_opts.hpp>

CREATE_CPP_LOCAL_LOGGER("_packObb.cpp");

template <typename Scalar> Eigen::Matrix<Scalar, 3, 3> matrixFromEulerAnglesXYZ(Scalar x, Scalar y, Scalar z)
{
	Eigen::Matrix<Scalar, 3, 3> m;
	m = Eigen::AngleAxis<Scalar>(x, Eigen::Matrix<Scalar, 3, 1>::UnitX()) * Eigen::AngleAxis<Scalar>(y, Eigen::Matrix<Scalar, 3, 1>::UnitY())
	        * Eigen::AngleAxis<Scalar>(z, Eigen::Matrix<Scalar, 3, 1>::UnitZ());
	return m;
}

#if EIGEN_VERSION_AT_LEAST(3, 3, 0) // this is for ubuntu 16.04 xenial
// https://eigen.tuxfamily.org/dox/unsupported/index.html : contributions from various users. They are provided "as is", without any support.
// better to use that, than reinvent own Euler angles.
#include <unsupported/Eigen/EulerAngles>

inline yade::Matrix3r makeFromEulerAngle(::yade::Real x, ::yade::Real y, ::yade::Real z)
{
	yade::Matrix3r ret = Eigen::EulerAngles<::yade::Real, Eigen::EulerSystemXYZ>(x, y, z).toRotationMatrix();
	assert(ret == matrixFromEulerAnglesXYZ<::yade::Real>(x, y, z));
	return ret;
}
#else
inline yade::Matrix3r makeFromEulerAngle(::yade::Real x, ::yade::Real y, ::yade::Real z) { return matrixFromEulerAnglesXYZ<::yade::Real>(x, y, z); }
#endif

namespace yade { // Cannot have #include directive inside.

// compute minimum bounding for a cloud of points

// returns volume
Real computeOBB(const std::vector<Vector3r>& pts, const Matrix3r& rot, Vector3r& center, Vector3r& halfSize)
{
	const Real inf = std::numeric_limits<Real>::infinity();
	Vector3r   mn(inf, inf, inf), mx(-inf, -inf, -inf);
	FOREACH(const Vector3r& pt, pts)
	{
		Vector3r ptT = rot * pt;
		mn           = mn.cwiseMin(ptT);
		mx           = mx.cwiseMax(ptT);
	}
	halfSize = .5 * (mx - mn);
	center   = .5 * (mn + mx);
	return 8 * halfSize[0] * halfSize[1] * halfSize[2];
}

void bestFitOBB(const std::vector<Vector3r>& pts, Vector3r& center, Vector3r& halfSize, Quaternionr& rot)
{
	Vector3r angle0(Vector3r::Zero()), angle(Vector3r::Zero());
	Vector3r center0;
	Vector3r halfSize0;
	Real     bestVolume = std::numeric_limits<Real>::infinity();
	Real     sweep      = Mathr::PI / 4;
	Real     steps      = 7.;
	while (sweep >= Mathr::PI / 180.) {
		bool found    = false;
		Real stepSize = sweep / steps;
		for (Real x = angle0[0] - sweep; x <= angle0[0] + sweep; x += stepSize) {
			for (Real y = angle0[1] - sweep; y < angle0[1] + sweep; y += stepSize) {
				for (Real z = angle0[2] - sweep; z < angle0[2] + sweep; z += stepSize) {
					Matrix3r rot0(makeFromEulerAngle(x, y, z));
					Real     volume = computeOBB(pts, rot0, center0, halfSize0);
					if (volume < bestVolume) {
						bestVolume = volume;
						angle      = angle0;
						// set return values, in case this will be really the best fit
						rot      = Quaternionr(rot0);
						rot      = rot.conjugate();
						center   = center0;
						halfSize = halfSize0;
						found    = true;
					}
				}
			}
		}
		if (found) {
			angle0 = angle;
			sweep *= .5;
		} else
			return;
	}
}

boost::python::tuple bestFitOBB_py(const boost::python::tuple& _pts)
{
	int l = boost::python::len(_pts);
	if (l <= 1)
		throw std::runtime_error("Cloud must have at least 2 points.");
	std::vector<Vector3r> pts;
	pts.resize(l);
	for (int i = 0; i < l; i++)
		pts[i] = boost::python::extract<Vector3r>(_pts[i]);
	Quaternionr rot;
	Vector3r    halfSize, center;
	bestFitOBB(pts, center, halfSize, rot);
	return boost::python::make_tuple(center, halfSize, rot);
}

} // namespace yade

// BOOST_PYTHON_MODULE cannot be inside yade namespace, it has 'extern "C"' keyword, which strips it out of any namespaces.
BOOST_PYTHON_MODULE(_packObb)
try {
	YADE_SET_DOCSTRING_OPTS;
	boost::python::scope().attr("__doc__") = "Computation of oriented bounding box for cloud of points.";
	boost::python::def(
	        "cloudBestFitOBB",
	        yade::bestFitOBB_py,
	        "Return (Vector3 center, Vector3 halfSize, Quaternion orientation) of\nbest-fit oriented bounding-box for given tuple of points\n(uses "
	        "brute-force velome minimization, do not use for very large clouds).");

} catch (...) {
	LOG_FATAL("Importing this module caused an exception and this module is in an inconsistent state now.");
	PyErr_Print();
	PyErr_SetString(PyExc_SystemError, __FILE__);
	boost::python::handle_exception();
	throw;
}
