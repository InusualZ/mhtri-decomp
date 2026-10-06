/* Leaf header (docs/plan.md 6.5 rule 2): `lb_interior_fx_tbl` (`.data` 0x805F2A38), inside `quest/quest_item_slot.cpp`'s
 * `.data` claim, which `lobby/lb_quest_screen.cpp`'s interior effect step walks. */
#ifndef MHTRI_QUEST_LB_INTERIOR_FX_TBL_H
#define MHTRI_QUEST_LB_INTERIOR_FX_TBL_H

#include "types.h"
#include "nw4r/math.h"

/* One interior effect record: the model index it belongs to (0xFF ends a list), its position and scale.
 * size: 0x14 */
struct LbInteriorFx {
    /* +0x00 */ u8 key_0x00;
    /* +0x01 */ u8 unused_0x01[0x3];
    /* +0x04 */ nw4r::math::VEC3 pos_0x04;
    /* +0x10 */ f32 scale_0x10;
};

#ifdef __cplusplus
extern "C" {
#endif

/* Per interior id, its effect list (NULL for none). */
extern LbInteriorFx* lb_interior_fx_tbl[];

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_QUEST_LB_INTERIOR_FX_TBL_H */
