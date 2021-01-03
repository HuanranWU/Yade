// (c) 2018 Bruno Chareyre <bruno.chareyre@grenoble-inp.fr>
// (c) 2019 Deepak Kunhappan, <deepak.kunhappan@3sr-grenoble.fr> <deepak.kn1990@gmail.com>
//  francois kneib

#pragma once

#include <lib/base/Logging.hpp>
#include <lib/base/Math.hpp>
#include <core/Body.hpp>
// #include<core/PartialEngine.hpp>
#include <core/MPIBodyContainer.hpp>
#include <core/Scene.hpp>
#include <pkg/common/Aabb.hpp>
#include <pkg/common/Dispatching.hpp>
#include <mpi.h>
#include <mpi4py/mpi4py.h> // for passing MPI_Comm from python to c++
namespace yade {           // Cannot have #include directive inside.

using projectedBoundElem = std::pair<double, std::pair<int, int>>; // [dist, subdomain, bodyid] this seems to be faster than struct.

class Subdomain : public Shape {
public:
	void init()
	{
		getRankSize();
		stringBuff.resize(commSize);
		initMasterContainer();
	}
	// 	typedef std::map<Body::id_t,std::vector<Body::id_t> > IntersectionMap; // the lists of bodies from other subdomaines intersecting this one
	//Map fails...
	typedef std::vector<std::vector<Body::id_t>> IntersectionMap; // the lists of bodies from other subdomaines intersecting this one
	                                                              // 	typedef numpy_boost<Body::id_t,2> testType({1,1}) ;
	boost::python::list intrs_get();
	void                intrs_set(const boost::python::list& intrs);
	boost::python::list mIntrs_get();
	void                mIntrs_set(const boost::python::list& intrs);
	//     	boost::python::dict members_get();
	vector<MPI_Request> mpiReqs;
	vector<MPI_Request> sendBodyReqs; // I could use mpiReqs, but then I will have to manage it between states send and bodies send.
	MPI_Comm*           myComm_p;

	MPI_Comm selfComm()
	{
		if (myComm_p)
			return *myComm_p;
		else
			return MPI_COMM_WORLD;
	}

	// pass python-generated communicator to the c++ side
	// inspired by https://bitbucket.org/mpi4py/mpi4py/src/master/demo/wrap-boost/helloworld.cxx
	void setMyComm(boost::python::object py_comm)
	{
		if (import_mpi4py() < 0)
			return; // must be somewhere to initialize mpi4py in c++, else segfault
		PyObject* py_obj = py_comm.ptr();
		myComm_p         = PyMPIComm_Get(py_obj);
		if (myComm_p == NULL)
			LOG_ERROR("invalid COMM received from Python");
	}
	PyObject* getMyComm() { return PyMPIComm_New(*myComm_p); }

	// returns pos,vel,angVel,ori of bodies interacting with a given otherDomain
	std::vector<Real> getStateValuesFromIds(const vector<Body::id_t>& search)
	{
		const shared_ptr<Scene>& scene = Omega::instance().getScene();
		unsigned int             N     = search.size();
		std::vector<Real>        res;
		res.reserve(N * 13);
		for (unsigned k = 0; k < N; k++) {
			const shared_ptr<State>& s = (*(scene->bodies))[search[k]]->state;
			for (unsigned i = 0; i < 3; i++)
				res.push_back(s->pos[i]);
			for (unsigned i = 0; i < 3; i++)
				res.push_back(s->vel[i]);
			for (unsigned i = 0; i < 3; i++)
				res.push_back(s->angVel[i]);
			for (unsigned i = 0; i < 4; i++)
				res.push_back(s->ori.coeffs()[i]);
		}
		return res;
	}


	// returns pos,vel,angVel,ori,bounds of bodies interacting with a given otherDomain
	std::vector<Real> getStateBoundsValuesFromIds(const vector<Body::id_t>& search)
	{
		const shared_ptr<Scene>& scene = Omega::instance().getScene();
		unsigned int             N     = search.size();
		std::vector<Real>        res;
		res.reserve(N * 19);
		for (unsigned k = 0; k < N; k++) {
			const shared_ptr<Body>&  b = (*(scene->bodies))[search[k]];
			const shared_ptr<State>& s = b->state;
			for (unsigned i = 0; i < 3; i++)
				res.push_back(s->pos[i]);
			for (unsigned i = 0; i < 3; i++)
				res.push_back(s->vel[i]);
			for (unsigned i = 0; i < 3; i++)
				res.push_back(s->angVel[i]);
			for (unsigned i = 0; i < 4; i++)
				res.push_back(s->ori.coeffs()[i]);
			//HACK
			if (b->bound) {
				for (unsigned i = 0; i < 3; i++)
					res.push_back(b->bound->min[i]);
				for (unsigned i = 0; i < 3; i++)
					res.push_back(b->bound->max[i]);
			} else {
				for (unsigned i = 0; i < 3; i++)
					res.push_back(0);
				for (unsigned i = 0; i < 3; i++)
					res.push_back(0);
			}
		}
		return res;
	}

	std::vector<Real> getStateValues(unsigned otherSubdomain)
	{
		const shared_ptr<Scene>& scene = Omega::instance().getScene();
		if (scene->subdomain == int(otherSubdomain)) {
			LOG_ERROR("subdomain cannot interact with itself");
			return std::vector<Real>();
		}
		if (otherSubdomain >= intersections.size()) {
			LOG_ERROR("otherSubdomain exceeds no. of subdomains (" << otherSubdomain << " vs. " << intersections.size());
			return std::vector<Real>();
		}
		const vector<Body::id_t>& search = intersections[otherSubdomain];
		return getStateValuesFromIds(search);
	}

	// 	BROKEN:
	// 	boost::python::dict getStateValuesFromIdsAsArray(int otherSubdomain);


	void setStateValuesFromIds(const vector<Body::id_t>& b_ids, const std::vector<Real>& input)
	{
		const shared_ptr<Scene>& scene = Omega::instance().getScene();
		unsigned int             N     = b_ids.size();
		if ((N * 13) != input.size())
			LOG_ERROR("size mismatch" << N * 13 << " vs " << input.size() << " in " << scene->subdomain);
		for (unsigned k = 0; k < N; k++) {
			unsigned int            idx = k * 13;
			const shared_ptr<Body>& b   = (*(scene->bodies))[b_ids[k]];
			if (!b) {
				LOG_ERROR(" Body id not found " << b_ids[k] << "  rank = " << subdomainRank);
				return;
			}
			const shared_ptr<State>& s = (*(scene->bodies))[b_ids[k]]->state;
			s->pos                     = Vector3r(input[idx], input[idx + 1], input[idx + 2]);
			s->vel                     = Vector3r(input[idx + 3], input[idx + 4], input[idx + 5]);
			s->angVel                  = Vector3r(input[idx + 6], input[idx + 7], input[idx + 8]);
			s->ori                     = Quaternionr(
                                input[idx + 12], input[idx + 9], input[idx + 10], input[idx + 11]); //note q.coeffs() and q(w,x,y,z) take different oredrings
		}
	}

	void setStateBoundsValuesFromIds(const vector<Body::id_t>& b_ids, const std::vector<Real>& input)
	{
		const shared_ptr<Scene>& scene = Omega::instance().getScene();
		unsigned int             N     = b_ids.size();
		if ((N * 19) != input.size())
			LOG_ERROR("size mismatch" << N * 19 << " vs " << input.size() << " in " << scene->subdomain);
		for (unsigned k = 0; k < N; k++) {
			const shared_ptr<Body>&  b   = (*(scene->bodies))[b_ids[k]];
			const shared_ptr<State>& s   = b->state;
			unsigned int             idx = k * 19;
			s->pos                       = Vector3r(input[idx], input[idx + 1], input[idx + 2]);
			s->vel                       = Vector3r(input[idx + 3], input[idx + 4], input[idx + 5]);
			s->angVel                    = Vector3r(input[idx + 6], input[idx + 7], input[idx + 8]);
			s->ori                       = Quaternionr(
                                input[idx + 12], input[idx + 9], input[idx + 10], input[idx + 11]); //note q.coeffs() and q(w,x,y,z) take different oredrings

			//b-bound should not be instanciate except pour the last body (k = N-1)
			if (!b->bound) {
				b->bound = boost::make_shared<Aabb>();
			}
			b->bound->min = Vector3r(input[idx + 13], input[idx + 14], input[idx + 15]);
			b->bound->max = Vector3r(input[idx + 16], input[idx + 17], input[idx + 18]);
		}
	}

	void setStateValuesFromBuffer(unsigned otherSubdomain)
	{
		if (mirrorIntersections.size() <= otherSubdomain or stateBuffer.size() <= otherSubdomain)
			LOG_ERROR(
			        "inconsistent size of mirrorIntersections and/or stateBuffer, " << mirrorIntersections.size() << " " << otherSubdomain << " "
			                                                                        << stateBuffer.size() << " "
			                                                                        << Omega::instance().getScene()->subdomain);
		setStateValuesFromIds(mirrorIntersections[otherSubdomain], stateBuffer[otherSubdomain]);
	}

	void mpiSendStates(/*vector<double> message,*/ unsigned otherSubdomain)
	{
		std::vector<Real> vals = getStateValues(otherSubdomain);
		MPI_Send(&vals.front(), vals.size(), MPI_DOUBLE, otherSubdomain, 177, selfComm());
	}

	void mpiRecvStates(unsigned otherSubdomain)
	{
		if (mirrorIntersections.size() <= otherSubdomain)
			LOG_ERROR("inconsistent size of mirrorIntersections and/or stateBuffer");
		if (stateBuffer.size() <= otherSubdomain)
			stateBuffer.resize(otherSubdomain + 1);
		const vector<Body::id_t>& b_ids = mirrorIntersections[otherSubdomain];
		unsigned                  nb    = b_ids.size() * 13;
		vector<Real>&             vals  = stateBuffer[otherSubdomain];
		vals.resize(nb);
		int        recv_count;
		MPI_Status recv_status;
		MPI_Recv(&vals.front(), nb, MPI_DOUBLE, otherSubdomain, 177, selfComm(), &recv_status);
		MPI_Get_count(&recv_status, MPI_DOUBLE, &recv_count);
		if (recv_count != int(nb))
			LOG_ERROR("length mismatch");
	}

	void mpiIrecvStates(unsigned otherSubdomain)
	{
		if (mirrorIntersections.size() <= otherSubdomain)
			LOG_ERROR("inconsistent size of mirrorIntersections and/or stateBuffer");
		if (stateBuffer.size() <= otherSubdomain)
			stateBuffer.resize(otherSubdomain + 1);
		if (mpiReqs.size() <= otherSubdomain)
			mpiReqs.resize(otherSubdomain + 1);

		const vector<Body::id_t>& b_ids = mirrorIntersections[otherSubdomain];
		unsigned                  nb    = b_ids.size() * 13;
		vector<Real>&             vals  = stateBuffer[otherSubdomain];
		vals.resize(nb);

		MPI_Irecv(&vals.front(), nb, MPI_DOUBLE, otherSubdomain, 177, selfComm(), &mpiReqs[otherSubdomain]);
	}


	void mpiWaitReceived(unsigned otherSubdomain) { MPI_Wait(&mpiReqs[otherSubdomain], MPI_STATUS_IGNORE); }

	//WARNING: precondition: the members bounds have been dispatched already, else we re-use old values. Carefull if subdomain is not at the end of O.bodies
	void setMinMax();

	// Functions dpk

	//functions (master)

	void recvBodyContainersFromWorkers();
	void initMasterContainer();
	bool allocContainerMaster = false; // flag

	//functions common

	void mergeOp();
	void splitBodiesToWorkers(const bool&);

	// functions (workers)

	void sendContainerString();
	void recvContainerString();
	void processContainerStrings(std::vector<char*>&, std::vector<int>&);
	void processContainerStrings();
	void sendAllBodiesToMaster();
	void sendBodies(const int receiver, const vector<Body::id_t>& idsToSend);
	void receiveBodies(const int sender);
	void setCommunicationContainers();
	void completeSendBodies();
	void setBodiesToBodyContainer(
	        Scene*,
	        std::vector<shared_ptr<MPIBodyContainer>>&,
	        bool,
	        bool overwriteBodies = false); //scene, vector of mpibodycontainers, set deleted bodies ?

	//communications util functions

	void sendStringBlocking(std::string&, int, int);       //string, rank, tag
	void sendString(std::string&, int, int, MPI_Request&); //non blocking send
	void recvBuff(char*, int, int, MPI_Request&);          // non blcoking recv
	int  probeIncoming(int, int);                          //non blocking probe, for getting char/string size
	void processReqs(std::vector<MPI_Request>&);           // process Requests (MPI_Waitall(request, status))
	void resetReqs(std::vector<MPI_Request>&);             // clear the vector of mpiReqs.
	int  probeIncomingBlocking(int, int);                  // blocking probe -> gets size of char/string : source rank, message tag.
	void recvBuffBlocking(char*, int, int, int);           // blockng recv : char buff, buff size, message tag, source rank
	void processReqsAll(
	        std::vector<MPI_Request>&, std::vector<MPI_Status>&); // clears vecs of MPI_Status & MPI_Request (non blocking sends & non blocking ecvs. )

	//Util functions -> serialization, deserializtion, setting body ids to a subdomain (aka to the member std::vector<Body::d_t> ids) and another function for  clearing it

	std::string                  serializeMPIBodyContainer(const shared_ptr<MPIBodyContainer>&); //serialize and return string
	shared_ptr<MPIBodyContainer> deSerializeMPIBodyContainer(const char*, int);                  //deserialize return MPIBodyContainer
	std::string                  fillContainerGetString(shared_ptr<MPIBodyContainer>&, const std::vector<Body::id_t>&);
	std::string                  idsToSerializedMPIBodyContainer(const std::vector<Body::id_t>& ids);
	void                         setIDstoSubdomain(boost::python::list&); // exposed to yadempi.py, function to change/reset the ids in a subdomain
	void                         clearSubdomainIds();                     // clears the member ids (std::vector <Body::id_t>
	void                         getRankSize();
	void                         clearRecvdCharBuff(std::vector<char*>&); // frees std::vector<char*>
	void                         setSubdomainIds(std::vector<Body::id_t>);
	std::vector<Body::id_t>      getSubdomainIds();
	void                         append(Body::id_t);
	void                         appendList(const boost::python::list&);


	//declarations dpk
	std::vector<std::pair<std::string, int>> sendContainer; // list containing the serialized data (string) to be sent with the destination rank.
	int  subdomainRank, commSize; //NOTE: subdomainRank is redundant with scene->subdomain, would be better to avoid to be sure they don't diverge
	int  TAG_STRING = 420, TAG_COUNT = 20, TAG_WALL_INTR = 100, TAG_FORCE = 200, TAG_BODY = 111, TAG_INTERSECTIONS = 112;
	bool commContainer   = false;
	bool containersRecvd = false;
	std::vector<shared_ptr<MPIBodyContainer>> recvdBodyContainers;
	std::vector<bool>                         commFlag;
	const int                                 master = 0;
	std::vector<int>                          wallIdsM;
	bool                                      initDone = false;
	std::vector<int>                          recvdStringSizes; // a buffer to hold the size of the incoming messages (char buffers).
	std::vector<MPI_Request>                  recvReqs;         // used for MPI_Irecv
	std::vector<char*>                        recvdCharBuff;
	std::vector<std::string>                  stringBuff;
	std::vector<int>                          recvRanks;
	std::vector<int>                          remoteCount;
	bool                                      ranksSet  = false;
	bool                                      bodiesSet = false; // flag

	// Geometry and other helper functions
	Real boundOnAxis(Bound& b, const Vector3r& direction, bool min)
	        const; //return projected extremum of an AABB in a particular direction (in the the '+' or '-' sign depending on 'min' )

	Vector3r centerOfMass() const
	{
		Vector3r                 center(0, 0, 0);
		Real                     mass  = 0;
		const shared_ptr<Scene>& scene = Omega::instance().getScene();
		for (unsigned k = 0; k < ids.size(); k++) {
			const shared_ptr<Body>& b = Body::byId(ids[k], scene);
			if (!b or b->getIsSubdomain())
				continue;
			center += b->state->mass * b->state->pos;
			mass += b->state->mass;
		}
		return center * (1 / mass);
	}
	// Count interactions of a body with given subdomain
	unsigned countIntsWith(Body::id_t body, Body::id_t someSubD, const shared_ptr<Scene>& scene) const
	{
		if (not Body::byId(body, scene)) {
			LOG_WARN("invalid body id");
			return 0;
		}
		const auto& intrs = Body::byId(body, scene)->intrs;
		return std::count_if(intrs.begin(), intrs.end(), [&](auto i) {
			assert(scene->bodies->exists(i.first));
			return (Body::byId(i.first, scene)->subdomain == someSubD and not Body::byId(i.first, scene)->getIsSubdomain());
		});
	}

	vector<Body::id_t> filteredInts(Body::id_t someSubD, bool mirror) const
	{
		auto&                    intrs = mirror ? mirrorIntersections[someSubD] : intersections[someSubD];
		std::vector<Body::id_t>  filtered;
		const shared_ptr<Scene>& scene = Omega::instance().getScene();
		std::copy_if(intrs.begin(), intrs.end(), std::back_inserter(filtered), [&](auto i) {
			return (this->countIntsWith(i, mirror ? scene->subdomain : someSubD, scene) > 0);
		});
		return filtered;
	}

	double filterIntersections(const shared_ptr<Scene>& scene)
	{
		// we don't touch intersections with zero yet, unsure it would work directly as it's a bit special
		assert(intersections.size() == mirrorIntersections.size());
		assert(scene->subdomain > 0); // this function should not be called by master
		unsigned oldNum(0), newNum(0);
		for (unsigned subd = 1; subd < intersections.size(); subd++)
			if (Body::id_t(subd) != scene->subdomain) {
				// PLEASE DON'T REMOVE THOSE COMMENTED DEBUG MESSAGES
				// 			unsigned oldI(intersections[subd].size()), oldM(mirrorIntersections[subd].size());
				oldNum += intersections[subd].size();
				mirrorIntersections[subd] = filteredInts(subd, true);
				intersections[subd]       = filteredInts(subd, false);
				newNum += intersections[subd].size();
				// 			LOG_WARN("SubD "<<scene->subdomain<<" suppressed "<<(oldI-intersections[subd].size())<<" / "<<oldI<<"  vs. "<<subd);
				// 			LOG_WARN("SubD "<<scene->subdomain<<" suppressed "<<(oldM-mirrorIntersections[subd].size())<<" / "<<oldM<<" from "<<subd);
			}
		// 		LOG_WARN("SubD "<<scene->subdomain<<" suppressed "<<oldNum-newNum<<" / "<<oldNum);
		return (oldNum ? (oldNum - newNum) / double(oldNum) : 0); //return overall ratio of removed elements (low means useless)
	}

	// body reallocation

	std::vector<yade::projectedBoundElem> projectedBoundsCPP(int, const Vector3r&, const Vector3r&, bool useAABB = false);
	std::vector<Body::id_t>               medianFilterCPP(boost::python::list&, int otherSD, const Vector3r&, const Vector3r&, bool useAABB = false);
	void                                  migrateBodiesSend(const std::vector<Body::id_t>&, int);
	void                                  updateLocalIds(bool);
	void                                  cleanIntersections(int);
	void                                  updateNewMirrorIntrs(int, const std::vector<Body::id_t>&);
	void                                  getMirrorIntersections();


	// clang-format off
	YADE_CLASS_BASE_DOC_ATTRS_CTOR_PY(Subdomain,Shape,"The bounding box of a mpi subdomain. Stores internals and provides optimized functions for communications between workers. This class may not be used directly. Instead, Subdomains are appended automatically to the scene bodies when using :yref:`mpy.mpirun`",
// 		((testType, testArray,testType({0,0}),,""))
		((Real,extraLength,0,,"verlet dist for the subdomain, added to bodies verletDist"))
		((Vector3r,boundsMin,Vector3r(NaN,NaN,NaN),,"min corner of all bboxes of members; differs from effective domain bounds by the extra length (sweepLength)"))
		((Vector3r,boundsMax,Vector3r(NaN,NaN,NaN),,"max corner of all bboxes of members; differs from effective domain bounds by the extra length (sweepLength)"))
		((IntersectionMap,intersections,IntersectionMap(),Attr::hidden,"[will be overridden below]"))
		((IntersectionMap,mirrorIntersections,IntersectionMap(),Attr::hidden,"[will be overridden below]"))
		((vector<Body::id_t>,ids,vector<Body::id_t>(),,"Ids of owned particles."))
		((vector<vector<Real> >,stateBuffer,vector<vector<Real> >(),(Attr::noSave | Attr::hidden),"container storing data from other subdomains")) 
		((vector<Body::id_t>, subdomains, vector<Body::id_t> (), (Attr::noSave | Attr::hidden), "subdomain ids (by body id)"))
		,/*ctor*/ createIndex(); myComm_p=NULL;
		,/*py*/ /*.add_property("members",&Clump::members_get,"Return clump members as {'id1':(relPos,relOri),...}")*/
		.def("init",&Subdomain::init,"Initialize subdomain variables as rank and buffer sizes, call this from each thread after scene distribution by master.")
		.def("setMinMax",&Subdomain::setMinMax,"returns bounding min-max based on members bounds. precondition: the members bounds have been dispatched already, else we re-use old values. Carefull if subdomain is not at the end of O.bodies.")
		.def("getStateValues",&Subdomain::getStateValues,(boost::python::arg("otherDomain")),"returns pos,vel,angVel,ori of bodies interacting with a given otherDomain, based on :yref:`Subdomain.intersections`.")
		.def("getStateValuesFromIds",&Subdomain::getStateValuesFromIds,(boost::python::arg("b_ids")),"returns pos,vel,angVel,ori of listed bodies.")
		.def("setStateValuesFromIds",&Subdomain::setStateValuesFromIds,(boost::python::arg("b_ids"),boost::python::arg("input")),"set pos,vel,angVel,ori from listed body ids and data.")
		.def("getStateBoundsValuesFromIds",&Subdomain::getStateBoundsValuesFromIds,(boost::python::arg("b_ids")),"returns pos,vel,angVel,ori,bounds of listed bodies.")
		.def("setStateBoundsValuesFromIds",&Subdomain::setStateBoundsValuesFromIds,(boost::python::arg("b_ids"),boost::python::arg("input")),"set pos,vel,angVel,ori,bounds from listed body ids and data.")
// 		.def("getStateValuesFromIdsAsArray",&Subdomain::getStateValuesFromIdsAsArray,(boost::python::arg("otherDomain")),"returns 1D numpy array of pos,vel,angVel,ori of bodies interacting with a given otherDomain, based on :yref:`Subdomain.intersections`.")
		.def("setStateValuesFromBuffer",&Subdomain::setStateValuesFromBuffer,(boost::python::arg("subdomain")),"set pos,vel,angVel,ori from state buffer.")
		.def("mpiSendStates",&Subdomain::mpiSendStates,(boost::python::arg("otherSubdomain")),"mpi-send states from current domain to another domain (blocking)")
		.def("mpiRecvStates",&Subdomain::mpiRecvStates,(boost::python::arg("otherSubdomain")),"mpi-recv states from another domain  (blocking)")
		.def("mpiIrecvStates",&Subdomain::mpiIrecvStates,(boost::python::arg("otherSubdomain")),"mpi-Irecv states from another domain  (non-blocking)")
		.def("mpiWaitReceived",&Subdomain::mpiWaitReceived,(boost::python::arg("otherSubdomain")),"mpi-Wait states from another domain (upon return the buffer is set)")		
                .def("mergeOp",&Subdomain::mergeOp,"merge with setting interactions")		
		.def("sendBodies",&Subdomain::sendBodies,(boost::python::arg("sender"),boost::python::arg("receiver"),boost::python::arg("idsToSend")), "Copy the bodies from MPI sender rank to MPI receiver rank")
		.def("receiveBodies",&Subdomain::receiveBodies,(boost::python::arg("sender")), "Receive the bodies from MPI sender rank to MPI receiver rank")
                .add_property("intersections",&Subdomain::intrs_get,&Subdomain::intrs_set,"lists of bodies from this subdomain intersecting other subdomains. WARNING: only assignement and concatenation allowed")
                .def("getRankSize", &Subdomain::getRankSize, "set subdomain ranks, used for communications -> merging, sending bodies etc.")
		.def("completeSendBodies", &Subdomain::completeSendBodies, "calls MPI_wait to complete the non blocking sends/recieves.")
		.add_property("mirrorIntersections",&Subdomain::mIntrs_get,&Subdomain::mIntrs_set,"lists of bodies from other subdomains intersecting this one. WARNING: only assignement and concatenation allowed")
		.add_property("comm",&Subdomain::getMyComm,&Subdomain::setMyComm,"Communicator to be used for MPI (converts mpi4py comm <-> c++ comm)")
		.add_property("subdomains", &Subdomain::getSubdomainIds, &Subdomain::setSubdomainIds, "subdomain ids of other bodies, WARNING: only assignement and concatenation allowed ")
		.def("splitBodiesToWorkers", &Subdomain::splitBodiesToWorkers,(boost::python::arg("eraseWorkerBodies")), "of true bodies in workers are erased and reassigned.")
		.def("boundOnAxis", &Subdomain::boundOnAxis,(boost::python::arg("bound"),boost::python::arg("axis"),boost::python::arg("min")), "computes projected position of a bound in a certain direction")
		.def("centerOfMass", &Subdomain::centerOfMass, "returns center of mass of assigned bodies")
		.def("countIntsWith", &Subdomain::countIntsWith, (boost::python::arg("body"),boost::python::arg("someSubDomain"),boost::python::arg("someSubDomain")=Omega::instance().getScene()), "returns for a body the count of interactions (real or virtual) with bodies from a certain subdomain, interactions with subdomains excluded. Third parameter (scene pointer) can be left to default (equivalent to O._sceneObj).")
		.def("filteredInts", &Subdomain::filteredInts, (boost::python::arg("someSubDomain"),boost::python::arg("mirror")), "return a copy of intersections or mirrorIntersections from which non-interacting bodies have been removed.")
		.def("filterIntersections", &Subdomain::filterIntersections,(boost::python::arg("someSubDomain")=Omega::instance().getScene()),"clear intersections and mirror intersections of all non-interacting bodies.")
		.def("setIDstoSubdomain", &Subdomain::setIDstoSubdomain, (boost::python::arg("idList")), "set list of ids to the subdomain." )
		.def("boundOnAxis", &Subdomain::boundOnAxis,(boost::python::arg("bound"),boost::python::arg("axis"),boost::python::arg("min")), "computes projected position of a bound in a certain direction")
		.def("centerOfMass", &Subdomain::centerOfMass, "returns center of mass of assigned bodies")
		.def("medianFilterCPP", &Subdomain::medianFilterCPP, (boost::python::arg("bodiesToRecv"), boost::python::arg("otherSubdomain"), boost::python::arg("oterSubdomainCenterofMass"), boost::python::arg("useAABB")), "cpp version of median filter, used for body reallocation operations. ")
		.def("migrateBodiesSend", &Subdomain::migrateBodiesSend, (boost::python::arg("bodiesToSend"), boost::python::arg("destination")), "ids of body to be sent have their subdomain parameter reassigned, followed by sendBodies")
		.def("updateLocalIds", &Subdomain::updateLocalIds, (boost::python::arg("eraseRemoteMastrer")), "updates the ids in the subdomain id vector, if not eraseRemoteMastrer, body->subdomain in master are updated.")
		.def("cleanIntersections",&Subdomain::cleanIntersections, (boost::python::arg("otherDomain")), "makes sure that the ids in the current subdomain belong to the current subdomain")
		.def("updateNewMirrorIntrs", &Subdomain::updateNewMirrorIntrs, (boost::python::arg("otherdomain"), boost::python::arg("newMirrorList")), "update the mirrorIntersections of a specific subdomain")
		.def("getMirrorIntrs", &Subdomain::getMirrorIntersections,"get mirrorIntersections from other subdomains")
	);
	// clang-format on
	DECLARE_LOGGER;
	REGISTER_CLASS_INDEX(Subdomain, Shape);
};
REGISTER_SERIALIZABLE(Subdomain);

class Bo1_Subdomain_Aabb : public BoundFunctor {
public:
	void go(const shared_ptr<Shape>& cm, shared_ptr<Bound>& bv, const Se3r& se3, const Body*);
	FUNCTOR1D(Subdomain);
	// clang-format off
	YADE_CLASS_BASE_DOC(Bo1_Subdomain_Aabb,BoundFunctor,"Creates/updates an :yref:`Aabb` of a :yref:`Facet`.");
	// clang-format on
	DECLARE_LOGGER;
};
REGISTER_SERIALIZABLE(Bo1_Subdomain_Aabb);


} // namespace yade
