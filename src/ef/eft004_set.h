/* ef/eft004_set.h - the declaration of `eft004_set`, which `ef/eft004_fx.cpp` defines at C++ scope (docs/plan.md 6.5
 * rule 2; the map name is the mangling `eft004_set__FUcPQ34nw4r4math4VEC3fUlUc`). A leaf header because
 * `ef/eft004.h`'s includers declare a C-linkage spelling of their own. */
#ifndef MHTRI_EF_EFT004_SET_H
#define MHTRI_EF_EFT004_SET_H

#include "types.h"
#include "nw4r/math.h"

#ifdef __cplusplus
/* 0x80100448 - spawns eft004 effect `type` at `pos` with a scale, a parameter word and a flag byte. */
void eft004_set(u8 type, nw4r::math::VEC3* pos, f32 scale, u32 param, u8 flag);
#endif

#endif /* MHTRI_EF_EFT004_SET_H */
