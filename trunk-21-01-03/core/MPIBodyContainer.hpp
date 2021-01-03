//Deepak kn, deepak.kn1990@gmail.com
// A dummy body container to serialize and send/recv in MPI messages.
#pragma once
#include <lib/base/Logging.hpp>
#include <core/Body.hpp>
#include <core/BodyContainer.hpp>
#include <core/Omega.hpp>
#include <core/Scene.hpp>
#include <mpi.h>
#include <vector>

namespace yade { // Cannot have #include directive inside.

class MPIBodyContainer : public Serializable {
public:
	int subdomainRank; // MPI_Comm_rank(MPI_COMM_WORLD, &procRank) commented for testing.;

	void insertBody(int id)
	{
		const shared_ptr<Scene>&   scene         = Omega::instance().getScene();
		shared_ptr<BodyContainer>& bodycontainer = scene->bodies;
		shared_ptr<Body>           b             = (*bodycontainer)[id];
		// if empty,  put body
		if (bContainer.size() == 0) {
			bContainer.push_back(b);
		}
		//check if body already exists
		else {
			int c = 0;
			for (std::vector<shared_ptr<Body>>::iterator bodyIter = bContainer.begin(); bodyIter != bContainer.end(); ++bodyIter) {
				const shared_ptr<Body>& inContainerBody = *(bodyIter);
				if (inContainerBody->id == b->id) {
					c += 1;
				}
			}
			if (c == 0) {
				bContainer.push_back(b);
			}
		}
	}

	void clearContainer();

	virtual ~MPIBodyContainer() {};

	void insertBodyListPy(boost::python::list& idList)
	{
		// insert a list of bodies
		unsigned int listSize = boost::python::len(idList);
		for (unsigned int i = 0; i != listSize; ++i) {
			int b_id = boost::python::extract<int>(idList[i]);
			insertBody(b_id);
		}
	}

	void insertBodyList(const std::vector<Body::id_t>& idList)
	{
		for (unsigned int i = 0; i != idList.size(); ++i) {
			insertBody(idList[i]);
		}
	}

	unsigned int getCount() { return bContainer.size(); }

	shared_ptr<Body> getBodyat(int pos) { return bContainer[pos]; }

	//     shared_ptr<Body> getBodybyId(int id ){
	//
	//     }


	std::vector<Body::id_t> getIds()
	{
		std::vector<Body::id_t> ids;
		for (unsigned int i = 0; i != bContainer.size(); ++i) {
			const shared_ptr<Body>& b = bContainer[i];
			ids.push_back(b->id);
		}
		return ids;
	}


	// clang-format off
  YADE_CLASS_BASE_DOC_ATTRS_CTOR_PY(MPIBodyContainer,Serializable,"a dummy container to serialize and send. ",
  ((vector<shared_ptr<Body>>, bContainer,,,"a dummy body container to serialize"))
  ,
//   MPI_Comm_rank(MPI_COMM_WORLD, &procRank); cannot call in constructor
  ,
  .def("insertBody",&MPIBodyContainer::insertBody,(boost::python::arg("bodyId")),  "insert a body (by id) in this container")
  .def("insertBodyListPy", &MPIBodyContainer::insertBodyListPy, (boost::python::arg("listOfIds")), "inset a list of bodies (by ids)")
  .def("clearContainer", &MPIBodyContainer::clearContainer, "clear bodies in the container")
  .def("getCount", &MPIBodyContainer::getCount, "get container count")
  .def_readonly("subdomainRank", &MPIBodyContainer::subdomainRank, "origin rank of this container") 
  );
	// clang-format on

	DECLARE_LOGGER;
};
REGISTER_SERIALIZABLE(MPIBodyContainer);

} // namespace yade
