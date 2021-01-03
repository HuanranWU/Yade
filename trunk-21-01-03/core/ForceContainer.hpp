// 2009 © Václav Šmilauer <eudoxos@arcig.cz>
// 2019 © Bruno Chareyre <bruno.chareyre@grenoble-inp.fr>
#pragma once

#include <lib/base/Math.hpp>
#include <core/Body.hpp>

#include <boost/static_assert.hpp>

#ifdef YADE_OPENMP
#include <omp.h>
#endif

namespace yade { // Cannot have #include directive inside.

// make sure that (void*)&vec[0]==(void*)&vec
// we could use some `constexpr auto vectorSize=3; …… =4; depending on #ifdef EIGEN_DONT_ALIGN, then only have a single line here:
// BOOST_STATIC_ASSERT(sizeof(Vector3r)==vectorSize*sizeof(Real));
#ifdef EIGEN_DONT_ALIGN
BOOST_STATIC_ASSERT(sizeof(Vector3r) == 3 * sizeof(Real));
#else
// For start let's try SSE vectorization without AlignedVector3.
// Later we will use this line, where AlignedVector3 has size of four (last one unused, but still SEE potentially could give three-times speedup).
//BOOST_STATIC_ASSERT(sizeof(Vector3r)==4*sizeof(Real));
BOOST_STATIC_ASSERT(sizeof(Vector3r) == 3 * sizeof(Real));
#endif

/*! Container for Body External Variables (forces), typically forces and torques from interactions.
 * Values should be reset at every iteration by calling ForceContainer::reset();
 * If you want to add your own force type, you need to:
 *
 * 	1. Create storage vector
 * 	2. Create accessor function
 * 	3. Update the resize function
 * 	4. Update the reset function
 * 	5. update the sync function (for the multithreaded implementation)
 *
 * This class exists in two flavors: non-parallel and parallel. The parallel one stores
 * force increments separately for every thread and sums those when sync() is called.
 * The reason of this design is that the container is not truly random-access, but rather
 * is written to everywhere in one phase and read in the next one. Adding to force/torque
 * marks the container as dirty and sync() must be performed before reading the stored data.
 * Calling getForce/getTorque when the container is not synchronized throws an exception.
 *
 * It is intentional that sync() needs to be called exlicitly, since syncs are expensive and
 * the programmer should be aware of that. Sync is however performed only if the container
 * is dirty. Every full sync increments the syncCount variable, that should ideally equal
 * the number of steps (one per step).
 *
 * The number of threads (omp_get_max_threads) may not change once ForceContainer is constructed.
 *
 * The non-parallel flavor has the same interface, but sync() is no-op and synchronization
 * is not enforced at all.
 */

//! This is the parallel flavor of ForceContainer
class ForceContainer {
private:
	typedef std::vector<Vector3r> vvector;
#ifdef YADE_OPENMP
	std::vector<vvector>    _forceData;
	std::vector<vvector>    _torqueData;
	std::vector<Body::id_t> _maxId;
	std::vector<size_t>     sizeOfThreads;
	void                    ensureSize(Body::id_t id, int threadN);
#else
	void       ensureSize(Body::id_t id);
	Body::id_t _maxId = 0;
#endif
	vvector        _force, _torque, _permForce, _permTorque;
	size_t         size        = 0;
	bool           syncedSizes = true;
	int            nThreads;
	bool           permForceUsed   = false;
	bool           permForceSynced = false;
	std::mutex     globalMutex;
	const Vector3r _zero = Vector3r::Zero();

	void ensureSynced();

	// dummy function to avoid template resolution failure
	friend class boost::serialization::access;
	template <class ArchiveT> void serialize(ArchiveT& ar, unsigned int version) { }

public:
	bool          synced    = true;
	unsigned long syncCount = 0;
	long          lastReset = 0;
	ForceContainer();
	const Vector3r& getForce(Body::id_t id);
	void            addForce(Body::id_t id, const Vector3r& f);
	const Vector3r& getTorque(Body::id_t id);
	void            addTorque(Body::id_t id, const Vector3r& t);
	void            addMaxId(Body::id_t id);

	void            setPermForce(Body::id_t id, const Vector3r& f);
	void            setPermTorque(Body::id_t id, const Vector3r& t);
	const Vector3r& getPermForce(Body::id_t id);
	const Vector3r& getPermTorque(Body::id_t id);

	/*! Function to allow friend classes to get force even if not synced. Used for clumps by NewtonIntegrator.
		* Dangerous! The caller must know what it is doing! (i.e. don't read after write
		* for a particular body id. */
	const Vector3r& getForceUnsynced(Body::id_t id);
	const Vector3r& getTorqueUnsynced(Body::id_t id);
	void            addForceUnsynced(Body::id_t id, const Vector3r& f);
	void            addTorqueUnsynced(Body::id_t id, const Vector3r& m);

	/* To be benchmarked: sum thread data in getForce/getTorque upon request for each body individually instead of by the sync() function globally */
	// this function is used from python so that running simulation is not slowed down by sync'ing on occasions
	// since Vector3r writes are not atomic, it might (rarely) return wrong value, if the computation is running meanwhile
	const Vector3r getForceSingle(Body::id_t id);
	const Vector3r getTorqueSingle(Body::id_t id);

#ifdef YADE_OPENMP
	void syncSizesOfContainers();
	void resize(size_t newSize, int threadN);
#else
	void       resize(size_t newSize);
#endif
	/* Sum contributions from all threads, save to _force&_torque.
		 * Locks globalMutex, since one thread modifies common data (_force&_torque).
		 * Must be called before get* methods are used. Exception is thrown otherwise, since data are not consistent. */
	void sync();

	void resizePerm(size_t newSize);
	/*! Reset all resetable data, also reset summary forces/torques and mark the container clean.
		If resetAll, reset also user defined forces and torques*/
	// perhaps should be private and friend Scene or whatever the only caller should be
	void reset(long iter, bool resetAll = false);
	//! say for how many threads we have allocated space
	int  getNumAllocatedThreads() const;
	bool getPermForceUsed() const;

	DECLARE_LOGGER;
};

} // namespace yade
