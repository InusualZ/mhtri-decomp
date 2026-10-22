/* Leaf header (docs/plan.md 6.5 rule 2): `sound/fn_800D7F54.cpp`'s se_item_drop_req */
#ifndef MHTRI_SOUND_SE_ITEM_DROP_REQ_H
#define MHTRI_SOUND_SE_ITEM_DROP_REQ_H

#include "types.h"
#include "nw4r/math.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800DCA08 - requests the item-drop sound at `pos` (GUESS name). */
void se_item_drop_req(nw4r::math::VEC3* pos);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_SOUND_SE_ITEM_DROP_REQ_H */
