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

/* The player work record `quest_item_work_merge`'s first argument points at (defined in `pl.h`);
 * nothing here reads it, so the forward declaration is all this header needs. */
struct _PLW;

/* The 4-byte `(item id, count)` slot `item_pair_copy` moves - and the record the lot picks write
 * their output with: a u16 id and a u16 count (the pick stores a sign-extended byte payload). */
struct Q_ItemPair {
    /* +0x0 */ u16 id;
    /* +0x2 */ u16 num;
};  /* size: 0x4 */

/* The same 4-byte slot as the item-slot search reads it: the count is a signed byte. */
struct Q_ItemCount {
    /* +0x0 */ u16 id;
    /* +0x2 */ s8 num;
    /* +0x3 */ u8 unused_0x3;
};  /* size: 0x4 */

/* A record/count block of the save data and of the item work: `count` entries followed by the four
 * per-group totals an index of 0x24..0x27 sums. */
struct Q_CountSet {
    /* +0x00 */ u16 count[0x24];
    /* +0x48 */ u16 total[4];
};  /* size: 0x50 */

/* One 4-byte entry of the weighted lot tables the pick helpers walk (`u16` key, one payload byte and
 * one weight byte); an entry whose key is 0 ends the table. */
struct Q_LotEntry {
    /* +0x0 */ u16 id;
    /* +0x2 */ u8 value;
    /* +0x3 */ u8 weight;
};  /* size: 0x4 */

/* One 8-byte slot pair of the result rows: the id a pick is matched against, the 0x30-byte group it
 * indexes, and the two counts the pick helper walks. */
struct Q_SlotPair {
    /* +0x0 */ u8 id_0x00;
    /* +0x1 */ u8 group_0x01;
    /* +0x2 */ u8 kind_0x02;
    /* +0x3 */ u8 count_0x03;
    /* +0x4 */ u8 pad_0x04;
    /* +0x5 */ u8 pad_0x05;
    /* +0x6 */ u8 pad_0x06;
    /* +0x7 */ u8 pad_0x07;
};  /* size: 0x8 */

/* The item work (`get_move_work_adrs(0)->0xDC`), the same block `quest_work_ptr` points at; only the
 * offsets this unit names are here (the lot tables' run at +0x9C is one region, addressed but not
 * walked by name). */
struct Q_ItemWork {
    /* +0x0000 */ u8 pad_0x0000[0x40];
    /* +0x0040 */ Q_ItemCount slots_0x40[5];  /* the five delivered-item slots `quest_item_slot_add` merges into */
    /* +0x0054 */ u8 rot_0x54;                /* the round-robin slot the next new id takes */
    /* +0x0055 */ u8 flag_0x55;               /* set when a slot changed */
    /* +0x0056 */ u8 pad_0x0056[0x9C - 0x56];
    /* +0x009C */ union {
        /* +0x009C */ Q_LotEntry lot_0x9C[0xFD];  /* the first weighted lot table */
        /* +0x009C */ u8 pad_0x009C[0x3F4];
    };
    /* +0x0490 */ Q_CountSet set_c;
    /* +0x04E0 */ u8 pad_0x04E0[0x2];
    /* +0x04E2 */ Q_CountSet set_d;
    /* +0x0532 */ u8 pad_0x0532[0x6A2A - 0x532];
    /* +0x6A2A */ s8 count_0x6A2A;         /* entries in the arena item table below */
    /* +0x6A2B */ u8 pad_0x6A2B[0xF];       /* the item table's own bytes */
    /* +0x6A3A */ u8 entry_send_0x6A3A[7];  /* the quest entry-send header block */
    /* +0x6A41 */ u8 pad_0x6A41[0x6AB8 - 0x6A41];
};  /* size: 0x6AB8 */

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
    /* +0x00E0 */ u8 pad_0x00E0[0x9];
    /* +0x00E9 */ u8 phase_0xE9;           /* the quest phase byte `quest_phase_get` returns */
    /* +0x00EA */ u8 pad_0x00EA[0xFA - 0xEA];
    /* +0x00FA */ u8 sub_0xFA;              /* the sub-state `quest_move_sub_state_ck` tests */
    /* +0x00FB */ u8 pad_0x00FB[0x10C - 0xFB];
    /* +0x010C */ u16 item_id_0x10C;         /* the item `quest_item_work_merge` records */
    /* +0x010E */ s16 item_value_0x10E;      /* its handed-over value */
    /* +0x0110 */ u8 item_flag_0x110;        /* set while that record is live */
    /* +0x0111 */ u8 pad_0x0111[0x113 - 0x111];
    /* +0x0113 */ u8 state_0x113;           /* 1 while the slot is in its entry state */
    /* +0x0114 */ u8 pad_0x0114[0x22D4 - 0x114];
    /* +0x22D4 */ u8 state_0x22D4;          /* the state byte `quest_move_state_valid_ck`/`_get` read */
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
/* The low byte of a slot work's +0x36C word; slot 0 means the local one. */
u8 quest_slot_progress_get(s32 slot);

/* The two item-pair tables `quest_item_pair_copy_block` indexes by item id: entries below 0x64 and
 * 0x64 and above.  The bytes are the DOL's (declared, never defined - playbook 29). */
extern Q_ItemPair* const q_item_pair_tbl_low[];
extern Q_ItemPair* const q_item_pair_tbl_high[];

/* This unit's own `.bss` (claimed and DEFINED by `src/quest/quest_entry.cpp`, `.bss`
 * 0x806C5558..0x806C5858): the band's four 0xC0-byte picked-pair tables.  The quest lot picks write
 * them (`quest_element_clear` refills `_a` from the item work's lot table, `fn_803AB914` fills `_a` at
 * four offsets), and `quest_item_pair_tbl_copy` moves `_a`/`_b` into a result row's two adjacent 0xC0
 * buffers. */
extern Q_ItemPair quest_item_pair_tbl_a[0x30];
extern Q_ItemPair quest_item_pair_tbl_b[0x30];
extern Q_ItemPair quest_item_pair_tbl_c[0x30];
extern Q_ItemPair quest_item_pair_tbl_d[0x30];

/* This unit's own symbols (map half renamed in the same change). */
s32 quest_record_a_count_get(u32 kind);
s32 quest_record_a_count_get_wide(s32 kind);
s32 quest_record_b_count_get(u32 kind);
s32 quest_record_b_count_get_wide(s32 kind);
void quest_item_pair_copy_block(Q_ItemPair* dst, u16 id, u8 kind);
void quest_item_pair_copy_row(Q_ItemPair* dst, u16 id, s8 row, u8 kind);
void quest_item_pair_copy_cell(Q_ItemPair* dst, u16 id, s8 col);
/* The five-slot list's entry for `id`, sign-extended; the callers compare it against 0. */
s16 quest_item_slot_find(Q_ItemCount* slots, u16 id);
/* Adds `count` of `id` to the list; the result byte is what the menu switches on. */
s32 quest_item_slot_add(Q_ItemCount* slots, u8* rot, u16 id, s16 count);
/* Moves the two first picked-pair tables into a result record's two adjacent 0xC0 buffers (0x803AB3BC). */
void quest_item_pair_tbl_copy(Q_ItemPair* dst_a, Q_ItemPair* dst_b);
/* The local slot's item work handed to the band below (0x803AA060) with `idx` and 1.  Body unwritten. */
u32 quest_item_work_notify(s32 idx);

/* Records the item a caller hands over in the local slot's move work, then merges it into the item
 * work's own five-slot list with the negated value (0x803AAB80).  The target never reads its first
 * argument, which the only caller passes as the player work pointer. */
void quest_item_work_merge(struct _PLW* owner, u16 id, s16 value);

/* The quest-phase and per-slot state getters (0x803AD8F4, 0x803AAE7C, 0x803AB15C, 0x803AE934,
 * 0x803B0D5C, 0x803AFB90). */
u8  quest_phase_get(void);
u32 quest_move_flag_ck(void);
u32 quest_play_state_ck(void);
u32 quest_item_work_flag_ck(void);
u32 quest_move_sub_state_ck(void);
u32 quest_work_busy_ck(void);
/* The move work's own +0x22D4 state byte: 0x803AB028 tests its bit 7 (the "a state code follows"
 * flag), 0x803AB070 returns its low 7 bits. */
u32 quest_move_state_valid_ck(void);
u32 quest_move_state_get(void);
/* Whether the local slot has a quest selected (0x803AAEC0). */
u32 quest_select_ready_ck(void);
/* The three quest-id range probes the bands below gate on (0x803AAF3C, 0x803AAF88, 0x803AAFE0). */
u32 quest_id_low_get(void);
/* The index of the loaded element a quest id names (0x803ADF84), or -1. */
s32 quest_element_find(u8 kind);
u32 quest_id_head_ck(void);
u32 quest_id_tail_ck(void);
/* The band's two quest-work predicates (0x803B0CD4, 0x803B0CFC). */
u32 quest_entry_active_ck(void);
u32 quest_entry_ready_ck(void);
/* The item-id range test the pair tables are selected with (0x803AAF1C). */
u32 quest_item_id_low_ck(u16 id);
/* The record's own +0x8B state byte (0x803ADF48). */
u8  quest_record_state_get(u8* rec);

/* The 8-byte slot pair `quest_pair_copy` moves and the 0x20-byte key block `quest_element_copy`
 * moves; both are the target's own byte-by-byte copies. */
struct Q_PairBlock {
    /* +0x00 */ u8 byte_0x00;
    /* +0x01 */ u8 byte_0x01;
    /* +0x02 */ u8 byte_0x02;
    /* +0x03 */ u8 byte_0x03;
    /* +0x04 */ u8 byte_0x04;
    /* +0x05 */ u8 byte_0x05;
    /* +0x06 */ u8 byte_0x06;
    /* +0x07 */ u8 byte_0x07;
};  /* size: 0x8 */

struct Q_ElementBlock {
    /* +0x00 */ u8 byte_0x00;
    /* +0x01 */ u8 byte_0x01;
    /* +0x02 */ u8 byte_0x02;
    /* +0x03 */ u8 byte_0x03;
    /* +0x04 */ u8 byte_0x04;
    /* +0x05 */ u8 byte_0x05;
    /* +0x06 */ u8 byte_0x06;
    /* +0x07 */ u8 byte_0x07;
    /* +0x08 */ u32 word_0x08;
    /* +0x0C */ u32 word_0x0C;
    /* +0x10 */ u32 word_0x10;
    /* +0x14 */ u32 word_0x14;
    /* +0x18 */ u32 word_0x18;
    /* +0x1C */ u32 word_0x1C;
};  /* size: 0x20 */

/* The arena element record `quest_element_build` fills and the pick's output lands in: the 0x40-byte
 * payload the builder clears, and the sub-flag byte just past it that its caller arms.  Only the two
 * offsets this band touches are named. */
struct Q_ArenaElement {
    /* +0x000 */ u8 pad_0x000[0x3F4];
    /* +0x3F4 */ Q_ItemPair payload_0x3F4[0x10];  /* 0x40 B, the pick's own output region */
    /* +0x434 */ u8 flag_0x434;
};  /* size: 0x435 (approximate: the highest offset this band reads + 1) */

/* The arena element builder (0x803AD008): clears the element's payload, then fills it from the item
 * work's own weighted lot table.  Its first parameter is the caller's context, which the target never
 * reads (`r3` is clobbered by the first statement). */
/* untyped: caller-owned context payload the target never reads */
void quest_element_build(void* owner, u32 kind, Q_ArenaElement* element);

/* The same build for the band's persisted pair tables (0x803ACE90): zeroes all four, then refills
 * `quest_item_pair_tbl_a` with 3, 5 or 8 picks from the item work's lot table. */
/* untyped: caller-owned context payload the target never reads */
void quest_element_clear(void* owner, u32 kind, Q_ArenaElement* element);

void quest_pair_copy(Q_SlotPair* dst, const Q_SlotPair* src);
void quest_element_copy(Q_ElementBlock* dst, const Q_ElementBlock* src);

/* The lot-table pick helpers (0x803AB614, 0x803AB728, 0x803AB80C): `pick` walks a weighted table and
 * appends one entry per iteration, `pick_first` additionally forces the first iteration's roll to 0,
 * and `pick_last` is the form `quest_monster_setup` drives its own table with.  All three take the
 * caller's chance byte-table first, as the target's own register use shows (`pick` does not read it). */
s32 quest_lot_pick_first(u8* chance, const Q_LotEntry* table, Q_ItemPair* out, s32 count, u16 total);
s32 quest_lot_pick(u8* chance, const Q_LotEntry* table, Q_ItemPair* out, s32 count, u16 total);
s32 quest_lot_pick_last(u8* chance, const Q_LotEntry* table, Q_ItemPair* out, s32 count, u16 total);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#ifdef __cplusplus
/* The move-work accessor this unit reaches every per-slot record through.  Its owner is
 * `ef/fn_800CDB2C.cpp`; that unit's own header cannot carry it (three other headers spell the same
 * mangling with a different return type, so a declaration there breaks ten units on
 * `illegal overloading`), so this is the one place the declaration is reachable from here.  Added
 * with the body pass, which nets the file's rule-2 set out by moving `move_work_state_ck` to its
 * owner's header.  At C++ scope, not inside the `extern "C"` block: the map's row is the mangling
 * `get_move_work_adrs__FUc`, and the four 100 %-scoring getters' `bl` carries the C++ spelling (the
 * C scope spelling is what makes `flipcheck.py` answer `undefined: 'get_move_work_adrs'`). */
/* untyped: opaque work-area handle passed through - the record's type depends on the slot index */
void* get_move_work_adrs(u8 index);
#endif

#endif /* MHTRI_QUEST_QUEST_ENTRY_H */
