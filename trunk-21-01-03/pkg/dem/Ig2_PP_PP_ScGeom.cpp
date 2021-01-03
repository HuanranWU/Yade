/* CWBoon 2015 */
/* C.W. Boon, G.T. Houlsby, S. Utili (2013).  A new contact detection algorithm for three-dimensional non-spherical particles.  Powder Technology, 248, pp 94-102. */
/* A code for calling MOSEK (ver 6) exists in a previous version of this script. MOSEK can be used to solve the second order cone programme of equations (SOCP), alternativelly to the code used here.*/

#ifdef YADE_POTENTIAL_PARTICLES
#include "Ig2_PP_PP_ScGeom.hpp"
#include <lib/high-precision/Constants.hpp>
#include <pkg/dem/ScGeom.hpp>
//#include <pkg/dem/PotentialParticle.hpp>
//#include <yade/lib-base/yadeWm3Extra.hpp>
#include <pkg/dem/KnKsLaw.hpp>

#include <lib/base/Math.hpp>
#include <core/Omega.hpp>
#include <core/Scene.hpp>
#include <pkg/common/InteractionLoop.hpp>

#include <fstream>
#include <iomanip>
#include <iostream>

#include <Eigen/Core>
#include <Eigen/LU>
#include <Eigen/QR>

#include <cstdlib>
#include <ctime>

namespace yade { // Cannot have #include directive inside.

YADE_PLUGIN((Ig2_PP_PP_ScGeom)
            //#ifdef YADE_OPENGL
            //		(Gl1_Ig2_PP_PP_ScGeom)
            //	#endif
);


CREATE_LOGGER(Ig2_PP_PP_ScGeom);

/* ***************************************************************************************************************************** */
bool Ig2_PP_PP_ScGeom::go(
        const shared_ptr<Shape>&       cm1,
        const shared_ptr<Shape>&       cm2,
        const State&                   state1,
        const State&                   state2,
        const Vector3r&                shift2,
        const bool&                    force,
        const shared_ptr<Interaction>& c)
{
	PotentialParticle* s1 = static_cast<PotentialParticle*>(cm1.get());
	PotentialParticle* s2 = static_cast<PotentialParticle*>(cm2.get());

	/* Short circuit if both particles are boundary particles */
	if ((s1->isBoundary == true) && (s2->isBoundary == true)) {
		return false;
	}

	/* Short circuit if both particles are fixed */
	if ((state1.blockedDOFs == State::DOF_ALL) && (state2.blockedDOFs == State::DOF_ALL)) {
		return false;
	}

	bool                 hasGeom = false;
	Vector3r             contactPt(0, 0, 0);
	shared_ptr<ScGeom>   scm;
	shared_ptr<KnKsPhys> phys;

	Real stepBisection = 0.001 * math::min(s1->R, s2->R);
	if (stepBisection
	    < pow(10,
	          -6)) { /*std::cout<<"R1: "<<s1->R<<", R2: "<<s2->R<<", stepBisection: "<<stepBisection<<", id1: "<<c->getId1()<<", id2: "<<c->getId2()<<endl;*/
	}                //FIXME: Check whether we need this check. It is commented in the PBs

	bool contact = false;

	Real     fA = 0.0;
	Real     fB = 0.0;
	Vector3r normalP1(0.0, 0.0, 0.0);
	Vector3r normalP2(0.0, 0.0, 0.0);
	Vector3r avgNormal(0.0, 0.0, 0.0);
	Vector3r ptOnP1(0.0, 0.0, 0.0);
	Vector3r ptOnP2(0.0, 0.0, 0.0);
	Vector3r shearDir(0.0, 0.0, 0.0);
	bool     converge = false;

	if (c->geom) {
		hasGeom = true;
		scm     = YADE_PTR_CAST<ScGeom>(c->geom);
		if (scm->penetrationDepth > stepBisection) {
			stepBisection = 0.5 * scm->penetrationDepth;
		}
		if (stepBisection < pow(10, -6)) { /*std::cout<<"stepBisection: "<<stepBisection<<", penetrationDepth: "<<scm->penetrationDepth<<endl;*/
		}
		contactPt = scm->contactPoint;
	} else {
		scm       = shared_ptr<ScGeom>(new ScGeom());
		c->geom   = scm;
		contactPt = 0.5 * (state1.pos + state2.pos + shift2);
	}

	bool hasPhys = false;
	if (c->phys) {
		phys    = YADE_PTR_CAST<KnKsPhys>(c->phys);
		hasPhys = true; //shearDir = phys->shearDir; //normalization should take place in the Contact Law
	}

	converge = true;
	//	fA = evaluatePP(cm1,state1,contactPt);
	//	fB = evaluatePP(cm2,state2,contactPt);

	//Default does not have warmstart
	//if(fA < 0.0 && fB <0.0){
	//	converge = customSolve(cm1,state1,cm2,state2,contactPt,true);
	//}else{
	converge = customSolve(cm1, state1, cm2, state2, shift2, contactPt, false /* c->phys->warmstart */);
	//}

	fA = evaluatePP(cm1, state1, Vector3r::Zero(), contactPt);
	fB = evaluatePP(cm2, state2, shift2, contactPt);

	//	if (fA*fB<0.0) {/* std::cout<<"fA: "<<fA<<", fB: "<<fB<<endl; */ } //FIXME: Check whether we need to output a message here

	//std::cout<<"converge: "<<converge<<", fA: "<<fA<<", fB: "<<fB<<endl;

	//timingDeltas->start();

	if (fA < 0.0 && fB < 0.0 && converge == true) {
		contact = true;
	} else {
		contact   = false;
		contactPt = 0.5 * (state1.pos + state2.pos);
	}
	//////////////////////////////////////////////////////////* PASS VARIABLES TO ScGeom Functor */////////////////////////////////////////////////////////////
	//* 1. Get overlap *//
	//* 2. Get information on active planes. *//
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	if (contact == true || c->isReal() || force) {
		if (contact == true) {
			normalP1 = getNormal(cm1, state1, Vector3r(0, 0, 0), contactPt);
			normalP1.normalize();
			normalP2 = getNormal(cm2, state2, shift2, contactPt);
			normalP2.normalize();
			//if(s1->isBoundary==true){avgNormal=normalP1;}else if(s2->isBoundary==true){avgNormal=-normalP2;}else{avgNormal = (normalP1 - normalP2);}
			avgNormal = (normalP1 - normalP2);
			avgNormal.normalize();

			if (s1->fixedNormal == true) {
				avgNormal = s1->boundaryNormal;
			}
			if (s2->fixedNormal == true) {
				avgNormal = -s2->boundaryNormal;
			}

			Vector3r step = avgNormal * stepBisection;
			//int locationStuck = 2;
			getPtOnParticle2(cm1, state1, Vector3r::Zero(), contactPt, step, ptOnP1);
			getPtOnParticle2(cm2, state2, shift2, contactPt, -1.0 * step, ptOnP2);

			//			vector<Vector3r> points;
			Real penetrationDepth = (ptOnP2 - ptOnP1).norm(); // overlap;
			//std::cout<<"contactpoint: "<<contactPt<<", penetrationDepth: "<<penetrationDepth<<", avgNormal: "<<avgNormal<<endl;

			if (hasPhys) {
				//				phys->normal= avgNormal;
				phys->ptOnP1                                                      = ptOnP1;
				phys->ptOnP2                                                      = ptOnP2;
				/*bool calJointLength = phys->calJointLength; */ Real jointLength = phys->jointLength;
				int                                                   smallerID   = 1;
				shearDir                                                          = phys->shearDir;
				shearDir.normalize();

				if (calContactArea) { //calculate jointLength for 2-D contacts and contactArea for 2-D and 3-D contacts
					phys->contactArea = getAreaPolygon2(
					        cm1,
					        state1,
					        cm2,
					        state2,
					        shift2,
					        contactPt,
					        avgNormal,
					        smallerID,
					        shearDir,
					        jointLength,
					        twoDimension,
					        unitWidth2D,
					        areaStep);
					/* phys->smallerID = smallerID; */
				} else { //don't calculate jointLength or contactArea; assume constant linear stiffness in both normal and shear directions
					phys->jointLength = 1.0;
					if (twoDimension == true) {
						phys->contactArea = phys->jointLength * unitWidth2D;
					} else {
						phys->contactArea = 1.0;
					}
				}
				//				if( math::isnan(jointLength ) {jointLength = 1.0; phys->jointLength = jointLength; /*math::min(s1->R,s2->R);*/}
			}

			scm->precompute(
			        state1,
			        state2,
			        scene,
			        c,
			        avgNormal,
			        !(hasGeom),
			        shift2,
			        false /* avoidGranularRatcheting */); //Assign contact point and normal after precompute!!!!
			scm->contactPoint     = contactPt;
			scm->penetrationDepth = penetrationDepth;
			if (math::isnan(avgNormal.norm())) { /* std::cout<<"avgNormal: "<<avgNormal<<endl; */
			}
			scm->normal = avgNormal;

		} else {
			//scm->normal = Vector3r(0,0,0);
			scm->precompute(
			        state1,
			        state2,
			        scene,
			        c,
			        avgNormal,
			        !(hasGeom),
			        shift2,
			        false /* avoidGranularRatcheting */); //Assign contact point and normal after precompute!!!!
			scm->contactPoint     = contactPt;
			scm->penetrationDepth = -1.0; //scm->normal = Vector3r(0,0,0);
		}
		return true;
	} else {
		scm->contactPoint     = contactPt;
		scm->penetrationDepth = -1.0;
		scm->normal           = Vector3r(0, 0, 0);
		return false;
	}
	return false;
}


/* ***************************************************************************************************************************** */
bool Ig2_PP_PP_ScGeom::goReverse(
        const shared_ptr<Shape>&       cm1,
        const shared_ptr<Shape>&       cm2,
        const State&                   state1,
        const State&                   state2,
        const Vector3r&                shift2,
        const bool&                    force,
        const shared_ptr<Interaction>& c)
{
	c->swapOrder();
	return go(cm2, cm1, state2, state1, -shift2, force, c); // This works with c->swapOrder()
	//			return go(cm1,cm2,state2,state1,-shift2,force,c); // Alternativelly, this works without c->swapOrder()
}


// In getAreaPolygon2 we calculate the contact area. This function follows a slightly different approach from the namesake function in the Potential Blocks. Here, to calculate the contact area we swipe along the shear direction in fixed angular steps, finding points of the overlapping volume through bisection searches.
/* ***************************************************************************************************************************** */
Real Ig2_PP_PP_ScGeom::getAreaPolygon2(
        const shared_ptr<Shape>& cm1,
        const State&             state1,
        const shared_ptr<Shape>& cm2,
        const State&             state2,
        const Vector3r&          shift2,
        const Vector3r           contactPoint,
        const Vector3r           contactNormal,
        int&                     smaller,
        Vector3r                 shearDir,
        Real&                    jointLength,
        const bool               twoD,
        Real                     unitWidth2D,
        int                      areaStep)
{
	Real areaTri = 0.0;
	//	Real bisectionStepSize = 1.0;//*math::min(s1->R, s2->R);

	if (!twoD) { //3D contact - search counter-clockwise
		int      count = 0, countParticleA = 0, countParticleB = 0;
		Vector3r ptOnBoundary  = contactPoint;
		Vector3r orthogonalDir = Vector3r(contactNormal.y(), -contactNormal.x(), 0.0); //Vector along the shear direction

		if (orthogonalDir.norm() < pow(10, -5)) {
			orthogonalDir = Vector3r(contactNormal.z(), 0.0, -contactNormal.x());
		} //TODO: Optimise these two ifs into a nested one
		if (orthogonalDir.norm() < pow(10, -5)) {
			orthogonalDir = Vector3r(0.0, contactNormal.z(), -contactNormal.y());
		}
		orthogonalDir.normalize();
		Real tol = pow(10, -8);
		//		Vector3r orthogonalDir2 = contactNormal.cross(orthogonalDir); orthogonalDir2.normalize(); //Vector along the shear direction, perpendicular to orthogonalDir

		Vector3r prevPoint = ptOnBoundary;
		Matrix3r area1;
		Matrix3r area2;
		Matrix3r area3;
		//http://en.wikipedia.org/wiki/Triangle#Using_coordinates
		area1(0, 0) = contactPoint.x();
		area2(0, 0) = contactPoint.y();
		area3(0, 0) = contactPoint.z();
		area1(1, 0) = contactPoint.y();
		area2(1, 0) = contactPoint.z();
		area3(1, 0) = contactPoint.x();
		area1(2, 0) = 1.0;
		area2(2, 0) = 1.0;
		area3(2, 0) = 1.0;

		Vector3r newPt(0, 0, 0), secondPoint(0, 0, 0), newSearchDir, ptOnP1a, ptOnP2a, p1, p2, v;
		Real     theta;

		for (int i = 0; i <= 360; i += areaStep) {   //Here we iterate every "areaStep" degrees, to find the contact area
			theta        = i * Mathr::PI / 180.; //theta is "i" in radians
			v            = orthogonalDir;
			newSearchDir = v * math::cos(theta) + contactNormal.cross(v) * math::sin(theta)
			        + contactNormal * (contactNormal.dot(v)) * (1 - math::cos(theta));
			newSearchDir.normalize(); //FIXME lose accuracy
			                          //			std::cout<< count << " | " << theta << " | " << newSearchDir	<<endl; //Debug message

			prevPoint = ptOnBoundary;
			ptOnP1a   = Vector3r(0, 0, 0);
			ptOnP2a   = Vector3r(0, 0, 0);

			getPtOnParticle2(cm1, state1, Vector3r(0, 0, 0), contactPoint, newSearchDir, ptOnP1a);
			getPtOnParticle2(cm2, state2, shift2, contactPoint, newSearchDir, ptOnP2a);

			p1 = (ptOnP1a - contactPoint);
			p2 = (ptOnP2a - contactPoint);
			if (p1.norm() < p2.norm()) {
				ptOnBoundary = ptOnP1a;
				countParticleA++;
			} else {
				ptOnBoundary = ptOnP2a;
				countParticleB++;
			}
			if (count >= 1) {
				area1(0, 1) = prevPoint.x();
				area2(0, 1) = prevPoint.y();
				area3(0, 1) = prevPoint.z();
				area1(1, 1) = prevPoint.y();
				area2(1, 1) = prevPoint.z();
				area3(1, 1) = prevPoint.x();
				area1(2, 1) = 1.0;
				area2(2, 1) = 1.0;
				area3(2, 1) = 1.0;

				area1(0, 2) = ptOnBoundary.x();
				area2(0, 2) = ptOnBoundary.y();
				area3(0, 2) = ptOnBoundary.z();
				area1(1, 2) = ptOnBoundary.y();
				area2(1, 2) = ptOnBoundary.z();
				area3(1, 2) = ptOnBoundary.x();
				area1(2, 2) = 1.0;
				area2(2, 2) = 1.0;
				area3(2, 2) = 1.0;

				areaTri += 0.5 * sqrt((pow(area1.determinant(), 2) + pow(area2.determinant(), 2) + pow(area3.determinant(), 2)));
				//				std::cout<<count<<" | "<<areaTri<<" | "<<contactPoint<<" | "<<prevPoint<< " | "<<ptOnBoundary<<endl;
			}
			if (count == 0) {
				secondPoint = ptOnBoundary;
			}
			newPt = ptOnBoundary;
			count++;
		}
		if (count < 4 || (newPt - secondPoint).norm() > tol) {
			std::cout << "The contactArea calculation did not end in a closed loop. The last/first points: " << newPt << " | " << secondPoint
			          << endl;
		}
		if (countParticleA > countParticleB) {
			smaller = 1;
		} else {
			smaller = 2;
		} //TODO: countParticleA,B and smaller variables could be removed in the future; they are not actively used so far.
	} else {  //2D contact (twoD=true)
		Vector3r searchDir = shearDir;
		Vector3r ptOnBoundary1(0, 0, 0), ptOnBoundary2(0, 0, 0);

		searchDir = contactNormal.cross(twoDdir);
		if (searchDir.squaredNorm() < pow(10, -14)) {
			Vector3r xDir = Vector3r(
			        1,
			        0,
			        0); //FIXME: Instead of the hardcoded Vector3r(1,0,0), here we should define a perpendicular direction to the twoDir attr in the 2D plane, where the particles are defined.
			searchDir = xDir.cross(twoDdir);
		}
		searchDir.normalize();
		//		searchDir *= bisectionStepSize;
		Vector3r ptOnP1(0, 0, 0);
		Vector3r ptOnP2(0, 0, 0);
		getPtOnParticle2(cm1, state1, Vector3r(0, 0, 0), contactPoint, searchDir, ptOnP1);
		getPtOnParticle2(cm2, state2, shift2, contactPoint, searchDir, ptOnP2);

		if ((ptOnP1 - contactPoint).squaredNorm() < (ptOnP2 - contactPoint).squaredNorm()) { //FIXME: just .norm() should work fine too
			ptOnBoundary1 = ptOnP1;
		} else {
			ptOnBoundary1 = ptOnP2;
		}
		getPtOnParticle2(cm1, state1, Vector3r(0, 0, 0), contactPoint, -searchDir, ptOnP1);
		getPtOnParticle2(cm2, state2, shift2, contactPoint, -searchDir, ptOnP2);

		if ((ptOnP1 - contactPoint).squaredNorm() < (ptOnP2 - contactPoint).squaredNorm()) {
			ptOnBoundary2 = ptOnP1;
		} else {
			ptOnBoundary2 = ptOnP2;
		}

		jointLength = (ptOnBoundary1 - ptOnBoundary2).norm(); //Contact length of 2-D contact
		areaTri     = unitWidth2D * jointLength;              //Contact area of 2-D contact
	}
	return areaTri;
}


/* ***************************************************************************************************************************** */
void Ig2_PP_PP_ScGeom::BrentZeroSurf(
        const shared_ptr<Shape>& cm1, const State& state1, const Vector3r& shift2, const Vector3r bracketA, const Vector3r bracketB, Vector3r& zero)
{
	Real     a = 0.0;
	Real     b = 1.0;
	Real     t = pow(10, -16);
	Real     m = 0.0;
	Real     p = 0.0;
	Real     q = 0.0;
	Real     r = 0.0;
	Real     s = 0.0;
	Real     c = 0.0;
	Real     d = 0.0;
	Real     e = 0.0;
	Real     h;
	Vector3r bracketRange = bracketB - bracketA;
	Real     fa           = evaluatePP(cm1, state1, shift2, bracketA); //evaluateFNoSphere(cm1,state1, bracketA); //
	Real     fb           = evaluatePP(cm1, state1, shift2, bracketB); //evaluateFNoSphere(cm1,state1, bracketB); //

	if (fa * fb > 0.00001) { //FIXME: Check whether I need to output a warning here, or else comment this statement
		                 //std::cout<<"fa: "<<fa<<", fb: "<<fb<<endl;
	}
	//if(fabs(fa)<fabs(fb)){bracketRange *= -1.0; Vector3r temp = bracketA; bracketA= bracketB; bracketB = temp;}
	Real fc = 0.0;

	Real tol     = 2.0 * DBL_EPSILON * fabs(b) + t;
	int  counter = 0;
	do {
		if (counter == 0) {
			c  = a;
			fc = fa;
			d  = b - a;
			e  = d;
			if (fabs(fc) < fabs(fb)) {
				a  = b;
				b  = c;
				c  = a;
				fa = fb;
				fb = fc;
				fc = fa;
			}
		} else {
			if (fb * fc > 0.0) {
				c  = a;
				fc = fa;
				d  = b - a;
				e  = d;
			}
			if (fabs(fc) < fabs(fb)) {
				a  = b;
				b  = c;
				c  = a;
				fa = fb;
				fb = fc;
				fc = fa;
			}
		}
		tol = 2.0 * DBL_EPSILON * fabs(b) + t;
		m   = 0.5 * (c - b);
		if (fabs(m) > tol && fabs(fb) > pow(10, -13)) {
			if (fabs(e) < tol || fabs(fa) <= fabs(fb)) {
				d = m;
				e = d;
			} else {
				s = fb / fa;
				if (fabs(a - c) < pow(10, -15)) {
					p = 2.0 * m * s;
					q = 1.0 - s;
				} else {
					q = fa / fc;
					r = fb / fc;
					p = s * (2.0 * m * q * (q - r) - (b - a) * (r - 1.0));
					q = (q - 1.0) * (r - 1.0) * (s - 1.0);
				}

				if (p > 0.0) {
					q = -q;
				} else {
					p = -p;
				}

				s = e;
				e = d;

				if ((2.0 * p < (3.0 * m * q - fabs(tol * q))) && (p < fabs(0.5 * s * q))) {
					d = p / q;
				} else {
					d = m;
					e = d;
				}
			}

			a  = b;
			fa = fb;
			h  = 0.0;
			if (fabs(d) > tol) {
				h = d;
			} else if (m > 0.0) {
				h = tol;
			} else {
				h = -tol;
			}
			b = b + h; // h*m_unitVec;

			zero = bracketA + b * bracketRange;
			fb   = evaluatePP(cm1, state1, shift2, zero); //evaluateFNoSphere(cm1,state1, zero); //


		} else {
			zero = bracketA + b * bracketRange;
			//std::cout<<"b: "<<b<<", m: "<<m<<", tol: "<<tol<<", c: "<<c<<", a: "<<a<<endl;
			break;
		}

		counter++;
		if (counter == 10000) { //FIXME: Check whether I need to output a warning here, or else comment this statement
			                //std::cout<<"counter: "<<counter<<", m.norm: "<<m<<", fb: "<<fb<<endl;
		}
	} while (1);
	//}while( m.norm() > tol && fabs(fb) > pow(10,-13) );
}


/* ***************************************************************************************************************************** */
/* Routine to get value of function (constraint) at a particular position */
Real Ig2_PP_PP_ScGeom::evaluatePP(const shared_ptr<Shape>& cm1, const State& state1, const Vector3r& shift2, const Vector3r newTrial)
{
	PotentialParticle* s1 = static_cast<PotentialParticle*>(cm1.get());
	///////////////////Transforming trial values to local frame of particles//////////////////
	Vector3r tempP1 = newTrial - state1.pos - shift2;
	/* Direction cosines */
	//state1.ori.normalize();
	Vector3r localP1 = state1.ori.conjugate() * tempP1;
	Real     x       = localP1.x();
	Real     y       = localP1.y();
	Real     z       = localP1.z();
	int      planeNo = s1->a.size();
	Real     pSum2   = 0.0, plane;
	for (int i = 0; i < planeNo; i++) {
		plane = s1->a[i] * x + s1->b[i] * y + s1->c[i] * z - s1->d[i];
		if (plane < pow(10, -15)) {
			plane = 0.0;
		}
		pSum2 += pow(plane, 2);
	}
	Real r = s1->r;
	Real R = s1->R;
	Real k = s1->k;
	/* Additional sphere */
	Real sphere = (pow(x, 2) + pow(y, 2) + pow(z, 2)) / pow(R, 2);
	/* Complete potential particle */
	Real f = (1.0 - k) * (pSum2 / pow(r, 2) - 1.0) + k * (sphere - 1.0);
	return f;
}


/* ***************************************************************************************************************************** */
Vector3r Ig2_PP_PP_ScGeom::getNormal(const shared_ptr<Shape>& cm1, const State& state1, const Vector3r& shift2, const Vector3r newTrial)
{
	PotentialParticle* s1 = static_cast<PotentialParticle*>(cm1.get());
	///////////////////Transforming trial values to local frame of particles//////////////////
	Vector3r tempP1 = newTrial - state1.pos - shift2;

	/* Direction cosines */
	Vector3r localP1(0, 0, 0);
	localP1 = state1.ori.conjugate() * tempP1; //the body has been rotated by state.ori.  So I will need to rotate the frame by state.ori too
	Real x  = localP1[0];
	Real y  = localP1[1];
	Real z  = localP1[2];

	////////////////////////////// assembling potential planes particle 1//////////////////////////////
	int          planeNo = s1->a.size();
	vector<Real> p;
	Real         pSum2 = 0.0, plane;
	for (int i = 0; i < planeNo; i++) {
		plane = s1->a[i] * x + s1->b[i] * y + s1->c[i] * z - s1->d[i];
		if (plane < pow(10, -15)) {
			plane = 0.0;
		}
		p.push_back(plane);
		pSum2 += pow(p[i], 2);
	}
	Real r = s1->r;
	Real R = s1->R;
	Real k = s1->k;
	/* Additional sphere */
	//Real sphere  = (pow(x,2) + pow(y,2) + pow(z,2))/pow(R,2);
	/* Complete potential particle */
	//Real f = (1.0-k)*(pSum2/pow(r,2)-1.0)+k*(sphere -1.0);

	//////////////////////////////// local first derivitive particle 1 /////////////////////////////////////
	Real pdxSum = 0.0;
	Real pdySum = 0.0;
	Real pdzSum = 0.0;
	for (int i = 0; i < planeNo;
	     i++) { //FIXME: Instead of saving the value of "plane" inside the vector "p" and iterate through "p[i]" in a different loop below, we can calculate pdxSum, pdySum, pdzSum directly here, and avoid storing all this information in the vector "p". In this way, we don't need "p"
		pdxSum += (s1->a[i] * p[i]);
		pdySum += (s1->b[i] * p[i]);
		pdzSum += (s1->c[i] * p[i]);
	}

	Real fdx = 2.0 * (1.0 - k) * pdxSum / pow(r, 2) + 2.0 * k * x / pow(R, 2);
	Real fdy = 2.0 * (1.0 - k) * pdySum / pow(r, 2) + 2.0 * k * y / pow(R, 2);
	Real fdz = 2.0 * (1.0 - k) * pdzSum / pow(r, 2) + 2.0 * k * z / pow(R, 2);

	///////////////////// Assembling rotation matrix Qt for particle 1 //////////////////////////////
	Matrix3r Q1 = Matrix3r::Zero();
	;
	//if(state1.ori==Quaternionr(1,0,0,0)){
	//	Q1.setIdentity();
	//}else{
	Real q0  = state1.ori.w();
	Real q1  = state1.ori.x();
	Real q2  = state1.ori.y();
	Real q3  = state1.ori.z();
	Q1(0, 0) = 2.0 * pow(q0, 2.0) - 1.0 + 2.0 * pow(q1, 2.0);
	Q1(0, 1) = 2.0 * q1 * q2 + 2.0 * q0 * q3;
	Q1(0, 2) = 2.0 * q1 * q3 - 2.0 * q0 * q2;
	Q1(1, 0) = 2.0 * q1 * q2 - 2.0 * q0 * q3;
	Q1(1, 1) = 2.0 * pow(q0, 2.0) - 1.0 + 2.0 * pow(q2, 2.0);
	Q1(1, 2) = 2.0 * q2 * q3 + 2.0 * q0 * q1;
	Q1(2, 0) = 2.0 * q1 * q3 + 2.0 * q0 * q2;
	Q1(2, 1) = 2.0 * q2 * q3 - 2.0 * q0 * q1;
	Q1(2, 2) = 2.0 * pow(q0, 2) - 1.0 + 2.0 * pow(q3, 2.0);

	//Matrix3r Q1n = state1.ori.toRotationMatrix();
	//Matrix3r Q1c = state1.ori.conjugate().toRotationMatrix();
	//std::cout<<"Q1: "<<endl<<Q1<<endl<<"Q1n: "<<endl<<Q1n<<endl<<"Q1c: "<<endl<<Q1c<<endl<<endl;
	//}

	///////////////////// global first and second derivitive for particle 1 /////////////////////////
	Real Fdx = fdx * Q1(0, 0) + fdy * Q1(1, 0) + fdz * Q1(2, 0);
	Real Fdy = fdx * Q1(0, 1) + fdy * Q1(1, 1) + fdz * Q1(2, 1);
	Real Fdz = fdx * Q1(0, 2) + fdy * Q1(1, 2) + fdz * Q1(2, 2);

	//	if (math::isnan(Fdx) == true || math::isnan(Fdy) == true || math::isnan(Fdz)==true) { //FIXME: Check whether I need a warning here, if the derivatives cannot be calculated
	//		//std::cout<<"Q1(0,0): "<<Q1(0,0)<<","<<Q1(0,1)<<","<<Q1(0,2)<<","<<Q1(1,0)<<","<<Q1(1,1)<<","<<Q1(1,2)<<","<<Q1(2,0)<<","<<Q1(2,1)<<","<<Q1(2,2)<<", q:"<<q0<<","<<q1<<","<<q2<<","<<q3<<", fd: "<<fdx<<","<<fdy<<","<<fdz<<endl;
	//	}

	return Vector3r(Fdx, Fdy, Fdz);
}


/* ***************************************************************************************************************************** */
void Ig2_PP_PP_ScGeom::getPtOnParticle2(
        const shared_ptr<Shape>& cm1, const State& state1, const Vector3r& shift2, Vector3r midPoint, Vector3r searchDir, Vector3r& ptOnParticle)
{
	//PotentialParticle *s1=static_cast<PotentialParticle*>(cm1.get());
	ptOnParticle   = midPoint;
	Real f         = evaluatePP(cm1, state1, shift2, ptOnParticle); //evaluateFNoSphere(cm1,state1, ptOnParticle); //
	Real fprevious = f;
	int  counter   = 0;
	//normal.normalize();
	Vector3r step = searchDir * math::sign(f) * -1.0;
	Vector3r bracketA(0, 0, 0), bracketB(0, 0, 0);

	do {
		ptOnParticle += step;
		fprevious = f;
		f         = evaluatePP(cm1, state1, shift2, ptOnParticle); //evaluateFNoSphere(cm1,state1, ptOnParticle); //
		counter++;
		if (counter == 50000) {
			//LOG_WARN("Initial point searching exceeded 500 iterations!");
			std::cout << "ptonparticle2 search exceeded 50000 iterations! step:" << step << endl;
		}
	} while (math::sign(fprevious) * math::sign(f) * 1.0 > 0.0);
	bracketA = ptOnParticle;
	bracketB = ptOnParticle - step;
	Vector3r zero(0, 0, 0);
	BrentZeroSurf(cm1, state1, shift2, bracketA, bracketB, zero);
	ptOnParticle = zero;
	//if( fabs(f)>0.1){std::cout<<"getInitial point f:"<<f<<endl;}
}


/* ***************************************************************************************************************************** */
bool Ig2_PP_PP_ScGeom::customSolve(
        const shared_ptr<Shape>& cm1,
        const State&             state1,
        const shared_ptr<Shape>& cm2,
        const State&             state2,
        const Vector3r&          shift2,
        Vector3r&                contactPt,
        bool                     warmstart)
{
	//timingDeltas->start();
	PotentialParticle* s1 = static_cast<PotentialParticle*>(cm1.get());
	PotentialParticle* s2 = static_cast<PotentialParticle*>(cm2.get());
	Vector3r           xGlobal(0.0, 0.0, 0.0);
	int                iter      = 0;
	int                totalIter = 0;

	/* Parameters for particles A and B */
	Real rA        = s1->r;
	Real kA        = s1->k;
	Real RA        = s1->R;
	Real rB        = s2->r;
	Real kB        = s2->k;
	Real RB        = s2->R;
	int  planeNoA  = s1->a.size();
	int  planeNoB  = s2->a.size();
	int  varNo     = 3 + 1 + planeNoA + planeNoB;
	int  planeNoAB = planeNoA + planeNoB;
	int  varNo2    = varNo * varNo;
	int  planeNoA3 = 3 + planeNoA;
	int  planeNoB3 = 3 + planeNoB;
	//int planeNoA2 =planeNoA*planeNoA;
	//int planeNoB2=planeNoB*planeNoB;
	Matrix3r QA = state1.ori.conjugate().toRotationMatrix(); /*direction cosine */
	Matrix3r QB = state2.ori.conjugate().toRotationMatrix(); /*direction cosine */

	int  blas3          = 3;
	char blasNT         = 'N';
	char blasT          = 'T';
	int  blas1planeNoA  = math::max(1, planeNoA);
	int  blas1planeNoB  = math::max(1, planeNoB);
	int  blas1planeNoAB = math::max(1, planeNoAB);
	Real blas0          = 0.0;
	Real blas1          = 1.0;
	Real blasNeg1       = -1.0;

	Real blasQA[9];
	Real blasQB[9];
	Real blasPosA[3];
	Real blasPosB[3];
	Real blasContactPt[3];
	Real blasC[varNo];
	for (int i = 0; i < varNo; i++) {
		blasC[i] = 0.0;
	}
	blasC[3]      = 1.0;
	int blasCount = 0;

	contactPt += shift2;

	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 3; j++) {
			blasQA[blasCount] = QA(j, i);
			blasQB[blasCount] = QB(j, i);
			blasCount++;
		}
		blasPosA[i]      = state1.pos[i];
		blasPosB[i]      = state2.pos[i] + shift2[i];
		blasContactPt[i] = contactPt[i];
	}
	//std::cout<<"QA: "<<QA<<endl;std::cout<<"blasQA: "<<endl;

	//for (int i=0; i<9; i++){
	//	std::cout<<blasQA[i]<<endl;
	//}

	/* Variables to keep things neat */
	Real kAs = sqrt(kA) / RA;
	Real kAp = sqrt(1.0 - kA) / rA;
	Real kBs = sqrt(kB) / RB;
	Real kBp = sqrt(1.0 - kB) / rB;

	/* penalty */
	Real t         = 1.0;
	Real mu        = 10.0;
	Real planePert = 0.1 * rA;
	Real sPert     = 1.0; //+ 10.0*planePert*(planeNoA+planeNoB)/(rA*rA);
	if (warmstart == true) {
		t         = 0.01;
		mu        = 50.0;
		planePert = 0.01;
		sPert     = 0.01; ///* pow(10,-1)+ */ sqrt(planePert*planePert*(planeNoA+planeNoB)/(rA*rA));
	}
	/* s */
	Real s     = 0.0;
	Real m     = 2.0;
	Real NTTOL = pow(10, -8);
	Real tol   = accuracyTol /*pow(10,-4)* RA*pow(10,-6)*/;
	Real gap   = 0.0;
	/* x */
	Real blasX[varNo];
	Real blasNewX[varNo];
	blasX[0] = contactPt[0];
	blasX[1] = contactPt[1];
	blasX[2] = contactPt[2];

	//////////////////// evaluate fA and fB to initialize ////////////////////////////////
	/* fA */
	Real blasTempP1[3];
	Real blasLocalP1[3];
	for (int i = 0; i < 3; i++) {
		blasTempP1[i] = blasContactPt[i] - blasPosA[i];
	}
	char transA = 'N';
	//char transB = 'N';
	//int  blasM = 3;
	//int  blasN = 3;
	int blasK = 3;
	//int blasLDA = 3;
	int  blasLDB   = 3;
	Real blasAlpha = 1.0;
	Real blasBeta  = 0.0;
	//int blasLDC = 3;
	int incx = 1;
	int incy = 1;
	//dgemv_(&transA, &blasM, &blasN, &blasAlpha, &blasQA[0], &blasLDA, &blasTempP1[0], &incx, &blasBeta, &blasLocalP1[0], &incy);
	dgemv_(&blasNT, &blas3, &blas3, &blas1, &blasQA[0], &blas3, &blasTempP1[0], &incx, &blas0, &blasLocalP1[0], &incy);

	//Vector3r tempP1 = contactPt - posA;
	//Vector3r localP1 = QA*tempP1;
	/* P1Q */ //MatrixXr P1 = MatrixXr::Zero(planeNoA,3);
	/*d1*/    //MatrixXr d1 = MatrixXr::Zero(planeNoA,1);

	Real blasP1[planeNoA * 3];
	Real blasD1[planeNoA];
	Real blasP1Q[planeNoA * 3];
	Real pertSumA2 = 0.0;

	for (int i = 0; i < planeNoA; i++) {
		Real plane = s1->a[i] * blasLocalP1[0] + s1->b[i] * blasLocalP1[1] + s1->c[i] * blasLocalP1[2] - s1->d[i];
		if (plane < pow(10, -15)) {
			plane        = 0.0;
			blasX[4 + i] = plane + planePert;
		} else {
			blasX[4 + i] = plane + planePert;
		}
		pertSumA2 += pow((plane + planePert), 2);
		blasP1[i]                = s1->a[i];
		blasP1[i + planeNoA]     = s1->b[i];
		blasP1[i + 2 * planeNoA] = s1->c[i];
		blasD1[i]                = s1->d[i];
	}
	//MatrixXr P1Q = P1*QA;
	//transA = 'N'; transB = 'N'; blasM = planeNoA;  blasN = 3; blasK = 3;
	// blasLDA = math::max(1,blasM);  blasLDC = math::max(1,blasM); blasLDB = 3; blasAlpha=1.0; blasBeta = 0.0;
	//dgemm_(&transA, &transB, &blasM, &blasN, &blasK, &blasAlpha, &blasP1[0], &blasLDA, &blasQA[0], &blasLDB, &blasBeta, &blasP1Q[0], &blasLDC);
	dgemm_(&blasNT, &blasNT, &planeNoA, &blas3, &blas3, &blas1, &blasP1[0], &blas1planeNoA, &blasQA[0], &blasLDB, &blas0, &blasP1Q[0], &blas1planeNoA);

	Real sphereA = (pow(blasLocalP1[0], 2) + pow(blasLocalP1[1], 2) + pow(blasLocalP1[2], 2)) / pow(RA, 2);
	Real sA      = (1.0 - kA) * (pertSumA2 / pow(rA, 2) - 1.0) + kA * (sphereA - 1.0);

	/* fB */
	Real blasTempP2[3];
	Real blasLocalP2[3];
	for (int i = 0; i < 3; i++) {
		blasTempP2[i] = blasContactPt[i] - blasPosB[i];
	}
	//transA = 'N'; blasM = 3;  blasN = 3;  blasLDA = 3;   blasAlpha=1.0; blasBeta = 0.0;
	//dgemv_(&transA, &blasM, &blasN, &blasAlpha, &blasQB[0], &blasLDA, &blasTempP2[0], &incx, &blasBeta, &blasLocalP2[0], &incy);
	dgemv_(&blasNT, &blas3, &blas3, &blas1, &blasQB[0], &blas3, &blasTempP2[0], &incx, &blas0, &blasLocalP2[0], &incy);

	// Vector3r tempP2 = contactPt - posB;
	// Vector3r localP2 = QB*tempP2;
	/*P2Q*/ //MatrixXr P2 = MatrixXr::Zero(planeNoB,3);
	/*d2*/  //MatrixXr d2 = MatrixXr::Zero(planeNoB,1);
	Real blasP2[planeNoB * 3];
	Real blasD2[planeNoB];
	Real blasP2Q[planeNoB * 3];
	Real pertSumB2 = 0.0;
	for (int i = 0; i < planeNoB; i++) {
		Real plane = s2->a[i] * blasLocalP2[0] + s2->b[i] * blasLocalP2[1] + s2->c[i] * blasLocalP2[2] - s2->d[i];
		if (plane < pow(10, -15)) {
			plane                   = 0.0;
			blasX[4 + planeNoA + i] = plane + planePert;
		} else {
			blasX[4 + planeNoA + i] = plane + planePert;
		}
		pertSumB2 += pow((plane + planePert), 2);
		blasP2[i]                = s2->a[i];
		blasP2[i + planeNoB]     = s2->b[i];
		blasP2[i + 2 * planeNoB] = s2->c[i];
		blasD2[i]                = s2->d[i];
	}
	//MatrixXr P2Q = P2*QB;
	//transA = 'N'; transB = 'N'; blasM = planeNoB;    blasN = 3; blasK = 3;
	//blasLDA = math::max(1,blasM);   blasLDC = math::max(1,blasM); blasLDB = 3; blasAlpha=1.0; blasBeta = 0.0;
	// dgemm_(&transA, &transB, &blasM, &blasN, &blasK, &blasAlpha, &blasP2[0], &blasLDA, &blasQB[0], &blasLDB, &blasBeta, &blasP2Q[0], &blasLDC);
	dgemm_(&blasNT, &blasNT, &planeNoB, &blas3, &blas3, &blasAlpha, &blasP2[0], &blas1planeNoB, &blasQB[0], &blas3, &blasBeta, &blasP2Q[0], &blas1planeNoB);
	Real sphereB = (pow(blasLocalP2[0], 2) + pow(blasLocalP2[1], 2) + pow(blasLocalP2[2], 2)) / pow(RB, 2);
	Real sB      = (1.0 - kB) * (pertSumB2 / pow(rB, 2) - 1.0) + kB * (sphereB - 1.0);
	//sPert = fabs(fA-fB);

	s = math::max(sqrt(fabs(sA + 1.0)), sqrt(fabs(sB + 1.0))) + sPert; //sqrt(fA+fB+2.0)+sPert; //sqrt(math::max(fabs(fA+1.0), fabs(fB+1.0))) + sPert; //
	//x[3] = s;
	blasX[3] = s;

	///////////////////  algebra formulation of the SOCP ///////////////////
	/* c */
	// MatrixXr c=MatrixXr::Zero(varNo,1);
	// c[3] = 1.0;

#if (YADE_REAL_BIT <= 64)
	Real blasA1[(3 + planeNoA) * varNo];
	Real blasA2[(3 + planeNoB) * varNo];
#else
	std::vector<Real> blasA1((3 + planeNoA) * varNo, /* fill with */ 0);
	std::vector<Real> blasA2((3 + planeNoB) * varNo, /* fill with */ 0);
#endif
	/* Second order cone constraints */
	/* A1 */
	//MatrixXr A1(3+planeNoA,varNo);
	//Matrix3r QAs=kAs*QA; //cwise()
	Real blasQAs[9];
	int  noElements  = 9;
	Real scaleFactor = kAs;
	dcopy_(&noElements, &blasQA[0], &incx, &blasQAs[0], &incx);
	dscal_(&noElements, &scaleFactor, &blasQAs[0], &incx);

	// A1 << QAs,MatrixXr::Zero(3,1+planeNoAB),
	// MatrixXr::Zero(planeNoA,4),kAp*MatrixXr::Identity(planeNoA, planeNoA),MatrixXr::Zero(planeNoA,planeNoB);

#if (YADE_REAL_BIT <= 64)
	memset(blasA1, 0.0, sizeof(blasA1));
#endif
	// the standard way, perfectly optimized by compiler.
	// NOTE: It is unnecessary here, beacuse it was already filled with zeros when creating it. It's only to show how to fill std::vector.
	//std::fill(blasA1.begin() , blasA1.end() , 0);

	for (int i = 0; i < 3; i++) {
		blasA1[i]                 = blasQAs[i];
		blasA1[i + planeNoA3]     = blasQAs[i + 3];
		blasA1[i + 2 * planeNoA3] = blasQAs[i + 6];
	}
	for (int i = 0; i < planeNoA; i++) {
		blasA1[3 + i + (planeNoA3) * (4 + i)] = kAp;
	}


	/* A2 */
	//MatrixXr A2(3+planeNoB,varNo);
	//Matrix3r QBs=kBs*QB; //cwise();
	Real blasQBs[9];
	noElements  = 9;
	scaleFactor = kBs;
	dcopy_(&noElements, &blasQB[0], &incx, &blasQBs[0], &incx);
	dscal_(&noElements, &scaleFactor, &blasQBs[0], &incx);

	// A2 << QBs,MatrixXr::Zero(3,1+planeNoAB),
	// MatrixXr::Zero(planeNoB,4),MatrixXr::Zero(planeNoB,planeNoA),kBp*MatrixXr::Identity(planeNoB, planeNoB);
#if (YADE_REAL_BIT <= 64)
	memset(blasA2, 0.0, sizeof(blasA2));
#endif
	for (int i = 0; i < 3; i++) {
		blasA2[i]                   = blasQBs[i];
		blasA2[i + planeNoB3]       = blasQBs[i + 3];
		blasA2[i + 2 * (planeNoB3)] = blasQBs[i + 6];
	}

	for (int i = 0; i < planeNoB; i++) {
		blasA2[3 + i + (planeNoB3) * (4 + planeNoA + i)] = kBp;
	}


	/*b1*/
#if 0
	MatrixXr b1=MatrixXr::Zero(3+planeNoA,1);
	Vector3r b1temp = QAs*posA;
	b1[0] = -b1temp[0];
	b1[1]= -b1temp[1];
	b1[2] = -b1temp[2];
#endif

#if (YADE_REAL_BIT <= 64)
	Real blasB1[planeNoA3];
	memset(blasB1, 0.0, sizeof(blasB1));
#else
	std::vector<Real> blasB1(planeNoA3, /* fill with */ 0);
#endif
	Real blasB1temp[3];
	// blasM = 3;   blasN=3;
	//  blasLDA = 3;    blasAlpha = -1.0;  blasBeta=0.0;  blasLDC = 3;
	// dgemv_(&transA, &blasM, &blasN, &blasAlpha, &blasQAs[0], &blasLDA, &blasPosA[0], &incx, &blasBeta, &blasB1temp[0], &incy);
	dgemv_(&blasNT, &blas3, &blas3, &blasNeg1, &blasQAs[0], &blas3, &blasPosA[0], &incx, &blas0, &blasB1temp[0], &incy);
	blasB1[0] = blasB1temp[0];
	blasB1[1] = blasB1temp[1];
	blasB1[2] = blasB1temp[2];

	/*b2*/
#if 0
	MatrixXr b2=MatrixXr::Zero(3+planeNoB,1);
	Vector3r b2temp = QBs*posB;
	b2[0] = -b2temp[0];
	b2[1]= -b2temp[1];
	b2[2] = -b2temp[2];
#endif

#if (YADE_REAL_BIT <= 64)
	Real blasB2[planeNoB3];
	memset(blasB2, 0.0, sizeof(blasB2));
#else
	std::vector<Real> blasB2(planeNoB3, /* fill with */ 0);
#endif
	Real blasB2temp[3];
	//  blasM = 3;   blasN=3; blasLDA = 3; blasAlpha=-1.0; blasBeta=0.0;
	// dgemv_(&transA, &blasM, &blasN, &blasAlpha, &blasQBs[0], &blasLDA, &blasPosB[0], &incx, &blasBeta, &blasB2temp[0], &incy);
	dgemv_(&blasNT, &blas3, &blas3, &blasNeg1, &blasQBs[0], &blas3, &blasPosB[0], &incx, &blas0, &blasB2temp[0], &incy);
	blasB2[0] = blasB2temp[0];
	blasB2[1] = blasB2temp[1];
	blasB2[2] = blasB2temp[2];

	/*AL*/
	// MatrixXr AL(planeNoAB,varNo);
	// AL<<P1Q, MatrixXr::Zero(planeNoA,1), -1.0*MatrixXr::Identity(planeNoA,planeNoA), MatrixXr::Zero(planeNoA,planeNoB), //cwise()
	//	      P2Q, MatrixXr::Zero(planeNoB,1), MatrixXr::Zero(planeNoB,planeNoA), -1.0*MatrixXr::Identity(planeNoB,planeNoB);

#if (YADE_REAL_BIT <= 64)
	Real blasAL[planeNoAB * varNo];
	memset(blasAL, 0.0, sizeof(blasAL));
#else
	std::vector<Real> blasAL(planeNoAB * varNo, /* fill with */ 0);
	//std::fill(blasAL.begin() , blasAL.end() , 0);
#endif
	for (int i = 0; i < planeNoA; i++) {
		blasAL[i]                       = blasP1Q[i];
		blasAL[i + planeNoAB]           = blasP1Q[i + planeNoA];
		blasAL[i + 2 * planeNoAB]       = blasP1Q[i + 2 * planeNoA];
		blasAL[i + (4 + i) * planeNoAB] = -1.0;
	}
	for (int i = 0; i < planeNoB; i++) {
		blasAL[planeNoA + i]                                  = blasP2Q[i];
		blasAL[planeNoA + i + planeNoAB]                      = blasP2Q[i + planeNoB];
		blasAL[planeNoA + i + 2 * planeNoAB]                  = blasP2Q[i + 2 * planeNoB];
		blasAL[planeNoA + i + (4 + planeNoA + i) * planeNoAB] = -1.0;
	}


	/*bL*/
#if 0
	MatrixXr bL(planeNoAB,1);
	MatrixXr pos1(3,1);
	pos1(0,0) = posA[0];
	pos1(1,0) = posA[1];
	pos1(2,0)=posA[2];
	MatrixXr pos2(3,1);
	pos2(0,0) = posB[0];
	pos2(1,0) = posB[1];
	pos2(2,0)=posB[2];
	MatrixXr btempU = P1Q*pos1 + d1;
	MatrixXr btempL = P2Q*pos2 + d2;
	bL<<btempU,btempL;
#endif

	Real blasBL[planeNoAB];
	Real blasBTempU[planeNoA];
	Real blasBTempL[planeNoB];
	noElements = planeNoA;
	dcopy_(&noElements, &blasD1[0], &incx, &blasBTempU[0], &incx);
	//transA = 'N';	blasM = planeNoA;   blasN = 3;
	//blasLDA = math::max(1,planeNoA);      blasAlpha = 1.0;   blasBeta = 1.0;
	//dgemv_(&transA, &blasM, &blasN, &blasAlpha, &blasP1Q[0], &blasLDA,&blasPosA[0], &incx, &blasBeta, &blasBTempU[0], &incy);
	dgemv_(&blasNT, &planeNoA, &blas3, &blas1, &blasP1Q[0], &blas1planeNoA, &blasPosA[0], &incx, &blas1, &blasBTempU[0], &incy);
	noElements = planeNoB;
	dcopy_(&noElements, &blasD2[0], &incx, &blasBTempL[0], &incx);
	//transA = 'N';	blasM = planeNoB;   blasN = 3;
	//blasLDA = math::max(1,planeNoB);      blasAlpha = 1.0;   blasBeta = 1.0;
	//dgemv_(&transA, &blasM, &blasN, &blasAlpha, &blasP2Q[0], &blasLDA,&blasPosB[0], &incx, &blasBeta, &blasBTempL[0], &incy);
	dgemv_(&blasNT, &planeNoB, &blas3, &blas1, &blasP2Q[0], &blas1planeNoB, &blasPosB[0], &incx, &blas1, &blasBTempL[0], &incy);
	for (int i = 0; i < planeNoA; i++) {
		blasBL[i] = blasBTempU[i];
	}
	for (int i = 0; i < planeNoB; i++) {
		blasBL[planeNoA + i] = blasBTempL[i];
	}

	Real u1;
	Real u2;
#if (YADE_REAL_BIT <= 64)
	Real blasCCtranspose[varNo2];
	memset(blasCCtranspose, 0.0, sizeof(blasCCtranspose));
#else
	std::vector<Real> blasCCtranspose(varNo2, 0);
#endif
	blasCCtranspose[3 + varNo * 3] = 1.0;

	Real blasCa1[varNo2];
//#if 0
#if 0
	MatrixXr ca1Transpose = ccTranspose - A1.transpose()*A1;
	MatrixXr ca2Transpose = ccTranspose - A2.transpose()*A2;
#endif
	/* ca1Transpose */
	noElements = varNo2;
	incx       = 1;
	incy       = 1;
	dcopy_(&noElements, &blasCCtranspose[0], &incx, &blasCa1[0], &incy);

	//transA = 'T';   transB = 'N';   blasM = varNo;   blasN = varNo;   blasK = 3+planeNoA;
	//blasLDA = blasK;   blasLDB = blasK;     blasAlpha = -1.0;   blasBeta = 1.0;
	//blasLDC = varNo;
	//dgemm_(&transA, &transB, &blasM, &blasN, &blasK, &blasAlpha, &blasA1[0], &blasLDA, &blasA1[0], &blasLDB, &blasBeta, &blasCa1[0], &blasLDC);
	dgemm_(&blasT, &blasNT, &varNo, &varNo, &planeNoA3, &blasNeg1, &blasA1[0], &planeNoA3, &blasA1[0], &planeNoA3, &blas1, &blasCa1[0], &varNo);


	/* ca2Transpose */
	Real blasCa2[varNo2];
	blasCount = 0;
	dcopy_(&noElements, &blasCCtranspose[0], &incx, &blasCa2[0], &incy);

	//transA = 'T';   transB = 'N';   blasM = varNo;   blasN = varNo;  blasK = 3+planeNoB;
	// blasLDA = blasK;   blasLDB = blasK;     blasAlpha = -1.0;   blasBeta = 1.0;
	// blasLDC = varNo;
	// dgemm_(&transA, &transB, &blasM, &blasN, &blasK, &blasAlpha, &blasA2[0], &blasLDA, &blasA2[0], &blasLDB, &blasBeta, &blasCa2[0], &blasLDC);
	dgemm_(&blasT, &blasNT, &varNo, &varNo, &planeNoB3, &blasNeg1, &blasA2[0], &planeNoB3, &blasA2[0], &planeNoB3, &blas1, &blasCa2[0], &varNo);

	/* DL */
#if (YADE_REAL_BIT <= 64)
	Real blasDL[planeNoAB * planeNoAB];
	memset(blasDL, 0.0, sizeof(blasDL));
#else
	std::vector<Real> blasDL(planeNoAB * planeNoAB, 0);
#endif
	blasCount = 0;

	//#endif


	Real wLlogsum = 0.0;
	Real val      = 0.0;
	Real newval   = 0.0;
	/*Linesearch */
	Real backtrack = 1.0;
	Real penalty   = 1.0 / t;

	/* LAPACK */
	int  info;
	char UPLO = 'L';
	int  KD   = varNo - 1;
	int  nrhs = 1;
	Real HessianChol[varNo][varNo];


	Real blasGA[varNo];
	Real blasGB[varNo];
	Real blasGL[varNo];

	Real blasW1[3 + planeNoA];
	Real blasW2[3 + planeNoB];
	Real blasWL[planeNoAB];
	Real blasVL[planeNoAB];
	Real minWL = 0.0;
	Real blasHA[varNo * varNo];
	Real blasHB[varNo * varNo];
	Real blasHL[varNo * varNo];
	Real blasADAtemp[varNo * planeNoAB];
	noElements     = varNo;
	Real blasW1dot = 0.0;
	Real blasW2dot = 0.0;
	Real blasGrad[varNo];
	Real blasHess[varNo * varNo];
	Real blasStep[varNo];
	Real blasFprime = 0.0;

#if 0
//	MAKE SURE POINTS ARE FEASIBLE //
	/* temp variables */
	/* w1 = A1*x + b1; */
	noElements = 3+planeNoA;
	dcopy_(&noElements, &blasB1[0], &incx, &blasW1[0], &incy);
	//transA = 'N';	blasM = 3+planeNoA;   blasN = varNo;
	//blasLDA = 3+planeNoA;      blasAlpha = 1.0;   blasBeta = 1.0;    incx =1;  incy=1;
	//dgemv_(&transA, &blasM, &blasN, &blasAlpha, &blasA1[0], &blasLDA, &blasX[0], &incx, &blasBeta, &blasW1[0], &incy);
	dgemv_(&blasNT, &planeNoA3, &varNo, &blas1, &blasA1[0], &planeNoA3, &blasX[0], &incx, &blas1, &blasW1[0], &incy);

	/* w2 = A2*x + b2; */
	noElements = 3+planeNoB;
	dcopy_(&noElements, &blasB2[0], &incx, &blasW2[0], &incy);
	//transA = 'N';	blasM = 3+planeNoB;   blasN = varNo;
	//blasLDA = 3+planeNoB;    blasAlpha = 1.0;   blasBeta = 1.0;
	//dgemv_(&transA, &blasM, &blasN, &blasAlpha, &blasA2[0], &blasLDA, &blasX[0], &incx, &blasBeta, &blasW2[0], &incy);
	dgemv_(&blasNT, &planeNoB3, &varNo, &blas1, &blasA2[0], &planeNoB3, &blasX[0], &incx, &blas1, &blasW2[0], &incy);

	/* u1 = s*s - w1.dot(w1); */
	/* u2 = s*s - w2.dot(w2); */
	//noElements = 3+planeNoA;
	blasW1dot = ddot_(&planeNoA3, &blasW1[0], &incx, &blasW1[0], &incy);
	//noElements = 3+planeNoB;
	blasW2dot = ddot_(&planeNoB3, &blasW2[0], &incx, &blasW2[0], &incy);

	s = math::max(sqrt(fabs(blasW1dot)),sqrt(fabs(blasW2dot)))+0.1;
	blasX[3]=s;
#endif

	//timingDeltas->checkpoint("setup");


	while (totalIter < 500) {
		penalty = 1.0 / t;
		/* Newton's method */
		/* s=x[3]; */
		s = blasX[3];

		/* temp variables */
		/* w1 = A1*x + b1; */
		noElements = 3 + planeNoA;
		dcopy_(&noElements, &blasB1[0], &incx, &blasW1[0], &incy);
		//transA = 'N';	blasM = 3+planeNoA;   blasN = varNo;
		//blasLDA = 3+planeNoA;      blasAlpha = 1.0;   blasBeta = 1.0;    incx =1;  incy=1;
		//dgemv_(&transA, &blasM, &blasN, &blasAlpha, &blasA1[0], &blasLDA, &blasX[0], &incx, &blasBeta, &blasW1[0], &incy);
		dgemv_(&blasNT, &planeNoA3, &varNo, &blas1, &blasA1[0], &planeNoA3, &blasX[0], &incx, &blas1, &blasW1[0], &incy);

		/* w2 = A2*x + b2; */
		noElements = 3 + planeNoB;
		dcopy_(&noElements, &blasB2[0], &incx, &blasW2[0], &incy);
		//transA = 'N';	blasM = 3+planeNoB;   blasN = varNo;
		//blasLDA = 3+planeNoB;    blasAlpha = 1.0;   blasBeta = 1.0;
		//dgemv_(&transA, &blasM, &blasN, &blasAlpha, &blasA2[0], &blasLDA, &blasX[0], &incx, &blasBeta, &blasW2[0], &incy);
		dgemv_(&blasNT, &planeNoB3, &varNo, &blas1, &blasA2[0], &planeNoB3, &blasX[0], &incx, &blas1, &blasW2[0], &incy);

		/* u1 = s*s - w1.dot(w1); */
		/* u2 = s*s - w2.dot(w2); */
		//noElements = 3+planeNoA;
		blasW1dot = ddot_(&planeNoA3, &blasW1[0], &incx, &blasW1[0], &incy);
		//noElements = 3+planeNoB;
		blasW2dot = ddot_(&planeNoB3, &blasW2[0], &incx, &blasW2[0], &incy);
		u1        = s * s - blasW1dot;
		u2        = s * s - blasW2dot;

		/* wL = bL - AL*x; */
		noElements = planeNoAB;
		dcopy_(&noElements, &blasBL[0], &incx, &blasWL[0], &incy);
		//transA = 'N';	blasN = varNo;
		//blasM = planeNoAB;     blasLDA = math::max(1,planeNoAB);      blasAlpha = -1.0;   blasBeta = 1.0;
		//dgemv_(&transA, &blasM, &blasN, &blasAlpha, &blasAL[0], &blasLDA, &blasX[0], &incx, &blasBeta, &blasWL[0], &incy);
		dgemv_(&blasNT, &planeNoAB, &varNo, &blasNeg1, &blasAL[0], &blas1planeNoAB, &blasX[0], &incx, &blas1, &blasWL[0], &incy);

		for (int i = 0; i < planeNoAB; i++) {
			blasVL[i]                 = 1.0 / blasWL[i];
			blasDL[i * planeNoAB + i] = blasVL[i] * blasVL[i];
		}

		/* Gradient */
		//gA = -2.0/u1*(s*c - A1.transpose()*w1); //-2.0/u1*(s*c - tempG); //
		//gB = -2.0/u2*(s*c - A2.transpose()*w2); //-2.0/u2*(s*c - tempG); //
		// gL = AL.transpose()*vL; //tempG; //
		// g = (c + penalty*(gA + gB + gL) );


		/* gA */
		//noElements = varNo;
		dcopy_(&varNo, &blasC[0], &incx, &blasGA[0], &incy);
		//transA = 'T';      blasM = 3+planeNoA;   blasLDA = 3+planeNoA;
		blasAlpha = -1.0 * -2.0 / u1;
		blasBeta  = 1.0 * -2.0 / u1 * s;
		//dgemv_(&transA, &blasM, &blasN, &blasAlpha, &blasA1[0], &blasLDA, &blasW1[0], &incx, &blasBeta, &blasGA[0], &incy);
		dgemv_(&blasT, &planeNoA3, &varNo, &blasAlpha, &blasA1[0], &planeNoA3, &blasW1[0], &incx, &blasBeta, &blasGA[0], &incy);

		/* gB */
		dcopy_(&varNo, &blasC[0], &incx, &blasGB[0], &incy);
		//transA = 'T';  blasM = 3+planeNoB;   blasLDA = 3+planeNoB;
		blasAlpha = -1.0 * -2.0 / u2;
		blasBeta  = 1.0 * -2.0 / u2 * s;
		//dgemv_(&transA, &blasM, &blasN, &blasAlpha, &blasA2[0], &blasLDA, &blasW2[0], &incx, &blasBeta, &blasGB[0], &incy);
		dgemv_(&blasT, &planeNoB3, &varNo, &blasAlpha, &blasA2[0], &planeNoB3, &blasW2[0], &incx, &blasBeta, &blasGB[0], &incy);

		/* gL */
		//transA = 'T'; blasM = planeNoAB;   blasLDA = math::max(1,planeNoAB);      blasAlpha = 1.0;   blasBeta = 0.0;
		//dgemv_(&transA, &blasM, &blasN, &blasAlpha, &blasAL[0], &blasLDA, &blasVL[0], &incx, &blasBeta, &blasGL[0], &incy);
		dgemv_(&blasT, &planeNoAB, &varNo, &blas1, &blasAL[0], &blas1planeNoAB, &blasVL[0], &incx, &blas0, &blasGL[0], &incy);


		/* g */
		for (int i = 0; i < varNo; i++) {
			blasGrad[i] = blasC[i] + penalty * (blasGA[i] + blasGB[i] + blasGL[i]);
		}

		/* Hessian */
		/* HA */ //MatrixXr HA =  (-2.0/u1)*(ca1Transpose)+ gA*gA.transpose();
		//noElements = varNo2;
		dcopy_(&varNo2, &blasCa1[0], &incx, &blasHA[0], &incy);
		//transA = 'N';   transB = 'T';   blasM = varNo;   blasN = varNo;   blasK = 1;blasLDA = blasM;   blasLDB = blasN;     blasAlpha = 1.0;  blasLDC = blasM;
		blasBeta = -2.0 / (u1);
		blasK    = 1;
		//dgemm_(&transA, &transB, &blasM, &blasN, &blasK, &blasAlpha, &blasGA[0], &blasLDA, &blasGA[0], &blasLDB, &blasBeta, &blasHA[0], &blasLDC);
		dgemm_(&blasNT, &blasT, &varNo, &varNo, &blasK, &blas1, &blasGA[0], &varNo, &blasGA[0], &varNo, &blasBeta, &blasHA[0], &varNo);

		/* HB */ // HB =  (-2.0/u2)*(ca2Transpose)+ gB*gB.transpose();
		dcopy_(&varNo2, &blasCa2[0], &incx, &blasHB[0], &incy);
		//transA = 'N';   transB = 'T';   blasM = varNo;   blasN = varNo;   blasK = 1; blasLDA = blasM;   blasLDB = blasN;     blasAlpha = 1.0;      blasLDC = blasM;
		blasBeta = -2.0 / (u2);
		//dgemm_(&transA, &transB, &blasM, &blasN, &blasK, &blasAlpha, &blasGB[0], &blasLDA, &blasGB[0], &blasLDB, &blasBeta, &blasHB[0], &blasLDC);
		dgemm_(&blasNT, &blasT, &varNo, &varNo, &blasK, &blas1, &blasGB[0], &varNo, &blasGB[0], &varNo, &blasBeta, &blasHB[0], &varNo);


		/* HL */ //MatrixXr HL1 = AL.transpose()*DL*AL;
		//transA = 'T';   transB = 'N';   blasM = varNo;   blasN = planeNoAB;   blasK = planeNoAB;
		//blasLDA = math::max(1,blasK);   blasLDB = math::max(1,blasK);     blasAlpha = 1.0;   blasBeta = 0.0; blasLDC = blasM;
		//dgemm_(&transA, &transB, &blasM, &blasN, &blasK, &blasAlpha, &blasAL[0], &blasLDA, &blasDL[0], &blasLDB, &blasBeta, &blasADAtemp[0], &blasLDC);
		dgemm_(&blasT,
		       &blasNT,
		       &varNo,
		       &planeNoAB,
		       &planeNoAB,
		       &blas1,
		       &blasAL[0],
		       &blas1planeNoAB,
		       &blasDL[0],
		       &blas1planeNoAB,
		       &blas0,
		       &blasADAtemp[0],
		       &varNo);
		//transA = 'N';   transB = 'N';   blasM = varNo;   blasN = varNo;   blasK = planeNoAB;
		//blasLDA = blasM;   blasLDB = math::max(1,blasK);     blasAlpha = 1.0;   blasBeta = 0.0;  blasLDC = blasM;
		//dgemm_(&transA, &transB, &blasM, &blasN, &blasK, &blasAlpha, &blasADAtemp[0], &blasLDA, &blasAL[0], &blasLDB, &blasBeta, &blasHL[0], &blasLDC);
		dgemm_(&blasNT, &blasNT, &varNo, &varNo, &planeNoAB, &blas1, &blasADAtemp[0], &varNo, &blasAL[0], &blas1planeNoAB, &blas0, &blasHL[0], &varNo);

		/* H */ // H = penalty*(HA + HB + HL);
		for (int i = 0; i < varNo2; i++) {
			blasHess[i] = penalty * (blasHA[i] + blasHB[i] + blasHL[i]);
		}


		//timingDeltas->checkpoint("assemble H and g");

		//std::cout<<"before Chol, totalIter: "<<totalIter<<", s : "<<s<<", u1: "<<u1<<", u2: "<<u2<<", wLmincoeff: "<<minWL<<endl;

		/* Cholesky factorization */
		//#if 0
		/* L */
		for (int j = 1; j <= varNo; j++) {
			blasStep[j - 1] = -blasGrad[j - 1];
			for (int i = j; i <= j + KD && i <= varNo; i++) {
				HessianChol[j - 1][1 + i - j - 1] = blasHess[(i - 1) + varNo * (j - 1)];
			}
		}
		//#endif

#if 0
		/* U */
		for( int j=1; j<=varNo ; j++) {
			blasStep[j-1]=-blasGrad[j-1];
			for (int i=math::max(1,j-KD); i<=j ; i++) {
				HessianChol[j-1][KD+1+i-j-1]    = blasHess[(i-1)+varNo*(j-1)]; //H(i-1,j-1);
			}
		}
#endif
		dpbsv_(&UPLO, &varNo, &KD, &nrhs, &HessianChol[0][0], &varNo, blasStep, &varNo, &info);
		if (info != 0) {
			//std::cout<<"chol error"<<", planeA: "<<planeNoA<<", planeB: "<<planeNoB<<endl;
			//return false;
			/* LU factorization */
			for (int i = 0; i < varNo; i++) {
				blasStep[i] = -1.0 * blasGrad[i]; //g[i];
			}
			int ipiv[varNo];
			int bColNo = 1;
			dgesv_(&varNo, &bColNo, blasHess, &varNo, ipiv, blasStep, &varNo, &info);
		}

		if (info != 0) {
			//std::cout<<"linear algebra error"<<endl;
		}

		//timingDeltas->checkpoint("Cholesky");

		//fprime = step.transpose()*g;
		//noElements = varNo;
		blasFprime = ddot_(&varNo, &blasStep[0], &incx, &blasGrad[0], &incy);

		wLlogsum = 0.0;
		for (int i = 0; i < planeNoAB; i++) {
			//wLlogsum += log(wL[i]);
			wLlogsum += log(blasWL[i]);
		}

		val = /* c.transpose()*/ blasX[3] - penalty * (log(u1) + log(u2) + wLlogsum);

		/*Linesearch */
		backtrack = 1.0;
		//noElements = varNo;
		dcopy_(&varNo, &blasX[0], &incx, &blasNewX[0], &incy);
		daxpy_(&varNo, &backtrack, &blasStep[0], &incx, &blasNewX[0], &incy);


		s = blasNewX[3];
		// w1 = A1*x + b1;

		dcopy_(&planeNoA3, &blasB1[0], &incx, &blasW1[0], &incy);
		//transA = 'N';	blasAlpha = 1.0;   blasBeta = 1.0;
		//dgemv_(&transA, &blasM, &blasN, &blasAlpha, &blasA1[0], &blasLDA, &blasNewX[0], &incx, &blasBeta, &blasW1[0], &incy);
		dgemv_(&blasNT, &planeNoA3, &varNo, &blas1, &blasA1[0], &planeNoA3, &blasNewX[0], &incx, &blas1, &blasW1[0], &incy);

		// w2 = A2*x + b2;
		dcopy_(&planeNoB3, &blasB2[0], &incx, &blasW2[0], &incy);
		//transA = 'N';	blasAlpha = 1.0;   blasBeta = 1.0;
		//dgemv_(&transA, &planeNoB3, &varNo, &blasAlpha, &blasA2[0], &planeNoB3, &blasNewX[0], &incx, &blasBeta, &blasW2[0], &incy);
		dgemv_(&blasNT, &planeNoB3, &varNo, &blas1, &blasA2[0], &planeNoB3, &blasNewX[0], &incx, &blas1, &blasW2[0], &incy);


		blasW1dot = ddot_(&planeNoA3, &blasW1[0], &incx, &blasW1[0], &incy);
		blasW2dot = ddot_(&planeNoB3, &blasW2[0], &incx, &blasW2[0], &incy);

		//u1 = s*s - w1.dot(w1);
		//u2 = s*s - w2.dot(w2);
		u1 = s * s - blasW1dot;
		u2 = s * s - blasW2dot;

		//wL = bL - AL*x;
		dcopy_(&planeNoAB, &blasBL[0], &incx, &blasWL[0], &incy);
		//blasAlpha = -1.0;
		//blasM=planeNoAB; blasN=varNo; blasLDA=math::max(1,planeNoAB);
		//dgemv_(&transA, &blasM, &blasN, &blasAlpha, &blasAL[0], &blasLDA, &blasNewX[0], &incx, &blasBeta, &blasWL[0], &incy);
		dgemv_(&transA, &planeNoAB, &varNo, &blasNeg1, &blasAL[0], &blas1planeNoAB, &blasNewX[0], &incx, &blas1, &blasWL[0], &incy);

		minWL = 0.00001;
		if (planeNoAB > 0) {
			minWL = blasWL[0];
		}
		for (int i = 0; i < planeNoAB; i++) {
			if (blasWL[i] < minWL) {
				minWL = blasWL[i];
			}
		}

		int count = 0;
		while (s < 0.0 || u1 < 0.0 || u2 < 0.0 || minWL < 0.0) {
			backtrack = 0.5 * backtrack;
			dcopy_(&varNo, &blasX[0], &incx, &blasNewX[0], &incy);
			daxpy_(&varNo, &backtrack, &blasStep[0], &incx, &blasNewX[0], &incy);

			s = blasNewX[3];
			// w1 = A1*x + b1;

			dcopy_(&planeNoA3, &blasB1[0], &incx, &blasW1[0], &incy);
			//transA = 'N';	blasAlpha = 1.0;   blasBeta = 1.0;
			//dgemv_(&transA, &blasM, &blasN, &blasAlpha, &blasA1[0], &blasLDA, &blasNewX[0], &incx, &blasBeta, &blasW1[0], &incy);
			dgemv_(&blasNT, &planeNoA3, &varNo, &blas1, &blasA1[0], &planeNoA3, &blasNewX[0], &incx, &blas1, &blasW1[0], &incy);

			// w2 = A2*x + b2;
			dcopy_(&planeNoB3, &blasB2[0], &incx, &blasW2[0], &incy);
			//transA = 'N';	blasAlpha = 1.0;   blasBeta = 1.0;
			//dgemv_(&transA, &blasM, &blasN, &blasAlpha, &blasA2[0], &blasLDA, &blasNewX[0], &incx, &blasBeta, &blasW2[0], &incy);
			dgemv_(&blasNT, &planeNoB3, &varNo, &blas1, &blasA2[0], &planeNoB3, &blasNewX[0], &incx, &blas1, &blasW2[0], &incy);
			blasW1dot = ddot_(&planeNoA3, &blasW1[0], &incx, &blasW1[0], &incy);
			blasW2dot = ddot_(&planeNoB3, &blasW2[0], &incx, &blasW2[0], &incy);

			//u1 = s*s - w1.dot(w1);
			//u2 = s*s - w2.dot(w2);
			u1 = s * s - blasW1dot;
			u2 = s * s - blasW2dot;

			//wL = bL - AL*x;

			dcopy_(&planeNoAB, &blasBL[0], &incx, &blasWL[0], &incy);
			// blasAlpha = -1.0;
			//blasM = planeNoAB;   blasLDA = math::max(1,planeNoAB);     blasN = varNo;    blasAlpha = -1.0;    blasBeta = 1.0;
			//dgemv_(&transA, &blasM, &blasN, &blasAlpha, &blasAL[0], &blasLDA, &blasNewX[0], &incx, &blasBeta, &blasWL[0], &incy);
			dgemv_(&blasNT, &planeNoAB, &varNo, &blasNeg1, &blasAL[0], &blas1planeNoAB, &blasNewX[0], &incx, &blas1, &blasWL[0], &incy);
			if (planeNoAB > 0) {
				minWL = blasWL[0];
			}
			for (int i = 0; i < planeNoAB; i++) {
				if (blasWL[i] < minWL) {
					minWL = blasWL[i];
				}
			}
			count++;
			//std::cout<<"count: "<<count<<", s : "<<s<<", u1: "<<u1<<", u2: "<<u2<<", wLmincoeff: "<<minWL<<endl;
			if (count == 200) {
				//std::cout<<"count: "<<count<<", totalIter: "<<totalIter<<", backtrack: "<<backtrack<<"s : "<<s<<", u1: "<<u1<<", u2: "<<u2<<", wLmincoeff: "<<minWL<<endl;
				//std::cout<<"wL: "<<endl<<wL<<endl<<endl;
			}
		}

		//timingDeltas->checkpoint("barrier search");

		wLlogsum = 0.0;
		for (int i = 0; i < planeNoAB; i++) {
			wLlogsum += log(blasWL[i]);
		}


		newval = /*c.tranpose()*/ blasNewX[3] - penalty * (log(u1) + log(u2) + wLlogsum);
		count  = 0;
		while (newval > val + backtrack * 0.01 * blasFprime) {
			backtrack = 0.5 * backtrack;
			//for (int i = 0; i<varNo; i++){
			//	blasNewX[i] = blasX[i] + backtrack*blasStep[i];
			//}
			//noElements = varNo;
			dcopy_(&varNo, &blasX[0], &incx, &blasNewX[0], &incy);
			daxpy_(&varNo, &backtrack, &blasStep[0], &incx, &blasNewX[0], &incy);

			s = blasNewX[3];
			// w1 = A1*x + b1;
			//noElements = 3+planeNoA;
			dcopy_(&planeNoA3, &blasB1[0], &incx, &blasW1[0], &incy);
			//transA = 'N';	blasM = 3+planeNoA;   blasN = varNo;
			//blasLDA = 3+planeNoA;      blasAlpha = 1.0;   blasBeta = 1.0;    incx =1;  incy=1;
			//dgemv_(&transA, &blasM, &blasN, &blasAlpha, &blasA1[0], &blasLDA, &blasNewX[0], &incx, &blasBeta, &blasW1[0], &incy);
			dgemv_(&blasNT, &planeNoA3, &varNo, &blas1, &blasA1[0], &planeNoA3, &blasNewX[0], &incx, &blas1, &blasW1[0], &incy);

			// w2 = A2*x + b2;
			//noElements = 3+planeNoB;
			dcopy_(&planeNoB3, &blasB2[0], &incx, &blasW2[0], &incy);
			//blasM = 3+planeNoB;   blasLDA = 3+planeNoB;     blasN = varNo;
			//dgemv_(&transA, &blasM, &blasN, &blasAlpha, &blasA2[0], &blasLDA, &blasNewX[0], &incx, &blasBeta, &blasW2[0], &incy);
			dgemv_(&blasNT, &planeNoB3, &varNo, &blas1, &blasA2[0], &planeNoB3, &blasNewX[0], &incx, &blas1, &blasW2[0], &incy);

			//noElements = 3+planeNoA;
			blasW1dot = ddot_(&planeNoA3, &blasW1[0], &incx, &blasW1[0], &incy);
			//noElements = 3+planeNoB;
			blasW2dot = ddot_(&planeNoB3, &blasW2[0], &incx, &blasW2[0], &incy);

			//u1 = s*s - w1.dot(w1);
			//u2 = s*s - w2.dot(w2);
			u1 = s * s - blasW1dot;
			u2 = s * s - blasW2dot;

			//wL = bL - AL*x;
			//noElements = planeNoAB;
			dcopy_(&planeNoAB, &blasBL[0], &incx, &blasWL[0], &incy);
			//blasM = planeNoAB;   blasLDA = math::max(1,planeNoAB);     blasN = varNo;    blasAlpha = -1.0;    blasBeta = 1.0;
			//dgemv_(&transA, &blasM, &blasN, &blasAlpha, &blasAL[0], &blasLDA, &blasNewX[0], &incx, &blasBeta, &blasWL[0], &incy);
			dgemv_(&blasNT, &planeNoAB, &varNo, &blasNeg1, &blasAL[0], &blas1planeNoAB, &blasNewX[0], &incx, &blas1, &blasWL[0], &incy);

			count++;
			if (count == 200) {
				//std::cout<<"count: "<<count<<", totalIter: "<<totalIter<<", backtrack: "<<backtrack<<"s : "<<s<<", u1: "<<u1<<", u2: "<<u2<<", wLmincoeff: "<<minWL<<endl;
				//std::cout<<"wL: "<<endl<<wL<<endl<<endl;
			}

			wLlogsum = 0.0;
			for (int i = 0; i < planeNoAB; i++) {
				wLlogsum += log(blasWL[i]);
			}

			newval = /* c.transpose()*/ blasNewX[3] - penalty * (log(u1) + log(u2) + wLlogsum);

			count++;
			if (backtrack < pow(10, -11)) {
				//std::cout<<"iter: "<<iter<<", totalIter: "<<totalIter<<", backtrack: "<<backtrack<<", val: "<<val<<", newval: "<<newval<<", t: "<<t<<", fprime: "<<blasFprime<<endl;

				break;
			}
		}
		//timingDeltas->checkpoint("line search");

		noElements = varNo;
		dcopy_(&noElements, &blasNewX[0], &incx, &blasX[0], &incy);


		if (blasFprime > 0.0) {
			//std::cout<<"count: "<<count<<", totalIter: "<<totalIter<<", blasFprime: "<<blasFprime<<endl;
		}


		if (-blasFprime * 0.5 < NTTOL) {
			gap = 2.0 * m / t;
			//if (warmstart == true){break;}
			if (gap < tol) {
				contactPt[0] = blasX[0];
				contactPt[1] = blasX[1];
				contactPt[2] = blasX[2];
				//if(warmstart==true){std::cout<<" totalIter : "<<totalIter<<endl;}

				//FIXME: check whether we need to output a warning here or else comment the indented 4 lines below
				Real fA = evaluatePP(cm1, state1, Vector3r(0, 0, 0), contactPt);
				Real fB = evaluatePP(cm2, state2, Vector3r(0, 0, 0), contactPt);
				if (fabs(fA - fB) > 0.001) {
					//std::cout<<"inside fA-fB: "<<fA-fB<<endl;
				}

				//timingDeltas->checkpoint("newton");
				return true;
			}
			t         = math::min(t * mu, (2.0 * m + 1.0) / tol);
			iter      = 0;
			totalIter = totalIter + 1;
		}
		if (totalIter > 100) {
			//std::cout<<"totalIter: "<<totalIter<<", t: "<<t<<", gap: "<<2.0*m/t<<", blasFprime: "<<blasFprime<<endl;
			for (int i = 0; i < varNo; i++) {
				//std::cout<<"blasStep, i"<<i<<", value: "<<blasStep[i]<<endl;
			}
			for (int i = 0; i < varNo; i++) {
				//std::cout<<"blasGrad, i"<<i<<", value: "<<blasGrad[i]<<endl;
			}
			return false;
			//break;
		}
		iter++;
		totalIter++;

		//timingDeltas->checkpoint("complete");
	}


	return (true);
}

} // namespace yade

#endif // YADE_POTENTIAL_PARTICLES
