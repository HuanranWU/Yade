#pragma once

#include "RegularTriangulation.h"
#include <Eigen/Core>
#include <fstream>
#include <iostream>

namespace yade {
namespace CGT {
	class Tens {
	public:
		virtual ~Tens()                                      = default;
		virtual Real  operator()(int /*i*/, int /*j*/) const = 0;
		virtual Real& operator()(int /*i*/, int /*j*/)       = 0;
		virtual void  reset()                                = 0;
		Real          Norme2();
		Real          Norme();
		Real          Trace();
	};

	class Tenseur3 : public Tens {
	private:
		Eigen::Array<Real, 3, 3> T = Eigen::Array<Real, 3, 3>::Zero();

	public:
		Tenseur3() = default;
		Tenseur3(const Tenseur3& source);
		Tenseur3(Real a11, Real a12, Real a13, Real a21, Real a22, Real a23, Real a31, Real a32, Real a33);

		Tenseur3& operator=(const Tenseur3& source);
		Tenseur3& operator/=(Real d);
		Tenseur3& operator+=(const Tenseur3& source);
		Real      operator()(int i, int j) const override;
		Real&     operator()(int i, int j) override;
		void      reset() override;
	};

	class Tenseur_sym3 : public Tens {
	private:
		Eigen::Array<Real, 6, 1> T = Eigen::Array<Real, 6, 1>::Zero();

	public:
		Tenseur_sym3() = default;
		Tenseur_sym3(const Tenseur_sym3& source);
		Tenseur_sym3(const Tenseur3& source);
		Tenseur_sym3(Real a11, Real a22, Real a33, Real a12, Real a13, Real a23);

		Tenseur_sym3& operator=(const Tenseur_sym3& source);
		Tenseur_sym3& operator/=(Real d);
		Tenseur_sym3  Deviatoric() const; //retourne la partie d�viatoire
		Real          operator()(int i, int j) const override;
		Real&         operator()(int i, int j) override;
		void          reset() override;
	};


	CVector operator*(Tens& tens, CVector& vect);
	void    Somme(Tenseur3& result, CVector& v1, CVector& v2);
	void    NORMALIZE(CVector& v);

	std::ostream& operator<<(std::ostream& os, const Tenseur3& T);
	std::ostream& operator<<(std::ostream& os, const Tenseur_sym3& T);

} // namespace CGT
} // namespace yade
