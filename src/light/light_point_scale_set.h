/* light/light_point_scale_set.h - leaf header (docs/plan.md 6.5 rule 2): `light/light.cpp`'s `light_point_scale_set`
 *   (0x802BFAE8), for `lobby/fn_802FA9A0.cpp`.  C linkage.  GUESS name (registry). */
#ifndef MHTRI_LIGHT_LIGHT_POINT_SCALE_SET_H
#define MHTRI_LIGHT_LIGHT_POINT_SCALE_SET_H

#include "types.h"
#include "nw4r/math.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802BFAE8: sets point light `id` at `pos` with its colour scaled by `scale` (`flag` 0 skips it during a demo). */
void light_point_scale_set(u16 id, VEC3* pos, f32 scale, u8 flag);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LIGHT_LIGHT_POINT_SCALE_SET_H */
