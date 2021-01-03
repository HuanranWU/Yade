// 2019 © William Chèvremont <william.chevremont@univ-grenoble-alpes.fr>

#include "LubricationWithPotential.hpp"
#include <boost/python/call_method.hpp>

namespace yade {

YADE_PLUGIN((Law2_ScGeom_PotentialLubricationPhys)(GenericPotential)(CundallStrackPotential)(CundallStrackAdhesivePotential)(LinExponentialPotential))

bool Law2_ScGeom_PotentialLubricationPhys::go(shared_ptr<IGeom>& iGeom, shared_ptr<IPhys>& iPhys, Interaction* interaction)
{
	// Physic & Geometry
	LubricationPhys* phys = static_cast<LubricationPhys*>(iPhys.get());
	ScGeom*          geom = static_cast<ScGeom*>(iGeom.get());

	if (!phys || !geom) {
		LOG_ERROR("Wrong physics and/or geometry!");
		return false;
	}

	// geometric parameters
	Real a((geom->radius1 + geom->radius2) / 2.);

	// End-of Interaction condition
	if (-geom->penetrationDepth > MaxDist * a) {
		return false;
	}

	// inititalization
	if (phys->u == -1.) {
		phys->u     = -geom->penetrationDepth;
		phys->delta = math::log(phys->u);
	}

	// Normal part
	if (!solve_normalForce(-geom->penetrationDepth / a, scene->dt * a * phys->kn / (phys->nun * 3. / 2.), *phys)) {
		LOG_ERROR("Unable to determine normal forces. MAYDAY MAYDAY MAYDAY!");
		return false;
	}

	potential->applyPotential(phys->u, *phys, geom->normal);                                      // Set contactForce, potentialForce, contact.
	phys->normalLubricationForce = phys->kn * a * phys->prevDotU * geom->normal;                  // From implicit formulation. Prevent computing divisions.
	phys->normalForce            = phys->kn * (-geom->penetrationDepth - phys->u) * geom->normal; // From regularization expression.

	// Get bodies properties
	Body::id_t             id1 = interaction->getId1();
	Body::id_t             id2 = interaction->getId2();
	const shared_ptr<Body> b1  = Body::byId(id1, scene);
	const shared_ptr<Body> b2  = Body::byId(id2, scene);
	State*                 s1  = b1->state.get();
	State*                 s2  = b2->state.get();

	// Shear and torques
	Vector3r C1 = Vector3r::Zero();
	Vector3r C2 = Vector3r::Zero();
	computeShearForceAndTorques_log(phys, geom, s1, s2, C1, C2);

	// Apply!
	scene->forces.addForce(id1, phys->normalForce + phys->shearForce);
	scene->forces.addTorque(id1, C1);

	scene->forces.addForce(id2, -(phys->normalForce + phys->shearForce));
	scene->forces.addTorque(id2, C2);

	return true;
}

CREATE_LOGGER(Law2_ScGeom_PotentialLubricationPhys);

bool Law2_ScGeom_PotentialLubricationPhys::solve_normalForce(Real const& un, Real const& dt, LubricationPhys& phys)
{
	// Init
	Real const& pDelta(phys.delta);
	Real const& a(phys.a);
	Real const  ga(phys.kn * a);
	Real        d1(pDelta - 1.), d2(pDelta + 1.), d;

	auto objf = [&, this](Real delta) -> Real {
		return potential->potential(a * math::exp(delta), phys) / ga + (1. - math::exp(pDelta - delta)) / dt - un + math::exp(delta);
	};
	Real F1(objf(d1)), F2(objf(d2)), F;

	// Seek to interval containing the zero
	Real inc = (F1 < 0.) ? 1. : -1;
	inc      = (F1 < F2) ? inc : -inc;

	while (F1 * F2 >= 0 && math::isfinite(F1) && math::isfinite(F2)) {
		LOG_TRACE("d1=" << d1 << " d2=" << d2 << " F1=" << F1 << " F2=" << F2);
		d1 += inc;
		d2 += inc;
		F1 = objf(d1);
		F2 = objf(d2);
	}

	if (!math::isfinite(F1) || !math::isfinite(F2)) {
		// Reset and search other way
		LOG_DEBUG("Wrong direction");
		d1  = pDelta - 1.;
		d2  = pDelta + 1.;
		F1  = objf(d1);
		F2  = objf(d2);
		inc = -inc;

		while (F1 * F2 >= 0 && math::isfinite(F1) && math::isfinite(F2)) {
			LOG_TRACE("d1=" << d1 << " d2=" << d2 << " F1=" << F1 << " F2=" << F2);
			d1 += inc;
			d2 += inc;
			F1 = objf(d1);
			F2 = objf(d2);
		}
	}

	if (!math::isfinite(F1) || !math::isfinite(F2)) {
		LOG_ERROR("Unable to find a start point. Abandon. d1=" << d1 << " d2=" << d2 << " F1=" << F1 << " F2=" << F2);
		return false;
	}

	// Iterate to find a solution
	uint i(MaxIter);
	do {
		if (F1 * F2 >= 0) {
			LOG_ERROR("Boundaries have the same sign. Algorithm FAIL.");
			return false;
		}

		d = (d1 + d2) / 2.;
		F = objf(d);

		if (!math::isfinite(F)) {
			LOG_ERROR("Objective function return non-real value. Abandon. d=" << d << " F=" << F);
			return false;
		}

		if (math::abs(F) < SolutionTol)
			break;

		if (F * F1 < 0) {
			d2 = d;
			F2 = F;
		} else {
			d1 = d;
			F1 = F;
		}
	} while (--i);

	// Apply
	Real up       = math::exp(d);
	phys.delta    = d;
	phys.u        = a * math::exp(d);
	phys.prevDotU = un - up - potential->potential(phys.u, phys) / ga; // dotu'/u'

	return true;
}

Real GenericPotential::potential(Real const&, LubricationPhys const&) const { return 0; }

void GenericPotential::applyPotential(Real const&, LubricationPhys& phys, Vector3r const&)
{
	phys.normalContactForce   = Vector3r::Zero();
	phys.normalPotentialForce = Vector3r::Zero();
	phys.contact              = false;
}

CREATE_LOGGER(GenericPotential);

Real CundallStrackPotential::potential(Real const& u, LubricationPhys const& phys) const { return math::min(0., -alpha * phys.kn * (phys.eps * phys.a - u)); }

void CundallStrackPotential::applyPotential(Real const& u, LubricationPhys& phys, Vector3r const& n)
{
	phys.contact              = u < phys.eps * phys.a;
	phys.normalContactForce   = (phys.contact) ? Vector3r(-alpha * phys.kn * (phys.eps * phys.a - u) * n) : Vector3r::Zero();
	phys.normalPotentialForce = Vector3r::Zero();
}

CREATE_LOGGER(CundallStrackPotential);

Real CundallStrackAdhesivePotential::potential(Real const& u, LubricationPhys const& phys) const
{
	Real ladh((phys.contact) ? fadh / phys.kn : 0.);

	if (u < phys.eps * phys.a + ladh)
		return -alpha * phys.kn * (phys.eps * phys.a - u);
	return 0;
}

void CundallStrackAdhesivePotential::applyPotential(Real const& u, LubricationPhys& phys, Vector3r const& n)
{
	Real ladh((phys.contact) ? fadh / phys.kn : 0.);

	phys.contact              = u < phys.eps * phys.a + ladh;
	phys.normalContactForce   = (phys.contact) ? Vector3r(-alpha * phys.kn * (phys.eps * phys.a - u) * n) : Vector3r::Zero();
	phys.normalPotentialForce = Vector3r::Zero();
}

CREATE_LOGGER(CundallStrackAdhesivePotential);

Real LinExponentialPotential::potential(Real const& u, LubricationPhys const& phys) const
{
	return math::min(0., -alpha * phys.kn * (phys.eps * phys.a - u)) + LinExpPotential(u / phys.a);
}

void LinExponentialPotential::applyPotential(Real const& u, LubricationPhys& phys, Vector3r const& n)
{
	phys.contact              = u < phys.eps * phys.a;
	phys.normalContactForce   = (phys.contact) ? Vector3r(-alpha * phys.kn * (phys.eps * phys.a - u) * n) : Vector3r::Zero();
	phys.normalPotentialForce = LinExpPotential(u / phys.a) * n;
}

void LinExponentialPotential::setParameters(Real const& x_0, Real const& x_e, Real const& k_)
{
	if (x_0 >= x_e)
		throw std::runtime_error("x0 must be lower than xe!");
	if (x_e == 0.)
		throw std::runtime_error("Extremum can't be at the origin.");

	x0 = x_0;
	xe = x_e;
	k  = k_;
	F0 = LinExpPotential(0);
	Fe = LinExpPotential(xe);
}

void LinExponentialPotential::computeParametersFromF0(Real const& F_0, Real const& x_e, Real const& k_)
{
	Real rho = x_e * x_e * +4. * F_0 * x_e / k_;

	if (rho <= 0)
		throw std::runtime_error("xe^2 + 4F0 xe/k must be positive!");
	if (x_e == 0.)
		throw std::runtime_error("Extremum can't be at the origin.");

	k  = k_;
	xe = x_e;
	F0 = F_0;
	x0 = (xe - math::sqrt(rho)) / 2.;
	Fe = LinExpPotential(xe);
}

void LinExponentialPotential::computeParametersFromF0Fe(Real const& x_e, Real const& F_e, Real const& F_0)
{
	using math::abs; // when used inside function it does not leak - it is safe.
	if (x_e == 0.)
		throw std::runtime_error("Extremum can't be at the origin.");
	if (F_e * F_0 < 0) {
		if (x_e < 0)
			throw std::runtime_error("When xe < 0, F0 and Fe must be same sign!");
		if (abs(F_e) <= 1.5 * abs(F_0))
			throw std::runtime_error("When F0 and Fe are different sign, you must ensure |Fe| > 1.5|F0|");
	} else {
		if (abs(F_e) <= abs(F_0))
			throw std::runtime_error("When F0 and F0 are same sign, you must ensure |Fe| > |F0|");
	}

	xe = x_e;

	k  = (F_e / (xe * math::exp(Real(-1))));
	x0 = 0.;
	F0 = F_0;
	Fe = F_e;

	for (int i(0); i < 100; i++) {
		x0 = (xe - math::sqrt(xe * xe + 4. * F0 * xe / k)) / 2.;
		k  = Fe * xe / ((xe - x0) * (xe - x0) * exp(-xe / (xe - x0)));

		// Iteration quit if relative difference is below 1%.
		if (math::sqrt(
		            (LinExpPotential(0) - F0) * (LinExpPotential(0) - F0) / (F0 * F0)
		            + (LinExpPotential(xe) - Fe) * (LinExpPotential(xe) - Fe) / (Fe * Fe))
		    < 0.01)
			break;
	}
}

Real LinExponentialPotential::LinExpPotential(Real const& u_) const { return k * ((xe - x0) / xe) * (u_ - x0) * math::exp(-u_ / (xe - x0)); }

CREATE_LOGGER(LinExponentialPotential);

} // namespace yade
