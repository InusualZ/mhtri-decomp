/* ef/eft052.cpp's header: the item-page / hold-block entry points other units call (rule 2 - an
 * `extern` lives with the unit that owns the symbol, never in a consumer's source and never in
 * `include/unsplit/<module>.h` once the address is registered).
 *
 * RULE 2 HOMES.  The three declarations below stood in `include/unsplit/lobby.h` while
 * 0x80358624..0x8035E034 was unclaimed; `ef/eft052.cpp` owns that range now, so they moved here and
 * the band header re-exports this one for the units that already include it (the same move
 * `include/menu/menu_message.h` and `include/camera/camera.h` made for their bands).
 * `lobby/fn_801E7530.cpp` calls `eft052_hold_entry_set`; `eft052_item_value_get` and
 * `eft052_page_counts_get` are called by `lobby/fn_801EC9F8.cpp` and `menu/menu_message.cpp`, which
 * still declare them at their own call sites (their rule-2 pass moves those to their headers).
 *
 * `eft052_hold_entry_set`'s entry argument is `void*`, not the unit's private `CockpitHoldEntry`
 * view: `lobby/fn_801E7530.cpp` hands its own 0x1C-byte item-hold parameter block (the same field
 * layout from +0x04 up) by address, and publishing the record as a shared type is a separate
 * change - the shape `include/ef/eft_slot.h` uses for `EftSlot`/`_ENEMY_DATA`.  The definition is
 * the fuller spelling: `void eft052_hold_entry_set(CockpitHoldEntry* entry, u8 flag)` in
 * `src/ef/eft052.cpp`.
 *
 * `eft052_page_count_add` and `eft052_hold_row_get` have no caller in `src/` today; they are kept
 * because the band header declared them (with `s32` tails the definitions do not have).
 */
#ifndef MHTRI_EF_EFT052_H
#define MHTRI_EF_EFT052_H

#include "types.h"

struct _ENEMY_WORK;

#ifdef __cplusplus
extern "C" {
#endif

/* The item page's count for one item id moved by `delta` (a negative `delta` adds). */
void eft052_page_count_add(u16 id, s16 delta);

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
/* 0x8035B700 - pushes one item message (`kind` the channel, `id` the message, `arg` the item id it names) onto
 * the HUD log: builds the line from the string table and the item name and hands it on.  Renamed from
 * `hud_item_msg_push` (GUESS name from those two uses); moved here from `unsplit/unknown.h`. */
s32 hud_item_msg_push(s32 kind, s32 id, u16 arg);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_EFT052_H */
