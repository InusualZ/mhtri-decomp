/* The declarations `lobby/lb_quest_screen.cpp` owns.  `note_pane_get_motion` takes `NoteWork*`, the record
 * `enemy/em_prog_support.cpp`'s pane drives (the 0x1F8 bytes `qn_get_motion_no` reads as `_QNPC_W`). */
#ifndef MHTRI_LOBBY_LB_QUEST_SCREEN_H
#define MHTRI_LOBBY_LB_QUEST_SCREEN_H

#include "types.h"

/* The record `note_pane_get_motion` takes, forward-declared so that this header stays a leaf: it is
 * included from `unsplit/menu.h`, which the whole menu band takes. */
struct NoteWork;
struct QuestWork;
struct QuestElement;
struct Q_MoveWork;
struct Q_ItemWork;
struct LbCompanionSlot;
struct _ENEMY_WORK;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x803A4E58 - reads the note NPC record's motion number (the +0x54 halfword `qn_get_motion_no`
 * returns) with the retail `VEC3 v; VEC3_ctor(&v);` call-site idiom in front of it. */
void note_pane_get_motion(struct NoteWork* self);

/* 0x803A9B7C - applies a kill another player reported (area list, spawn order, count, kill or capture, quest
 * element): takes the count off the spawn record and books it.  GUESS name. */
void quest_net_kill_apply(u8 area, u16 order, u16 count, s32 how, u8 element);

/* 0x803A87A8 - the item work of the local slot, or NULL when the slot has none. */
struct Q_ItemWork* move_work_item_work_get(void);

/* The quest-work accessors: the time left (+0x24) and the base time (+0x20), both clamped up to 0, the whole limit
 * (+0x1C), the work's +0x6AA0 state word, the slot's sub-state test, and the low byte of clock word `index` (+0x6C,
 * 0 without an item work). */
s32 quest_time_elapsed_get(void);
s32 quest_time_base_get(void);
s32 quest_time_limit_get(void);
s32 quest_work_state_get(void);
s32 quest_sub_state_end_ck(s32 flag);
u8 quest_clock_byte_get(u8 index);

/* 0x803A9F28 - files `index` in the first free one of the item work's three finish-order bytes (+0x309) and, unless
 * the element's own flags or the current record rule it out, announces the element and copies its reward cell.
 * `flag` is the caller's, never read.  GUESS name. */
void quest_element_done_mark(struct QuestWork* work, struct QuestElement* slot, u16 index, u32 flag);

/* 0x803A8DA4 - seeds the quest random source (the item work's 0x5A halfword) with the sum of the seven (low, high << 8)
 * pairs of the clock snapshot at +0x6C, or 451 when that is 0.  GUESS name. */
void quest_rand_seed_set(void);
/* 0x803A8EE4 - the next quest random value (x176 mod 65363 on the 0x5A halfword, state 0 taken as 1; 0 without an item
 * work); `u32` because the caller masks the 16-bit value. */
u32 quest_rand_next(void);

/* 0x803A7E68 - files the monster slots of `count` area lists (the quest work's +0x69A8 handles) into spawn list
 * `index` of the move work and spawns them.  GUESS name. */
void quest_area_spawn_setup(struct Q_MoveWork* work, u32* area, u8 count, u8 index);
/* 0x803A8128 - applies the area `index`'s spawn for sub-state `sub` to the slot's move work.  GUESS name. */
void quest_area_spawn_apply(struct Q_MoveWork* work, u8 index, u8 sub);

/* 0x803A9130 - sets the item work's time limit (+0x24, in frames) and its floating copy at +0x6A60.  GUESS
 * name from those two stores. */
void quest_time_limit_set(s32 frames);

/* 0x803A8D4C - the current quest id: the option block's selected id while the local slot has a quest in progress, else
 * the item work's +0x10 key. */
u32 quest_id_get(void);

/* 0x803A9DEC - writes one 0x60-byte quest-work element's flag word (`value` is the flag mask). */
void quest_element_set(struct QuestWork* work, u16 index, u32 value);

/* 0x803AA060 - whether the item work may roll element `index` (flag bit 3 set, bit 6 clear, its +0x2D8 word's bit 6
 * set; `index` 4 counts all three).  The item work's views disagree (`QuestWork` here, `LbCompanionWork` in
 * `lobby/lb_companion_ui.h`, `Q_ItemWork` in `quest/quest_entry.h`), so the parameter is forward-declared. */
u32 quest_element_pick_ck(struct QuestWork* work, s32 index, u8 use_alt);

/* 0x803A9E68 - marks element `index` finished and records it locally or sends it to the server.  GUESS name. */
void quest_element_finish(struct QuestWork* work, struct QuestElement* element, u16 index, u32 value);

/* 0x803A7E1C - the arena item table's entry count (0 in the entry state or without an item work). */
u32 quest_arena_item_count_get(void);
/* 0x803A918C - restarts the quest's real-time clock.  GUESS name. */
void quest_clock_reset(void);
/* 0x803A91D4 - books a slain or captured monster (`mini` set for a small-monster record).  GUESS name. */
void quest_enemy_kill_dispatch(struct _ENEMY_WORK* enemy, u8 mini);
/* 0x803A9240 / 0x803A92C4 - count one more broken part `value` of monster key `id`, and set key bit `bit` of it, each
 * followed by the arena gate's re-check.  GUESS names. */
void quest_arena_need_add(u8 id, u32 unused, u8 value);
void quest_arena_key_set(u8 id, u8 bit);
/* 0x803A933C / 0x803A941C / 0x803A94F8 / 0x803A95C4 - monster `id`'s four spawn argument bytes (the first from the
 * accepted row for an id of 32 or more).  GUESS names. */
u8 quest_em_stat_tbl_get(u16 id);
u8 quest_em_stat_var_get(u16 id);
u8 quest_em_size_get(u16 id);
u8 quest_em_size_var_get(u16 id);


#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* The four 6-entry sprite/index tables `note_value_to_slot`/`note_slot_to_value` search and the flat u16 run they
 * continue into (both 0-terminated), in this unit's `.data`/`.sdata` claims. */
extern const u16* note_slot_table[4];

extern const u16 note_slot_flat_table[];

/* 0x803A86EC / 0x803A86F0 - release the current area's enemy entries (the move work is the caller's, passed through),
 * and load the quest NPC models and voice banks of (`map`, `area`) (GUESS names). */
void quest_screen_enemy_start(struct Q_MoveWork* work);
void quest_area_res_load(u8 map, u8 area);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_QUEST_SCREEN_H */
