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

/* The result row `Q_ItemWork::record_0x3C` points at, as this unit reads it: only the word at +0x310,
 * whose bit 0x00800000 marks a live row.  `menu/arena_result.cpp`'s own view of the same record
 * (`include/unsplit/menu.h`'s `QuestRecord`) names the rest of it; that header is not this unit's to
 * extend, so the prefix this unit reads stays here - merging the two views is a follow-up. */
struct Q_ResultRow {
    /* +0x000 */ u8 pad_0x000[0x310];
    /* +0x310 */ u32 flags_0x310;   /* bit 0x00800000: the row is live */
};  /* size: 0x314 (a prefix of the 0x714-byte row) */

/* The item work (`get_move_work_adrs(0)->0xDC`), the same block `quest_work_ptr` points at; only the
 * offsets this unit names are here (the lot tables' run at 0x9C is one region, addressed but not
 * walked by name).  MEASURED size 0x6AB8 (`accessextent.py quest_work` / `quest_work_ptr`):
 *   * literal - `quest_init` clears the whole block, `memset(quest_work_ptr, 0, 0x6AB8)` at 0x803AD4BC,
 *     and `fn_803B7F70` does the same at 0x803B7FA0 (`accessextent.py quest_work`, the object form);
 *   * cross-check - the block's readers reach 0x6AB4 at their furthest (`fn_803AEED0`'s `stb r0,27316`),
 *     i.e. 0x6AB8 rounded up, over 364 resolved accesses in 38 functions;
 *   * the tool's own DISAGREEMENT line here is the OTHER literal: `memcpy(quest_work_ptr, ..., 0x6000)`
 *     at 0x803AD680 copies the 0x6000-byte save sub-block INTO the record, so 0x6000 is not its extent.
 * The .bss row tiles the record exactly (`quest_text_buffer` begins at 0x806CC310 = 0x806C5858 + 0x6AB8). */
struct Q_ItemWork {
    /* +0x0000 */ u8 pad_0x0000[0x3C];
    /* +0x003C */ Q_ResultRow* record_0x3C;     /* the current result row, 0 when there is none */
    /* +0x0040 */ Q_ItemCount slots_0x40[5];  /* the five delivered-item slots `quest_item_slot_add` merges into */
    /* +0x0054 */ u8 rot_0x54;                /* the round-robin slot the next new id takes */
    /* +0x0055 */ u8 flag_0x55;               /* set when a slot changed */
    /* +0x0056 */ u8 pad_0x0056[0x9C - 0x56];
    /* +0x009C */ union {
        /* +0x009C */ Q_LotEntry lot_0x9C[0xFD];  /* the first weighted lot table */
        struct {
            /* +0x009C */ Q_LotEntry lot_e0a_0x9C[0xB];   /* element 0's own table (see `lot_e0b`) */
            /* +0x00C8 */ Q_LotEntry lot_e0b_0xC8[0xD];   /* its second table: `quest_pair_roll_first` walks both */
            /* +0x00FC */ Q_LotEntry lot_e1_0xFC[0x18];   /* element 1's table */
            /* +0x015C */ Q_LotEntry lot_e2_0x15C[0xCD];  /* element 2's table */
        };
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

/* The game's save/user block, seen only as the four count blocks this unit reads.  It is the arena
 * buffer `get_userdata` hands out (`*(void**)(&system_w + 0x95C)`, set by `ef/fn_800CDB2C.cpp`'s
 * `fn_800CF3E4`), and 0x6000 is the whole extent `fn_80047398.cpp`'s `fn_800497B4`/`fn_800498EC`
 * clear with `memset(get_userdata(), 0, 0x6000)`.  Nothing reads at or past it: of the 344 functions
 * that reach the block the furthest is `menu/get_pop_dat_ptr.cpp`'s `fn_803C0F3C`, at `+0x5364` for
 * sixteen 2-byte entries, so the run past the count sets stays unnamed filler. */
struct Q_UserData {
    /* +0x0000 */ u8 pad_0x0000[0x490];
    /* +0x0490 */ Q_CountSet set_c;
    /* +0x04E0 */ u8 pad_0x04E0[0x2];
    /* +0x04E2 */ Q_CountSet set_d;
    /* +0x0532 */ u8 pad_0x0532[0x348E];
    /* +0x39C0 */ Q_CountSet set_a;
    /* +0x3A10 */ u8 pad_0x3A10[0xB0];
    /* +0x3AC0 */ Q_CountSet set_b;
    /* +0x3B10 */ u8 pad_0x3B10[0x6000 - 0x3B10];
};  /* size: 0x6000 */

/* The per-slot "move work" `get_move_work_adrs` indexes; only the fields this unit names are here.
 * MEASURED size 0x22E8 (was 0x22E0 approximate): the record is the stride its own allocator clears -
 * `ef/fn_800CDB2C.cpp`'s `fn_800CF948` sizes slot 0's block as `max * 0x22E8` (`mulli r30,r3,8936` at
 * 0x800CF9D4) and memsets exactly that (0x800CFA68), and no other site in the DOL indexes a move work by
 * a different constant (`grep 8936` = 1 site).  The literal and the reach disagree by 4 bytes, which is
 * the interesting case: the furthest access anything makes is +0x22E3 (`lb_area_change_flag`'s
 * `stb r0,8931(r3)`, the lobby band's own `flag_0x22E3`), so the inferred reach is 0x22E4 and the last 4
 * bytes are cleared but never read.  The literal decides - it is what the allocator reserved and what
 * `max` multiplies - so the tail stays filler. */
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
    /* +0x22DD */ u8 pad_0x22DD[0x22E8 - 0x22DD];  /* cleared, never read: the lobby's own view names
                                                   * +0x22E3, this one reads up to +0x22DC */
};  /* size: 0x22E8 */

/* One 3-byte entry of the reward-group table `fn_803ABE44` indexes by a group count (clamped to the
 * last entry) and reads `picks`/`flag` from: the band's own roll table at `.data` 0x805F7AC0. */
struct Q_RewardGroup {
    /* +0x0 */ u8 index;   /* the entry's own number, 0..5 */
    /* +0x1 */ u8 picks;   /* how many roll slots the group fills */
    /* +0x2 */ u8 flag;    /* the pair kind the rolls are stored with */
};  /* size: 0x3 */

/* The band's roll-threshold tables (`.data` 0x805F7AB0..0x805F7AF8, claimed and DEFINED by
 * `src/quest/quest_entry.cpp`): each `chance` table is a run of one byte per pick slot, 32 or 22, in
 * the same shape `quest_element_clear` builds in-line.  `_a` is the four-pick form `quest_pair_roll_all`
 * copies, `_b` the two-pick form `quest_pair_roll_first`/`_last` copy and `_c` the one
 * `quest_monster_setup` copies; each is used as two 8-entry halves.  The reward-group table is
 * `fn_803ABE44`'s.  Nothing writes any of them. */
extern u8 quest_pair_chance_tbl_a[0x10];
extern u8 quest_pair_chance_tbl_b[0x10];
extern u8 quest_pair_chance_tbl_c[0x10];
extern u8 quest_pair_chance_tbl_d[0x4];
extern Q_RewardGroup quest_reward_group_tbl[6];

/* The unit's declarations.  C linkage: the two units that already call into this band
 * (`menu/menu_item_page.cpp`, `lobby/lb_companion_ui.cpp`) declare them inside their own `extern "C"`
 * blocks, so the original symbols were C-linkage names. */
#ifdef __cplusplus
extern "C" {
#endif

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
 * them (`quest_element_clear` refills `_a` from the item work's lot table, `quest_pair_roll_all` fills `_a` at
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

/* The arena pair-roll variants `menu/arena_result.cpp`'s `fn_803B2354` dispatches between by quest
 * flag (0x803AB914 is its fallback, 0x803ABB74 and 0x803ABCDC the two flagged arms): each copies one
 * of the band's chance tables in, sets the pl skill slots and zeroes all four pair tables, then fills
 * `quest_item_pair_tbl_a` from the item work's own lot tables.  The first parameter is the caller's
 * player record - `quest_pl_skill_slot_set` hands it to `Pl_Skill_ck__FP4_PLWUs`, which is what fixes
 * the type. */
void quest_pair_roll_all(_PLW* owner, u8 state);
void quest_pair_roll_first(_PLW* owner, u8 state);
void quest_pair_roll_last(_PLW* owner, u8 state);
/* The pl-skill slot fix-up the three rolls call first: the 4-element chance buffer the picks then
 * walk, and a set/clear mode.  Body unwritten (0x803AB438). */
void quest_pl_skill_slot_set(_PLW* owner, u8* chance, s32 mode);
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
/* The save/user block `get_userdata` returns - its band has no registered owner, so the declaration
 * lives here, and at C++ scope like `get_move_work_adrs` below: the map's row is the mangling
 * `get_userdata__Fv` (0x8004D120), so a C-scope declaration makes the object relocate an unmangled
 * `get_userdata` that no link input defines and `undefrefs.py` answers NOT READY. */
struct Q_UserData* get_userdata(void);

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
