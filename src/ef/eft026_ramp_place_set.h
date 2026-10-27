/* Leaf header (docs/plan.md 6.5 rule 2): `ef/eft026_fx.cpp`'s `eft026_ramp_place_set`, the one declaration
 * `ef/eft053.cpp` needs.  GUESS name, from the body (listed on the NAMES line of ef/eft053.cpp). */
#ifndef MHTRI_EF_EFT026_RAMP_PLACE_SET_H
#define MHTRI_EF_EFT026_RAMP_PLACE_SET_H

#include "types.h"
#include "nw4r/math.h"
#include "ef/cp_vector.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80117EFC (0xFC): places a new eft026 record of the ramping `kind` (7 or 10) at `pos` with rotation `rot` and
 * stores `scale`; any other kind is released again */
void eft026_ramp_place_set(u8 kind, nw4r::math::VEC3* pos, _CP_VECTOR* rot, u8 area, f32 scale);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_EFT026_RAMP_PLACE_SET_H */
