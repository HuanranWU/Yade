// 2019 © William Chèvremont <william.chevremont@univ-grenoble-alpes.fr>

#include "Lubrication.hpp"

namespace yade {

class GenericPotential : public Serializable {
public:
	/* This is where the magic happens.
         * This function needs to be reimplemented by childs class.
         * @param u distances between surfaces
         * @param a mean radius
         * @param phys Actual physics on which potential is computed
         * @return The total force from potential (contact + potential)
         */
	virtual Real potential(Real const& u, LubricationPhys const& phys) const;
	virtual void applyPotential(Real const& u, LubricationPhys& phys, Vector3r const& n);
	// clang-format off
	YADE_CLASS_BASE_DOC_ATTRS_CTOR_PY(GenericPotential,Serializable,
		"Generic class for potential representation in PotentialLubrication law. Don't do anything. If set as potential, the result will be a lubrication-only simulation.",
		// ATTRS
		, // CTOR
		, // PY
	);
	// clang-format on
	//        REGISTER_CLASS_AND_BASE(GenericPotential,Serializable);
	DECLARE_LOGGER;
};
REGISTER_SERIALIZABLE(GenericPotential);

class Law2_ScGeom_PotentialLubricationPhys : public Law2_ScGeom_ImplicitLubricationPhys {
public:
	bool go(shared_ptr<IGeom>& iGeom, shared_ptr<IPhys>& iPhys, Interaction* interaction);

	/*
             * This function solve the lubricated interaction with provided potential. It set:
             * phys.delta = log(u/a) = log(u'),
             * phys.u = u,
             * phys.prevDotU = dotu'/u'
             * @param un dimentionless geometric distance (un/a)
             * @param dt dimentionless time step
             * @param a dimentionnal mean radius
             * @param phys Physics
             * @return false if something went wrong.
             */
	bool solve_normalForce(Real const& un, Real const& dt, LubricationPhys& phys);

	FUNCTOR2D(GenericSpheresContact, LubricationPhys);

	// clang-format off
	YADE_CLASS_BASE_DOC_ATTRS_CTOR_PY(Law2_ScGeom_PotentialLubricationPhys,
		Law2_ScGeom_ImplicitLubricationPhys,
		"Material law for lubrication + potential between two spheres. The potential model include contact. This material law will solve the system with lubrication and the provided potential.",
		// ATTR
		((shared_ptr<GenericPotential>,potential, new GenericPotential(), ,"Physical potential force between spheres."))
		,// CTOR
		,// PY
	);
	// clang-format on
	DECLARE_LOGGER;
};
REGISTER_SERIALIZABLE(Law2_ScGeom_PotentialLubricationPhys);

class CundallStrackPotential : public GenericPotential {
public:
	Real potential(Real const& u, LubricationPhys const& phys) const;
	void applyPotential(Real const& u, LubricationPhys& phys, Vector3r const& n);
	// clang-format off
        YADE_CLASS_BASE_DOC_ATTRS_CTOR_PY(CundallStrackPotential,GenericPotential,
		"Potential with only Cundall-and-Strack-like contact.",
		// ATTRS
		((Real, alpha, 1, , "Bulk-to-roughness stiffness ratio"))
		, // CTOR
		, // PY
        );
	// clang-format on
	DECLARE_LOGGER;
};
REGISTER_SERIALIZABLE(CundallStrackPotential)

class CundallStrackAdhesivePotential : public CundallStrackPotential {
public:
	Real potential(Real const& u, LubricationPhys const& phys) const;
	void applyPotential(Real const& u, LubricationPhys& phys, Vector3r const& n);
	// clang-format off
	YADE_CLASS_BASE_DOC_ATTRS_CTOR_PY(CundallStrackAdhesivePotential, CundallStrackPotential,
		"CundallStrack model with adhesive part. Contact is created when $u/a-\\varepsilon < 0$ and released when $u/a-\\varepsilon > l_{adh}$, where $l_{adh} = f_{adh}/k_n$. This lead to an hysteretic attractive part.",
		// ATTRS
		((Real, fadh, 0, , "Adhesion force."))
		, // CTOR
		, // PY
        );
	// clang-format on
	DECLARE_LOGGER;
};
REGISTER_SERIALIZABLE(CundallStrackAdhesivePotential)

class LinExponentialPotential : public CundallStrackPotential {
public:
	Real potential(Real const& u, LubricationPhys const& phys) const;
	void applyPotential(Real const& u, LubricationPhys& phys, Vector3r const& n);

	Real LinExpPotential(Real const& u_) const;
	void setParameters(Real const& x_0, Real const& xe, Real const& k);
	void computeParametersFromF0(Real const& F0, Real const& xe, Real const& k);
	void computeParametersFromF0Fe(Real const& x_e, Real const& Fe, Real const& F0);

	// clang-format off
	YADE_CLASS_BASE_DOC_ATTRS_CTOR_PY(LinExponentialPotential, CundallStrackPotential,
		"LinExponential Potential with only Cundall-and-Strack-like contact. The LinExponential potential formula is $F(u) = \\frac{k*(x_e-x_0)}{x_e}(u/a-x_0)\\exp\\left(\\frac{-(u/a)}{x_e-x_0}\\right)$. Where $k$ is the slope at the origin, $x_0$ is the position where the potential cross $0$ and $x_e$ is the position of the extremum. ",
		// ATTRS
		((Real, x0, 0, Attr::readonly, "Equilibrium distance. Potential force is 0 at $x_0$ (LinExponential)"))
		((Real, xe, 1, Attr::readonly, "Extremum position. Position of local max/min of force. (LinExponential)"))
		((Real, k, 1, Attr::readonly, "Slope at the origin (stiffness). (LinExponential)"))
		((Real, F0, 1, Attr::readonly, "Force at contact. Force when $F_0 = F(u=0)$ (LinExponential)"))
		((Real, Fe, 1, Attr::readonly, "Extremum force. Value of force at extremum. (LinExponential)"))
		, // CTOR
		, // PY
		.def("setParameters", &LinExponentialPotential::setParameters, py::args("x0", "xe", "k"),"Set parameters of the potential")
		.def("computeParametersFromF0", &LinExponentialPotential::computeParametersFromF0, py::args("F0", "xe", "k"),  "Set parameters of the potential, with $k$ computed from $F_0$")
		.def("computeParametersFromF0Fe", &LinExponentialPotential::computeParametersFromF0Fe, py::args("xe", "Fe", "F0"), "Set parameters of the potential, with $k$ and $x_0$ computed from $F_0$ and $F_e$")
		.def("potential", &LinExponentialPotential::LinExpPotential, py::args("u"), "Get potential value at any point.")
        );
	// clang-format on
	DECLARE_LOGGER;
};
REGISTER_SERIALIZABLE(LinExponentialPotential)

} // namespace yade
