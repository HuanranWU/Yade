/*************************************************************************
*  Copyright (C) 2019 by Robert Caulk <rob.caulk@gmail.com>              *
*  Copyright (C) 2019 by Bruno Chareyre <bruno.chareyre@hmg.inpg.fr>     *
*                                                                        *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/
// Experimental engine under development
#ifdef PARTIALSAT
#include "PartialSatClayEngine.hpp"
#include <lib/high-precision/Constants.hpp>
#include <pkg/common/Sphere.hpp>
#include <pkg/dem/CohesiveFrictionalContactLaw.hpp>
#include <pkg/dem/HertzMindlin.hpp>
#include <boost/range/algorithm_ext/erase.hpp>

namespace yade { // Cannot have #include directive inside.
CREATE_LOGGER(PartialSatClayEngine);
YADE_PLUGIN((PartialSatClayEngine));

PartialSatClayEngine::~PartialSatClayEngine() { }

// clang-format off
void PartialSatClayEngine::action()
{
	if (debug) cout << "Entering partialSatEngineAction "<<endl;
	if (partialSatDT != 0) timeStepControl();

	if (!isActivated) return;
	timingDeltas->start();
	if (desiredPorosity != 0) {
		Real actualPorosity = Shop::getPorosityAlt();
		volumeCorrection    = desiredPorosity / actualPorosity;
	}
	if (debug) cout << "about to set positions" << endl;
	setPositionsBuffer(true);
	if (!first and alphaBound >= 0)
		addAlphaToPositionsBuffer(true);
	timingDeltas->checkpoint("Position buffer");
	if (first) {
		if (debug) cout << "about to build triangulation" << endl;
		if (multithread)
			setPositionsBuffer(false);
		buildTriangulation(pZero, *solver);
		//if (debug) cout <<"about to add alphatopositionsbuffer" << endl;
		if (alphaBound >= 0) addAlphaToPositionsBuffer(true);
		if (debug) cout << "about to initializevolumes" << endl;
		initializeVolumes(*solver);
		backgroundSolver    = solver;
		backgroundCompleted = true;
		if (partialSatEngine) {
			cout << "setting initial porosity" << endl;
			if (imageryFilePath.compare("none") == 0)
				setInitialPorosity(*solver);
			else {
				cout << "using imagery to set porosity" << endl;
				setPorosityWithImageryGrid(imageryFilePath, *solver);
			}
			//if ( crackCellPoroThreshold>0 ) crackCellsAbovePoroThreshold(*solver); // set initial crack network using porosity threshold from imagery
			if (blockCellPoroThreshold > 0) blockCellsAbovePoroThreshold(*solver);
			if (mineralPoro > 0) {
				cout << "about to block low poros" << endl;
				blockLowPoroRegions(*solver);
			}
			cout << "initializing saturations" << endl;
			initializeSaturations(*solver);
			solver->computePermeability(); //computing permeability again since they depend on the saturations. From here on, we only compute perm once per triangulation
			if (particleSwelling && volumes) setOriginalParticleValues();
			if (useKeq) computeEquivalentBulkModuli(*solver);
			if (debug) cout << "Particle swelling model active, original particle volumes set" << endl;
		}
	}
#ifdef YADE_OPENMP
	solver->ompThreads = ompThreads > 0 ? ompThreads : omp_get_max_threads();
#endif
	timingDeltas->checkpoint("Triangulating");
	updateVolumes(*solver);
	if (!first && partialSatEngine) {
		if (resetOriginalParticleValues) {
			setOriginalParticleValues();
			resetOriginalParticleValues = false;
		}
		if (forceConfinement) simulateConfinement();
		//initializeSaturations(*solver);
		if (!resetVolumeSolids) updatePorosity(*solver);
		else resetPoresVolumeSolids(*solver);

		// if (resetPorosity) { // incase we want to reach a certain state and then set porosity again...chicken and egg problem
		//         resetVolumeSolids=true;
		//         if (imageryFilePath.compare("none")==0) setInitialPorosity(*solver);
		//         else {
		//                 cout << "using imagery to set porosity" << endl;
		//                 setPorosityWithImageryGrid(imageryFilePath, *solver);
		//         }
		// }
		if (useKeq) computeEquivalentBulkModuli(*solver);
	}
	timingDeltas->checkpoint("Update_Volumes");

	epsVolCumulative += epsVolMax;
	if (partialSatDT == 0) retriangulationLastIter++;
	if (!updateTriangulation) updateTriangulation = // If not already set true by another function of by the user, check conditions
		        (defTolerance > 0 && epsVolCumulative > defTolerance) || (meshUpdateInterval > 0 && retriangulationLastIter > meshUpdateInterval);

	// remesh everytime a bond break occurs (for DFNFlow-JCFPM coupling)
	if (breakControlledRemesh) remeshForFreshlyBrokenBonds();

	///compute flow and and forces here
	if (pressureForce) {
#ifdef LINSOLV
		permUpdateIters++;
		if (fixTriUpdatePermInt > 0 && permUpdateIters >= fixTriUpdatePermInt) updateLinearSystem(*solver);
#endif
		if (partialSatEngine) setCellsDSDP(*solver);
		solver->gaussSeidel(partialSatDT == 0 ? scene->dt : solverDT);
		timingDeltas->checkpoint("Factorize + Solve");
		if (partialSatEngine) {
			//initializeSaturations(*solver);
			if (!freezeSaturation) updateSaturation(*solver);
			if (debug) cout << "finished initializing saturations" << endl;
		}
		if (!decoupleForces) solver->computeFacetForcesWithCache();
		if (debug) cout << "finished computing facet forces" << endl;
	}
	if (particleSwelling and (retriangulationLastIter == 1 or partialSatDT != 0)) {
		if (suction) {
			if (fracBasedPointSuctionCalc) computeVertexSphericalArea();
			collectParticleSuction(*solver);
		}
		if (swelling) swellParticles();
	}
	if (debug) cout << "finished collecting suction and swelling " << endl;
	//if (freeSwelling && crackModelActive) determineFracturePaths();
	if (brokenBondsRemoveCapillaryforces) removeForcesOnBrokenBonds();
	timingDeltas->checkpoint("compute_Forces");
	///Application of vicscous forces
	scene->forces.sync();
	timingDeltas->checkpoint("forces.sync()");
	//if (!decoupleForces) computeViscousForces ( *solver );
	timingDeltas->checkpoint("viscous forces");
	if (!decoupleForces) {
		if (partialSatDT != 0) addPermanentForces(*solver);
		else applyForces(*solver);
	}
	timingDeltas->checkpoint("Applying Forces");
	if (debug) cout << "finished computing forces and applying" << endl;
///End compute flow and forces
#ifdef LINSOLV
	int sleeping = 0;
	if (multithread && !first) {
		while (updateTriangulation && !backgroundCompleted) { /*cout<<"sleeping..."<<sleeping++<<endl;*/
			sleeping++;
			boost::this_thread::sleep(boost::posix_time::microseconds(1000));
		}
		if (debug && sleeping)
			cerr << "sleeping..." << sleeping << endl;
		if (updateTriangulation || ((meshUpdateInterval > 0 && ellapsedIter > (0.5 * meshUpdateInterval)) && backgroundCompleted)) {
			if (debug) cerr << "switch flow solver" << endl;
			if (useSolver == 0)
				LOG_ERROR("background calculations not available for Gauss-Seidel");
			if (!fluxChanged) {
				if (fluidBulkModulus > 0 || doInterpolate || partialSatEngine)
					solver->interpolate(solver->T[solver->currentTes], backgroundSolver->T[backgroundSolver->currentTes]);


				//Copy imposed pressures/flow from the old solver
				backgroundSolver->imposedP = vector<pair<CGT::Point, Real>>(solver->imposedP);
				backgroundSolver->imposedF = vector<pair<CGT::Point, Real>>(solver->imposedF);
				solver                     = backgroundSolver;
			} else {
				fluxChanged = false;
			}

			backgroundSolver = shared_ptr<FlowSolver>(new FlowSolver);
			if (metisForced) {
				backgroundSolver->eSolver.cholmod().nmethods           = 1;
				backgroundSolver->eSolver.cholmod().method[0].ordering = CHOLMOD_METIS;
			}
			backgroundSolver->imposedP = vector<pair<CGT::Point, Real>>(solver->imposedP);
			backgroundSolver->imposedF = vector<pair<CGT::Point, Real>>(solver->imposedF);
			if (debug)
				cerr << "switched" << endl;
			setPositionsBuffer(false); //set "parallel" buffer for background calculation
			backgroundCompleted     = false;
			retriangulationLastIter = ellapsedIter;
			updateTriangulation     = false;
			epsVolCumulative        = 0;
			ellapsedIter            = 0;
			boost::thread workerThread(&PartialSatClayEngine::backgroundAction, this);
			workerThread.detach();
			if (debug) cerr << "backgrounded" << endl;
			initializeVolumes(*solver);
			//computeViscousForces(*solver);
			if (debug) cerr << "volumes initialized" << endl;
		} else {
			if (debug && !backgroundCompleted) cerr << "still computing solver in the background, ellapsedIter=" << ellapsedIter << endl;
			ellapsedIter++;
		}
	} else
#endif
	{
		if (updateTriangulation && !first) {
			if (debug) cout << "building tri" <<endl;
			buildTriangulation(pZero, *solver);
			if (debug) cout << "adding alpha to posbuf" <<endl;
			if (alphaBound >= 0) addAlphaToPositionsBuffer(true);
			if (debug) cout << "initializing volumes" <<endl;
			initializeVolumes(*solver);
			if (debug) cout << "out of init volumes" <<endl;
			//resetVolumeSolids=true; //sets new vSolid and (invVoidVolume factored by porosity)
			//computeViscousForces(*solver);
			updateTriangulation     = false;
			epsVolCumulative        = 0;
			retriangulationLastIter = 0;
			ReTrg++;
		}
	}
	first = false;
	timingDeltas->checkpoint("triangulate + init volumes");
}


/////// Partial Sat Tools /////////
void PartialSatClayEngine::timeStepControl()
{
	if (((elapsedIters > int(partialSatDT / scene->dt)) and partialSatDT != 0) or first) {
		isActivated = true;
		retriangulationLastIter += elapsedIters;
		elapsedIters = 0;
		if (first) {
			collectedDT = scene->dt;
			solverDT    = scene->dt;

		} else {
			solverDT    = collectedDT;
			collectedDT = 0;
		}
		if (debug)
			cout << "using flowtime step =" << solverDT << endl;
	} else {
		if (partialSatDT != 0) {
			elapsedIters++;
			collectedDT += scene->dt;
			isActivated = true;
		}
		isActivated = emulatingAction ? true : false;
		solverDT    = scene->dt;
	}
}

void PartialSatClayEngine::addPermanentForces(FlowSolver& flow)
{
	typedef typename Solver::FiniteVerticesIterator FiniteVerticesIterator;

	FiniteVerticesIterator vertices_end = flow.tesselation().Triangulation().finite_vertices_end();

	for (FiniteVerticesIterator V_it = flow.tesselation().Triangulation().finite_vertices_begin(); V_it != vertices_end; V_it++) {
		scene->forces.setPermForce(V_it->info().id(), makeVector3r(V_it->info().forces));
	}
}

void PartialSatClayEngine::blockLowPoroRegions(FlowSolver& flow)
{
	int          numClumps = 0;
	Tesselation& Tes       = flow.T[flow.currentTes];
	//	#ifdef YADE_OPENMP
	const long size = Tes.cellHandles.size();
	//#pragma omp parallel for
	//cout << "blocking low poro regions" << endl;
	for (long i = 0; i < size; i++) {
		CellHandle& cell = Tes.cellHandles[i];
		if (cell->info().porosity <= mineralPoro) {
			//cout << "found a cell with mineral poro" << endl;
			std::vector<Body::id_t> clumpIds;
			cell->info().blocked = true;
			cell->info().clumped = true;
			//cout << "adding incident particle ids to clump list" << endl;
			addIncidentParticleIdsToClumpList(cell, clumpIds);
			blockMineralCellRecursion(cell, clumpIds); //now cycle on the neighbors
			//cout << "creating clump" << endl;
			if (clumpIds.size() > 0) {
				this->clump(clumpIds, 0);
				numClumps++;
			}
		}
	}
	cout << "clumps created " << numClumps << endl;
}

void PartialSatClayEngine::blockMineralCellRecursion(CellHandle cell, std::vector<Body::id_t>& clumpIds)
{
	for (int facet = 0; facet < 4; facet++) {
		CellHandle nCell = cell->neighbor(facet);
		if (solver->T[solver->currentTes].Triangulation().is_infinite(nCell))
			continue;
		if (nCell->info().Pcondition)
			continue;
		if (nCell->info().clumped)
			continue;
		if (nCell->info().blocked)
			continue;
		if (nCell->info().porosity > mineralPoro)
			continue;

		nCell->info().blocked = true;
		nCell->info().clumped = true;
		//cout << "adding incident particle ids to clump list" << endl;
		addIncidentParticleIdsToClumpList(nCell, clumpIds);
		blockMineralCellRecursion(nCell, clumpIds);
	}
}

void PartialSatClayEngine::addIncidentParticleIdsToClumpList(CellHandle nCell, std::vector<Body::id_t>& clumpIds)
{
	for (int v = 0; v < 4; v++) {
		//if (cell->vertex(v)->info().isFictious) continue;
		const Body::id_t id = Body::id_t(nCell->vertex(v)->info().id());
		if (std::find(clumpIds.begin(), clumpIds.end(), id) != clumpIds.end())
			continue;
		else
			clumpIds.push_back(id);
	}
}

void PartialSatClayEngine::computeEquivalentBulkModuli(FlowSolver& flow)
{
	Tesselation& Tes = flow.T[flow.currentTes];
	//	#ifdef YADE_OPENMP
	const long size = Tes.cellHandles.size();
#pragma omp parallel for
	for (long i = 0; i < size; i++) {
		CellHandle& cell                   = Tes.cellHandles[i];
		Real        waterFrac              = (cell->info().sat() * cell->info().porosity) / Kw;
		Real        airFrac                = cell->info().porosity * (1. - cell->info().sat()) / Ka;
		Real        solidFrac              = (1. - cell->info().porosity) / Ks;
		Real        Keq                    = 1 / (waterFrac + airFrac + solidFrac);
		cell->info().equivalentBulkModulus = Keq;
	}
}

void PartialSatClayEngine::resetPoresVolumeSolids(FlowSolver& flow)
{
	Tesselation& Tes = flow.T[flow.currentTes];
	//	#ifdef YADE_OPENMP
	const long size  = Tes.cellHandles.size();
	crackedCellTotal = 0;
#pragma omp parallel for
	for (long i = 0; i < size; i++) {
		CellHandle& cell     = Tes.cellHandles[i];
		cell->info().vSolids = cell->info().volume() * (1. - cell->info().porosity);
	}
	resetVolumeSolids = false;
}

void PartialSatClayEngine::updatePorosity(FlowSolver& flow)
{
	Tesselation& Tes = flow.T[flow.currentTes];
	//	#ifdef YADE_OPENMP
	const long size  = Tes.cellHandles.size();
	crackedCellTotal = 0;
#pragma omp parallel for
	for (long i = 0; i < size; i++) {
		CellHandle& cell = Tes.cellHandles[i];
		//if (cell->info().isAlpha) continue;
		if (!freezePorosity) {
			if ((!onlyFractureExposedCracks and cell->info().crack) or cell->info().isExposed) {
				crackedCellTotal++; //, cell->info().porosity=fracPorosity;
				                    //crackCellAbovePoroThreshold(cell);
			}                           //maxPoroClamp;
			else {
				Real poro = 1. - cell->info().vSolids / cell->info().volume();
				//cout << "old poro" << cell->info().porosity << "new poro" << poro << endl;
				if (poro < minPoroClamp)
					poro = minPoroClamp;
				if (poro > maxPoroClamp)
					poro = maxPoroClamp;
				if (!freezeSaturation and directlyModifySatFromPoro) { // updatesaturation with respect to volume change
					Real dt         = partialSatDT == 0 ? scene->dt : solverDT;
					Real volWater_o = (cell->info().volume() - cell->info().dv() * dt) * cell->info().porosity * cell->info().saturation;
					cell->info().saturation = volWater_o / (poro * cell->info().volume()); // update the saturation with respect to new porosity and volume
				}
				cell->info().porosity = poro;
			}
		} // if we dont want to modify porosity during genesis, but keep the cell curve params updated between triangulations
		// update the parameters for the unique pcs curve in this cell (keep this for interpolation purposes):
		cell->info().Po = Po * exp(a * (meanInitialPorosity - cell->info().porosity)); // use unique cell initial porosity or overall average porosity (mu)?
		if (cell->info().Po > maxPo) cell->info().Po = maxPo;
		cell->info().lambdao = lmbda * exp(b * (meanInitialPorosity - cell->info().porosity));
		if (cell->info().lambdao < minLambdao) cell->info().lambdao = minLambdao;
	}
}


void PartialSatClayEngine::setPorosityWithImageryGrid(string imageryFilePath, FlowSolver& flow)
{
	ifstream file;
	file.open(imageryFilePath);
	if (!file) {
		cerr << "Unable to open imagery grid reverting to weibull porosity distribution" << endl;
		setInitialPorosity(flow);
		return;
	}
	std::vector<Vector3r> gridCoords;
	std::vector<Real>     porosities;
	int                   l = 0;
	Real                  x, y, z, porosity;
	while (file >> x >> y >> z >> porosity) {
		gridCoords.push_back(Vector3r(x, y, z));
		//gridCoords[l][1] = y;
		//gridCoords[l][2] = z;
		porosities.push_back(porosity);
		l++;
	}
	cout << "finished creating coords vec and porosities" << endl;
	Tesselation& Tes = flow.T[flow.currentTes];
	//	#ifdef YADE_OPENMP
	const long size = Tes.cellHandles.size();
#pragma omp parallel for
	for (long i = 0; i < size; i++) {
		CellHandle& cell      = Tes.cellHandles[i];
		Real        finalDist = 1e10;
		Real        finalPoro = meanInitialPorosity;
		CVector     bc        = flow.cellBarycenter(cell);
		// Real xi = cell->info()[0];
		// Real yi = cell->info()[1];
		// Real zi = cell->info()[2];
		//Vector3r cellCoords(xi,yi,zi);
		Vector3r cellCoords(bc[0], bc[1], bc[2]);
		if (!resetVolumeSolids) {
			for (unsigned int k = 0; k < gridCoords.size(); k++) {
				Vector3r vec = cellCoords - gridCoords[k];
				if (vec.norm() < finalDist) {
					finalDist = vec.norm();
					finalPoro = porosities[k];
				}
			}

			// finalPoro = meanInitialPorosity;
			if (finalPoro <= minPoroClamp)
				finalPoro = minPoroClamp;
			if (finalPoro >= maxPoroClamp)
				finalPoro = maxPoroClamp;

			if (cell->info().Pcondition) {
				//cout << "setting boundary cell porosity to mean initial" << endl;
				cell->info().porosity = meanInitialPorosity;
			}
			cell->info().porosity = cell->info().initialPorosity = finalPoro;
			if (finalPoro > maxPorosity)
				maxPorosity = finalPoro;
		}

		cell->info().vSolids = cell->info().volume() * (1. - cell->info().porosity);
		if (!resetVolumeSolids) {
			cell->info().Po = Po* exp(a * (meanInitialPorosity - cell->info().porosity)); // use unique cell initial porosity or overall average porosity (mu)?
			cell->info().lambdao = lmbda * exp(b * (meanInitialPorosity - cell->info().porosity));
		}
	}
	if (resetVolumeSolids)
		resetVolumeSolids = false;
}


void PartialSatClayEngine::setInitialPorosity(FlowSolver& flow)
{ // assume that the porosity is weibull distributed
	Tesselation& Tes = flow.T[flow.currentTes];
	//	#ifdef YADE_OPENMP
	const long size = Tes.cellHandles.size();
#pragma omp parallel for
	for (long i = 0; i < size; i++) {
		CellHandle& cell = Tes.cellHandles[i];
		//if (cell->info().isAlpha) continue;

		//Real voidSpace = cell->info().volume()*meanInitialPorosity; // assume voidSpace is equivalent to mean porosity
		//		Real oneSixth = (1./6.); // speed up calcs
		//unsigned long long int numPoreCounts = ceil(voidSpace/(oneSixth*pow(meanPoreSizeDiameter,3)*M_PI)); // determine number of pores we expect to encounter for this cell
		//cout << "numPoreCounts "<< numPoreCounts << " voidSpace " << voidSpace << " volPore " << (oneSixth*pow(meanPoreSizeDiameter,3)*M_PI) <<  endl;

		// generate a pore size based on exponential distribution for each expected pore (using MIP values to fit proper parameters)
		//		Real cellPoreVol = 0; int numPoreCounts = 1e6;
		//		#pragma omp parallel
		//		for (int j=0;j<numPoreCounts;j++)
		//		{
		//			Real poreDiam = exponentialDeviate(alphaExpRate,betaExpRate);
		//			//cout << "pore diam generated " << poreDiam <<endl;
		//			cellPoreVol += oneSixth * pow(poreDiam*1e-6,3) * M_PI;
		//		}
		//
		//		// determine new porosity value
		//		Real poro = cellPoreVol / cell->info().volume();
		if (!resetVolumeSolids) {
			Real poro;
			if (!constantPorosity) {
				poro = meanInitialPorosity * weibullDeviate(lambdaWeibullShape, kappaWeibullScale);
				if (poro < minPoroClamp)
					poro = minPoroClamp;
				if (poro > maxPoroClamp)
					poro = maxPoroClamp;
			} else {
				poro = meanInitialPorosity;
			}
			cell->info().porosity = cell->info().initialPorosity = poro;
			if (poro > maxPorosity)
				maxPorosity = poro;
		}

		cell->info().vSolids = cell->info().volume() * (1. - cell->info().porosity);
		// cell->info().invVoidVolume() = 1./(cell->info().volume()*cell->info().porosity); // do this at each triangulation?
		// set parameters for unique PcS curve on this cell:
		if (!resetVolumeSolids) {
			cell->info().Po = Po * exp(a * (meanInitialPorosity - cell->info().porosity)); // use unique cell initial porosity or overall average porosity (mu)?
			cell->info().lambdao = lmbda * exp(b * (meanInitialPorosity - cell->info().porosity));
		}
	}
	if (resetVolumeSolids)
		resetVolumeSolids = false;
}

Real PartialSatClayEngine::weibullDeviate(Real lambda, Real k)
{
	std::random_device              rd;
	std::mt19937                    e2(rd());
	std::weibull_distribution<Real> weibullDistribution(lambda, k);
	Real                            correction = weibullDistribution(e2);
	return correction;
}

Real PartialSatClayEngine::exponentialDeviate(Real a, Real b)
{
	std::random_device                   dev;
	std::mt19937                         rng(dev());
	std::uniform_real_distribution<Real> dist(0, 1.);
	Real                                 x = dist(rng);
	if (x == 1.)
		return 9.999999999999e-1; // return value to avoid undefined behavior
	Real deviate = -(1. / b) * (log(1. - x) - log(a));
	return exp(deviate); // linearized value, so we convert back using exp(y)
}

Real PartialSatClayEngine::laplaceDeviate(Real mu, Real b)
{
	std::random_device                   dev;
	std::mt19937                         rng(dev());
	std::uniform_real_distribution<Real> dist(-0.5, 0.5);
	Real                                 x   = dist(rng);
	Real                                 sgn = x > 0 ? 1. : -1.;
	return mu - b * sgn * log(1. - 2. * fabs(x)); // inverse of laplace CDF
	                                              //	if (x<mu) return  1./2. * exp((x-mu)/b);
	                                              //	else return 1. - 1./2.*exp(-(x-mu)/b);
}

void PartialSatClayEngine::setCellsDSDP(FlowSolver& flow)
{
	Tesselation& Tes = flow.T[flow.currentTes];
	//	#ifdef YADE_OPENMP
	const long size = Tes.cellHandles.size();
#pragma omp parallel for
	for (long i = 0; i < size; i++) {
		CellHandle& cell = Tes.cellHandles[i];
		Real        deriv;
		if (cell->info().isAlpha)
			continue;
		if (!freezeSaturation)
			deriv = dsdp(cell);
		else
			deriv = 0;
		if (!isnan(deriv))
			cell->info().dsdp = deriv;
		else
			cell->info().dsdp = 0;
		cell->info().oldPressure = cell->info().p();
		//cout << "dsdp " << deriv << endl;
	}
}

Real PartialSatClayEngine::dsdp(CellHandle& cell)
{
	//	Real pc1 = pAir - cell->info().p();
	//	// derivative estimate
	//	Real saturation1 = pow((1. + pow(pc1/cell->info().Po,1./(1.-cell->info().lambdao))),-cell->info().lambdao);
	//	Real pc2 = pc1+100.; // small increment
	//	Real saturation2 = pow((1. + pow(pc2/cell->info().Po,1./(1.-cell->info().lambdao))),-cell->info().lambdao);
	//	Real dsdp = (saturation1 - saturation2) / (pc1-pc2);
	//	return dsdp;
	// analytical derivative of van genuchten
	const Real pc = pAir - cell->info().p(); // suction
	if (pc <= 0) return 0;
	const Real term1 = pow(pow(pc / cell->info().Po, 1. / (1. - cell->info().lambdao)) + 1., (-cell->info().lambdao - 1.));
	const Real term2 = cell->info().lambdao * pow(pc / cell->info().Po, 1. / (1. - cell->info().lambdao) - 1.);
	const Real term3 = cell->info().Po * (1. - cell->info().lambdao);
	return term1 * term2 / term3; // techncially this derivative should be negative, but we use a van genuchten fit for suction, not water pressure. Within the numerical formulation, we want the change of saturation with respect to water pressure (not suction). Which essentially reverses the sign of the derivative.

	// alternate form of analytical derivative from VG 1908 https://www.nrc.gov/docs/ML0330/ML033070005.pdf
	//	Real term1 = -cell->info().lambdao/(pc*(1.-cell->info().lambdao));
	//	Real term2 = pow(vanGenuchten(cell,pc),1./cell->info().lambdao);
	//	Real term3 = pow(pow(1.-vanGenuchten(cell,pc),1./cell->info().lambdao),cell->info().lambdao);
	//	return term1*term2*term3;
}

Real PartialSatClayEngine::vanGenuchten(CellHandle& cell, Real pc)
{
	return pow((1. + pow(pc / cell->info().Po, 1. / (1. - cell->info().lambdao))), -cell->info().lambdao);
}

void PartialSatClayEngine::initializeSaturations(FlowSolver& flow)
{
	Tesselation& Tes = flow.T[flow.currentTes];
	//	#ifdef YADE_OPENMP
	const long size = Tes.cellHandles.size();
#pragma omp parallel for
	for (long i = 0; i < size; i++) {
		CellHandle& cell = Tes.cellHandles[i];
		//if (cell->info().blocked) continue;
		setSaturationFromPcS(cell);
	}
}

void PartialSatClayEngine::updateBoundarySaturation(FlowSolver& flow)
{
	if (alphaBound >= 0) {
		for (FlowSolver::VCellIterator it = flow.alphaBoundingCells.begin(); it != flow.alphaBoundingCells.end(); it++) {
			if ((*it) == NULL) continue;
			setSaturationFromPcS(*it);
		}
	} else {
		for (int i = 0; i < 6; i++) {
			for (FlowSolver::VCellIterator it = flow.boundingCells[i].begin(); it != flow.boundingCells[i].end(); it++) {
				if ((*it) == NULL) continue;
				setSaturationFromPcS(*it);
			}
		}
	}
}

Real PartialSatClayEngine::getTotalVolume()
{
	Tesselation& Tes = solver->T[solver->currentTes];
	//	#ifdef YADE_OPENMP
	totalSpecimenVolume = 0;
	const long size     = Tes.cellHandles.size();
#pragma omp parallel for
	for (long i = 0; i < size; i++) {
		CellHandle& cell = Tes.cellHandles[i];
		if (cell->info().isAlpha or cell->info().isFictious)
			continue;
		totalSpecimenVolume += cell->info().volume();
	}
	return totalSpecimenVolume;
}

void PartialSatClayEngine::setSaturationFromPcS(CellHandle& cell)
{
	// using van genuchten
	Real pc = pAir - cell->info().p(); // suction in macrostructure
	Real saturation;
	if (pc >= 0)
		saturation = vanGenuchten(cell, pc); //pow((1. + pow(pc/cell->info().Po,1./(1.-cell->info().lambdao))),-cell->info().lambdao);
	else
		saturation = 1.;
	if (saturation < SrM)
		saturation = SrM;
	if (saturation > SsM)
		saturation = SsM;
	cell->info().saturation        = saturation;
	cell->info().initialSaturation = saturation;
}


void PartialSatClayEngine::updateSaturation(FlowSolver& flow)
{
	// 	Tesselation& Tes = flow.T[flow.currentTes];
	// //	#ifdef YADE_OPENMP
	// 	const long size = Tes.cellHandles.size();
	// //	#pragma omp parallel for num_threads(ompThreads>0 ? ompThreads : 1)
	//     	for (long i=0; i<size; i++){
	// 		CellHandle& cell = Tes.cellHandles[i];
	// 		if (cell->info().Pcondition or cell->info().isAlpha) continue;
	// 		Real sum=0;
	// 		for (int j=0; j<4; j++){
	// 			CellHandle neighborCell = cell->neighbor(j);
	// 			//if (neighborCell->info().isAlpha) continue;
	// 			sum += cell->info().kNorm()[j] * (cell->info().p() - neighborCell->info().p());
	// 		}
	//         	cell->info().saturation = cell->info().saturation - scene->dt * cell->info().invVoidVolume() * sum;
	//                 if (cell->info().saturation<1e-6) {
	//                         cell->info().saturation = 1e-6;
	//                         cerr << "cell saturation dropped below threshold" << endl;
	//                 }
	//                 if (cell->info().saturation>1) {
	//                         cell->info().saturation = 1;
	//                         cerr << "cell saturation exceeded 1" << endl;
	//                 }
	// 	}


	Tesselation& Tes = flow.T[flow.currentTes];
	//	#ifdef YADE_OPENMP
	const long size = Tes.cellHandles.size();
	//	#pragma omp parallel for num_threads(ompThreads>0 ? ompThreads : 1)
	for (long i = 0; i < size; i++) {
		CellHandle& cell = Tes.cellHandles[i];
		if (cell->info().Pcondition or cell->info().isAlpha or cell->info().blocked)
			continue;
		cell->info().saturation = cell->info().saturation + cell->info().dsdp * (cell->info().p() - cell->info().oldPressure);
		if (cell->info().saturation < SrM)
			cell->info().saturation = SrM;
		if (cell->info().saturation > SsM)
			cell->info().saturation = SsM;

		// if (cell->info().saturation < 1e-5) {
		// 	cell->info().saturation = 1e-5;
		// 	//cerr << "cell saturation dropped below threshold" << endl;
		// } // keep an ultra low minium value to avoid numerical issues?
	}


	//         Tesselation& Tes = flow.T[flow.currentTes];
	// 	const long sizeFacets = Tes.facetCells.size();
	// //	#pragma omp parallel for  //FIXME: does not like running in parallel for some reason
	//     	for (long i=0; i<sizeFacets; i++){
	// 		std::pair<CellHandle,int> facetPair = Tes.facetCells[i];
	// 		const CellHandle& cell = facetPair.first;
	// 		const CellHandle& neighborCell = cell->neighbor(facetPair.second);
	//                 const Real satFlux = cell->info().invVoidVolume()*cell->info().kNorm()[facetPair.second] * (cell->info().p() - neighborCell->info().p());
	//                 if (!cell->info().Pcondition) cell->info().saturation -= satFlux*scene->dt;
	//                 if (!cell->info().Pcondition) neighborCell->info().saturation += satFlux*scene->dt;
	//         }
}

void PartialSatClayEngine::resetParticleSuctions()
{
	const shared_ptr<BodyContainer>& bodies = scene->bodies;
	const long                       size   = bodies->size();

#pragma omp parallel for
	for (long i = 0; i < size; i++) {
		const shared_ptr<Body>& b = (*bodies)[i];
		if (b->shape->getClassIndex() != Sphere::getClassIndexStatic() || !b)
			continue;
		if (!b->isStandalone())
			continue;

		PartialSatState* state = dynamic_cast<PartialSatState*>(b->state.get());
		state->suction         = 0;
	}
}

void PartialSatClayEngine::collectParticleSuction(FlowSolver& flow)
{
	resetParticleSuctions();
	shared_ptr<BodyContainer>& bodies = scene->bodies;
	Tesselation&               Tes    = flow.T[flow.currentTes];
	const long                 size   = Tes.cellHandles.size();
#pragma omp parallel for
	for (long i = 0; i < size; i++) {
		CellHandle& cell = Tes.cellHandles[i];

		if (cell->info().isGhost || cell->info().blocked || cell->info().isAlpha) //|| cell->info().Pcondition || cell->info().isFictious || cell->info().isAlpha
			continue; // Do we need special cases for fictious cells? May need to consider boundary contriubtion to node saturation...
		for (int v = 0; v < 4; v++) {
			//if (cell->vertex(v)->info().isFictious) continue;
			if (cell->vertex(v)->info().isAlpha) continue; // avoid adding alpha vertex to suction?
			const long int          id = cell->vertex(v)->info().id();
			const shared_ptr<Body>& b  = (*bodies)[id];
			if (b->shape->getClassIndex() != Sphere::getClassIndexStatic() || !b)
				continue;
			PartialSatState* state  = dynamic_cast<PartialSatState*>(b->state.get());
			Sphere*          sphere = dynamic_cast<Sphere*>(b->shape.get());
			//if (cell->info().isExposed) state->suctionSum+= pAir; // use different pressure for exposed cracks?
			if (!fracBasedPointSuctionCalc) {
				state->suctionSum += pAir - cell->info().p();
				state->incidentCells += 1;
			} else {
				Real areaFrac  = cell->info().sphericalVertexSurface[v];
				Real totalArea = 4 * M_PI * pow(sphere->radius, 2);
				state->suction += (areaFrac / totalArea) * (pAir - cell->info().p());
			}
		}
	}
}

void PartialSatClayEngine::setOriginalParticleValues()
{
	const shared_ptr<BodyContainer>& bodies = scene->bodies;
	const long                       size   = bodies->size();
#pragma omp parallel for
	for (long i = 0; i < size; i++) {
		const shared_ptr<Body>& b = (*bodies)[i];
		if (b->shape->getClassIndex() != Sphere::getClassIndexStatic() || !b)
			continue;
		Sphere*          sphere = dynamic_cast<Sphere*>(b->shape.get());
		const Real       volume = 4. / 3. * M_PI * pow(sphere->radius, 3.);
		PartialSatState* state  = dynamic_cast<PartialSatState*>(b->state.get());
		state->volumeOriginal   = volume;
		state->radiiOriginal    = sphere->radius;
	}
}


void PartialSatClayEngine::swellParticles()
{
	const shared_ptr<BodyContainer>& bodies   = scene->bodies;
	const long                       size     = bodies->size();
	const Real                       suction0 = pAir - pZero;
	totalVolChange                            = 0;
#pragma omp parallel for
	for (long i = 0; i < size; i++) {
		const shared_ptr<Body>& b = (*bodies)[i];
		if (b->shape->getClassIndex() != Sphere::getClassIndexStatic() || !b) continue;
		if (!b->isStandalone()) continue;
		Sphere* sphere = dynamic_cast<Sphere*>(b->shape.get());
		//const Real volume = 4./3. * M_PI*pow(sphere->radius,3);
		PartialSatState* state = dynamic_cast<PartialSatState*>(b->state.get());

		if (!fracBasedPointSuctionCalc) {
			state->lastIncidentCells = state->incidentCells;
			state->suction           = state->suctionSum / state->incidentCells;
			state->incidentCells     = 0; // reset to 0 for next time step
			state->suctionSum        = 0; //
		}

		const Real volStrain = betam / alpham * (exp(-alpham * state->suction) - exp(-alpham * suction0));
		//		const Real rOrig = pow(state->volumeOriginal * 3. / (4.*M_PI),1./3.);
		//
		const Real vNew = state->volumeOriginal * (volStrain + 1.);
		const Real rNew = pow(3. * vNew / (4. * M_PI), 1. / 3.);
		if (rNew <= minParticleSwellFactor * state->radiiOriginal) continue;  // dont decrease size too much
		totalVolChange += (pow(rNew, 3.) - pow(sphere->radius, 3.)) * 4. / 3. * M_PI;
		state->radiiChange = rNew - state->radiiOriginal;
		sphere->radius     = rNew;
		//		cout << "volStrain "<<volStrain<<" avgSuction "<<avgSuction<<" suction0 " <<suction0<<" rDel "<<rDel<<" rNew "<< rNew << " rOrig "<< rOrig << endl;
	}
}

//////// Post processing tools //////

void PartialSatClayEngine::saveFractureNetworkVTK(string fileName) // Superceded by savePermeabilityNetworkVTK
{
	vtkSmartPointer<vtkPoints>    intrCellPos = vtkSmartPointer<vtkPoints>::New();
	vtkSmartPointer<vtkCellArray> fracCells   = vtkSmartPointer<vtkCellArray>::New();

	boost::unordered_map<int, int> cIdVector;
	int                            curId = 0;
	Tesselation&                   Tes   = solver->tesselation(); //flow.T[flow.currentTes];
	const long                     size  = Tes.cellHandles.size();
	//#pragma omp parallel for
	for (long i = 0; i < size; i++) {
		CellHandle& cell = Tes.cellHandles[i];
		if (solver->T[solver->currentTes].Triangulation().is_infinite(cell))
			continue;
		if (cell->info().Pcondition)
			continue;
		if (cell->info().isFictious)
			continue;
		Point& p2 = cell->info();
		intrCellPos->InsertNextPoint(p2[0], p2[1], p2[2]);
		cIdVector.insert(std::pair<int, int>(cell->info().id, curId));
		curId++;
	}

	//Tesselation& Tes = solver->tesselation(); //flow.T[flow.currentTes];
	const long sizeFacets = Tes.facetCells.size();
	//#pragma omp parallel for
	for (long i = 0; i < sizeFacets; i++) {
		std::pair<CellHandle, int> facetPair = Tes.facetCells[i];
		const CellHandle&          cell      = facetPair.first;
		const CellHandle&          nCell     = cell->neighbor(facetPair.second);
		if (solver->T[solver->currentTes].Triangulation().is_infinite(nCell) or solver->T[solver->currentTes].Triangulation().is_infinite(cell))
			continue;
		if (nCell->info().Pcondition or cell->info().Pcondition)
			continue;
		if (nCell->info().isFictious or cell->info().isFictious)
			continue;
		if (cell->info().crack and nCell->info().crack) {
			const auto iterId1    = cIdVector.find(cell->info().id);
			const auto iterId2    = cIdVector.find(nCell->info().id);
			const auto setId1Line = iterId1->second;
			const auto setId2Line = iterId2->second;

			vtkSmartPointer<vtkLine> line = vtkSmartPointer<vtkLine>::New();
			line->GetPointIds()->SetId(0, setId1Line);
			line->GetPointIds()->SetId(1, setId2Line);
			fracCells->InsertNextCell(line);
		}
	}
	vtkSmartPointer<vtkPolyData> intrPd = vtkSmartPointer<vtkPolyData>::New();
	intrPd->SetPoints(intrCellPos);
	intrPd->SetLines(fracCells);

	vtkSmartPointer<vtkXMLPolyDataWriter> writer = vtkSmartPointer<vtkXMLPolyDataWriter>::New();
	//if(compress) writer->SetCompressor(compressor);
	//if(ascii) writer->SetDataModeToAscii();
	string fn = fileName + "fracNet." + boost::lexical_cast<string>(scene->iter) + ".vtp";
	writer->SetFileName(fn.c_str());
#ifdef YADE_VTK6
	writer->SetInputData(intrPd);
#else
	writer->SetInput(intrPd);
#endif
	writer->Write();
}


void PartialSatClayEngine::savePermeabilityNetworkVTK(string fileName)
{
	vtkSmartPointer<vtkPoints>              intrCellPos = vtkSmartPointer<vtkPoints>::New();
	vtkSmartPointer<vtkDoubleArrayFromReal> permArray   = vtkSmartPointer<vtkDoubleArrayFromReal>::New();
	vtkSmartPointer<vtkDoubleArrayFromReal> fracArray   = vtkSmartPointer<vtkDoubleArrayFromReal>::New();
	permArray->SetNumberOfComponents(1);
	permArray->SetName("permeability");
	fracArray->SetName("fractured");
	vtkSmartPointer<vtkCellArray> permCells = vtkSmartPointer<vtkCellArray>::New();

	boost::unordered_map<int, int> cIdVector;
	int                            curId = 0;
	Tesselation&                   Tes   = solver->tesselation(); //flow.T[flow.currentTes];
	const long                     size  = Tes.cellHandles.size();
	//#pragma omp parallel for
	for (long i = 0; i < size; i++) {
		CellHandle& cell = Tes.cellHandles[i];
		if (solver->T[solver->currentTes].Triangulation().is_infinite(cell))
			continue;
		//if (cell->info().Pcondition) continue;
		if (cell->info().isFictious)
			continue;
		Point& p2 = cell->info();
		intrCellPos->InsertNextPoint(p2[0], p2[1], p2[2]);
		cIdVector.insert(std::pair<int, int>(cell->info().id, curId));
		curId++;
	}

	//Tesselation& Tes = solver->tesselation(); //flow.T[flow.currentTes];
	const long sizeFacets = Tes.facetCells.size();
	//#pragma omp parallel for
	for (long i = 0; i < sizeFacets; i++) {
		std::pair<CellHandle, int> facetPair = Tes.facetCells[i];
		const CellHandle&          cell      = facetPair.first;
		const CellHandle&          nCell     = cell->neighbor(facetPair.second);
		if (solver->T[solver->currentTes].Triangulation().is_infinite(nCell) or solver->T[solver->currentTes].Triangulation().is_infinite(cell))
			continue;
		if (nCell->info().Pcondition or cell->info().Pcondition)
			continue;
		if (nCell->info().isFictious or cell->info().isFictious)
			continue;

		const auto iterId1    = cIdVector.find(cell->info().id);
		const auto iterId2    = cIdVector.find(nCell->info().id);
		const auto setId1Line = iterId1->second;
		const auto setId2Line = iterId2->second;

		vtkSmartPointer<vtkLine> line = vtkSmartPointer<vtkLine>::New();
		line->GetPointIds()->SetId(0, setId1Line);
		line->GetPointIds()->SetId(1, setId2Line);
		permCells->InsertNextCell(line);
		permArray->InsertNextValue(cell->info().kNorm()[facetPair.second]);
		if (cell->info().crack and nCell->info().crack)
			fracArray->InsertNextValue(1);
		else
			fracArray->InsertNextValue(0);
	}
	vtkSmartPointer<vtkPolyData> intrPd = vtkSmartPointer<vtkPolyData>::New();
	intrPd->SetPoints(intrCellPos);
	intrPd->SetLines(permCells);
	intrPd->GetCellData()->AddArray(permArray);
	intrPd->GetCellData()->AddArray(fracArray);

	vtkSmartPointer<vtkXMLPolyDataWriter> writer = vtkSmartPointer<vtkXMLPolyDataWriter>::New();
	//if(compress) writer->SetCompressor(compressor);
	//if(ascii) writer->SetDataModeToAscii();
	string fn = fileName + "permNet." + boost::lexical_cast<string>(scene->iter) + ".vtp";
	writer->SetFileName(fn.c_str());
#ifdef YADE_VTK6
	writer->SetInputData(intrPd);
#else
	writer->SetInput(intrPd);
#endif
	writer->Write();
}


void PartialSatClayEngine::saveUnsatVtk(const char* folder, bool withBoundaries)
{
	vector<int>
	            allIds; //an ordered list of cell ids (from begin() to end(), for vtk table lookup), some ids will appear multiple times since boundary cells are splitted into multiple tetrahedra
	vector<int> fictiousN;
	bool        initNoCache = solver->noCache;
	solver->noCache         = false;

	static unsigned int number = 0;
	char                filename[250];
	mkdir(folder, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	sprintf(filename, "%s/out_%d.vtk", folder, number++);
	basicVTKwritter vtkfile(0, 0);
	solver->saveMesh(vtkfile, withBoundaries, allIds, fictiousN, filename);
	solver->noCache = initNoCache;

	vtkfile.begin_data("Porosity", CELL_DATA, SCALARS, FLOAT);
	for (unsigned kk = 0; kk < allIds.size(); kk++)
		vtkfile.write_data(solver->tesselation().cellHandles[allIds[kk]]->info().porosity);
	vtkfile.end_data();

	vtkfile.begin_data("Saturation", CELL_DATA, SCALARS, FLOAT);
	for (unsigned kk = 0; kk < allIds.size(); kk++)
		vtkfile.write_data(solver->tesselation().cellHandles[allIds[kk]]->info().saturation);
	vtkfile.end_data();

	vtkfile.begin_data("Alpha", CELL_DATA, SCALARS, FLOAT);
	for (unsigned kk = 0; kk < allIds.size(); kk++)
		vtkfile.write_data(solver->tesselation().cellHandles[allIds[kk]]->info().isAlpha);
	vtkfile.end_data();

	vtkfile.begin_data("Pressure", CELL_DATA, SCALARS, FLOAT);
	for (unsigned kk = 0; kk < allIds.size(); kk++)
		vtkfile.write_data(solver->tesselation().cellHandles[allIds[kk]]->info().p());
	vtkfile.end_data();

	vtkfile.begin_data("fictious", CELL_DATA, SCALARS, INT);
	for (unsigned kk = 0; kk < allIds.size(); kk++)
		vtkfile.write_data(fictiousN[kk]);
	vtkfile.end_data();

	vtkfile.begin_data("blocked", CELL_DATA, SCALARS, INT);
	for (unsigned kk = 0; kk < allIds.size(); kk++)
		vtkfile.write_data(solver->tesselation().cellHandles[allIds[kk]]->info().blocked);
	vtkfile.end_data();

	vtkfile.begin_data("id", CELL_DATA, SCALARS, INT);
	for (unsigned kk = 0; kk < allIds.size(); kk++)
		vtkfile.write_data(allIds[kk]);
	vtkfile.end_data();

	vtkfile.begin_data("fracturedCells", CELL_DATA, SCALARS, INT);
	for (unsigned kk = 0; kk < allIds.size(); kk++)
		vtkfile.write_data(solver->tesselation().cellHandles[allIds[kk]]->info().crack);
	vtkfile.end_data();

	vtkfile.begin_data("porosityChange", CELL_DATA, SCALARS, FLOAT);
	for (unsigned kk = 0; kk < allIds.size(); kk++)
		vtkfile.write_data(
		        solver->tesselation().cellHandles[allIds[kk]]->info().porosity - solver->tesselation().cellHandles[allIds[kk]]->info().initialPorosity);
	vtkfile.end_data();

	vtkfile.begin_data("saturationChange", CELL_DATA, SCALARS, FLOAT);
	for (unsigned kk = 0; kk < allIds.size(); kk++)
		vtkfile.write_data(
		        solver->tesselation().cellHandles[allIds[kk]]->info().saturation
		        - solver->tesselation().cellHandles[allIds[kk]]->info().initialSaturation);
	vtkfile.end_data();


	vtkfile.begin_data("Permeability", CELL_DATA, SCALARS, FLOAT);
	for (unsigned kk = 0; kk < allIds.size(); kk++) {
		std::vector<Real> perm    = solver->tesselation().cellHandles[allIds[kk]]->info().kNorm();
		Real              permSum = 0;
		for (unsigned int i = 0; i < perm.size(); i++)
			permSum += perm[i];
		vtkfile.write_data(permSum / perm.size());
	}
	vtkfile.end_data();

	solver->averageRelativeCellVelocity();
	vtkfile.begin_data("Velocity", CELL_DATA, VECTORS, FLOAT);
	for (unsigned kk = 0; kk < allIds.size(); kk++)
		vtkfile.write_data(
		        solver->tesselation().cellHandles[allIds[kk]]->info().averageVelocity()[0],
		        solver->tesselation().cellHandles[allIds[kk]]->info().averageVelocity()[1],
		        solver->tesselation().cellHandles[allIds[kk]]->info().averageVelocity()[2]);
	vtkfile.end_data();

#define SAVE_CELL_INFO(INFO)                                                                                                                                   \
	vtkfile.begin_data(#INFO, CELL_DATA, SCALARS, FLOAT);                                                                                                  \
	for (unsigned kk = 0; kk < allIds.size(); kk++)                                                                                                        \
		vtkfile.write_data(solver->tesselation().cellHandles[allIds[kk]]->info().INFO);                                                                \
	vtkfile.end_data();
	//SAVE_CELL_INFO(saturation)
	SAVE_CELL_INFO(Po)
	SAVE_CELL_INFO(lambdao)
	SAVE_CELL_INFO(Pcondition)
	SAVE_CELL_INFO(isExposed)
	SAVE_CELL_INFO(crackNum)
	//	SAVE_CELL_INFO(porosity)
}

void PartialSatClayEngine::initSolver(FlowSolver& flow)
{
	flow.Vtotalissimo = 0;
	flow.VSolidTot    = 0;
	flow.vPoral       = 0;
	flow.sSolidTot    = 0;
	flow.slipBoundary = slipBoundary;
	flow.kFactor      = permeabilityFactor;
	flow.debugOut     = debug;
	flow.useSolver    = useSolver;
#ifdef LINSOLV
	flow.numSolveThreads     = numSolveThreads;
	flow.numFactorizeThreads = numFactorizeThreads;
#endif
	flow.factorizeOnly         = false;
	flow.meanKStat             = meanKStat;
	flow.viscosity             = viscosity;
	flow.tolerance             = tolerance;
	flow.relax                 = relax;
	flow.clampKValues          = clampKValues;
	flow.maxKdivKmean          = maxKdivKmean;
	flow.minKdivKmean          = minKdivKmean;
	flow.meanKStat             = meanKStat;
	flow.permeabilityMap       = permeabilityMap;
	flow.fluidBulkModulus      = fluidBulkModulus;
	flow.fluidRho              = fluidRho;
	flow.fluidCp               = fluidCp;
	flow.thermalEngine         = thermalEngine;
	flow.multithread           = multithread;
	flow.getCHOLMODPerfTimings = getCHOLMODPerfTimings;
	flow.tesselation().maxId   = -1;
	flow.blockedCells.clear();
	flow.sphericalVertexAreaCalculated = false;
	flow.xMin = 1000.0, flow.xMax = -10000.0, flow.yMin = 1000.0, flow.yMax = -10000.0, flow.zMin = 1000.0, flow.zMax = -10000.0;
	flow.partialSatEngine   = partialSatEngine;
	flow.pAir               = pAir;
	flow.freeSwelling       = freeSwelling;
	flow.matricSuctionRatio = matricSuctionRatio;
	flow.nUnsatPerm         = nUnsatPerm;
	flow.SrM                = SrM;
	flow.SsM                = SsM;
	flow.tesselation().vertexHandles.clear();
	flow.tesselation().vertexHandles.resize(scene->bodies->size() + 6, NULL);
	flow.tesselation().vertexHandles.shrink_to_fit();
	flow.alphaBound          = alphaBound;
	flow.alphaBoundValue     = alphaBoundValue;
	flow.freezePorosity      = freezePorosity;
	flow.useKeq              = useKeq;
	flow.useKozeny           = useKozeny;
	flow.bIntrinsicPerm      = bIntrinsicPerm;
	flow.meanInitialPorosity = meanInitialPorosity;
	flow.freezeSaturation    = freezeSaturation;
	flow.permClamp           = permClamp;
	flow.manualCrackPerm     = manualCrackPerm;
}

void PartialSatClayEngine::buildTriangulation(Real pZero, Solver& flow)
{
	//cout << "retriangulation" << endl;
	if (first) flow.currentTes = 0;
	else {
		flow.currentTes = !flow.currentTes;
		if (debug) cout << "--------RETRIANGULATION-----------" << endl;
	}
	if (debug) cout << "about to reset network" << endl;
	flow.resetNetwork();
	initSolver(flow);
	if (alphaBound < 0) addBoundary(flow);
	else flow.alphaBoundingCells.clear();
	if (debug) cout << "about to add triangulate" << endl;
	//   std::cout << "About to triangulate, push button...";
	//   std::cin.get();
	triangulate(flow);
	//    std::cout << "just triangulated, size" << sizeof(flow.tesselation().Triangulation()) << " push button..." << endl;
	//    std::cin.get();
	if (debug) cout << endl << "Tesselating------" << endl << endl;
	flow.tesselation().compute();
	if (alphaBound < 0) flow.defineFictiousCells();
	// For faster loops on cells define this vector
	flow.tesselation().cellHandles.clear();
	flow.tesselation().cellHandles.reserve(flow.tesselation().Triangulation().number_of_finite_cells());
	FiniteCellsIterator cell_end = flow.tesselation().Triangulation().finite_cells_end();
	int k = 0;
	for (FiniteCellsIterator cell = flow.tesselation().Triangulation().finite_cells_begin(); cell != cell_end; cell++) {
		if (cell->info().isAlpha) continue; // do we want alpha cells in the general cell handle list?
		flow.tesselation().cellHandles.push_back(cell);
		cell->info().id = k++;
	} //define unique numbering now, corresponds to position in cellHandles
	flow.tesselation().cellHandles.shrink_to_fit();

	if (thermalEngine || partialSatEngine) {
		flow.tesselation().facetCells.clear();
		flow.tesselation().facetCells.reserve(flow.tesselation().Triangulation().number_of_finite_facets());
		for (FiniteCellsIterator cell = flow.tesselation().Triangulation().finite_cells_begin(); cell != cell_end; cell++) {
			for (int i = 0; i < 4; i++) {
				if (cell->info().id < cell->neighbor(i)->info().id) {
					flow.tesselation().facetCells.push_back(std::pair<CellHandle, int>(cell, i));
				}
			}
		}
		flow.tesselation().facetCells.shrink_to_fit();
	}
	flow.displayStatistics();
	if (!blockHook.empty()) {
		LOG_INFO("Running blockHook: " << blockHook);
		pyRunString(blockHook);
	}
	//flow.computePermeability(); // move to after interpolate since perm now depends on saturation, and saturation is interpolated value
	//	std::cout << "computed perm, about tto initialize pressure, push button...";
	//   std::cin.get();
	if (multithread && (fluidBulkModulus > 0 || partialSatEngine))
		initializeVolumes(flow); // needed for multithreaded compressible flow (old site, fixed bug https://bugs.launchpad.net/yade/+bug/1687355)
		                         //	if (crackModelActive) trickPermeability(&flow);
	porosity = flow.vPoralPorosity / flow.vTotalPorosity;

	if (alphaBound < 0) boundaryConditions(flow);
	if (debug) cout << "about to initialize pressure" << endl;
	flow.initializePressure(pZero);

	if (thermalEngine) {
		//initializeVolumes(flow);
		thermalBoundaryConditions(flow);
		flow.initializeTemperatures(tZero);
		flow.sphericalVertexAreaCalculated = false;
	}

	if (debug) cout << "about to interpolate" << endl;
	if (!first && !multithread && (useSolver == 0 || fluidBulkModulus > 0 || doInterpolate || thermalEngine || partialSatEngine)) {
		flow.interpolate(flow.T[!flow.currentTes], flow.tesselation());

		//if (mineralPoro>0) blockLowPoroRegions(*solver);
	}
	if (debug) cout << "made it out of interpolate" << endl;
	flow.computePermeability();
	//if (crackCellPoroThreshold>0) trickPermOnCrackedCells(*solver);
	if (crackModelActive)
		trickPermeability(&flow);
	if (freeSwelling && crackModelActive)
		determineFracturePaths();
	if (blockIsoCells)
		blockIsolatedCells(*solver);
	if (partialSatEngine) {
		updateBoundarySaturation(flow);
		flow.sphericalVertexAreaCalculated = false;
	}
	if (waveAction)
		flow.applySinusoidalPressure(flow.tesselation().Triangulation(), sineMagnitude, sineAverage, 30);
	else if (boundaryPressure.size() != 0)
		flow.applyUserDefinedPressure(flow.tesselation().Triangulation(), boundaryXPos, boundaryPressure);
	if (normalLubrication || shearLubrication || viscousShear)
		flow.computeEdgesSurfaces();
}


void PartialSatClayEngine::initializeVolumes(FlowSolver& flow)
{
	totalSpecimenVolume = 0;
	typedef typename Solver::FiniteVerticesIterator FiniteVerticesIterator;

	FiniteVerticesIterator vertices_end = flow.tesselation().Triangulation().finite_vertices_end();
	CGT::CVector           Zero(0, 0, 0);
	for (FiniteVerticesIterator V_it = flow.tesselation().Triangulation().finite_vertices_begin(); V_it != vertices_end; V_it++)
		V_it->info().forces = Zero;
	//cout << "in initialize volumes, after force reset" <<endl;
	FOREACH(CellHandle & cell, flow.tesselation().cellHandles)
	{
		//if (cell->info().isAlpha) continue; // not concerned about volumes for alpha cells?
		switch (cell->info().fictious()) {
			case (0): cell->info().volume() = volumeCell(cell); break;
			case (1): cell->info().volume() = volumeCellSingleFictious(cell); break;
			case (2): cell->info().volume() = volumeCellDoubleFictious(cell); break;
			case (3): cell->info().volume() = volumeCellTripleFictious(cell); break;
			default: break;
		}
		//cout << "made it past the volume calc" <<endl;
		if (flow.fluidBulkModulus > 0 || thermalEngine || iniVoidVolumes) {
			cell->info().invVoidVolume() = 1 / (std::abs(cell->info().volume()) - volumeCorrection * flow.volumeSolidPore(cell));
		} else if (partialSatEngine) {
			if (cell->info().volume() <= 0 and cell->info().isAlpha and debug)
				cerr << "cell volume zero, bound to be issues" << endl;
			cell->info().invVoidVolume() = 1 / std::abs(cell->info().volume());
			//cell->info().invVoidVolume() = 1. / std::max(minCellVol, math::abs(cell->info().volume())); // - flow.volumeSolidPore(cell)));
			//if (cell->info().invVoidVolume() == 1. / minCellVol) {
			//	cell->info().blocked = 1;
			//	cout << "using minCellVolume, blocking cell" << endl;
			//}
		}
		//cout << "made it past the invvoidvolume assignment" <<endl;
		if (!cell->info().isAlpha and !cell->info().isFictious)
			totalSpecimenVolume += cell->info().volume();
	}
	if (debug) cout << "Volumes initialised." << endl;
}

void PartialSatClayEngine::updateVolumes(FlowSolver& flow)
{
	if (debug) cout << "Updating volumes.............." << endl;
	Real invDeltaT      = 1 / (partialSatDT == 0 ? scene->dt : solverDT);
	epsVolMax           = 0;
	Real totVol         = 0;
	Real totDVol        = 0;
	totalSpecimenVolume = 0;
#ifdef YADE_OPENMP
	const long size = flow.tesselation().cellHandles.size();
#pragma omp parallel for num_threads(ompThreads > 0 ? ompThreads : 1)
	for (long i = 0; i < size; i++) {
		CellHandle& cell = flow.tesselation().cellHandles[i];
#else
	FOREACH(CellHandle & cell, flow.tesselation().cellHandles)
	{
#endif
		Real newVol, dVol;
		//if (cell->info().isAlpha) continue; // dont care about volumes of alpha cells?
		switch (cell->info().fictious()) {
			case (3): newVol = volumeCellTripleFictious(cell); break;
			case (2): newVol = volumeCellDoubleFictious(cell); break;
			case (1): newVol = volumeCellSingleFictious(cell); break;
			case (0): newVol = volumeCell(cell); break;
			default: newVol = 0; break;
		}
		dVol = cell->info().volumeSign * (newVol - cell->info().volume());
		if (!thermalEngine) cell->info().dv() = dVol * invDeltaT;
		else cell->info().dv() += dVol * invDeltaT; // thermalEngine resets dv() to zero and starts adding to it before this.
		cell->info().volume() = newVol;
		if (!cell->info().isFictious and !cell->info().isAlpha)
			totalSpecimenVolume += newVol;
		if (defTolerance > 0) { //if the criterion is not used, then we skip these updates a save a LOT of time when Nthreads > 1
#pragma omp atomic
			totVol += cell->info().volumeSign * newVol;
#pragma omp atomic
			totDVol += dVol;
		}
	}
	if (defTolerance > 0) epsVolMax = totDVol / totVol;
	//FIXME: move this loop to FlowBoundingSphere
	for (unsigned int n = 0; n < flow.imposedF.size(); n++) {
		flow.IFCells[n]->info().dv() += flow.imposedF[n].second;
		flow.IFCells[n]->info().Pcondition = false;
	}
	if (debug) cout << "Updated volumes, total =" << totVol << ", dVol=" << totDVol << endl;
}


/////// Discrete Fracture Network Functionality ////////

void PartialSatClayEngine::blockCellsAbovePoroThreshold(FlowSolver& flow)
{
	Tesselation& Tes = flow.T[flow.currentTes];
	//	#ifdef YADE_OPENMP
	const long size = Tes.cellHandles.size();
	//#pragma omp parallel for
	//cout << "blocking low poro regions" << endl;
	for (long i = 0; i < size; i++) {
		CellHandle& cell = Tes.cellHandles[i];
		if (cell->info().porosity > crackCellPoroThreshold) {
			cell->info().blocked = true;
			for (int j = 0; j < 4; j++) {
				const CellHandle& nCell = cell->neighbor(j);
				nCell->info().blocked   = true;
			}
		}
	}
}

// void PartialSatClayEngine::blockIsolatedCells(FlowSolver& flow)
// {
//         Tesselation& Tes = flow.T[flow.currentTes];
//         //	#ifdef YADE_OPENMP
//         const long size = Tes.cellHandles.size();
//         //#pragma omp parallel for
//         //cout << "blocking low poro regions" << endl;
//         for (long i=0; i<size; i++){
//                 CellHandle& cell = Tes.cellHandles[i];
//                 if (cell->info().blocked) continue;
//                 for (int j=0; j<4; j++){
//                         const CellHandle& nCell = cell->neighbor(j);
//                         if (!nCell->info().blocked) break;
//                         nCell->info().blocked=true; //cell is surrounded by blocked cells, and therefore needs to be blocked itself.
//                 }
//         }
//
// }

void PartialSatClayEngine::blockIsolatedCells(FlowSolver& flow)
{
	Tesselation& Tes = flow.T[flow.currentTes];
	//	#ifdef YADE_OPENMP
	const long size = Tes.cellHandles.size();
	//#pragma omp parallel for
	//cout << "blocking low poro regions" << endl;
	for (long i = 0; i < size; i++) {
		CellHandle& cell = Tes.cellHandles[i];
		if (cell->info().blocked)
			continue;
		int numBlocked = 0;
		for (int j = 0; j < 4; j++) {
			const CellHandle& nCell = cell->neighbor(j);
			if (nCell->info().blocked)
				numBlocked++;
		}
		if (numBlocked == 4)
			cell->info().blocked = true;
		cell->info().Pcondition = false;
	}
}

void PartialSatClayEngine::removeForcesOnBrokenBonds()
{
	const RTriangulation&                  Tri          = solver->T[solver->currentTes].Triangulation();
	const shared_ptr<InteractionContainer> interactions = scene->interactions;
	FiniteEdgesIterator                    edge         = Tri.finite_edges_begin();
	for (; edge != Tri.finite_edges_end(); ++edge) {
		const VertexInfo&              vi1         = (edge->first)->vertex(edge->second)->info();
		const VertexInfo&              vi2         = (edge->first)->vertex(edge->third)->info();
		const shared_ptr<Interaction>& interaction = interactions->find(vi1.id(), vi2.id());

		if (interaction && interaction->isReal()) {
			if (edge->first->info().isFictious)
				continue; /// avoid trick permeability for fictitious
			//auto mindlingeom = YADE_PTR_CAST<ScGeom>(interaction->geom);
			//ScGeom* mindlingeom = YADE_CAST<ScGeom*>(interaction->geom.get());
			auto mindlinphys = YADE_PTR_CAST<MindlinPhys>(interaction->phys);
			//MindlinPhys* mindlinphys = YADE_CAST<MindlinPhys*>(interaction->phys.get());));
			if (!mindlinphys->isBroken)
				continue;
			circulateFacetstoRemoveForces(edge);
		}
	}
}

void PartialSatClayEngine::circulateFacetstoRemoveForces(RTriangulation::Finite_edges_iterator& edge)
{
	const RTriangulation&            Tri    = solver->T[solver->currentTes].Triangulation();
	RTriangulation::Facet_circulator facet1 = Tri.incident_facets(*edge);
	RTriangulation::Facet_circulator facet0 = facet1++;
	removeForceOnVertices(facet0, edge);
	while (facet1 != facet0) {
		removeForceOnVertices(facet1, edge);
		facet1++;
	}
	/// Needs the fracture surface for this edge?
	// Real edgeArea = solver->T[solver->currentTes].computeVFacetArea(edge); cout<<"edge area="<<edgeArea<<endl;
}

void PartialSatClayEngine::removeForceOnVertices(RTriangulation::Facet_circulator& facet, RTriangulation::Finite_edges_iterator& ed_it)
{
	const RTriangulation::Facet& currentFacet
	        = *facet; /// seems verbose but facet->first was declaring a junk cell and crashing program (old site, fixed bug https://bugs.launchpad.net/yade/+bug/1666339)
	//const RTriangulation& Tri = solver->T[solver->currentTes].Triangulation();
	const CellHandle& cell1 = currentFacet.first;
	const CellHandle& cell2 = currentFacet.first->neighbor(facet->second);
	VertexInfo&       vi1   = (ed_it->first)->vertex(ed_it->second)->info();
	VertexInfo&       vi2   = (ed_it->first)->vertex(ed_it->third)->info();

	// compute area
	Point&  CellCentre1 = cell1->info(); /// Trying to get fracture's surface
	Point&  CellCentre2 = cell2->info(); /// Trying to get fracture's surface
	CVector edge        = ed_it->first->vertex(ed_it->second)->point().point() - ed_it->first->vertex(ed_it->third)->point().point();
	CVector unitV       = edge * (1. / sqrt(edge.squared_length()));
	Point p3 = ed_it->first->vertex(ed_it->third)->point().point() + unitV * (cell1->info() - ed_it->first->vertex(ed_it->third)->point().point()) * unitV;
	Real  halfCrackArea = crackAreaFactor * 0.5 * sqrt(std::abs(cross_product(CellCentre1 - p3, CellCentre2 - p3).squared_length()));

	// modify forces to remove since it is broken
	CVector capillaryForce = edge * halfCrackArea * ((cell1->info().p() + cell2->info().p()) / 2.) * ((cell1->info().sat() + cell2->info().sat()) / 2.);
	//cout << "total force on body"<<vi1.forces[0]<<" "<<vi1.forces[1]<<" "<<vi1.forces[2]<<endl;
	//cout << "capillary force computed" << capillaryForce[0] << " "<<capillaryForce[1]<<" "<<capillaryForce[2]<<endl;
	vi1.forces = vi1.forces + capillaryForce;
	vi2.forces = vi2.forces - capillaryForce;
	//cell1->vertex(facetVertices[j][y])->info().forces = cell1->vertex(facetVertices[j][y])->info().forces -facetNormal*pAir*crossSections[j][y];
}

void PartialSatClayEngine::computeFracturePerm(RTriangulation::Facet_circulator& facet, Real aperture, RTriangulation::Finite_edges_iterator& ed_it)
{
	const RTriangulation::Facet& currentFacet
	        = *facet; /// seems verbose but facet->first was declaring a junk cell and crashing program (old site, fixed bug https://bugs.launchpad.net/yade/+bug/1666339)
	const RTriangulation& Tri   = solver->T[solver->currentTes].Triangulation();
	const CellHandle&     cell1 = currentFacet.first;
	const CellHandle&     cell2 = currentFacet.first->neighbor(facet->second);
	if (Tri.is_infinite(cell1) || Tri.is_infinite(cell2))
		cerr << "Infinite cell found in trickPermeability, should be handled somehow, maybe" << endl;
	if (cell1->info().initiallyCracked)
		return;
	Real fracturePerm = apertureFactor * pow(aperture, 3.) / (12. * viscosity);
	if (changeCrackSaturation and cell1->info().saturation < SsM) {
		cell1->info().crack = 1;
		cell2->info().crack = 1;
		//cell1->info().blocked=1;
		//cell2->info().blocked=1;
		//cell1->info().saturation = SrM;
		//cell2->info().saturation = SrM; //set low saturation to keep some minimum cohesion
		cell1->info().kNorm()[currentFacet.second]                          = manualCrackPerm > 0 ? manualCrackPerm : solver->averageK * 0.01;
		cell2->info().kNorm()[Tri.mirror_index(cell1, currentFacet.second)] = manualCrackPerm > 0 ? manualCrackPerm : solver->averageK * 0.01;
		//cell1->info().p() =
		//cout << "tricked perm on cell " << cell1->info().id << endl;
		// for (int i=0;i<4;i++){ // block this cell from all neighbors, cracked or not cracked.
		//         cell1->info().kNorm()[i] = 0;
		//         cell1->neighbor(i)->info().kNorm()[Tri.mirror_index(cell1,currentFacet.second)]=0;
		// }
	} else { // only using parallel plate approximation if the crack is saturated
		if (!onlyFractureExposedCracks or (onlyFractureExposedCracks and cell1->info().isExposed)) {
			cell1->info().crack = 1;
			cell1->info().kNorm()[currentFacet.second] += fracturePerm;
		} //
		if (!onlyFractureExposedCracks or (onlyFractureExposedCracks and cell2->info().isExposed)) {
			cell2->info().crack = 1;
			cell2->info().kNorm()[Tri.mirror_index(cell1, currentFacet.second)] += fracturePerm;
		}
	}

	//cout << "crack set to true in pore"<<endl;
	// cell2->info().blocked = cell1->info().blocked = cell2->info().Pcondition = cell1->info().Pcondition = false; /// those ones will be included in the flow problem
	Point&  CellCentre1             = cell1->info();                                /// Trying to get fracture's surface
	Point&  CellCentre2             = cell2->info();                                /// Trying to get fracture's surface
	CVector networkFractureLength   = CellCentre1 - CellCentre2;                    /// Trying to get fracture's surface
	Real    networkFractureDistance = sqrt(networkFractureLength.squared_length()); /// Trying to get fracture's surface
	Real    networkFractureArea     = pow(networkFractureDistance, 2);              /// Trying to get fracture's surface
	totalFractureArea += networkFractureArea;                                       /// Trying to get fracture's surface
	// 	cout <<" ------------------ The total surface area up to here is --------------------" << totalFractureArea << endl;
	// 	printFractureTotalArea = totalFractureArea; /// Trying to get fracture's surface
	if (calcCrackArea and !cell1->info().isFictious and !cell1->info().isAlpha) {
		CVector edge  = ed_it->first->vertex(ed_it->second)->point().point() - ed_it->first->vertex(ed_it->third)->point().point();
		CVector unitV = edge * (1. / sqrt(edge.squared_length()));
		Point   p3    = ed_it->first->vertex(ed_it->third)->point().point()
		        + unitV * (cell1->info() - ed_it->first->vertex(ed_it->third)->point().point()) * unitV;
		Real halfCrackArea = crackAreaFactor * 0.5 * sqrt(std::abs(cross_product(CellCentre1 - p3, CellCentre2 - p3).squared_length())); //
		cell1->info().crackArea += halfCrackArea;
		cell2->info().crackArea += halfCrackArea;
		crackArea += halfCrackArea;
		crackVolume += halfCrackArea * aperture;
	}
}

void PartialSatClayEngine::circulateFacets(RTriangulation::Finite_edges_iterator& edge, Real aperture)
{
	const RTriangulation&            Tri    = solver->T[solver->currentTes].Triangulation();
	RTriangulation::Facet_circulator facet1 = Tri.incident_facets(*edge);
	RTriangulation::Facet_circulator facet0 = facet1++;
	computeFracturePerm(facet0, aperture, edge);
	while (facet1 != facet0) {
		computeFracturePerm(facet1, aperture, edge);
		facet1++;
	}
	/// Needs the fracture surface for this edge?
	// Real edgeArea = solver->T[solver->currentTes].computeVFacetArea(edge); cout<<"edge area="<<edgeArea<<endl;
}

void PartialSatClayEngine::trickPermeability(Solver* flow)
{
	leakOffRate               = 0;
	const RTriangulation& Tri = flow->T[solver->currentTes].Triangulation();
	//	if (!first) interpolateCrack(solver->T[solver->currentTes],flow->T[flow->currentTes]);

	const shared_ptr<InteractionContainer> interactions                         = scene->interactions;
	int                                    numberOfCrackedOrJointedInteractions = 0;
	Real                                   SumOfApertures                       = 0.;
	averageAperture                                                             = 0;
	maxAperture                                                                 = 0;
	crackArea                                                                   = 0;
	crackVolume                                                                 = 0;
	//Real totalFractureArea=0; /// Trying to get fracture's surface
	// 	const shared_ptr<IGeom>& ig;
	// 	const ScGeom* geom; // = static_cast<ScGeom*>(ig.get());
	FiniteEdgesIterator edge = Tri.finite_edges_begin();
	for (; edge != Tri.finite_edges_end(); ++edge) {
		const VertexInfo&              vi1         = (edge->first)->vertex(edge->second)->info();
		const VertexInfo&              vi2         = (edge->first)->vertex(edge->third)->info();
		const shared_ptr<Interaction>& interaction = interactions->find(vi1.id(), vi2.id());

		if (interaction && interaction->isReal()) {
			if (edge->first->info().isFictious or edge->first->info().isAlpha)
				continue; /// avoid trick permeability for fictitious

			if (displacementBasedCracks) {
				//const shared_ptr<Clump> clump=YADE_PTR_CAST<Clump>(clumpBody->shape);
				auto mindlingeom = YADE_PTR_CAST<ScGeom>(interaction->geom);
				//ScGeom* mindlingeom = YADE_CAST<ScGeom*>(interaction->geom.get());
				auto mindlinphys = YADE_PTR_CAST<MindlinPhys>(interaction->phys);
				//MindlinPhys* mindlinphys = YADE_CAST<MindlinPhys*>(interaction->phys.get());
				Real crackAperture = mindlingeom->penetrationDepth - mindlinphys->initD; // if negative, it has opened up
				//if ( crackAperture < 0 ) std::cout << "-crackAp" << -mindlingeom->penetrationDepth << std::endl;
				// shared_ptr< ScGeom > mindlingeom = std::dynamic_pointer_cast< ScGeom >(std::make_shared(interaction->geom.get()));
				if (-crackAperture < residualAperture and !mindlinphys->isBroken)
					continue;
				mindlinphys->isBroken
				        = true; //now even if the displacement reduces back below residAp, we keep tricking this edge in the future
				circulateFacets(edge, -crackAperture);

			} else {
				cout << "cohfrict phys partial sat integration not enabled in this version" << endl;
				return;
				// CohFrictPhys* cohfrictphys = YADE_CAST<CohFrictPhys*>(interaction->phys.get());
				// //shared_ptr< CohFrictPhys > cohfrictphys = std::dynamic_pointer_cast< CohFrictPhys >(std::make_shared(interaction->phys.get()));
				//
				// if (!cohfrictphys->isBroken) continue;
				// Real aperture = (cohfrictphys->crackAperture <= residualAperture)? residualAperture : cohfrictphys->crackAperture;
				// if (aperture > maxAperture) maxAperture = aperture;
				// SumOfApertures += aperture;
				// circulateFacets(edge,aperture);
			}
		}
	}
	averageAperture = SumOfApertures / numberOfCrackedOrJointedInteractions; /// DEBUG
	// 	cout << " Average aperture in joint ( -D ) = " << AverageAperture << endl; /// DEBUG
}

void PartialSatClayEngine::determineFracturePaths()
{
	RTriangulation&     tri     = solver->T[solver->currentTes].Triangulation();
	FiniteCellsIterator cellEnd = tri.finite_cells_end();
	for (FiniteCellsIterator cell = tri.finite_cells_begin(); cell != cellEnd; cell++) {
		if (cell->info().Pcondition)
			continue;
		cell->info().isExposed = false;
	}
	totalCracks = 0;
	// add logic for handling alpha cells
	if (alphaBound >= 0) {
		for (FlowSolver::VCellIterator it = solver->alphaBoundingCells.begin(); it != solver->alphaBoundingCells.end(); it++) {
			if ((*it) == NULL)
				continue;
			// exposureRecursion(*it); FIXME: add the correct bndPressure argument for alpha shape
		}
	} else {
		for (int i = 0; i < 6; i++) {
			for (FlowSolver::VCellIterator it = solver->boundingCells[i].begin(); it != solver->boundingCells[i].end(); it++) {
				if ((*it) == NULL)
					continue;
				Real bndPressure = solver->boundary(wallIds[i]).value;
				exposureRecursion(*it, bndPressure);
			}
		}
	}
}

void PartialSatClayEngine::exposureRecursion(CellHandle cell, Real bndPressure)
{
	for (int facet = 0; facet < 4; facet++) {
		CellHandle nCell = cell->neighbor(facet);
		if (solver->T[solver->currentTes].Triangulation().is_infinite(nCell))
			continue;
		if (nCell->info().Pcondition)
			continue;
		//         if ( (nCell->info().isFictious) && (!isInvadeBoundary) ) continue;
		if (!nCell->info().crack)
			continue;
		if (nCell->info().isExposed)
			continue; // another recursion already found it

		if (cell->info().crackNum == 0)
			nCell->info().crackNum = ++totalCracks; // enable visualization of discretely connected cracks
		else
			nCell->info().crackNum = cell->info().crackNum;

		nCell->info().isExposed = true;
		//imposePressureFromId(nCell->info().id,bndPressure); // make this a boundary condition now
		nCell->info().Pcondition = true;
		nCell->info().p()        = bndPressure;

		exposureRecursion(nCell, bndPressure);
	}
}


Body::id_t PartialSatClayEngine::clump(vector<Body::id_t> ids, unsigned int discretization)
{
	// create and add clump itself
	//Scene*            scene(Omega::instance().getScene().get());
	shared_ptr<Body>  clumpBody = shared_ptr<Body>(new Body());
	shared_ptr<Clump> clump     = shared_ptr<Clump>(new Clump());
	clumpBody->shape            = clump;
	clumpBody->setBounded(false);
	scene->bodies->insert(clumpBody);
	// add clump members to the clump
	FOREACH(Body::id_t id, ids)
	{
		if (Body::byId(id, scene)->isClumpMember()) {                                                 //Check, whether the body is clumpMember
			Clump::del(Body::byId(Body::byId(id, scene)->clumpId, scene), Body::byId(id, scene)); //If so, remove it from there
		}
	};

	FOREACH(Body::id_t id, ids) Clump::add(clumpBody, Body::byId(id, scene));
	Clump::updateProperties(clumpBody, discretization);
	return clumpBody->getId();
}


bool PartialSatClayEngine::findInscribedRadiusAndLocation(CellHandle& cell, std::vector<Real>& coordAndRad)
{
	//cout << "using least sq to find inscribed radius " << endl;
	const Real      prec = 1e-5;
	Eigen::MatrixXd A(4, 3);
	Eigen::Vector4d b;
	Eigen::Vector3d x;
	Eigen::Vector4d r;
	//std::vector<Real> r(4);
	//std::vector<Real> coordAndRad(4);
	CVector baryCenter(0, 0, 0); // use cell barycenter as initial guess
	for (int k = 0; k < 4; k++) {
		baryCenter = baryCenter + 0.25 * (cell->vertex(k)->point().point() - CGAL::ORIGIN);
		if (cell->vertex(k)->info().isFictious)
			return 0;
	}
	Real xo, yo, zo;
	int  count = 0;
	Real rMean;
	xo            = baryCenter[0];
	yo            = baryCenter[1];
	zo            = baryCenter[2];
	bool finished = false;
	while (finished == false) {
		count += 1;
		if (count > 1000) {
			cerr << "too many iterations during sphere inscription, quitting" << endl;
			return 0;
		}
		// build A matrix (and part of b)
		for (int i = 0; i < 4; i++) {
			Real xi, yi, zi;
			xi               = cell->vertex(i)->point().x();
			yi               = cell->vertex(i)->point().y();
			zi               = cell->vertex(i)->point().z();
			A(i, 0)          = xo - cell->vertex(i)->point().x();
			A(i, 1)          = yo - cell->vertex(i)->point().y();
			A(i, 2)          = zo - cell->vertex(i)->point().z();
			const Real sqrdD = pow(xo - xi, 2) + pow(yo - yi, 2) + pow(zo - zi, 2);
			r(i)             = sqrt(sqrdD) - sqrt(cell->vertex(i)->point().weight());
		}
		rMean = r.sum() / 4.;

		// build b
		for (int i = 0; i < 4; i++) {
			Real xi, yi, zi;
			xi   = cell->vertex(i)->point().x();
			yi   = cell->vertex(i)->point().y();
			zi   = cell->vertex(i)->point().z();
			b(i) = (pow(rMean + sqrt(cell->vertex(i)->point().weight()), 2.) - (pow(xo - xi, 2.) + pow(yo - yi, 2.) + pow(zo - zi, 2.))) / 2.;
		}

		// use least squares (normal equation) to minimize residuals
		x = (A.transpose() * A).ldlt().solve(A.transpose() * b);
		// if the values are greater than precision, update the guess and repeat
		if (abs(x(0)) > prec || abs(x(1)) > prec || abs(x(2)) > prec) {
			xo += x(0);
			yo += x(1);
			zo += x(2);
		} else {
			coordAndRad[0] = xo + x(0);
			coordAndRad[1] = yo + x(1);
			coordAndRad[2] = zo + x(2);
			coordAndRad[3] = rMean;
			if (rMean > sqrt(cell->vertex(0)->point().weight()))
				return 0; // inscribed sphere might be excessively large if it is in a flat boundary cell
			finished = true;
		}

	} // end while finished == false

	return 1;
}

void PartialSatClayEngine::insertMicroPores(const Real fracMicroPore)
{
	cout << "Inserting micro pores in " << fracMicroPore << " perc. of existing pores " << endl;
	Eigen::MatrixXd M(6, 6);
	//if (!solver->T[solver->currentTes]){cerr << "No triangulation, not inserting micropores" << endl; return}
	Tesselation& Tes = solver->T[solver->currentTes];
	//cout << "Tes set" << endl;
	const long        size = Tes.cellHandles.size();
	std::vector<bool> visited(size);
	std::vector<int>  poreIndices(int(ceil(fracMicroPore * size)));
	bool              numFound;
// randomly select the pore indices that we will turn into micro pores
#pragma omp parallel for
	for (unsigned int i = 0; i < poreIndices.size(); i++) {
		numFound = false;
		while (!numFound) {
			const long num = rand() % size; // + 1?
			if (!visited[num] && !Tes.cellHandles[num]->info().isFictious) {
				visited[num]   = true;
				poreIndices[i] = num;
				numFound       = true;
			}
		}
	}
	cout << "find inscribed sphere radius" << endl;

	// find inscribed sphere radius in selected pores and add body
	// FIXME How do we deal with inscribed spheres that might be overlapping after inscription?
	std::vector<Real> coordsAndRad(4);
	//#pragma omp parallel for
	for (unsigned int i = 0; i < poreIndices.size(); i++) {
		const int   idx  = poreIndices[i];
		CellHandle& cell = Tes.cellHandles[idx];
		for (int j = 0; j < 4; j++)
			if (cell->neighbor(j)->info().isFictious)
				continue; // avoid inscribing spheres in very flat exterior cells (FIXME: we can avoid this by using a proper alpha shape)
		//if (cell->info().Pcondition) continue;
		bool inscribed = findInscribedRadiusAndLocation(cell, coordsAndRad);
		if (!inscribed)
			continue; // sphere couldn't be inscribed, continue loop
		bool contained = checkSphereContainedInTet(cell, coordsAndRad);
		if (!contained)
			continue;
		//cout << "converting to Vector3r" << endl;
		Vector3r coords;
		coords[0]         = Real(coordsAndRad[0]);
		coords[1]         = Real(coordsAndRad[1]);
		coords[2]         = Real(coordsAndRad[2]);
		const Real radius = coordsAndRad[3];
		//cout << "adding body" << endl;
		shared_ptr<Body> body;
		createSphere(body, coords, radius);
		scene->bodies->insert(body);
	}
}

//bool PartialSatClayEngine::checkSphereContainedInTet(CellHandle& cell,std::vector<Real>& coordsAndRad)
//{
//	Eigen::Vector3d inscSphere(coordsAndRad[0],coordsAndRad[1],coordsAndRad[2]);
//	Eigen::Vector3d cellLoc(cell->info()[0],cell->info()[1],cell->info()[2]);
//	Real radius = coordsAndRad[3];
//	 for ( int i=0; i<4; i++ ) {
//		Eigen::Vector3d neighborCellLoc(cell->neighbor(i)->info()[0],cell->neighbor(i)->info()[1],cell->neighbor(i)->info()[2]);
//		Eigen::Vector3d vertexLoc(cell->vertex(facetVertices[i][0])->point().x(),cell->vertex(facetVertices[i][0])->point().y(),cell->vertex(facetVertices[i][0])->point().z());

//		Eigen::Vector3d Surfk = cellLoc-neighborCellLoc;
//		Eigen::Vector3d SurfkNormed = Surfk.normalized();
//		Eigen::Vector3d branch = vertexLoc - inscSphere;
//		Real distToFacet = branch.dot(SurfkNormed);
//		if (distToFacet<0){
//			cerr<< "sphere center outside tet, skipping insertion"<<endl;
//			return 0;
//		} else if (distToFacet<radius) {
//			cerr << "inscribed sphere too large for tetrahedral, decreasing size from "<< radius <<" to "<<distToFacet<<endl;
//			coordsAndRad[3] = distToFacet;
//			radius = distToFacet;
//		}
//	}
//	return 1;
//}

bool PartialSatClayEngine::checkSphereContainedInTet(CellHandle& cell, std::vector<Real>& coordsAndRad)
{
	Eigen::Vector3d inscSphere(coordsAndRad[0], coordsAndRad[1], coordsAndRad[2]);
	//const Real origRadius = coordsAndRad[3];
	//Eigen::Vector3d neighborCellLoc(cell->neighbor(i)->info()[0],cell->neighbor(i)->info()[1],cell->neighbor(i)->info()[2]);
	//	Eigen::MatrixXd A(3,4);
	//	Eigen::Vector4d x;
	//	Eigen::Vector3d bvec(0,0,0);
	//	Real a,b,c,d;
	Real radius = coordsAndRad[3];
	for (int i = 0; i < 4; i++) {
		// using same logic as above but more explicit
		Eigen::Vector3d nhat(cell->info().facetSurfaces[i][0], cell->info().facetSurfaces[i][1], cell->info().facetSurfaces[i][2]);
		nhat = nhat / sqrt(cell->info().facetSurfaces[i].squared_length());
		Eigen::Vector3d xi(
		        cell->vertex(facetVertices[i][0])->point().x(),
		        cell->vertex(facetVertices[i][0])->point().y(),
		        cell->vertex(facetVertices[i][0])->point().z());
		Real distToFacet  = nhat.dot(inscSphere - xi);
		Real exampleScale = sqrt(cell->vertex(facetVertices[i][0])->point().weight());
		Real scale        = exampleScale * minMicroRadFrac;
		// even more explicit, creating plane out of 3 verticies!
		//		for (int j=0;j<3;j++){
		//			A(j,0) = cell->vertex(facetVertices[i][j])->point().x();
		//			A(j,1) = cell->vertex(facetVertices[i][j])->point().y();
		//			A(j,2) = cell->vertex(facetVertices[i][j])->point().z();
		//			A(j,3) = 1;
		//		}
		if (!(distToFacet >= scale)) {
			cout << "minimum radius size doesn't fit,in tet skipping" << endl;
			return 0;
		}
		//		x = A.colPivHouseholderQr().solve(bvec);
		//		a=x(0);b=x(1);c=x(2);d=x(3);
		//		Real sqrtSum = sqrt(a*a+b*b+c*c);
		//		Real distToFacet = (a*coordsAndRad[0]+b*coordsAndRad[1]+c*coordsAndRad[2]+d)/sqrtSum;

		if (distToFacet < 0) {
			cerr << "sphere center outside tet, skipping insertion" << endl;
			return 0;
		} else if (distToFacet < radius) {
			cerr << "inscribed sphere too large for tetrahedral, decreasing size from " << radius << " to " << distToFacet << endl;
			coordsAndRad[3] = distToFacet; //*(1.-minMicroRadFrac);
			radius          = distToFacet; //*(1.-minMicroRadFrac);
		}                                      //else {
		//cerr << "inscribed sphere too small, skipping insertion, btw rad*minMicro= " << exampleScale*minMicroRadFrac << " while dist to facet = " << distToFacet << " and the logic " << (distToFacet>=scale) << endl;
		//	return 0;
		//}
	}
	return 1;
}

void PartialSatClayEngine::createSphere(shared_ptr<Body>& body, Vector3r position, Real radius)
{
	body                     = shared_ptr<Body>(new Body);
	body->groupMask          = 2;
	PartialSatState*   state = dynamic_cast<PartialSatState*>(body->state.get());
	shared_ptr<Aabb>   aabb(new Aabb);
	shared_ptr<Sphere> iSphere(new Sphere);
	state->blockedDOFs = State::DOF_NONE;
	const Real volume  = 4. / 3. * M_PI * pow(radius, 3.);
	state->mass        = volume * microStructureRho;
	//body->state->inertia	= Vector3r(2.0/5.0*body->state->mass*radius*radius,
	//			2.0/5.0*body->state->mass*radius*radius,
	//			2.0/5.0*body->state->mass*radius*radius);
	state->pos            = position;
	state->volumeOriginal = volume;
	state->radiiOriginal  = radius;
	shared_ptr<CohFrictMat> mat(new CohFrictMat);
	mat->young          = microStructureE;
	mat->poisson        = microStructureNu;
	mat->frictionAngle  = microStructurePhi * Mathr::PI / 180.0; //compactionFrictionDeg * Mathr::PI/180.0;
	mat->normalCohesion = mat->shearCohesion = microStructureAdh;
	aabb->color                              = Vector3r(0, 1, 0);
	iSphere->radius                          = radius;
	//iSphere->color	= Vector3r(0.4,0.1,0.1);
	//iSphere->color           = Vector3r(math::unitRandom(),math::unitRandom(),math::unitRandom());
	//iSphere->color.normalize();
	body->shape    = iSphere;
	body->bound    = aabb;
	body->material = mat;
}

void PartialSatClayEngine::printPorosityToFile(string file)
{
	RTriangulation&     tri     = solver->T[solver->currentTes].Triangulation();
	FiniteCellsIterator cellEnd = tri.finite_cells_end();
	ofstream            myfile;
	myfile.open(file + boost::lexical_cast<string>(scene->iter) + ".txt");
	for (FiniteCellsIterator cell = tri.finite_cells_begin(); cell != cellEnd; cell++) {
		myfile << cell->info().id << " " << cell->info().porosity << " " << cell->info().crack << "\n";
	}
	myfile.close();
}

void PartialSatClayEngine::simulateConfinement() // TODO: needs to be updated for alpha boundary usage
{
	RTriangulation&                  Tri    = solver->T[solver->currentTes].Triangulation();
	const shared_ptr<BodyContainer>& bodies = scene->bodies;
	for (int bound = 0; bound < 6; bound++) {
		int& id = *solver->boundsIds[bound];
		//solver->conductionBoundingCells[bound].clear();
		if (id < 0)
			continue;

		VectorCell tmpCells;
		tmpCells.resize(10000);
		VCellIterator cells_it  = tmpCells.begin();
		VCellIterator cells_end = Tri.incident_cells(solver->T[solver->currentTes].vertexHandles[id], cells_it);

		for (VCellIterator it = tmpCells.begin(); it != cells_end; it++) {
			CellHandle& cell = *it;
			for (int v = 0; v < 4; v++) {
				if (!cell->vertex(v)->info().isFictious) {
					const long int          id = cell->vertex(v)->info().id();
					const shared_ptr<Body>& b  = (*bodies)[id];
					if (b->shape->getClassIndex() != Sphere::getClassIndexStatic() || !b)
						continue;
					//auto* state = b->state.get();
					b->setDynamic(false);
				}
			}
		}
	}
	forceConfinement = false;
}

void PartialSatClayEngine::computeVertexSphericalArea() // TODO: update for alpha boundary
{
	Tesselation& Tes = solver->T[solver->currentTes];
	//	#ifdef YADE_OPENMP
	const long size = Tes.cellHandles.size();
#pragma omp parallel for
	for (long i = 0; i < size; i++) {
		CellHandle& cell = Tes.cellHandles[i];
		//	#else
		if (cell->info().blocked) //(cell->info().isFictious) ||
			continue;

		VertexHandle W[4];
		for (int k = 0; k < 4; k++)
			W[k] = cell->vertex(k);
		if (cell->vertex(0)->info().isFictious)
			cell->info().sphericalVertexSurface[0] = 0;
		else
			cell->info().sphericalVertexSurface[0]
			        = solver->fastSphericalTriangleArea(W[0]->point(), W[1]->point().point(), W[2]->point().point(), W[3]->point().point());
		if (cell->vertex(1)->info().isFictious)
			cell->info().sphericalVertexSurface[1] = 0;
		else
			cell->info().sphericalVertexSurface[1]
			        = solver->fastSphericalTriangleArea(W[1]->point(), W[0]->point().point(), W[2]->point().point(), W[3]->point().point());
		if (cell->vertex(2)->info().isFictious)
			cell->info().sphericalVertexSurface[2] = 0;
		else
			cell->info().sphericalVertexSurface[2]
			        = solver->fastSphericalTriangleArea(W[2]->point(), W[1]->point().point(), W[0]->point().point(), W[3]->point().point());
		if (cell->vertex(3)->info().isFictious)
			cell->info().sphericalVertexSurface[3] = 0;
		else
			cell->info().sphericalVertexSurface[3]
			        = solver->fastSphericalTriangleArea(W[3]->point(), W[1]->point().point(), W[2]->point().point(), W[0]->point().point());
	}
	solver->sphericalVertexAreaCalculated = true;
}



/// UNUSED EXTRAS ////


void PartialSatClayEngine::interpolateCrack(Tesselation& Tes, Tesselation& NewTes)
{
	RTriangulation& Tri = Tes.Triangulation();
//RTriangulation& newTri = NewTes.Triangulation();
//FiniteCellsIterator cellEnd = newTri.finite_cells_end();
#ifdef YADE_OPENMP
	const long size = NewTes.cellHandles.size();
#pragma omp parallel for num_threads(ompThreads > 0 ? ompThreads : 1)
	for (long i = 0; i < size; i++) {
		CellHandle& newCell = NewTes.cellHandles[i];
#else
	FOREACH(CellHandle & newCell, NewTes.cellHandles)
	{
#endif
		if (newCell->info().isGhost or newCell->info().isAlpha)
			continue;
		CVector center(0, 0, 0);
		if (newCell->info().fictious() == 0)
			for (int k = 0; k < 4; k++)
				center = center + 0.25 * (Tes.vertex(newCell->vertex(k)->info().id())->point().point() - CGAL::ORIGIN);
		else {
			Real boundPos = 0;
			int  coord    = 0;
			for (int k = 0; k < 4; k++) {
				if (!newCell->vertex(k)->info().isFictious)
					center = center
					        + (1. / (4. - newCell->info().fictious()))
					                * (Tes.vertex(newCell->vertex(k)->info().id())->point().point() - CGAL::ORIGIN);
			}
			for (int k = 0; k < 4; k++) {
				if (newCell->vertex(k)->info().isFictious) {
					coord    = solver->boundary(newCell->vertex(k)->info().id()).coordinate;
					boundPos = solver->boundary(newCell->vertex(k)->info().id()).p[coord];
					center   = CVector(
                                                coord == 0 ? boundPos : center[0], coord == 1 ? boundPos : center[1], coord == 2 ? boundPos : center[2]);
				}
			}
		}
		CellHandle oldCell    = Tri.locate(CGT::Sphere(center[0], center[1], center[2]));
		newCell->info().crack = oldCell->info().crack;
		//		For later commit newCell->info().fractureTip = oldCell->info().fractureTip;
		//		For later commit newCell->info().cellHalfWidth = oldCell->info().cellHalfWidth;

		/// compute leakoff rate by summing the flow through facets abutting non-cracked neighbors
		//		if (oldCell->info().crack && !oldCell->info().fictious()){
		//			Real facetFlowRate=0;
		//			facetFlowRate -= oldCell->info().dv();
		//			for (int k=0; k<4;k++) {
		//				if (!oldCell->neighbor(k)->info().crack){
		//					facetFlowRate = oldCell->info().kNorm()[k]*(oldCell->info().shiftedP()-oldCell->neighbor(k)->info().shiftedP());
		//					leakOffRate += facetFlowRate;
		//				}
		//			}
		//		}
	}
}

// void crackCellAbovePoroThreshold(CellHandle& cell)
// {
//         cell->info().crack = 1;
//         for (int j=0; j<4; j++){
//                 CellHandle& ncell = cell->neighbor(i);
//                 Real fracturePerm = apertureFactor*pow(residualAperture,3.)/(12.*viscosity);
//                  //nCell->info().crack=1;
//                 cell->info().kNorm()[i] += fracturePerm; //
//                 nCell->info().kNorm()[Tri.mirror_index(cell,i)] += fracturePerm;
//         }
// }

// void PartialSatClayEngine::trickPermOnCrackedCells(FlowSolver& flow)
// {
//         Tesselation& Tes = flow.T[flow.currentTes];
//         //	#ifdef YADE_OPENMP
//         const long size = Tes.cellHandles.size();
//         Real fracturePerm = apertureFactor*pow(residualAperture,3.)/(12.*viscosity);
//         //#pragma omp parallel for
//         //cout << "blocking low poro regions" << endl;
//         for (long i=0; i<size; i++){
//                 CellHandle& cell = Tes.cellHandles[i];
//                 if ( cell->info().initiallyCracked ){
//                         for (int j=0; j<4; j++){
//                                 const CellHandle& nCell = cell->neighbor(j);
//                                 if (!changeCrackSaturation or (changeCrackSaturation and cell->info().saturation>=SsM) or nCell->info().isFictious) {
//                                         cell->info().kNorm()[j] = fracturePerm;
//                                         nCell->info().kNorm()[Tes.Triangulation().mirror_index(cell,j)] = fracturePerm;
//                                 } else { // block cracked cell if it isnt saturated
//                                         cell->info().crack=1;
//                                         nCell->info().crack=1;
//                                         cell->info().blocked=1;
//                                         nCell->info().blocked=1;
//                                         cell->info().saturation=0;
//                                         nCell->info().saturation=0;
//                                 }
//                         }
//                 }
//         }
// }


// void PartialSatClayEngine::crackCellsAbovePoroThreshold(FlowSolver& flow)
// {
//         Tesselation& Tes = flow.T[flow.currentTes];
//         //	#ifdef YADE_OPENMP
//         const long size = Tes.cellHandles.size();
//         //#pragma omp parallel for
//         //cout << "blocking low poro regions" << endl;
//         for (long i=0; i<size; i++){
//                 CellHandle& cell = Tes.cellHandles[i];
//                 if ( ( first and cell->info().porosity > crackCellPoroThreshold ) or ( cell->info().initiallyCracked ) ){
//                         cell->info().crack = true; cell->info().initiallyCracked = true;
//                         Real fracturePerm = apertureFactor*pow(residualAperture,3.)/(12.*viscosity);
//
//                         for (int j=0; j<4; j++){
//                                 const CellHandle& nCell = cell->neighbor(j);
//                                 if (!changeCrackSaturation or (changeCrackSaturation and cell->info().saturation>=SsM) or nCell->info().isFictious) {
//                                         cell->info().kNorm()[j] = fracturePerm;
//                                         nCell->info().kNorm()[Tes.Triangulation().mirror_index(cell,j)] = fracturePerm;
//                                 } else { // block cracked cell if it isnt saturated
//                                         cell->info().crack=1;
//                                         nCell->info().crack=1;
//                                         cell->info().blocked=1;
//                                         nCell->info().blocked=1;
//                                         cell->info().saturation=0;
//                                         nCell->info().saturation=0;
//                                 }
//                         }
//                 }
//         }
// }


} //namespace yade
// clang-format on
#endif //PartialSat
