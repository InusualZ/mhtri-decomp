/* Leaf header (docs/plan.md 6.5 rule 2): `fn_8004CAD8`'s `calc_xz_angle`, which `Pl/pl_coll.cpp` calls.
 * C linkage (the map row is a plain name).  GUESS name: `calcVecAng2` tail-calls it with two vectors' x/z. */
#ifndef MHTRI_CALC_XZ_ANGLE_H
#define MHTRI_CALC_XZ_ANGLE_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80050A40 - the 16-bit yaw from (`x1`, `z1`) toward (`x2`, `z2`), in the low half of the result. */
u32 calc_xz_angle(f32 x1, f32 z1, f32 x2, f32 z2);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_CALC_XZ_ANGLE_H */
