/* The quest head's accessors, `quest/quest_entry.cpp`'s (0x803B465C..0x803B744C): the result-row field and
 * flag tests, the arena element passes, the score, supply and bonus steps, the extra-condition tests and the
 * staging-download version helpers.  `enemy/em_pop.h` includes this header for its consumers. */
#ifndef MHTRI_QUEST_QUEST_RESULT_FIELD_TEXT_CUR_GET_H
#define MHTRI_QUEST_QUEST_RESULT_FIELD_TEXT_CUR_GET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x803B6078 - fills two u16s from the work's per-slot pair when the slot is armed. */
u16 em_work_slot_pair_get(u16 index, s16* out);

/* The quest/arena accessors at 0x803B4BEC..0x803B68F0 (`quest_flag_*_ck`, `quest_arena_*_get`,
 * `quest_element_value_get`); their callers are the enemy, lobby, menu and quest units. */
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
/* 0x803B5700 / 0x803B58B8 - step the special elements and the grade's team goals of the work block. */
void quest_special_element_step(struct QuestWork* work);
void quest_grade_goal_step(struct QuestWork* work);
/* 0x803B6B14 / 0x803B6DEC - step the quest's supply drops; pick the quest's bonus byte. */
void quest_supply_drop_step(void);
u8 quest_bonus_pick(void);
/* 0x803B7154 - the record's extra condition (`quest_ex_condition_tbl` entry +0x198): kinds 0/4 pass, 1 tests
 * `param` against the entry's range, 2 tests `arg` against its byte, 3 needs the board data's six counters at 0.
 * GUESS name. */
struct LbQuestBoardData;
u32 quest_ex_condition_ck(QuestRecord* rec, u16 param, u8 arg, struct LbQuestBoardData* data);
/* 0x803B700C - the same test with the row's first kind, for the quest list (GUESS name). */
u32 quest_ex_condition_show_ck(QuestRecord* rec, u16 param, u8 arg, struct LbQuestBoardData* data);

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
struct Q_QuestStat* quest_stat_get(void);
u32 quest_key_row_ck(u8 key);
u32 quest_key20_flag_ck(u8 invert);

/* 0x803B7434 - whether the downloaded file in `buffer` carries `version` (its u16 at +0x2C); the
 * network band's staging download checks with it. */
u32 matchesFileVersion(const u8* buffer, u16 version);
/* 0x803B729C - steps the staging download of the staged file whose header carries `version` (-1 when none
 * does).  GUESS name from the loop over `net_ctrl_wk`'s ten file buffers. */
s32 stepStagingDownloadForVersion(u16 version);

struct Q_ItemWork;
/* 0x803B5658 - fills the quest result work's stat block (+0x3E0) from the item work at the end of a hunt of
 * a finished quest: cleared when the item work carries a stat, else the em020 hit info.  GUESS name. */
void quest_result_stat_fill(struct QuestWork* item);
/* 0x803B5408 - the objective code of a result row (the current one when `rec` is NULL), from its +0x310
 * flag word: 1, 9, 2, 3 or 4, and 0 while the slot is in its entry state or no row exists.  GUESS name. */
u8 quest_objective_get(QuestRecord* rec);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_QUEST_QUEST_RESULT_FIELD_TEXT_CUR_GET_H */
