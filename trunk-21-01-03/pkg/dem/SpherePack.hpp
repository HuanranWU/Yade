// © 2009 Václav Šmilauer <eudoxos@arcig.cz>

#pragma once

#include <lib/base/Logging.hpp>
#include <lib/base/Math.hpp>

namespace yade { // Cannot have #include directive inside.

/*! Class representing geometry of spherical packing, with some utility functions. */
class SpherePack {
	// return coordinate wrapped to x0…x1, relative to x0; don't care about period
	// copied from PeriodicInsertionSortCollider
	Real cellWrapRel(const Real x, const Real x0, const Real x1) const;
	Real periPtDistSq(const Vector3r& p1, const Vector3r& p2) const;
	struct ClumpInfo {
		int      clumpId;
		Vector3r center;
		Real     rad;
		int      minId, maxId;
	};

public:
	enum class e_Mode { UNDEFINED, RDIST_RMEAN, RDIST_NUM, RDIST_PSD };
	struct Sph {
		Vector3r c;
		Real     r;
		int      clumpId;
		Sph(const Vector3r& _c, Real _r, int _clumpId = -1)
		        : c(_c)
		        , r(_r)
		        , clumpId(_clumpId) {};
		boost::python::tuple asTuple() const;
		boost::python::tuple asTupleNoClump() const;
	};
	std::vector<Sph> pack;
	Vector3r         cellSize;
	Real             appliedPsdScaling; //a scaling factor that can be applied on size distribution
	bool             isPeriodic;
	SpherePack()
	        : cellSize(Vector3r::Zero())
	        , appliedPsdScaling(1.)
	        , isPeriodic(0) {};
	SpherePack(const boost::python::list& l)
	        : cellSize(Vector3r::Zero())
	{
		fromList(l);
	}
	// add single sphere
	void add(const Vector3r& c, Real r);

	// I/O
	void                fromList(const boost::python::list& l);
	void                fromLists(const vector<Vector3r>& centers, const vector<Real>& radii); // used as ctor in python
	boost::python::list toList() const;
	void                fromFile(const string& file);
	void                toFile(const string& file) const;
	void                fromSimulation();

	// random generation; if num<0, insert as many spheres as possible; if porosity>0, recompute meanRadius (porosity>0.65 recommended) and try generating this porosity with num spheres.
	long makeCloud(
	        Vector3r            min,
	        Vector3r            max,
	        Real                rMean          = -1,
	        Real                rFuzz          = 0,
	        int                 num            = -1,
	        bool                periodic       = false,
	        Real                porosity       = -1,
	        const vector<Real>& psdSizes       = vector<Real>(),
	        const vector<Real>& psdCumm        = vector<Real>(),
	        bool                distributeMass = false,
	        int                 seed           = 0,
	        Matrix3r            hSize          = Matrix3r::Zero());
	// return number of piece for x in piecewise function defined by cumm with non-decreasing elements ∈(0,1)
	// norm holds normalized coordinate withing the piece
	int psdGetPiece(Real x, const vector<Real>& cumm, Real& norm) const;

	// interpolate a variable with power distribution (exponent -3) between two margin values, given uniformly distributed x∈(0,1)
	Real pow3Interp(Real x, Real a, Real b) const;

	// generate packing of clumps, selected with equal probability
	// periodic boundary is supported
	long makeClumpCloud(
	        const Vector3r& mn, const Vector3r& mx, const vector<shared_ptr<SpherePack>>& clumps, bool periodic = false, int num = -1, int seed = -1);

	// periodic repetition
	void cellRepeat(Vector3i count);
	void cellFill(Vector3r volume);

	// spatial characteristics
	Vector3r             dim() const;
	boost::python::tuple aabb_py() const;
	void                 aabb(Vector3r& mn, Vector3r& mx) const;
	Vector3r             midPt() const;
	Real                 relDensity() const;
	boost::python::tuple psd(int bins = 10, bool mass = false) const;
	bool                 hasClumps() const;
	boost::python::tuple getClumps() const;

	// transformations
	void translate(const Vector3r& shift);
	void rotate(const Vector3r& axis, Real angle);
	void rotateAroundOrigin(const Quaternionr& rot);
	void scale(Real scale);

	// iteration
	size_t               len() const;
	boost::python::tuple getitem(size_t idx) const;
	struct _iterator {
		const SpherePack& sPack;
		size_t            pos;
		_iterator(const SpherePack& _sPack)
		        : sPack(_sPack)
		        , pos(0)
		{
		}
		_iterator            iter();
		boost::python::tuple next();
	};
	SpherePack::_iterator getIterator() const;
	DECLARE_LOGGER;
};

} // namespace yade
