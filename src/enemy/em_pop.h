/* The enemy population/roster records `enemy/em_pop.cpp` owns: the 0x224-byte per-monster roster record (the `.bss`
 * tables `em_bui_tbl`, `em_hagi_tbl`, `em_drop_tbl` at 0x80794C28..0x80794C58 are runtime-dump names) and the manager
 * work they live in.  Sizes from the object's arithmetic: `work_mem_alloc(0x34A68)` for the work, `0x11200` for the
 * record array (`128 x 0x224`), `0x1600` for the sub-records, and `mulli r0, r3, 0x224` in `em_roster_record_get`.
 * `enemy/em033_prog.cpp`'s `EmRosterRec` (0x1F4, ending at the +0x1F0 approach radius) is a prefix view of it.
 */
#ifndef MHTRI_ENEMY_EM_POP_H
#define MHTRI_ENEMY_EM_POP_H

#include "types.h"
#include "nw4r/math.h"

struct _g3d_work;
struct _ENEMY_WORK;

/* One roster record.  Written by the allocator (`state_0x000 = 1`), read by every
 * `em_roster_record_*` helper. */
typedef struct EmPopRec {
    /* +0x000 */ u8 state_0x000;      /* 1 = live; 0 = free (`em_roster_free_record_get` finds these) */
    /* +0x001 */ u8 field_0x001;
    /* +0x002 */ u8 index_0x002;      /* the record's own slot number, stamped by the allocator */
    /* +0x003 */ u8 action_0x003;     /* the action code `em_roster_record_result_get` switches on */
    /* +0x004 */ u8 kind_0x004;       /* the "team"/kind byte the kind searches compare against */
    /* +0x005 */ u8 field_0x005;
    /* +0x006 */ s8 field_0x006;
    /* +0x007 */ u8 pad_0x007;
    /* +0x008 */ u8 field_0x008;
    /* +0x009 */ u8 field_0x009;
    /* +0x00A */ u8 field_0x00A;
    /* +0x00B */ u8 pad_0x00B[9];
    /* +0x014 */ nw4r::math::VEC3 pos_0x014;   /* the placement position */
    /* +0x020 */ u8 pad_0x020[0x124];
    /* +0x144 */ struct _g3d_work* g3d_0x144; /* the model handle released before the record is cleared */
    /* +0x148 */ u8 pad_0x148[0x88];
    /* +0x1D0 */ nw4r::math::VEC3 aim_0x1D0;   /* the aim position (same vector as `pos_0x014`) */
    /* +0x1DC */ u8 pad_0x1DC[0x14];
    /* +0x1F0 */ f32 radius_0x1F0;
    /* +0x1F4 */ u8 pad_0x1F4[6];
    /* +0x1FA */ u16 slot_id_0x1FA;   /* the id `em_roster_record_slot_id_get` reports (-1 when unset) */
    /* +0x1FC */ u8 pad_0x1FC[0x28];
} EmPopRec; /* size: 0x224 */

/* The sub-record array the manager allocates alongside the roster (stride 0x20C).  Only the two
 * offsets the helpers touch are named. */
typedef struct EmPopSubRec {
    /* +0x000 */ u8 field_0x000;
    /* +0x001 */ u8 pad_0x001[0x143];
    /* +0x144 */ struct _g3d_work* g3d_0x144;
    /* +0x148 */ u8 pad_0x148[0xC4];
} EmPopSubRec; /* size: 0x20C */

/* The manager work, allocated whole by the work allocator. */
typedef struct EmPopWork {
    /* +0x000 */ u8 pad_0x000[0x1C];
    /* +0x01C */ EmPopRec* recs;      /* 0x11200 B = 128 x 0x224 */
    /* +0x020 */ u32 rec_num;         /* 0x80 */
    /* +0x024 */ EmPopSubRec* subs;   /* 0x1600 B, stride 0x20C */
    /* +0x028 */ u32 sub_num;         /* 0x80 */
    /* +0x02C */ u8 pad_0x02C[0x34A08];
    /* +0x34A34 */ u8 field_0x34A34[0x10];
    /* +0x34A44 */ u8 field_0x34A44;
    /* +0x34A45 */ u8 field_0x34A45;
    /* +0x34A46 */ u8 pad_0x34A46;
    /* +0x34A47 */ u8 field_0x34A47;
    /* +0x34A48 */ u8 field_0x34A48;
    /* +0x34A49 */ s8 field_0x34A49[0x1F];
} EmPopWork; /* size: 0x34A68 */

/* The one work pointer the whole module goes through.  The map's symbol (`lbl_80794C58`) spans
 * 0x8 B and only its first word is ever read, so the second is carried as this unit's filler to
 * keep the object's `.sbss` contribution byte-exact. */
typedef struct EmPopWorkSlot {
    /* +0x00 */ EmPopWork* work;
    /* +0x04 */ u32 unused_0x04;
} EmPopWorkSlot; /* size: 0x8 */

#ifdef __cplusplus
extern "C" {
#endif

/* The roster accessors, the work pointer `em_pop_w` and the model-band helpers (0x803B936C..0x803BE30C) are
 * `enemy/em_model.cpp`'s, declared in `enemy/em_model.h`. */

/* 0x803B6078 - fills two u16s from the work's per-slot pair when the slot is armed.
 * 0x803B8E1C - the em_set work's own state word. */
u16 em_work_slot_pair_get(u16 index, s16* out);
s32 em_set_work_state_get(void);

/* The quest/arena accessors at 0x803B4BEC..0x803B68F0 (`quest_flag_*_ck`, `quest_arena_*_get`,
 * `quest_element_value_get`): in this unit's range but the quest/arena UI band's own (the unit is probably several
 * TUs, docs/enemy.md); their callers are the enemy, lobby, menu and quest units. */
struct QuestRecord;   /* defined in `quest/quest_types.h` (the arena-result band's record) */
/* The result record's +0x310 flag word, one test per bit; `rec` 0 means the current record (NULL
 * selects the work's own through `quest_record_get`).  The bit names are this header's reading of
 * the call sites, all in the quest board and the result screens; the 0x800000 predicate came in
 * with `quest/quest_entry.cpp`, whose entry test it is. */
u32 quest_flag_8_ck(QuestRecord* rec);
u32 quest_flag_10_ck(QuestRecord* rec);
u32 quest_flag_80_ck(QuestRecord* rec);
u32 quest_flag_100_ck(QuestRecord* rec);
u32 quest_flag_800_ck(QuestRecord* rec);
u32 quest_flag_40000_ck(QuestRecord* rec);
u32 quest_flag_80000_ck(QuestRecord* rec);
u32 quest_flag_100000_ck(QuestRecord* rec);
u32 quest_flag_200000_ck(QuestRecord* rec);
u32 quest_flag_800000_ck(QuestRecord* rec);
u32 quest_flag_1000000_ck(QuestRecord* rec);
u32 quest_flag_2000000_ck(QuestRecord* rec);
u32 quest_flag_2000000_only_ck(QuestRecord* rec);
u32 quest_flag_2000000_or_80000000_ck(QuestRecord* rec);
u32 quest_flag_4000000_ck(QuestRecord* rec);
u32 quest_flag_10000000_ck(QuestRecord* rec);
u32 quest_flag_40000000_ck(QuestRecord* rec);
u32 quest_flag_80000000_ck(QuestRecord* rec);
/* 0x803B465C - the text a result-screen field kind (0..31) shows for the current row.  GUESS name: the
 * `_cur_` twin of `quest_result_field_text_get`, which takes the row. */
char* quest_result_field_text_cur_get(u8 kind);
/* The result row's per-slot and odd-field accessors (`rec` 0 means the current row): the slot's byte and
 * halfword values, and the +0x37A / +0x32C / +0x32E fields.  GUESS names. */
u8 quest_slot_byte_get(QuestRecord* rec, u8 slot);
u16 quest_slot_word_get(QuestRecord* rec, u8 slot);
u16 quest_field37A_get(QuestRecord* rec);
u32 quest_field32C_ck(QuestRecord* rec);
u8 quest_field32E_get(QuestRecord* rec);
u32 quest_field32C_or_4000000_ck(void);
u32 quest_row394_get(u8 index);
u8 quest_objective_result_get(QuestRecord* rec);
/* The arena element `id`'s stored value; `out` gets it, and the return is 0 when no element
 * matches (0x803B5F28). */
s32 quest_element_value_get(u16 id, s16* out);
/* Element `index`'s stored value when `id` matches its own id, else -1 (0x803B5FF0). */
s32 quest_element_value_at(u16 id, u16 index);
/* Element `index`'s stored value, ignoring its id, else -1 (0x803B5F84). */
s32 quest_element_value_index_get(u16 index);
/* Element `index`'s value minus the local player's held count, clamped up to 0 (0x803B6150). */
s32 quest_element_remaining_get(u16 index);
/* The item `id`'s count over every player slot `system_w`'s player count covers, 0 when no element
 * carries the id (0x803B61DC). */
s32 quest_all_player_item_count_sum(u16 id);
/* How many of arena element `index`'s item the player still holds, and how many are still needed
 * (-1 when the element is not live). */
s32 quest_arena_count_get(s32 index);
s32 quest_arena_need_get(s32 index);
/* 0x803B6998 - takes `points` off the quest work's score (+0x5D8, clamped at 0) and plays the penalty sound at
 * enemy `unique_id`'s position.  GUESS name. */
void quest_score_deduct(u16 points, u16 unique_id);
/* 0x803B7154 - the record's extra condition (`quest_ex_condition_tbl` entry +0x198): kinds 0/4 pass, 1 tests
 * `param` against the entry's range, 2 tests `arg` against its byte, 3 needs the board data's six counters at 0.
 * GUESS name. */
struct LbQuestBoardData;
u32 quest_ex_condition_ck(QuestRecord* rec, u16 param, u8 arg, struct LbQuestBoardData* data);

/* Whether any of the item work's three elements is in its entry state: `quest_work_ptr` must be set,
 * the move work's own +0x22D4 state byte must not be 1, and one of the three element flag words at
 * +0x94/+0xF4/+0x154 needs bit 4 set together with bit 2 and bit 10 (with or without bit 4 again) -
 * 0x404 or 0x414.  Forces the pair rolls in `quest_pair_roll_all` (0x803B5E2C). */
u32 quest_element_state_ck(void);
/* The state (0x4014, 0x414 or 0x404) of the element whose low id byte is `key` (0x803B5CA4), and whether any
 * element carries state 0x404 (0x803B5D90).  GUESS names. */
u32 quest_element_state_find(u8 key);
u32 quest_element_404_ck(void);
/* 0x803B5B1C - scans the item work's three elements for the em020 hunt goal and arms the one whose team window
 * has closed (1 when one was armed).  GUESS name. */
u32 quest_element_window_check(struct QuestWork* work);
/* 0x803B62D8 / 0x803B63C0 / 0x803B64F4 - the arena element passes: a value clear test, a key clear, and the
 * per-frame countdown step (`first` gets the first finished index, -1 when none).  GUESS names. */
u32 quest_arena_value_clear_ck(u16 id, u16 value);
u32 quest_arena_key_clear(u8 key);
u32 quest_element_progress_step(u16 kind, u8 mode, s8* first);
/* 0x803B6AC4 / 0x803B6F2C / 0x803B6F90 - the +0x6AA4 run, and the key-row tests.  GUESS names. */
u8* quest_field6AA4_get(void);
u32 quest_key_row_ck(u8 key);
u32 quest_key20_flag_ck(u8 invert);

/* 0x803B7434 - whether the downloaded file in `buffer` carries `version` (its u16 at +0x2C); the
 * network band's staging download checks with it. */
u32 matchesFileVersion(const u8* buffer, u16 version);
/* 0x803B729C - steps the staging download of the staged file whose header carries `version` (-1 when none
 * does).  GUESS name from the loop over `net_ctrl_wk`'s ten file buffers. */
s32 stepStagingDownloadForVersion(u16 version);
/* The staged file's header as `matchesFileVersion` reads it; only the version halfword is evidenced.
 * size: 0x2E (lower bound) */
typedef struct StagingFileHeader {
    /* +0x00 */ u8 unused_0x00[0x2C];
    /* +0x2C */ u16 version_0x2C;
} StagingFileHeader;

/* 0x803B88A8 / 0x803B88EC - the local slot's move-work sub-state: read whether it is 4 and set it to 4.  GUESS names. */
s32 quest_move_sub_state_4_get(void);
void quest_move_sub_state_4_set(u8 flag);

/* One entry of a 0xFFFF-terminated weight table: its `weight` and the `value` a pick returns. size: 0x4 */
typedef struct EmWeightEntry {
    /* +0x0 */ u16 weight;
    /* +0x2 */ u16 value;
} EmWeightEntry;
/* 0x803B8E40 - settles the carried-item pouch (`Q_ItemPair` x35): returns the credit, `flag` set when a
 * category-2 item was cleared.  GUESS name. */
struct Q_ItemPair;
s32 quest_pouch_items_settle(struct Q_ItemPair* pouch, u8* flag);

struct Q_ItemWork;
/* 0x803B5658 - fills the quest result work's stat block (+0x3E0) from the item work at the end of a hunt of
 * a finished quest: cleared when the item work carries a stat, else the em020 hit info.  GUESS name. */
void quest_result_stat_fill(struct QuestWork* item);
/* 0x803B5408 - the objective code of a result row (the current one when `rec` is NULL), from its +0x310
 * flag word: 1, 9, 2, 3 or 4, and 0 while the slot is in its entry state or no row exists.  GUESS name. */
u8 quest_objective_get(QuestRecord* rec);

/* 0x803B8650 - warps the local player to the start of stage 1 (the hub): the stage's start position and
 * facing go through `pl_warp_start` with the player's chunk offset.  GUESS name from that use. */
void quest_warp_hub(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_EM_POP_H */
