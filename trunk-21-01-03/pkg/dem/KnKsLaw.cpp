#ifdef YADE_POTENTIAL_PARTICLES
#include "KnKsLaw.hpp"
#include <lib/high-precision/Constants.hpp>
#include <core/Scene.hpp>
//#include <pkg/dem/ScGeom.hpp>
#include <core/Omega.hpp>
#include <pkg/dem/PotentialParticle.hpp>


namespace yade { // Cannot have #include directive inside.


YADE_PLUGIN((Law2_SCG_KnKsPhys_KnKsLaw)(Ip2_FrictMat_FrictMat_KnKsPhys)(KnKsPhys));

/* ***************************************************************************************************************************** */
/** Function which returns the ratio between the number of sliding contacts to the total number at a given time */
Real Law2_SCG_KnKsPhys_KnKsLaw::ratioSlidingContacts()
{
	Real ratio(0);
	int  count(0);
	FOREACH(const shared_ptr<Interaction>& I, *scene->interactions)
	{
		if (!I->isReal())
			continue;
		KnKsPhys* phys = dynamic_cast<KnKsPhys*>(I->phys.get()); /* contact physics */
		if (phys->isSliding) {
			ratio += 1;
		}
		count++;
	}
	ratio /= count;
	return ratio;
}


/* ***************************************************************************************************************************** */
/** Energy calculations */
Real Law2_SCG_KnKsPhys_KnKsLaw::getPlasticDissipation() { return (Real)plasticDissipation; }
void Law2_SCG_KnKsPhys_KnKsLaw::initPlasticDissipation(Real initVal)
{
	plasticDissipation.reset();
	plasticDissipation += initVal;
}
Real Law2_SCG_KnKsPhys_KnKsLaw::getnormDampDissip() { return (Real)normDampDissip; }
Real Law2_SCG_KnKsPhys_KnKsLaw::getshearDampDissip() { return (Real)shearDampDissip; }

Real Law2_SCG_KnKsPhys_KnKsLaw::elasticEnergy()
{
	Real energy = 0;
	FOREACH(const shared_ptr<Interaction>& I, *scene->interactions)
	{
		if (!I->isReal())
			continue;
		KnKsPhys* phys = dynamic_cast<KnKsPhys*>(I->phys.get()); /* contact physics */
		if (phys) {
			//FIXME: Check whether we need to add the viscous forces to the elastic ones below, since the normalForce is reduced by normalViscous
			//Currently, damping in the shear direction is deactivated, so shearViscous=Vector3r(0,0,0) in all cases.
			/* reduced+viscous */ //energy += 0.5*( (phys->normalForce + phys->normalViscous).squaredNorm()/phys->kn + (phys->shearForce + phys->shearViscous).squaredNorm()/phys->ks);
			/* reduced*/ energy += 0.5 * ((phys->normalForce).squaredNorm() / phys->kn + (phys->shearForce).squaredNorm() / phys->ks);
		}
	}
	return energy;
}


/* ***************************************************************************************************************************** */
/** Law2_SCG_KnKsPhys_KnKsLaw */
CREATE_LOGGER(Law2_SCG_KnKsPhys_KnKsLaw);

bool Law2_SCG_KnKsPhys_KnKsLaw::go(shared_ptr<IGeom>& ig, shared_ptr<IPhys>& ip, Interaction* contact)
{
	const Real&        dt         = scene->dt;
	int                id1        = contact->getId1();
	int                id2        = contact->getId2();
	ScGeom*            geom       = static_cast<ScGeom*>(ig.get());
	KnKsPhys*          phys       = static_cast<KnKsPhys*>(ip.get());
	State*             de1        = Body::byId(id1, scene)->state.get();
	State*             de2        = Body::byId(id2, scene)->state.get();
	Shape*             shape1     = Body::byId(id1, scene)->shape.get();
	Shape*             shape2     = Body::byId(id2, scene)->shape.get();
	PotentialParticle* s1         = static_cast<PotentialParticle*>(shape1);
	PotentialParticle* s2         = static_cast<PotentialParticle*>(shape2);
	Vector3r&          shearForce = phys->shearForce;
	Real               un         = geom->penetrationDepth;
	//TRVAR3(geom->penetrationDepth,de1->se3.position,de2->se3.position);

	/* Need to initialise in python.  In the 1st time step.  All the particles in contact (controlled by initialOverlap) are identified.  The interactions are set to tensile and cohesive (tensionBroken = false and cohesionBroken = false).  If there is no initial tension or cohesion, the contact law is run in a tensionless or cohesionless mode */

	if (un < 0.0) {
		if (neverErase) {
			phys->shearForce    = Vector3r::Zero();
			phys->normalForce   = Vector3r::Zero();
			phys->normalViscous = Vector3r::Zero();
			phys->shearViscous  = Vector3r::Zero();
			geom->normal        = Vector3r::Zero();
			phys->tensionBroken = true;
		} else {
			scene->interactions->requestErase(id1, id2);
			return false;
		}
		return true;
	}

	//Vector3r shearForceBeforeRotate = shearForce;
	//	Vector3r shiftVel = Vector3r(0,0,0); //scene->isPeriodic ? (Vector3r)((scene->cell->velGrad*scene->cell->Hsize)*Vector3r((Real) contact->cellDist[0],(Real) contact->cellDist[1],(Real) contact->cellDist[2])) : Vector3r::Zero();
	geom->rotate(shearForce); //AndGetShear(shearForce,phys->prevNormal,de1,de2,dt,shiftVel,/*avoid ratcheting*/false);
	//Vector3r shearForceAfterRotate = shearForce;
	//Linear elasticity giving "trial" shear force

	Vector3r shiftVel = scene->isPeriodic ? Vector3r(scene->cell->velGrad * scene->cell->hSize * contact->cellDist.cast<Real>()) : Vector3r::Zero();
	Vector3r shift2   = scene->isPeriodic ? Vector3r(scene->cell->hSize * contact->cellDist.cast<Real>()) : Vector3r::Zero();

	const shared_ptr<Body>&b1 = Body::byId(id1, scene), b2 = Body::byId(id2, scene);
	//erase the interaction when aAbB shows separation, otherwise keep it to be able to store previous separating plane for fast detection of separation
	//	Vector3r shift2=scene->cell->hSize*I->cellDist.cast<Real>();
	if (b1->bound->min[0] >= b2->bound->max[0] + shift2[0] || b1->bound->min[1] >= b2->bound->max[1] + shift2[1]
	    || b1->bound->min[2] >= b2->bound->max[2] + shift2[2] || b2->bound->min[0] + shift2[0] >= b1->bound->max[0]
	    || b2->bound->min[1] + shift2[1] >= b1->bound->max[1] || b2->bound->min[2] + shift2[2] >= b1->bound->max[2]) {
		return false;
	}

	//	Vector3r shift2(0,0,0);
	Vector3r incidentV      = geom->getIncidentVel(de1, de2, dt, shift2, shiftVel, /*preventGranularRatcheting*/ false);
	Vector3r incidentVn     = geom->normal.dot(incidentV) * geom->normal; // contact normal velocity
	Vector3r incidentVs     = incidentV - incidentVn;                     // contact shear velocity
	Vector3r shearIncrement = incidentVs * dt;
	phys->shearDir          = shearIncrement;
	phys->shearIncrementForCD += shearIncrement.norm();
	Real du = 0.0;
	//Real debugFn = 0.0;
	//Real u_prev = fabs(phys->u_cumulative);
	if (phys->shearDir.norm() > pow(10, -15)) {
		phys->shearDir.normalize(); // FIXME: Maybe normalise the shearDir regardless of its magnitude?
	}
	Real degradeLength = phys->brittleLength; /*jointLength = 100u_peak */
	/* Elastic and plastic displacement can have negative signs but must be consistent throughout the simulation */
	if (phys->initialShearDir.norm() < pow(10, -11)) {
		phys->initialShearDir = phys->shearDir;
		du                    = shearIncrement.norm();
		if (fabs(phys->mobilizedShear) > 0.99999) {
			phys->u_cumulative += du;
			phys->cumulative_us += du;
		} else {
			phys->u_elastic += du;
		}
	} else {
		du = math::sign(phys->initialShearDir.dot(phys->shearDir)) * shearIncrement.norm(); //check cumulative shear displacement
		if (fabs(phys->mobilizedShear) > 0.99999) {
			if (du > 0.0) { //if negative it means it is unloading
				phys->u_cumulative += du;
				phys->cumulative_us += du;
			} else {
				phys->u_elastic += du;
			}
		} else {
			phys->u_elastic += du;
		}
	}


	/* Original */
	//	if(phys->twoDimension) { //moved this to Ig2_PP_PP_ScGeom.cpp @vsangelidakis
	//		phys->contactArea = phys->unitWidth2D*phys->jointLength;
	//	}
	if (s1->isBoundary == true || s2->isBoundary == true) {
		phys->tensionBroken  = true;
		phys->cohesionBroken = true;
	}
	if (!Talesnick) { //FIXME: Talesnick is not developed for the PPs. Either remove this check or develop it
		un = un - initialOverlapDistance;

		if (phys->jointType == 3) {
			phys->prevSigma = un * phys->kn_i / (1.0 - un / phys->maxClosure);
		} else {
			phys->prevSigma = phys->knVol * un;
		}
		//}
		phys->normalForce = phys->prevSigma * math::max(pow(10, -15), phys->contactArea) * geom->normal;
	}

	phys->kn = phys->knVol * math::max(pow(10, -15), phys->contactArea);

	if ((un < 0.0 && fabs(phys->prevSigma) > phys->tension && phys->tensionBroken == false /* first time tension is broken */)
	    || (un < 0.0 && phys->tensionBroken == true)) {
		if (neverErase) {
			phys->shearForce    = Vector3r::Zero();
			phys->normalForce   = Vector3r::Zero();
			phys->normalViscous = Vector3r::Zero();
			geom->normal        = Vector3r::Zero();
			phys->tensionBroken = true;
		} else {
			return false;
		}
		return true;
	}


	/*ORIGINAL */
	Vector3r c1x = geom->contactPoint - de1->pos;
	Vector3r c2x = geom->contactPoint - de2->pos;
	incidentV    = (de2->vel + de2->angVel.cross(c2x))
	        - (de1->vel
	           + de1->angVel.cross(
	                   c1x)); //FIXME: If we need to recalculate the relative velocity here, we should add shiftVel manually in this line, to handle periodicity @vsangelidakis
	incidentVn     = geom->normal.dot(incidentV) * geom->normal; // contact normal velocity
	incidentVs     = incidentV - incidentVn;                     // contact shear velocity
	shearIncrement = incidentVs
	        * dt; //FIXME: Do we need to recalculate incidentV, incidentVn, incidentVs and shearIncrement here? Need to revise whether to subtract shift2 from c2x @vsangelidakis

	if (!Talesnick) {
		Real Ks = 0.0;
		if (phys->jointType == 3) {
			Ks = phys->ks_i * pow(phys->prevSigma, 0.6);
		} else {
			Ks = phys->ksVol;
		}
		shearForce -= Ks * shearIncrement * math::max(pow(10, -15), phys->contactArea);
	}
	phys->ks = phys->ksVol * math::max(pow(10, -15), phys->contactArea);


	//	const shared_ptr<Body>& b1=Body::byId(id1,scene);
	//	const shared_ptr<Body>& b2=Body::byId(id2,scene);
	Real mbar    = (!b1->isDynamic() && b2->isDynamic())
	           ? de2->mass
	           : ((!b2->isDynamic() && b1->isDynamic())
	                      ? de1->mass
	                      : (de1->mass * de2->mass
                              / (de1->mass + de2->mass))); // get equivalent mass if both bodies are dynamic, if not set it equal to the one of the dynamic body
	Real Cn_crit = 2. * sqrt(mbar * phys->kn);            // Critical damping coefficient (normal direction)
	Real Cs_crit = 2. * sqrt(mbar * phys->ks);            // Critical damping coefficient (shear direction)
	// Note: to compare with the analytical solution you provide cn and cs directly (since here we used a different method to define c_crit)
	Real cn = Cn_crit * phys->viscousDamping; // Damping normal coefficient
	Real cs = Cs_crit * phys->viscousDamping; // Damping tangential coefficient

	// add normal viscous component if damping is included
	//Real maxFnViscous = phys->normalForce.norm();
	phys->normalViscous = cn * incidentVn;
	//if(phys->normalViscous.norm() > maxFnViscous){
	//	phys->normalViscous = phys->normalViscous * maxFnViscous/phys->normalViscous.norm();
	//}

	/* Check whether to allow fictitious (unnatural) attractive forces due to viscous damping, near the end of a collision */
	if (not allowViscousAttraction) {
// viscous force should not exceed the value of current normal force, i.e. no attraction force should be permitted if particles are non-adhesive
// *** enforce normal force to zero if no adhesion is permitted ***

// This commented block is the approach used in Hertz-Mindlin. I don't think this worked correctly, since using it gave me some -practically- infinite plastic slips in some individual timesteps, which make me believe that this approach some times can lead to negative (attractive) contact forces. Thus, I used the uncommented code-block below, following this one. @vsangelidakis
#if 0
		Vector3r normTemp = phys->normalForce - phys->normalViscous; // temporary normal force
		if (normTemp.dot(geom->normal) < 0.0){
			phys->normalViscous = phys->normalForce;
		}
#endif

		//		#if 0
		if (phys->normalViscous.norm() > phys->normalForce.norm()) {
			phys->normalViscous = phys->normalForce;
		}
		//		#endif

		//FIXME: The same must be done for the shearForce, if viscous damping is to be considered in the shear direction as well in the future
	}

	phys->normalForce -= phys->normalViscous;
	//Real baseElevation =  geom->contactPoint.z();

	/* Water pressure, heat effect */

	/* strength degradation */
	//	const Real PI = math::atan(1.0)*4;
	Real tan_effective_phi = 0.0;


	if (s1->isBoundary == true || s2->isBoundary == true || phys->jointType == 2) { // clay layer at boundary;
		phys->effective_phi = phys->phi_b;                                      // - 3.25*(1.0-exp(-fabs(phys->cumulative_us)/0.4));
		tan_effective_phi   = tan(phys->effective_phi / 180.0 * Mathr::PI);
	} else if (phys->intactRock == true) {
		phys->effective_phi = phys->phi_r + (phys->phi_b - phys->phi_r) * (exp(-fabs(phys->u_cumulative) / degradeLength));
		tan_effective_phi   = tan(phys->effective_phi / 180.0 * Mathr::PI);
	} else {
		phys->effective_phi = phys->phi_b;
		tan_effective_phi   = tan(phys->effective_phi / 180.0 * Mathr::PI);
	}


	/* shear loss */
	Vector3r dampedShearForce = shearForce;
	Real     maxFs            = 0.0;
	if (un > 0.0 /*compression*/) {
		Real fN = phys->normalForce.norm();
		if (phys->intactRock == true) {
			if (phys->cohesionBroken == true && allowBreakage == true) {
				maxFs = math::max(fN, 0.0) * tan_effective_phi;
			} else {
				Real cohesiveForce = phys->cohesion * math::max(pow(10, -15), phys->contactArea);
				maxFs              = cohesiveForce + math::max(fN, 0.0) * tan_effective_phi;
			}
		} else {
			maxFs = math::max(fN, 0.0) * tan_effective_phi;
		}
	}

	/* ********************************************************************************************************************* */
	/** SHEAR CORRECTION - MOHR-COULOMB CRITERION */
	phys->isSliding = false;

	if (!scene->trackEnergy && !traceEnergy) { //Update force but don't compute energy terms (see below))
		if (shearForce.norm() > maxFs) {
			phys->isSliding = true;
			Real ratio      = maxFs / shearForce.norm();
			shearForce *= ratio;
			if (allowBreakage == true) {
				phys->cohesionBroken = true;
			}
			dampedShearForce   = shearForce; /* no damping when it slides */
			phys->shearViscous = Vector3r(0, 0, 0);
		} else {
			phys->shearViscous = Vector3r::Zero(); //cs*incidentVs;  //For now we do not consider viscous damping in the shear direction
			dampedShearForce   = shearForce - phys->shearViscous;
		}
	} else {
		//almost the same with additional Vector3r instatinated for energy tracing,
		//duplicated block to make sure there is no cost for the instanciation of the vector when traceEnergy==false
		if (shearForce.norm() > maxFs) {
			phys->isSliding               = true;
			Real               ratio      = maxFs / shearForce.norm();
			/*const*/ Vector3r trialForce = shearForce; //Store prev force for definition of plastic slip
			shearForce *= ratio;
			if (allowBreakage == true) {
				phys->cohesionBroken = true;
			}
			dampedShearForce   = shearForce; /* no damping when it slides */
			phys->shearViscous = Vector3r(0, 0, 0);

			/* Plastic dissipation due to friction */
			/*const*/ Real dissip = ((1 / phys->ks) * (trialForce - shearForce)) /*plastic disp*/.dot(shearForce) /*active force*/;
			if (traceEnergy)
				plasticDissipation += dissip;
			else if (dissip > 0)
				scene->energy->add(dissip, "plastDissip", plastDissipIx, /*reset at every timestep*/ false);
		} else {
			phys->shearViscous = Vector3r::Zero(); //cs*incidentVs;  //For now we do not consider viscous damping in the shear direction
			dampedShearForce   = shearForce - phys->shearViscous;
		}

		// ------------------------------------------------------------------------------------------------------------------------------
		/* Elastic potential energy*/

		//FIXME: Check whether we need to add the viscous forces to the elastic ones on the elastic potential energy below, since the normalForce is reduced by normalViscous

		/* reduced+viscous */ //scene->energy->add(0.5*( (phys->normalForce + phys->normalViscous).squaredNorm()/phys->kn + (phys->shearForce + phys->shearViscous).squaredNorm()/phys->ks),"elastPotential",elastPotentialIx,/*reset at every timestep*/true);

		/* reduced*/ scene->energy->add(
		        0.5 * (phys->normalForce.squaredNorm() / phys->kn + phys->shearForce.squaredNorm() / phys->ks),
		        "elastPotential",
		        elastPotentialIx,
		        /*reset at every timestep*/ true);

		// ------------------------------------------------------------------------------------------------------------------------------
		/* Dissipation due to viscous damping*/
		if (phys->viscousDamping > 0.0) {
			/*const*/ Real normDampDissipValue = phys->normalViscous.dot(incidentVn * dt);
			if (traceEnergy)
				normDampDissip += normDampDissipValue; // calc dissipation of energy due to normal damping
			else if (normDampDissipValue > 0)
				scene->energy->add(normDampDissipValue, "normDampDissip", normDampDissipIx, /*reset at every timestep*/ false);
			// Here, instead of checking shearViscous.norm(), I should consider a boolean variable "noShearDamp", like in HertzMindlin.cpp
			if (phys->shearViscous.norm() > 0.0) {
				/*const*/ Real shearDampDissipValue = phys->shearViscous.dot(incidentVs * dt);
				if (traceEnergy) {
					shearDampDissip += shearDampDissipValue; // calc dissipation of energy due to shear damping damping
				} else {
					scene->energy->add(shearDampDissipValue, "shearDampDissip", shearDampDissipIx, /*reset at every timestep*/ false);
				}
			}
		}
	}
	if (shearForce.norm() < pow(10, -11)) {
		phys->mobilizedShear = 1.0;
	} else {
		phys->mobilizedShear = shearForce.norm() / maxFs;
	}

	Vector3r force = -phys->normalForce - dampedShearForce;
	if (math::isnan(force.norm())) { //FIXME: Check who necessarry this output is or else comment out this branch
		std::cout << "shearForce: " << shearForce << ", normalForce: " << phys->normalForce << ", viscousNormal: " << phys->normalViscous
		          << ", viscousShear: " << phys->shearViscous << ", geom normal: " << geom->normal << ", effective_phi: " << phys->effective_phi
		          << ", shearIncrement: " << shearIncrement << ", cs: " << cs << ", incidentVs: " << incidentVs << ", id1: " << id1 << ", id2: " << id2
		          << ", phys->mobilizedShear: " << phys->mobilizedShear << endl;
	}
	scene->forces.addForce(id1, force);
	scene->forces.addForce(id2, -force);
	//Vector3r normal = geom->normal;
	scene->forces.addTorque(id1, c1x.cross(force));
	scene->forces.addTorque(id2, -(c2x).cross(force));

	phys->prevNormal = geom->normal;
	return true;
}


/* ***************************************************************************************************************************** */
/** Ip2_FrictMat_FrictMat_KnKsPhys */
CREATE_LOGGER(Ip2_FrictMat_FrictMat_KnKsPhys);

void Ip2_FrictMat_FrictMat_KnKsPhys::go(const shared_ptr<Material>& b1, const shared_ptr<Material>& b2, const shared_ptr<Interaction>& interaction)
{
	//	const Real PI = 3.14159265358979323846;
	if (interaction->phys)
		return;

	ScGeom* scg = YADE_CAST<ScGeom*>(interaction->geom.get());
	assert(scg);

	const shared_ptr<FrictMat>& sdec1 = YADE_PTR_CAST<FrictMat>(b1);
	const shared_ptr<FrictMat>& sdec2 = YADE_PTR_CAST<FrictMat>(b2);

	shared_ptr<KnKsPhys> contactPhysics(new KnKsPhys());
	//interaction->interactionPhysics = shared_ptr<MomentPhys>(new MomentPhys());
	//const shared_ptr<MomentPhys>& contactPhysics = YADE_PTR_CAST<MomentPhys>(interaction->interactionPhysics);

	/* From interaction physics */
	Real fa = sdec1->frictionAngle;
	Real fb = sdec2->frictionAngle;

	//	/* calculate stiffness */
	//	Real Kn= Knormal;
	//	Real Ks= Kshear;

	/* Pass values calculated from above to CSPhys */
	contactPhysics->viscousDamping = viscousDamping;
	//	contactPhysics->useOverlapVol = useOverlapVol;
	contactPhysics->knVol = Knormal; //Kn
	contactPhysics->ksVol = Kshear;  //Ks
	contactPhysics->kn_i  = Knormal;
	contactPhysics->ks_i  = Kshear;
	//	contactPhysics->u_peak = u_peak;
	contactPhysics->maxClosure     = maxClosure;
	contactPhysics->cohesionBroken = cohesionBroken;
	contactPhysics->tensionBroken  = tensionBroken;
	//	contactPhysics->unitWidth2D = unitWidth2D;
	contactPhysics->frictionAngle = math::min(fa, fb);
	if (!useFaceProperties) {
		contactPhysics->phi_r = math::min(fa, fb) / Mathr::PI * 180.0;
		contactPhysics->phi_b = contactPhysics->phi_r;
	}
	//	contactPhysics->tanFrictionAngle	= math::tan(contactPhysics->frictionAngle);
	//contactPhysics->initialOrientation1	= Body::byId(interaction->getId1())->state->ori;
	//contactPhysics->initialOrientation2	= Body::byId(interaction->getId2())->state->ori;
	contactPhysics->prevNormal = scg->normal; //This is also done in the Contact Law.  It is not redundant because this class is only called ONCE!
	                                          //	contactPhysics->calJointLength = calJointLength;
	                                          //	contactPhysics->twoDimension = twoDimension;
	contactPhysics->useFaceProperties = useFaceProperties;
	contactPhysics->brittleLength     = brittleLength;
	interaction->phys                 = contactPhysics;
}

/* ***************************************************************************************************************************** */
/** KnKsPhys */
CREATE_LOGGER(KnKsPhys);
/* KnKsPhys */
KnKsPhys::~KnKsPhys() { }

} // namespace yade

#endif // YADE_POTENTIAL_PARTICLES
