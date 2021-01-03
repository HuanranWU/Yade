// (c) 2019  Deepak kunhappan : deepak.kunhappan@3sr-grenoble.fr; deepak.kn1990@gmail.com

#ifdef YADE_MPI

#include "FoamCoupling.hpp"
#include <mpi.h>

#include <pkg/common/Box.hpp>
#include <pkg/common/Facet.hpp>
#include <pkg/common/Grid.hpp>
#include <pkg/common/Sphere.hpp>

#include <iostream>

namespace yade { // Cannot have #include directive inside.

CREATE_LOGGER(FoamCoupling);
YADE_PLUGIN((FoamCoupling));
YADE_PLUGIN((FluidDomainBbox));
CREATE_LOGGER(FluidDomainBbox);
YADE_PLUGIN((Bo1_FluidDomainBbox_Aabb));


void Bo1_FluidDomainBbox_Aabb::go(const shared_ptr<Shape>& cm, shared_ptr<Bound>& bv, const Se3r&, const Body*)
{
	FluidDomainBbox* domain = static_cast<FluidDomainBbox*>(cm.get());
	if (!bv) {
		bv = shared_ptr<Bound>(new Aabb);
	}
	Aabb* aabb = static_cast<Aabb*>(bv.get());

	aabb->min = scene->isPeriodic ? scene->cell->wrapPt(domain->minBound) : domain->minBound;
	aabb->max = scene->isPeriodic ? scene->cell->wrapPt(domain->maxBound) : domain->maxBound;
	return;
}


/******serial version *****/

void FoamCoupling::getRank()
{
	if (couplingModeParallel) {
		scene = Omega::instance().getScene().get();
		MPI_Comm_rank(selfComm(), &localRank);
		MPI_Comm_size(selfComm(), &localCommSize);

		MPI_Comm_rank(MPI_COMM_WORLD, &worldRank);
		MPI_Comm_size(MPI_COMM_WORLD, &worldCommSize);
		commSizeSet = true;
		stride      = localCommSize;
		commSzdff   = abs(localCommSize - worldCommSize);
	} else {
		MPI_Comm_rank(MPI_COMM_WORLD, &rank);
		MPI_Comm_size(MPI_COMM_WORLD, &commSize);
		commSizeSet = true;
	}
}

void FoamCoupling::setNumParticles(int np)
{
	getRank();
	numParticles = np;
	initDone     = true;
}

void FoamCoupling::setIdList(const std::vector<int>& alist)
{
	bodyList.clear();
	bodyList.resize(alist.size());
	for (unsigned int i = 0; i != bodyList.size(); ++i) {
		bodyList[i] = alist[i];
	}
	bodyListModified = true;
}


void FoamCoupling::insertBodyId(int bId)
{
	const auto& iter = std::find(bodyList.begin(), bodyList.end(), bId);
	if (iter != bodyList.end()) {
		LOG_WARN("Body Id " << bId << "  already exists in coupling. ")
	} else {
		bodyList.push_back(bId);
	}
	bodyListModified = true;
}

void FoamCoupling::eraseId(int bId)
{
	auto it = std::find(bodyList.begin(), bodyList.end(), bId);
	if (it != bodyList.end()) {
		bodyList.erase(it);
		bodyListModified = true;
	} else {
		LOG_ERROR("Id not found in list of ids in coupling");
	}
}


int FoamCoupling::getNumBodies() { return bodyList.size(); }

std::vector<int> FoamCoupling::getIdList() { return bodyList; }

void FoamCoupling::castParticle()
{
	int sz = bodyList.size();
	MPI_Bcast(&sz, 1, MPI_INT, rank, MPI_COMM_WORLD);

	procList.resize(sz);
	hydroForce.resize(sz * 6);
	particleData.resize(10 * sz);

	std::fill(procList.begin(), procList.end(), -1);
	std::fill(hydroForce.begin(), hydroForce.end(), 0.0);

#ifdef YADE_OPENMP
#pragma omp parallel for collapse(1)
#endif

	for (unsigned int i = 0; i < bodyList.size(); ++i) {
		const Body* b = (*scene->bodies)[bodyList[i]].get();
		if (scene->isPeriodic) {
			const Vector3r& pos      = scene->cell->wrapPt(b->state->pos);
			particleData[i * 10]     = pos[0];
			particleData[i * 10 + 1] = pos[1];
			particleData[i * 10 + 2] = pos[2];
		} else {
			particleData[i * 10]     = b->state->pos[0];
			particleData[i * 10 + 1] = b->state->pos[1];
			particleData[i * 10 + 2] = b->state->pos[2];
		}
		particleData[i * 10 + 3] = b->state->vel[0];
		particleData[i * 10 + 4] = b->state->vel[1];
		particleData[i * 10 + 5] = b->state->vel[2];
		particleData[i * 10 + 6] = b->state->angVel[0];
		particleData[i * 10 + 7] = b->state->angVel[1];
		particleData[i * 10 + 8] = b->state->angVel[2];
		shared_ptr<Sphere> s     = YADE_PTR_DYN_CAST<Sphere>(b->shape);
		particleData[i * 10 + 9] = s->radius;
	}

	MPI_Bcast(&particleData.front(), particleData.size(), MPI_DOUBLE, rank, MPI_COMM_WORLD);
	// clear array after bcast
	particleData.clear();
}

void FoamCoupling::castNumParticle(int value) { MPI_Bcast(&value, 1, MPI_INT, rank, MPI_COMM_WORLD); }


void FoamCoupling::castTerminate()
{
	int value = 10;
	MPI_Bcast(&value, 1, MPI_INT, rank, MPI_COMM_WORLD);
}

void FoamCoupling::updateProcList()
{
	for (unsigned int i = 0; i != bodyList.size(); ++i) {
		int dummy_val = -5;
		MPI_Allreduce(&dummy_val, &procList[i], 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);
		if (procList[i] < 0)
			LOG_WARN("particle id" << i << "not found in FOAM" << std::endl);
	}
}

void FoamCoupling::recvHydroForce()
{
	std::fill(hydroForce.begin(), hydroForce.end(), 0.0);
	for (unsigned int i = 0; i != procList.size(); ++i) {
		int                 recvFrom = procList[i];
		std::vector<double> fBuff(6, 0.0);
		MPI_Recv(&fBuff.front(), 6, MPI_DOUBLE, recvFrom, TAG_FORCE, MPI_COMM_WORLD, &status);
		for (unsigned int j = 0; j != 6; ++j) {
			hydroForce[6 * i + j] = fBuff[j];
		}
	}
}

void FoamCoupling::setHydroForce()
{
	// clear hydroforce before summation

#ifdef YADE_OPENMP
#pragma omp parallel for collapse(1)
#endif
	for (unsigned int i = 0; i < bodyList.size(); ++i) {
		const Vector3r& fx = Vector3r(hydroForce[6 * i], hydroForce[6 * i + 1], hydroForce[6 * i + 2]);
		const Vector3r& tx = Vector3r(hydroForce[6 * i + 3], hydroForce[6 * i + 4], hydroForce[6 * i + 5]);
		scene->forces.addForce(bodyList[i], fx);
		scene->forces.addTorque(bodyList[i], tx);
	}
}

void FoamCoupling::sumHydroForce()
{
	// clear the vector
	std::fill(hydroForce.begin(), hydroForce.end(), 0.0);
	Real dummy_val = 0.0;
	for (unsigned int i = 0; i != bodyList.size(); ++i) {
		for (unsigned int j = 0; j != 6; ++j) {
			MPI_Allreduce(&dummy_val, &hydroForce[6 * i + j], 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
		}
	}
}


void FoamCoupling::resetProcList() { procList.clear(); }


void FoamCoupling::exchangeDeltaT()
{
	// Recv foamdt  first and broadcast;
	MPI_Recv(&foamDeltaT, 1, MPI_DOUBLE, 1, TAG_FLUID_DT, MPI_COMM_WORLD, &status);
	//bcast yadedt to others.
	Real yadeDt = scene->dt;
	MPI_Bcast(&yadeDt, 1, MPI_DOUBLE, rank, MPI_COMM_WORLD);
	// calculate the interval . TODO: to include hydrodynamic time scale if inertial in openfoam
	// here -> hDeltaT = getViscousTimeScale();
	dataExchangeInterval = (long int)((yadeDt < foamDeltaT) ? foamDeltaT / yadeDt : 1);
}

Real FoamCoupling::getViscousTimeScale()
{
	//  Real hDeltaT = 0.0;
	//  Real dummy = 1e9;
	//
	//  MPI_Allreduce(&dummy, &hDeltaT, MPI_DOUBLE, MPI_MIN, MPI_COMM_WORLD);
	//
	return 0;
}


/***********parallel version *******/

void FoamCoupling::getFluidDomainBbox()
{
	/* get the bounding box of the grid from each fluid solver processes, this gird minmax is used to set the min/max of the body of shape FluidDomainBbox. 
	 All Yade processes have ranks from 0 to yadeCommSize - 1 in the  MPI_COMM_WORLD communicator, the fluid Ranks are then from yadeCommSize to size(M
	 PI_COMM_WORLD) -1, all yade ranks receive the min max of the fluid domains, and insert it to their body containers. The fluid subdomain bodies have subdomain=0, they are actually owned 
	 by the master process (rank=0) in the yade communicator. */

	// local commsize and rank

	MPI_Comm_rank(selfComm(), &localRank);
	MPI_Comm_size(selfComm(), &localCommSize);
	//world comm size and world rank

	MPI_Comm_size(MPI_COMM_WORLD, &worldCommSize);
	MPI_Comm_rank(MPI_COMM_WORLD, &worldRank);


	commSzdff = abs(localCommSize - worldCommSize);

	stride = localCommSize;

	std::vector<std::vector<double>> minMaxBuff;

	//alloc memory
	for (int i = 0; i != commSzdff; ++i) {
		std::vector<double> buff(6, 1e-50);
		minMaxBuff.push_back(buff);
	}


	//recv the grid minmax from fluid solver.

	for (int rnk = 0; rnk != commSzdff; ++rnk) {
		MPI_Status           status;
		std::vector<double>& buff = minMaxBuff[rnk];
		MPI_Recv(&buff.front(), 6, MPI_DOUBLE, rnk + stride, TAG_GRID_BBOX, MPI_COMM_WORLD, &status);
	}

	fluidDomains.resize(commSzdff);
	//create fluidDomainBbox bodies and get their ids.
	for (int fd = 0; fd != commSzdff; ++fd) {
		shared_ptr<Body>            flBody(shared_ptr<Body>(new Body()));
		shared_ptr<FluidDomainBbox> flBodyshape(shared_ptr<FluidDomainBbox>(new FluidDomainBbox()));
		flBodyshape->setMinMax(minMaxBuff[fd]);
		flBodyshape->domainRank      = stride + fd;
		flBodyshape->hasIntersection = false;
		flBody->shape                = flBodyshape;
		flBody->subdomain            = 0;
		flBody->setIsFluidDomainBbox(true);
		fluidDomains[fd] = scene->bodies->insert(flBody);
	}
	minMaxBuff.clear(); // dealloc the recvd minMaxBuff;
}


/*
void FoamCoupling::buildSharedIdsMapv2(){
	for (const auto& flId : fluidDomains){
		const shared_ptr<Body>& flbody = Body::byId(flId, scene);
		if (flbody){
			const shared_ptr<FluidDomainBbox>& flbox = YADE_PTR_CAST<FluidDomainBbox>(flbody->shape); 
			for (const auto& flIntrs : flbdy->intrs){
				Body::id_t otherBodyId; 
				if (flId == intr->getId1()) {otherBodyId = flIntrs->getId2(); } else {otherBodyId==flIntrs->getId2()}
				if (!ifDomainBodies(*scene->bodies[otherBodyId])){
					flbox->hasIntersection = true; 
					flbox->bIds.push_back(otherBodyId); 
				}
			}
		}
	}
}*/

void FoamCoupling::buildSharedIdsMap()
{
	/*Builds the list of ids interacting with a fluid subdomain and stores those body ids that has intersections with several fluid domains. 
	 sharedIdsMapIndx = a vector of std::pair<Body::id_t, std::map<fluidDomainId, indexOfthebodyinthefluidDomain aka index in flbdy-> bIds> > */

	inCommunicationProc.clear();

	// const shared_ptr<Subdomain>& subd = YADE_PTR_CAST<Subdomain>((*scene->bodies)[scene->thisSubdomainId]->shape); //not needed as we have localIds list.
	for (const auto& bodyId : localIds) {
		std::map<int, int> testMap;
		const auto&        bIntrs = (*scene->bodies)[bodyId]->intrs;
		for (const auto& itIntr : bIntrs) {
			const shared_ptr<Interaction>& intr = itIntr.second;
			Body::id_t                     otherId;
			if (bodyId == intr->getId1()) {
				otherId = intr->getId2();
			} else {
				otherId = intr->getId1();
			}
			const auto& otherBody = Body::byId(otherId, scene);
			if (otherBody->getIsFluidDomainBbox()) {
				const shared_ptr<FluidDomainBbox>& flbox = YADE_PTR_CAST<FluidDomainBbox>(otherBody->shape);
				flbox->bIds.push_back(bodyId);
				if (!flbox->hasIntersection) {
					flbox->hasIntersection = true;
				}
				int indx = (flbox->bIds.size()) - 1;
				testMap.insert(std::make_pair(
				        otherId,
				        indx)); // get the fluiddomainbbox body id and index in flbody->bIds, this will be used in the verifyTracking function
			}
		}
		if (testMap.size() > 1) { // this body has intersections with more than one fluid domains, hence this is a shared id .
			sharedIdsMapIndx.push_back(std::make_pair(bodyId, testMap));
		}
	}

	//for quickly identifying fluid procs.
	for (const auto& fluidId : fluidDomains) {
		const shared_ptr<Body>& flb = (*scene->bodies)[fluidId];
		if (flb) {
			const shared_ptr<FluidDomainBbox>& flBox = YADE_PTR_CAST<FluidDomainBbox>(flb->shape);
			if (flBox->bIds.size() > 0) {
				inCommunicationProc.push_back(std::make_pair(flBox->domainRank, flBox->bIds.size()));
			}
		}
	}
}


//unused.
void FoamCoupling::buildSharedIds()
{
	/*It is possible to have one yade body to have interactions with several  fluid subdomains, (we just have the bounding box of the fluid domain, the fluid domain is a regular polygon with several faces). 
	 Building a list of those ids which are have several interactions helps to identify those fliud procs from whom to receive the hydrodynamic force and tracking. This is used in the function 
	verifyParticleDetection. */

	const shared_ptr<Subdomain>& subD = YADE_PTR_CAST<Subdomain>(scene->subD);
	for (int bodyId = 0; bodyId != static_cast<int>(subD->ids.size()); ++bodyId) {
		std::vector<Body::id_t> fluidIds;
		const shared_ptr<Body>& testBody = (*scene->bodies)[subD->ids[bodyId]];
		for (const auto& bIntrs : testBody->intrs) {
			const shared_ptr<Interaction>& intr = bIntrs.second;
			Body::id_t                     otherId;
			if (testBody->id == intr->getId1()) {
				otherId = intr->getId2();
			} else {
				otherId = intr->getId1();
			}
			if (ifFluidDomain(otherId)) {
				fluidIds.push_back(otherId);
			}
		}
		if (fluidIds.size() > 1) {
			sharedIds.push_back(std::make_pair(subD->ids[bodyId], fluidIds));
		} // this body has interaction with  more than one fluid  grids.
	}
}


int FoamCoupling::ifSharedIdMap(const Body::id_t& testId)
{
	int res = -1;
	//std::vector<std::pair<int, std::map<int, int> > >::iterator
	auto it = std::find_if(sharedIdsMapIndx.begin(), sharedIdsMapIndx.end(), [&testId](const std::pair<int, std::map<int, int>> elem) -> bool {
		return testId == elem.first;
	});

	if (it != sharedIdsMapIndx.end()) {
		res = it - sharedIdsMapIndx.begin();
	}
	return res;
}

bool FoamCoupling::ifFluidDomain(const Body::id_t& testId)
{
	/* function to check if the body is fluidDomainBox*/
	const auto& iter = std::find(fluidDomains.begin(), fluidDomains.end(), testId);
	if (iter != fluidDomains.end()) {
		return true;
	} else {
		return false;
	}
}


void FoamCoupling::buildLocalIds()
{
	if (localRank == yadeMaster) {
		return;
	} // master has no bodies.
	if (bodyList.size() == 0) {
		LOG_ERROR("Ids for coupling has no been set, FAIL!");
		return;
	}
	const shared_ptr<Subdomain>& subD = YADE_PTR_CAST<Subdomain>(scene->subD);
	if (!subD) {
		LOG_ERROR("subdomain not found for local rank/ world rank  = " << localRank << "   " << worldRank);
		return;
	}
	for (const auto& testId : bodyList) {
		std::vector<Body::id_t>::iterator iter = std::find(subD->ids.begin(), subD->ids.end(), testId); // can subD have ids sorted?
		if (iter != subD->ids.end()) {
			localIds.push_back(*iter);
		}
	}
}


bool FoamCoupling::ifDomainBodies(const shared_ptr<Body>& b)
{
	// check if body is subdomain, wall, facet, or other fluidDomainBbox

	shared_ptr<Box>             boxShape   = YADE_PTR_DYN_CAST<Box>(b->shape);
	shared_ptr<FluidDomainBbox> fluidShape = YADE_PTR_DYN_CAST<FluidDomainBbox>(b->shape);
	shared_ptr<Facet>           facetShape = YADE_PTR_DYN_CAST<Facet>(b->shape);

	if (b->getIsSubdomain()) {
		return true;
	} else if (boxShape) {
		return true;
	} else if (facetShape) {
		return true;
	} else {
		return false;
	}
}

void FoamCoupling::sendIntersectionToFluidProcs()
{
	// notify the fluid procs about intersection based on number of intersecting bodies.
	// vector of sendRecvRanks, with each vector element containing the number of bodies, if no bodies, send negative val.
	std::vector<int> sendRecvRanks(fluidDomains.size(), -1);

	for (unsigned f = 0; f != fluidDomains.size(); ++f) {
		const shared_ptr<Body>& fdomain = (*scene->bodies)[fluidDomains[f]];
		if (fdomain) {
			const shared_ptr<FluidDomainBbox>& fluidBox = YADE_PTR_CAST<FluidDomainBbox>(fdomain->shape);
			if (fluidBox->bIds.size() > 0) {
				sendRecvRanks[f] = fluidBox->bIds.size();

			} else {
				sendRecvRanks[f] = -1;
			}
		} else {
			sendRecvRanks[f] = -1;
		}
	}
	//
	int buffSz = fluidDomains.size();

	//MPI_Send ..

	for (int rnk = 0; rnk != commSzdff; ++rnk) {
		MPI_Send(&sendRecvRanks.front(), buffSz, MPI_INT, rnk + stride, TAG_SZ_BUFF, MPI_COMM_WORLD);
	}
}

void FoamCoupling::sendBodyData()
{
	/* send the particle data to the associated fluid procs. prtData -> pos, vel, angvel, raidus (for sphere), if fiber -> ori  */
	bool isPeriodic = scene->isPeriodic;
	for (int f = 0; f != static_cast<int>(fluidDomains.size()); ++f) {
		const shared_ptr<Body>& flbody = (*scene->bodies)[fluidDomains[f]];
		if (flbody) {
			const shared_ptr<FluidDomainBbox> flbox = YADE_PTR_CAST<FluidDomainBbox>(flbody->shape);
			if (flbox->hasIntersection) {
				std::vector<Real> prtData(10 * flbox->bIds.size(), 1e-50);
				for (unsigned int i = 0; i != flbox->bIds.size(); ++i) {
					const shared_ptr<Body>& b = (*scene->bodies)[flbox->bIds[i]];
					if (isPeriodic) {
						const Vector3r& pos = scene->cell->wrapPt(b->state->pos);
						prtData[10 * i]     = pos[0];
						prtData[10 * i + 1] = pos[1];
						prtData[10 * i + 2] = pos[2];

					} else {
						prtData[10 * i]     = b->state->pos[0];
						prtData[10 * i + 1] = b->state->pos[1];
						prtData[10 * i + 2] = b->state->pos[2];
					}
					prtData[10 * i + 3] = b->state->vel[0];
					prtData[10 * i + 4] = b->state->vel[1];
					prtData[10 * i + 5] = b->state->vel[2];
					prtData[10 * i + 6] = b->state->angVel[0];
					prtData[10 * i + 7] = b->state->angVel[1];
					prtData[10 * i + 8] = b->state->angVel[2];

					const shared_ptr<Sphere>& sph = YADE_PTR_CAST<Sphere>(b->shape);
					prtData[10 * i + 9]           = sph->radius;
				}

				int sz = prtData.size();
				MPI_Send(&prtData.front(), sz, MPI_DOUBLE, flbox->domainRank, TAG_PRT_DATA, MPI_COMM_WORLD);
			}
		}
	}
}


void FoamCoupling::verifyParticleDetection()
{
	/* check if the sent particles are located on the fluid procs, verify all particles (in fluid coupling) owned by the yade process has been accounted for. Some particles may intersect the 
	 fluid domains bounding box but may not be actually inside the fluid mesh. 
	 Method : Everty fluid proc sents a vector of it's search result. if found res = 1, else res =0, for each particle. 
	 each yade rank receives this vector from intersecting fluid ranks, looks through the vector to find the fails. 
	 if fail is found : see if this id is a sharedid. if not this particle has been 'lost'. if shared id :  check the vector of verifyTracking of the intersecting fluid domain till found in 
	 at least one intersecting fluid box. if not particle has been lost. */

	//std::map<int, std::vector<int> > verifyTracking;  //vector containing domainRank, vector of "found/misses" for each body, miss = -1, found = 1.

	std::vector<std::pair<int, std::vector<int>>> verifyTracking;

	for (const auto& proc : inCommunicationProc) {
		std::vector<int> vt(proc.second, -1);
		verifyTracking.push_back(std::make_pair(proc.first, std::move(vt)));
	}

	// recv the vec.
	for (auto& it : verifyTracking) {
		std::vector<int>& vt  = it.second;
		int               rnk = it.first;
		MPI_Status        status;
		int               buffSz = vt.size();
		MPI_Recv(&vt.front(), buffSz, MPI_INT, rnk, TAG_SEARCH_RES, MPI_COMM_WORLD, &status);
	}


	//check for misses
	std::vector<Body::id_t> unFoundSharedIds;
	for (const auto& vt : verifyTracking) {
		const int&                         flBdyIndx = abs(vt.first - stride);
		const shared_ptr<FluidDomainBbox>& flbody    = YADE_PTR_CAST<FluidDomainBbox>((*scene->bodies)[fluidDomains[flBdyIndx]]->shape);
		int                                bIndx     = 0;
		for (const auto& val : vt.second) {
			if (val < 0) {
				// this body was not found in the fluid domain.
				const Body::id_t& testId = (*scene->bodies)[flbody->bIds[bIndx]]->id;
				// check if this body is a sharedId  from sharedIdsMap.
				int sharedIndx = ifSharedIdMap(testId);
				if (sharedIndx < 0) {
					const Vector3r& pos = (*scene->bodies)[testId]->state->pos;
					LOG_ERROR(
					        "Particle ID  = " << testId << " pos = " << pos[0] << " " << pos[1] << " " << pos[2]
					                          << " was not found in fluid domain"
					                          << "lost Particle in proc = " << localRank);
				} else {
					Body::id_t unfoundId = testId;
					unFoundSharedIds.push_back(unfoundId);
				}
			}
			++bIndx;
		}
	}

	// check if the 'sharedIds' has been located in any of the fluid procs.
	if (unFoundSharedIds.size() > 0) {
		for (const auto& idPair : sharedIdsMapIndx) {
			const auto& bodyId = idPair.first;
			//const int mpSz = idPair.second.size();
			bool found = false;
			for (const auto& fdIndx : idPair.second) {
				const shared_ptr<FluidDomainBbox>& flbox = YADE_PTR_CAST<FluidDomainBbox>((*scene->bodies)[fdIndx.first]->shape);
				for (const auto& vt : verifyTracking) {
					if (vt.first == flbox->domainRank) {
						if (vt.second[fdIndx.second] > 0)
							found = true;
					}
				}
			}
			if (!found) {
				const Vector3r& pos = (*scene->bodies)[bodyId]->state->pos;
				LOG_ERROR(
				        "Particle ID (SHARED ID )  = " << bodyId << " pos = " << pos[0] << " " << pos[1] << " " << pos[2]
				                                       << " was not found in fluid domain"
				                                       << " lost particle in proc " << localRank);
			}
		}
	}
}

void FoamCoupling::getParticleForce()
{
	hForce.clear();


	for (const auto& proc : inCommunicationProc) {
		std::vector<double> forceVec(6 * proc.second, 0.0);
		hForce.push_back(std::make_pair(proc.first, std::move(forceVec)));
	}


	for (auto& recvForce : hForce) {
		std::vector<double>& tmpForce = recvForce.second;
		int                  recvRank = recvForce.first;
		int                  buffSz   = tmpForce.size();
		MPI_Status           status;
		/* fluid procs having no particles will send 0 force, torque */
		MPI_Recv(&tmpForce.front(), buffSz, MPI_DOUBLE, recvRank, TAG_FORCE, MPI_COMM_WORLD, &status);
	}
}


void FoamCoupling::resetFluidDomains()
{
	// clear the vector ids held fluidDomainBbox->bIds
	for (unsigned f = 0; f != fluidDomains.size(); ++f) {
		const shared_ptr<Body>& fdomain = (*scene->bodies)[fluidDomains[f]];
		if (fdomain) {
			const shared_ptr<FluidDomainBbox>& fluidBox = YADE_PTR_CAST<FluidDomainBbox>(fdomain->shape);
			fluidBox->bIds.clear();
		}
	}
	sharedIdsMapIndx.clear();
	localIds.clear();
	//inCommunicationProc.clear();
}


void FoamCoupling::setHydroForceParallel()
{
	// add the force
	if (localRank == yadeMaster) {
		return;
	}
	for (const auto& rf : hForce) {
		int                                indx     = abs(rf.first - localCommSize);
		const std::vector<double>&         forceVec = rf.second;
		const shared_ptr<FluidDomainBbox>& flbox    = YADE_PTR_CAST<FluidDomainBbox>((*scene->bodies)[fluidDomains[indx]]->shape);
		for (unsigned int i = 0; i != flbox->bIds.size(); ++i) {
			Vector3r fx;
			fx[0] = forceVec[6 * i];
			fx[1] = forceVec[6 * i + 1];
			fx[2] = forceVec[6 * i + 2];
			Vector3r tx;
			tx[0] = forceVec[6 * i + 3];
			tx[1] = forceVec[6 * i + 4];
			tx[2] = forceVec[6 * i + 5];
			scene->forces.addForce(flbox->bIds[i], fx);
			scene->forces.addTorque(flbox->bIds[i], tx);
		}
	}
}

void FoamCoupling::exchangeDeltaTParallel()
{
	// Recv foamdt  first and broadcast;

	if (localRank == yadeMaster) {
		MPI_Status status;
		int        fluidMaster = stride;
		MPI_Recv(&foamDeltaT, 1, MPI_DOUBLE, fluidMaster, TAG_FLUID_DT, MPI_COMM_WORLD, &status);
	}

	//bcast  the fluidDt to all yade_procs.
	// Real  yadeDt = scene-> dt;
	MPI_Bcast(&foamDeltaT, 1, MPI_DOUBLE, yadeMaster, selfComm());

	//do a MPI_Allreduce (min) and get the minDt of all the yade procs..
	Real myDt = scene->dt;
	Real yadeDt;
	MPI_Allreduce(&myDt, &yadeDt, 1, MPI_DOUBLE, MPI_MIN, selfComm());


	// send the minDt to fluid proc (master .. )
	if (localRank == yadeMaster) {
		int fluidMaster = stride;
		MPI_Send(&yadeDt, 1, MPI_DOUBLE, fluidMaster, TAG_YADE_DT, MPI_COMM_WORLD);
	}

	// calculate the interval . TODO: to include hydrodynamic time scale if inertial in openfoam
	// here -> hDeltaT = getViscousTimeScale();
	dataExchangeInterval = (long int)((yadeDt < foamDeltaT) ? foamDeltaT / yadeDt : 1);
}


void FoamCoupling::runCoupling()
{
	castParticle();
	updateProcList();
	if (isGaussianInterp) {
		sumHydroForce();
	} else {
		recvHydroForce();
	}
}

void FoamCoupling::runCouplingParallel()
{
	if (localRank > yadeMaster) { // master proc does not take part in the coupling except for timestep exchange and  receiving the grid minmax
		buildLocalIds();
		buildSharedIdsMap();
		sendIntersectionToFluidProcs();
		sendBodyData();
		verifyParticleDetection();
		getParticleForce();
		//setParticleForceParallel();
	}
}

void FoamCoupling::action()
{
	if (!commSizeSet) {
		getRank();
	}

	if (!couplingModeParallel) {
		if (exchangeData()) {
			runCoupling();
			exchangeDeltaT();
		}
		setHydroForce();
	} else {
		if (exchangeData()) {
			resetFluidDomains();
			runCouplingParallel();
			exchangeDeltaTParallel();
		}
		setHydroForceParallel();
	}
}

bool FoamCoupling::exchangeData() { return scene->iter % dataExchangeInterval == 0; }

void FoamCoupling::killMPI()
{
	castTerminate();
	if (!couplingModeParallel)
		MPI_Finalize();
}
} // namespace yade

#endif
