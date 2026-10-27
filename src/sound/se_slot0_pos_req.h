/* Leaf header (docs/plan.md 6.5 rule 2): `se_slot0_pos_req` (0x800DC868), defined by `src/sound/fn_800D7F54.cpp`. */
#ifndef MHTRI_SOUND_SE_SLOT0_POS_REQ_H
#define MHTRI_SOUND_SE_SLOT0_POS_REQ_H

#include "types.h"
#include "nw4r/math.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

void se_slot0_pos_req(nw4r::math::VEC3* pos);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_SOUND_SE_SLOT0_POS_REQ_H */
