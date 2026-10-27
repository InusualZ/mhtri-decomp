/* Leaf header (docs/plan.md 6.5 rule 2): `stage_area_word_get`, which `stage/stg_w.cpp` owns, in the owner's own
 * signature.  C linkage. */
#ifndef MHTRI_STAGE_STAGE_AREA_WORD_GET_H
#define MHTRI_STAGE_STAGE_AREA_WORD_GET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802B04A0 - the address of area `id`'s 0xFF-ended byte list, or 0. */
u32 stage_area_word_get(u8 id);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_STAGE_STAGE_AREA_WORD_GET_H */
