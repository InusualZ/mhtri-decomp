/*
 * `quest/quest_entry.cpp`'s declarations (docs/plan.md 6.5 rule 2).  The unit's own types live here; the
 * foreign functions it calls are declared here too because the band they live in has no registered owner
 * yet (the map resolves them to unsplit addresses, and the units bracketing them name different modules,
 * so no `include/unsplit/<module>.h` is sound).  A later pass that registers those bands moves each
 * declaration to its owner's header.
 *
 * Names: every one is derived from the callee's own body and is marked GUESS in the unit header - the
 * runtime dump answers `zz_` for all of them.
 */
#ifndef MHTRI_QUEST_QUEST_ENTRY_H
#define MHTRI_QUEST_QUEST_ENTRY_H

#include "types.h"
#include "menu/menu_item.h"     /* GetItemData - owned by menu/menu_item.cpp (rule 2) */
#include "fn_80047398.h"        /* item_pair_copy - owned by fn_80047398.cpp (rule 2) */

/* The 4-byte `(item id, count)` slot `item_pair_copy` moves: a u16 id and a u16 count. */
struct Q_ItemPair {
    /* +0x0 */ u16 id;
    /* +0x2 */ u16 num;
};  /* size: 0x4 */

/* The same 4-byte slot as the item-slot search reads it: the count is a signed byte. */
struct Q_ItemCount {
    /* +0x0 */ u16 id;
    /* +0x2 */ u8 num;
    /* +0x3 */ u8 unused_0x3;
};  /* size: 0x4 */

/* A record/count block of the save data and of the item work: `count` entries followed by the four
 * per-group totals an index of 0x24..0x27 sums. */
struct Q_CountSet {
    /* +0x00 */ u16 count[0x24];
    /* +0x48 */ u16 total[4];
};  /* size: 0x50 */

/* The item work (`get_move_work_adrs(0)->0xDC`), seen only as the two count blocks this unit reads. */
struct Q_ItemWork {
    /* +0x000 */ u8 pad_0x000[0x490];
    /* +0x490 */ Q_CountSet set_c;
    /* +0x4E0 */ u8 pad_0x4E0[0x2];
    /* +0x4E2 */ Q_CountSet set_d;
};  /* size: 0x532 (approximate: the highest offset this unit reads + 2) */

/* The game's save/user block, seen only as the four count blocks this unit reads. */
struct Q_UserData {
    /* +0x0000 */ u8 pad_0x0000[0x490];
    /* +0x0490 */ Q_CountSet set_c;
    /* +0x04E0 */ u8 pad_0x04E0[0x2];
    /* +0x04E2 */ Q_CountSet set_d;
    /* +0x0532 */ u8 pad_0x0532[0x348E];
    /* +0x39C0 */ Q_CountSet set_a;
    /* +0x3A10 */ u8 pad_0x3A10[0xB0];
    /* +0x3AC0 */ Q_CountSet set_b;
};  /* size: 0x3B10 (approximate: the highest offset this unit reads + 0x50) */

/* The per-slot "move work" `get_move_work_adrs` indexes; only the fields this unit names are here. */
struct Q_MoveWork {
    /* +0x0000 */ u8 pad_0x0000[0xDC];
    /* +0x00DC */ Q_ItemWork* item_work;   /* the item work `move_work_item_work_get` returns */
    /* +0x00E0 */ u8 pad_0x00E0[0x2D];
    /* +0x010D */ u8 pad_0x010D[0x6];
    /* +0x0113 */ u8 state_0x113;           /* 1 while the slot is in its entry state */
    /* +0x0114 */ u8 pad_0x0114[0x22D4 - 0x114];
    /* +0x22D4 */ u8 flag_0x22D4;
    /* +0x22D5 */ u8 pad_0x22D5[0x7];
    /* +0x22DC */ u8 flag_0x22DC;
};  /* size: 0x22E0 (approximate) */

/* The unit's declarations.  C linkage: the two units that already call into this band
 * (`menu/menu_item_page.cpp`, `lobby/lb_companion_ui.cpp`) declare them inside their own `extern "C"`
 * blocks, so the original symbols were C-linkage names. */
#ifdef __cplusplus
extern "C" {
#endif

/* The save/user block `get_userdata` returns (declared here: its band has no registered owner yet). */
struct Q_UserData* get_userdata(void);

/* The item work of the local slot, or NULL when the slot has none. */
Q_ItemWork* move_work_item_work_get(void);
/* Whether the slot's entry-state byte is 1. */
u32 move_work_state_ck(void);
/* The low byte of a slot work's +0x36C word; slot 0 means the local one. */
u8 quest_slot_progress_get(s32 slot);

/* The two item-pair tables `quest_item_pair_copy_block` indexes by item id: entries below 0x64 and
 * 0x64 and above.  The bytes are the DOL's (declared, never defined - playbook 29). */
extern Q_ItemPair* const q_item_pair_tbl_low[];
extern Q_ItemPair* const q_item_pair_tbl_high[];

/* This unit's own symbols (map half renamed in the same change). */
s32 quest_record_a_count_get(u32 kind);
s32 quest_record_a_count_get_wide(s32 kind);
s32 quest_record_b_count_get(u32 kind);
s32 quest_record_b_count_get_wide(s32 kind);
void quest_item_pair_copy_block(Q_ItemPair* dst, u16 id, u8 kind);
void quest_item_pair_copy_row(Q_ItemPair* dst, u16 id, s8 row, u8 kind);
void quest_item_pair_copy_cell(Q_ItemPair* dst, u16 id, s8 col);
s8 quest_item_slot_find(Q_ItemCount* slots, u16 id);
/* The local slot's item work handed to the band below (0x803AA060) with `idx` and 1.  Body unwritten. */
u32 quest_item_work_notify(s32 idx);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif /* MHTRI_QUEST_QUEST_ENTRY_H */
