/* sound/se_torch_slot_req.h - leaf header (docs/plan.md 6.5 rule 2): `sound/fn_800D7F54.cpp`'s `se_torch_slot_req`
 *   (0x800DD3B8), for `lobby/fn_802FA9A0.cpp`.  C linkage.  GUESS name (registry). */
#ifndef MHTRI_SOUND_SE_TORCH_SLOT_REQ_H
#define MHTRI_SOUND_SE_TORCH_SLOT_REQ_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800DD3B8: requests sound 0x2E on slot 0x1E + (index & 3) at the origin. */
void se_torch_slot_req(u8 index);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_SOUND_SE_TORCH_SLOT_REQ_H */
