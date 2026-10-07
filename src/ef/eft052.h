/* ef/eft052.h - the item-page and hold-block entry points of `ef/eft052.cpp` other units call (docs/plan.md 6.5
 * rule 2); `unsplit/lobby.h` re-exports it.  `eft052_hold_entry_set` takes `void*`: `lobby/fn_801E7530.cpp` hands
 * its own 0x1C-byte item-hold block, the definition spells it `CockpitHoldEntry*`. */
#ifndef MHTRI_EF_EFT052_H
#define MHTRI_EF_EFT052_H

#include "types.h"

struct _ENEMY_WORK;

#ifdef __cplusplus
extern "C" {
#endif

/* The item page's count for one item id moved by `delta` (a negative `delta` adds). */
void eft052_page_count_add(u16 id, s16 delta);

/* 0x803592D4 - the item page's count for item `id` (`use_rows` selects the row-limited count). */
s32 eft052_page_count_ck(u16 id, u8 use_rows);

/* 0x80359628 - steps the item-hold strip's input (the confirm and cancel buttons) and answers 1 when the
 * entry was taken, 2 when cancelled (GUESS name). */
s32 eft052_hold_step(void);

/* 0x80359B00 - draws the item-hold strip and its help lines for `mode` (GUESS name). */
void eft052_hold_draw(s32 mode);

/* The hold block's cursor row table value and its +0x0A word, through optional out pointers. */
void eft052_hold_row_get(u16* out_row, s32* out_value);

/* Steps the enemy part gauge (`gauge` field of the em024 work view) by `delta` while the monster is in
 * state 2, clamped to 0..500. */
void eft052_part_gauge_add(struct _ENEMY_WORK* self, s16 delta);

/* Whether the enemy part's damage flag is set; also a slot of the em024 program table. */
u8 eft052_part_damage_ck(struct _ENEMY_WORK* self, u32 part);

/* Whether the enemy part's damage level is even; also a slot of the em024 program table. */
s32 eft052_part_level_even_ck(struct _ENEMY_WORK* self, u32 part);

/* Hands the caller's item-hold entry (its +0x19 byte set to `flag`) to the hold block and re-seeds
 * the block from it. */
void eft052_hold_entry_set(void* entry, u8 flag);

/* 0x8035B5FC - pushes the text `text` on channel `ch` onto the HUD message list and returns the new
 * entry's id (-1 when the list is missing or full).  GUESS name; `lobby/lb_companion_ui.h` still carries
 * its own two-argument view. */
s32 hud_msg_push(u32 ch, const char* text);
/* 0x8035B700 - pushes one item message (`kind` the channel, `id` the message, `arg` the item id it names) onto the
 * HUD log: builds the line from the string table and the item name.  `hud_item_msg_push` is a GUESS name from
 * those two uses. */
s32 hud_item_msg_push(s32 kind, s32 id, u16 arg);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_EFT052_H */
