
#include <core/Body.hpp>
#include <core/InteractionContainer.hpp>
#include <core/Omega.hpp>
#include <core/Scene.hpp>

namespace yade { // Cannot have #include directive inside.

//! This could be -1 if id_t is re-typedef'ed as `int'
// note about possible switch to std::optional / boost::optional and std::nullopt / boost::none here:
//    InsertionSortCollider::insertionSortParallel rests on assuption that this is signed. Negative values are used.
const Body::id_t Body::ID_NONE = Body::id_t(-1);

const shared_ptr<Body>& Body::byId(Body::id_t _id, Scene* rb) { return (*((rb ? rb : Omega::instance().getScene().get())->bodies))[_id]; }
const shared_ptr<Body>& Body::byId(Body::id_t _id, shared_ptr<Scene> rb) { return (*(rb->bodies))[_id]; }

// return list of interactions of this particle
boost::python::list Body::py_intrs()
{
	boost::python::list ret;
	for (Body::MapId2IntrT::iterator it = this->intrs.begin(), end = this->intrs.end(); it != end; ++it) { //Iterate over all bodie's interactions
		if (!(*it).second->isReal())
			continue;
		ret.append((*it).second);
	}
	return ret;
}

// return number of interactions of this particle
unsigned int Body::coordNumber() const
{
	unsigned int intrSize = 0;
	for (auto it = this->intrs.begin(), end = this->intrs.end(); it != end; ++it) { //Iterate over all bodie's interactions
		if (!(*it).second->isReal())
			continue;
		intrSize++;
	}
	return intrSize;
}


bool Body::maskOk(int mask) const { return (mask == 0 || ((groupMask & mask) != 0)); }
bool Body::maskCompatible(int mask) const { return (groupMask & mask) != 0; }
#ifdef YADE_MASK_ARBITRARY
bool Body::maskOk(const mask_t& mask) const { return (mask == 0 || ((groupMask & mask) != 0)); }
bool Body::maskCompatible(const mask_t& mask) const { return (groupMask & mask) != 0; }
#endif

} // namespace yade
