// 2010 © Václav Šmilauer <eudoxos@arcig.cz>

#include "BodyContainer.hpp"
#include "Body.hpp"
#include "Clump.hpp"
#include "Scene.hpp"
#ifdef YADE_OPENMP
#include <omp.h>
#endif

namespace yade { // Cannot have #include directive inside.


CREATE_LOGGER(BodyContainer);

Body::id_t BodyContainer::insert(shared_ptr<Body> b)
{
	const shared_ptr<Scene>& scene = Omega::instance().getScene();
	b->iterBorn                    = scene->iter;
	b->timeBorn                    = scene->time;
	b->id                          = body.size();
	scene->doSort                  = true;

	if (enableRedirection) {
		insertedBodies.push_back(b->id);
		dirty             = true;
		checkedByCollider = false;
	}
	body.push_back(b);
	// Notify ForceContainer about new id
	scene->forces.addMaxId(b->id);
	return b->id;
}

Body::id_t BodyContainer::insertAtId(shared_ptr<Body> b, Body::id_t candidate)
{
	if (not b)
		LOG_ERROR("Inserting null body");
	const shared_ptr<Scene>& scene = Omega::instance().getScene();
	if (enableRedirection) {
		dirty             = true;
		checkedByCollider = false;
		useRedirection    = true;
		insertedBodies.push_back(candidate); /*realBodies.push_back(candidate); */
	}                                            // if that special insertion is used, switch to algorithm optimized for non-full body container
	if (unsigned(candidate) >= size()) {
		body.resize(candidate + 1, nullptr);
		scene->forces.addMaxId(candidate);
	} else if (body[candidate]) {
		LOG_ERROR("invalid candidate id: " << candidate);
		return -1;
	}

	b->iterBorn   = scene->iter;
	b->timeBorn   = scene->time;
	b->id         = candidate;
	body[b->id]   = b;
	scene->doSort = true;
	return b->id;
}

void BodyContainer::clear()
{
	body.clear();
	dirty             = true;
	checkedByCollider = false;
}

BodyContainer::iterator BodyContainer::begin() { return iterator(body.begin(), body.end()); }

BodyContainer::iterator BodyContainer::end() { return iterator(body.end(), body.end()); }

size_t BodyContainer::size() const { return body.size(); }

shared_ptr<Body>& BodyContainer::operator[](unsigned int id) { return body[id]; }
const shared_ptr<Body>& BodyContainer::operator[](unsigned int id) const { return body[id]; }

bool BodyContainer::exists(Body::id_t id) const { return ((id >= 0) && ((size_t)id < body.size()) && ((bool)body[id])); }

bool BodyContainer::erase(Body::id_t id, bool eraseClumpMembers)
{ //default is false (as before)
	if (!body[id])
		return false;
	if (enableRedirection) {
		useRedirection    = true;
		dirty             = true;
		checkedByCollider = false;
	} // as soon as a body is erased we switch to algorithm optimized for non-full body container
	const shared_ptr<Body>& b = Body::byId(id);
	if ((b) and (b->isClumpMember())) {
		const shared_ptr<Body>  clumpBody = Body::byId(b->clumpId);
		const shared_ptr<Clump> clump     = YADE_PTR_CAST<Clump>(clumpBody->shape);
		Clump::del(clumpBody, b);
		if (clump->members.size() == 0)
			this->erase(clumpBody->id, false); //Clump has no members any more. Remove it
	}

	if ((b) and (b->isClump())) {
		//erase all members if eraseClumpMembers is true:
		const shared_ptr<Clump>&    clump   = YADE_PTR_CAST<Clump>(b->shape);
		std::map<Body::id_t, Se3r>& members = clump->members;
		std::vector<Body::id_t>     idsToRemove;
		for (const auto mm : members)
			idsToRemove.push_back(mm.first); // Prepare an array of ids, which need to be removed.
		for (Body::id_t memberId : idsToRemove) {
			if (eraseClumpMembers) {
				this->erase(memberId, false); // erase members
			} else {
				//when the last members is erased, the clump will be erased automatically, see above
				Body::byId(memberId)->clumpId = Body::ID_NONE; // make members standalones
			}
		}
		body[id].reset();
		return true;
	}
	const shared_ptr<Scene>& scene = Omega::instance().getScene();
	for (auto it = b->intrs.begin(), end = b->intrs.end(); it != end;) { //Iterate over all body's interactions
		Body::MapId2IntrT::iterator willBeInvalid = it;
		++it;
		scene->interactions->erase(willBeInvalid->second->getId1(), willBeInvalid->second->getId2(), willBeInvalid->second->linIx);
	}
	b->id = -1; //else it sits in the python scope without a chance to be inserted again
	body[id].reset();
	return true;
}

void BodyContainer::updateShortLists()
{
	if (not useRedirection) {
#ifdef YADE_MPI
		subdomainBodies.clear();
#endif
		realBodies.clear();
		return;
	}
	if (not dirty)
		return; //already ok
	unsigned long size1 = realBodies.size();
	realBodies.clear();
	realBodies.reserve((long unsigned)(size1 * 1.3));
#ifdef YADE_MPI
	unsigned long size2 = subdomainBodies.size();
	subdomainBodies.clear();
	subdomainBodies.reserve((long unsigned)(size2 * 1.3));
	const int& subdomain = Omega::instance().getScene()->subdomain;
#endif
	for (const auto& b : *(Omega::instance().getScene()->bodies)) {
		if (not b)
			continue;
		realBodies.push_back(b->getId());
#ifdef YADE_MPI
		if (b->subdomain == subdomain and not b->getIsSubdomain())
			subdomainBodies.push_back(b->id);
#endif
	}
	dirty = false;
}


} // namespace yade