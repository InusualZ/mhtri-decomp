/*
 * `quest/quest_entry.cpp`'s declarations (docs/plan.md 6.5 rule 2): the body of the quest entry band
 * (`.text` 0x803AB3BC..0x803B0F98).  The band's head is `quest/quest_item_slot.h`'s, the records are
 * `quest/quest_types.h`'s.  The foreign functions the unit calls are declared here too because the band
 * they live in has no registered owner yet (the map resolves them to unsplit addresses, and the units
 * bracketing them name different modules, so no `include/unsplit/<module>.h` is sound).  A later pass that
 * registers those bands moves each declaration to its owner's header.
 *
 * Names: every one is derived from the callee's own body and is marked GUESS in the unit header - the
 * runtime dump answers `zz_` for all of them.
 */
#ifndef MHTRI_QUEST_QUEST_ENTRY_H
#define MHTRI_QUEST_QUEST_ENTRY_H

#include "types.h"
#include "quest/quest_types.h"
#include "quest/quest_item_slot.h"  /* the band's head: consumers of the whole band include this one header */
#include "quest/quest_result_enter.h"  /* quest_result_enter, quest_start_enter, quest_reward_faint_penalty (leaf header) */
#include "quest/quest_list_values.h"  /* quest_list_values (this unit defines it) */
#include "menu/menu_item.h"     /* GetItemData - owned by menu/menu_item.cpp (rule 2) */
#include "fn_80047398.h"        /* item_pair_copy - owned by fn_80047398.cpp (rule 2) */

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
 * walk, and a set/clear mode (0x803AB438). */
void quest_pl_skill_slot_set(_PLW* owner, u8* chance, u8 mode);

/* The quest-phase and per-slot state getters (0x803AD8F4, 0x803AE934, 0x803B0D5C, 0x803AFB90). */
u8  quest_phase_get(void);
u32 quest_item_work_flag_ck(void);
u32 quest_move_sub_state_ck(void);
u32 quest_work_busy_ck(void);
/* The index of the loaded element a quest id names (0x803ADF84), or -1. */
s32 quest_element_find(u8 kind);
/* The band's two quest-work predicates (0x803B0CD4, 0x803B0CFC). */
u32 quest_entry_active_ck(void);
u32 quest_entry_ready_ck(void);

/* Spawns the quest's monsters into the slot's move work, and (phase `phase`) the area's own (0x803AE424,
 * 0x803AE030).  Bodies unwritten: each names `lbl_805F76A0`/`lbl_805F4F80`, data this unit cannot claim. */
void quest_monsters_spawn(struct Q_ItemWork* item);
void quest_monster_spawn_area(u8 phase, struct Q_MoveWork* work);
/* Spawns the current quest work's monsters (0x803ADF40). */
void quest_monsters_spawn_now(void);
/* Steps the quest finish sequence (0x803B0DC4). */
void quest_finish_step(void);
/* Credits the finished quest's count runs to the save block and clears the quest result work (0x803AD114). */
void quest_result_work_flush(void);
/* Re-rolls the quest work's clock words, reseeds the quest random source and clears the per-quest
 * state (0x803ADC44). */
void quest_work_start_reset(void);
/* Warps the player to the quest's start position (0x803AFBEC). */
void quest_start_warp(struct _PLW* plw);
/* Builds the area handle list from the result row's area block (0x803AEBC0). */
void quest_area_list_init(void);
/* Rebuilds the area handle list for the entry kind `kind` once (0x803AEE20); `item` is the item work the
 * caller holds (the body reads the global work instead). */
void quest_area_list_refill(struct Q_ItemWork* item, u8 kind);
/* Releases the armed elements' monsters, flags the hand-off and loads the lobby area (0x803ADA70). */
void quest_monsters_release(void);
/* Loads the carried-item pouch into the quest work (0x803AD92C). */
void quest_work_pouch_load(void);
/* Marks graded elements and derives the quest grade (0x803ADD24). */
void quest_grade_set(void);
/* The record's own +0x8B state byte (0x803ADF48). */
u8  quest_record_state_get(struct QuestRecord* rec);

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

/* 0x803AD47C - clears the quest work block and returns it; `kind` selects the block's mode.  Added with
 * `quest/arenatask.cpp`, which reads the record the block points at (`record_0x03C`).  The block type is
 * `unsplit/menu.h`'s `QuestWork`. */
struct QuestWork;
QuestWork* quest_init(u8 kind);
#endif

#endif /* MHTRI_QUEST_QUEST_ENTRY_H */
