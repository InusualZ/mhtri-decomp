/*
 * `quest/quest_entry.cpp`'s declarations (docs/plan.md 6.5 rule 2): the body of the quest entry band
 * (`.text` 0x803AB3BC..0x803B0F98).  The band's head is `quest/quest_item_slot.h`'s, the records are
 * `quest/quest_types.h`'s.  The foreign functions the unit calls are declared here too because the band
 * they live in has no registered owner yet (the map resolves them to unsplit addresses, and the units
 * bracketing them name different modules, so no `unsplit/<module>.h` is sound).  A later pass that
 * registers those bands moves each declaration to its owner's header.
 *
 * The result screen's record accessors and text getters (0x803B0F98..0x803B465C) are declared here too.
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
#include "menu/menu_item.h"     /* GetItemData - owned by menu/menu_item.cpp (rule 2) */
#include "fn_80047398.h"        /* item_pair_copy - owned by fn_80047398.cpp (rule 2) */
#include "menu/menu_sysmsg.h"    /* quest_grade_none_text_table - owned by menu/menu_sysmsg.cpp (rule 2) */
#include "menu/quest_str_tbl_35_get.h"  /* quest_str_tbl_35_get (leaf header) */
#include "lobby/lb_quest_screen.h"      /* move_work_item_work_get - owned by lobby/lb_quest_screen.cpp (rule 2) */

/* The quest work block and the band's data (`quest/quest_entry.cpp` owns the ranges; the
 * record types are `quest/quest_types.h`'s). */
extern QuestWork quest_work;              /* .bss 0x806C5858 */
extern QuestWork* quest_work_ptr;         /* .sbss 0x80794C40, set by `quest_init` */
/* The u16 key/value array inside the quest list block `quest_list_load_hunt`/`_arena` allocate (block + 0x1A0;
 * `.sbss` 0x80794C24, the third word of the list-block run). */
extern u16* quest_list_values;
/* The 0x64400-byte quest list block: the record pointers, their keys and the list file image (the size of the
 * high-tier hunt list file, `quest_file_table[1]`). */
typedef struct QuestListBlock {
    /* +0x000 */ QuestRecord* items[0x68];
    /* +0x1A0 */ u16 values[0x70];
    /* +0x280 */ u8 file[0x64180];
} QuestListBlock; /* size: 0x64400 */
/* The reward roll tables (.sbss 0x80794C28..0x80794C3C, set outside this unit): the per-key part lists of
 * (part, tier) byte pairs, the part-break lot tables per tier and the capture lot tables per key, each for the
 * low and the high progress. */
extern u8** em_bui_tbl;
extern Q_LotEntry** em_bui_rem_l;
extern Q_LotEntry** em_bui_rem_h;
extern Q_LotEntry** em_hokaku_rem_l;
extern Q_LotEntry** em_hokaku_rem_h;
/* The list block itself (.sbss 0x80794C20) and the file image inside it (.sbss 0x80794C1C). */
extern QuestListBlock* quest_list_pool;
extern u8* quest_list_file;
/* The quest list block's item array and its count (`quest_work_word_get` walks them with
 * `quest_list_values`). */
extern QuestRecord** quest_list_items;     /* .sbss 0x80794C3C */
extern s32 quest_list_count;               /* .sbss 0x80794C44 */
extern char quest_text_buffer[0x100];     /* .bss 0x806CC310, the 100-byte text scratch */
extern u8 quest_pair_table[];      /* .data 0x805F7898, read [index * 2 + sub] */
extern u8 quest_byte_table[];      /* .data 0x805F78B4 */
extern u16* arena_time_table[];    /* .data 0x805F7AF8, 12 pointers to u16 time tables */
extern u16 multi_arena_clr_time[];     /* .data 0x805F7B28, 10 u16 pairs */
/* Pooled float constants the target objects address as globals (playbook 29: declare, never
 * define - a definition would make MWCC emit a second copy in `.sdata2`). */
extern const f32 frames_per_second_60f;   /* .sdata2 0x8079C524 */
extern const f32 percent_scale_100f;      /* .sdata2 0x8079C558 */
extern const f32 quest_grade_ratio_10f;   /* .sdata2 0x8079C540 */
extern const f32 quest_grade_ratio_30f;   /* .sdata2 0x8079C55C */
extern const f32 quest_intruder_wait_3f;  /* .sdata2 0x8079C54C: the intruder's three-second wait */
extern const f64 quest_timer_zero_d;      /* .sdata2 0x8079C510: 0.0 */
extern const f64 quest_timer_start_d;     /* .sdata2 0x8079C518: 29970.0 */
/* The sixteen ascending quest-id thresholds `quest_init` scans per list record (.data 0x805F2A98). */
extern u16 quest_id_threshold_tbl[16];
extern const f32 quest_grade_ratio_50f;   /* .sdata2 0x8079C520 */

/* One 8-byte entry of a monster's variant list: the quest rank it applies to (0xFF ends the list) and the
 * monster slot records (the work block's own `QuestEntrySlot` layout) a slot pair's group indexes. size: 0x8 */
typedef struct QuestMonsterVariantList {
    /* +0x0 */ u8 rank;
    /* +0x1 */ u8 unused_0x1[0x3];
    /* +0x4 */ QuestEntrySlot* records;
} QuestMonsterVariantList;
/* The callback table the dump names `nora_set_proc` (.bss 0x806CC410, filled at run time): an element
 * check, a row/work key match and the weighted lot table of a monster id. size: 0x10 */
typedef struct NoraSetProc {
    /* +0x0 */ u8 (*element_ck)(u8 kind, u8 index, struct QuestWork* work);
    /* +0x4 */ u8 (*key_match)(u8* row_keys, u8* work_keys);
    /* +0x8 */ Q_LotEntry* (*lot_table_get)(u16 monster);
    /* +0xC */ u8 unused_0xC[0x4];
} NoraSetProc;
extern NoraSetProc nora_set_proc;

/* The area monster lists the hunt spawns from, indexed by the move work's area (.data 0x805F4F80, 24 entries). */
extern QuestEntrySlot* quest_area_monster_tbl[24];

/* The minute marks `quest_enter_load` classes the time limit by (.data 0x805F78C4: 0, 5, 10). */
extern s32 quest_time_class_tbl[3];

/* The supply drop schedules by time rank (.data 0x805F79A4, 13 lists of (minute mark / 2, chance in 128) byte
 * pairs, 0 ends a list), and the bonus lists by name index and the row's +0x37A word (.data 0x805F7A7C, 13 tables
 * of (weight, value) byte pairs ending at 0xFF).  Read by `enemy/em_pop.cpp`'s supply and bonus steps. */
extern u8* quest_supply_drop_tbl[13];
extern u8** quest_bonus_pick_tbl[13];

/* The per-monster variant lists, indexed by monster id (.data 0x805F76A0, 41 entries, NULL for none). */
extern QuestMonsterVariantList* quest_monster_variant_tbl[41];
/* The free hunt's area set rows (one set-file number per area, -1 for none): by lobby quest (.data 0x805F7744) and
 * by season (.data 0x805F77C8). */
extern s16 em_set_quest_area_tbl[5][13];
extern s16 em_set_season_area_tbl[8][13];

/* The band's roll-threshold tables (`.data` 0x805F7AB0..0x805F7AF8, claimed and DEFINED by
 * `src/quest/quest_entry.cpp`): each `chance` table is a run of one byte per pick slot, 32 or 22, in
 * the same shape `quest_element_clear` builds in-line.  `_a` is the four-pick form `quest_pair_roll_all`
 * copies, `_b` the two-pick form `quest_pair_roll_first`/`_last` copy and `_c` the one
 * `quest_monster_setup` copies; each is used as two 8-entry halves.  The reward-group table is
 * `quest_part_reward_roll`'s.  Nothing writes any of them. */
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

/* `move_work_item_work_get` is `lobby/lb_quest_screen.h`'s (included above). */
/* The low byte of a result record's +0x36C word; a NULL record means the local slot (the two views of this function
 * - the entry band's `s32 slot` caller and the arena band's record - are one declaration). */
u8 quest_slot_progress_get(QuestRecord* rec);

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

/* Roll the part-break and the capture rewards into the picked-pair tables (0x803ABE44, 0x803AC6B0). */
void quest_part_reward_roll(QuestWork* work);
void quest_capture_reward_roll(QuestWork* work, _PLW* owner);

/* The arena pair-roll variants the result fill `quest_result_fill` dispatches between by quest
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

/* Spawns the quest's monsters into the slot's move work, and the area spawn for rank `rank` (0x803AE424,
 * 0x803AE030). */
void quest_monsters_spawn(struct Q_ItemWork* item);
void quest_monster_spawn_area(u8 rank, struct Q_MoveWork* work);
/* Rolls the tier monsters' lot tables into the second picked-pair table (0x803ACD10). */
void quest_monster_setup(Q_ItemWork* item, _PLW* owner);
/* Fills the work block's monster slots from the current result row's slot pairs (0x803AE990). */
void quest_pair_apply(void);
/* Spawns the current quest work's monsters (0x803ADF40); `unused` is not read (the one caller passes 0). */
void quest_monsters_spawn_now(s32 unused);
/* Steps the quest finish sequence (0x803B0DC4). */
void quest_finish_step(void);
/* Credits the finished quest's count runs to the save block and clears the quest result work (0x803AD114). */
void quest_result_work_flush(void);
/* Allocate the quest list block and load the hunt / arena list file into it (0x803AD200, 0x803AD6E0). */
void quest_list_load_hunt(void);
void quest_list_load_arena(void);
/* Re-rolls the quest work's clock words, reseeds the quest random source and clears the per-quest
 * state (0x803ADC44). */
void quest_work_start_reset(void);
/* Warps the player to the quest's start position (0x803AFBEC). */
void quest_start_warp(struct _PLW* plw);
/* Enters the quest: the result buffer, the reset, the time limit and the stage's dcm archives (0x803AF4B4). */
void quest_enter_load(void);
/* The text a result field kind shows for the row `rec` (0x803B4BEC..). */
char* quest_result_field_text_get(QuestRecord* rec, u8 kind);
/* The quest's per-frame step (0x803B01C4) and its two extra-monster steps: the intruder (quest flag 0x100 or
 * the row's +0x32C chance, 0x803B0F98) and the successive hunt (quest flag 0x4000000, 0x803B177C). */
void quest_main_step(void);
/* Loads the quest the lobby picked into the work block (0x803AEED0). */
void quest_start_load(void);
void quest_intruder_step(void);
void quest_successive_step(struct Q_MoveWork* work);
/* Prepares the item work for a quest entry from its result row (0x803AFD44). */
void quest_entry_setup(struct Q_ItemWork* item);
/* Builds the area handle list from the result row's area block (0x803AEBC0). */
void quest_area_list_init(u8 kind);
/* Rebuilds the area handle list for the entry kind `kind` once (0x803AEE20); `item` is the item work the
 * caller holds (the body reads the global work instead). */
void quest_area_list_refill(u8 map, u8 kind);
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

/* The quest/arena result record accessors and the result screen's text getters (0x803B0F98..0x803B465C,
 * absorbed from `menu/arena_result.cpp`): `quest_record_get` hands back the record the quest work block's
 * `record_0x03C` points at and `quest_record_find` looks a record up by quest id.  The record type stays in the
 * shared header `quest/quest_types.h`. */

/* The current result row, or NULL when the screen has none. */
QuestRecord* quest_record_get(void);
/* 0x803B20B8 / 0x803B21E8 - the result's kept (category 1) and delivered (category 0x10) item lists, moved out
 * of the player's 35 item slots; 0x803B2354 / 0x803B2800 - the quest and the arena result fills. */
void quest_result_keep_items(Q_ItemPair* items, Q_ResultWork* result);
u8 quest_result_deliver_items(Q_ResultWork* result, Q_ItemPair* items);
void quest_result_fill(struct Q_MoveWork* work);
void quest_arena_summary_step(struct Q_MoveWork* work);
/* 0x803B2958 - the gallery cell check of the quest's area 1 (GUESS name). */
void quest_gallery_cell_step(struct Q_MoveWork* work);
/* The key the quest list pairs with quest id `key`, and whether exactly one player key is set and all are done. */
u16 quest_work_word_get(u16 key);
s32 quest_players_state_get(void);
/* The quest's elapsed frames, and the prune of the unusable entries of a 35-slot item list. */
s32 quest_elapsed_time_get(void);
void quest_item_list_prune(QuestItemSlot* slots);
/* 0x803B421C / 0x803B4308.. - the row's name index and its +0x372 item id (the current row for NULL). */
u8 quest_name_index_get(QuestRecord* rec);
u16 quest_field372_get(QuestRecord* rec);

/* The result record of quest `quest_id` from the loaded quest list (or the network control's slots), or
 * NULL when none carries that id. */
QuestRecord* quest_record_find(u16 quest_id);

/* 0x803B33B0 / 0x803B33E8 - the quest message string tables 35 and 4: the string at `index`
 * (`quest_str_tbl_35_get` is declared in the leaf header below). */
char* quest_str_tbl_4_get(u32 index);

/* 0x803B2D50 - how many of item `id` the player `who` holds (the quest band's element accessors
 * subtract it from an element's target). */
s16 quest_item_count_sum(u16 id, s32 who);

/* 0x803B2C60 - spends `count` of item `id` against the three elements that ask for it, then sends the new total.
 * 0x803B31E0 - an element's supply state: 0 while the pad input blocks it, 1 when nothing is left to deliver,
 * 2 otherwise.  The signatures are `Pl/pl_act.cpp`'s call sites. */
u32 quest_element_item_use(_PLW* owner, u16 id, s32 count);
/* 0x803B2EC8 - deposits up to `count` of item `id` from `owner` into its quest item list; returns the count moved. */
u16 quest_item_deposit(_PLW* owner, u16 id, s16 count);
/* 0x803B2AFC / 0x803B2BB8 - re-checks the live elements from `index` on against item `id` (`delta` is the
 * caller's change, unread); the element's id and what it still asks for. */
void quest_element_value_set(s32 index, u16 id, s32 delta);
u16 quest_element_item_count_get(s32 index, u16* out);
s32 quest_element_supply_state_get(s32 index);
/* 0x803B2A0C - steps the first quest's intro countdown held in the move work `work`. */
void quest_intro_step(struct Q_MoveWork* work);
/* 0x803B3310 - warps the local player to entry point 2 of the current map. */
void quest_entry_point_warp(void);

/* 0x803B3454.. - the result screen's text getters: the `_of` forms take the row, the plain forms read the
 * current one; each returns `quest_text_buffer` or a string-table entry. */
char* quest_name_text_get(void);
char* quest_field198_text_get(void);
char* quest_field13A_text_get(void);
char* quest_time_text_get(u8 which);
char* quest_field348_text_get(void);
char* quest_field2E8_text_get(void);
char* quest_clear_time_text_get(void);
char* quest_elapsed_time_text_get(void);
char* quest_score_text_get(void);
char* quest_arena_items_text_get(u8 index);
char* quest_arena_time_text_get(QuestRecord* rec, s32 which);
char* quest_grade_text_get(u8 grade);
char* quest_grade_text_cur_get(void);
char* quest_monster_text_get(QuestRecord* rec, u8 which);


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
 * `ef/system_core.cpp`; that unit's own header cannot carry it (three other headers spell the same
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
 * `quest/quest_types.h`'s `QuestWork`. */
struct QuestWork;
QuestWork* quest_init(u8 kind);
#endif

#endif /* MHTRI_QUEST_QUEST_ENTRY_H */
