/* Leaf header (docs/plan.md 6.5 rule 2): `stage/stg_w.cpp`'s stage_cell_vec_get */
#ifndef MHTRI_STAGE_STAGE_CELL_VEC_GET_H
#define MHTRI_STAGE_STAGE_CELL_VEC_GET_H

#include "types.h"
#include "nw4r/math.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802B02C4 - copies vector `k` of the area cell row `i` into `out`. */
void stage_cell_vec_get(nw4r::math::VEC3* out, u8 i, u8 k);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_STAGE_STAGE_CELL_VEC_GET_H */
