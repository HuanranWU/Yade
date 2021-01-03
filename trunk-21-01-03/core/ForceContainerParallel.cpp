// 2009 © Václav Šmilauer <eudoxos@arcig.cz>
// 2019 © Bruno Chareyre <bruno.chareyre@grenoble-inp.fr>

#include <core/BodyContainer.hpp>
#include <core/ForceContainer.hpp>
#include <core/Scene.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

namespace yade { // Cannot have #include directive inside.

CREATE_LOGGER(ForceContainer);

void ForceContainer::ensureSynced()
{
	if (!synced)
		throw runtime_error("ForceContainer not thread-synchronized; call sync() first!");
}

void ForceContainer::addForceUnsynced(Body::id_t id, const Vector3r& f)
{
	assert((size_t)id < size);
	_force[id] += f;
}

void ForceContainer::addTorqueUnsynced(Body::id_t id, const Vector3r& m)
{
	assert((size_t)id < size);
	_torque[id] += m;
}

void ForceContainer::resizePerm(size_t newSize)
{
	if (newSize < _permForce.size())
		LOG_WARN("permForce may have been assigned to an id larger than maxId, and will be ignored in that case");
	if (newSize > _permForce.size()) {
		_permForce.reserve(size_t(1.5 * newSize));
		_permTorque.reserve(size_t(1.5 * newSize));
		_permForce.resize(newSize, Vector3r::Zero());
		_permTorque.resize(newSize, Vector3r::Zero());
		syncedSizes = false;
	}
}

#ifdef YADE_OPENMP
#include <omp.h>
void ForceContainer::ensureSize(Body::id_t id, int threadN)
{
	if (threadN < 0) {
		if (id >= Body::id_t(_permForce.size()))
			resizePerm(id + 1);

	} else {
		_maxId[threadN] = std::max(_maxId[threadN], id);
		if (not(sizeOfThreads[threadN] > unsigned(_maxId[threadN])))
			resize(_maxId[threadN] + 1, threadN);
	}
}

ForceContainer::ForceContainer()
{
	nThreads = omp_get_max_threads();
	size     = 0;
	for (int i = 0; i < nThreads; i++) {
		_forceData.push_back(vvector());
		_torqueData.push_back(vvector());
		sizeOfThreads.push_back(0);
		_maxId.push_back(0);
	}
}

const Vector3r& ForceContainer::getForce(Body::id_t id)
{
	ensureSynced();
	return ((size_t)id < size) ? _force[id] : _zero;
}

void ForceContainer::addForce(Body::id_t id, const Vector3r& f)
{
	ensureSize(id, omp_get_thread_num());
	synced = false;
	_forceData[omp_get_thread_num()][id] += f;
}

const Vector3r& ForceContainer::getTorque(Body::id_t id)
{
	ensureSynced();
	return ((size_t)id < size) ? _torque[id] : _zero;
}

void ForceContainer::addTorque(Body::id_t id, const Vector3r& t)
{
	ensureSize(id, omp_get_thread_num());
	synced = false;
	_torqueData[omp_get_thread_num()][id] += t;
}

void ForceContainer::addMaxId(Body::id_t id)
{
	if (_maxId[omp_get_thread_num()] < id)
		synced = false;
	_maxId[omp_get_thread_num()] = std::max(id, _maxId[omp_get_thread_num()]);
}

void ForceContainer::setPermForce(Body::id_t id, const Vector3r& f)
{
	ensureSize(id, -1);
	if (permForceSynced) { // current permF has been accumulated already, substract it form total then add new value; else just set the value below
		addForceUnsynced(id, f - _permForce[id]);
	}
	_permForce[id] = f;
	if (not permForceUsed) {
		synced        = false;
		permForceUsed = true;
	}
}

void ForceContainer::setPermTorque(Body::id_t id, const Vector3r& t)
{
	ensureSize(id, -1);
	if (permForceSynced) { // current permF has been accumulated already, substract it form total then add new value; else just set the value below
		addTorqueUnsynced(id, t - _permTorque[id]);
	}
	_permTorque[id] = t;
	if (not permForceUsed) {
		synced        = false;
		permForceUsed = true;
	}
}

const Vector3r& ForceContainer::getPermForce(Body::id_t id) { return ((size_t)id < size) ? _permForce[id] : _zero; }

const Vector3r& ForceContainer::getPermTorque(Body::id_t id) { return ((size_t)id < size) ? _permTorque[id] : _zero; }

const Vector3r& ForceContainer::getForceUnsynced(Body::id_t id)
{
	assert((size_t)id < size);
	return _force[id];
}

const Vector3r& ForceContainer::getTorqueUnsynced(Body::id_t id)
{
	assert((size_t)id < size);
	return _torque[id];
}

const Vector3r ForceContainer::getForceSingle(Body::id_t id)
{
	Vector3r ret(Vector3r::Zero());
	for (int t = 0; t < nThreads; t++) {
		ret += ((size_t)id < sizeOfThreads[t]) ? _forceData[t][id] : _zero;
	}
	if (permForceUsed)
		ret += _permForce[id];
	return ret;
}

const Vector3r ForceContainer::getTorqueSingle(Body::id_t id)
{
	Vector3r ret(Vector3r::Zero());
	for (int t = 0; t < nThreads; t++) {
		ret += ((size_t)id < sizeOfThreads[t]) ? _torqueData[t][id] : _zero;
	}
	if (permForceUsed)
		ret += _permTorque[id];
	return ret;
}

void ForceContainer::sync()
{
	if (synced)
		return;
	const std::lock_guard<std::mutex> lock(globalMutex);
	if (synced)
		return; // if synced meanwhile

	syncSizesOfContainers();
	const bool  redirect   = Omega::instance().getScene()->bodies->useRedirection;
	const auto& realBodies = Omega::instance().getScene()->bodies->realBodies;
	if (redirect)
		Omega::instance().getScene()->bodies->updateShortLists();
	const unsigned long len = redirect ? (unsigned long)realBodies.size() : (unsigned long)size;

#pragma omp parallel for schedule(static)
	for (unsigned long k = 0; k < len; k++) {
		const Body::id_t& id = redirect ? realBodies[k] : k;
		Vector3r          sumF(Vector3r::Zero()), sumT(Vector3r::Zero());
		for (int thread = 0; thread < nThreads; thread++) {
			sumF += _forceData[thread][id];
			sumT += _torqueData[thread][id];
			_forceData[thread][id]  = Vector3r::Zero();
			_torqueData[thread][id] = Vector3r::Zero();
		} //reset here so we don't have to do it later
		_force[id] += sumF;
		_torque[id] += sumT;
		if (permForceUsed and not permForceSynced) {
			_force[id] += _permForce[id];
			_torque[id] += _permTorque[id];
		}
	}
	permForceSynced = true;
	synced          = true;
	syncCount++;
}

#if (YADE_REAL_BIT <= 64)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpragmas"
// this is to remove warning about manipulating raw memory
#pragma GCC diagnostic ignored "-Wclass-memaccess"

void ForceContainer::reset(long iter, bool resetAll)
{
	syncSizesOfContainers();
	const shared_ptr<Scene>& scene = Omega::instance().getScene();
	size_t                   currSize;
	if (scene->bodies->useRedirection) { //if using short lists we only reset forces for bodies not-vanished
		scene->bodies->updateShortLists();
		const auto& sdIds = scene->bodies->realBodies;
		currSize          = sdIds.size();
// no need to reset force-torque per thread since they are set to zero in sync() already
#pragma omp parallel for schedule(static)
		for (unsigned long k = 0; k < currSize; k++) /*_force[sdIds[k]]=Vector3r::Zero(); */
			memset(&_force[sdIds[k]], 0, sizeof(Vector3r));
#pragma omp parallel for schedule(static)
		for (unsigned long k = 0; k < currSize; k++) /*_torque[sdIds[k]]=Vector3r::Zero();*/
			memset(&_torque[sdIds[k]], 0, sizeof(Vector3r));
		// those need a reset per-thread OTOH
		if (resetAll) {
			for (unsigned long k = 0; k < currSize; k++)
				_permForce[sdIds[k]] = Vector3r::Zero();
			for (unsigned long k = 0; k < currSize; k++)
				_permTorque[sdIds[k]] = Vector3r::Zero();
			permForceUsed = false;
		}

	} else { // else reset everything
		memset(&_force[0], 0, sizeof(Vector3r) * size);
		memset(&_torque[0], 0, sizeof(Vector3r) * size);
	}

	if (resetAll and permForceUsed) {
		memset(&_permForce[0], 0, sizeof(Vector3r) * size);
		memset(&_permTorque[0], 0, sizeof(Vector3r) * size);
		permForceUsed = false;
	}

	if (!permForceUsed)
		synced = true;
	else
		synced = false;
	permForceSynced = false;
	lastReset       = iter;
}

#pragma GCC diagnostic pop

#else
// The same function, but don't use memset
void ForceContainer::reset(long iter, bool resetAll)
{
	syncSizesOfContainers();
	const shared_ptr<Scene>& scene = Omega::instance().getScene();
	size_t                   currSize;
	if (scene->bodies->useRedirection) { //if using short lists we only reset forces for bodies not-vanished
		scene->bodies->updateShortLists();
		const auto& sdIds = scene->bodies->realBodies;
		currSize          = sdIds.size();
// no need to reset force-torque per thread since they are set to zero in sync() already
#pragma omp parallel for schedule(static)
		for (unsigned long k = 0; k < currSize; k++)
			_force[sdIds[k]] = Vector3r::Zero();
#pragma omp parallel for schedule(static)
		for (unsigned long k = 0; k < currSize; k++)
			_torque[sdIds[k]] = Vector3r::Zero();
		// those need a reset per-thread OTOH
		if (resetAll) {
			for (unsigned long k = 0; k < currSize; k++)
				_permForce[sdIds[k]] = Vector3r::Zero();
			for (unsigned long k = 0; k < currSize; k++)
				_permTorque[sdIds[k]] = Vector3r::Zero();
			permForceUsed = false;
		}
	} else { // else reset everything
		// the standard way, perfectly optimized by compiler.
		std::fill(_force.begin(), _force.end(), Vector3r::Zero());
		std::fill(_torque.begin(), _torque.end(), Vector3r::Zero());
	}
	if (resetAll and permForceUsed) {
		// the standard way, perfectly optimized by compiler.
		std::fill(_permForce.begin(), _permForce.end(), Vector3r::Zero());
		std::fill(_permTorque.begin(), _permTorque.end(), Vector3r::Zero());
		permForceUsed = false;
	}
	if (!permForceUsed)
		synced = true;
	else
		synced = false;
	lastReset = iter;
}
#endif

void ForceContainer::resize(size_t newSize, int threadN)
{
	if (sizeOfThreads[threadN] >= newSize)
		return;
	LOG_DEBUG("Resize ForceContainer from the size " << size << " to the size " << newSize);
	_forceData[threadN].reserve(size_t(newSize * 1.5));
	_torqueData[threadN].reserve(size_t(newSize * 1.5));
	_forceData[threadN].resize(newSize, Vector3r::Zero());
	_torqueData[threadN].resize(newSize, Vector3r::Zero());
	sizeOfThreads[threadN] = newSize;
	_maxId[threadN]        = newSize - 1;
	syncedSizes            = false;
}

int  ForceContainer::getNumAllocatedThreads() const { return nThreads; }
bool ForceContainer::getPermForceUsed() const { return permForceUsed; }

void ForceContainer::syncSizesOfContainers()
{
	//check whether all containers have equal length, and if not resize it
	size_t maxThreadSize = 0;
	for (int i = 0; i < nThreads; i++)
		maxThreadSize = std::max(maxThreadSize, size_t(_maxId[i] + 1));
	if (permForceUsed)
		maxThreadSize = std::max(maxThreadSize, size_t(_permForce.size()));
	if (maxThreadSize > size)
		syncedSizes = false;
	if (syncedSizes)
		return;
	size_t newSize = std::max(size, maxThreadSize);
	for (int i = 0; i < nThreads; i++)
		resize(newSize, i);

	if (newSize > size) {
		_force.reserve(size_t(newSize * 1.3));
		_torque.reserve(size_t(newSize * 1.3));
		_force.resize(newSize, Vector3r::Zero());
		_torque.resize(newSize, Vector3r::Zero());
	}
	if (permForceUsed)
		resizePerm(newSize);
	syncedSizes = true;
	size        = newSize;
}
#endif

} // namespace yade
