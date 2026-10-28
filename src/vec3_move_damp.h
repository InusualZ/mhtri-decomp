/* vec3_move_damp.h - leaf header (docs/plan.md 6.5 rule 2) for `vec3_move_damp`, which `fn_8004CAD8.cpp` owns, with the
 * owner's signature. */
#ifndef MHTRI_VEC3_MOVE_DAMP_H
#define MHTRI_VEC3_MOVE_DAMP_H

#include "types.h"
#include "nw4r/math.h"

#ifdef __cplusplus
extern "C" {
#endif
/* 0x800524C0 (0x124): `to`, pulled back toward `from` by `k` times how far `dir` points against the move, into
 * `out`. */
void vec3_move_damp(const nw4r::math::VEC3* from, const nw4r::math::VEC3* to, const nw4r::math::VEC3* dir, f32 k,
                    nw4r::math::VEC3* out);
#ifdef __cplusplus
}
#endif

#endif /* MHTRI_VEC3_MOVE_DAMP_H */
