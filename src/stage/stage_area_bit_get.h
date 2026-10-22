/* Leaf header (docs/plan.md 6.5 rule 2): `stage/stg_w.cpp`'s `stage_area_bit_get`, which `Pl/pl_coll.cpp` calls.
 * C linkage (the owner defines it `extern "C"`).  GUESS name. */
#ifndef MHTRI_STAGE_STAGE_AREA_BIT_GET_H
#define MHTRI_STAGE_STAGE_AREA_BIT_GET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802B0AEC - the stage's area bit for area `index` (a collision polygon's area mask is tested against it). */
u8 stage_area_bit_get(u8 index);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_STAGE_STAGE_AREA_BIT_GET_H */
