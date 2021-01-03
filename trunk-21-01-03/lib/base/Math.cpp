#include <lib/base/Math.hpp>
#include <boost/math/constants/constants.hpp>

namespace yade { // Cannot have #include directive inside.

template <> int  ZeroInitializer<int>() { return (int)0; }
template <> Real ZeroInitializer<Real>() { return (Real)0; }

#ifdef YADE_MASK_ARBITRARY
bool   operator==(const mask_t& g, int i) { return g == mask_t(i); }
bool   operator==(int i, const mask_t& g) { return g == i; }
bool   operator!=(const mask_t& g, int i) { return !(g == i); }
bool   operator!=(int i, const mask_t& g) { return g != i; }
mask_t operator&(const mask_t& g, int i) { return g & mask_t(i); }
mask_t operator&(int i, const mask_t& g) { return g & i; }
mask_t operator|(const mask_t& g, int i) { return g | mask_t(i); }
mask_t operator|(int i, const mask_t& g) { return g | i; }
bool   operator||(const mask_t& g, bool b) { return (g != 0) || b; }
bool   operator||(bool b, const mask_t& g) { return g || b; }
bool   operator&&(const mask_t& g, bool b) { return (g != 0) && b; }
bool   operator&&(bool b, const mask_t& g) { return g && b; }
#endif

}; // namespace yade
