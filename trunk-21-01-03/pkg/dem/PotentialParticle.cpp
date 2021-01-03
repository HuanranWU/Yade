/*CWBoon 2015 */

#ifdef YADE_POTENTIAL_PARTICLES
//! To implement potential particles (Houlsby 2009) using sphere
#include "PotentialParticle.hpp"

namespace yade { // Cannot have #include directive inside.

YADE_PLUGIN((PotentialParticle));


PotentialParticle::~PotentialParticle() { }

} // namespace yade

#endif // YADE_POTENTIAL_PARTICLES
