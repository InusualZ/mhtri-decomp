/*
 * Leaf header (docs/plan.md 6.5 rule 2): `sqrt_f32` (0x80050BC0, `x * FrSqrt(x)`), owned by `src/fn_8004CAD8.cpp`;
 * separate so `ef/ef_cube.cpp`, whose own `Vec3` cannot sit beside `nw4r/math.h`, can reach it.
 */
#ifndef MHTRI_SQRT_F32_H
#define MHTRI_SQRT_F32_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

f32 sqrt_f32(f32 x);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_SQRT_F32_H */
