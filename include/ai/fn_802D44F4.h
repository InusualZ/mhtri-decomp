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

/* One 0x20-byte slot of the HUD notice pool at 0x806BD808 (16 slots; `hud_notice_spawn` returns the
 * free slot it filled, or 0 when the pool is full).  Only the watched-flag pointer the cockpit
 * sets is written by the consumers.  size: 0x20 */
typedef struct HudNotice {
    /* +0x00 */ u8 unused_0x00[0x08 - 0x00];
    /* +0x08 */ u8* flag_ptr;   /* the byte the notice reads its live state from */
    /* +0x0C */ u8 unused_0x0C[0x20 - 0x0C];
} HudNotice;

/* 0x802DA1A8 - stores the watched flag pointer into the slot `hud_notice_spawn` returned. */
void hud_notice_set_flag_ptr(HudNotice* notice, u8* flag);
/* 0x802DA1B0 - clears the sixteen 0x20-byte AI slot records at 0x806BD808 (each body is a byte store); the
 * function reads no argument.  Renamed with `quest/quest_entry.cpp`; GUESS name from that loop. */
void ai_slots_clear(void);
void fn_802DA344(void);                  /* 0x802DA344 */
/* 0x802DA454 - claims a free slot of the notice pool and fills it from the arguments (kind, id, the
 * position and three mode bytes); returns the slot. */
HudNotice* hud_notice_spawn(s32 kind, s32 id, s16 x, s16 y, s32 a, s32 b, s32 c);

struct _AINPC_W;
/* 0x802D7DB4 - whether the AI NPC's +0x420 == 5 state has its +0x422 timer running (0 for an empty record). */
s32 ai_npc_hold_ck(struct _AINPC_W* self);
/* 0x802D7E20 - whether the AI NPC has arrived (its step counter is at 1; 0 for an empty record). */
s32 ai_npc_arrived_ck(struct _AINPC_W* self);

/* 0x802D9F58 - the HUD colour word of player `player`. */
u32 player_color_get(u8 player);
void ai_npc_hold_item_arm(void);                  /* 0x802D6534, called by hud/cockpit_quest.cpp's
                                          * `quest_slot_arm_a` (its body is written in this unit) */

void fn_802DA3CC(void);                  /* 0x802DA3CC, registered by address */

/* 0x802D94C4 - the area-mode setter `enemy/em020_handlers.cpp`'s `em020_area_model_set` calls for
 * the em020 area models: once the screen-state query `quest_move_state_valid_ck` is true it arms
 * `ainpc_w`'s +0x484 mode byte and its 90-frame +0x486 timer (1 for 0, 2 otherwise) and clears
 * +0x485.  Added with that registration (rule 2: this range owns the address; the signature is the
 * callee's own body, which narrows r3 with `clrlwi r3,24` before the `cmpwi r3,0`). */
void fn_802D94C4(u8 mode);

/* 0x802D9EA4 - forwards to the reaction dispatcher (its consumers call it with no argument: the AI work
 * pointer is the caller's `r3`).  Added with `quest/arenatask.cpp` (rule 2: this range owns it). */
void ai_npc_reaction_forward(void);

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
