/*
 * ai/fn_802D44F4.h - declarations of the 0x802D44F4-0x802DDC04 band, now `ai/ai_npc.cpp` (to 0x802D9EA4) and
 *   `hud/cockpit.cpp` (from there): the notice-pool and slot helpers the cockpit calls, `ai_slots_clear`, `PutPageArrow`
 *   and the `ai_npc` entries other units call.  `ainpc_entry_draw_all` is declared as the function whose address
 *   `subTransSetPrio` registers.
 */
#ifndef MHTRI_AI_FN_802D44F4_H
#define MHTRI_AI_FN_802D44F4_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The looping-animation work of a kind-1 notice (`hud/cockpit.cpp`'s kind-1 update/draw pair).  size: 0x8 */
typedef struct HudNoticeLoop {
    /* +0x00 */ const u16* ids;      /* the sprite id run, one past the table row's lead id */
    /* +0x04 */ u8 last_frame;       /* the frame the animation wraps after */
    /* +0x05 */ u8 pad_0x05[3];
} HudNoticeLoop;

/* The two-layer scroll work of a kind-2 notice (`hud/cockpit.cpp`'s kind-2 update/draw pair).  size: 0xC */
typedef struct HudNoticeScroll {
    /* +0x00 */ const u16* front_ids; /* the run drawn over the back layer, one sprite at a time */
    /* +0x04 */ const u16* back_ids;  /* the back layer (a wide-screen variant exists) */
    /* +0x08 */ s16 tex_ofs;          /* the random texture offset added to each front sprite */
    /* +0x0A */ s16 hold_timer;       /* frames the notice holds before it fades out */
} HudNoticeScroll;

/* The kind-specific tail of a notice (`hud_notice_work_get` returns its address).  size: 0x10 */
typedef union HudNoticeWork {
    /* +0x00 */ HudNoticeLoop loop;
    /* +0x00 */ HudNoticeScroll scroll;
    /* +0x00 */ u8 raw[0x10];
} HudNoticeWork;

/* One 0x20-byte slot of the HUD notice pool at 0x806BD808 (16 slots; `hud_notice_spawn` returns the
 * free slot it filled, or 0 when the pool is full).  size: 0x20 */
typedef struct HudNotice {
    /* +0x00 */ u8 active;      /* the slot is in use */
    /* +0x01 */ u8 kind;        /* the update/draw table index (0..2) */
    /* +0x02 */ u8 id;          /* the notice id inside its kind (the per-kind tables' row) */
    /* +0x03 */ u8 pad_0x03;
    /* +0x04 */ u8 state;       /* the per-kind step counter */
    /* +0x05 */ u8 visible;     /* drawn while set */
    /* +0x06 */ u8 animating;   /* the frame advances while set */
    /* +0x07 */ u8 frame;       /* the animation frame */
    /* +0x08 */ u8* flag_ptr;   /* the byte the notice reads its live state from */
    /* +0x0C */ s16 x;          /* the anchor the sprites are drawn at */
    /* +0x0E */ s16 y;
    /* +0x10 */ HudNoticeWork work;
} HudNotice;

/* 0x802DA1A8 - stores the watched flag pointer into the slot `hud_notice_spawn` returned. */
void hud_notice_set_flag_ptr(HudNotice* notice, u8* flag);
/* 0x802DA1B0 - clears the sixteen 0x20-byte AI slot records at 0x806BD808 (each body is a byte store); the
 * function reads no argument.  Renamed with `quest/quest_entry.cpp`; GUESS name from that loop. */
void ai_slots_clear(void);
void ainpc_entry_update_all(void);                  /* 0x802DA344 */
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

void ainpc_entry_draw_all(void);                  /* 0x802DA3CC, registered by address */

/* 0x802D94C4 - the area-mode setter `enemy/em020_prog.cpp`'s `em020_area_model_set` calls for
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
