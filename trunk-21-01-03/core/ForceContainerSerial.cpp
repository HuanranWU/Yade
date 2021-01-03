#ifndef YADE_OPENMP
#include <core/ForceContainer.hpp>

namespace yade { // Cannot have #include directive inside.

using math::max;
using math::min;

ForceContainer::ForceContainer() {};
void ForceContainer::ensureSize(Body::id_t id)
{
	const Body::id_t idMaxTmp = max(id, _maxId);
	_maxId                    = 0;
	if (size <= (size_t)idMaxTmp) {
		resize(min((size_t)1.5 * (idMaxTmp + 100), (size_t)(idMaxTmp + 2000)));
	};
}

const Vector3r& ForceContainer::getForce(Body::id_t id)
{
	ensureSize(id);
	return _force[id];
}

void ForceContainer::addForce(Body::id_t id, const Vector3r& f)
{
	ensureSize(id);
	_force[id] += f;
}

const Vector3r& ForceContainer::getTorque(Body::id_t id)
{
	ensureSize(id);
	return _torque[id];
}

void ForceContainer::addTorque(Body::id_t id, const Vector3r& t)
{
	ensureSize(id);
	_torque[id] += t;
}

void ForceContainer::addMaxId(Body::id_t id) { _maxId = id; }

void ForceContainer::setPermForce(Body::id_t id, const Vector3r& f)
{
	ensureSize(id);
	_permForce[id] = f;
	permForceUsed  = true;
}

void ForceContainer::setPermTorque(Body::id_t id, const Vector3r& t)
{
	ensureSize(id);
	_permTorque[id] = t;
	permForceUsed   = true;
}

const Vector3r& ForceContainer::getPermForce(Body::id_t id)
{
	ensureSize(id);
	return _permForce[id];
}

const Vector3r& ForceContainer::getPermTorque(Body::id_t id)
{
	ensureSize(id);
	return _permTorque[id];
}

const Vector3r& ForceContainer::getForceUnsynced(Body::id_t id) { return getForce(id); }

const Vector3r& ForceContainer::getTorqueUnsynced(Body::id_t id) { return getForce(id); }

const Vector3r ForceContainer::getForceSingle(Body::id_t id)
{
	ensureSize(id);
	if (permForceUsed) {
		return _force[id] + _permForce[id];
	} else {
		return _force[id];
	}
}
const Vector3r ForceContainer::getTorqueSingle(Body::id_t id)
{
	ensureSize(id);
	if (permForceUsed) {
		return _torque[id] + _permTorque[id];
	} else {
		return _torque[id];
	}
}

void ForceContainer::sync()
{
	if (_maxId > 0) {
		ensureSize(_maxId);
		_maxId = 0;
	}
	if (permForceUsed) {
		for (long id = 0; id < (long)size; id++) {
			_force[id] += _permForce[id];
			_torque[id] += _permTorque[id];
		}
	}
	return;
}

#if (YADE_REAL_BIT <= 64)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpragmas"
// this is to remove warning about manipulating raw memory
#pragma GCC diagnostic ignored "-Wclass-memaccess"
void                   ForceContainer::reset(long iter, bool resetAll)
{
	memset(&_force[0], 0, sizeof(Vector3r) * size);
	memset(&_torque[0], 0, sizeof(Vector3r) * size);
	if (resetAll) {
		memset(&_permForce[0], 0, sizeof(Vector3r) * size);
		memset(&_permTorque[0], 0, sizeof(Vector3r) * size);
		permForceUsed = false;
	}
	lastReset = iter;
}
#pragma GCC diagnostic pop
#else
void ForceContainer::reset(long iter, bool resetAll)
{
	// the standard way, perfectly optimized by compiler.
	std::fill(_force.begin(), _force.end(), Vector3r::Zero());
	std::fill(_torque.begin(), _torque.end(), Vector3r::Zero());
	if (resetAll) {
		std::fill(_permForce.begin(), _permForce.end(), Vector3r::Zero());
		std::fill(_permTorque.begin(), _permTorque.end(), Vector3r::Zero());
		permForceUsed = false;
	}
	lastReset = iter;
}
#endif

void ForceContainer::resize(size_t newSize)
{
	_force.resize(newSize, Vector3r::Zero());
	_torque.resize(newSize, Vector3r::Zero());
	_permForce.resize(newSize, Vector3r::Zero());
	_permTorque.resize(newSize, Vector3r::Zero());
	size = newSize;
}

int  ForceContainer::getNumAllocatedThreads() const { return 1; }
bool ForceContainer::getPermForceUsed() const { return permForceUsed; }

} // namespace yade

#endif
