#include "Tenseur3.h"
#include "RegularTriangulation.h"

namespace yade {
namespace CGT {

	void NORMALIZE(CVector& v) { v = v * (1.0 / sqrt(pow(v[0], 2) + pow(v[1], 2) + pow(v[2], 2))); }

	Real Tens::Norme2()
	{
		Real N = 0;
		for (int i = 1; i <= 3; i++)
			for (int j = 1; j <= 3; j++)
				N += pow(operator()(i, j), 2);
		return N;
	}
	Real Tens::Norme() { return sqrt(this->Norme2()); }

	Real Tens::Trace() { return this->operator()(1, 1) + this->operator()(2, 2) + this->operator()(3, 3); }

	CVector operator*(Tens& tens, CVector& vect)
	{
		const auto result = CVector(
		        tens(1, 1) * vect.x() + tens(1, 2) * vect.y() + tens(1, 3) * vect.z(),
		        tens(2, 1) * vect.x() + tens(2, 2) * vect.y() + tens(2, 3) * vect.z(),
		        tens(3, 1) * vect.x() + tens(3, 2) * vect.y() + tens(3, 3) * vect.z());
		return result;
	}

	Tenseur3::Tenseur3(const Tenseur3& source) { T = source.T; }

	Tenseur3::Tenseur3(Real a11, Real a12, Real a13, Real a21, Real a22, Real a23, Real a31, Real a32, Real a33)
	{
		T(0, 0) = a11;
		T(0, 1) = a12;
		T(0, 2) = a13;
		T(1, 0) = a21;
		T(1, 1) = a22;
		T(1, 2) = a23;
		T(2, 0) = a31;
		T(2, 1) = a32;
		T(2, 2) = a33;
	}


	Tenseur3& Tenseur3::operator=(const Tenseur3& source)
	{
		if (&source != this) {
			T = source.T;
		}
		return *this;
	}

	Tenseur3& Tenseur3::operator/=(Real d)
	{
		if (d != 0) {
			T /= d;
		}
		return *this;
	}

	Tenseur3& Tenseur3::operator+=(const Tenseur3& source)
	{
		T += source.T;
		return *this;
	}

	Real Tenseur3::operator()(int i, int j) const
	{
		if (i >= 1 && i <= 3 && j >= 1 && j <= 3) {
			return T(i - 1, j - 1);
		} else {
			throw logic_error("Tensor indexes are out of bounds!");
		}
	}

	Real& Tenseur3::operator()(int i, int j)
	{
		if (i >= 1 && i <= 3 && j >= 1 && j <= 3) {
			return T(i - 1, j - 1);
		} else {
			throw logic_error("Tensor indexes are out of bounds!");
		}
	}

	void Tenseur3::reset() { T.setZero(); }

	///////////		 Classe Tenseur_sym3		////////////
	Tenseur_sym3::Tenseur_sym3(const Tenseur_sym3& source) { T = source.T; }

	Tenseur_sym3::Tenseur_sym3(const Tenseur3& source)
	{
		for (int i = 1; i <= 3; i++) {
			T[i - 1] = source(i, i);
			for (int j = 3; j > i; j--)
				T[i + j] = (source(i, j) + source(j, i)) * 0.5;
		}
	}

	Tenseur_sym3::Tenseur_sym3(Real a11, Real a22, Real a33, Real a12, Real a13, Real a23)
	{
		T[0] = a11;
		T[1] = a22;
		T[2] = a33;
		T[3] = a12;
		T[4] = a13;
		T[5] = a23;
	}


	Tenseur_sym3& Tenseur_sym3::operator=(const Tenseur_sym3& source)
	{
		if (&source != this) {
			T = source.T;
		}
		return *this;
	}

	Tenseur_sym3& Tenseur_sym3::operator/=(Real d)
	{
		if (d != 0) {
			T /= d;
		}
		return *this;
	}

	Real Tenseur_sym3::operator()(int i, int j) const
	{
		if (i == j)
			return T[i - 1];
		else
			return T[i + j];
	}

	Real& Tenseur_sym3::operator()(int i, int j)
	{
		if (i == j)
			return T[i - 1];
		else
			return T[i + j];
	}

	Tenseur_sym3 Tenseur_sym3::Deviatoric() const //retourne la partie d�viatoire
	{
		Tenseur_sym3 temp(*this);
		Real         spheric = temp.Trace() / 3;
		temp(1, 1) -= spheric;
		temp(2, 2) -= spheric;
		temp(3, 3) -= spheric;
		return temp;
	}

	void Tenseur_sym3::reset() { T.setZero(); }

	void Somme(Tenseur3& result, CVector& v1, CVector& v2)
	{
		result(1, 1) += v1.x() * v2.x();
		result(1, 2) += v1.x() * v2.y();
		result(1, 3) += v1.x() * v2.z();
		result(2, 1) += v1.y() * v2.x();
		result(2, 2) += v1.y() * v2.y();
		result(2, 3) += v1.y() * v2.z();
		result(3, 1) += v1.z() * v2.x();
		result(3, 2) += v1.z() * v2.y();
		result(3, 3) += v1.z() * v2.z();
	}


	// Fonctions d'�criture
	std::ostream& operator<<(std::ostream& os, const Tenseur3& T)
	{
		for (int j = 1; j < 4; j++) {
			for (int i = 1; i < 4; i++) {
				os << T(j, i) << " ";
			}
			os << std::endl;
		}
		return os;
	}

	std::ostream& operator<<(std::ostream& os, const Tenseur_sym3& T)
	{
		for (int j = 1; j < 4; j++) {
			for (int i = 1; i < 4; i++) {
				os << T(j, i) << " ";
			}
			os << std::endl;
		}
		return os;
	}

} // namespace CGT
} // namespace yade