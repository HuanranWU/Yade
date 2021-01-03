#ifdef YADE_POTENTIAL_BLOCKS


#include "KnKsPBLaw.hpp"
#include <lib/high-precision/Constants.hpp>
#include <core/Omega.hpp>
#include <core/Scene.hpp>
#include <pkg/dem/PotentialBlock.hpp>
#include <pkg/dem/ScGeom.hpp>

#include "KnKsPBLaw.hpp"
#include <core/Scene.hpp>
//#include <pkg/dem/ScGeom.hpp>
#include <core/Omega.hpp>
#include <pkg/dem/PotentialBlock.hpp>

namespace yade { // Cannot have #include directive inside.

YADE_PLUGIN((Law2_SCG_KnKsPBPhys_KnKsPBLaw)(Ip2_FrictMat_FrictMat_KnKsPBPhys)(KnKsPBPhys));

/* ***************************************************************************************************************************** */
/** Function which returns the ratio between the number of sliding contacts to the total number at a given time */
Real Law2_SCG_KnKsPBPhys_KnKsPBLaw::ratioSlidingContacts()
{
	Real ratio(0);
	int  count(0);
	FOREACH(const shared_ptr<Interaction>& I, *scene->interactions)
	{
		if (!I->isReal())
			continue;
		KnKsPBPhys* phys = dynamic_cast<KnKsPBPhys*>(I->phys.get()); /* contact physics */
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
Real Law2_SCG_KnKsPBPhys_KnKsPBLaw::getPlasticDissipation() { return (Real)plasticDissipation; }
void Law2_SCG_KnKsPBPhys_KnKsPBLaw::initPlasticDissipation(Real initVal)
{
	plasticDissipation.reset();
	plasticDissipation += initVal;
}
Real Law2_SCG_KnKsPBPhys_KnKsPBLaw::getnormDampDissip() { return (Real)normDampDissip; }
Real Law2_SCG_KnKsPBPhys_KnKsPBLaw::getshearDampDissip() { return (Real)shearDampDissip; }

Real Law2_SCG_KnKsPBPhys_KnKsPBLaw::elasticEnergy()
{
	Real energy = 0;
	FOREACH(const shared_ptr<Interaction>& I, *scene->interactions)
	{
		if (!I->isReal())
			continue;
		KnKsPBPhys* phys = dynamic_cast<KnKsPBPhys*>(I->phys.get()); /* contact physics */
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
/** Law2_SCG_KnKsPBPhys_KnKsPBLaw */
CREATE_LOGGER(Law2_SCG_KnKsPBPhys_KnKsPBLaw);

bool Law2_SCG_KnKsPBPhys_KnKsPBLaw::go(shared_ptr<IGeom>& ig /* contact geometry */, shared_ptr<IPhys>& ip /* contact physics */, Interaction* contact)
{
	TIMING_DELTAS_START();

	const Real&     dt     = scene->dt;                           /* size of time step */
	int             id1    = contact->getId1();                   /* id of Body1 */
	int             id2    = contact->getId2();                   /* id of Body2 */
	ScGeom*         geom   = static_cast<ScGeom*>(ig.get());      /* contact geometry */
	KnKsPBPhys*     phys   = static_cast<KnKsPBPhys*>(ip.get());  /* contact physics */
	State*          de1    = Body::byId(id1, scene)->state.get(); /* pointer to Body1 */
	State*          de2    = Body::byId(id2, scene)->state.get(); /* pointer to Body2 */
	Shape*          shape1 = Body::byId(id1, scene)->shape.get(); /* pointer to Shape1 */
	Shape*          shape2 = Body::byId(id2, scene)->shape.get(); /* pointer to Shape2 */
	PotentialBlock* s1     = static_cast<PotentialBlock*>(shape1);
	PotentialBlock* s2     = static_cast<PotentialBlock*>(shape2);

	Vector3r& shearForce = phys->shearForce;       /* shear force at previous timestep */
	Real      un         = geom->penetrationDepth; /* overlap distance */
	//TRVAR3(geom->penetrationDepth,de1->se3.position,de2->se3.position);

	TIMING_DELTAS_CHECKPOINT("Setup");

	/* ********************************************************************************************************************* */
	/** ERASE CONTACT OR RESET PARAMETERS IF NO OVERLAP */
	if (un < 0.0) {
		if (neverErase) {
			phys->normalForce   = Vector3r::Zero();
			phys->shearForce    = Vector3r::Zero();
			phys->kn            = 0;
			phys->ks            = 0;
			phys->normalViscous = Vector3r::Zero();
			phys->shearViscous  = Vector3r::Zero();

			geom->normal        = Vector3r::Zero();
			phys->tensionBroken = true;
			return true;
		} else {
			scene->interactions->requestErase(id1, id2);
			return false;
		}
	}


	/* ********************************************************************************************************************* */
	//	Vector3r shiftVel = Vector3r::Zero(); //scene->isPeriodic ? (Vector3r)((scene->cell->velGrad*scene->cell->Hsize)*Vector3r((Real) contact->cellDist[0],(Real) contact->cellDist[1],(Real) contact->cellDist[2])) : Vector3r::Zero();
	geom->rotateNonSpherical(shearForce); /*rotate shear force according to new contact plane (normal) */
	Vector3r oriShear = shearForce;
	//Vector3r oriNormalF = phys->normalForce;

	/** CALCULATE SHEAR INCREMENT */
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
	Vector3r incidentV      = geom->getIncidentVel(de1, de2, dt, shift2, shiftVel, /*preventGranularRatcheting*/ false); /* get relative velocity */
	Vector3r incidentVn     = geom->normal.dot(incidentV) * geom->normal;                                                /* get normal relative velocity */
	Vector3r incidentVs     = incidentV - incidentVn;                                                                    /* get shear relative velocity */
	Vector3r shearIncrement = incidentVs * dt; /* calculate shear increment from shear velocity */

	Real du        = shearIncrement.norm(); /* magnitude of shear increment */
	phys->shearDir = shearIncrement;        /* get shear direction */
	if (phys->shearDir.norm() > pow(10, -15)) {
		phys->shearDir.normalize(); // FIXME: Maybe normalise the shearDir regardless of its magnitude?
	}

	bool oneIsBoundary = (s1->isBoundary == true || s2->isBoundary == true); //Whether one of the particles is part of the boundary
	bool oneIsLining   = (s1->isLining == true || s2->isLining == true); //Whether one of the particles is part of a Tunnel lining (see RockLiningGlobal)

	//	if(phys->twoDimension) { phys->contactArea = phys->unitWidth2D*phys->jointLength;} /* contact area in 2D */ //moved this to Ig2_PP_PP_ScGeom.cpp @vsangelidakis

	if (oneIsBoundary) {
		phys->tensionBroken  = true;
		phys->cohesionBroken = true;
	} /* no cohesion and tension if the contact is a boundary*/
	  //	if( s1->isBoundary == true || s2->isBoundary==true ){ phys->tensionBroken = true; phys->cohesionBroken = true; }

	//TODO: In many instances, we consider a minimum value for the contactArea, equal to: pow(10,-15). This is meant to cover the cases where the contactArea is so small that the contact volume is degenerated to nearly a point. I think we should let the user decide the size of this minimum contactArea, since it depends on the scale of the problem and the unit system they consider @vsangelidakis

	TIMING_DELTAS_CHECKPOINT("After shearIncrement");

	/* ********************************************************************************************************************* */
	/** NORMAL CONTACT FORCE */
	Real normalStiffness = phys->knVol; /* use default stiffness values */
	if (Talesnick) {
		//phys->prevSigma = phys->knVol*math::max(un,(Real) 0.0);
		//phys->normalForce=phys->prevSigma*phys->normal*math::max(pow(10,-15),phys->contactArea);
		//#if 0
		//#if 0
		/* linear */
		//un = un - 8.0*pow(10.0,-5);
		Real A            = 0.5 * 4.0 * pow(10.0, 9); //*pow(10.0,9);
		Real B            = 0.5 * 7.4 * pow(10.0, 4);
		Real expTerm      = B * (math::max(un, 0.0)) + math::log(A);
		phys->prevSigma   = math::max(((math::exp(expTerm) - A)) / B, 0.0);
		Real Fn           = phys->prevSigma * math::max(pow(10, -15), phys->contactArea);
		phys->normalForce = Fn * geom->normal;
		phys->knVol       = (A + B * phys->prevSigma);
//#endif
#if 0
		/* power */phys->h
		Real Fn = pow(525000*math::max(un,(Real) 0),1.0/0.25)*math::max(pow(10,-11),phys->contactArea);
		phys->knVol = 2.1*pow(10.0,6)*pow(phys->prevSigma,0.75);
#endif

#if 0
		Real A = 7.0*pow(10.0,13);//*pow(10.0,9);
		Real B = 1.0*pow(10.0,6);
		phys->prevSigma = (A*un)*un - B*un;
		Real Fn = phys->prevSigma*math::max(pow(10,-15),phys->contactArea);
		phys->normalForce = Fn*geom->normal;
		phys->knVol = A*2.0*un - pow(10,6);
#endif

#if 0
		//power
		Real A = 2.1*pow(10.0,6); //4.2*pow(10.0,5);
		Real B = 0.75; //0.88;
		phys->prevSigma = pow( (1.0-B)*A*math::max(un, 0.0),1.0/(1.0-B) );
		Real Fn = phys->prevSigma*math::max(pow(10,-14),phys->contactArea);
		phys->knVol = A*pow(phys->prevSigma,B); //1.0/(1.0-B)*pow( (1.0-B)*A,1.0/(1.0-B) ) * pow(math::max(un,0.0), B/(1.0-B)); //A*pow(phys->prevSigma,B); //
		phys->normalForce = Fn*geom->normal;
#endif

		if (phys->prevSigma > pow(10.0, 15) /* || Fn < 0.0 */) {
			std::cout << "prevSigma: " << phys->prevSigma /* <<", Fn: "<<Fn*/ << endl;
			while (1) { }
		}
		phys->kn = phys->knVol * math::max(pow(10, -15), phys->contactArea);
	} else {
		if (!oneIsBoundary) {
			if (!oneIsLining) {
				un = un - initialOverlapDistance; /*GENERAL CASE: initialOverlapDistance is the offset distance for tension overlap, i.e. negative overlap*/
				if (phys->tensionBroken == true) {
					//if(allowBreakage == false && un > 0.0){phys->tensionBroken = false;}
					phys->prevSigma = normalStiffness * math::max(un, 0.0);
				} else {
					phys->prevSigma = normalStiffness * un;
				}
			} else {
				if (s1->isLining == true) {
					normalStiffness = s1->liningStiffness;
					un              = un - s1->liningTensionGap;
					phys->prevSigma = normalStiffness * un;
				} else if (s2->isLining == true) {
					normalStiffness = s2->liningStiffness;
					un              = un - s2->liningTensionGap;
					phys->prevSigma = normalStiffness * un;
				}
			}
		} else {
			normalStiffness = phys->kn_i; /* use special stiffness for boundaries */
			phys->prevSigma = normalStiffness * un;
		}
		phys->normalForce = phys->prevSigma * math::max(pow(10, -15), phys->contactArea) * geom->normal;
		phys->kn          = normalStiffness * math::max(pow(10, -15), phys->contactArea);

// I compacted the existing commented code block below into the statement above, in order to minimise the number of "if" statements. I keep the old code for now, just in case a bug occurs @vsangelidakis
#if 0
		if(s1->isBoundary == true || s2->isBoundary==true/*|| s1->isEastBoundary == true || s2->isEastBoundary==true*/){
			normalStiffness = phys->kn_i; /* use special stiffness for boundaries */
			phys->prevSigma = normalStiffness*un;
			phys->normalForce = phys->prevSigma*math::max(pow(10,-15),phys->contactArea)*geom->normal; //math::max(pow(10,-15),phys->contactArea)*
			phys->kn = normalStiffness*math::max(pow(10,-15),phys->contactArea);
		}else{
			if(s1->isLining==true){normalStiffness=s1->liningStiffness; un = un-s1->liningTensionGap; phys->prevSigma = normalStiffness*un;}
			else if(s2->isLining==true){normalStiffness=s2->liningStiffness; un = un-s2->liningTensionGap; phys->prevSigma = normalStiffness*un;}
			else{
				un = un-initialOverlapDistance;
				if(phys->tensionBroken == true){
					//if(allowBreakage == false && un > 0.0){phys->tensionBroken = false;}
					phys->prevSigma = normalStiffness*math::max(un,0.0);
				}else{
					phys->prevSigma = normalStiffness*un;
				}
			} /* GENERAL CASE - initialOverlapDistance is the offset distance for tension overlap, i.e. negative overlap */
			phys->normalForce = phys->prevSigma*math::max(pow(10,-15),phys->contactArea)*geom->normal;
			phys->kn = normalStiffness*math::max(pow(10,-15),phys->contactArea);
		}
#endif
	}


	/* ********************************************************************************************************************* */
	/** ERASE CONTACT IF TENSION IS BROKEN */
	if ((un < 0.0 && fabs(phys->prevSigma) > phys->tension && phys->tensionBroken == false /* first time tension is broken */)
	    || (un < 0.0 && phys->tensionBroken == true)) {
		if (neverErase) {
			phys->normalForce   = Vector3r::Zero();
			phys->shearForce    = Vector3r::Zero();
			phys->kn            = 0;
			phys->ks            = 0;
			phys->normalViscous = Vector3r::Zero();
			phys->shearViscous  = Vector3r::Zero();
			//geom->normal = Vector3r::Zero(); //FIXME: Do we need to uncomment this?
			phys->tensionBroken = true;
			return true;
		} else {
			//FIXME: Do we need to delete the interaction using: scene->interactions->requestErase(id1, id2) here, like above? @vsangelidakis
			return false;
		}
	}

	TIMING_DELTAS_CHECKPOINT("After normalForce");
	/* ********************************************************************************************************************* */
	/** SHEAR CONTACT FORCE */
	Real Ks = 0.0;
	if (Talesnick) {
//shearForce -= phys->ksVol*shearIncrement*math::max(pow(10,-15),phys->contactArea);
#if 0
		/* TALESNICK */
		/* linear law */
		Real shearStiffness = 1.0*pow(10.0,8) + 9.7*pow(10.0,4)*phys->prevSigma; /* current sigmaN from above */
		/* power law */
		//Real shearStiffness = 0.95*pow(10.0,6)*pow(phys->prevSigma,0.7); /* current sigmaN from above */
		//Real shearStiffness = 3.3*pow(10.0,5)*pow(phys->prevSigma,0.88); /* current sigmaN from above */
		phys->ksVol = shearStiffness;
		Ks = shearStiffness;
		shearForce -= shearStiffness*shearIncrement*math::max(pow(10,-15),phys->contactArea);
#endif

		//#if 0
		phys->ksVol = 1.9 * pow(10.0, 6) * pow(phys->prevSigma, 0.7);
		shearForce -= phys->ksVol * shearIncrement * math::max(pow(10, -15), phys->contactArea);
		//#endif
		phys->ks = phys->ksVol * math::max(pow(10, -15), phys->contactArea);
	} else {
		if (!oneIsBoundary) {
			if (!oneIsLining) {
				Ks = phys->ksVol; /* use default values */
			} else {
				if (s1->isLining == true) {
					Ks = s1->liningStiffness;
				} else if (s2->isLining == true) {
					Ks = s2->liningStiffness;
				}
			}
		} else {
			Ks = phys->ks_i; /* use special stiffness for boundaries */
		}
		shearForce -= Ks * shearIncrement * math::max(pow(10, -15), phys->contactArea);
		phys->ks = Ks * math::max(pow(10, -15), phys->contactArea);


// I compacted the existing commented code block below into the statement above, in order to minimise the number of "if" statements. I keep the old code for now, just in case a bug occurs @vsangelidakis
#if 0
		Ks = phys->ksVol; /* use default values */
		if(s1->isBoundary == true || s2->isBoundary==true/* || s1->isEastBoundary == true || s2->isEastBoundary==true*/){
			Ks = phys->ks_i;
			shearForce -= Ks*shearIncrement*math::max(pow(10,-15),phys->contactArea);
			phys->ks = Ks*math::max(pow(10,-15),phys->contactArea);
		}else{
			if(s1->isLining==true){Ks=s1->liningStiffness;}
			if(s2->isLining==true){Ks=s2->liningStiffness;}
			shearForce -= Ks*shearIncrement*math::max(pow(10,-15),phys->contactArea); /* GENERAL CASE */
			phys->ks = Ks*math::max(pow(10,-15),phys->contactArea);
		}
#endif
	}

	TIMING_DELTAS_CHECKPOINT("After shearForce");
	/* ********************************************************************************************************************* */
	/** CONTACT DAMPING */
	Real mass1 = de1->mass, mass2 = de2->mass;
	//	const shared_ptr<Body>& b1=Body::byId(id1,scene);
	//	const shared_ptr<Body>& b2=Body::byId(id2,scene);
	if (b1->isClumpMember() == true) {
		State* stateClump = Body::byId(b1->clumpId, scene)->state.get();
		mass1             = stateClump->mass;
	}
	if (b2->isClumpMember() == true) {
		State* stateClump = Body::byId(b2->clumpId, scene)->state.get();
		mass2             = stateClump->mass;
	}
	//	if (b1->isClumpMember() == false && b2->isClumpMember() == false){
	//		mass1 = de1->mass;
	//		mass2 = de2->mass;
	//	}
	Real mbar    = (!b1->isDynamic() && b2->isDynamic())
	           ? mass2
	           : ((!b2->isDynamic() && b1->isDynamic()) ? mass1 : (mass1 * mass2 / (mass1 + mass2))); // get equivalent mass
	Real Cn_crit = 2. * sqrt(mbar * phys->kn); // Critical damping coefficient (normal direction) 2.*sqrt(mbar*math::min(phys->kn,phys->ks))
	Real Cs_crit = Cn_crit;                    // Critical damping coefficient (shear direction)
	// Real Cs_crit = 2.*sqrt(mbar*phys->ks); //TODO: Calculation of Cs_crit to be revisited, if viscous damping is to be considered in the shear direction @vsangelidakis
	// Note: to compare with the analytical solution you provide cn and cs directly (since here we used a different method to define c_crit)
	Real cn = Cn_crit * phys->viscousDamping; // Damping normal coefficient
	Real cs = Cs_crit * phys->viscousDamping; // Damping tangential coefficient

	/* Add normal viscous component if damping is included */
	phys->normalViscous = cn * incidentVn;

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

	TIMING_DELTAS_CHECKPOINT("After viscous coeffs");

	/* ********************************************************************************************************************* */
	/** FRICTION LIMIT */
	//	const Real PI = math::atan(1.0)*4;
	Real tan_effective_phi  = 0.0;
	bool useIterativeMethod = false;
	if (Talesnick) {
		phys->cumulative_us = phys->cumulative_us + fabs(du);
		phys->effective_phi = phys->phi_b;
		tan_effective_phi   = tan(phys->effective_phi / 180.0 * Mathr::PI);
#if 0
		Real upeak = 2.0*pow(10.0,-6)*pow(phys->prevSigma,0.213);
		Real delta_miu = phys->ksVol/phys->prevSigma*(1.0 - math::min(fabs(phys->cumulative_us)/upeak, 1.0) ); if (isnan(delta_miu)){delta_miu = phys->ksVol/phys->prevSigma;}
		if (shearForce.norm() > phys->normalForce.norm()*phys->effective_phi){
			phys->effective_phi = phys->effective_phi + delta_miu*fabs(du);
		}
		tan_effective_phi = phys->effective_phi;
#endif
	} else {
		phys->effective_phi = phys->phi_b;
		/* if(oneIsBoundary || phys->jointType==2 ){ // clay layer at boundary;
			//if (allowBreakage == true) {useIterativeMethod = true;}else{
				useIterativeMethod = false;
				//phys->effective_phi = phys->phi_b; // - 3.25*(1.0-exp(-fabs(phys->cumulative_us)/0.4));
		}
		else*/
		if (oneIsLining) {
			if (s1->isLining == true) {
				phys->effective_phi = s1->liningFriction;
			} else if (s2->isLining == true) {
				phys->effective_phi = s2->liningFriction;
			}
		}
		tan_effective_phi = tan(phys->effective_phi / 180.0 * Mathr::PI);

// I compacted the existing commented code block below into the statement above, in order to minimise the number of "if" statements. I keep the old code for now, just in case a bug occurs. Also, commented the "isBoundary" branch for now, since nothing was changing inside it. @vsangelidakis
#if 0
		if(s1->isBoundary==true || s2->isBoundary == true || phys->jointType==2 ){ // clay layer at boundary;
			//if (allowBreakage == true) {useIterativeMethod = true;}else{
				useIterativeMethod = false;
				phys->effective_phi = phys->phi_b; // - 3.25*(1.0-exp(-fabs(phys->cumulative_us)/0.4));
				tan_effective_phi = tan(phys->effective_phi/180.0*Mathr::PI);
			//}
		}else if( s1->isLining==true ){ phys->effective_phi = s1->liningFriction; tan_effective_phi = tan(phys->effective_phi/180.0*Mathr::PI);
		}else if( s2->isLining==true ){ phys->effective_phi = s2->liningFriction; tan_effective_phi = tan(phys->effective_phi/180.0*Mathr::PI);
		}else{ // This last branch is the most common one to be invoked for 3D contacts, so we should see how to (carefully) move it as the first branch, to improve efficiency
			phys->effective_phi = phys->phi_b;
			//if(s1->isEastBoundary==true || s2->isEastBoundary==true){phys->effective_phi = 0.0;}
			tan_effective_phi = tan(phys->effective_phi/180.0*Mathr::PI);
		}
#endif
	}

	/* ********************************************************************************************************************* */
	/** SHEAR CORRECTION - MOHR-COULOMB CRITERION */
	Vector3r dampedShearForce = shearForce;
	Real     maxFs            = 0.0; //Real maxShear = 0.0;

	if (useIterativeMethod == false) {
		Real fN = phys->normalForce.dot(geom->normal); //This calculation takes into account the sign of fN
		//		Real fN = phys->normalForce.norm(); //This alternative calculation does not check whether fN is compressive or tensile (attractive)

		//		if (fN<=0) { std::cout<<"Negative fN: "<<fN<<endl; } //This is here for debugging purposes, to check when fN can be negative
		//		else       { std::cout<<"Positive fN: "<<fN<<endl; }

		if (math::isnan(fN)) {
			fN = 0.0;
		} //FIXME: Maybe output a warning if fN is negative (i.e. attractive) and/or include an assertion
		//fN should contribute to friction only when it is compressive. If allowViscousAttraction=True, fN can be tensile, and friction is not present

		if (phys->intactRock == true) {
			if (allowBreakage == false || phys->cohesionBroken == false) {
				Real cohesiveForce = phys->cohesion * math::max(pow(10, -15), phys->contactArea);
				maxFs              = cohesiveForce + fN * tan_effective_phi;
			} else {
				maxFs = math::max(fN, 0.0) * tan_effective_phi;
			}
		} else {
			maxFs = math::max(fN, 0.0) * tan_effective_phi;
		}

		phys->isSliding = false;
		if (!scene->trackEnergy && !traceEnergy) { //Update force but don't compute energy terms (see below))
			// PFC3d SlipModel, is using friction angle. CoulombCriterion
			if (shearForce.norm() > maxFs) {
				phys->isSliding = true;
				Real ratio      = maxFs / shearForce.norm(); //Define the plastic work input and increment the total plastic energy dissipated
				shearForce *= ratio;
				shearForce = shearForce; //FIXME: Check whether this line is necessary
				if (allowBreakage == true) {
					phys->cohesionBroken = true;
				}
				dampedShearForce = shearForce; /* no damping when it slides */
				phys->shearForce = shearForce; //FIXME: Stop storing the shearForce in two places: dampedShearForce and phys->shearForce
			} else {
				phys->shearViscous = Vector3r::Zero(); //cs*incidentVs; //For now we do not consider viscous damping in the shear direction
				dampedShearForce   = shearForce - phys->shearViscous;
				phys->shearForce   = shearForce
				        - phys->shearViscous; //FIXME: Stop storing the shearForce in two places: dampedShearForce and phys->shearForce
			}
		} else {
			//almost the same with additional Vector3r instatinated for energy tracing,
			//duplicated block to make sure there is no cost for the instanciation of the vector when traceEnergy==false
			if (shearForce.norm() > maxFs) {
				phys->isSliding = true;
				Real ratio      = maxFs / shearForce.norm(); //Define the plastic work input and increment the total plastic energy dissipated
				/*const*/ Vector3r trialForce = shearForce;  //Store prev force for definition of plastic slip
				shearForce *= ratio;
				shearForce = shearForce; //FIXME: Check whether this line is necessary
				if (allowBreakage == true) {
					phys->cohesionBroken = true;
				}
				dampedShearForce = shearForce; /* no damping when it slides */
				phys->shearForce = shearForce; //FIXME: Stop storing the shearForce in two places: dampedShearForce and phys->shearForce

				/* Plastic dissipation due to friction */
				/*const*/ Real dissip = ((1 / phys->ks) * (trialForce - shearForce)) /*plastic disp*/.dot(shearForce) /*active force*/;
				if (traceEnergy)
					plasticDissipation += dissip;
				else if (dissip > 0)
					scene->energy->add(dissip, "plastDissip", plastDissipIx, /*reset at every timestep*/ false);

			} else {
				phys->shearViscous = Vector3r::Zero(); //cs*incidentVs; //For now we do not consider viscous damping in the shear direction
				dampedShearForce   = shearForce - phys->shearViscous;
				phys->shearForce   = shearForce
				        - phys->shearViscous; //FIXME: Stop storing the shearForce in two places: dampedShearForce and phys->shearForce
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
						scene->energy->add(
						        shearDampDissipValue, "shearDampDissip", shearDampDissipIx, /*reset at every timestep*/ false);
					}
				}
			}
		}

	} else {                                     //FIXME: The traceEnergy feature is yet to be implemented for this branch, where useIterativeMethod=True
		Vector3r Fs_prev  = oriShear;        /* shear force before stress update */
		Vector3r delta_us = -shearIncrement; /* increment of shear displacement */
		//Real beta = 0.0; /* rate of plastic multiplier */
		Real beta_prev = phys->cumulative_us;      /* accumulated plastic mutliplier before stress update */
		Real fN        = phys->normalForce.norm(); //FIXME: We calculate this differently in the above branch. Check whether they lead to the same value
		if (math::isnan(fN)) {
			fN = 0.0;
		}
		Real     phi = phys->phi_b;
		Vector3r newFs(0, 0, 0);
		Real     plasticDisp = 0.0;
		if (!Talesnick) {
			plasticDisp = stressUpdateVec(ip /*contact physics */, Fs_prev, delta_us, beta_prev, phys->ks /*shear stiffness */, fN, phi, newFs);
		} else {
			Real upeak  = 2.0 * pow(10.0, -6) * pow(phys->prevSigma, 0.213);
			plasticDisp = stressUpdateVecTalesnick(
			        ip /*contact physics */, Fs_prev, delta_us, beta_prev, phys->ks /*shear stiffness */, fN, phi, newFs, upeak);
		}
		shearForce          = newFs;
		dampedShearForce    = newFs;
		phys->cumulative_us = phys->cumulative_us + plasticDisp; //beta*shearIncrement; /* add plastic displacements */
		Real miu_peak       = tan(phys->phi_b / 180.0 * Mathr::PI);
		Real delta_miu      = 0.059266;
		tan_effective_phi   = miu_peak - delta_miu * (1.0 - exp(-phys->cumulative_us / 0.35));
		phys->effective_phi = atan(tan_effective_phi) / Mathr::PI * 180.0;
		maxFs               = fN * tan_effective_phi;
		//if(shearForce.norm()/maxFs > 1.02) {std::cout<<"shearForce.norm()/maxFs: "<<shearForce.norm()/maxFs<<", shearForce-maxFs:"<<shearForce.norm()-maxFs<<", maxFs: "<<maxFs<<", shearForce: "<<shearForce<<endl;}
		if (plasticDisp < pow(10, -15)) {              /*elastic*/
			phys->shearViscous = Vector3r::Zero(); //cs*incidentVs; //
			dampedShearForce   = shearForce - phys->shearViscous;
		}
	}

	TIMING_DELTAS_CHECKPOINT("After Mohr-Coulomb");

	/* ********************************************************************************************************************* */
	/** APPLY FORCES */
	Vector3r c1x   = geom->contactPoint - de1->pos;
	Vector3r c2x   = geom->contactPoint - de2->pos;
	Vector3r force = -phys->normalForce - dampedShearForce;
	scene->forces.addForce(id1, force);
	scene->forces.addForce(id2, -force);
	//Vector3r normal = geom->normal;
	scene->forces.addTorque(id1, c1x.cross(force));
	scene->forces.addTorque(id2, -(c2x).cross(force));
	phys->prevNormal = geom->normal;


	/* ********************************************************************************************************************* */
	/** RECORDING VALUES AND DEBUGGING */
	if (shearForce.norm() < pow(10, -11)) {
		phys->mobilizedShear = 1.0;
	} else {
		phys->mobilizedShear = shearForce.norm() / maxFs;
	}
	if (s1->isLining == true) {
		s1->liningTotalPressure  = 1.0 / s1->liningLength * force;
		s1->liningNormalPressure = -1.0 / s1->liningLength * phys->normalForce;
	} else if (s2->isLining == true) {
		s2->liningTotalPressure  = -1.0 / s2->liningLength * force;
		s2->liningNormalPressure = 1.0 / s2->liningLength * phys->normalForce;
	}
	if (math::isnan(force.norm())) {
		std::cout << "shearForce: " << shearForce << ", normalForce: " << phys->normalForce << ", viscousNormal: " << phys->normalViscous
		          << ", viscousShear: " << phys->shearViscous << /*", normal: "<<phys->normal<< */ ", geom normal: " << geom->normal
		          << ", effective_phi: " << phys->effective_phi << ", shearIncrement: " << shearIncrement << ", cs: " << cs
		          << ", incidentVs: " << incidentVs << ", id1: " << id1 << ", id2: " << id2 << ", debugShear: " << oriShear
		          << /* " cyF: "<<cyF<<", cyR: "<<cyR<< */ ", phys->mobilizedShear: " << phys->mobilizedShear << endl;
	}
	return true;
}


/* ***************************************************************************************************************************** */
/** FUNCTION RETURNS PLASTIC MULTIPLIER RATE (beta) AND CURRENT SHEAR FORCE */
Real Law2_SCG_KnKsPBPhys_KnKsPBLaw::stressUpdateVec(
        shared_ptr<IPhys>& /*ip*/,
        const Vector3r Fs_prev /*prev shear force*/,
        const Vector3r du /*shear displacement increment*/,
        const Real     beta_prev /* prev plastic displacements*/,
        const Real     Ks /*shear stiffness */,
        const Real     fN /*normal force*/,
        const Real     phi_b /*peak friction angle*/,
        Vector3r&      newFs /*new shear force*/)
{
	newFs      = Vector3r::Zero();
	Real maxFs = 0.0;
	//	const Real PI = math::atan(1.0)*4;
	// Define beta_prev as the cumulated plastic multiplier
	// Define beta as the rate of plastic multiplier at the current time step
	// Fs_new = Fs_prev + dF
	// dF = Ks*du_elastic = Ks*(du - du_plastic) = Ks*(du - beta*du_p)
	//Real beta = 0.0; //beta is the plastic multiplier
	Real effective_phi     = phi_b;
	Real tan_effective_phi = tan(effective_phi / 180.0 * Mathr::PI);
	Real miu_peak          = tan_effective_phi;
	Real delta_miu         = 0.059266;
	Real function          = 0.0;
	Real lambda            = 0.0;

	newFs = Fs_prev + Ks * du;
	maxFs = fN * (miu_peak - delta_miu * (1.0 - exp(-1.0 * beta_prev / 0.35)));

	//If new stress after elastic update is outside the previous yield surface

	if (newFs.norm() - maxFs > pow(10, -11) && fN > pow(10, -11) && (Ks * du).norm() > pow(10, -11)) {
		// Fs_new = Fs_prev + dF
		// dF = Ks*du_elastic = Ks*(du - du_plastic) = Ks*(du - beta*du_p)
		// where du_p is a unit vector whose sign is Sign((Fs_prev + Ks*du).dot(du))*Sign(du);

		/* EQUATION TO SOLVE */
		/* Solve for beta */
		// Fs + Ks*(du - beta*du_p) = N*( miu_peak-delta_miu*(1-exp(beta_prev + beta) ) )
		//beta = 0.0;

		/* ************************************************************************************************************* */
		/** ESTABLISH LOWER AND UPPER BOUNDS (positive and negative) FOR BRACKETING beta (plastic multiplier) */
		/* Establish lower bound for lambda*/
		/* Lower bound = 0.0, i.e. fully elastic */
		/* f_lower_bound <0.0, because it is outside the yield surface */
		Real     lowerBound    = 0.0;
		Vector3r termA         = (Fs_prev + Ks * du) / (Ks * lowerBound + 1.0);
		Real     beta          = beta_prev + lowerBound * termA.norm();
		Real     f_lower_bound = termA.norm() - fN * (miu_peak - delta_miu * (1.0 - exp(-1.0 * beta / 0.35)));

		/* Establish upper bound for lambda*/
		Real upperBound = du.norm() / Fs_prev.norm();
		if (math::isnan(upperBound) == true) {
			upperBound = 1.0;
		}
		if (math::isinf(upperBound) == true || upperBound > pow(10.0, 12)) {
			upperBound = pow(10.0, 12);
		}
		termA              = (Fs_prev + Ks * du) / (Ks * upperBound + 1.0);
		beta               = beta_prev + upperBound * termA.norm();
		Real f_upper_bound = termA.norm() - fN * (miu_peak - delta_miu * (1.0 - exp(-1.0 * beta / 0.35)));
		int  iterUpper     = 0;
		while (math::sign(f_upper_bound) * math::sign(f_lower_bound) > 0.0) {
			upperBound    = 5.0 * upperBound;
			termA         = (Fs_prev + Ks * du) / (Ks * upperBound + 1.0);
			beta          = beta_prev + upperBound * termA.norm();
			f_upper_bound = termA.norm() - fN * (miu_peak - delta_miu * (1.0 - exp(-1.0 * beta / 0.35)));
			iterUpper++;
			if (iterUpper > 1000) {
				std::cout << "iterUpper: " << iterUpper << endl;
			}
		}

		Real oriUpperBound    = upperBound;
		Real orif_upper_bound = f_upper_bound;
		Real orif_lower_bound = f_lower_bound;
		Real midTrial         = 0.5 * (lowerBound + upperBound);

		int iter = 0;
		function = 1.0;
		/* Bisection to find beta*/
		Real Fmid;
		while (fabs(function) > pow(10, -14) && fabs(lowerBound - upperBound) > pow(10, -14)) {
			midTrial = 0.5 * (lowerBound + upperBound);
			lambda   = midTrial;
			termA    = (Fs_prev + Ks * du) / (Ks * lambda + 1.0);
			beta     = beta_prev + lambda * termA.norm();
			function = termA.norm() - fN * (miu_peak - delta_miu * (1.0 - exp(-1.0 * beta / 0.35)));
			Fmid     = function;
			if (math::sign(Fmid) == math::sign(f_lower_bound)) {
				lowerBound    = midTrial;
				f_lower_bound = function;
			} else {
				upperBound    = midTrial;
				f_upper_bound = function;
			}
			iter++;
			if (iter > 98) {
				if (fabs(function) > pow(10, -6) && fabs(lowerBound - upperBound) > pow(10, -6)) {
					std::cout << "iter: " << iter << ", Fs_prev:" << Fs_prev << ", beta: " << beta << ", function: " << function
					          << ", fN: " << fN << ", beta_prev: " << beta_prev << ", lowerBound: " << lowerBound
					          << ", upperBound: " << upperBound << ", lowerBound-upperBound: " << lowerBound - upperBound
					          << ", f_lower_bound: " << f_lower_bound << ", f_upper_bound: " << f_upper_bound
					          << ", oriUpperBound: " << oriUpperBound << ", orif_upper_bound: " << orif_upper_bound
					          << ", orif_lower_bound: " << orif_lower_bound << endl;
				}
				break;
			}
		}
		newFs = termA;
		maxFs = fN * (miu_peak - delta_miu * (1.0 - exp(-1.0 * beta / 0.35)));

		if (newFs.norm() / maxFs > 1.05) {
			std::cout << "newFs.norm()/maxFs: " << newFs.norm() / maxFs << ", newFs-maxFs: " << newFs.norm() - maxFs << ", newFs: " << newFs
			          << ", maxFs: " << maxFs << ", beta_prev: " << beta_prev << ", newFs.dotFsprev: " << newFs.dot(Fs_prev)
			          << "f_upper_bound: " << f_upper_bound << ", f_lower_bound: " << f_lower_bound << ", upperBound: " << upperBound
			          << ", lowerBound: " << lowerBound << ", du: " << du.norm() << ", Ks (GPa): " << Ks * pow(10, -9) << endl;
		}
		if (math::isnan(beta) == true) {
			std::cout << "beta: " << beta << ", oriUpperBound: " << oriUpperBound << ", lambda: " << lambda << ", lowerBound: " << lowerBound
			          << ", upperBound: " << upperBound << ", termA,: " << termA << ", beta_prev: " << beta_prev
			          << ", orif_upper_bound: " << orif_upper_bound << ", orif_lower_bound: " << orif_lower_bound << endl;
		}
		return beta - beta_prev;
	} else {
		// CASE1: FULLY ELASTIC
		newFs = Fs_prev + Ks * du;
		return 0.0;
	}
}


/* ***************************************************************************************************************************** */
/** FUNCTION RETURNS PLASTIC MULTIPLIER RATE (beta) AND CURRENT SHEAR FORCE */
Real Law2_SCG_KnKsPBPhys_KnKsPBLaw::stressUpdateVecTalesnick(
        shared_ptr<IPhys>& /*ip*/,
        const Vector3r Fs_prev /*prev shear force*/,
        const Vector3r du /*shear displacement increment*/,
        const Real     beta_prev /* prev plastic displacements*/,
        const Real     Ks /*shear stiffness */,
        const Real     fN /*normal force*/,
        const Real     phi_b /*peak friction angle*/,
        Vector3r&      newFs /*new shear force*/,
        const Real     upeak)
{
	newFs      = Vector3r::Zero();
	Real maxFs = 0.0;
	//	const Real PI = math::atan(1.0)*4;

	// Define beta_prev as the cumulated plastic multiplier
	// Define beta as the rate of plastic multiplier at the current time step
	// Fs_new = Fs_prev + dF
	// dF = Ks*du_elastic = Ks*(du - du_plastic) = Ks*(du - beta*du_p)
	//Real beta = 0.0; //beta is the plastic multiplier
	Real effective_phi     = phi_b;
	Real tan_effective_phi = tan(effective_phi / 180.0 * Mathr::PI);
	Real miu_peak          = tan_effective_phi;
	Real function          = 0.0;
	Real lambda            = 0.0;

	newFs = Fs_prev + Ks * du;
	maxFs = fN * (miu_peak * (1.0 - exp(-1.0 * beta_prev / upeak)));

	//If new stress after elastic update is outside the previous yield surface

	if (newFs.norm() - maxFs > pow(10, -11) && fN > pow(10, -11) && (Ks * du).norm() > pow(10, -11)) {
		// Fs_new = Fs_prev + dF
		// dF = Ks*du_elastic = Ks*(du - du_plastic) = Ks*(du - beta*du_p)
		// where du_p is a unit vector whose sign is Sign((Fs_prev + Ks*du).dot(du))*Sign(du);

		/* EQUATION TO SOLVE */
		/* Solve for beta */
		// Fs + Ks*(du - beta*du_p) = N*( miu_peak-delta_miu*(1-exp(beta_prev + beta) ) )
		//beta = 0.0;

		/* ************************************************************************************************************* */
		/** ESTABLISH LOWER AND UPPER BOUNDS (positive and negative) FOR BRACKETING beta (plastic multiplier) */
		/* Establish lower bound for lambda*/
		/* Lower bound = 0.0, i.e. fully elastic */
		/* f_lower_bound <0.0, because it is outside the yield surface */
		Real     lowerBound    = 0.0;
		Vector3r termA         = (Fs_prev + Ks * du) / (Ks * lowerBound + 1.0);
		Real     beta          = beta_prev + lowerBound * termA.norm();
		Real     f_lower_bound = termA.norm() - fN * (miu_peak * (1.0 - exp(-1.0 * beta_prev / upeak)));

		/* Establish upper bound for lambda*/
		Real upperBound = du.norm() / Fs_prev.norm();
		if (math::isnan(upperBound) == true) {
			upperBound = 1.0;
		}
		if (math::isinf(upperBound) == true || upperBound > pow(10.0, 12)) {
			upperBound = pow(10.0, 12);
		}
		termA              = (Fs_prev + Ks * du) / (Ks * upperBound + 1.0);
		beta               = beta_prev + upperBound * termA.norm();
		Real f_upper_bound = termA.norm() - fN * (miu_peak * (1.0 - exp(-1.0 * beta_prev / upeak)));
		int  iterUpper     = 0;
		while (math::sign(f_upper_bound) * math::sign(f_lower_bound) > 0.0) {
			upperBound    = 5.0 * upperBound;
			termA         = (Fs_prev + Ks * du) / (Ks * upperBound + 1.0);
			beta          = beta_prev + upperBound * termA.norm();
			f_upper_bound = termA.norm() - fN * (miu_peak * (1.0 - exp(-1.0 * beta_prev / upeak)));
			iterUpper++;
			if (iterUpper > 1000) {
				std::cout << "iterUpper: " << iterUpper << endl;
			}
		}

		Real oriUpperBound    = upperBound;
		Real orif_upper_bound = f_upper_bound;
		Real orif_lower_bound = f_lower_bound;
		Real midTrial         = 0.5 * (lowerBound + upperBound);

		int iter = 0;
		function = 1.0;
		/* Bisection to find beta*/
		Real Fmid;
		while (fabs(function) > pow(10, -14) && fabs(lowerBound - upperBound) > pow(10, -14)) {
			midTrial = 0.5 * (lowerBound + upperBound);
			lambda   = midTrial;
			termA    = (Fs_prev + Ks * du) / (Ks * lambda + 1.0);
			beta     = beta_prev + lambda * termA.norm();
			function = termA.norm() - fN * (miu_peak * (1.0 - exp(-1.0 * beta_prev / upeak)));
			Fmid     = function;
			if (math::sign(Fmid) == math::sign(f_lower_bound)) {
				lowerBound    = midTrial;
				f_lower_bound = function;
			} else {
				upperBound    = midTrial;
				f_upper_bound = function;
			}
			iter++;
			if (iter > 98) {
				if (fabs(function) > pow(10, -6) && fabs(lowerBound - upperBound) > pow(10, -6)) {
					std::cout << "iter: " << iter << ", Fs_prev:" << Fs_prev << ", beta: " << beta << ", function: " << function
					          << ", fN: " << fN << ", beta_prev: " << beta_prev << ", lowerBound: " << lowerBound
					          << ", upperBound: " << upperBound << ", lowerBound-upperBound: " << lowerBound - upperBound
					          << ", f_lower_bound: " << f_lower_bound << ", f_upper_bound: " << f_upper_bound
					          << ", oriUpperBound: " << oriUpperBound << ", orif_upper_bound: " << orif_upper_bound
					          << ", orif_lower_bound: " << orif_lower_bound << endl;
				}
				break;
			}
		}

		newFs = termA;
		maxFs = fN * (miu_peak * (1.0 - exp(-1.0 * beta_prev / upeak)));

		if (newFs.norm() / maxFs > 1.05) {
			std::cout << "newFs.norm()/maxFs: " << newFs.norm() / maxFs << ", newFs-maxFs: " << newFs.norm() - maxFs << ", newFs: " << newFs
			          << ", maxFs: " << maxFs << ", beta_prev: " << beta_prev << ", newFs.dotFsprev: " << newFs.dot(Fs_prev)
			          << "f_upper_bound: " << f_upper_bound << ", f_lower_bound: " << f_lower_bound << ", upperBound: " << upperBound
			          << ", lowerBound: " << lowerBound << endl;
		}
		if (math::isnan(beta) == true) {
			std::cout << "beta: " << beta << ", oriUpperBound: " << oriUpperBound << ", lambda: " << lambda << ", lowerBound: " << lowerBound
			          << ", upperBound: " << upperBound << ", termA,: " << termA << ", beta_prev: " << beta_prev
			          << ", orif_upper_bound: " << orif_upper_bound << ", orif_lower_bound: " << orif_lower_bound << endl;
		}
		return beta - beta_prev;
	} else {
		// CASE1: FULLY ELASTIC
		newFs = Fs_prev + Ks * du;
		return 0.0;
	}
}

/* ***************************************************************************************************************************** */
/** Ip2_FrictMat_FrictMat_KnKsPBPhys */
CREATE_LOGGER(Ip2_FrictMat_FrictMat_KnKsPBPhys);

void Ip2_FrictMat_FrictMat_KnKsPBPhys::go(const shared_ptr<Material>& b1, const shared_ptr<Material>& b2, const shared_ptr<Interaction>& interaction)
{
	//	const Real PI = 3.14159265358979323846;
	if (interaction->phys)
		return;

	ScGeom* scg = YADE_CAST<ScGeom*>(interaction->geom.get());
	assert(scg);

	const shared_ptr<FrictMat>& sdec1 = YADE_PTR_CAST<FrictMat>(b1);
	const shared_ptr<FrictMat>& sdec2 = YADE_PTR_CAST<FrictMat>(b2);

	shared_ptr<KnKsPBPhys> contactPhysics(new KnKsPBPhys());
	//interaction->interactionPhysics = shared_ptr<MomentPhys>(new MomentPhys());
	//const shared_ptr<MomentPhys>& contactPhysics = YADE_PTR_CAST<MomentPhys>(interaction->interactionPhysics);

	/* From interaction physics */
	Real fa = sdec1->frictionAngle;
	Real fb = sdec2->frictionAngle;

	//	/* calculate stiffness */
	//	Real Kn = Knormal;
	//	Real Ks = Kshear;

	/* Pass values calculated from above to CSPhys */
	contactPhysics->viscousDamping = viscousDamping;
	//	contactPhysics->useOverlapVol = useOverlapVol;
	contactPhysics->knVol = Knormal; //Kn
	contactPhysics->ksVol = Kshear;  //Ks
	contactPhysics->kn_i  = kn_i;
	contactPhysics->ks_i  = ks_i;
	//	contactPhysics->u_peak = u_peak;
	//	contactPhysics->maxClosure = maxClosure;
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
	contactPhysics->prevNormal = scg->normal; //This is also done in the Contact Law. It is not redundant because this class is only called ONCE!
	                                          //	contactPhysics->calJointLength = calJointLength;
	                                          //	contactPhysics->twoDimension = twoDimension;
	contactPhysics->useFaceProperties = useFaceProperties;
	//	contactPhysics->brittleLength = brittleLength;
	interaction->phys = contactPhysics;
}


/* ***************************************************************************************************************************** */
/** KnKsPBPhys */
CREATE_LOGGER(KnKsPBPhys);
/* KnKsPBPhys */
KnKsPBPhys::~KnKsPBPhys() { }

} // namespace yade

#endif // YADE_POTENTIAL_BLOCKS
