/*************************************************************************
*  Copyright (C) 2018 by Robert Caulk <rob.caulk@gmail.com>  		 *
*  Copyright (C) 2018 by Bruno Chareyre <bruno.chareyre@grenoble-inp.fr>     *
*									 *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/
/* This engine is under active development. Experimental only */
/* Thermal Engine was developed with funding provided by the Chateaubriand Fellowship */

/* Theoretical framework and experimental validation presented in:

Caulk, R. and Chareyre, B. (2019) An open framework for the simulation of thermal-hydraulic-mechanical processes in discrete element systems. Thermal Process Engineering: Proceedings of DEM8, Enschede Netherlands, July 2019.

*/

//#define THERMAL
#ifdef THERMAL
#ifdef YADE_OPENMP
#include <core/Omega.hpp>
#include <core/Scene.hpp>
#include <pkg/common/Sphere.hpp>
#include <pkg/dem/FrictPhys.hpp>
#include <pkg/dem/Shop.hpp>
#include <pkg/pfv/Thermal.hpp>
//YADE_PLUGIN((ThermalState));
//ThermalState::~ThermalState(){};

namespace yade { // Cannot have #include directive inside.

CREATE_LOGGER(ThermalEngine);
YADE_PLUGIN((ThermalEngine));

ThermalEngine::~ThermalEngine() { } // destructor

void ThermalEngine::action()
{
	scene = Omega::instance().getScene().get();
	if (first)
		setInitialValues();
	if (debug)
		cout << "initial values set" << endl;
	FOREACH(const shared_ptr<Engine> e, Omega::instance().getScene()->engines)
	{
		if (e->getClassName() == "FlowEngine") {
			flow = dynamic_cast<FlowEngineT*>(e.get());
		}
	}
	// some initialization stuff for timestep and efficiency.
	elapsedTime += scene->dt;
	if (elapsedIters >= conductionIterPeriod || first) {
		runConduction     = true;
		timeStepEstimated = false;
		if (first)
			thermalDT = scene->dt;
		else
			thermalDT = elapsedTime;
		if (advection && first) {
			Pr = flow->fluidCp * flow->viscosity / fluidK;
			setReynoldsNumbers();
		}
		first = false;
	} else if (tsSafetyFactor <= 0) {
		thermalDT = scene->dt;
	}
	if (flow->updateTriangulation) {
		setReynoldsNumbers();
		flow->updateTriangulation = false; // thermalFlipping this back for FlowEngine
	}
	if (thermoMech && letThermalRunFlowForceUpdates)
		flow->decoupleForces = true; // let thermal engine handle force estimates
	//	if ((elapsedIters == conductionIterPeriod) && tsSafetyFactor>0) flow->decoupleForces=true; // don't let flow handle forces during next step, since thermal will be adjusting pressures and handling forces // CURRENTLY NOT USEFUL
	resetBoundaryFluxSums();
	//resetStepFluxSums();
	if (!boundarySet)
		setConductionBoundary();
	if (!flowTempBoundarySet)
		resetFlowBoundaryTemps();
	if (debug)
		cout << "boundaries set" << endl;
	if (!conduction)
		thermoMech = false; //don't allow thermoMech if conduction is not activated
	if (advection) {
		flow->solver->initializeInternalEnergy(); // internal energy of cells
		flow->solver->augmentConductivityMatrix(scene->dt);
	}
	if (debug)
		cout << "advection done" << endl;
	if (conduction && runConduction) {
		computeSolidSolidFluxes();
		if (advection)
			computeSolidFluidFluxes();
	}
	if (unboundCavityBodies)
		unboundCavityParticles();
	if (advection && fluidConduction) { // need to avoid duplicating energy, so reinitializing pore energy before conduction
		flow->solver->setNewCellTemps(false);
		flow->solver->initializeInternalEnergy();
		computeFluidFluidConduction();
	}
	if (debug)
		cout << "conduction done" << endl;
	if (conduction && runConduction)
		computeNewParticleTemperatures();
	if (advection)
		flow->solver->setNewCellTemps(fluidConduction); // in case of fluid conduction, the delta temps are added to, not replaced
	if (debug)
		cout << "temps set" << endl;
	if (delT > 0 && runConduction)
		applyTempDeltaToSolids(delT);
	if (thermoMech && runConduction) {
		thermalExpansion();
		//		if (tsSafetyFactor>0){
		if (letThermalRunFlowForceUpdates)
			updateForces(); // currently not working. thermal must be run each step to see delp reflected
			                //			flow->decoupleForces=false;  // let flow take control back of forces
	}
	if (!timeStepEstimated && tsSafetyFactor > 0)
		timeStepEstimate();
	if (debug)
		cout << "timeStepEstimated " << timeStepEstimated << endl;
	if (tsSafetyFactor > 0)
		elapsedIters += 1;
}

void ThermalEngine::setReynoldsNumbers()
{
	Tesselation& Tes  = flow->solver->T[flow->solver->currentTes];
	const long   size = Tes.cellHandles.size();
	if (uniformReynolds != -1) { // gain efficiency if we know the flow is creep (assigning uniform low reynolds to all cells)
		Real NussfluidK = (2. + 0.6 * pow(uniformReynolds, 0.5) * pow(Pr, 0.33333)) * fluidK;
#pragma omp parallel for
		for (long i = 0; i < size; i++) {
			CellHandle& cell           = Tes.cellHandles[i];
			cell->info().Reynolds      = uniformReynolds;
			cell->info().NutimesFluidK = NussfluidK;
		}
		return;
	}

	// else get fluid velocity, compute reynolds, compute proper nusselt
	flow->solver->averageRelativeCellVelocity();
	//	#ifdef YADE_OPENMP
	const Real poro = Shop::getPorosityAlt();
#pragma omp parallel for
	for (long i = 0; i < size; i++) {
		CellHandle& cell = Tes.cellHandles[i];
		CVector     l;
		Real        charLength = 0.000001;
		//Real NutimesFluidK = 2*fluidK;
		Real Nusselt = 2.;
		for (int j = 0; j < 4; j++) {
			if (!cell->neighbor(j)->info().isFictious) {
				l          = cell->info() - cell->neighbor(j)->info();
				charLength = sqrt(l.squared_length());
			}
		}
		const Real avgCellFluidVel = sqrt(cell->info().averageVelocity().squared_length());
		double     Reynolds        = flow->solver->fluidRho * avgCellFluidVel * charLength / flow->viscosity;
		if (Reynolds < 0 || math::isnan(Reynolds)) {
			cerr << "Reynolds is negative or nan" << endl;
			Reynolds = 0;
		}
		if (Reynolds > 1000 || poro < 0.35) {
			Nusselt               = 2. + 0.6 * pow(Reynolds, 0.5) * pow(Pr, 0.33333);
			cell->info().Reynolds = Reynolds;
		} else if (
		        (0 <= Reynolds)
		        && (Reynolds
		            <= 1000)) { //Tavassoli, H., Peters, E.A.J.F., and Kuipers, J.A.M. (2015) Direct numerical simulation of fluid–particle heat transfer in fixed random arrays of non‐spherical particles. Chemical Engineering Science, 129, 42–48
			Nusselt = (7. - 10. * poro + 5. * poro * poro) * (1. + 0.1 * pow(Reynolds, 0.2) * pow(Pr, 0.333))
			        + (1.33 - 2.19 * poro + 1.15 * poro * poro) * pow(Reynolds, 0.7) * pow(Pr, 0.333);
			NutimesFluidK         = Nu * fluidK;
			cell->info().Reynolds = Reynolds;
		}
		cell->info().NutimesFluidK = Nusselt * fluidK;
	}
	return;
}

void ThermalEngine::setInitialValues()
{
	YADE_PARALLEL_FOREACH_BODY_BEGIN(const shared_ptr<Body>& b, scene->bodies)
	{
		if (b->shape->getClassIndex() != Sphere::getClassIndexStatic() || !b)
			continue;
		auto* state  = b->state.get();
		state->temp  = particleT0;
		state->k     = particleK;
		state->Cp    = particleCp;
		state->alpha = particleAlpha;
		if (advection)
			state->isCavity = true; // easiest to start by assuming cavity and flip if touching non-cavity
	}
	YADE_PARALLEL_FOREACH_BODY_END();
}

void ThermalEngine::timeStepEstimate()
{
	//	#pragma omp parallel for
	for (const auto& b : *scene->bodies) {
		if (b->shape->getClassIndex() != Sphere::getClassIndexStatic() || !b)
			continue;
		auto*      thState            = b->state.get();
		Sphere*    sphere             = dynamic_cast<Sphere*>(b->shape.get());
		const Real mass               = (particleDensity > 0 ? particleDensity * M_PI * pow(sphere->radius, 2) : thState->mass);
		const Real bodyTimeStep       = mass * thState->Cp / thState->stabilityCoefficient;
		thState->stabilityCoefficient = 0; // reset the stability coefficient
		if (!maxTimeStep)
			maxTimeStep = bodyTimeStep;
		if (bodyTimeStep < maxTimeStep)
			maxTimeStep = bodyTimeStep;
	}

	if (advection && fluidConduction) {
		Tesselation& Tes = flow->solver->T[flow->solver->currentTes];
		//	#ifdef YADE_OPENMP
		const long sizeCells = Tes.cellHandles.size();
		//	#pragma omp parallel for
		for (long i = 0; i < sizeCells; i++) {
			CellHandle& cell = Tes.cellHandles[i];
			Real        poreVolume;
			if (cell->info().isCavity)
				poreVolume = cell->info().volume();
			else if (porosityFactor > 0)
				poreVolume = cell->info().volume() * porosityFactor;
			else
				poreVolume = 1. / cell->info().invVoidVolume();
			const Real mass                   = flow->fluidRho * poreVolume;
			const Real poreTimeStep           = mass * flow->fluidCp / cell->info().stabilityCoefficient;
			cell->info().stabilityCoefficient = 0;
			if (!maxTimeStep)
				maxTimeStep = poreTimeStep;
			if (poreTimeStep < maxTimeStep)
				maxTimeStep = poreTimeStep;
		}
	}


	if (debug)
		cout << "body steps done" << endl;
	timeStepEstimated = true;
	// estimate the conduction iterperiod based on current mechanical/fluid timestep
	conductionIterPeriod = int(tsSafetyFactor * maxTimeStep / scene->dt);
	if (debug)
		cout << "conduction iter period set" << conductionIterPeriod << endl;
	elapsedIters      = 0;
	elapsedTime       = 0;
	timeStepEstimated = true;
	runConduction     = false;
}

void ThermalEngine::resetBoundaryFluxSums()
{
	for (int i = 0; i < 6; i++)
		thermalBndFlux[i] = 0;
}

void ThermalEngine::setConductionBoundary()
{
	for (int k = 0; k < 6; k++) {
		flow->solver->conductionBoundary(flow->wallIds[k]).fluxCondition = !bndCondIsTemperature[k];
		flow->solver->conductionBoundary(flow->wallIds[k]).value         = thermalBndCondValue[k];
	}

	RTriangulation&                  Tri    = flow->solver->T[flow->solver->currentTes].Triangulation();
	const shared_ptr<BodyContainer>& bodies = scene->bodies;
	for (int bound = 0; bound < 6; bound++) {
		int& id = *flow->solver->boundsIds[bound];
		flow->solver->conductionBoundingCells[bound].clear();
		if (id < 0)
			continue;
		CGT::ThermalBoundary& bi = flow->solver->conductionBoundary(id);

		if (!bi.fluxCondition) {
			VectorCell tmpCells;
			tmpCells.resize(10000);
			VCellIterator cells_it  = tmpCells.begin();
			VCellIterator cells_end = Tri.incident_cells(flow->solver->T[flow->solver->currentTes].vertexHandles[id], cells_it);

			for (VCellIterator it = tmpCells.begin(); it != cells_end; it++) {
				CellHandle& cell = *it;
				for (int v = 0; v < 4; v++) {
					if (!cell->vertex(v)->info().isFictious) {
						const long int id = cell->vertex(v)->info().id();
						if (!Body::byId(id))
							continue;
						const shared_ptr<Body>& b = (*bodies)[id];
						if (b->shape->getClassIndex() != Sphere::getClassIndexStatic() || !b)
							continue;
						auto* thState       = b->state.get();
						thState->Tcondition = true;
						thState->temp       = bi.value;
						thState->boundaryId = bound;
					}
				}
				flow->solver->conductionBoundingCells[bound].push_back(cell);
			}
		}
	}
	boundarySet = true;
}

// FIXME: buggy, commenting out for now
//void ThermalEngine::applyBoundaryHeatFluxes()
//{
//	RTriangulation&                  Tri    = flow->solver->T[flow->solver->currentTes].Triangulation();
//	const shared_ptr<BodyContainer>& bodies = scene->bodies;
//	for (int bound = 0; bound < 6; bound++) {
//		int& id = *flow->solver->boundsIds[bound];
//		flow->solver->conductionBoundingCells[bound].clear();
//		if (id < 0)
//			continue;
//		CGT::ThermalBoundary& bi = flow->solver->conductionBoundary(id);
//		if (bi.fluxCondition) {
//			VectorCell tmpCells;
//			tmpCells.resize(10000);
//			VCellIterator cells_it  = tmpCells.begin();
//			VCellIterator cells_end = Tri.incident_cells(flow->solver->T[flow->solver->currentTes].vertexHandles[id], cells_it);

//			for (VCellIterator it = tmpCells.begin(); it != cells_end; it++) {
//				CellHandle& cell = *it;
//				for (int v = 0; v < 4; v++) {
//					if (!cell->vertex(v)->info().isFictious) {
//						const long int          id = cell->vertex(v)->info().id();
//						const shared_ptr<Body>& b  = (*bodies)[id];
//						if (b->shape->getClassIndex() != Sphere::getClassIndexStatic() || !b)
//							continue;
//						auto* thState       = b->state.get();
//						thState->Tcondition = false;
//						thState->stepFlux += bi.value;
//						thState->boundaryId = bound;
//					}
//				}
//			}
//		}
//	}
//}

void ThermalEngine::computeSolidFluidFluxes()
{
	if (!flow->solver->sphericalVertexAreaCalculated)
		computeVertexSphericalArea();
	shared_ptr<BodyContainer>& bodies = scene->bodies;
	Tesselation&               Tes    = flow->solver->T[flow->solver->currentTes];
	//	#ifdef YADE_OPENMP
	const long size = Tes.cellHandles.size();
#pragma omp parallel for
	for (long i = 0; i < size; i++) {
		CellHandle& cell = Tes.cellHandles[i];
		//	#else
		if ((ignoreFictiousConduction && cell->info().isFictious) || cell->info().blocked)
			continue;
		for (int v = 0; v < 4; v++) {
			if (cell->vertex(v)->info().isFictious)
				continue;
			if (!cell->info().Tcondition && cell->info().isFictious)
				continue; // don't compute conduction with boundary cells that do not have a temperature assigned
			const long int id = cell->vertex(v)->info().id();
			if (!Body::byId(id))
				continue;
			const shared_ptr<Body>& b = (*bodies)[id];
			if (b->shape->getClassIndex() != Sphere::getClassIndexStatic() || !b)
				continue;
			auto* thState = b->state.get();
			if (!cell->info().isCavity
			    && unboundCavityBodies) { // nothing to do with fluxes, but the difficulty required to get here warrants tracking cavitybodies while we are here
				thState->isCavity = false;
			}
			if (!first && thState->isCavity)
				continue; // avoid heat transfer with placeholder cavity bodies
			const Real surfaceArea = cell->info().sphericalVertexSurface[v];
			computeFlux(cell, b, surfaceArea);
		}
	}
}


void ThermalEngine::unboundCavityParticles()
{ // maybe move to flowbounding sphere, but all tools are here atm
	YADE_PARALLEL_FOREACH_BODY_BEGIN(const shared_ptr<Body>& b, scene->bodies)
	{
		if (b->shape->getClassIndex() != Sphere::getClassIndexStatic() || !b)
			continue;
		auto* thState = b->state.get();
		if (thState->isCavity) {
			b->setBounded(false);
			if (debug)
				cout << "cavity body unbounded" << endl;
		}
	}
	YADE_PARALLEL_FOREACH_BODY_END();
}


void ThermalEngine::computeVertexSphericalArea()
{
	Tesselation& Tes = flow->solver->T[flow->solver->currentTes];
	//	#ifdef YADE_OPENMP
	const long size = Tes.cellHandles.size();
#pragma omp parallel for
	for (long i = 0; i < size; i++) {
		CellHandle& cell = Tes.cellHandles[i];
		//	#else
		if ((ignoreFictiousConduction && cell->info().isFictious) || cell->info().blocked)
			continue;

		VertexHandle W[4];
		for (int k = 0; k < 4; k++)
			W[k] = cell->vertex(k);
		if (cell->vertex(0)->info().isFictious)
			cell->info().sphericalVertexSurface[0] = 0;
		else
			cell->info().sphericalVertexSurface[0]
			        = flow->solver->fastSphericalTriangleArea(W[0]->point(), W[1]->point().point(), W[2]->point().point(), W[3]->point().point());
		if (cell->vertex(1)->info().isFictious)
			cell->info().sphericalVertexSurface[1] = 0;
		else
			cell->info().sphericalVertexSurface[1]
			        = flow->solver->fastSphericalTriangleArea(W[1]->point(), W[0]->point().point(), W[2]->point().point(), W[3]->point().point());
		if (cell->vertex(2)->info().isFictious)
			cell->info().sphericalVertexSurface[2] = 0;
		else
			cell->info().sphericalVertexSurface[2]
			        = flow->solver->fastSphericalTriangleArea(W[2]->point(), W[1]->point().point(), W[0]->point().point(), W[3]->point().point());
		if (cell->vertex(3)->info().isFictious)
			cell->info().sphericalVertexSurface[3] = 0;
		else
			cell->info().sphericalVertexSurface[3]
			        = flow->solver->fastSphericalTriangleArea(W[3]->point(), W[1]->point().point(), W[2]->point().point(), W[0]->point().point());
	}
	flow->solver->sphericalVertexAreaCalculated = true;
}


void ThermalEngine::computeFlux(CellHandle& cell, const shared_ptr<Body>& b, const Real surfaceArea)
{
	const Sphere* sphere  = dynamic_cast<Sphere*>(b->shape.get());
	auto*         thState = b->state.get();
	const Real    h       = cell->info().NutimesFluidK / (2. * sphere->radius);
	//const Real h = cell->info().NutimesFluidK / (2.*sphere->radius); // heat transfer coeff assuming Re<<1 (stokes flow)
	const Real flux = h * surfaceArea * (cell->info().temp() - thState->temp);
	if (runConduction && tsSafetyFactor > 0) {
		thState->stabilityCoefficient += h * surfaceArea; // for auto time step estimation
		cell->info().stabilityCoefficient += h * surfaceArea;
	}
	if (!cell->info().Tcondition && !cell->info().isFictious && !cell->info().blocked)
		cell->info().internalEnergy -= flux * thermalDT;
	if (!thState->Tcondition)
		thState->stepFlux += flux;
}


void ThermalEngine::computeSolidSolidFluxes()
{
	//	#ifdef YADE_OPENMP
	const shared_ptr<InteractionContainer>& interactions = scene->interactions;
	const long                              size         = interactions->size();
#pragma omp parallel for
	for (long i = 0; i < size; i++) {
		const shared_ptr<Interaction>& I = (*interactions)[i];
		//	#else
		//	for (const auto & I : *scene->interactions){
		//	#endif
		const ScGeom* geom;
		if (!I || !I->geom.get() || !I->phys.get() || !I->isReal())
			continue;
		if (I->geom.get()) {
			geom = YADE_CAST<ScGeom*>(I->geom.get());
			if (!geom)
				continue;
			const Real pd = geom->penetrationDepth;
			if (!Body::byId(I->getId1(), scene) or !Body::byId(I->getId2(), scene))
				continue;
			const shared_ptr<Body>& b1_ = Body::byId(I->getId1(), scene);
			const shared_ptr<Body>& b2_ = Body::byId(I->getId2(), scene);
			if (b1_->shape->getClassIndex() != Sphere::getClassIndexStatic() || b2_->shape->getClassIndex() != Sphere::getClassIndexStatic() || !b1_
			    || !b2_)
				continue;
			auto*      thState1 = b1_->state.get();
			auto*      thState2 = b2_->state.get();
			FrictPhys* phys     = static_cast<FrictPhys*>(I->phys.get());
			if (!first && (thState1->isCavity || thState2->isCavity))
				continue; // avoid conduction with placeholder cavity bodies
			Sphere* sphere1 = dynamic_cast<Sphere*>(b1_->shape.get());
			Sphere* sphere2 = dynamic_cast<Sphere*>(b2_->shape.get());

			FrictMat*  mat1 = dynamic_cast<FrictMat*>(b1_->material.get());
			FrictMat*  mat2 = dynamic_cast<FrictMat*>(b2_->material.get());
			const Real k1   = thState1->k;
			const Real k2   = thState2->k;
			const Real r1   = sphere1->radius;
			const Real r2   = sphere2->radius;
			const Real T1   = thState1->temp;
			const Real T2   = thState2->temp;
			const Real d    = r1 + r2 - pd;
			const Real E1   = mat1->young;
			const Real E2   = mat1->young;
			const Real nu1  = mat1->poisson;
			const Real nu2  = mat2->poisson;
			const Real F    = phys->normalForce.squaredNorm();
			if (d == 0)
				continue;
			Real R = 0;
			Real r = 0;
			// for equation:
			if (r1 >= r2) {
				R = r1;
				r = r2;
			} else if (r1 < r2) {
				R = r2;
				r = r1;
			}
			// The radius of the intersection found by: Kern, W. F. and Bland, J. R. Solid Mensuration with Proofs, 2nd ed. New York: Wiley, p. 97, 1948.	http://mathworld.wolfram.com/Sphere-SphereIntersection.html
			//const Real area = M_PI*pow(rc,2);

			//const Real dt = scene->dt;
			//const Real fluxij = 4.*rc*(T1-T2) / (1./k1 + 1./k2);

			// compute the overlapping volume for thermodynamic considerations
			//		const Real capHeight1 = (r1-r2+d)*(r1+r2-d)/2*d;
			//		const Real capHeight2 = (r2-r1+d)*(r2+r1-d)/2*d;
			//		thState1->capVol += (1./3.)*M_PI*pow(capHeight1,2)*(3.*r1-capHeight1);
			//		thState2->capVol += (1./3.)*M_PI*pow(capHeight2,2)*(3.*r2-capHeight2);

			// compute the thermal resistance of the pair and the associated flux
			Real thermalResist;
			if (useKernMethod) {
				const Real numerator = pow((-d + r - R) * (-d - r + R) * (-d + r + R) * (d + r + R), 0.5);
				const Real rc        = numerator / (2. * d);
				//thermalResist = 4.*rc / (1./k1 + 1./k2);
				thermalResist = 2. * (k1 + k2) * rc * rc / (r1 + r2 - pd);
			} //thermalResist = ((k1+k2)/2.)*area/(r1+r2-pd);}
			else if (useHertzMethod) {
				const Real re    = 1. / r1 + 1. / r2;
				const Real Eavg  = (E1 + E2) / 2.;
				const Real Nuavg = (nu1 + nu2) / 2.;
				const Real Estar = Eavg / (1. - pow(Nuavg, 2));
				const Real a     = pow(3. * F * re / (4. * Estar), 1. / 3.);
				thermalResist    = ((k1 + k2) / 2.) / (r1 + r2) * M_PI * pow(a, 2);
			} else {
				thermalResist = 2. * (k1 + k2) * r1 * r2 / (r1 + r2 - pd);
			}
			const Real fluxij = thermalResist * (T1 - T2);

			//cout << "Flux b/w "<< b1_->id << " & "<< b2_->id << " fluxij " << fluxij << endl;
			if (runConduction && tsSafetyFactor > 0) {
				thState1->stabilityCoefficient += thermalResist;
				thState2->stabilityCoefficient += thermalResist;
			}
			if (!thState1->Tcondition)
				thState1->stepFlux -= fluxij; //U1 -= fluxij*dt;
			else
				thermalBndFlux[thState1->boundaryId] -= fluxij;
			if (!thState2->Tcondition)
				thState2->stepFlux += fluxij; // U2 += fluxij*dt;
			else
				thermalBndFlux[thState2->boundaryId] += fluxij;
		}
	}
}

void ThermalEngine::computeFluidFluidConduction()
{
	//CVector fluidSurfK; CVector poreVector; Real distance; Real area; Real conductionEnergy;
	// cycle through facets instead of cells to avoid duplicate math

	/* Parallel version */
	Tesselation& Tes        = flow->solver->T[flow->solver->currentTes];
	const long   sizeFacets = Tes.facetCells.size();
	//	#pragma omp parallel for  //FIXME: does not like running in parallel for some reason
	for (long i = 0; i < sizeFacets; i++) {
		CVector                    fluidSurfK;
		CVector                    poreVector;
		Real                       distance;
		Real                       area;
		Real                       conductionEnergy;
		std::pair<CellHandle, int> facetPair    = Tes.facetCells[i];
		const CellHandle&          cell         = facetPair.first;
		const CellHandle&          neighborCell = cell->neighbor(facetPair.second);
		if (cell->info().isFictious || neighborCell->info().isFictious || cell->info().blocked || neighborCell->info().blocked)
			continue;
		delT = cell->info().temp() - neighborCell->info().temp();
		Real fluidToSolidRatio;
		if (cell->info().isCavity && neighborCell->info().isCavity)
			fluidToSolidRatio = 1.;
		else
			fluidToSolidRatio = cell->info().facetFluidSurfacesRatio[facetPair.second];
		//if (flow->thermalPorosity>0) fluidConductionAreaFactor=flow->thermalPorosity;
		area = fluidConductionAreaFactor * sqrt(cell->info().facetSurfaces[facetPair.second].squared_length()) * fluidToSolidRatio;
		//area = sqrt(fluidSurfK.squared_length());
		//poreVector = cell->info() - neighborCell->info();
		poreVector = cellBarycenter(cell) - cellBarycenter(neighborCell); // voronoi was breaking for hexagonal packings
		distance   = sqrt(poreVector.squared_length());
		if (distance < minimumFluidCondDist)
			distance = minimumFluidCondDist;
		//cout << "conduction distance" << distance << endl;
		//if (distance < area) continue;  // hexagonal packings result in extremely small distances that blow up the simulation
		const Real thermalResist = fluidK * area / distance;
		conductionEnergy         = thermalResist * delT * thermalDT;
		if (math::isnan(conductionEnergy))
			conductionEnergy = 0;
		cell->info().stabilityCoefficient += thermalResist;
		//cout << "conduction distance" << distance << endl;
		if (!cell->info().Tcondition && !math::isnan(conductionEnergy))
			cell->info().internalEnergy -= conductionEnergy;
		if (!neighborCell->info().Tcondition && !math::isnan(conductionEnergy))
			neighborCell->info().internalEnergy += conductionEnergy;
		//cout << "added conduction energy"<< conductionEnergy << endl;
	}

	/* non parallel version */
	//	RTriangulation& Tri = flow->solver->T[flow->solver->currentTes].Triangulation();
	//	for (FiniteFacetsIterator f_it=Tri.finite_facets_begin(); f_it != Tri.finite_facets_end();f_it++){
	//		const CellHandle& cell = f_it->first;
	//		const CellHandle& neighborCell = f_it->first->neighbor(f_it->second);
	//		if (cell->info().isFictious || neighborCell->info().isFictious || cell->info().blocked || neighborCell->info().blocked) continue;
	//		delT = cell->info().temp() - neighborCell->info().temp();
	//		Real fluidToSolidRatio;
	//		if (cell->info().isCavity && neighborCell->info().isCavity) fluidToSolidRatio = 1.;
	//		else fluidToSolidRatio = cell->info().facetFluidSurfacesRatio[f_it->second];
	//		area = fluidConductionAreaFactor * math::sqrt(cell->info().facetSurfaces[f_it->second].squared_length())*fluidToSolidRatio;
	//		//area = math::sqrt(fluidSurfK.squared_length());
	//		//poreVector = cell->info() - neighborCell->info();
	//		poreVector = cellBarycenter(cell) - cellBarycenter(neighborCell);  // voronoi was breaking for hexagonal packings
	//		distance = math::sqrt(poreVector.squared_length());
	//		//cout << "conduction distance" << distance << endl;
	//		//if (distance < area) continue;  // hexagonal packings result in extremely small distances that blow up the simulation
	//		const Real thermalResist = fluidK*area/distance;
	//		conductionEnergy = thermalResist * delT * thermalDT;
	//		if (isnan(conductionEnergy)) conductionEnergy=0;
	//		cell->info().stabilityCoefficient+=thermalResist;
	//		//cout << "conduction distance" << distance << endl;
	//		if (!cell->info().Tcondition && !isnan(conductionEnergy)) cell->info().internalEnergy -= conductionEnergy;
	//		if (!neighborCell->info().Tcondition && !isnan(conductionEnergy)) neighborCell->info().internalEnergy += conductionEnergy;
	//		//cout << "added conduction energy"<< conductionEnergy << endl;
	//	}
}

CVector ThermalEngine::cellBarycenter(const CellHandle& cell)
{
	CVector center(0, 0, 0);
	for (int k = 0; k < 4; k++)
		center = center + 0.25 * (cell->vertex(k)->point().point() - CGAL::ORIGIN);
	return center;
}

void ThermalEngine::computeNewPoreTemperatures()
{
	bool addToDeltaTemp = false;
	flow->solver->setNewCellTemps(addToDeltaTemp);
}

void ThermalEngine::computeNewParticleTemperatures()
{
	//applyBoundaryHeatFluxes(); // FIXME: buggy commenting out for now

	YADE_PARALLEL_FOREACH_BODY_BEGIN(const shared_ptr<Body>& b, scene->bodies)
	{
		if (b->shape->getClassIndex() != Sphere::getClassIndexStatic() || !b)
			continue;
		auto* thState = b->state.get();
		if (!first && thState->isCavity)
			continue;
		Sphere*    sphere  = dynamic_cast<Sphere*>(b->shape.get());
		const Real density = (particleDensity > 0 ? particleDensity : b->material->density);
		const Real volume  = 4. / 3. * M_PI * pow(sphere->radius, 3); // - thState->capVol;
		if (thState->Tcondition)
			continue;
		thState->oldTemp  = thState->temp;
		thState->temp     = thState->stepFlux * thermalDT / (thState->Cp * density * volume) + thState->oldTemp; // first order forward difference
		thState->stepFlux = 0;
	}
	YADE_PARALLEL_FOREACH_BODY_END();
}


void ThermalEngine::thermalExpansion()
{
	// adjust particle size
	if (particleAlpha > 0) {
		YADE_PARALLEL_FOREACH_BODY_BEGIN(const shared_ptr<Body>& b, scene->bodies)
		{
			if (b->shape->getClassIndex() != Sphere::getClassIndexStatic() || !b)
				continue;
			Sphere* sphere  = dynamic_cast<Sphere*>(b->shape.get());
			auto*   thState = b->state.get();
			if (!first && thState->isCavity)
				continue;
			if (!thState->Tcondition) {
				thState->delRadius = thState->alpha * sphere->radius * (thState->temp - thState->oldTemp);
				sphere->radius += thState->delRadius;
			}
			//}
		}
		YADE_PARALLEL_FOREACH_BODY_END();
	}
	// adjust cell pressure
	if (fluidBeta > 0) {
		cavitySolidVolumeChange = 0;
		cavityVolume            = 0;
		cavityDtemp             = 0;
		flow->solver->cavityDV  = 0; //setting cavity rate of volume change to 0 here, will be added to in adjustCavityVolumeChange
		Tesselation& Tes        = flow->solver->T[flow->solver->currentTes];
		Real         cavDens    = flow->solver->cavityFluidDensity;
		//	#ifdef YADE_OPENMP
		const long sizeCells = Tes.cellHandles.size();
#pragma omp parallel for
		for (long i = 0; i < sizeCells; i++) {
			CellHandle& cell = Tes.cellHandles[i];
			//	#else
			if (cell->info().isFictious || cell->info().blocked)
				continue;
			cell->info().dv() = 0; // reset dv so we can start adding to it
			if (cell->info().isCavity)
				cavityVolume += 1. / cell->info().invVoidVolume();
			if (solidThermoMech && particleAlpha > 0)
				computeCellVolumeChangeFromSolidVolumeChange(cell);
			if (fluidThermoMech)
				computeCellVolumeChangeFromDeltaTemp(cell, cavDens);
		}
		if (solidThermoMech && particleAlpha > 0 && flow->controlCavityPressure)
			accountForCavitySolidVolumeChange();
	}
	if (fluidThermoMech && flow->controlCavityVolumeChange)
		accountForCavityThermalVolumeChange();
}

void ThermalEngine::accountForCavityThermalVolumeChange() { flow->solver->cavityDV += -cavityVolume * fluidBeta * cavityDtemp / thermalDT; }

void ThermalEngine::accountForCavitySolidVolumeChange()
{
	Tesselation& Tes = flow->solver->T[flow->solver->currentTes];
	//	#ifdef YADE_OPENMP
	const long sizeCells = Tes.cellHandles.size();
#pragma omp parallel for
	for (long i = 0; i < sizeCells; i++) {
		CellHandle& cell = Tes.cellHandles[i];
		if (!cell->info().isCavity || cell->info().blocked)
			continue;
		cell->info().p() += (1. / flow->solver->equivalentCompressibility) * cavityVolume * cavitySolidVolumeChange;
	}
}

void ThermalEngine::computeCellVolumeChangeFromSolidVolumeChange(CellHandle& cell)
{
	const shared_ptr<BodyContainer>& bodies = scene->bodies;
	//const Real K = flow->fluidBulkModulus;
	//	const Real Vo = 1./cell->info().invVoidVolume(); // FIXME: depends on volumeSolidPore(), which only uses CGAL triangulations so this value is only updated with remeshes. We need our own function for volumeSolidPore.
	Real solidVolumeChange = 0;
	for (int v = 0; v < 4; v++) {
		const long int id = cell->vertex(v)->info().id();
		if (!Body::byId(id))
			continue;
		const shared_ptr<Body>& b = (*bodies)[id];
		if (b->shape->getClassIndex() != Sphere::getClassIndexStatic() || !b)
			continue;
		Sphere* sphere  = dynamic_cast<Sphere*>(b->shape.get());
		auto*   thState = b->state.get();
		if (!first && thState->isCavity)
			continue; // don't consider fake cavity bodies
		const Real surfaceArea
		        = cell->info().sphericalVertexSurface[v]; // from last fluid engine remesh. We could store this in cell->info() if it becomes a burden
		const Real rCubedDiff = pow(sphere->radius, 3) - pow(sphere->radius - thState->delRadius, 3);
		const Real areaTot    = 4. * M_PI * pow(sphere->radius, 2); // We could store this in cell->info() if it becomes a burden
		solidVolumeChange += surfaceArea / areaTot * rCubedDiff * M_PI * 4. / 3.;
	}

	//	Real newVoidVol = Vo - solidVolumeChange;
	if (!cell->info().isCavity || !flow->controlCavityPressure) {
		//cell->info().invVoidVolume() = newVoidVol > 0 ? 1./newVoidVol : cell->info().invVoidVolume();
		cell->info().dv() += -solidVolumeChange / thermalDT;
	} else {
		cavitySolidVolumeChange += solidVolumeChange;
	}
}

//void ThermalEngine::computeInvVoidVolume(){
//	const shared_ptr<BodyContainer>& bodies=scene->bodies;
//   	Tesselation& Tes = flow->solver->T[flow->solver->currentTes];
//	const long sizeCells = Tes.cellHandles.size();
//	#pragma omp parallel for
//    	for (long i=0; i<sizeCells; i++){
//		Real solidVolume = 0;
//		CellHandle& cell = Tes.cellHandles[i];
//		if (cell->info().isFictious) continue;
//    		for (int v=0;v<4;v++){
//        		const long int id = cell->vertex(v)->info().id();
//       			const shared_ptr<Body>& b =(*bodies)[id];
//        		if (b->shape->getClassIndex()!=Sphere::getClassIndexStatic() || !b) continue;
//        		Sphere* sphere = dynamic_cast<Sphere*>(b->shape.get());
//			const Real surfaceArea = cell->info().sphericalVertexSurface[v];
//			const Real areaTot = 4. * M_PI * pow(sphere->radius,2);
//			solidVolume += surfaceArea/areaTot * pow(sphere->radius,3) * M_PI * 4./3.;
//		}
//		cell->info().invVoidVolume() = abs(cell->info().volume()-solidVolume);
//	}
//}

void ThermalEngine::computeCellVolumeChangeFromDeltaTemp(CellHandle& cell, Real /*cavDens*/)
{
	Real beta;
	if (tempDependentFluidBeta)
		beta = 7.5e-6 * cell->info().temp() + 5.7e-5; // linear model for thermal expansion
	else
		beta = fluidBeta;
	Real poreVolume;
	if (porosityFactor > 0)
		poreVolume = cell->info().volume() * porosityFactor; // allows us to simulate low porosity matrices
	else
		poreVolume = (1. / cell->info().invVoidVolume());
	//	else K = flow->fluidBulkModulus;
	if (!cell->info().isCavity) {
		cell->info().dv() += -poreVolume * beta * cell->info().dtemp() / thermalDT;
	} else if (cell->info().isCavity) {
		cell->info().dv() += -(cell->info().volume()) * beta * cell->info().dtemp()
		        / thermalDT; // ignore the particles used for fluid discretization in the pore (i.e. use volume())
	}
}

// used for computing flow forces after thermalengine modifies pressures (thermomech) // CURRENTLY USELESS
void ThermalEngine::updateForces()
{
	flow->solver->computeFacetForcesWithCache();
	scene->forces.sync();
	flow->computeViscousForces(*flow->solver);
	flow->applyForces(*flow->solver);
}

// used for instantly changing temperature and observing volumetric expansion
void ThermalEngine::applyTempDeltaToSolids(Real delT)
{
	YADE_PARALLEL_FOREACH_BODY_BEGIN(const shared_ptr<Body>& b, scene->bodies)
	{
		if (b->shape->getClassIndex() != Sphere::getClassIndexStatic() || !b)
			continue;
		auto* thState = b->state.get();
		if (!thState->Tcondition) {
			thState->temp += delT;
		}
	}
	YADE_PARALLEL_FOREACH_BODY_END();
	delT = 0;
}

void ThermalEngine::resetFlowBoundaryTemps()
{
	for (int k = 0; k < 6; k++) {
		flow->solver->thermalBoundary(flow->wallIds[k]).fluxCondition = !flow->bndCondIsTemperature[k];
		flow->solver->thermalBoundary(flow->wallIds[k]).value         = flow->thermalBndCondValue[k];
	}

	RTriangulation& Tri = flow->solver->T[flow->solver->currentTes].Triangulation();

	for (int bound = 0; bound < 6; bound++) {
		int& id = *flow->solver->boundsIds[bound];
		//flow->solver->thermalBoundingCells[bound].clear();
		if (id < 0)
			continue;
		CGT::ThermalBoundary& bi = flow->solver->thermalBoundary(id);

		if (!bi.fluxCondition) {
			VectorCell tmpCells;
			tmpCells.resize(10000);
			VCellIterator cells_it  = tmpCells.begin();
			VCellIterator cells_end = Tri.incident_cells(flow->solver->T[flow->solver->currentTes].vertexHandles[id], cells_it);
			for (VCellIterator it = tmpCells.begin(); it != cells_end; it++) {
				(*it)->info().temp() = bi.value;
			}
		}
	}
	flowTempBoundarySet = true;
}

// void ThermalEngine::computeVolumeSolidPore(CellHandle& Cell){
//     const shared_ptr<BodyContainer>& bodies=scene->bodies;
// 	Tesselation& Tes = flow->solver->T[flow->solver->currentTes];
// //	#ifdef YADE_OPENMP
// 	const long size = Tes.cellHandles.size();
// //	#pragma omp parallel for num_threads(ompThreads>0 ? ompThreads : 1)
//     for (long i=0; i<size; i++){
// 		CellHandle& cell = Tes.cellHandles[i];
//         Real vSolid;
//         VertexHandle W[4];
//         Body
// 		for (int k=0;k<4;k++){
//
//             const long int id = cell->vertex(k)->info().id();
// 			const shared_ptr<Body>& b =(*bodies)[id];
//             for (int l=0;l<4;k++) W[l] = cell->vertex(l);
// 			if (b->shape->getClassIndex()!=Sphere::getClassIndexStatic() || !b) continue;
//             Sphere* sphere = dynamic_cast<Sphere*>(b->shape.get());
//             W[k] = cell->vertex(k);
//             vSolid += sphericalTriangleVolume(*sphere,W[1]->point().point,W[2]->point().point(),W[3]->point().point())
//
// }
//
// Real ThermalEngine::sphericalTriangleVolume(const Sphere& ST1, const Point& PT1, const Point& PT2, const Point& PT3)
// {
//         Real rayon = math::sqrt(ST1.weight());
//         if (rayon == 0.0) return 0.0;
//         return ((ONE_THIRD * rayon) * (fastSphericalTriangleArea(ST1, PT1, PT2, PT3))) ;
// }
//
// Real ThermalEngine::fastSolidAngle(const Point& STA1, const Point& PTA1, const Point& PTA2, const Point& PTA3)
// {
//         //! This function needs to be fast because it is used heavily. Avoid using vector operations which require constructing vectors (~50% of cpu time in the non-fast version), and compute angle using the 3x faster formula of Oosterom and StrackeeVan Oosterom, A; Strackee, J (1983). "The Solid Angle of a Plane Triangle". IEEE Trans. Biom. Eng. BME-30 (2): 125-126. (or check http://en.wikipedia.org/wiki/Solid_angle)
//         using namespace CGAL;
//         Real M[3][3];
//         M[0][0] = PTA1.x() - STA1.x();
//         M[0][1] = PTA2.x() - STA1.x();
//         M[0][2] = PTA3.x() - STA1.x();
//         M[1][0] = PTA1.y() - STA1.y();
//         M[1][1] = PTA2.y() - STA1.y();
//         M[1][2] = PTA3.y() - STA1.y();
//         M[2][0] = PTA1.z() - STA1.z();
//         M[2][1] = PTA2.z() - STA1.z();
//         M[2][2] = PTA3.z() - STA1.z();
//
//         Real detM = M[0][0]* (M[1][1]*M[2][2]-M[2][1]*M[1][2]) +
//                       M[1][0]* (M[2][1]*M[0][2]-M[0][1]*M[2][2]) +
//                       M[2][0]* (M[0][1]*M[1][2]-M[1][1]*M[0][2]);
//
//         Real pv12N2 = pow(M[0][0],2) +pow(M[1][0],2) +pow(M[2][0],2);
//         Real pv13N2 = pow(M[0][1],2) +pow(M[1][1],2) +pow(M[2][1],2);
//         Real pv14N2 = pow(M[0][2],2) +pow(M[1][2],2) +pow(M[2][2],2);
//
//         Real pv12N = sqrt(pv12N2);
//         Real pv13N = sqrt(pv13N2);
//         Real pv14N = sqrt(pv14N2);
//
//         Real cp12 = (M[0][0]*M[0][1]+M[1][0]*M[1][1]+M[2][0]*M[2][1]);
//         Real cp13 = (M[0][0]*M[0][2]+M[1][0]*M[1][2]+M[2][0]*M[2][2]);
//         Real cp23 = (M[0][1]*M[0][2]+M[1][1]*M[1][2]+M[2][1]*M[2][2]);
//
//         Real ratio = detM/ (pv12N*pv13N*pv14N+cp12*pv14N+cp13*pv13N+cp23*pv12N);
//         return abs(2*atan(ratio));
// }

} // namespace yade

#endif //YADE_OPENMP
#endif //THERMAL
