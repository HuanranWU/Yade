
#ifdef YADE_MPI
#include "MPIBodyContainer.hpp"

namespace yade { // Cannot have #include directive inside.

YADE_PLUGIN((MPIBodyContainer));
CREATE_LOGGER(MPIBodyContainer);


void MPIBodyContainer::clearContainer() { bContainer.clear(); }

} // namespace yade

#endif //YADE_MPI
