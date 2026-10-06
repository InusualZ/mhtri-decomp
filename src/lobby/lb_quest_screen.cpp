/* lobby/lb_quest_screen.cpp - the quest/multiplayer screen band after `menu/multi_result.cpp`, headed by the note-pane
 *   helpers and ended by the quest work's tail (item-work accessors, quest clock, random source, element flags, spawn
 *   lists and kill bookkeeping).
 * RANGE. .text 0x803A3A50-0x803AA4A4 (95 functions); .data 0x805F2038-0x805F2940 (from the two private switch tables
 *   `jumptable_805F2038`/`jumptable_805F2068`), .sdata 0x80793530-0x807935D0, .sdata2 0x8079C448-0x8079C4C0, extab,
 *   extabindex.  Not one TU: a `--max-bytes` slice of five bands (note pane, lobby item/quest screen, resource loading,
 *   model/quest bookkeeping, enemy area logic; docs/lobby.md); the `.sdata2` run carries the int-to-double magic twice
 *   (0x8079C468, 0x8079C480), one pool per TU.  The unwind runs tile with `menu/multi_result.cpp`'s.
 * FLAGS. `#pragma peephole off` (the target keeps `clrlwi`+`slwi` unfused in both bands: `note_timer_ready_ck` 82.5 ->
 *   100, `quest_clock_byte_get` 92 -> 100) and `#pragma fp_contract off` (no fused multiply-add anywhere in the target;
 *   `quest_enemy_kill_record` keeps `fmuls`+`fadds`).
 * NAMES. Module `lobby` and `lb_quest_screen` are a GUESS from the biggest band (`lobby_w`, `lb_param_w`, `Screen_w`,
 *   `lobby_world_block`, `LbStr`, `lb_item_get_data`, `subTransSet`, `menu_cursor_step`); no `__FILE__` string or dump
 *   name covers the range.  Every function name is a GUESS from its body; the note-pane sub-states from the pane's
 *   state order; the quest tail's from the item-work fields they touch and their callers (`quest_em_*_get`: the four
 *   spawn-argument rows `enemy/enemy_control.cpp` turns into a level and a size).
 *   GUESS (from each body and its callers): quest_arena_item_count_get, quest_area_spawn_setup, quest_area_entry_collect,
 *   GUESS: quest_area_spawn_apply, quest_screen_enemy_start, quest_area_res_load, move_work_item_work_get,
 *   GUESS: quest_time_elapsed_get, quest_time_base_get, quest_time_limit_get, quest_id_set, quest_record_copy,
 *   GUESS: quest_id_get, quest_rand_seed_set, quest_rand_next, quest_clock_byte_get, quest_clock_step,
 *   GUESS: quest_time_limit_set, quest_clock_reset, quest_enemy_kill_dispatch, quest_arena_need_add, quest_arena_key_set,
 *   GUESS: quest_em_stat_tbl_get, quest_em_stat_var_get, quest_em_size_get, quest_em_size_var_get, quest_work_state_get,
 *   GUESS: quest_enemy_kill_record, quest_net_kill_apply, quest_element_set, quest_element_finish, quest_element_done_mark,
 *   GUESS: quest_element_pick_ck, quest_element_failed_ck, quest_element_flag20_ck, quest_element_live_ck,
 *   GUESS: quest_failed_ck, quest_sub_state_end_ck
 * RESIDUALS. 48 rows unwritten: 0x803A3A50-0x803A4170, 0x803A4214-0x803A4DD4, 0x803A4EC0-0x803A4F7C,
 *   0x803A5100-0x803A7E1C.  The quest tail 0x803A7E1C-0x803AA4A4 is written (38 rows).  Partial: `note_pane_pos_step`,
 *   `note_value_to_slot`, `note_slot_to_value`; in the tail `quest_record_copy` (the struct assignment is right - MWCC
 *   copies member by member - but `quest/quest_types.h`'s `QuestRecord` is 0x714 bytes with four `u8`s at +0x32C, a
 *   `u8[2]` member at +0x30E, a `u8[0x10]` run at +0x35C/+0x380 and a `u8[6]` run at +0x374, while the copy shows a
 *   0x4B8-byte record with one 4-byte array at +0x32C, padding at +0x30E, words at +0x35C..+0x368, halfwords at
 *   +0x374/+0x376, words at +0x37C/+0x380 and a 12-byte array at +0x384), `quest_area_spawn_apply` (the six-kind
 *   membership test: retail compares without an index register, every loop shape tried keeps one), the kill
 *   bookkeeping and spawn-list rows (register allocation; `quest_enemy_kill_record` saves one register more,
 *   `_savegpr_20` against retail's `_savegpr_21`), `note_value_to_slot`/`note_slot_to_value` (retail reaches
 *   `note_slot_flat_table` through `r13`, ours through `lis`/`addi`: the table is not yet emitted as small data), `quest_element_finish` (`lb_sub16_send`'s owner spells its
 *   flag `s8`, retail's caller narrows with `clrlwi`; the owner's own row drops to 96.4 with `u8`),
 *   `quest_net_kill_apply` (retail tests the element against 3 with two branches and keeps an empty first arm of the
 *   flag chain).  The lobby screen band 0x803A52A4-0x803A75D8 waits on a view merge: it reads `lobby_w.menu_0xAC`
 *   +0x28/+0x2A/+0x3C/+0xD0 with meanings `LbMenuWork` (`unsplit/lobby.h`) does not give them, and re-typing that header
 *   moves every includer (playbook 60); the 0x803A7718-0x803A7E1C slot group's array is at `lobby_w` +0xCC, inside
 *   `LbLobbyWork`'s +0x0C0 talk block.
 *   flipcheck: `.data` 0x28 against 0x908; `.sdata`/`.sdata2` claimed, not emitted; `.text`/extab/extabindex short of
 *   the claim; the `.sdata`/`.sdata2` pool is shared with `menu/multi_result.cpp` (fold candidate).
 * SHAPES. The note band's `NoteWork` (`enemy/note_work.h` -> `sound/mhchar.h`) and the quest tail's
 *   `quest/quest_entry.h` (-> `pl.h`) define `MHchar` and `_GXChannelID` twice, so the tail reaches its `quest_entry`
 *   callees through the leaf `quest/quest_record_find.h`.  `QuestBossSpawn` (`em_large_spawn`) and `EmAreaEntry`
 *   (`em_area_entry_tbl`) are one 0x44-byte record (`quest_area_spawn_apply` hands the first to
 *   `em_area_entry_release`).  `quest_element_pick_ck`'s `use_alt` test is a conditional expression (an `if` drops a
 *   branch).
 */

#include "types.h"
#include "nw4r/math.h"
#include "enemy/note_work.h"
#include "enemy/fn_80382310.h"
#include "ef/fn_800CDB2C.h"
#include "lobby/lb_quest_screen.h"
#include "lobby/quest_element_failed_ck.h"
#include "mh3_pad/vec3.h"
#include "mh3_pad/lb_param_w.h"
#include "mh3_pad/Screen_w.h"
#include "quest/quest_record_find.h"
#include "quest/quest_item_slot.h"
#include "ef/get_move_work_adrs.h"
#include "ef/system_core.h"
#include "ef/load_file_req.h"
#include "ef/eft052.h"
#include "NAND/nand.h"
#include "unsplit/OS.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "Network/network_pat_control.h"
#include "menu/quest_str_tbl_35_get.h"
#include "enemy/ENEMY_WORK.h"
#include "enemy/ENEMY_MINI_WORK.h"
#include "enemy/em_pop.h"
#include "enemy/em_ground_rec_clear.h"
#include "enemy/enemy_control.h"
#include "enemy/em_common.h"
#include "enemy/em_quest_element_set.h"
#include "enemy/em_prog_support.h"
#include "enemy/fn_8012BDF4.h"
#include "enemy/fn_8012EC74.h"
#include "draw_shape/qnpc_res_table.h"
#include "sound/snd_bank_loader.h"
#include "sound/fn_800F2A94.h"
#include "lobby/lb_event_request.h"
#include "lobby/lb_sub16_send.h"
#include "Pl/pl_act.h"
#include "Pl/quest_spawn_rec_find.h"
#include "pad_connect.h"
#include "camera/camera.h"
#include "camera/camera_kill_cut_start_split.h"
#include "get_FqResult_work.h"
#include "enemy/em_area_entry_tbl.h"
#include "enemy/EmAreaEntry.h"
#include "enemy/fn_8013F764.h"
#include "nw_resource.h"
#include "ef/work_mem_free.h"

/* The quest tail's own rows other units do not call (address order below). */
extern "C" {
void quest_record_copy(QuestRecord* dst, QuestRecord* src);
void quest_enemy_kill_record(Q_MoveWork* work, _ENEMY_WORK* enemy, u8 mini);
}

#pragma peephole off
#pragma fp_contract off

/* -------------------------------------------------------------------------------------------------
 * The note-pane band's own still-unwritten members: the nine tail-called sub-states
 * `note_pane_state_dispatch` selects, in the pane's state order.
 * ------------------------------------------------------------------------------------------------- */
extern "C" {
void note_pane_state_dispatch(NoteWork* self);
void note_pane_state_0(NoteWork* self);
void note_pane_state_1(NoteWork* self);
void note_pane_state_2(NoteWork* self);
void note_pane_state_3(NoteWork* self);
void note_pane_state_4(NoteWork* self);
void note_pane_state_5(NoteWork* self);
void note_pane_state_6(NoteWork* self, u32 variant);
void note_pane_state_9(NoteWork* self);
}

/* -------------------------------------------------------------------------------------------------
 * The note-pane band (0x803A3A50..0x803A52A4).
 * ------------------------------------------------------------------------------------------------- */

/* The lobby lifetime/timer record at `lobby_world_block + 0x5270` that `note_timer_*` reads: a flag, a
 * two-refresh counter and a four-entry field whose refresh loop runs 50 times per entry (the retail
 * source's own leftover nesting, which the target's codegen reproduces exactly).
 * size: 0x18 */
typedef struct NoteTimer {
    /* +0x00 */ u8 field_0x00;
    /* +0x01 */ u8 flag_0x01;
    /* +0x02 */ u8 pad_0x02[0x04 - 0x02];
    /* +0x04 */ u16 field_0x04;
    /* +0x06 */ s8 count_0x06;
    /* +0x07 */ u8 pad_0x07[0x10 - 0x07];
    /* +0x10 */ u16 field_0x10[4];
} NoteTimer;

/* 0x803A4170 - steps the pane's eased position towards its target (4004 units while its animation pair is (0,4), else
 * 1820), snapping on within one step. */
extern "C" void note_pane_pos_step(NoteWork* self) {
    u32 step = (note_pane_anim_pair_ck(self, 0, 4) != 0) ? 4004 : 1820;
    u32 target = self->field_0x1A8;
    u32 cur = (u16)self->field_0x18C;
    u16 avail = (u16)(target - cur);
    if ((u16)(avail + step) < (u32)(step * 2)) {
        self->field_0x18C = target;
    } else if (avail < 0x8000) {
        self->field_0x18C = (u16)(cur + step);
    } else {
        self->field_0x18C = (u16)(cur - step);
    }
}

/* 0x803A4208 - puts the pane on its animation pair (0,1). */
extern "C" void note_pane_anim_pair_0_1(NoteWork* self) {
    note_pane_set_anim_pair(self, 0, 1);
}

/* 0x803A4E30 - the pane's null sub-state. */
extern "C" void note_pane_idle(NoteWork* self) {
}

/* 0x803A4E34 - runs the pane sub-state `+0x19D` selects: 0 dispatches on the pane's state byte,
 * 1 is the null one. */
extern "C" void note_pane_dispatch(NoteWork* self) {
    switch (self->field_0x19D) {
    case 0: note_pane_state_dispatch(self); break;
    case 1: note_pane_idle(self); break;
    }
}

/* 0x803A4DD4 - the note pane's per-state dispatcher: `NoteWork::field_0x19F` selects one of ten
 * tail-called sub-state bodies. */
extern "C" void note_pane_state_dispatch(NoteWork* self) {
    switch (self->field_0x19F) {
    case 0: note_pane_state_0(self); break;
    case 1: note_pane_state_1(self); break;
    case 2: note_pane_state_2(self); break;
    case 3: note_pane_state_3(self); break;
    case 4: note_pane_state_4(self); break;
    case 5: note_pane_state_5(self); break;
    case 6: note_pane_state_6(self, 0); break;
    case 7: note_pane_state_6(self, 1); break;
    case 8: note_pane_state_6(self, 2); break;
    case 9: note_pane_state_9(self); break;
    }
}

/* 0x803A4E58 - reads the quest NPC's motion number through `qn_get_motion_no` (the record is the pane's 0x1F8-byte
 * `NoteWork`; the vector local is retail's `VEC3 v; VEC3_ctor(&v);` idiom). */
extern "C" void note_pane_get_motion(NoteWork* self) {
    nw4r::math::VEC3 v;
    VEC3_ctor(&v);
    qn_get_motion_no((_QNPC_W*)self);
}

/* 0x803A4E90 - true while the lifetime record still has a live countdown and a set flag. */
extern "C" s32 note_timer_ready_ck(NoteTimer* timer) {
    if ((s8)timer->count_0x06 <= 0) {
        if ((s8)timer->flag_0x01 != 0) {
            return 1;
        }
    }
    return 0;
}

/* 0x803A4F7C - the inverse of `note_slot_to_value`: the flat slot index holding `value` in `note_slot_table`'s four
 * 6-entry tables, then `note_slot_flat_table`; 255 when absent. */
extern "C" u32 note_value_to_slot(u16 value) {
    u32 index = 0;
    s8 t;
    for (t = 0; t < 4; t++) {
        const u16* p = note_slot_table[t];
        u32 j;
        for (j = 0; j < 6; j++) {
            if (value == p[j]) {
                return index;
            }
            index++;
        }
    }
    {
        const u16* q = note_slot_flat_table;
        while (*q != 0) {
            if (value == *q) {
                return index;
            }
            index++;
            q++;
        }
    }
    return 255;
}

/* 0x803A5070 - maps a flat slot index to its table value over the same four 6-entry tables and the
 * flat run at `note_slot_flat_table`; 0 past the end of both. */
extern "C" u16 note_slot_to_value(u8 slot) {
    u32 index = 0;
    u16 t;
    for (t = 0; t < 4; t++) {
        const u16* p = note_slot_table[t];
        u32 j;
        for (j = 0; j < 6; j++) {
            if (slot == (u8)index) {
                return p[j];
            }
            index++;
        }
    }
    {
        const u16* q = note_slot_flat_table;
        while (*q != 0) {
            if (slot == (u8)index) {
                return *q;
            }
            index++;
            q++;
        }
    }
    return 0;
}

/* -------------------------------------------------------------------------------------------------
 * The quest tail (0x803A7E1C..0x803AA4A4): the item-work accessors, the quest clock and random source,
 * the element flags and the kill/capture bookkeeping.
 * ------------------------------------------------------------------------------------------------- */

/* 0x803A7E1C (0x4C): the arena item table's entry count (0 in the entry state or without an item work). */
extern "C" u32 quest_arena_item_count_get(void) {
    Q_ItemWork* item;

    if (move_work_state_ck() != 0) {
        return 0;
    }
    item = move_work_item_work_get();
    if (item == NULL) {
        return 0;
    }
    return item->count_0x6A2A;
}

/* 0x803A7E68 (0x210): files the monster slots of `count` area lists into spawn list `index` of the move work
 * (continuing the previous list's spawn order), spawns each, and keeps the area's main monster in `main_0x2258`. */
extern "C" void quest_area_spawn_setup(Q_MoveWork* work, u32* area, u8 count, u8 index) {
    EmGroundRec ground;
    QuestSpawnRec* main;
    u32* num;
    QuestSpawnRec* rec;
    s32 i;
    QuestEntrySlot* slot;
    u32 order;
    QuestBossSpawn* boss;
    s32 element;

    em_ground_rec_clear(&ground);
    if (work->main_0x2258 == NULL) {
        work->main_0x2258 = (QuestSpawnRec*)work_mem_alloc(16);
        memset(work->main_0x2258, 0, 16);
    }
    main = work->main_0x2258;
    if (index == 0) {
        num = &work->area_counts_0x2154[index];
        *num = 0;
    } else {
        num = &work->area_counts_0x2154[index];
        *num = work->area_counts_0x2154[index - 1];
    }
    rec = work->area_recs_0x154[index];
    for (i = 0; i < 6; i++) {
        work->res_a_0x2194[index][i] = -1;
        work->res_b_0x21F4[index][i] = -1;
    }
    for (i = 0; i < count; i++) {
        slot = (QuestEntrySlot*)*area;
        if (slot != NULL) {
            for (; slot->monster_0x00 != 0; slot++) {
                if (slot->element_index_0x28 == -1) {
                    order = *num + 32;
                    em_ground_rec_set(&ground, &slot->element_0x08);
                    boss = em_large_spawn(slot->monster_0x00, &ground, order);
                    if (boss == NULL) {
                        return;
                    }
                    memset(rec, 0, 16);
                    rec->order_0x0 = order;
                    rec->slot_0x8 = slot;
                    rec->count_0x4 = slot->count_0x04;
                    rec->monster_0x6 = slot->monster_0x00;
                    rec->boss_0xC = boss;
                    if (slot->element_0x08.byte_0x02 == 0) {
                        main->order_0x0 = order;
                        main->slot_0x8 = slot;
                        main->count_0x4 = slot->count_0x04;
                        main->monster_0x6 = slot->monster_0x00;
                        main->boss_0xC = boss;
                    }
                    element = quest_element_find(slot->monster_0x00);
                    if (element >= 0) {
                        em_quest_element_set_large((_ENEMY_WORK*)boss, element);
                    }
                    rec++;
                    (*num)++;
                }
            }
        }
        area++;
    }
}

/* 0x803A8078 (0xB0): respawns every large monster of the first spawn list whose count has run out and collects
 * the new spawns into `out`; returns how many it collected. */
extern "C" s32 quest_area_entry_collect(Q_MoveWork* work, QuestBossSpawn** out) {
    EmGroundRec ground;
    s32 count;
    QuestSpawnRec* rec;
    s32 n;
    s32 i;
    QuestBossSpawn* boss;

    em_ground_rec_clear(&ground);
    count = work->area_counts_0x2154[0];
    rec = work->area_recs_0x154[0];
    n = 0;
    for (i = 0; i < count; i++, rec++) {
        if (rec->count_0x4 == 0) {
            em_ground_rec_set(&ground, &rec->slot_0x8->element_0x08);
            boss = em_large_spawn(rec->slot_0x8->monster_0x00, &ground, i + 32);
            if (boss != NULL) {
                *out = boss;
                out++;
                n++;
            }
        }
    }
    return n;
}

/* 0x803A8128 (0x5C4): reworks the move work's six resident monster kinds for area list `index`/area `sub`: keeps
 * the kinds the live area entries near the area and the live small monsters still need, releases the others (their
 * resources, control slot and SE slot), loads the new ones, and respawns the large monsters whose count ran out. */
extern "C" void quest_area_spawn_apply(Q_MoveWork* work, u8 index, u8 sub) {
    s8 keep[6];
    s32 need[6];
    s32 add[6];
    EmAreaEntry* entry = &em_area_entry_tbl[0][0];
    s32 i;
    s32 j;
    s32 n;
    QuestBossSpawn** list;
    QuestSpawnRec* rec;
    QuestEntrySlot* slot;
    _ENEMY_WORK* enemy;
    u16 max;
    u8 slot_index;

    for (i = 0; i < 6; i++) {
        add[i] = 0;
        need[i] = 0;
        work->kinds_prev_0x2170[i] = work->kinds_0x2164[i];
        if (work->kinds_prev_0x2170[i] != 0) {
            keep[i] = 0;
        } else {
            keep[i] = -1;
        }
    }
    n = 0;
    list = (QuestBossSpawn**)work_mem_alloc(0x200);
    if (list != NULL) {
        n = quest_area_entry_collect(work, list);
    }
    for (i = 0; i < 128; entry++, i++) {
        if (entry->active == 0 || entry->order_0x16 < 32) {
            continue;
        }
        rec = quest_spawn_rec_find(entry->order_0x16);
        if (rec == NULL) {
            continue;
        }
        slot = rec->slot_0x8;
        if (slot->element_0x08.byte_0x02 == 0) {
            if (work->main_0x2258 == NULL) {
                work->main_0x2258 = (QuestSpawnRec*)work_mem_alloc(16);
                memset(work->main_0x2258, 0, 16);
            }
        } else if (em_area_entry_near_ck(entry, index, sub) == 0) {
            continue;
        }
        for (j = 0; j < 6; j++) {
            if (slot->monster_0x00 == need[j]) {
                break;
            }
        }
        if (j != 6) {
            continue;
        }
        for (j = 0; j < 6; j++) {
            if (need[j] == 0) {
                break;
            }
        }
        if (j >= 6) {
            break;
        }
        need[j] = slot->monster_0x00;
    }
    enemy = (_ENEMY_WORK*)get_move_work_adrs(3);
    for (max = get_move_work_max(3); max > 0; max--, enemy++) {
        if (enemy->active == 0 || (enemy->field_0x1C8 & 1) != 0 || enemy->field_0x00C != 0) {
            continue;
        }
        for (j = 0; j < 6; j++) {
            if (enemy->team == need[j]) {
                break;
            }
        }
        if (j != 6) {
            continue;
        }
        for (j = 0; j < 6; j++) {
            if (need[j] == 0) {
                break;
            }
        }
        if (j >= 6) {
            break;
        }
        need[j] = enemy->team;
    }
    for (i = 0; i < 6; i++) {
        if (need[i] != 0) {
            for (j = 0; j < 6; j++) {
                if (work->kinds_prev_0x2170[j] == need[i]) {
                    keep[j] = 1;
                    break;
                }
            }
            if (j >= 6) {
                add[i] = need[i];
            }
        }
    }
    for (i = 0; i < 6; i++) {
        if (keep[i] == 0) {
            if (work->res_a_0x2194[0][i] != -1) {
                nw_res_entry_clear(work->res_a_0x2194[0][i]);
                work->res_a_0x2194[0][i] = -1;
            }
            if (work->res_b_0x21F4[0][i] != -1) {
                nw_res_entry_clear(work->res_b_0x21F4[0][i]);
                work->res_b_0x21F4[0][i] = -1;
            }
            slot_index = em_kind_slot_find((u8)work->kinds_0x2164[i]);
            if (slot_index != 0xFF) {
                em_kind_slot_release(slot_index);
            }
            snd_em_se_slot_release(work->kinds_0x2164[i]);
            work->kinds_0x2164[i] = 0;
        }
    }
    for (i = 0; i < 6; i++) {
        if (add[i] != 0) {
            for (j = 0; j < 6; j++) {
                if (work->kinds_0x2164[j] == 0) {
                    break;
                }
            }
            if (j < 6) {
                work->kinds_0x2164[j] = add[i];
                em_kind_release(work->kinds_0x2164[j]);
            }
        }
    }
    if (list != NULL) {
        for (i = 0; i < n; i++) {
            em_area_entry_release((EmAreaEntry*)list[i]);
        }
        work_mem_free(list);
    }
}

/* 0x803A86EC (0x4): releases the enemy area entries of the current area. */
extern "C" void quest_screen_enemy_start(Q_MoveWork* work) {
    em_area_entries_release();
}

/* 0x803A86F0 (0xB8): when the quest NPC of (`kind`, `sub`) needs its models, reads every `qnpc_res_table` file
 * back to back into a free enemy resource buffer and loads the NPC's voice banks. */
extern "C" void quest_area_res_load(u8 map, u8 area) {
    s32 i = 0;
    u8* buf;
    QnpcResEntry* entry;
    u32 ctx;

    if (qnpc_load_ck(map, area) != 0 && (buf = em_res_buffer_get()) != NULL) {
        for (entry = qnpc_res_table; entry->size != 0; entry++, i++) {
            ctx = (u8)i;
            load_file_req(entry->path, (u32)buf, entry->size, (u32)qnpc_res_load_done, 1, &ctx);
            buf += entry->size;
        }
        snd_npc_voice_bank_load(map, area);
    }
}

/* 0x803A87A8 (0x38): the item work slot 0's move work points at, NULL without a move work. */
extern "C" Q_ItemWork* move_work_item_work_get(void) {
    Q_MoveWork* work = (Q_MoveWork*)get_move_work_adrs(0);
    if (work == NULL) {
        return NULL;
    }
    return work->item_work;
}

/* 0x803A87E0 (0x3C): the time left (+0x24), clamped up to 0. */
extern "C" s32 quest_time_elapsed_get(void) {
    Q_ItemWork* item = move_work_item_work_get();
    if (item == NULL) {
        return 0;
    }
    return item->time_limit_0x24 < 0 ? 0 : item->time_limit_0x24;
}

/* 0x803A881C (0x3C): the time the screen counts down from (+0x20), clamped up to 0. */
extern "C" s32 quest_time_base_get(void) {
    Q_ItemWork* item = move_work_item_work_get();
    if (item == NULL) {
        return 0;
    }
    return item->time_base_0x20 < 0 ? 0 : item->time_base_0x20;
}

/* 0x803A8858 (0x34): the quest's whole time limit (+0x1C). */
extern "C" s32 quest_time_limit_get(void) {
    Q_ItemWork* item = move_work_item_work_get();
    if (item == NULL) {
        return 0;
    }
    return item->time_total_0x1C;
}

/* 0x803A888C (0x108): accepts quest `id`: copies its record into the item work's row and takes the quest id, the
 * slot byte, the time limit (minutes, scaled to frames) and the row's three words from it. */
extern "C" void quest_id_set(s32 id) {
    Q_ItemWork* item = move_work_item_work_get();
    QuestRecord* rec;

    if (item != NULL && item->record_0x3C != NULL) {
        rec = quest_record_find((u16)id);
        quest_record_copy((QuestRecord*)item->record_0x3C, rec);
        item->quest_id_0x10 = rec->field_0x02C;
        item->field_0x14 = rec->slot_bytes_0x314[0].value;
        item->time_total_0x1C = rec->field_0x13A * (60.0f * Screen_w.frame_scale);
        item->time_limit_0x24 = rec->field_0x13A * (60.0f * Screen_w.frame_scale);
        item->rec_value_0x30 = rec->field_0x34C;
        item->rec_value_0x34 = rec->field_0x358;
        item->rec_flags_0x38 = rec->flags_0x310;
    }
}

/* 0x803A8994 (0x3B8): copies a whole quest result record. */
extern "C" void quest_record_copy(QuestRecord* dst, QuestRecord* src) {
    *dst = *src;
}

/* 0x803A8D4C (0x58): the current quest id: the lobby's selected id while the slot is in its entry state, else
 * the item work's own. */
extern "C" u32 quest_id_get(void) {
    Q_ItemWork* item = move_work_item_work_get();
    if (item == NULL) {
        return 0;
    }
    if (move_work_state_ck() != 0) {
        return lb_param_w.field_0x00;
    }
    return item->quest_id_0x10;
}

/* 0x803A8DA4 (0x140): seeds the random source with the sum of the clock snapshot's seven (low, high << 8)
 * pairs, 451 when that sum is 0. */
extern "C" void quest_rand_seed_set(void) {
    Q_ItemWork* item = move_work_item_work_get();
    if (item != NULL) {
        item->rand_state_0x5A = 0;
        item->rand_state_0x5A += (u16)(item->rand_words_0x6C[0] + (u16)(item->rand_words_0x6C[1] << 8));
        item->rand_state_0x5A += (u16)(item->rand_words_0x6C[2] + (u16)(item->rand_words_0x6C[3] << 8));
        item->rand_state_0x5A += (u16)(item->rand_words_0x6C[4] + (u16)(item->rand_words_0x6C[5] << 8));
        item->rand_state_0x5A += (u16)(item->rand_words_0x6C[6] + (u16)(item->rand_words_0x6C[7] << 8));
        item->rand_state_0x5A += (u16)(item->rand_words_0x6C[8] + (u16)(item->rand_words_0x6C[9] << 8));
        item->rand_state_0x5A += (u16)(item->rand_words_0x6C[10] + (u16)(item->rand_words_0x6C[11] << 8));
        item->rand_state_0x5A += (u16)(item->rand_words_0x6C[12] + (u16)(item->rand_words_0x6C[13] << 8));
        if (item->rand_state_0x5A == 0) {
            item->rand_state_0x5A = 451;
        }
    }
}

/* 0x803A8EE4 (0x7C): steps the random source (x176 mod 65363, a 0 state taken as 1) and returns it. */
extern "C" u32 quest_rand_next(void) {
    Q_ItemWork* item = move_work_item_work_get();
    u32 state = item->rand_state_0x5A;
    if (item == NULL) {
        return 0;
    }
    if (state == 0) {
        state = 1;
    }
    return item->rand_state_0x5A = (state * 176) % 65363;
}

/* 0x803A8F60 (0x50): the low byte of clock snapshot word `index`. */
extern "C" u8 quest_clock_byte_get(u8 index) {
    Q_ItemWork* item = move_work_item_work_get();
    if (item == NULL) {
        return 0;
    }
    return item->rand_words_0x6C[index];
}

/* 0x803A8FB0 (0x180): counts the time left down by the real time since the last step (at least one frame, none
 * with `mode` 0) once less than 99 minutes are left; with no previous reading `mode` 1 takes one frame. */
extern "C" void quest_clock_step(s32 mode) {
    Q_ItemWork* item = move_work_item_work_get();
    f64 frames;

    if (item == NULL) {
        return;
    }
    if (item->time_limit_0x24 >= 99.0f * (60.0f * Screen_w.frame_scale)) {
        return;
    }
    item->timer_0x6A50 = item->timer_0x6A48;
    item->timer_0x6A48 = OSGetTime();
    if (item->timer_0x6A50 > 0.0) {
        frames = 8000.0 * (item->timer_0x6A48 - item->timer_0x6A50) / (OS_BUS_CLOCK / 4 / 125000) / 1000.0 /
                 item->timer_rate_0x6A58;
        if (frames < 1.0) {
            frames = 1.0;
        }
        if (mode == 0) {
            frames = 0.0;
        }
        item->time_left_0x6A60 -= frames;
    } else if (mode == 1) {
        item->time_left_0x6A60 -= 1.0;
    }
    if (item->time_limit_0x24 > 0) {
        item->time_limit_0x24 = item->time_left_0x6A60;
        if (item->time_limit_0x24 < 0) {
            item->time_limit_0x24 = 0;
        }
    }
}

/* 0x803A9130 (0x5C): sets the time left (in frames) and its floating copy. */
extern "C" void quest_time_limit_set(s32 frames) {
    Q_ItemWork* item = move_work_item_work_get();
    if (item != NULL) {
        item->time_limit_0x24 = frames;
        item->time_left_0x6A60 = item->time_limit_0x24;
    }
}

/* 0x803A918C (0x48): restarts the real-time clock: both readings take the current time. */
extern "C" void quest_clock_reset(void) {
    Q_ItemWork* item = move_work_item_work_get();
    if (item != NULL) {
        item->timer_0x6A48 = OSGetTime();
        item->timer_0x6A50 = item->timer_0x6A48;
    }
}

/* 0x803A91D4 (0x6C): books a kill: the entry-state slot's record or the quest's. */
extern "C" void quest_enemy_kill_dispatch(_ENEMY_WORK* enemy, u8 from_net) {
    Q_MoveWork* work = (Q_MoveWork*)get_move_work_adrs(0);
    if (work != NULL) {
        if (work->state_0x113 == 1) {
            em_set_kill_record(enemy, from_net);
        } else {
            quest_enemy_kill_record(work, enemy, from_net);
        }
    }
}

/* 0x803A9240 (0x84): counts one more arena item `value` of element `id` (values 0..7) and lets the arena gate
 * re-check it; nothing in the entry state. */
extern "C" void quest_arena_need_add(u8 id, u32 unused, u8 value) {
    QuestWork* work;

    if (move_work_state_ck() != 1 && (work = (QuestWork*)move_work_item_work_get()) != NULL && value <= 7) {
        work->part_counts_0x6803[id][value]++;
        quest_arena_value_clear_ck(id, value);
    }
}

/* 0x803A92C4 (0x78): sets key bit `bit` of arena element `id` and lets the arena gate re-check it. */
extern "C" void quest_arena_key_set(u8 id, u8 bit) {
    QuestWork* work;

    if (move_work_state_ck() != 1 && (work = (QuestWork*)move_work_item_work_get()) != NULL) {
        work->key_bits_0x694C[id] |= 1 << bit;
        quest_arena_key_clear(id);
    }
}

/* 0x803A933C (0xE0): monster `id`'s first parameter: the accepted row's for an id of 32 or more, else the item
 * work's row entry for the id (0 when the id has no row). */
extern "C" u8 quest_em_stat_tbl_get(u16 id) {
    Q_ItemWork* item;
    s32 i;

    if (move_work_state_ck() != 1 && (item = move_work_item_work_get()) != NULL && item->record_0x3C != NULL) {
        if (id >= 32) {
            return item->record_0x3C->em_stat_0x380;
        }
        for (i = 0; i < 6; i++) {
            if (id == item->slot_key_0x46C[i]) {
                break;
            }
        }
        if (i != 6) {
            return item->spawn_args_0x478[0][i];
        }
    }
    return 0;
}

/* 0x803A941C (0xDC): monster `id`'s second parameter row entry (0 for an id of 32 or more or without a row). */
extern "C" u8 quest_em_stat_var_get(u16 id) {
    Q_ItemWork* item;
    s32 i;

    if (move_work_state_ck() != 1 && (item = move_work_item_work_get()) != NULL && item->record_0x3C != NULL) {
        if (id >= 32) {
            return 0;
        }
        for (i = 0; i < 6; i++) {
            if (id == item->slot_key_0x46C[i]) {
                break;
            }
        }
        if (i != 6) {
            return item->spawn_args_0x478[1][i];
        }
    }
    return 0;
}

/* 0x803A94F8 (0xCC): monster `id`'s third parameter row entry (0 without a row). */
extern "C" u8 quest_em_size_get(u16 id) {
    Q_ItemWork* item;
    s32 i;

    if (move_work_state_ck() != 1 && (item = move_work_item_work_get()) != NULL && item->record_0x3C != NULL) {
        for (i = 0; i < 6; i++) {
            if (id == item->slot_key_0x46C[i]) {
                break;
            }
        }
        if (i != 6) {
            return item->spawn_args_0x478[2][i];
        }
    }
    return 0;
}

/* 0x803A95C4 (0xCC): monster `id`'s fourth parameter row entry (0 without a row). */
extern "C" u8 quest_em_size_var_get(u16 id) {
    Q_ItemWork* item;
    s32 i;

    if (move_work_state_ck() != 1 && (item = move_work_item_work_get()) != NULL && item->record_0x3C != NULL) {
        for (i = 0; i < 6; i++) {
            if (id == item->slot_key_0x46C[i]) {
                break;
            }
        }
        if (i != 6) {
            return item->spawn_args_0x478[3][i];
        }
    }
    return 0;
}

/* 0x803A9690 (0x34): the item work's state word. */
extern "C" s32 quest_work_state_get(void) {
    Q_ItemWork* item = move_work_item_work_get();
    if (item == NULL) {
        return 0;
    }
    return item->state_0x6AA0;
}

/* Runs the kill cut-in on `enemy` (the split-screen form while the screen is split, else only inside its area). */
static inline void quest_kill_cut_start(_ENEMY_WORK* enemy) {
    if (screen_split_mode_ck() != 0) {
        camera_kill_cut_start_split(1, enemy);
    } else if (em_area_ck(enemy) == 1) {
        camera_kill_cut_start(1, enemy);
    }
}

/* 0x803A96C4 (0x4B8): books one slain or captured monster (a small-monster record when `from_net` is set): the
 * per-kind counts and sizes, its spawn entry's count, the tracked-kind kills or the quest element's progress, and
 * the kill cut-in unless that finished the quest. */
extern "C" void quest_enemy_kill_record(Q_MoveWork* work, _ENEMY_WORK* enemy, u8 from_net) {
    _ENEMY_WORK* target = NULL;
    Q_ItemWork* item = move_work_item_work_get();
    s32 done;
    s32 size;
    u32 big;
    u16 key;
    u8 element;
    u8 kind;
    u8 how;
    u8 area;
    QuestSpawnRec* entry;
    s32 i;
    s32 need;
    s8 first;

    if (item == NULL) {
        return;
    }
    done = 0;
    size = 0;
    big = 0;
    if (from_net == 0) {
        target = enemy;
        key = enemy->field_0x01A;
        element = enemy->field_0x00D;
        kind = enemy->team;
        how = em_captured_ck(enemy);
        em_record_hit_ck(enemy);
        if ((enemy->field_0x1C8 & 1) != 0) {
            big = 1;
            size = 100.0f * get_em_chg_scale(enemy) + 0.5f;
        }
        area = enemy->area_slot_0x013;
    } else {
        _ENEMY_MINI_WORK* mini = (_ENEMY_MINI_WORK*)enemy;
        key = mini->order_0x16;
        element = mini->element_0x0D;
        kind = mini->monster_0x02;
        how = em_mini_kill_kind_get(enemy);
        em_mini_hit_ck(enemy);
        area = mini->area_slot_0x1D;
    }
    if (how == 0) {
        item->set_c.count[kind]++;
        quest_element_item_apply(1, kind, 1);
    }
    if (how == 1) {
        item->set_d.count[kind]++;
    }
    if (big == 1) {
        if (item->size_0x534[kind].min == 0) {
            item->size_0x534[kind].min = size;
        } else if (item->size_0x534[kind].min > (u16)size) {
            item->size_0x534[kind].min = size;
        }
        if (item->size_0x534[kind].max == 0) {
            item->size_0x534[kind].max = size;
        } else if (item->size_0x534[kind].max < (u16)size) {
            item->size_0x534[kind].max = size;
        }
    }
    entry = quest_spawn_rec_find_in(key, area);
    if (entry != NULL && entry->count_0x4 > 0) {
        entry->count_0x4--;
    }
    if (element > 2) {
        if (big == 1 && Pl_motion_input_ck(0) == 0) {
            for (i = 0; i < 3; i++) {
                if (kind == work->spawn_0x2274[i + 3].monster_0x6) {
                    ((QuestWork*)item)->slot_kills_0x6980[i]++;
                    break;
                }
            }
        }
        return;
    }
    if (quest_flag_4000000_ck(NULL) == 1) {
        item->hunt_end_0x6977 = 0;
        element = (s8)(2 - element);
    }
    if (quest_flag_80000000_ck(NULL) == 1) {
        item->hunt_end_0x6977 = 0;
    }
    if (quest_element_progress_step(kind, how, &first) != 1) {
        return;
    }
    if (quest_flag_8_ck(NULL) == 1) {
        return;
    }
    if (quest_flag_80000_ck(NULL) == 0) {
        if (quest_flag_4000000_ck(NULL) == 1) {
            if ((u8)(element - 1) <= 1) {
                done = 1;
            }
        } else if (quest_flag_2000000_ck(NULL) == 1 || quest_flag_80000000_ck(NULL) == 1) {
            if ((item->elements_0x94[0].flags & 9) == 9 && (item->elements_0x94[1].flags & 9) == 9) {
                if ((s8)element == 0) {
                    if (quest_element_pick_ck((QuestWork*)item, 1, 0) == 0) {
                        done = 1;
                    }
                } else if ((s8)element == 1) {
                    if (quest_element_pick_ck((QuestWork*)item, 0, 0) == 0) {
                        done = 1;
                    }
                }
            } else if ((item->elements_0x94[0].flags & 9) == 9 && item->elements_0x94[0].value != 0) {
                done = 1;
            }
        } else if (quest_flag_40000_ck(NULL) == 1) {
            for (i = 0; i < 3; i++) {
                if (quest_element_live_ck((QuestWork*)item, i) != 0 &&
                    quest_element_pick_ck((QuestWork*)item, i, 0) == 0) {
                    done = 1;
                    break;
                }
            }
        } else if ((s8)element != 0 || first != 0) {
            done = 1;
        }
        if (target != NULL && done == 0) {
            quest_kill_cut_start(target);
        }
    } else {
        done = 0;
        for (i = 0; i < 3; i++) {
            quest_arena_count_get(i);
            need = quest_arena_need_get(i);
            if (need >= 0) {
                done += need;
            }
        }
        if (done == 0) {
            if (target != NULL) {
                quest_kill_cut_start(target);
            }
        } else {
            snd_hunt_stream_start(0);
        }
    }
}

/* 0x803A9B7C (0x270): applies a kill another player reported: takes `count` off the spawn entry under (`key`,
 * `area`), books the taken ones as kills (`how` 0) or captures (1) of its kind, and steps the quest element. */
extern "C" void quest_net_kill_apply(u8 area, u16 key, u16 count, s32 how, u8 element) {
    Q_MoveWork* work = (Q_MoveWork*)get_move_work_adrs(0);
    Q_ItemWork* item;
    s32 taken;
    u8 kind;
    QuestSpawnRec* entry;
    s32 i;
    s8 first;

    if (work != NULL && work->state_0x113 != 1 && (item = move_work_item_work_get()) != NULL) {
        taken = 0;
        kind = 0;
        entry = quest_spawn_rec_find_in(key, area);
        if (entry != NULL) {
            kind = entry->monster_0x6;
            if (entry->count_0x4 >= count) {
                taken = entry->count_0x4 - count + 1;
                entry->count_0x4 -= (s16)taken;
            }
        }
        if (kind != 0 && taken != 0) {
            if ((u8)how == 0) {
                item->set_c.count[kind] += (u16)taken;
                quest_element_item_apply(1, kind, taken);
                userdata_hunt_count_add(kind, (u16)taken);
            }
            if ((u8)how == 1) {
                item->set_d.count[kind] += (u16)taken;
                userdata_capture_count_add(kind, (u16)taken);
            }
            if (element <= 3 && element != 3) {
                if (quest_flag_4000000_ck(NULL) == 1) {
                    item->hunt_end_0x6977 = 0;
                    element = 2 - element;
                }
                if (quest_flag_80000000_ck(NULL) == 1) {
                    item->hunt_end_0x6977 = 0;
                }
                if (quest_element_progress_step(kind, how, &first) == 1 && quest_flag_8_ck(NULL) != 1 &&
                    quest_flag_80000_ck(NULL) == 0) {
                    if (quest_flag_4000000_ck(NULL) == 1) {
                    } else if (quest_flag_2000000_ck(NULL) == 1 || quest_flag_80000000_ck(NULL) == 1) {
                        if (element == 0) {
                            quest_element_pick_ck((QuestWork*)item, 1, 0);
                        } else if (element == 1) {
                            quest_element_pick_ck((QuestWork*)item, 0, 0);
                        }
                    } else if (quest_flag_40000_ck(NULL) == 1) {
                        for (i = 0; i < 3; i++) {
                            if (quest_element_live_ck((QuestWork*)item, i) != 0 &&
                                quest_element_pick_ck((QuestWork*)item, i, 0) == 0) {
                                break;
                            }
                        }
                    }
                }
            }
        }
    }
}

/* 0x803A9DEC (0x7C): finishes element `index` with flag `value` unless the slot's sub-state has ended. */
extern "C" void quest_element_set(QuestWork* work, u16 index, u32 value) {
    QuestElement* element = &work->elements_0x0094[index];
    if (quest_sub_state_end_ck(1) == 0) {
        quest_element_finish(work, element, index, value);
    }
}

/* 0x803A9E68 (0xC0): marks element `index` finished (flag 0x20) unless it already counts as picked, announces
 * the first element's special clear, and records it locally or sends it to the server. */
extern "C" void quest_element_finish(QuestWork* work, QuestElement* element, u16 index, u32 value) {
    if (quest_element_pick_ck(work, index, 0) != 1) {
        element->flags |= 0x20;
        if (index == 0 && quest_flag_1000000_ck(NULL) == 1) {
            lb_event_request(13);
        }
        if (isServerSelectState() == 0) {
            quest_element_done_mark(work, element, index, value);
        } else {
            lb_sub16_send(32, index);
        }
    }
}

/* 0x803A9F28 (0x138): files `index` in the first free finish-order slot, marks its flag word and, for an element
 * past the first that the quest's rules do not rule out, announces it and copies its reward cell. */
extern "C" void quest_element_done_mark(QuestWork* work, QuestElement* element, u16 index, u32 flag) {
    s32 i;
    u8 result;

    for (i = 0; i < 3; i++) {
        if (work->slot_ids_0x309[i] == 0xFF) {
            work->slot_ids_0x309[i] = index;
            work->counters_0x2D8[index] |= 0x20;
            if (index != 0 && (element->flags & 0x10) == 0 && quest_flag_4000000_ck(NULL) == 0) {
                result = quest_objective_result_get(NULL);
                if (result != 7 && result != 9 && quest_flag_80000_ck(NULL) == 0 && quest_flag_2000000_ck(NULL) == 0) {
                    hud_msg_push(0, quest_str_tbl_35_get(index + 23));
                    snd_hunt_stream_start(0);
                    hud_msg_push(1, quest_str_tbl_35_get(index + 33));
                    quest_item_pair_copy_cell(work->supply_0x5E2, quest_field372_get(NULL), index - 1);
                }
            }
            break;
        }
    }
}

/* 0x803AA060 (0x158): whether element `index` counts as picked: active (bit 3), not failed (bit 6) and finished
 * (bit 5) in its flag word or, with `use_alt`, in its +0x2D8 word; `index` 4 asks whether all three are (an empty
 * element counts). */
extern "C" u32 quest_element_pick_ck(QuestWork* work, s32 index, u8 use_alt) {
    s32 count;
    s32 i;
    u32 flags;
    u32 alt;

    if (index == 4) {
        count = 0;
        for (i = 0; i < 3; i++) {
            flags = work->elements_0x0094[i].flags;
            if (use_alt == 0) {
                alt = flags;
            } else {
                alt = work->counters_0x2D8[i];
            }
            if (flags == 0) {
                count++;
            } else if ((flags & 8) != 0 && (alt & 0x20) != 0) {
                count++;
            }
        }
        return count >= 3;
    }
    flags = work->elements_0x0094[index].flags;
    if ((flags & 8) != 0) {
        if ((flags & 0x40) == 0x40) {
            return 0;
        }
        flags = (use_alt == 0) ? flags : work->counters_0x2D8[index];
        if ((flags & 0x20) != 0) {
            return 1;
        }
    }
    return 0;
}

/* 0x803AA1B8 (0xE8): whether element `index` has failed (active with bit 6); `index` 4 asks whether all three have
 * (an empty element counts). */
extern "C" u32 quest_element_failed_ck(QuestWork* work, s32 index) {
    s32 count;
    s32 i;
    u32 flags;

    if (index == 4) {
        count = 0;
        for (i = 0; i < 3; i++) {
            flags = work->elements_0x0094[i].flags;
            if (flags == 0) {
                count++;
            } else if ((flags & 8) != 0 && (flags & 0x40) != 0) {
                count++;
            }
        }
        return count >= 3;
    }
    flags = work->elements_0x0094[index].flags;
    if ((flags & 8) != 0) {
        return (flags & 0x40) == 0x40;
    }
    return 0;
}

/* 0x803AA2A0 (0x20): whether element `index` is finished (flag bit 5). */
extern "C" u32 quest_element_flag20_ck(QuestWork* work, s32 index) {
    return (work->elements_0x0094[index].flags & 0x20) != 0;
}

/* 0x803AA2C0 (0x30): whether element `index` is set up and active (flag bit 3). */
extern "C" s32 quest_element_live_ck(QuestWork* work, s32 index) {
    u32 flags = work->elements_0x0094[index].flags;
    if (flags == 0) {
        return 0;
    }
    return (flags & 8) != 0;
}

/* 0x803AA2F0 (0x12C): whether one of the elements the quest grade selects has failed, for a row whose +0x310 flags
 * carry exactly one of bits 0 and 2. */
extern "C" u32 quest_failed_ck(QuestWork* work) {
    QuestRecord* record = work->record_0x03C;
    u32 kind;
    u32 mask;
    s32 i;
    u32 flags;

    if (record == NULL) {
        return 0;
    }
    kind = record->flags_0x310 & 5;
    if (kind == 0) {
        return 0;
    }
    if (kind == 5) {
        return 0;
    }
    switch (work->grade_0x93) {
    case 3:
        mask = 3;
        break;
    case 4:
        mask = 7;
        break;
    case 6:
        mask = 5;
        break;
    default:
        mask = 1;
        break;
    }
    for (i = 0; i < 3; i++) {
        if ((mask & (1 << i)) != 0) {
            flags = work->elements_0x0094[i].flags;
            if ((flags & 8) != 0 && (flags & 0x40) == 0x40) {
                return 1;
            }
        }
    }
    return 0;
}

/* 0x803AA41C (0x88): 1 when the slot's sub-state is 3, -1 at 8 (and at 5/7 for a non-zero `flag`), else 0;
 * -1 without a move work. */
extern "C" s32 quest_sub_state_end_ck(s32 flag) {
    Q_MoveWork* work = (Q_MoveWork*)get_move_work_adrs(0);
    if (work == NULL) {
        return -1;
    }
    if (work->sub_0xFA == 8) {
        return -1;
    }
    if (work->sub_0xFA == 3) {
        return 1;
    }
    if (work->sub_0xFA == 5 || work->sub_0xFA == 7) {
        if (flag != 0) {
            return -1;
        }
    }
    return 0;
}
