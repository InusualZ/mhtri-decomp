/* Leaf header (docs/plan.md 6.5 rule 2): a symbol `ef/eft026_fx.cpp` defines, for a consumer that cannot include
 * the owner's full header.  C++ linkage (the map row carries the mangling). */
#ifndef MHTRI_EF_EFT028_SET_KOWARE_H
#define MHTRI_EF_EFT028_SET_KOWARE_H

#include "types.h"
#include "nw4r/math.h"

/* 0x80119A40 - spawns the eft028 break effect `kind` at `pos`. */
void eft028_set_koware(u8 kind, nw4r::math::VEC3* pos, u8 area, long param);

#endif /* MHTRI_EF_EFT028_SET_KOWARE_H */
