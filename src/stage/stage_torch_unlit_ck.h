/* stage/stage_torch_unlit_ck.h - leaf header (docs/plan.md 6.5 rule 2): `stage/stg_w.cpp`'s `stage_torch_unlit_ck`
 *   (0x802B46DC), for `lobby/fn_802FA9A0.cpp`.  C linkage.  GUESS name (registry). */
#ifndef MHTRI_STAGE_STAGE_TORCH_UNLIT_CK_H
#define MHTRI_STAGE_STAGE_TORCH_UNLIT_CK_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802B46DC: whether bit `index` of the stage work's torch halfword (+0x2F92) is clear. */
u32 stage_torch_unlit_ck(u8 index);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_STAGE_STAGE_TORCH_UNLIT_CK_H */
