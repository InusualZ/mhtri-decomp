/* Leaf header (docs/plan.md 6.5 rule 2): `stage_char_res_body_get`, which `stage/stg_w.cpp` owns, in the owner's
 * own signature.  C linkage. */
#ifndef MHTRI_STAGE_STAGE_CHAR_RES_BODY_GET_H
#define MHTRI_STAGE_STAGE_CHAR_RES_BODY_GET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802B050C - the resource body of stage character `index`, or 0 when the slot is empty. */
/* untyped: opaque handle - the owner's definition returns the resource body untyped */
void* stage_char_res_body_get(u8 index);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_STAGE_STAGE_CHAR_RES_BODY_GET_H */
