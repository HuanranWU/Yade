#ifdef YADE_FEM
#include <pkg/fem/Node.hpp>

namespace yade { // Cannot have #include directive inside.

Node::~Node() { }
YADE_PLUGIN((Node));

} // namespace yade

#endif //YADE_FEM
