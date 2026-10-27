/* enemy/em_pop.cpp - the free-hunt monster set ("em_set"): the placement entries and their save records, the area
 *   set files, the rotation, the field setup and area entry, the frame step, the kill tally and reward items, and the
 *   stored result; its string pool names the data it loads (`05/em_set/em_set_m%02da%02d_%03d.esd` 0x805F81F4,
 *   `m%03d_%06d_c_pop.dat` 0x805F84E0).
 * RANGE. .text 0x803B744C-0x803B936C (26 functions); extab 0x80018F4C-0x80018FEC, extabindex 0x80039558-0x80039648,
 *   .ctors 0x8056F3B4-0x8056F3B8 (the static initialiser), .data 0x805F7CE8-0x805F8220, .bss 0x806CC420-0x806D2A68,
 *   .sdata 0x80793688-0x80793690, .sdata2 0x8079C568-0x8079C5B8.  `enemy/em_model.cpp` continues the band at
 *   0x803B936C.
 * SEAM. `quest/quest_entry.cpp` ends at 0x803B744C: its tail loads that unit's pooled literals (up to 9950.0f
 *   0x8079C560), while this unit's rows from em_set_entry_rec_apply on carry their own pool from 0x8079C568 (a second
 *   60.0f and int-to-float magic, first read by em_set_rotation_step) behind the `.sdata` hole at 0x80793684, and
 *   em_set_entry_rec_apply is called only by em_set_work_init and em_set_rotation_step.
 * FLAGS. `cflags_menu`, the address neighbour's group (`cflags_main` + `-opt nopeephole`), and a file-wide
 *   `#pragma optimization_level 4` as in quest_entry.cpp (idea 108: em_set_kill_record, em_set_work_init,
 *   em_set_field_setup, em_set_live_count and em_set_entries_spawn rise, nothing falls).
 * NAMES. The module and `em_pop` are a GUESS from the callers (nine `src/enemy/*` units); the em_set work types are
 *   `enemy/em_set_work.h`.  `__ct__9EmSetWorkFv`, `__ct__10EmSetEntryFv` and `__sinit_\em_pop_cpp` are the
 *   compiler's; every other name is a GUESS from its body: the work `em_set_work`/`em_set_work_ptr`,
 *   `em_set_default_tbl`, `em_set_reward_rows` (the 21 0x18-byte rows folded into one symbol), `em_set_reward_tbl`.
 *   GUESS: em_set_entry_rec_apply, em_set_work_init, em_set_live_count, em_set_area_files_load, em_set_entries_spawn
 *   GUESS: em_set_rotation_step, em_set_start, em_set_field_setup, quest_clock_real_step, em_set_move_start
 *   GUESS: em_set_area_enter, quest_warp_hub, em_set_frame_step, quest_move_sub_state_4_get/_set
 *   GUESS: em_set_reward_item_add, em_set_kill_record, em_set_work_state_get, em_set_userdata_flag_ck
 *   GUESS: quest_pouch_items_settle, em_set_result_store, em_set_result_restore, em_set_enemy_detach
 * RESIDUALS. Every row is written.
 *  - em_set_work_init: retail clears the 16 area sets as a 2x8 unrolled loop over a byte offset, ours unrolls it
 *    fully (loop shapes, index types and the opt_* pragmas tried); opt_propagation off is its best;
 *  - em_set_kill_record: register numbering, the `capped == 1` exit as `bne; b` in retail, a `mr` copy of the first
 *    reward rate, and `em_mini_kill_kind_get`'s s32 prototype (retail masks a u8 result at each use, request
 *    enemy-em-pop#2);
 *  - em_set_move_start: `stage_map_area_count_get` returns u8 and `quest_area_spawn_setup`'s count is not u8 in
 *    retail (requests enemy-em-pop#3/#4, owners' headers);
 *  - em_set_field_setup and quest/quest_entry.cpp's quest_enter_load: retail keeps `stage * 8` in a saved
 *    register across `stage_dcm_path_get` (register colouring; decl permutes and local types tried);
 *  - data: `.bss`/`.ctors`/`.sdata`/`.sdata2` byte-identical; `.data` 1329 of 1336 B (trailing alignment).
 * SHAPES. The entry's and the work's constructors are inline and defined last under `dont_inline`, so they follow
 *   `__sinit` as in retail and the entry's ground record is cleared in its body; the rotation fill walks a byte
 *   offset beside the count; the point rows copy field by field through pointers; em_set_result_store takes the
 *   player work before `my_player_no`.
 */

#include "types.h"
#include "nw4r/math.h"
#include "mh3_pad/vec3.h"
#include "ef/fn_800CDB2C.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "enemy/em_pop.h"
#include "enemy/em_model.h"
#include "quest/quest_entry.h"
#include "ef/get_move_work_adrs.h"
#include "font/flfnt.h"
#include "quest/quest_types.h"
#include "enemy/enemy_control.h"
#include "enemy/em020_ai.h"
#include "enemy/fn_8012EC74.h"
#include "enemy/ENEMY_WORK.h"
#include "fn_8004CAD8/get_qResult_work.h"
#include "menu/menu_item.h"
#include "Pl/fn_80273B14.h"
#include "Network/network_pat_control.h"
#include "unsplit/unknown.h"
#include "unsplit/Runtime.PPCEABI.H.h"
#include "lobby/lb_quest_screen.h"
#include "ef/eft052.h"
#include "enemy/fn_801251D0.h"
#include "enemy/em020_area2_action20_ck.h" /* em020_area2_action20_ck (rule 2) */
#include "lobby/lb_entry_start_default.h"  /* lb_entry_start_default (rule 2) */
#include "quest/quest_result_enter.h"      /* quest_result_enter (rule 2) */
#include "lobby/lb_sub17_send.h"           /* lb_sub17_send (rule 2) */
#include "lobby/quest_element_failed_ck.h" /* quest_element_live_ck (rule 2) */
#include "stage/stg_w.h"                   /* get_now_areano (rule 2) */
#include "sound/fn_800D7F54.h"             /* se_req_pos_ps (rule 2) */
#include "mh3_pad/lb_param_w.h"            /* lb_param_w (rule 2) */
#include "unsplit/menu.h"                  /* Screen_w */
#include "enemy/em_set_work.h"             /* the free-hunt work types */
#include "lobby/lb_quest_board_data.h"     /* LbQuestBoardData */
#include "unsplit/OS.h"                    /* OS_BUS_CLOCK */
#include "unsplit/lobby.h"                 /* stage_dcm_buffer_tbl (camera/camera_main.cpp's, declared in the band) */
#include "quest/quest_file_table.h"        /* stage_dcm_file_table (rule 2) */
#include "ef/system_core.h"                /* work_mem_alloc, load_file_req (rule 2) */
#include "NAND/nand.h"                     /* OSGetTime (rule 2) */
#include "camera/stage_dcm_path_get.h"     /* stage_dcm_path_get (rule 2) */
#include "ai/result_hunt_rank_get.h"       /* the result rank helpers (rule 2) */
#include "stage_map_set.h"                 /* stage_map_set (rule 2) */
#include "lobby/lb_quest_work_init.h"      /* lb_quest_work_init (rule 2) */
#include "Pl/pl_act_stage_latch_set.h"     /* pl_act_stage_latch_set, my_player_work_get (rule 2) */
#include "enemy/ENEMY_MINI_WORK.h"         /* _ENEMY_MINI_WORK */
#include "quest/quest_item_slot.h"         /* quest_item_id_low_ck, quest_move_flag_ck (rule 2) */
#include "sound/snd_bank_loader.h"         /* the stage and quest sound loaders (rule 2) */
#include "Pl/fn_80262940.h"                /* init_player_work, player_init_data_load (rule 2) */
#include "light/light.h"                   /* light_init (rule 2) */
#include "stage/shell.h"                   /* shell_work_init (rule 2) */
#include "mh3_pad.h"                       /* setVec3, copyVec3 (rule 2) */
#include "enemy/fn_8013F764.h"             /* em_kind_release (rule 2) */
#include "menu/quest_str_tbl_35_get.h"     /* quest_str_tbl_35_get (rule 2) */
#include "ef/eft_res.h"                    /* eft_control_init, eft_common_load (rule 2) */
#include "enemy/em_common.h"               /* the kill tests (rule 2) */
#include "enemy/enemy_control.h"           /* the entry spawns (rule 2) */
#include "enemy/em_model.h"                /* em_pop_work_step, em_pop_res_load (rule 2) */
#include "enemy/em_prog_work_init.h"         /* em_prog_slots_init, em_prog_work_init (rule 2) */
#include "enemy/em_quest_element_set.h"    /* em_quest_element_set (rule 2) */
#include "draw_shape.h"                    /* draw_shape_stage_load (rule 2) */
#include "fn_80056F24.h"                   /* GlareFilter_on, filter_panel_on (rule 2) */
#include "Pl/pl_act_step.h"                /* pl_area_entry_set (rule 2) */
#include "Pl/quest_spawn_rec_find.h"       /* quest_spawn_rec_find (rule 2) */
#include "camera/camera_work_init.h"       /* camera_work_init, camera_area_reset (rule 2) */
#include "ai/ainpc_init.h"                 /* ainpc_init, ainpc_res_load (rule 2) */
#include "hud/cockpit_quest_init.h"        /* cockpit_quest_init, cockpit_hunt_log_push (rule 2) */
#include "get_FqResult_work.h"             /* get_FqResult_work and the save counters (rule 2) */
#include "sound/fn_800F2A94.h"             /* snd_bgm_hold_ck, snd_hunt_stream_start (rule 2) */
#include "stage/stg_w.h"                   /* the stage area helpers (rule 2) */
#include "lobby/lb_quest_screen.h"         /* quest_area_res_load, quest_time_limit_set (rule 2) */
#include "menu/menu_item.h"                /* menu_item_work_init (rule 2) */
#include "Pl/fn_80288CEC.h"                /* pl_warp_start (rule 2) */
#include "lobby/lb_event_request.h"        /* lb_event_request (rule 2) */
#include "gallery_open.h"                  /* gallery_open (rule 2) */


extern "C" {

/* Retail is scheduled at optimization level 4 (idea 108), like `quest/quest_entry.cpp`. */
#pragma optimization_level 4


/* The default placement (thirteen entries, a zero kind ends it), the per-monster kill-reward rows and the
 * reward row each monster kind uses (NULL for none). */
EmSetDefault em_set_default_tbl[13] = {
    {12, 1, {0x10, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x42CA851F, 0x40966666, 0x448128F6, 0x00000000, 0x000074CD, 0x00000000}, {0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
    {12, 1, {0x10, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x42CA851F, 0x40966666, 0x448128F6, 0x00000000, 0x000074CD, 0x00000000}, {0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
    {3, 1, {0xFF, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000}, {0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
    {3, 1, {0xFF, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000}, {0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
    {16, 1, {0xFF, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000}, {0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
    {16, 1, {0xFF, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000}, {0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
    {1, 1, {0xFF, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000}, {0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
    {1, 1, {0xFF, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000}, {0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
    {15, 1, {0xFF, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC5561000, 0xC53B8000, 0x4544E000, 0x00000000, 0x00002500, 0x00000000}, {0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
    {15, 1, {0xFF, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x44BB8000, 0xC4BB8000, 0xC53B8000, 0x00000000, 0x00000000, 0x00000000}, {0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
    {2, 1, {0xFF, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000}, {0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
    {12, 1, {0x10, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x42CA851F, 0x40966666, 0x448128F6, 0x00000000, 0x000074CD, 0x00000000}, {0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
    {0, 0, {0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000}, {0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
};
EmSetReward em_set_reward_rows[21][2] = {
    {{{10, 30, 35, 25}, {0x0, 0xFE, 0x100, 0xF2}}, {{5, 35, 30, 30}, {0x0, 0xFE, 0x100, 0xF2}}},
    {{{10, 40, 25, 25}, {0x0, 0xFE, 0x100, 0xC1}}, {{5, 40, 25, 30}, {0x0, 0xFE, 0x100, 0xC1}}},
    {{{15, 25, 40, 20}, {0x0, 0xEF, 0xBF, 0xFD}}, {{10, 30, 35, 25}, {0x0, 0xEF, 0xBF, 0xFD}}},
    {{{15, 40, 20, 25}, {0x0, 0xEF, 0xF9, 0xDC}}, {{10, 40, 20, 30}, {0x0, 0xEF, 0xF9, 0xDC}}},
    {{{15, 25, 35, 25}, {0x0, 0x100, 0xF1, 0xFF}}, {{10, 25, 35, 30}, {0x0, 0x100, 0xF1, 0xFF}}},
    {{{10, 40, 25, 25}, {0x0, 0xF9, 0xE4, 0xF1}}, {{5, 35, 30, 30}, {0x0, 0xF9, 0xE4, 0xF1}}},
    {{{25, 60, 15, 0}, {0x0, 0xFC, 0xC0, 0x0}}, {{0, 0, 0, 0}, {0x0, 0x0, 0x0, 0x0}}},
    {{{55, 43, 2, 0}, {0xFC, 0xC0, 0xF9, 0x0}}, {{0, 0, 0, 0}, {0x0, 0x0, 0x0, 0x0}}},
    {{{15, 50, 35, 0}, {0x0, 0xFB, 0xED, 0x0}}, {{0, 0, 0, 0}, {0x0, 0x0, 0x0, 0x0}}},
    {{{20, 55, 25, 0}, {0x0, 0x1D, 0xFB, 0x0}}, {{0, 0, 0, 0}, {0x0, 0x0, 0x0, 0x0}}},
    {{{45, 40, 15, 0}, {0x0, 0xEA, 0xE8, 0x0}}, {{0, 0, 0, 0}, {0x0, 0x0, 0x0, 0x0}}},
    {{{15, 30, 55, 0}, {0x0, 0x1D, 0xC5, 0x0}}, {{0, 95, 5, 0}, {0x0, 0xF7, 0xDC, 0x0}}},
    {{{20, 35, 45, 0}, {0x0, 0xCF, 0xD0, 0x0}}, {{0, 0, 0, 0}, {0x0, 0x0, 0x0, 0x0}}},
    {{{20, 45, 35, 0}, {0x0, 0xCF, 0xD0, 0x0}}, {{0, 0, 0, 0}, {0x0, 0x0, 0x0, 0x0}}},
    {{{25, 45, 30, 0}, {0x0, 0xEA, 0xDB, 0x0}}, {{0, 0, 0, 0}, {0x0, 0x0, 0x0, 0x0}}},
    {{{30, 40, 30, 0}, {0x0, 0xE0, 0xEA, 0x0}}, {{0, 0, 0, 0}, {0x0, 0x0, 0x0, 0x0}}},
    {{{25, 60, 15, 0}, {0x0, 0xE0, 0xF4, 0x0}}, {{0, 0, 100, 0}, {0x0, 0x0, 0xF8, 0x0}}},
    {{{15, 15, 70, 0}, {0x0, 0x0, 0xF4, 0x0}}, {{0, 95, 5, 0}, {0x0, 0xE8, 0xFA, 0x0}}},
    {{{15, 15, 70, 0}, {0x0, 0x0, 0xF4, 0x0}}, {{0, 95, 5, 0}, {0x0, 0xE8, 0xFA, 0x0}}},
    {{{15, 15, 70, 0}, {0x0, 0x0, 0xF4, 0x0}}, {{0, 95, 5, 0}, {0x0, 0xE8, 0xFA, 0x0}}},
    {{{15, 15, 70, 0}, {0x0, 0x0, 0xF4, 0x0}}, {{0, 95, 5, 0}, {0x0, 0xE8, 0xFA, 0x0}}},
};
EmSetReward* em_set_reward_tbl[41] = {
    NULL, em_set_reward_rows[0], em_set_reward_rows[1], em_set_reward_rows[2],
    NULL, NULL, NULL, NULL,
    NULL, NULL, em_set_reward_rows[6], em_set_reward_rows[7],
    em_set_reward_rows[3], NULL, NULL, em_set_reward_rows[4],
    em_set_reward_rows[5], em_set_reward_rows[8], NULL, NULL,
    NULL, NULL, NULL, em_set_reward_rows[9],
    NULL, NULL, em_set_reward_rows[10], em_set_reward_rows[11],
    NULL, NULL, em_set_reward_rows[12], em_set_reward_rows[13],
    em_set_reward_rows[14], em_set_reward_rows[15], em_set_reward_rows[16], NULL,
    em_set_reward_rows[17], em_set_reward_rows[18], em_set_reward_rows[19], em_set_reward_rows[20],
    NULL,
};

/* The free-hunt work (built by the unit's static constructor) and the slot the module reads it through. */
EmSetWork em_set_work;
EmSetWorkSlot em_set_work_ptr = {&em_set_work, 0};

/* The stage dcm archive buffers, by stage. */
static inline u8** em_set_dcm_buffers(void) {
    return (u8**)stage_dcm_buffer_tbl;
}

/* Copies a save record's monster, parameters and stat rows into a placement entry. */
void em_set_entry_rec_apply(EmSetEntry* entry, EmSetSaveRec* rec) {
    entry->monster_0x31 = rec->monster_0x0;
    entry->param_0x32[0] = rec->param_0x1[0];
    entry->param_0x32[1] = rec->param_0x1[1];
    entry->param_0x32[2] = rec->param_0x1[2];
    entry->act_set_0x35 = rec->act_set_0x4;
    entry->act_sub_0x36 = rec->act_sub_0x6;
    entry->stat_a_0x38 = rec->stat_a_0x8;
    entry->stat_b_0x3A = rec->stat_b_0xA;
}

/* Builds the free-hunt work: clears it, copies the save block in, seeds the entries from the default table,
 * allocates the 32 set-file buffers, applies the saved live monsters, and takes the lobby's rotation, point rows
 * and options. */
#pragma opt_propagation off
void em_set_work_init(void) {
    Q_MoveWork* work;
    EmSetDefault* def;
    EmSetEntry* entry;
    EmSetSave* save;
    LbParamWork* param;
    s32 i;
    s32 count;
    s32 ofs;

    work = (Q_MoveWork*)get_move_work_adrs(0);
    if (work != NULL) {
        memset(&em_set_work, 0, sizeof(EmSetWork));
        memcpy(&em_set_work_ptr.work->userdata_0x04E4, get_userdata(), sizeof(Q_UserData));
        entry = em_set_work_ptr.work->entries_0x005C;
        def = em_set_default_tbl;
        i = 0;
        while (def->kind_0x00 != 0) {
            entry->kind_0x00 = def->kind_0x00;
            entry->count_0x02 = def->count_0x04;
            em_ground_rec_set(&entry->ground_0x08.rec, &def->place_0x08);
            i++;
            def++;
            entry++;
            if (i >= 13) {
                break;
            }
        }
        for (i = 0; i < 32; i++) {
            em_set_work_ptr.work->area_buffers_0x6544[i] = (u8*)work_mem_alloc(0x800);
            if (em_set_work_ptr.work->area_buffers_0x6544[i] != NULL) {
                memset(em_set_work_ptr.work->area_buffers_0x6544[i], 0, 0x800);
            }
        }
        em_set_work_ptr.work->active_0x000C[0] = -1;
        em_set_work_ptr.work->active_0x000C[1] = -1;
        em_set_work_ptr.work->active_0x000C[2] = -1;
        save = &em_set_work_ptr.work->userdata_0x04E4.em_set_0x3F98;
        if (save->active_0xD0[0] == 0 && save->active_0xD0[1] == 0) {
            em_set_work_ptr.work->active_0x000C[0] = -1;
            em_set_work_ptr.work->active_0x000C[1] = -1;
            em_set_work_ptr.work->active_0x000C[2] = -1;
        } else {
            for (i = 0; i < 2; i++) {
                if (save->active_0xD0[i] != 0) {
                    em_set_work_ptr.work->active_0x000C[i] = save->active_0xD0[i] - 1;
                    em_set_entry_rec_apply(&em_set_work_ptr.work->entries_0x005C[em_set_work_ptr.work->active_0x000C[i]],
                                           &save->recs_0x00[save->active_0xD0[i]]);
                }
            }
        }
        for (i = 0; i < 16; i++) {
            em_set_work_ptr.work->area_sets_0x001C[i] = -1;
        }
        em_set_work_ptr.work->season_0x000B = em_set_work_ptr.work->userdata_0x04E4.em_set_0x3F98.season_0xD2;
        param = &lb_param_w;
        work->state_0x22D4 = param->entry_0x08;
        work->bgm_0x22D5 = param->sub_0x29;
        work->bgm_0x22D6 = param->sub_0x2A;
        count = 0;
        ofs = 0;
        if (param->rotation_0x1E[0][0] != 0) {
            em_set_work_ptr.work->rotation_bytes_0x65C4[ofs] = param->rotation_0x1E[0][0];
            em_set_work_ptr.work->rotation_bytes_0x65C4[ofs + 1] = param->rotation_0x1E[0][1];
            ofs += 2;
            count++;
        }
        if (param->rotation_0x1E[1][0] != 0) {
            em_set_work_ptr.work->rotation_bytes_0x65C4[ofs] = param->rotation_0x1E[1][0];
            em_set_work_ptr.work->rotation_bytes_0x65C4[ofs + 1] = param->rotation_0x1E[1][1];
            ofs += 2;
            count++;
        }
        if (param->rotation_0x1E[2][0] != 0) {
            em_set_work_ptr.work->rotation_bytes_0x65C4[ofs] = param->rotation_0x1E[2][0];
            em_set_work_ptr.work->rotation_bytes_0x65C4[ofs + 1] = param->rotation_0x1E[2][1];
            ofs += 2;
            count++;
        }
        if (param->rotation_0x1E[3][0] != 0) {
            em_set_work_ptr.work->rotation_bytes_0x65C4[ofs] = param->rotation_0x1E[3][0];
            em_set_work_ptr.work->rotation_bytes_0x65C4[ofs + 1] = param->rotation_0x1E[3][1];
            ofs += 2;
            count++;
        }
        for (i = count; i < 4; i++) {
            em_set_work_ptr.work->rotation_0x65C4[i][0] = 0;
            em_set_work_ptr.work->rotation_0x65C4[i][1] = 0;
        }
        em_set_work_ptr.work->rotation_state_0x65CC = 0;
        em_set_work_ptr.work->rotation_index_0x65CD = 0;
        em_set_work_ptr.work->rotation_ptr_0x65D0 = em_set_work_ptr.work->rotation_0x65C4;
        quest_snd_wk_init();
        work->flag_0x22DC = lb_param_w.hunt_option_0x0A;
        work->rank_sel_0x22D8 = lb_param_w.rank_sel_0x09;
        em_set_work_ptr.work->userdata_flag_0x6644 = em_set_work_ptr.work->userdata_0x04E4.em_set_0x3F98.flag_0xD4;
    }
}
#pragma opt_propagation reset

/* Loads the monster of every live entry and counts them. */
void em_set_live_count(void) {
    s32 i;
    em_set_work_ptr.work->live_count_0x000F = 0;
    for (i = 0; i < 3; i++) {
        if (em_set_work_ptr.work->active_0x000C[i] != -1 &&
            em_set_work_ptr.work->entries_0x005C[em_set_work_ptr.work->active_0x000C[i]].monster_0x31 != 0) {
            em_kind_release(em_set_work_ptr.work->entries_0x005C[em_set_work_ptr.work->active_0x000C[i]].monster_0x31);
            em_set_work_ptr.work->live_count_0x000F++;
        }
    }
}

/* Takes the area set rows (the lobby quest's, or the save's season row) and requests every area's set file of
 * map `map`. */
void em_set_area_files_load(Q_MoveWork* work, s32 map) {
    char path[0x100];
    s16* row;
    s32 i;
    u8 count;

    count = stage_map_area_count_get(map);
    if (quest_select_ready_ck() == 1) {
        row = em_set_quest_area_tbl[lb_param_w.field_0x00];
    } else {
        row = em_set_season_area_tbl[em_set_work_ptr.work->season_0x000B];
    }
    em_set_work_ptr.work->area_sets_0x001C[0] = row[0];
    em_set_work_ptr.work->area_sets_0x001C[1] = row[1];
    em_set_work_ptr.work->area_sets_0x001C[2] = row[2];
    em_set_work_ptr.work->area_sets_0x001C[3] = row[3];
    em_set_work_ptr.work->area_sets_0x001C[4] = row[4];
    em_set_work_ptr.work->area_sets_0x001C[5] = row[5];
    em_set_work_ptr.work->area_sets_0x001C[6] = row[6];
    em_set_work_ptr.work->area_sets_0x001C[7] = row[7];
    em_set_work_ptr.work->area_sets_0x001C[8] = row[8];
    em_set_work_ptr.work->area_sets_0x001C[9] = row[9];
    em_set_work_ptr.work->area_sets_0x001C[10] = row[10];
    em_set_work_ptr.work->area_sets_0x001C[11] = row[11];
    em_set_work_ptr.work->area_sets_0x001C[12] = row[12];
    for (i = 0; i < count; i++) {
        if (em_set_work_ptr.work->area_sets_0x001C[i] != -1) {
            sprintf(path, "05/em_set/em_set_m%02da%02d_%03d.esd", map, i, em_set_work_ptr.work->area_sets_0x001C[i]);
            load_file_req(path, (u32)em_set_work_ptr.work->area_buffers_0x6544[i], 0x800, 0, 0, NULL);
        }
    }
}

/* Spawns the live entries' monsters into the move work's spawn records (stops at the first that fails). */
void em_set_entries_spawn(Q_MoveWork* work) {
    QuestSpawnRec* rec;
    struct _ENEMY_WORK* enemy;
    s32 i;
    s32 spawned;
    EmSetEntry* entry;

    spawned = 0;
    i = 0;
    rec = work->spawn_0x2274;
    for (; i < 3; rec++, i++) {
        if (em_set_work_ptr.work->active_0x000C[i] != -1) {
            entry = &em_set_work_ptr.work->entries_0x005C[em_set_work_ptr.work->active_0x000C[i]];
            enemy = em_set_entry_spawn(entry, i);
            if (enemy == NULL) {
                return;
            }
            entry->enemy_0x28 = enemy;
            entry->flags_0x30 = (u8)spawned | 0x80;
            entry->live_0x2C = 1;
            em_quest_element_set(enemy, spawned);
            rec->order_0x0 = i;
            rec->count_0x4 = entry->count_0x02;
            rec->monster_0x6 = entry->monster_0x31;
            spawned++;
        }
    }
}

/* Steps the lobby's monster rotation: waits each slot's delay with no monster live, loads the slot's monster,
 * spawns it once its files are in and records its spawn. */
void em_set_rotation_step(Q_MoveWork* work) {
    struct _ENEMY_WORK* enemy;
    struct _ENEMY_MINI_WORK* mini;
    EmSetEntry* entry;
    s32 order;
    s32 limit;
    u8 kind;

    switch (em_set_work_ptr.work->rotation_state_0x65CC) {
    case 0:
        if (em_set_work_ptr.work->rotation_index_0x65CD < 4 && em_set_work_ptr.work->live_count_0x000F <= 0) {
            kind = em_set_work_ptr.work->rotation_0x65C4[em_set_work_ptr.work->rotation_index_0x65CD][0];
            if (kind != 0 && kind <= 13) {
                kind--;
                em_set_work_ptr.work->rotation_timer_0x65D4++;
                limit = 60.0f * Screen_w.frame_scale *
                        lb_param_w.rotation_0x1E[em_set_work_ptr.work->rotation_index_0x65CD][1];
                if (em_set_work_ptr.work->rotation_timer_0x65D4 >= limit) {
                    entry = &em_set_work_ptr.work->entries_0x005C[kind];
                    if (entry->kind_0x00 != 0 && em_kind_release_ck(entry->kind_0x00) != 0) {
                        em_set_work_ptr.work->active_0x000C[0] = -1;
                        em_set_work_ptr.work->active_0x000C[1] = -1;
                        em_set_work_ptr.work->active_0x000C[2] = -1;
                        em_set_work_ptr.work->rotation_state_0x65CC = 1;
                        em_set_work_ptr.work->active_0x000C[0] = kind;
                        em_set_entry_rec_apply(entry, &em_set_work_ptr.work->userdata_0x04E4.em_set_0x3F98.recs_0x00[kind + 1]);
                        em_kind_release(entry->monster_0x31);
                        em_set_work_ptr.work->live_count_0x000F++;
                    }
                }
            }
        }
        break;
    case 1:
        if (file_loading_ck(NULL, NULL) != 1) {
            em_set_work_ptr.work->rotation_state_0x65CC = 2;
        }
        break;
    case 2:
        em_set_boss_spawn(&em_set_work_ptr.work->entries_0x005C[em_set_work_ptr.work->active_0x000C[0]],
                          em_set_work_ptr.work->rotation_index_0x65CD + 2);
        em_set_work_ptr.work->rotation_state_0x65CC = 3;
        break;
    case 3:
        enemy = NULL;
        mini = NULL;
        if ((u8)em_get_unique_work(em_set_work_ptr.work->rotation_index_0x65CD + 2, &enemy, &mini) != 0) {
            entry = &em_set_work_ptr.work->entries_0x005C[em_set_work_ptr.work->active_0x000C[0]];
            entry->enemy_0x28 = enemy;
            order = em_set_work_ptr.work->rotation_index_0x65CD + 2;
            work->spawn_0x2274[order].order_0x0 = order;
            work->spawn_0x2274[order].count_0x4 = 1;
            work->spawn_0x2274[order].monster_0x6 = entry->monster_0x31;
            em_set_work_ptr.work->rotation_index_0x65CD++;
            em_set_work_ptr.work->rotation_state_0x65CC = 0;
        }
        break;
    }
}

/* Starts the free hunt: builds the work, clears the quest work, sets the players, enemies and stage up for the
 * hunt's map and requests its files. */
void em_set_start(void) {
    Q_MoveWork* work = (Q_MoveWork*)get_move_work_adrs(0);
    em_set_work_init();
    memset(&quest_work, 0, sizeof(QuestWork));
    init_player_work();
    em_prog_work_init();
    em_prog_slots_init();
    stage_map_set(work->quest_id_0x120);
    eft_common_load();
    if (quest_select_ready_ck() == 1) {
        work->em_level_0x22DB = 0;
        em_level_set(0);
    }
    em_set_live_count();
    em_set_area_files_load(work, 1);
    player_init_data_load();
    ainpc_res_load();
    em_pop_res_load(work->quest_id_0x120);
    draw_shape_stage_load(work->quest_id_0x120);
}

/* Sets the field up for the free hunt: resets the enemy, effect, stage, camera, cockpit and item works, picks the
 * map by the rank byte, restores the saved result, loads the lobby quest's dcm archives, and sets the time limit
 * and the point rows. */
void em_set_field_setup(Q_MoveWork* work) {
    char path[0x20];
    EmSetPointRow* dst;
    EmSetPointRow* src;
    s32 limit;
    s32 extra;
    s32 i;
    s32 stage;

    enemy_work_reset();
    if (work->npc_0xEF != 0) {
        ainpc_init();
    }
    eft_control_init();
    shell_work_init();
    light_init();
    stage_work_init();
    camera_work_init();
    cockpit_quest_init();
    menu_item_work_init();
    if (work->rank_sel_0x22D8 == 0) {
        em_set_work_ptr.work->map_0x0008 = 1;
    } else {
        em_set_work_ptr.work->map_0x0008 = 12;
    }
    em_set_work_ptr.work->area_0x0009 = 1;
    em_set_work_ptr.work->warp_stage_0x000A = 0;
    em_set_result_restore();
    if (quest_select_ready_ck() == 1 && quest_item_id_low_ck(lb_param_w.field_0x00) == 1) {
        lb_quest_work_init(lb_param_w.field_0x00);
        snd_quest_bgm_load();
        work->wait_0x114 = 60;
        stage = lb_param_w.field_0x00;
        if (stage != 0) {
            if (stage_dcm_file_table[stage].size != 0) {
                stage_dcm_path_get(path, stage);
                em_set_dcm_buffers()[stage] = (u8*)work_mem_alloc(stage_dcm_file_table[stage].size);
                load_file(path, (u32)em_set_dcm_buffers()[stage], stage_dcm_file_table[stage].size);
            }
            extra = -1;
            switch (lb_param_w.field_0x00) {
            case 1:
                extra = 14;
                break;
            case 2:
                extra = 19;
                break;
            case 4:
                extra = 20;
                break;
            }
            if (extra != -1 && stage_dcm_file_table[extra].size != 0) {
                stage_dcm_path_get(path, extra);
                em_set_dcm_buffers()[extra] = (u8*)work_mem_alloc(stage_dcm_file_table[extra].size);
                load_file(path, (u32)em_set_dcm_buffers()[extra], stage_dcm_file_table[extra].size);
            }
        }
    }
    work->map_0xED = work->phase_0xE9 = em_set_work_ptr.work->map_0x0008;
    work->area_0xEE = work->sub_0xEA = em_set_work_ptr.work->area_0x0009;
    work->sub_0xFA = 2;
    work->item_flag_0x110 = 0;
    work->result_buffer_0x150 = (u8*)work_mem_alloc(0x4800);
    limit = 50.0f * (60.0f * Screen_w.frame_scale);
    quest_work.field_0x01C = limit;
    quest_work.timer_0x6A58 = 29970.0;
    quest_time_limit_set(limit);
    dst = em_set_work_ptr.work->points_0x65D8;
    src = lb_param_w.points_0x32;
    for (i = 0; i < 17; i++, src++, dst++) {
        dst->kind_0x0 = src->kind_0x0;
        dst->kill_points_0x2 = src->kill_points_0x2;
        dst->capture_points_0x4 = src->capture_points_0x4;
    }
    dst->kind_0x0 = 0;
    dst->kill_points_0x2 = 0;
    dst->capture_points_0x4 = 0;
}

/* Steps the item work's real-time clock: the frame's elapsed time over the clock rate (at least one tick, none
 * in mode 0) comes off the time left, which the frame limit follows while it runs. */
void quest_clock_real_step(s32 mode) {
    Q_ItemWork* work;
    f64 step;

    work = move_work_item_work_get();
    if (work != NULL) {
        work->timer_0x6A50 = work->timer_0x6A48;
        work->timer_0x6A48 = OSGetTime();
        if (work->timer_0x6A50 > 0.0) {
            step = 8000.0 * (work->timer_0x6A48 - work->timer_0x6A50) / (OS_BUS_CLOCK / 4 / 125000) / 1000.0 /
                   work->timer_rate_0x6A58;
            if (step < 1.0) {
                step = 1.0;
            }
            if (mode == 0) {
                step = 0.0;
            }
            work->time_left_0x6A60 -= step;
        } else if (mode == 1) {
            work->time_left_0x6A60 -= 1.0;
        }
        if (work->time_limit_0x24 > 0) {
            work->time_limit_0x24 = work->time_left_0x6A60;
            if (work->time_limit_0x24 < 0) {
                work->time_limit_0x24 = 0;
            }
        }
    }
}

/* Once the player may move: loads the map's resources, enters the area, loads the stage and player sound banks
 * and the stage BGM, and hands the area set buffers to the area spawn.  0 while the player cannot move yet. */
s32 em_set_move_start(Q_MoveWork* work) {
    u8 state;

    if (player_move_start_ck() == 0) {
        return 0;
    }
    player_move_start(-1);
    stage_map_res_load(work->phase_0xE9);
    stage_map_obj_load(work->phase_0xE9);
    stage_area_load(0, work->phase_0xE9, work->sub_0xEA);
    em_set_work_ptr.work->area_count_0x0018 = stage_map_area_count_get(work->phase_0xE9);
    snd_stage_bank_load(work->phase_0xE9, work->sub_0xEA, work->rank_sel_0x22D8);
    snd_player_banks_load();
    state = work->state_0x22D4;
    if (state & 0x80) {
        snd_quest_bgm_set(state & 0x7F, work->bgm_0x22D5, work->bgm_0x22D6);
    }
    quest_area_spawn_setup(work, (u32*)em_set_work_ptr.work->area_buffers_0x6544, em_set_work_ptr.work->area_count_0x0018, 0);
    quest_area_spawn_apply(work, work->phase_0xE9, work->sub_0xEA);
    quest_area_res_load(work->phase_0xE9, work->sub_0xEA);
    return 1;
}

/* Once the files are in: enters the stage area, places the player at its entry, spawns the live entries and
 * the area's enemies and turns the glare filter on.  0 while files are still loading. */
s32 em_set_area_enter(Q_MoveWork* work) {
    u16 angle;

    if (file_loading_ck(NULL, NULL) == 1) {
        return 0;
    }
    stage_area_enter(work->phase_0xE9, work->sub_0xEA);
    copyVec3(&work->entry_pos_0x128, stage_entry_pos_get(work->phase_0xE9, work->sub_0xEA));
    angle = stage_entry_angle_get(work->phase_0xE9, work->sub_0xEA);
    work->entry_angle_0x134 = angle;
    pl_area_entry_set(&work->entry_pos_0x128, angle, work->phase_0xE9, work->sub_0xEA);
    camera_area_reset();
    em_set_entries_spawn(work);
    quest_screen_enemy_start(work);
    em_pop_work_step();
    work->area_slots_0xE0[work->area_slot_index_0xE8] = (s8)stage_area_slot_get(work->phase_0xE9, work->sub_0xEA);
    GlareFilter_on();
    filter_panel_on();
    return 1;
}

/* Warps the local player to the start of stage 1 (the hub): the stage's start position and facing go through
 * `pl_warp_start` with the work's warp stage. */
void quest_warp_hub(void) {
    nw4r::math::VEC3 pos;
    s32 angle;

    setVec3(&pos, 0.0f, 0.0f, 0.0f);
    stage_start_get(1, &pos, &angle);
    pl_act_stage_latch_set(my_player_work_get(), 2);
    pl_warp_start(em_set_work_ptr.work->warp_stage_0x000A, &pos, angle);
}

/* Steps the free hunt each frame: counts the hunt time (up to 25 and 99 minutes), runs the real-time clock, then
 * either requests the lobby event once or runs the gallery cell check, the area sound frame and the rotation. */
void em_set_frame_step(Q_MoveWork* work) {
    struct _PLW* players;
    StageCell cell;

    if ((f32)em_set_work_ptr.work->frames_0x0010 < 25.0f * (60.0f * Screen_w.frame_scale)) {
        em_set_work_ptr.work->frames_0x0010++;
    }
    if ((f32)em_set_work_ptr.work->frames_long_0x0014 < 99.0f * (60.0f * Screen_w.frame_scale)) {
        em_set_work_ptr.work->frames_long_0x0014++;
    }
    quest_clock_real_step(1);
    switch (em_set_work_ptr.work->step_0x0004) {
    case 0:
        if (quest_select_ready_ck() == 1) {
            lb_event_request(0);
        }
        em_set_work_ptr.work->step_0x0004++;
        break;
    case 1:
        if (quest_select_ready_ck() == 1) {
            quest_gallery_cell_step(work);
            if (work->local_0x112 == 0) {
                snd_quest_frame_begin(0, work->phase_0xE9, work->sub_0xEA);
                snd_quest_frame_end();
            }
        } else {
            if (snd_bgm_hold_ck() == 0 && work->sub_0xEA == 1) {
                players = (struct _PLW*)get_move_work_adrs(2);
                if (players != NULL) {
                    cell.word = stage_cell_get(players[(s8)my_player_no()].area_cell_0x5AB, 1);
                    if (cell.bytes[0] == 0xFA && cell.bytes[1] == 0xFF && cell.bytes[2] == 0xFF) {
                        snd_bgm_hold_set();
                        gallery_open(14);
                    }
                }
            }
            if (work->local_0x112 == 0) {
                snd_quest_frame_begin(work->rank_sel_0x22D8, work->phase_0xE9, work->sub_0xEA);
                snd_quest_frame_end();
            }
            em_set_rotation_step(work);
        }
        break;
    }
}

/* The entry sub-state of the local slot's move work: 1 when it is 4, 0 otherwise, -1 without a work. */
s32 quest_move_sub_state_4_get(void) {
    Q_MoveWork* work = (Q_MoveWork*)get_move_work_adrs(0);
    if (work == NULL) {
        return -1;
    }
    return work->sub_0xFA == 4;
}

/* Sets the local slot's sub-state to 4 (and its 0x22D9 flag when `flag` is 1). */
void quest_move_sub_state_4_set(u8 flag) {
    Q_MoveWork* work = (Q_MoveWork*)get_move_work_adrs(0);
    if (work != NULL) {
        work->sub_0xFA = 4;
        if (flag == 1) {
            work->sub_flag_0x22D9 = 1;
        }
    }
}

/* Adds one of `item` to the hunt's reward box: 0 when added, 1 when its stack is already at 99, 2 when the box
 * is full. */
s32 em_set_reward_item_add(u16 item) {
    EmSetRewardItem* slot;
    EmSetRewardItem* scan;
    s32 i;

    slot = em_set_work_ptr.work->rewards_0x0460;
    scan = slot;
    for (i = 0; i < 30; i++, scan++) {
        if (scan->item_0x0 == item) {
            if (scan->count_0x2 < 99) {
                scan->count_0x2++;
                return 0;
            }
            return 1;
        }
    }
    for (i = 0; i < 30; i++, slot++) {
        if (slot->item_0x0 == 0) {
            slot->item_0x0 = item;
            slot->count_0x2++;
            return 0;
        }
    }
    return 2;
}

/* Records a kill of `enemy` (a mini-work record when `mini` is set): drops its spawn count, tallies the kill or
 * capture (capped at 99) in the work and the save block, adds the kill's points, logs it on the cockpit and
 * rolls the monster's reward row once per kill (twice for a large kill, four times for a capture). */
void em_set_kill_record(struct _ENEMY_WORK* enemy, u8 mini) {
    s32 tries;
    struct _ENEMY_WORK* large_enemy;
    s32 size;
    s32 large;
    u16 order;
    u8 monster;
    u32 hit;
    u8 end_kind;
    QuestSpawnRec* rec;
    EmSetReward* reward;
    s32 count;
    u8 log_kind;
    u32 capped;
    EmSetPointRow* row;
    s32 i;
    s32 roll;
    s32 pick;
    s32 sum;

    tries = 0;
    large_enemy = NULL;
    get_userdata();
    size = 0;
    large = 0;
    if (mini == 0) {
        large_enemy = enemy;
        order = enemy->field_0x01A;
        monster = enemy->team;
        hit = em_record_hit_ck(enemy);
        end_kind = em_captured_ck(enemy);
        if (enemy->field_0x1C8 & 1) {
            large = 1;
            size = 100.0f * get_em_chg_scale(enemy);
        }
    } else {
        order = ((struct _ENEMY_MINI_WORK*)enemy)->order_0x16;
        monster = ((struct _ENEMY_MINI_WORK*)enemy)->monster_0x02;
        hit = em_mini_hit_ck(enemy);
        end_kind = em_mini_kill_kind_get(enemy);
    }
    rec = quest_spawn_rec_find(order);
    if (rec != NULL && rec->count_0x4 > 0) {
        rec->count_0x4--;
    }
    reward = em_set_reward_tbl[monster];
    log_kind = 0;
    count = 0;
    if (large != 0) {
        em_set_work_ptr.work->live_count_0x000F--;
        if (em_set_work_ptr.work->live_count_0x000F == 0) {
            em_set_work_ptr.work->rotation_timer_0x65D4 = 0;
        }
        if (end_kind == 0) {
            em_set_work_ptr.work->hunted_0x0368[monster]++;
            tries = 2;
            count = em_set_work_ptr.work->hunted_0x0368[monster];
            log_kind = 1;
            if (em_set_work_ptr.work->hunted_0x0368[monster] > 99) {
                em_set_work_ptr.work->hunted_0x0368[monster] = 99;
            }
            userdata_hunt_count_add(monster, 1);
        }
        if (end_kind == 1) {
            em_set_work_ptr.work->captured_0x03BA[monster]++;
            if (reward != NULL) {
                reward++;
            }
            tries = 4;
            count = em_set_work_ptr.work->captured_0x03BA[monster];
            log_kind = 2;
            if (em_set_work_ptr.work->captured_0x03BA[monster] > 99) {
                em_set_work_ptr.work->captured_0x03BA[monster] = 99;
            }
            userdata_capture_count_add(monster, 1);
        }
        userdata_size_record_set(monster, size);
        if (large_enemy != NULL) {
            em_set_enemy_detach(large_enemy);
        }
        snd_hunt_stream_start(0);
    } else {
        if (end_kind == 2) {
            return;
        }
        em_set_work_ptr.work->small_count_0x045E++;
        if (em_set_work_ptr.work->small_count_0x045E > 99) {
            em_set_work_ptr.work->small_count_0x045E = 99;
        }
        if (hit == 1 && reward != NULL) {
            reward++;
        }
        em_set_work_ptr.work->hunted_0x0368[monster]++;
        tries = 1;
        count = em_set_work_ptr.work->hunted_0x0368[monster];
        log_kind = 0;
        if (em_set_work_ptr.work->hunted_0x0368[monster] > 99) {
            em_set_work_ptr.work->hunted_0x0368[monster] = 99;
        }
        userdata_hunt_count_add(monster, 1);
    }
    if (count > 99) {
        count = 99;
        capped = 1;
    } else {
        capped = 0;
    }
    if (quest_move_flag_ck() == 0) {
        return;
    }
    if (capped == 0) {
        row = em_set_work_ptr.work->points_0x65D8;
        while ((u8)row->kind_0x0 != 0) {
            if ((u8)row->kind_0x0 == monster) {
                if (large != 0) {
                    if (end_kind == 0) {
                        em_set_work_ptr.work->points_0x04D8 += row->kill_points_0x2;
                    }
                    if (end_kind == 1) {
                        em_set_work_ptr.work->points_0x04D8 += row->capture_points_0x4;
                    }
                } else {
                    em_set_work_ptr.work->points_0x04D8 += row->kill_points_0x2;
                }
                break;
            }
            row++;
        }
        if (em_set_work_ptr.work->points_0x04D8 > 999999) {
            em_set_work_ptr.work->points_0x04D8 = 999999;
        }
    }
    cockpit_hunt_log_push(monster, count, log_kind);
    if (reward == NULL) {
        return;
    }
    if (capped == 1) {
        return;
    }
    for (i = 0; i < tries; i++) {
        roll = (u16)ran_suu(0) % 100;
        pick = 0;
        sum = reward->rate_0x0[0];
        if (sum <= roll) {
            pick = 1;
            sum += reward->rate_0x0[1];
            if (sum <= roll) {
                pick = 2;
                sum += reward->rate_0x0[2];
                if (sum <= roll) {
                    pick = 3;
                    sum += reward->rate_0x0[3];
                    if (sum <= roll) {
                        pick = 4;
                    }
                }
            }
        }
        if (pick < 4 && reward->item_0x4[pick] != 0) {
            em_set_reward_item_add(reward->item_0x4[pick]);
        }
    }
}

/* The hunt's points (the cockpit's score). */
s32 em_set_work_state_get(void) {
    return em_set_work_ptr.work->points_0x04D8;
}

/* Whether the save block's free-hunt flag is set. */
s32 em_set_userdata_flag_ck(void) {
    return em_set_work_ptr.work->userdata_flag_0x6644 != 0;
}

/* Settles the carried-item pouch: sells the category-1 pairs (ten each), clears the category-2 pairs (raising
 * `flag`) and drops the ones the player cannot use; returns the credit. */
s32 quest_pouch_items_settle(Q_ItemPair* pouch, u8* flag) {
    s32 i;
    s32 credit = 0;
    *flag = 0;
    for (i = 0; i < 35; i++, pouch++) {
        if (pouch->id != 0 && (s16)pouch->num != 0) {
            if (item_category_ck(pouch->id, 1)) {
                credit += (s16)pouch->num * 10;
                pouch->id = 0;
                pouch->num = 0;
            } else if (item_category_ck(pouch->id, 2)) {
                pouch->id = 0;
                pouch->num = 0;
                *flag = 1;
            } else if (Pl_item_id_usable_ck(pouch->id, 1) == 0) {
                pouch->id = 0;
                pouch->num = 0;
            }
        }
    }
    return credit;
}

/* Stores the free hunt's result: the counts, the reward box and the points, the settled pouch (announcing the
 * sold and dropped items), the live monsters, the hunt time and the result ranks.  `end_kind` is how the hunt
 * ended. */
void em_set_result_store(u8 end_kind) {
    char text[0x80];
    FqResultWork* result;
    struct _PLW* me;
    s32 credit;
    u8 cleared;
    s32 i;

    result = get_FqResult_work();
    me = (struct _PLW*)get_move_work_adrs(2);
    me = &me[(s8)my_player_no()];
    memcpy(result->hunted_0x000, em_set_work_ptr.work->hunted_0x0368, 0x52);
    memcpy(result->captured_0x052, em_set_work_ptr.work->captured_0x03BA, 0x52);
    memcpy(result->counts_c_0x0A4, em_set_work_ptr.work->counts_c_0x040C, 0x52);
    result->small_count_0x0F6 = em_set_work_ptr.work->small_count_0x045E;
    memcpy(result->rewards_0x0F8, em_set_work_ptr.work->rewards_0x0460, 0x78);
    credit = quest_pouch_items_settle((struct Q_ItemPair*)me->slot_id, &cleared);
    if (cleared == 1) {
        sprintf(text, quest_str_tbl_35_get(0x15));
        hud_msg_push(0, text);
    }
    if (credit > 0) {
        userdata_zenny_add(credit);
        sprintf(text, quest_str_tbl_35_get(0x1D), credit);
        hud_msg_push(0, text);
    }
    memcpy(result->pouch_0x1B4, me->slot_id, 0x60);
    memcpy(result->spare_0x214, me->spare_slot_id, 0x20);
    result->points_0x170 = 0;
    for (i = 0; i < 3; i++) {
        if (em_set_work_ptr.work->active_0x000C[i] == -1) {
            result->active_0x174[i] = 0;
        } else {
            result->active_0x174[i] = em_set_work_ptr.work->active_0x000C[i] + 1;
            if (em_set_work_ptr.work->entries_0x005C[em_set_work_ptr.work->active_0x000C[i]].enemy_0x28 != NULL) {
                result->monsters_0x178[i].area_0x0 =
                    em_set_work_ptr.work->entries_0x005C[em_set_work_ptr.work->active_0x000C[i]].enemy_0x28->area_no;
            }
        }
    }
    result->frames_0x1A8 = em_set_work_ptr.work->frames_0x0010;
    result->seconds_0x1AC = (f32)em_set_work_ptr.work->frames_0x0010 / (60.0f * Screen_w.frame_scale);
    result->frames_long_0x1B0 = em_set_work_ptr.work->frames_long_0x0014;
    result->end_kind_0x240 = end_kind;
    system_w.field_0x7ce = end_kind;
    result->hunt_rank_0x242 = result_hunt_rank_get(&em_set_work_ptr.work->hunted_set_0x0368, &em_set_work_ptr.work->captured_set_0x03BA);
    result->time_rank_0x244 = result_time_rank_get(result->seconds_0x1AC);
    result_rank_rows_fill(result->rank_rows_0x246);
    result->rank_points_0x278 = result_rank_points_get(&em_set_work_ptr.work->hunted_set_0x0368, &em_set_work_ptr.work->captured_set_0x03BA);
}

/* Restores the counts, the reward box and the points from the stored free-hunt result. */
void em_set_result_restore(void) {
    FqResultWork* result = get_FqResult_work();
    memcpy(em_set_work_ptr.work->hunted_0x0368, result->hunted_0x000, 0x52);
    memcpy(em_set_work_ptr.work->captured_0x03BA, result->captured_0x052, 0x52);
    memcpy(em_set_work_ptr.work->counts_c_0x040C, result->counts_c_0x0A4, 0x52);
    em_set_work_ptr.work->small_count_0x045E = result->small_count_0x0F6;
    memcpy(em_set_work_ptr.work->rewards_0x0460, result->rewards_0x0F8, 0x78);
    em_set_work_ptr.work->points_0x04D8 = result->points_0x170;
}

/* Releases `enemy`'s hold flag on whichever live entry spawned it. */
void em_set_enemy_detach(struct _ENEMY_WORK* enemy) {
    s32 i;
    for (i = 0; i < 3; i++) {
        if (em_set_work_ptr.work->active_0x000C[i] != -1 &&
            em_set_work_ptr.work->entries_0x005C[em_set_work_ptr.work->active_0x000C[i]].enemy_0x28 == enemy) {
            em_set_work_ptr.work->entries_0x005C[em_set_work_ptr.work->active_0x000C[i]].flags_0x30 &= 0x7F;
        }
    }
}

} /* extern "C" */

/* Builds an entry's ground record. */
inline EmSetEntry::EmSetEntry() {
    em_ground_rec_clear(&ground_0x08.rec);
}

/* The constructors are inline, so they follow the static initialiser; `__sinit` (compiled at the end of the file)
 * calls the work's constructor out of line. */
#pragma dont_inline on

/* Builds the work's thirteen entries. */
inline EmSetWork::EmSetWork() {}
