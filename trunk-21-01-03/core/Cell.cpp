
#include <core/Cell.hpp>

namespace yade { // Cannot have #include directive inside.

CREATE_LOGGER(Cell);

void Cell::integrateAndUpdate(Real dt)
{
	//incremental displacement gradient
	_trsfInc = dt * velGrad;
	// total transformation; M = (Id+G).M = F.M
	trsf += _trsfInc * trsf;
	_invTrsf = trsf.inverse();
	// hSize contains colums with updated base vectors
	prevHSize        = hSize;
	_vGradTimesPrevH = velGrad * prevHSize;
	hSize += _trsfInc * hSize;
	if (hSize.determinant() == 0) {
		throw runtime_error("Cell is degenerate (zero volume).");
	}
	// lengths of transformed cell vectors, skew cosines
	Matrix3r Hnorm; // normalized transformed base vectors
	for (int i = 0; i < 3; i++) {
		Vector3r base(hSize.col(i));
		_size[i] = base.norm();
		base /= _size[i]; //base is normalized now
		Hnorm(0, i) = base[0];
		Hnorm(1, i) = base[1];
		Hnorm(2, i) = base[2];
	};
	// skew cosines
	for (int i = 0; i < 3; i++) {
		int i1 = (i + 1) % 3, i2 = (i + 2) % 3;
		// sin between axes is cos of skew
		_cos[i] = (Hnorm.col(i1).cross(Hnorm.col(i2))).squaredNorm();
	}
	// pure shear trsf: ones on diagonal
	_shearTrsf = Hnorm;
	// pure unshear transformation
	_unshearTrsf = _shearTrsf.inverse();
	// some parts can branch depending on presence/absence of shear
	_hasShear = (hSize(0, 1) != 0 || hSize(0, 2) != 0 || hSize(1, 0) != 0 || hSize(1, 2) != 0 || hSize(2, 0) != 0 || hSize(2, 1) != 0);
	// OpenGL shear matrix (used frequently)
	fillGlShearTrsfMatrix(_glShearTrsfMatrix);
}

void Cell::fillGlShearTrsfMatrix(double m[16])
{
	// it is for display only, assume conversion from Real to double.
	m[0]  = double(_shearTrsf(0, 0));
	m[4]  = double(_shearTrsf(0, 1));
	m[8]  = double(_shearTrsf(0, 2));
	m[12] = 0;
	m[1]  = double(_shearTrsf(1, 0));
	m[5]  = double(_shearTrsf(1, 1));
	m[9]  = double(_shearTrsf(1, 2));
	m[13] = 0;
	m[2]  = double(_shearTrsf(2, 0));
	m[6]  = double(_shearTrsf(2, 1));
	m[10] = double(_shearTrsf(2, 2));
	m[14] = 0;
	m[3]  = 0;
	m[7]  = 0;
	m[11] = 0;
	m[15] = 1;
}

} // namespace yade
