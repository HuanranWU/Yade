// 2017 © Raphael Maurin <raphael.maurin@imft.fr>
// 2017 © Julien Chauchat <julien.chauchat@legi.grenoble-inp.fr>

#include "HydroForceEngine.hpp"
#include <lib/high-precision/Constants.hpp>
#include <lib/smoothing/LinearInterpolate.hpp>
#include <core/Scene.hpp>
#include <pkg/common/Sphere.hpp>
#include <pkg/dem/Shop.hpp>

#include <core/IGeom.hpp>
#include <core/IPhys.hpp>

#include <boost/optional.hpp>
#include <boost/random/linear_congruential.hpp>
#include <boost/random/normal_distribution.hpp>
#include <boost/random/variate_generator.hpp>

namespace yade { // Cannot have #include directive inside.

using math::max;
using math::min;

YADE_PLUGIN((HydroForceEngine));

void HydroForceEngine::action()
{
	/* Application of hydrodynamical forces */
	Vector3r gravityBuoyancy = gravity;
	if (steadyFlow == true)
		gravityBuoyancy[0] = 0.; // If the fluid flow is steady, no streamwise buoyancy contribution from gravity
	FOREACH(Body::id_t id, ids)
	{
		Body* b = Body::byId(id, scene).get();
		if (!b)
			continue;
		if (!(scene->bodies->exists(id)))
			continue;
		const Sphere* sphere = dynamic_cast<Sphere*>(b->shape.get());
		if (sphere) {
			Vector3r posSphere = b->state->pos;                                    //position vector of the sphere
			int      p         = int(math::floor((posSphere[2] - zRef) / deltaZ)); //cell number in which the particle is
			if ((p < nCell) && (p >= 0)) {
				Vector3r liftForce    = Vector3r::Zero();
				Vector3r dragForce    = Vector3r::Zero();
				Vector3r convAccForce = Vector3r::Zero();
				//deterministic version
				Vector3r vRel
				        = Vector3r(vxFluid[p + 1] + vFluctX[id], vFluctY[id], vFluctZ[id]) - b->state->vel; //fluid-particle relative velocity
				//Drag force calculation
				if (vRel.norm() != 0.0) {
					dragForce = 0.5 * densFluid * Mathr::PI * pow(sphere->radius, 2.0)
					        * (0.44 * vRel.norm() + 24.4 * viscoDyn / (densFluid * sphere->radius * 2)) * pow(1 - phiPart[p], -expoRZ)
					        * vRel;
				}
				//lift force calculation due to difference of fluid pressure between top and bottom of the particle
				int intRadius = int(math::floor(sphere->radius / deltaZ));
				if ((p + intRadius < nCell) && (p - intRadius > 0) && (lift == true)) {
					Real vRelTop = vxFluid[p + 1 + intRadius]
					        - b->state->vel[0]; // relative velocity of the fluid wrt the particle at the top of the particle
					Real vRelBottom = vxFluid[p + 1 - intRadius] - b->state->vel[0]; // same at the bottom
					liftForce[2]
					        = 0.5 * densFluid * Mathr::PI * pow(sphere->radius, 2.0) * Cl * (vRelTop * vRelTop - vRelBottom * vRelBottom);
				}
				//buoyant weight force calculation
				Vector3r buoyantForce = -4.0 / 3.0 * Mathr::PI * pow(sphere->radius, 3.0) * densFluid * gravityBuoyancy;
				if (convAccOption == true) {
					convAccForce[0] = -convAcc[p];
				}
				//add the hydro forces to the particle
				scene->forces.addForce(id, dragForce + liftForce + buoyantForce + convAccForce);
			}
		}
	}
}

void HydroForceEngine::averageProfile()
{
	//Initialization
	int      minZ;
	int      maxZ;
	int      numLayer;
	Real     deltaCenter;
	Real     zInf;
	Real     zSup;
	Real     volPart;
	Vector3r uRel  = Vector3r::Zero();
	Vector3r fDrag = Vector3r::Zero();

	int          nMax = nCell;
	vector<Real> velAverageX(nMax, 0.0);
	vector<Real> velAverageY(nMax, 0.0);
	vector<Real> velAverageZ(nMax, 0.0);
	vector<Real> phiAverage(nMax, 0.0);
	vector<Real> dragAverage(nMax, 0.0);
	vector<Real> phiAverage1(nMax, 0.0);
	vector<Real> dragAverage1(nMax, 0.0);
	vector<Real> velAverageX1(nMax, 0.0);
	vector<Real> velAverageY1(nMax, 0.0);
	vector<Real> velAverageZ1(nMax, 0.0);
	vector<Real> phiAverage2(nMax, 0.0);
	vector<Real> dragAverage2(nMax, 0.0);
	vector<Real> velAverageX2(nMax, 0.0);
	vector<Real> velAverageY2(nMax, 0.0);
	vector<Real> velAverageZ2(nMax, 0.0);

	//Loop over the particles
	for (const auto& b : *Omega::instance().getScene()->bodies) {
		shared_ptr<Sphere> s = YADE_PTR_DYN_CAST<Sphere>(b->shape);
		if (!s)
			continue;
		const Real zPos = b->state->pos[2] - zRef;
		int        Np   = int(math::floor(
                        zPos
                        / deltaZ)); //Define the layer number with 0 corresponding to zRef. Let the z position wrt to zero, that way all z altitude are positive. (otherwise problem with volPart evaluation)
		//		if ((b->state->blockedDOFs==State::DOF_ALL)&&(zPos > s->radius)) continue;// to remove contribution from the fixed particles on the sidewalls.

		// Relative fluid/particle velocity using also the associated fluid vel. fluct.
		if ((Np >= 0) && (Np < nCell)) {
			uRel = Vector3r(vxFluid[Np + 1] + vFluctX[b->id], vFluctY[b->id], vFluctZ[b->id]) - b->state->vel;
			// Drag force with a Dallavalle formulation (drag coef.) and Richardson-Zaki Correction (hindrance effect)
			fDrag = 0.5 * Mathr::PI * pow(s->radius, 2.0) * densFluid * (0.44 * uRel.norm() + 24.4 * viscoDyn / (densFluid * 2.0 * s->radius))
			        * pow((1 - phiPart[Np]), -expoRZ) * uRel;
		} else
			fDrag = Vector3r::Zero();

		minZ        = int(math::floor((zPos - s->radius) / deltaZ));
		maxZ        = int(math::floor((zPos + s->radius) / deltaZ));
		deltaCenter = zPos - Np * deltaZ;

		// Loop over the cell in which the particle is contained
		numLayer = minZ;
		while (numLayer <= maxZ) {
			if ((numLayer >= 0)
			    && (numLayer
			        < nMax)) { //average under zRef does not interest us, avoid also negative values not compatible with the evaluation of volPart
				zInf = (numLayer - Np - 1) * deltaZ + deltaCenter;
				zSup = (numLayer - Np) * deltaZ + deltaCenter;
				if (zInf < -s->radius)
					zInf = -s->radius;
				if (zSup > s->radius)
					zSup = s->radius;

				//Analytical formulation of the volume of a slice of sphere
				volPart = Mathr::PI * pow(s->radius, 2) * (zSup - zInf + (pow(zInf, 3) - pow(zSup, 3)) / (3 * pow(s->radius, 2)));

				phiAverage[numLayer] += volPart;
				velAverageX[numLayer] += volPart * b->state->vel[0];
				velAverageY[numLayer] += volPart * b->state->vel[1];
				velAverageZ[numLayer] += volPart * b->state->vel[2];
				dragAverage[numLayer] += volPart * fDrag[0];
				if (twoSize == true) {
					if (s->radius == radiusPart1) {
						phiAverage1[numLayer] += volPart;
						dragAverage1[numLayer] += volPart * fDrag[0];
						velAverageX1[numLayer] += volPart * b->state->vel[0];
						velAverageY1[numLayer] += volPart * b->state->vel[1];
						velAverageZ1[numLayer] += volPart * b->state->vel[2];
					}
					if (s->radius == radiusPart2) {
						phiAverage2[numLayer] += volPart;
						dragAverage2[numLayer] += volPart * fDrag[0];
						velAverageX2[numLayer] += volPart * b->state->vel[0];
						velAverageY2[numLayer] += volPart * b->state->vel[1];
						velAverageZ2[numLayer] += volPart * b->state->vel[2];
					}
				}
			}
			numLayer += 1;
		}
	}
	//Normalized the weighted velocity by the volume of particles contained inside the cell
	for (int n = 0; n < nMax; n++) {
		if (phiAverage[n] != 0) {
			velAverageX[n] /= phiAverage[n];
			velAverageY[n] /= phiAverage[n];
			velAverageZ[n] /= phiAverage[n];
			dragAverage[n] /= phiAverage[n];
			//Normalize the concentration after
			phiAverage[n] /= vCell;
			if (twoSize == true) {
				if (phiAverage1[n] != 0) {
					dragAverage1[n] /= phiAverage1[n];
					velAverageX1[n] /= phiAverage1[n];
					velAverageY1[n] /= phiAverage1[n];
					velAverageZ1[n] /= phiAverage1[n];
				} else {
					dragAverage1[n] = 0.0;
					velAverageX1[n] = 0.0;
					velAverageY1[n] = 0.0;
					velAverageZ1[n] = 0.0;
				}
				if (phiAverage2[n] != 0) {
					dragAverage2[n] /= phiAverage2[n];
					velAverageX2[n] /= phiAverage2[n];
					velAverageY2[n] /= phiAverage2[n];
					velAverageZ2[n] /= phiAverage2[n];
				} else {
					dragAverage2[n] = 0.0;
					velAverageX2[n] = 0.0;
					velAverageY2[n] = 0.0;
					velAverageZ2[n] = 0.0;
				}
				phiAverage1[n] /= vCell;
				phiAverage2[n] /= vCell;
			}
		} else {
			velAverageX[n] = 0.0;
			velAverageY[n] = 0.0;
			velAverageZ[n] = 0.0;
			dragAverage[n] = 0.0;
			if (twoSize == true) {
				dragAverage1[n] = 0.0;
				dragAverage2[n] = 0.0;
				velAverageX1[n] = 0.0;
				velAverageY1[n] = 0.0;
				velAverageZ1[n] = 0.0;
				velAverageX2[n] = 0.0;
				velAverageY2[n] = 0.0;
				velAverageZ2[n] = 0.0;
			}
		}
	}
	//Assign the results to the global/public variables of HydroForceEngine
	phiPart      = phiAverage;
	vxPart       = velAverageX;
	vyPart       = velAverageY;
	vzPart       = velAverageZ;
	averageDrag  = dragAverage;
	phiPart1     = phiAverage1; //Initialize everything to zero if the twoSize option is not activated
	phiPart2     = phiAverage2;
	averageDrag1 = dragAverage1;
	averageDrag2 = dragAverage2;
	vxPart1      = velAverageX1;
	vyPart1      = velAverageY1;
	vzPart1      = velAverageZ1;
	vxPart2      = velAverageX2;
	vyPart2      = velAverageY2;
	vzPart2      = velAverageZ2;
}


void HydroForceEngine::averageProfilePP()
{
	//Initialization
	// I had a warning ‘volPart’ may be used uninitialized in this function [-Wmaybe-uninitialized], but I don't know what should be the initialization value
	// so instead I use optional which remembers when it is not initialized. // Janek
	boost::optional<Real> volPart = boost::make_optional<Real>(false, -10);
	Vector3r              uRel    = Vector3r::Zero();
	Vector3r              fDrag   = Vector3r::Zero();

	int          nMax = nCell;
	vector<Real> velAverageX(nMax, 0.0);
	vector<Real> velAverageY(nMax, 0.0);
	vector<Real> velAverageZ(nMax, 0.0);
	vector<Real> phiAverage(nMax, 0.0);
	vector<Real> dragAverage(nMax, 0.0);

	//Loop over the particles
	for (const auto& b : *Omega::instance().getScene()->bodies) {
		shared_ptr<Sphere> s = YADE_PTR_DYN_CAST<Sphere>(b->shape);
		if (!s)
			continue;
		const Real zPos = b->state->pos[2] - zRef;
		int        Np   = int(math::floor(
                        zPos
                        / deltaZ)); //Define the layer number with 0 corresponding to zRef. Let the z position wrt to zero, that way all z altitude are positive.
		// Relative fluid/particle velocity using also the associated fluid vel. fluct.
		if ((Np >= 0) && (Np < nCell)) {
			uRel = Vector3r(vxFluid[Np + 1] + vFluctX[b->id], vFluctY[b->id], vFluctZ[b->id]) - b->state->vel;
			// Drag force with a Dallavalle formulation (drag coef.) and Richardson-Zaki Correction (hindrance effect)
			fDrag = 0.5 * Mathr::PI * pow(s->radius, 2.0) * densFluid * (0.44 * uRel.norm() + 24.4 * viscoDyn / (densFluid * 2.0 * s->radius))
			        * pow((1 - phiPart[Np]), -expoRZ) * uRel;
		} else
			fDrag = Vector3r::Zero();
		if ((Np >= 0) && (Np < nCell)) {
			volPart = 4. / 3. * Mathr::PI * pow(s->radius, 3);
			phiAverage[Np] += 1.;
			velAverageX[Np] += b->state->vel[0];
			velAverageY[Np] += b->state->vel[1];
			velAverageZ[Np] += b->state->vel[2];
			dragAverage[Np] += fDrag[0];
		}
	}
	//Normalized the weighted velocity by the volume of particles contained inside the cell
	for (int n = 0; n < nMax; n++) {
		if (phiAverage[n] != 0) {
			velAverageX[n] /= phiAverage[n];
			velAverageY[n] /= phiAverage[n];
			velAverageZ[n] /= phiAverage[n];
			dragAverage[n] /= phiAverage[n];
			//Normalize the concentration after
			phiAverage[n] *= (volPart.value() / vCell);
		} else {
			velAverageX[n] = 0.0;
			velAverageY[n] = 0.0;
			velAverageZ[n] = 0.0;
			dragAverage[n] = 0.0;
		}
	}
	//Assign the results to the global/public variables of HydroForceEngine
	phiPart     = phiAverage;
	vxPart      = velAverageX;
	vyPart      = velAverageY;
	vzPart      = velAverageZ;
	averageDrag = dragAverage;
}


/* Velocity fluctuation determination.  To execute at a given (changing) period corresponding to the eddy turn over time*/
/* Should be initialized before running HydroForceEngine */
void HydroForceEngine::turbulentFluctuation()
{
	/* check size */
	size_t size = vFluctX.size();
	if (size < scene->bodies->size()) {
		size = scene->bodies->size();
		vFluctX.resize(size);
		vFluctY.resize(size);
		vFluctZ.resize(size);
	}
	/* reset stored values to zero */
#if (YADE_REAL_BIT <= 64)
	memset(&vFluctX[0], 0, size);
	memset(&vFluctY[0], 0, size);
	memset(&vFluctZ[0], 0, size);
#else
	// the standard way, perfectly optimized by compiler.
	std::fill(vFluctX.begin(), vFluctX.end(), 0);
	std::fill(vFluctY.begin(), vFluctY.end(), 0);
	std::fill(vFluctZ.begin(), vFluctZ.end(), 0);
#endif

	/* Create a random number generator rnd() with a gaussian distribution of mean 0 and stdev 1.0 */
	/* see http://www.boost.org/doc/libs/1_55_0/doc/html/boost_random/reference.html and the chapter 7 of Numerical Recipes in C, second edition (1992) for more details */
	static boost::minstd_rand0                                                              randGen((int)TimingInfo::getNow(true));
	static boost::normal_distribution<Real>                                                 dist(0.0, 1.0);
	static boost::variate_generator<boost::minstd_rand0&, boost::normal_distribution<Real>> rnd(randGen, dist);

	Real rand1 = 0.0;
	Real rand2 = 0.0;
	Real rand3 = 0.0;
	/* Attribute a fluid velocity fluctuation to each body above the bed elevation */
	FOREACH(Body::id_t id, ids)
	{
		Body* b = Body::byId(id, scene).get();
		if (!b)
			continue;
		if (!(scene->bodies->exists(id)))
			continue;
		const Sphere* sphere = dynamic_cast<Sphere*>(b->shape.get());
		if (sphere) {
			Vector3r posSphere = b->state->pos;                                    //position vector of the sphere
			int      p         = int(math::floor((posSphere[2] - zRef) / deltaZ)); //cell number in which the particle is
			// If the particle is inside the water and above the bed elevation, so inside the turbulent flow, evaluate a turbulent fluid velocity fluctuation which will be used to apply the drag.
			// The fluctuation magnitude is linked to the value of the Reynolds stress tensor at the given position, a kind of local friction velocity ustar
			// The fluctuations along wall-normal and streamwise directions are correlated in order to be consistent with the formulation of the Reynolds stress tensor and to recover the result
			// that the magnitude of the fluctuation along streamwise = 2*along wall normal
			if ((p < nCell)
			    && (posSphere[2] - zRef
			        > bedElevation)) { // Remove the particles outside of the flow and inside the granular bed, they are not submitted to turbulent fluctuations.
				Real uStar2 = ReynoldStresses[p] / densFluid;
				if (uStar2 > 0.0) {
					Real uStar = sqrt(uStar2);
					rand1      = rnd();
					rand2      = rnd();
					rand3      = -rand1
					        + rnd(); // x and z fluctuation are correlated as measured by Nezu 1977 and as expected from the formulation of the Reynolds stress tensor.
					vFluctZ[id] = rand1 * uStar;
					vFluctY[id] = rand2 * uStar;
					vFluctX[id] = rand3 * uStar;
				}
			} else {
				vFluctZ[id] = 0.0;
				vFluctY[id] = 0.0;
				vFluctX[id] = 0.0;
			}
		}
	}
}

/* Alternative Velocity fluctuation model, same as turbulentFluctuation model but with a time step associated with the fluctuation generation depending on z */
/* Should be executed in the python script at a period dtFluct corresponding to the smallest value of the fluctTime vector */
/* Should be initialized before running HydroForceEngine */
void HydroForceEngine::turbulentFluctuationBIS()
{
	int  idPartMax = vFluctX.size();
	Real rand1     = 0.0;
	Real rand2     = 0.0;
	Real rand3     = 0.0;
	/* Create a random number generator rnd() with a gaussian distribution of mean 0 and stdev 1.0 */
	/* see http://www.boost.org/doc/libs/1_55_0/doc/html/boost_random/reference.html and the chapter 7 of Numerical Recipes in C, second edition (1992) for more details */
	static boost::minstd_rand0                                                              randGen((int)TimingInfo::getNow(true));
	static boost::normal_distribution<Real>                                                 dist(0.0, 1.0);
	static boost::variate_generator<boost::minstd_rand0&, boost::normal_distribution<Real>> rnd(randGen, dist);

	//Loop on the particles
	for (int idPart = 0; idPart < idPartMax; idPart++) {
		//Remove the time ran since last application of the function (dtFluct define in global)
		fluctTime[idPart] -= dtFluct;
		//If negative, means that the time of application of the fluctuation is over, generate a new one with a new associated time
		if (fluctTime[idPart] <= 0) {
			fluctTime[idPart] = 10 * dtFluct; //Initialisation of the application time
			Body* b           = Body::byId(idPart, scene).get();
			if (!b)
				continue;
			if (!(scene->bodies->exists(idPart)))
				continue;
			const Sphere* sphere = dynamic_cast<Sphere*>(b->shape.get());
			Real          uStar  = 0.0;
			if (sphere) {
				Vector3r posSphere = b->state->pos;                                    //position vector of the sphere
				int      p         = int(math::floor((posSphere[2] - zRef) / deltaZ)); //cell number in which the particle is
				if (ReynoldStresses[p] > 0.0)
					uStar = sqrt(ReynoldStresses[p] / densFluid);
				// Remove the particles outside of the flow and inside the granular bed, they are not submitted to turbulent fluctuations.
				if ((p < nCell) && (posSphere[2] - zRef > bedElevation)) {
					rand1 = rnd();
					rand2 = rnd();
					rand3 = -rand1
					        + rnd(); // x and z fluctuation are correlated as measured by Nezu 1977 and as expected from the formulation of the Reynolds stress tensor.
					vFluctZ[idPart] = rand1 * uStar;
					vFluctY[idPart] = rand2 * uStar;
					vFluctX[idPart] = rand3 * uStar;
					// Limit the value to avoid the application of fluctuations in the viscous sublayer
					const Real zPos = max(b->state->pos[2] - zRef - bedElevation, 11.6 * viscoDyn / densFluid / uStar);
					// Time of application of the fluctuation as a function of depth from kepsilon model
					if (uStar > 0.0)
						fluctTime[idPart] = min(0.33 * 0.41 * zPos / uStar, 10.);
				} else {
					vFluctZ[idPart]   = 0.0;
					vFluctY[idPart]   = 0.0;
					vFluctX[idPart]   = 0.0;
					fluctTime[idPart] = 0.0;
				}
			}
		}
	}
}

/* Velocity fluctuation determination.  To execute at a given period*/
/* Should be initialized before running HydroForceEngine */
void HydroForceEngine::turbulentFluctuationFluidizedBed()
{
	/* check size */
	size_t size = vFluctX.size();
	if (size < scene->bodies->size()) {
		size = scene->bodies->size();
		vFluctX.resize(size);
		vFluctY.resize(size);
		vFluctZ.resize(size);
	}
	/* reset stored values to zero */
#if (YADE_REAL_BIT <= 64)
	memset(&vFluctX[0], 0, size);
	memset(&vFluctY[0], 0, size);
	memset(&vFluctZ[0], 0, size);
#else
	// the standard way, perfectly optimized by compiler.
	std::fill(vFluctX.begin(), vFluctX.end(), 0);
	std::fill(vFluctY.begin(), vFluctY.end(), 0);
	std::fill(vFluctZ.begin(), vFluctZ.end(), 0);
#endif

	/* Create a random number generator rnd() with a gaussian distribution of mean 0 and stdev 1.0 */
	/* see http://www.boost.org/doc/libs/1_55_0/doc/html/boost_random/reference.html and the chapter 7 of Numerical Recipes in C, second edition (1992) for more details */
	static boost::minstd_rand0                                                              randGen((int)TimingInfo::getNow(true));
	static boost::normal_distribution<Real>                                                 dist(0.0, 1.0);
	static boost::variate_generator<boost::minstd_rand0&, boost::normal_distribution<Real>> rnd(randGen, dist);

	Real rand1 = 0.0;
	Real rand2 = 0.0;
	Real rand3 = 0.0;
	/* Attribute a fluid velocity fluctuation to each body above the bed elevation */
	FOREACH(Body::id_t id, ids)
	{
		Body* b = Body::byId(id, scene).get();
		if (!b)
			continue;
		if (!(scene->bodies->exists(id)))
			continue;
		const Sphere* sphere = dynamic_cast<Sphere*>(b->shape.get());
		if (sphere) {
			Vector3r posSphere = b->state->pos;                                    //position vector of the sphere
			int      p         = int(math::floor((posSphere[2] - zRef) / deltaZ)); //cell number in which the particle is
			// If the particle is inside the water and above the bed elevation, so inside the turbulent flow, evaluate a turbulent fluid velocity fluctuation which will be used to apply the drag.
			// The fluctuation magnitude is linked to the value of the Reynolds stress tensor at the given position, a kind of local friction velocity ustar
			if ((p < nCell) && (posSphere[2] - zRef > 0.)) { // Remove the particles outside of the flow
				Real uStar2 = ReynoldStresses[p] / densFluid;
				if (uStar2 > 0.0) {
					Real uStar  = sqrt(uStar2);
					rand1       = rnd();
					rand2       = rnd();
					rand3       = rnd();
					vFluctZ[id] = rand1 * uStar;
					vFluctY[id] = rand2 * uStar;
					vFluctX[id] = rand3 * uStar;
				}
			} else {
				vFluctZ[id] = 0.0;
				vFluctY[id] = 0.0;
				vFluctX[id] = 0.0;
			}
		}
	}
}


///////////////////////
/* Fluid Resolution */
///////////////////////
// Fluid resolution routine: 1D vertical volume-averaged fluid momentum balance resolution
void HydroForceEngine::fluidResolution(Real tfin, Real dt)
{
	//Variables declaration
	int  i, j, maxiter, q, ii;
	Real phi_nodej, termeVisco_j, termeVisco_jp1, termeTurb_j, termeTurb_jp1, viscof, dz, sum, phi_lim, dudz, ustar, fluidHeight, urel, urel_bound, Re, eps,
	        ff, ffold, delta, dddiv, ejm1, phijm1, upjm1, ejp1, phijp1, upjp1, secondMemberPart;
	vector<Real> sig(nCell, 0.0), dsig(nCell - 1, 0.), viscoeff(nCell, 0.), ufn(nCell + 1, 0.), wallFriction(nCell - 1, 0.), viscoft(nCell, 0.),
	        ufnp(nCell + 1, 0.), a(nCell + 1, 0.), b(nCell + 1, 0.), c(nCell + 1, 0.), s(nCell + 1, 0.), lm(nCell, 0.), ddem(nCell + 1, 0.),
	        ddfm(nCell + 1, 0.), deltaz(nCell, 0.), epsilon_node(nCell, 0.), epsilon(nCell, 0.);

	//Initialisation
	Real time = 0;
	ufn       = vxFluid;              //Assign the global variable vxFluid to ufn, i.e. the last fluid velocity profile evaluated
	viscof    = viscoDyn / densFluid; // compute the kinematic viscosity
	dz        = deltaZ;
	Real imp  = 0.5; // Implicitation factor of the lateral sink term due to wall friction
	for (j = 0; j < nCell; j++) {
		epsilon[j] = 1. - phiPart[j]; // compute fluid phase volume fraction or porosity
	}

	// Mesh definition: regular of step dz
	sig[0]  = 0.;
	dsig[0] = dz;
	for (j = 1; j < nCell - 1; j++) {
		sig[j]  = sig[j - 1] + dsig[j - 1];
		dsig[j] = dz;
	}
	sig[nCell - 1] = sig[nCell - 2] + dsig[nCell - 2];

	// Usefull quantities for the staggered grid, i.e. quantities defined at mesh nodes or in between (velocity nodes)
	for (j = 0; j < nCell; j++) {
		if ((j != 0) && (j != nCell - 1)) {
			deltaz[j]       = 0.5 * (dsig[j - 1] + dsig[j]); //Space between two velocity nodes, j-1/2 and j+1/2
			epsilon_node[j] = 0.5
			        * (epsilon[j - 1] + epsilon[j]); //Fluid volume fraction extrapolated at the regular mesh node j (evaluated at velocity nodes)
		} else if (j == 0) {
			deltaz[j]       = 0.5 * dsig[j]; //Space between velocity node 0 (boundary) and velocity node 1/2
			epsilon_node[j] = epsilon[j];    //Fluid volume fraction at the bottom, taken to be equal to the one at the first step (choice)
		} else if (j == nCell - 1) {
			deltaz[j]       = 0.5 * dsig[j - 1]; //Space between velocity node ndimz-3/2 and velocity node ndimz-1 (boundary)
			epsilon_node[j] = epsilon[j - 1];    //Fluid volume fraction at the top, taken to be equal to the one at the last step (choice)
		}
	}

	// compute the fluid height
	fluidHeight = sig[nCell - 1];

	sig_cpp.resize(nCell);
	dsig_cpp.resize(nCell);
	sig_cpp  = sig;
	dsig_cpp = dsig;

	////////////////////////////////////  //  computeTaufsi(dt);
	// Compute the fluid-particle momentum transfer associated to drag force, taufsi = phi/Vp*<fd>/rhof/(uf - up), not changing during the fluid resolution

	//Initialization
	taufsi.resize(nCell);
#if (YADE_REAL_BIT <= 64)
	memset(&taufsi[0], 0, nCell); //Resize and initialize taufsi
#else
	// the standard way, perfectly optimized by compiler.
	std::fill(taufsi.begin(), taufsi.end(), 0);
#endif
	Real lim = 1e-5, dragTerm = 0., partVolume = 1., partVolume1 = 1., partVolume2 = 1.;
	// Evaluate particles volume
	if (twoSize == true) {
		partVolume1 = 4. / 3. * Mathr::PI * pow(radiusPart1, 3);
		partVolume2 = 4. / 3. * Mathr::PI * pow(radiusPart2, 3);
	} else
		partVolume = 4. / 3. * Mathr::PI * pow(radiusPart, 3);
	// Compute taufsi
	taufsi[0] = 0.;
	for (i = 1; i < nCell; i++) {
		if (twoSize == true) {
			dragTerm = phiPart1[i] / partVolume1 * averageDrag1[i] + phiPart2[i] / partVolume2 * averageDrag2[i];
		} else {
			dragTerm = phiPart[i] / partVolume * averageDrag[i];
		}
		urel = math::abs(
		        ufn[i + 1]
		        - vxPart[i]); // Difference of definition between ufn and vxPart, ufn starts at 0, while vxPart starts at 1/2. The two therefore corresponds for i+1 and i
		urel_bound = math::max(urel, lim); //limit the value to avoid division by 0
		taufsi[i]  = math::max(
                        0.,
                        math::min(
                                dragTerm / urel_bound / densFluid,
                                pow(10 * dt,
                                    -1.))); //limit the max value of taufsi to the fluid resolution limit, i.e. 1/(10dt) and required positive (charact. time)
	}

	////////////////////////////////////
	//  Compute the effective viscosity (due to the presence of particles)
	// 0 : Pure fluid viscosity
	if (irheolf == 0) {
		for (j = 0; j < nCell; j++)
			viscoeff[j] = viscof;
	}
	// 1 : Einstein's effective viscosity
	else if (irheolf == 1) {
		viscoeff[0] = viscof * (1. + 2.5 * phiPart[0]);
		for (j = 1; j < nCell; j++) {
			phi_nodej   = 0.5 * (phiPart[j - 1] + phiPart[j]); // solid volume fraction at (scalar) node j.
			viscoeff[j] = viscof * (1. + 2.5 * phi_nodej);
		}
	}
	//2: fluid volume fraction power-law effective viscosity
	else if (irheolf == 2) {
		viscoeff[0] = viscof * (1. + 2.5 * phiPart[0]);
		for (j = 1; j < nCell; j++) {
			phi_nodej   = 0.5 * (phiPart[j - 1] + phiPart[j]); // solid volume fraction at (scalar) node j.
			viscoeff[j] = viscof * pow(1 - phi_nodej, -2.8);
		}
	}

	///////////////////////////////////////////////
	// FLUID VELOCITY PROFILE RESOLUTION: LOOP OVER TIME (main loop)
	while (time <= tfin) {
		// Advance time
		time = time + dt;

		////////////////////////////////////
		// Compute the eddy viscosity depth profile, viscoft
		// Eddy viscosity
		// 0 : No turbulence
		if (iturbu == 0) {
			for (j = 0; j < nCell; j++)
				viscoft[j] = 0.;
		}
		// iturbu = 1 : Turbulence activated
		else if (iturbu == 1) {
			// ilm = 0 : Prandtl mixing length
			if (ilm == 0) {
				lm[0] = 0.;
				for (j = 1; j < nCell; j++)
					lm[j] = kappa * sig[j];
			}
			// ilm = 1 : Parabolic profile (free surface flows)
			else if (ilm == 1) {
				lm[0] = 0.;
				for (j = 1; j < nCell; j++)
					lm[j] = kappa * sig[j] * sqrt(1. - sig[j] / fluidHeight);
				lm[nCell - 1] = 0.;
			}
			// ilm = 2 : Li and Sawamoto (1995) integral of concentration profile
			else if (ilm == 2) {
				sum   = 0.;
				lm[0] = 0.;
				for (j = 1; j < nCell; j++) {
					phi_lim = math::min(phiPart[j - 1], phiMax);
					sum += kappa * (phiMax - phi_lim) / phiMax * dsig[j - 1];
					lm[j] = sum;
				}
			}
			// end if ilm
			// Compute the velocity gradient and the mixing length
			for (j = 0; j < nCell; j++) {
				dudz       = (ufn[j + 1] - ufn[j]) / deltaz[j];
				viscoft[j] = pow(lm[j], 2) * fabs(dudz);
			}
			// test on y+ for viscous sublayer for log profile validation
			ustar = sqrt(fabs(gravity[0]) * sig[nCell - 1]);
			if (viscousSubLayer == 1) {
				for (j = 1; j < nCell; j++)
					if (sig[j] * ustar / viscof < 11.3)
						viscoft[j] = 0.;
			}
		}

		////////////////////////////////////      end if iturbu


		//////////////////////////////////
		// Compute the lateral wall friction profile, if activated
		if (fluidWallFriction == true) {
			maxiter = 100;  //Maximum number iteration for the resolution
			eps     = 1e-2; //Tolerance for the equation resolution
			for (j = 0; j < nCell - 1; j++) {
				Re    = max(1e-10, fabs(ufn[j + 1]) * channelWidth / viscof);
				ffold = pow(0.32, -2); //Initial guess of the friction factor
				delta = 1e10;          //Initialize at a random value greater than eps
				q     = 0;
				while ((delta >= eps)
				       && (q < maxiter)) { //Loop while the required precision is reached or the  maximum iteration number is overpassed
					q += 1;
					//Graf and Altinakar 1993 formulation of the friction factor
					ff    = pow(2. * log(Re * sqrt(ffold)) + 0.32, -2);
					delta = fabs(ff - ffold) / ffold;
					ffold = ff;
				}
				if (q == maxiter)
					ff = 0.;
				wallFriction[j] = fluidFrictionCoef * ff;
			}
		}
		////////////////////////////////// end wall friction


		//////////////////////////////////
		// Compute the system of equation in matricial form (Compute a,b,c,s)

		// Bottom boundary condition: (always no-slip)
		a[0] = 0.;
		b[0] = 1.;
		c[0] = 0.;
		s[0] = 0.;

		// Top boundary condition: (0: no-slip / 1: zero gradient)
		if (iusl == 0) {
			a[nCell] = 0.;
			b[nCell] = 1.;
			c[nCell] = 0.;
			s[nCell] = uTop;
		} else if (iusl == 1) {
			a[nCell] = -1.;
			b[nCell] = 1.;
			c[nCell] = 0.;
			s[nCell] = 0.;
		}


		//Loop over the spatial mesh to determine the matricial coefficient, (a,b,c,s), from j+1=1 to j+1=nCell-1 (values 0 and nCell correspond to BC)
		for (j = 0; j < nCell - 1; j++) {
			//Interesting quantities to compute
			termeVisco_j   = dt * epsilon[j] / dsig[j] * viscoeff[j] / deltaz[j];
			termeVisco_jp1 = dt * epsilon[j] / dsig[j] * viscoeff[j + 1] / deltaz[j + 1];
			termeTurb_j    = dt / dsig[j] * epsilon_node[j] * viscoft[j] / deltaz[j];
			termeTurb_jp1  = dt / dsig[j] * epsilon_node[j + 1] * viscoft[j + 1] / deltaz[j + 1];

			if (j == 0) {
				ejm1   = epsilon[j];
				phijm1 = phiPart[j];
				upjm1  = vxPart[j];
			} else {
				ejm1   = epsilon[j - 1];
				phijm1 = phiPart[j - 1];
				upjm1  = vxPart[j - 1];
			}
			if (j == nCell - 1) {
				ejp1   = epsilon[j];
				phijp1 = phiPart[j];
				upjp1  = vxPart[j];
			} else {
				ejp1   = epsilon[j + 1];
				phijp1 = phiPart[j + 1];
				upjp1  = vxPart[j + 1];
			}

			secondMemberPart = dt * epsilon[j] / dsig[j]
			        * (viscoeff[j + 1] / deltaz[j + 1] * (phijp1 * upjp1 - phiPart[j] * vxPart[j])
			           - viscoeff[j] / deltaz[j] * (phiPart[j] * vxPart[j] - phijm1 * upjm1));

			// LHS: algebraic system coefficients
			a[j + 1] = -termeVisco_j * ejm1 - termeTurb_j; //eq. 24 of the manual Maurin 2018
			b[j + 1] = termeVisco_jp1 * epsilon[j] + termeVisco_j * epsilon[j] + termeTurb_jp1 + termeTurb_j + epsilon[j] + dt * taufsi[j]
			        + imp * dt * epsilon[j] * 2. / channelWidth * 0.125 * wallFriction[j]
			                * pow(ufn[j + 1], 2);              //eq. 25 of the manual Maurin 2018 + fluid wall correction
			c[j + 1] = -termeVisco_jp1 * ejp1 - termeTurb_jp1; //eq. 26 of the manual Maurin 2018

			// RHS: unsteady, gravity, drag, pressure gradient, lateral wall friction
			s[j + 1] = ufn[j + 1] * epsilon[j] + epsilon[j] * dt * math::abs(gravity[0]) + dt * taufsi[j] * vxPart[j] + secondMemberPart
			        - (1. - imp) * dt * epsilon[j] * 2. / channelWidth * 0.125 * wallFriction[j] * pow(ufn[j + 1], 2)
			        - epsilon[j] * dpdx / densFluid * dt; //eq. 27 of the manual Maurin 2018 + fluid wall correction and pressure gradient forcing.
		}
		//////////////////////////////////


		////////////////////////////////////
		// Solve the matricial tridiagonal system using Real-sweep algorithm

		// downward sweep
		dddiv   = b[0];
		ddem[0] = -c[0] / dddiv;
		ddfm[0] = s[0] / dddiv;
		for (i = 1; i <= nCell - 1; i++) {
			dddiv   = b[i] + a[i] * ddem[i - 1];
			ddem[i] = -c[i] / dddiv;
			ddfm[i] = (s[i] - a[i] * ddfm[i - 1]) / dddiv;
		}
		// upward sweep
		dddiv       = b[nCell] + a[nCell] * ddem[nCell - 1];
		ddfm[nCell] = (s[nCell] - a[nCell] * ddfm[nCell - 1]) / dddiv;
		ufnp[nCell] = ddfm[nCell];
		for (ii = 1; ii < nCell + 1; ii++) {
			i       = nCell - ii;
			ufnp[i] = ddem[i] * ufnp[i + 1] + ddfm[i];
		}
		//////////////////////////////////

		// Update solution for next time step
		for (j = 0; j < nCell + 1; j++)
			ufn[j] = ufnp[j];
	}
	///////////////////////////// END OF THE LOOP ON THE TIME


	//Update Fluid velocity, turbulent viscosity and Reynolds stresses
	ReynoldStresses.resize(nCell);
	turbulentViscosity.resize(nCell);
	vxFluid.resize(nCell + 1);
#if (YADE_REAL_BIT <= 64)
	memset(&ReynoldStresses[0], 0, nCell);
	memset(&turbulentViscosity[0], 0, nCell);
	memset(&vxFluid[0], 0, nCell + 1);
#else
	// the standard way, perfectly optimized by compiler.
	std::fill(ReynoldStresses.begin(), ReynoldStresses.end(), 0);
	std::fill(turbulentViscosity.begin(), turbulentViscosity.end(), 0);
	std::fill(vxFluid.begin(), vxFluid.end(), 0);
#endif

	for (j = 0; j < nCell; j++) {
		vxFluid[j]            = ufn[j]; // Update fluid velocity
		turbulentViscosity[j] = viscoft[j];
		ReynoldStresses[j]    = densFluid * epsilon_node[j] * viscoft[j] * (ufn[j + 1] - ufn[j]) / deltaz[j];
	}
	vxFluid[nCell] = ufn[nCell];
}
///////////////////////////////////////////////////////////////////////// END OF HydroForceEngine::fluidResolution

} // namespace yade
