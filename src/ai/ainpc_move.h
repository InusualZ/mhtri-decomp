/* Leaf header (docs/plan.md 6.5 rule 2): the `ai/ai_npc.cpp` symbols other units call, named for `ainpc_move`.
 * C scope: every map row here is an unmangled name, so the declarations keep C linkage. */
#ifndef MHTRI_AI_AINPC_MOVE_H
#define MHTRI_AI_AINPC_MOVE_H

#include "types.h"

struct _AINPC_W;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802D44F4 (0x2040): picks and starts the companion's next motion from its current motion number. */
void ai_npc_motion_dispatch(struct _AINPC_W* self);
/* 0x802D84D8 (0x78): the companion's per-frame move step - position, sub-states, model and motion. */
void ainpc_move(void);
/* 0x802D8788 (0xC4): returns 1 while the companion's hand-over motion is within reach of its partner. */
s32 ai_npc_hand_reach_ck(void);
/* 0x802D884C (0x110): hands the companion's held item to its partner and reports the item and count given. */
void ai_npc_item_give(u16* item, s16* count);
/* 0x802D895C (0x78): rolls the companion's formation choice against its formation record's percentage. */
void ai_npc_formation_roll(struct _AINPC_W* self);
/* 0x802D8D94 (0x60): returns a value scaled by the behaviour record of the given index. */
u16 ai_behaviour_scaled_get(u16 value, u8 index);
/* 0x802D8F84 (0x80): returns the OR of `1 << n` over the non-zero skill numbers of slots 1..count. */
u32 ai_skill_mask_get(u8 count);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_AI_AINPC_MOVE_H */
