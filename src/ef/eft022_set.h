/* Leaf header (docs/plan.md 6.5 rule 2): `ef/eft022_fx.cpp`'s `eft022_set`, the one declaration `ai/ai_npc.cpp` needs.  C++
 * linkage: the map row is the mangling. */
#ifndef MHTRI_EF_EFT022_SET_H
#define MHTRI_EF_EFT022_SET_H

#include "types.h"
#include "nw4r/math.h"

/* 0x80116368 (0xC8): spawns effect 22 at a position in an area. */
void eft022_set(nw4r::math::VEC3* pos, u8 area);

#endif /* MHTRI_EF_EFT022_SET_H */
