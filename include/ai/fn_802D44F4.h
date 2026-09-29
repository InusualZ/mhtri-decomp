/* The declarations `src/ai/fn_802D44F4.cpp` owns (docs/plan.md 6.5 rule 2: a consumer includes the
 * owner's header, it never declares the symbol itself).
 *
 * The unit is the cockpit-side AI band 0x802D44F4-0x802DDC04.  The `fn_802DAxxx` entries below are the
 * ones the cockpit band above it (`menu/fn_802E4978.cpp`, 0x802E4978-0x802E7408) calls; they had been
 * declared in `include/unsplit/menu.h` while the address had no registered owner, and their signatures
 * are that consumer's call sites (no body of them is written yet).
 *
 * `fn_802DA3CC` is a body of this range (0x88 B) whose *address* the consumer registers with
 * `subTransSetPrio`, so it is declared as the function it is rather than as a data object.
 */
#ifndef MHTRI_AI_FN_802D44F4_H
#define MHTRI_AI_FN_802D44F4_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

void fn_802DA1A8(void* work);            /* 0x802DA1A8 */
void fn_802DA1B0(void* work);            /* 0x802DA1B0 */
void fn_802DA344(void);                  /* 0x802DA344 */
s32 fn_802DA454(s32, s32, s16, s16, s32, s32, s32); /* 0x802DA454 */
void ai_npc_hold_item_arm(void);                  /* 0x802D6534, called by hud/cockpit_quest.cpp's
                                          * `quest_slot_arm_a` (its body is written in this unit) */

void fn_802DA3CC(void);                  /* 0x802DA3CC, registered by address */

/* 0x802D94C4 - the area-mode setter `enemy/em020_handlers.cpp`'s `em020_area_model_set` calls for
 * the em020 area models: once the screen-state query `quest_move_state_valid_ck` is true it arms
 * `lbl_806BD360`'s +0x484 mode byte and its 90-frame +0x486 timer (1 for 0, 2 otherwise) and clears
 * +0x485.  Added with that registration (rule 2: this range owns the address; the signature is the
 * callee's own body, which narrows r3 with `clrlwi r3,24` before the `cmpwi r3,0`). */
void fn_802D94C4(u8 mode);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
/* C++-linkage declarations: the map's mangled name is the C++ front-end's own spelling of these
 * (rule 9), so they are declared outside the `extern "C"` block above. */

/* 0x802DAF48 - the page-arrow putter the equip-information screens call: it draws the arrow sprite
 * rows `table` names at the anchor `pos`, for the (current, last) page pair and the flags byte.
 * Added with that consumer (rule 2: this range owns the address; the signature is its call site's
 * argument list, and the vector type is forward-declared so the header stays self-contained). */
struct _mh_ivec2_;
void PutPageArrow(u16* table, s16 cur, s16 last, u16 flags, const struct _mh_ivec2_* pos, u8 extra);
#endif

#endif /* MHTRI_AI_FN_802D44F4_H */
