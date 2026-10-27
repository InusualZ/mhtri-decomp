/*
 * Pl/pl_motion.cpp - the player actor's motion layer and the scene driver: the act-style `_PLW` class-7 state
 *   handlers and their dispatcher (0x80288CEC-0x8028B524), the three game-mode tasks with their per-mode scene steps
 *   (quest, field and versus; 0x8028B524-0x8028EF30) and the root-work accessors (0x8028EF30 on).
 * RANGE. .text 0x80288CEC-0x8028F44C (74 functions); .ctors 0x8056F36C, .data 0x805CBFC0-0x805CDC44, .bss
 *   0x806AB830-0x806AB848, .sdata 0x807921B0-0x80792270, .sbss 0x80794B38-0x80794B58, .sdata2 0x8079A270-0x8079A310,
 *   extab, extabindex.  Left seam: the unit's `.sdata2` run starts with `pl_act_class7_frame_update`'s first constant
 *   (0.0f) where the preceding unit's run ends.  `Pl/pl_hit_sphere.cpp` follows it.
 * NAMES. The file name is a GUESS from the motion entry points (`pl_motion_set` loads the motion banks); `pl_motion_set`
 *   and `get_gm_daynight` are the map's names.  The root work `get_move_work_adrs(0)` hands back is `_PL_ROOT`
 *   (`Pl/fn_80288CEC.h`), the record `enemy/em_common.cpp` defines again as `_PLAYER_ROOT` and the quest units declare
 *   as `struct Q_MoveWork` (rule 1); the calls into the quest units cast it.
 *   GUESS (each body and its callers): quest_spawn_rec_find, quest_spawn_rec_find_in, quest_move_sub_flag_get,
 *   quest_em_level_get, arena_userdata_slot_apply, pl_act_class7_frame_update(_0/_1) (the class-7 hit/quake update).
 *   GUESS (the dispatch chain, `Pl/pl_act_class3.cpp`'s scheme): pl_act_class7_dispatch and pl_act_class7_handler_00..26
 *   in address order; pl_act_class7_handler_NN_rows/_steps/_gauge_steps name the handler that reads them.
 *   GUESS (the scene driver: who hands them to `Tsk_Change` and which root mode they run): quest_game_task,
 *   field_game_task, vs_game_task; quest_/field_/vs_scene_boot/_load/_main/_end (root modes 0-3);
 *   quest_scene_work_init, vs_scene_work_init, quest_scene_res_load, vs_scene_res_load, scene_actors_move,
 *   scene_hud_update, scene_area_change_step, scene_se_handles_release, scene_exit_point_ck, net_player_drop_step,
 *   net_error_dialog_draw; data quest_stage_table/_no_table/_variant_table, quest_stage_NN, quest_stage_release_*,
 *   pl_motion_res_names/_name_*, net_error_msg, net_error_code_fmt, scene_exit_point_a/b.
 *   Registry GUESS names for other units' rows are listed in their leaf headers (`names.py list --lane pl-motion`).
 * RESIDUALS. Every function is written.  Below 100 %:
 *  - quest_scene_boot, quest_scene_end, vs_scene_end: retail shares one tail between two switch cases (case 4's movie
 *    cut jumps into its close path; the end modes' case 2 reuses case 1's `scene_actors_move` call, and the versus
 *    end's case 2 reuses case 99's leave block) - a `goto` shape (rule 8); the source repeats the tail.
 *  - field_scene_main: the post-exit block sits after the switch in retail; the source keeps it in case 0.
 *  - quest_scene_load, scene_area_change_step: a `case 0: case 1:` pair compares `cmplwi 1`/`cmpwi 2` in retail; the
 *    `(u32)` switch gets `cmplwi` for both.  quest_scene_res_load: the `release`/`param` registers swap.
 *  - net_error_dialog_draw: the button x (`x + 526`) is rematerialised at each call instead of kept in r31, and the
 *    two `0.5f` products swap their `fmuls` operands.
 *  - data: the class-7 step/row tables, the stage tables and the `.sdata` stage records are `extern` map symbols the
 *    object does not emit (dtk's split of the TU's `.data` carries relocations into another unit inside some tables),
 *    so `.data`/`.sdata` are not matched; `.sbss` lacks `quest_stage_release_none` (only the `.sdata` records use it).
 *    The two jump tables are compiler-local (`@NNNN`) against the map's `jumptable_805CBFC0`/`jumptable_805CC278`.
 * SHAPES. Callees whose owner header types a parameter or return differently from the retail call are called through a
 *   `_c1` cast view.  The exit points are a `setVec3`-constructed `SceneExitPoint`, so MWCC's `__sinit` builds them
 *   (`.ctors`).  The scene inits clear the root counters with unrolled loops; the network slot loops count in an `int`.
 */

#include "types.h"
#include "nw4r/math.h"
#include "pl.h"
#include "Pl/pl_act.h"
#include "Pl/Pl_master_ck.h"
#include "Pl/fn_802693C4.h"
#include "Pl/pl_motion.h"
#include "fn_8004CAD8.h" /* rotVecY (rule 2) */
#include "g3d/g3d_calcworld.h" /* addVec3To (rule 2) */
#include "ef/eft004.h" /* eft004_set_pl2 (rule 2) */
#include "Pl/fn_80288CEC.h" /* `PlBox` and `_PL_ROOT`, shared with `Pl/pl_hit_sphere.cpp` and `Pl/pl_coll.cpp` (rule 1) */
#include "Pl/fn_80262940.h" /* player_init_data_load (the owner's header, rule 2) */
#include "Pl/fn_802840DC.h" /* PlMotionRow, pl_act_attack_set, pl_act_motion_row_apply (rule 2) */
#include "ef/eft029_fx.h" /* eft029_set_scale (rule 2) */
#include "ef/fn_800CDB2C.h"
#include "enemy/em_pop.h" /* quest_flag_8_ck, quest_flag_80_ck (the owner's header, rule 2) */
#include "fn_80047398.h" /* `arena_userdata_apply` (rule 2) */
#include "quest/arenatask.h" /* `arena_user_data_buf` (rule 2) */
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "Network/network_pat_control.h" /* the owner's header (rule 2) */
#include "quest/quest_entry.h" /* quest_monsters_release (the owner's header, rule 2) */
#include "mh3_pad/task.h" /* TaskSlot (rule 1) */
#include "mh3_pad/system_w.h" /* system_w (rule 2) */
#include "mh3_pad/lb_param_w.h" /* lb_param_w (rule 2) */
#include "ef/system_core.h" /* work_mem_alloc, system_stream_count_step (rule 2) */
#include "ef/get_move_work_adrs.h" /* get_move_work_adrs, get_move_work_max (rule 2) */
#include "ef/system_work_mem_reset.h"
#include "sound/snd_bank_loader.h"
#include "sound/fn_800D7F54.h" /* se_entry_request, se_work_init (rule 2) */
#include "sound/fn_800F2A94.h" /* bgm_ctrl_init (rule 2) */
#include "enemy/enemy_control.h"
#include "enemy/em_model.h"
#include "draw_shape.h"
#include "ef/eft_res.h"
#include "ef/eft052.h" /* hud_msg_push, hud_item_msg_push (rule 2) */
#include "fn_80040598.h" /* kbd_init (rule 2) */
#include "Runtime.PPCEABI.H/memset.h"
#include "Pl/fn_80273B14.h"
#include "nw_resource.h" /* nwMoveStart, nwMoveEnd, ckResourceName (rule 2) */
#include "sound/fn_800E3CBC.h"
#include "sound/fn_800E46E8.h"
#include "sound/fn_800EF7D8.h"
#include "sound/set_str_bgm_main_vol.h"
#include "sound/set_str_se_main_vol.h"
#include "sound/set_BGM_volume.h"
#include "sound/set_SE_volume.h"
#include "sound/demo_bgm_load.h"
#include "sound/mhchar_res_bank_set.h"
#include "Network/net_session_close.h"
#include "menu/Movie_open.h"
#include "menu/movie.h"
#include "menu/get_pop_dat_ptr.h"
#include "mh3_pad/Psw.h"
#include "fn_80047398.h"
#include "ai/ainpc_init.h"
#include "enemy/fn_8013F764.h"
#include "draw_shape_state_clear.h"
#include "fade_done_ck.h"
#include "ef/loading_disp_flag_set.h"
#include "ef/quest_npc_flags_set.h"
#include "ef/pad_warn_mask_clear.h"
#include "ef/pad_disconnect_notice.h"
#include "sound/snd_banks_loaded_ck.h"
#include "sound/snd_result_banks_load.h"
#include "sound/snd_area_env_bank_load.h"
#include "sound/snd_quest_end_stream_load.h"
#include "sound/bgm_quest_clear_frame.h"
#include "sound/bgm_quest_pending_frame.h"
#include "sound/bgm_ch_fade_out.h"
#include "sound/bgm_main_stop.h"
#include "sound/bgm_mode_jingle_play.h"
#include "sound/bgm_env_ch_stop.h"
#include "sound/bgm_main_fade_out.h"
#include "ef/eft_res_slots_release.h"
#include "ef/eft_instances_release.h"
#include "Pl/pl_player_count_get.h"
#include "Pl/pl_players_ready_ck.h"
#include "Pl/pl_players_draw.h"
#include "enemy/em_prog_area_start.h"
#include "enemy/em_prog_entries_update.h"
#include "light/light_point_all_clear.h"
#include "sound/GetStreamStatus.h"
#include "SetSystemVcnt.h"
#include "ai/ainpc_frame.h"
#include "hud/ainpc_page_idle.h"
#include "hud/ainpc_page_reset.h"
#include "hud/ainpc_page_update.h"
#include "quest/arena_sub_mode_set.h"
#include "camera/camera_update.h"
#include "hud/cockpit_event_notice_spawn.h"
#include "hud/cockpit_notice_hold.h"
#include "hud/cockpit_npc_entries_update.h"
#include "hud/cockpit_split_init.h"
#include "hud/cockpit_frame_update.h"
#include "hud/cockpit_split_frame_update.h"
#include "lobby/demo_sound_update.h"
#include "lobby/demo_work_start.h"
#include "mh3_pad/control.h"
#include "menu/hit_lists_init.h"
#include "menu/hit_registry_clear.h"
#include "menu/hit_registry_update.h"
#include "ef/hud_msg_frame.h"
#include "ef/hud_msg_init.h"
#include "lobby/lb_tutorial_step.h"
#include "menu/multi_box_result_step.h"
#include "menu/q_result_task.h"
#include "quest/quest_byte_table_get.h"
#include "quest_clear_event_flag_set.h"
#include "quest_fin_tex_load.h"
#include "reward_tex_load.h"
#include "rso_overlay_mode1_load.h"
#include "rso_overlay_mode3_load.h"
#include "stage/stage_area_slots_reset.h"
#include "stage/stage_frame_update.h"
#include "menu/option_nop.h"
#include "enemy/enemy_work_release.h"
#include "pl_weapon_motion_res_ids.h"
#include "light/light.h"
#include "enemy/em_prog_work_init.h"
#include "stage/shell.h"
#include "camera/camera_work_init.h"
#include "stage/stg_w.h"
#include "hud/cockpit_quest_init.h"
#include "menu/menu_item.h"
#include "stage_map_set.h"
#include "lobby/lb_quest_screen.h"
#include "Pl/pl_act_step.h"
#include "fn_80056F24.h"
#include "lobby/lb_sub0a_send.h"
#include "main.h" /* get_ScreenSize (rule 2) */
#include "font/flfnt.h"
#include "sound/set_zmode.h"
#include "menu/menu_message.h"
#include "hud/draw_font.h"
#include "hud/net_char_sync.h"
#include "enemy/em_prog_support.h"
#include "quest/quest_item_slot.h"
#include "lobby/event_demo_running_ck.h"
#include "lobby/lb_server_sel_trans.h"
#include "ai/fn_802D44F4.h"
#include "Pl/pl_yure.h"
#include "ai/ainpc_move.h"
#include "pad_hold_ck.h"
#include "MSL_C/printf.h"
#include "menu/menu_result.h"
#include "ef/hbm_enable.h"
#include "VI/vi.h"
#include "gallery_open.h"
#include "userdata_item.h"
#include "pad_connect.h"
#include "sound/snd_stream_bank_reset.h"

/* One stage the quests name: its map and area, and the enemy kinds the quest scene releases when it loads
 * (`{flag, kind}` byte pairs, ended by a 0 flag).  size: 0x8 */
struct QuestStage {
    /* +0x0 */ u8 map_no;
    /* +0x1 */ u8 area_no;
    /* +0x2 */ u8 pad_0x2[0x2];
    /* +0x4 */ u8* release_kinds;
};

/* `player_count_set` takes a `u8` in its owner's header; retail passes the move-work count as a signed byte. */
#define player_count_set_c1 ((void (*)(s8))player_count_set)
/* `mhchar_res_bank_set` takes a `u8` index in its owner's header; retail passes the player loop's `int`. */
/* `stage_map_kind_get` returns a `u8` in its owner's header; retail takes the whole return register. */
#define stage_map_kind_get_c1 ((u32 (*)(u8))stage_map_kind_get)
/* `stage_area_slot_get` returns an `s32` in its owner's header; retail sign-extends a byte. */
#define stage_area_slot_get_c1 ((s8 (*)(u8, u8))stage_area_slot_get)
/* `em_set_result_store` takes an `s8` in its owner's header; retail passes the exit point as a `u8`. */
#define em_set_result_store_c1 ((void (*)(u8))em_set_result_store)
/* `get_option_cfg` returns a `u8` in its owner's header; retail narrows the whole return register. */
#define get_option_cfg_c1 ((u32 (*)(u8))get_option_cfg)
#define mhchar_res_bank_set_c1 ((void (*)(s32, s32, u32*))mhchar_res_bank_set)

/* This unit's own small data: the network error message the dialog prints and its format, and the staff-roll
 * program hooks the final quest's end mode runs (the overlay fills them). */
extern "C" {
u8 quest_stage_release_none[4];
void (*end_prog_load)(void);
void (*end_prog_init)(void);
u32 (*end_prog_main)(void);
void (*end_prog_release)(void);
void (*staff_pc)(void);
u32 (*staff_pc2)(void);
char* net_error_msg;

/* The enemy kinds stage 0's quest scene releases: `{flag, kind}` pairs ended by a 0 flag. */
u8 quest_stage_release_00[8] = {1, 1, 1, 2, 0, 0, 0, 0};
/* The stages, in their `.sdata` order (`quest_stage_table` indexes them). */
QuestStage quest_stage_00 = {0x00, 0, {0, 0}, quest_stage_release_00};
QuestStage quest_stage_01 = {0x01, 1, {0, 0}, quest_stage_release_none};
QuestStage quest_stage_02 = {0x02, 0, {0, 0}, quest_stage_release_none};
QuestStage quest_stage_03 = {0x03, 0, {0, 0}, quest_stage_release_none};
QuestStage quest_stage_04 = {0x04, 0, {0, 0}, quest_stage_release_none};
QuestStage quest_stage_05 = {0x01, 0, {0, 0}, quest_stage_release_none};
QuestStage quest_stage_06 = {0x05, 0, {0, 0}, quest_stage_release_none};
QuestStage quest_stage_07 = {0x06, 0, {0, 0}, quest_stage_release_none};
QuestStage quest_stage_08 = {0x07, 0, {0, 0}, quest_stage_release_none};
QuestStage quest_stage_09 = {0x08, 0, {0, 0}, quest_stage_release_none};
QuestStage quest_stage_10 = {0x09, 0, {0, 0}, quest_stage_release_none};
QuestStage quest_stage_11 = {0x0A, 0, {0, 0}, quest_stage_release_none};
QuestStage quest_stage_13 = {0x0C, 0, {0, 0}, quest_stage_release_none};
QuestStage quest_stage_14 = {0x0D, 0, {0, 0}, quest_stage_release_none};
QuestStage quest_stage_15 = {0x0E, 0, {0, 0}, quest_stage_release_none};
QuestStage quest_stage_16 = {0x0F, 0, {0, 0}, quest_stage_release_none};
QuestStage quest_stage_17 = {0x10, 0, {0, 0}, quest_stage_release_none};
QuestStage quest_stage_18 = {0x11, 0, {0, 0}, quest_stage_release_none};
QuestStage quest_stage_19 = {0x12, 0, {0, 0}, quest_stage_release_none};
QuestStage quest_stage_20 = {0x13, 0, {0, 0}, quest_stage_release_none};
QuestStage quest_stage_12 = {0x0B, 0, {0, 0}, quest_stage_release_none};
QuestStage quest_stage_21 = {0x14, 0, {0, 0}, quest_stage_release_none};
char net_error_code_fmt[] = "%s = %d";
}

/* This unit's own stage tables (see the unit header). */
extern "C" {
extern QuestStage* quest_stage_table[];
extern u8 quest_stage_no_table[];
extern u8 quest_stage_variant_table[];
extern char* pl_motion_res_names[];
}

/* Older declarations this unit still carries for other units' functions. */
extern "C" {




void draw_shape_stage_load(u8 idx);
void eft_common_load(void);

u32 pl_hit_result_best_get(void* self);
void camera_pl_quake_req(void* self, u32 sub);
void em_pop_res_load(u8 idx);

}

/* A vector built through `setVec3` (0x80041E8C), so a file-scope one is constructed by the unit's `__sinit`.
 * size: 0xC */
struct SceneExitPoint : public nw4r::math::VEC3 {
    /* Builds (x, y, z). */
    SceneExitPoint(f32 x, f32 y, f32 z)
    {
        setVec3(this, x, y, z);
    }
};
extern "C" SceneExitPoint scene_exit_point_a;
extern "C" SceneExitPoint scene_exit_point_b;

/* This unit's own `.data` step and row tables and `.sdata2` constants: the object does not emit them yet, so the
 * bodies load the map's symbols (see the unit header). */
extern "C" {
extern PlMotionRow pl_act_class7_handler_04_rows[];
extern PlMotionRow pl_act_class7_handler_08_rows[];
extern PlMotionRow pl_act_class7_handler_11_rows[];
extern PlMotionRow pl_act_class7_handler_13_rows[];
extern PlMotionRow pl_act_class7_handler_06_rows[];
extern PlMotionRow pl_act_class7_handler_10_rows[];
extern PlMotionRow pl_act_class7_handler_15_rows[];
extern PlMotionRow pl_act_class7_handler_17_rows[];
extern PlMotionRow pl_act_class7_handler_19_rows[];
extern u32 pl_act_class7_handler_00_steps[];
extern u32 pl_act_class7_handler_02_steps[];
extern u32 pl_act_class7_handler_06_gauge_steps[];
extern u32 pl_act_class7_handler_06_steps[];
extern u32 pl_act_class7_handler_10_steps[];
extern u32 pl_act_class7_handler_15_gauge_steps[];
extern u32 pl_act_class7_handler_15_steps[];
extern u32 pl_act_class7_handler_17_steps[];
extern u32 pl_act_class7_handler_19_steps[];
extern u32 pl_act_class7_handler_20_steps[];
extern u32 pl_act_class7_handler_23_steps[];
extern u32 pl_act_class7_handler_01_steps[];
extern u32 pl_act_class7_handler_03_steps[];
extern u32 pl_act_class7_handler_04_steps[];
extern u32 pl_act_class7_handler_05_steps[];
extern u32 pl_act_class7_handler_07_steps[];
extern u32 pl_act_class7_handler_08_steps[];
extern u32 pl_act_class7_handler_09_steps[];
extern u32 pl_act_class7_handler_11_steps[];
extern u32 pl_act_class7_handler_12_steps[];
extern u32 pl_act_class7_handler_13_steps[];
extern u32 pl_act_class7_handler_14_steps[];
extern u32 pl_act_class7_handler_16_steps[];
extern u32 pl_act_class7_handler_18_steps[];
extern u32 pl_act_class7_handler_24_steps[];
}

/* ------------------------------------------------------------------------------------------------ *
 * Bodies, in address order.
 * ------------------------------------------------------------------------------------------------ */

/* Every bare `fn_XXXXXXXX` is the map's own (unmangled) name, so the definitions carry C linkage;
 * `get_gm_daynight` is mangled in the map and stays at C++ scope below. */
extern "C" {

/* 0x80288CEC - the player's primary-act update: `act_no` selects the primary-act flag through a jump table,
 * `pl_hit_result_best_get` latches `+0x565` and `Pl_frame_check` arms `+0x566` and hands the player to `fn_802BE568` once. */
void pl_act_class7_frame_update(_PLW* self, u32 sub, f32 dt) {
    u32 primary = 0;
    s32 result;

    if (self->field_0x565 == 0) {
        switch (self->act_no) {
        case 0x10:
        case 0x12:
        case 0x14:
        case 0x17:
        case 0x18:
        case 0x1A:
        case 0x1B:
        case 0x21:
        case 0x23:
        case 0x25:
        case 0x28:
        case 0x29:
        case 0x2B:
        case 0x2C:
        case 0x2F:
        case 0x3A:
            primary = 1;
            break;
        default:
            primary = 0;
            break;
        }

        result = (u8)pl_hit_result_best_get(self);
        switch (result) {
        case 2:
            self->field_0x565 = 1;
            break;
        case 3:
        case 4:
            self->field_0x565 = 1;
            break;
        case 0:
        case 1:
        case 5:
            self->field_0x565 = 1;
            if (primary == 0) {
                if (self->kind_0x09 == 3) {
                    pl_act_enter(self, 4, 0x39, 0x4080);
                } else {
                    pl_act_enter(self, 4, 0xF, 0x4000);
                }
            }
            break;
        }
    }

    if (self->field_0x566 == 0 && self->field_0x565 == 0 && dt > 0.0f &&
        Pl_frame_check(self, 0, dt, 0.0f) == 1) {
        self->field_0x566 = 1;
        camera_pl_quake_req(self, 0);
    }
}

/* 0x80288E58 / 0x80288E60 - the two `sub` bindings of the primary-act update. */
void pl_act_class7_frame_update_0(_PLW* self, f32 dt) {
    pl_act_class7_frame_update(self, 0, dt);
}

void pl_act_class7_frame_update_1(_PLW* self, f32 dt) {
    pl_act_class7_frame_update(self, 1, dt);
}

/* 0x80288E68 (0x2C4): runs the three-stage class-7 act on motion 0x44D: `kind` picks the opening motion, and a
 * `variant` act steps the player 7.5 units forward between frames 40 and 56 of the last stage. */
void pl_act_class7_handler_00(_PLW* self, s32 kind, s32 variant) {
    nw4r::math::VEC3 step;

    VEC3_ctor(&step);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        self->field_0x018 = 1;
        pl_act_clear_flag5bb(self);
        Pl_act_set_motion(self, 0, 1, 0);
        pl_act_set_motion_gate(self, 40);
        switch (kind) {
        case 0:
            self->act_step_0x05++;
            Pl_chr_set_attr_default(self, 0x44D, 4, 10);
            pl_act_attack_set(self, 1, 0);
            Pl_act_set_step_table(self, (u32)pl_act_class7_handler_00_steps, 0);
            break;
        case 1:
            Pl_chr_set_attr_default(self, 0x45A, 2, 0);
            pl_act_timer_step(self);
            break;
        case 2:
            Pl_chr_set_attr_default(self, 0x45B, 2, 0);
            break;
        }
        pl_act_set_frame_timer(self);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            self->act_step_0x05++;
            switch (kind) {
            case 1:
                Pl_chr_set_attr_default(self, 0x44D, 2, 38);
                pl_act_attack_set(self, 1, 0);
                break;
            case 2:
                Pl_chr_set_attr_default(self, 0x44D, 0, 48);
                pl_act_attack_set(self, -1, 0);
                Pl_zanzo_set(self, 9, 3);
                break;
            }
            Pl_act_set_step_table(self, (u32)pl_act_class7_handler_00_steps, 0);
        }
        break;
    case 2:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 0, 4, 0);
        } else {
            if (Pl_frame_check(self, 0, 48.0f, 0.0f) == 1 && kind != 2) {
                Pl_zanzo_set(self, 9, 3);
            }
            pl_act_class7_frame_update_0(self, -1.0f);
            if (Pl_frame_check(self, 3, 40.0f, 56.0f) == 1 && variant != 0) {
                step.x = 0.0f;
                step.y = 0.0f;
                step.z = 7.5f;
                rotVecY(&step, self->field_0x058);
                self->motion_pos_0x3C += step.x;
                self->motion_pos_0x40 += step.y;
                self->motion_pos_0x44 += step.z;
            }
        }
        break;
    }
}

/* 0x8028912C (0xF0): runs the class-7 act whose step table is `pl_act_class7_handler_01_steps`: arms the motion and
 * its attack, then trails an afterimage at frame 18 and hands over at the motion's end. */
void pl_act_class7_handler_01(_PLW* self) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_chr_set_attr_default(self, 0x451, 4, 0);
        pl_act_set_frame_timer(self);
        pl_act_attack_set(self, 2, 0);
        pl_act_set_motion_gate(self, 60);
        Pl_act_set_step_table(self, (u32)pl_act_class7_handler_01_steps, 0);
        break;
    case 1:
        if (Pl_frame_check(self, 0, 18.0f, 0.0f) == 1) {
            Pl_zanzo_set(self, 9, 3);
        }
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 0, 4, 0);
        } else {
            pl_act_class7_frame_update_0(self, -1.0f);
        }
        break;
    }
}

/* 0x8028921C (0x1B8): runs the class-7 act on motion 0x44E; `kind` 1 opens with motion 0x460 first, and the step
 * motion trails an afterimage at frame 48. */
void pl_act_class7_handler_02(_PLW* self, s32 kind) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 0, 1, 0);
        switch (kind) {
        case 0:
            self->act_step_0x05++;
            Pl_chr_set_attr_default(self, 0x44E, 4, 10);
            pl_act_attack_set(self, 4, 0);
            pl_act_set_motion_gate(self, 70);
            Pl_act_set_step_table(self, (u32)pl_act_class7_handler_02_steps, 0);
            break;
        case 1:
            Pl_chr_set_attr_default(self, 0x460, 2, 0);
            pl_act_set_motion_gate(self, 40);
            break;
        }
        pl_act_set_frame_timer(self);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            self->act_step_0x05++;
            Pl_chr_set_attr_default(self, 0x44E, 0, 24);
            pl_act_set_frame_timer(self);
            pl_act_attack_set(self, 4, 0);
            Pl_act_set_step_table(self, (u32)pl_act_class7_handler_02_steps, 0);
        }
        break;
    case 2:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 0, 4, 0);
        } else {
            pl_act_class7_frame_update_0(self, -1.0f);
            if (Pl_frame_check(self, 0, 48.0f, 0.0f) == 1) {
                Pl_zanzo_set(self, 9, 3);
            }
        }
        break;
    }
}

/* 0x802893D4 (0x144): runs the two-motion class-7 act whose step table is `pl_act_class7_handler_03_steps`, trailing
 * an afterimage at frame 22 of the second motion. */
void pl_act_class7_handler_03(_PLW* self) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 0, 1, 0);
        Pl_chr_set_attr_default(self, 0x45C, 2, 0);
        pl_act_set_frame_timer(self);
        pl_act_set_motion_gate(self, 40);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            self->act_step_0x05++;
            Pl_chr_set_attr_default(self, 0x453, 0, 20);
            pl_act_set_frame_timer(self);
            pl_act_attack_set(self, 5, 0);
            Pl_act_set_step_table(self, (u32)pl_act_class7_handler_03_steps, 0);
        }
        break;
    case 2:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 0, 4, 0);
        } else {
            pl_act_class7_frame_update_0(self, -1.0f);
            if (Pl_frame_check(self, 0, 22.0f, 0.0f) == 1) {
                Pl_zanzo_set(self, 5, 3);
            }
        }
        break;
    }
}

/* 0x80289518 (0x13C): runs the class-7 act for row `kind` of `pl_act_class7_handler_04_rows`, trailing an afterimage
 * at frame 22 of the step motion. */
void pl_act_class7_handler_04(_PLW* self, s32 kind) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 0, 1, 0);
        pl_act_motion_row_apply(self, 0, &pl_act_class7_handler_04_rows[kind], (u32)pl_act_class7_handler_04_steps);
        pl_act_set_motion_gate(self, 40);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            self->act_step_0x05++;
            pl_act_motion_row_apply(self, 1, &pl_act_class7_handler_04_rows[kind],
                                    (u32)pl_act_class7_handler_04_steps);
        }
        break;
    case 2:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 0, 4, 0);
        } else {
            pl_act_class7_frame_update_0(self, -1.0f);
            if (Pl_frame_check(self, 0, 22.0f, 0.0f) == 1) {
                Pl_zanzo_set(self, 17, 3);
            }
        }
        break;
    }
}

/* 0x80289654 (0xA4): runs the class-7 act whose step table is `pl_act_class7_handler_05_steps`, handing over at the
 * motion's end. */
void pl_act_class7_handler_05(_PLW* self) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 0, 0, 0);
        Pl_chr_set_attr_default(self, 0x4C5, 0, 0);
        Pl_act_set_step_table(self, (u32)pl_act_class7_handler_05_steps, 0);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 0, 10, 0);
        }
        break;
    }
}

/* 0x802896F8 (0x234): runs the class-7 act for row `kind` of `pl_act_class7_handler_06_rows`; the even rows spend 15
 * gauge, reset the eft029 scales and take the gauge step table. */
void pl_act_class7_handler_06(_PLW* self, s32 kind) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        self->field_0x018 = 1;
        pl_act_clear_flag5bb(self);
        Pl_act_set_motion(self, 0, 1, 0);
        pl_act_set_motion_gate(self, 35);
        if (kind == 0 || kind == 2 || kind == 4) {
            pl_act_add_class7_gauge(self, -15);
            pl_act_motion_row_apply(self, 0, &pl_act_class7_handler_06_rows[kind],
                                    (u32)pl_act_class7_handler_06_gauge_steps);
            eft029_set_scale(self, 11, 1.0f);
            eft029_set_scale(self, 12, 1.0f);
            eft029_set_scale(self, 13, 1.0f);
        } else {
            pl_act_motion_row_apply(self, 0, &pl_act_class7_handler_06_rows[kind], (u32)pl_act_class7_handler_06_steps);
        }
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            self->act_step_0x05++;
            if (kind == 0 || kind == 2 || kind == 4) {
                pl_act_motion_row_apply(self, 1, &pl_act_class7_handler_06_rows[kind],
                                        (u32)pl_act_class7_handler_06_gauge_steps);
            } else {
                pl_act_motion_row_apply(self, 1, &pl_act_class7_handler_06_rows[kind],
                                        (u32)pl_act_class7_handler_06_steps);
            }
        }
        break;
    case 2:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 0, 4, 0);
        } else {
            pl_act_class7_frame_update_0(self, -1.0f);
            if (Pl_frame_check(self, 0, 40.0f, 0.0f) == 1) {
                if (kind == 0 || kind == 2 || kind == 4) {
                    Pl_zanzo_set(self, 6, 4);
                } else {
                    Pl_zanzo_set(self, 6, 3);
                }
            }
        }
        break;
    }
}

/* 0x8028992C (0x134): runs the two-motion class-7 act whose step table is `pl_act_class7_handler_07_steps`, trailing
 * an afterimage at frame 18 of the second motion. */
void pl_act_class7_handler_07(_PLW* self) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 0, 1, 0);
        Pl_chr_set_attr_default(self, 0x463, 2, 0);
        pl_act_set_motion_gate(self, 60);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            self->act_step_0x05++;
            Pl_chr_set_attr_default(self, 0x451, 0, 16);
            pl_act_attack_set(self, 3, 0);
            Pl_act_set_step_table(self, (u32)pl_act_class7_handler_07_steps, 0);
        }
        break;
    case 2:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 0, 4, 0);
        } else {
            if (Pl_frame_check(self, 0, 18.0f, 0.0f) == 1) {
                Pl_zanzo_set(self, 9, 3);
            }
            pl_act_class7_frame_update_0(self, -1.0f);
        }
        break;
    }
}

/* 0x80289A60 (0x178): runs the class-7 act for row `kind` of `pl_act_class7_handler_08_rows`: drains the gauge by 20
 * and resets the three eft029 effect scales, then trails an afterimage at frame 28 of the step motion. */
void pl_act_class7_handler_08(_PLW* self, s32 kind) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 0, 1, 0);
        pl_act_motion_row_apply(self, 0, &pl_act_class7_handler_08_rows[kind], (u32)pl_act_class7_handler_08_steps);
        pl_act_add_class7_gauge(self, -20);
        pl_act_set_motion_gate(self, 60);
        eft029_set_scale(self, 11, 1.0f);
        eft029_set_scale(self, 12, 1.0f);
        eft029_set_scale(self, 13, 1.0f);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            self->act_step_0x05++;
            pl_act_motion_row_apply(self, 1, &pl_act_class7_handler_08_rows[kind],
                                    (u32)pl_act_class7_handler_08_steps);
        }
        break;
    case 2:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 0, 4, 0);
        } else {
            pl_act_class7_frame_update_0(self, -1.0f);
            if (Pl_frame_check(self, 0, 28.0f, 0.0f) == 1) {
                Pl_zanzo_set(self, 8, 4);
            }
        }
        break;
    }
}

/* 0x80289BD8 (0xFC): runs the class-7 act whose step table is `pl_act_class7_handler_09_steps`, trailing an
 * afterimage at frame 22. */
void pl_act_class7_handler_09(_PLW* self) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 0, 1, 0);
        Pl_chr_set_attr_default(self, 0x453, 4, 0);
        pl_act_attack_set(self, 6, 0);
        pl_act_set_motion_gate(self, 60);
        Pl_act_set_step_table(self, (u32)pl_act_class7_handler_09_steps, 0);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 0, 4, 0);
        } else {
            pl_act_class7_frame_update_0(self, -1.0f);
            if (Pl_frame_check(self, 0, 22.0f, 0.0f) == 1) {
                Pl_zanzo_set(self, 5, 3);
            }
        }
        break;
    }
}

/* 0x80289CD4 (0x31C): runs the gauge-spending class-7 act for row `kind` of `pl_act_class7_handler_10_rows`, timing its
 * afterimages, two attacks and the eft004 effects off the step motion's frames. */
void pl_act_class7_handler_10(_PLW* self, s32 kind) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 0, 1, 0);
        pl_act_motion_row_apply(self, 0, &pl_act_class7_handler_10_rows[kind], (u32)pl_act_class7_handler_10_steps);
        pl_act_add_class7_gauge(self, -25);
        pl_act_set_motion_gate(self, 60);
        eft029_set_scale(self, 11, 1.0f);
        eft029_set_scale(self, 12, 1.0f);
        eft029_set_scale(self, 13, 1.0f);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            self->act_step_0x05++;
            pl_act_motion_row_apply(self, 1, &pl_act_class7_handler_10_rows[kind],
                                    (u32)pl_act_class7_handler_10_steps);
        }
        break;
    case 2:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 0, 4, 0);
        } else {
            if (Pl_frame_check(self, 0, 14.0f, 0.0f) == 1) {
                Pl_zanzo_set(self, 6, 4);
            }
            if (Pl_frame_check(self, 0, 34.0f, 0.0f) == 1) {
                Pl_zanzo_set(self, 6, 4);
            }
            if (Pl_frame_check(self, 0, 84.0f, 0.0f) == 1) {
                Pl_zanzo_set(self, 8, 4);
            }
            if (Pl_frame_check(self, 0, 34.0f, 0.0f) == 1) {
                pl_act_attack_set(self, 12, 0);
            }
            if (Pl_frame_check(self, 0, 88.0f, 0.0f) == 1) {
                pl_act_attack_set(self, 13, 0);
            }
            pl_act_class7_frame_update_0(self, -1.0f);
            if (Pl_frame_check(self, 0, 88.0f, 0.0f) == 1) {
                eft004_set_pl2(self, 16, 11, 1.0f, 0.0f, 60.0f, 0);
            }
            if (Pl_frame_check(self, 0, 94.0f, 0.0f) == 1) {
                eft004_set_pl2(self, 11, 11, 1.0f, 0.0f, 160.0f, 0);
                eft004_set_pl2(self, 13, 11, 1.5f, 0.0f, 150.0f, 0);
                eft004_set_pl2(self, 3, 11, 1.5f, 0.0f, 150.0f, 0);
            }
            if (Pl_frame_check(self, 0, 98.0f, 0.0f) == 1) {
                eft004_set_pl2(self, 6, 11, 1.0f, 0.0f, 160.0f, 0);
            }
        }
        break;
    }
}

/* 0x80289FF0 (0x128): runs the class-7 act for row `kind` of `pl_act_class7_handler_11_rows`; row 3 also steps the
 * act timer. */
void pl_act_class7_handler_11(_PLW* self, s32 kind) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 3, 1, 0);
        self->field_0x018 = 1;
        pl_act_motion_row_apply(self, 0, &pl_act_class7_handler_11_rows[kind], (u32)pl_act_class7_handler_11_steps);
        pl_act_set_motion_gate(self, 60);
        if (kind == 3) {
            pl_act_timer_step(self);
        }
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            self->act_step_0x05++;
            pl_act_motion_row_apply(self, 1, &pl_act_class7_handler_11_rows[kind],
                                    (u32)pl_act_class7_handler_11_steps);
        }
        break;
    case 2:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 3, 4, 0);
        } else {
            pl_act_class7_frame_update_0(self, -1.0f);
        }
        break;
    }
}

/* 0x8028A118 (0xD0): runs the class-7 act whose step table is `pl_act_class7_handler_12_steps`. */
void pl_act_class7_handler_12(_PLW* self) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 3, 1, 0);
        Pl_chr_set_attr_default(self, 0x482, 4, 0);
        pl_act_attack_set(self, 15, 0);
        pl_act_set_motion_gate(self, 60);
        Pl_act_set_step_table(self, (u32)pl_act_class7_handler_12_steps, 0);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 3, 4, 0);
        } else {
            pl_act_class7_frame_update_0(self, -1.0f);
        }
        break;
    }
}

/* 0x8028A1E8 (0x110): runs the class-7 act for row `kind` of `pl_act_class7_handler_13_rows`. */
void pl_act_class7_handler_13(_PLW* self, s32 kind) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 3, 1, 0);
        pl_act_motion_row_apply(self, 0, &pl_act_class7_handler_13_rows[kind], (u32)pl_act_class7_handler_13_steps);
        pl_act_set_motion_gate(self, 60);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            self->act_step_0x05++;
            pl_act_motion_row_apply(self, 1, &pl_act_class7_handler_13_rows[kind],
                                    (u32)pl_act_class7_handler_13_steps);
        }
        break;
    case 2:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 3, 4, 0);
        } else {
            pl_act_class7_frame_update_0(self, -1.0f);
        }
        break;
    }
}

/* 0x8028A2F8 (0x108): runs the two-motion class-7 act whose step table is `pl_act_class7_handler_14_steps`. */
void pl_act_class7_handler_14(_PLW* self) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 3, 1, 0);
        Pl_chr_set_attr_default(self, 0x48A, 4, 0);
        pl_act_set_motion_gate(self, 60);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            self->act_step_0x05++;
            Pl_chr_set_attr_default(self, 0x484, 0, 10);
            pl_act_attack_set(self, 17, 0);
            Pl_act_set_step_table(self, (u32)pl_act_class7_handler_14_steps, 0);
        }
        break;
    case 2:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 3, 4, 0);
        } else {
            pl_act_class7_frame_update_0(self, -1.0f);
        }
        break;
    }
}

/* 0x8028A400 (0x22C): runs the class-7 act for row `kind` of `pl_act_class7_handler_15_rows`; the even rows reset the
 * eft029 scales, spend 15 gauge and take the gauge step table. */
void pl_act_class7_handler_15(_PLW* self, s32 kind) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        self->field_0x018 = 1;
        Pl_act_set_motion(self, 3, 1, 0);
        pl_act_set_motion_gate(self, 35);
        if (kind == 0 || kind == 2 || kind == 4) {
            eft029_set_scale(self, 11, 1.0f);
            eft029_set_scale(self, 12, 1.0f);
            eft029_set_scale(self, 13, 1.0f);
            pl_act_add_class7_gauge(self, -15);
            pl_act_motion_row_apply(self, 0, &pl_act_class7_handler_15_rows[kind],
                                    (u32)pl_act_class7_handler_15_gauge_steps);
        } else {
            pl_act_motion_row_apply(self, 0, &pl_act_class7_handler_15_rows[kind], (u32)pl_act_class7_handler_15_steps);
        }
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            self->act_step_0x05++;
            if (kind == 0 || kind == 2 || kind == 4) {
                pl_act_motion_row_apply(self, 1, &pl_act_class7_handler_15_rows[kind],
                                        (u32)pl_act_class7_handler_15_gauge_steps);
            } else {
                pl_act_motion_row_apply(self, 1, &pl_act_class7_handler_15_rows[kind],
                                        (u32)pl_act_class7_handler_15_steps);
            }
        }
        break;
    case 2:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 3, 4, 0);
        } else {
            pl_act_class7_frame_update_0(self, -1.0f);
            if (Pl_frame_check(self, 0, 40.0f, 0.0f) == 1) {
                if (kind == 0 || kind == 2 || kind == 4) {
                    Pl_zanzo_set(self, 6, 4);
                } else {
                    Pl_zanzo_set(self, 6, 3);
                }
            }
        }
        break;
    }
}

/* 0x8028A62C (0x134): runs the two-motion class-7 act whose step table is `pl_act_class7_handler_16_steps`, trailing
 * an afterimage at frame 18 of the second motion. */
void pl_act_class7_handler_16(_PLW* self) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 3, 1, 0);
        Pl_chr_set_attr_default(self, 0x491, 4, 0);
        pl_act_set_motion_gate(self, 60);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            self->act_step_0x05++;
            Pl_chr_set_attr_default(self, 0x482, 0, 16);
            pl_act_attack_set(self, 20, 0);
            Pl_act_set_step_table(self, (u32)pl_act_class7_handler_16_steps, 0);
        }
        break;
    case 2:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 3, 4, 0);
        } else {
            if (Pl_frame_check(self, 0, 18.0f, 0.0f) == 1) {
                Pl_zanzo_set(self, 9, 3);
            }
            pl_act_class7_frame_update_0(self, -1.0f);
        }
        break;
    }
}

/* 0x8028A760 (0x178): runs the gauge-spending class-7 act for row `kind` of `pl_act_class7_handler_17_rows`, trailing
 * an afterimage at frame 28 of the step motion. */
void pl_act_class7_handler_17(_PLW* self, s32 kind) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 3, 1, 0);
        pl_act_motion_row_apply(self, 0, &pl_act_class7_handler_17_rows[kind], (u32)pl_act_class7_handler_17_steps);
        pl_act_add_class7_gauge(self, -20);
        pl_act_set_motion_gate(self, 60);
        eft029_set_scale(self, 11, 1.0f);
        eft029_set_scale(self, 12, 1.0f);
        eft029_set_scale(self, 13, 1.0f);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            self->act_step_0x05++;
            pl_act_motion_row_apply(self, 1, &pl_act_class7_handler_17_rows[kind],
                                    (u32)pl_act_class7_handler_17_steps);
        }
        break;
    case 2:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 3, 4, 0);
        } else {
            pl_act_class7_frame_update_0(self, -1.0f);
            if (Pl_frame_check(self, 0, 28.0f, 0.0f) == 1) {
                Pl_zanzo_set(self, 8, 4);
            }
        }
        break;
    }
}

/* 0x8028A8D8 (0xFC): runs the class-7 act whose step table is `pl_act_class7_handler_18_steps`, trailing an
 * afterimage at frame 22. */
void pl_act_class7_handler_18(_PLW* self) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 3, 1, 0);
        Pl_chr_set_attr_default(self, 0x484, 4, 0);
        pl_act_attack_set(self, 22, 0);
        Pl_act_set_step_table(self, (u32)pl_act_class7_handler_18_steps, 0);
        pl_act_set_motion_gate(self, 60);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 3, 4, 0);
        } else {
            pl_act_class7_frame_update_0(self, -1.0f);
            if (Pl_frame_check(self, 0, 22.0f, 0.0f) == 1) {
                Pl_zanzo_set(self, 5, 3);
            }
        }
        break;
    }
}

/* 0x8028A9D4 (0x240): runs the gauge-spending class-7 act for row `kind` of `pl_act_class7_handler_19_rows`, arming
 * two attacks and three afterimages off the step motion's frames. */
void pl_act_class7_handler_19(_PLW* self, s32 kind) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 3, 1, 0);
        pl_act_motion_row_apply(self, 0, &pl_act_class7_handler_19_rows[kind], (u32)pl_act_class7_handler_19_steps);
        pl_act_add_class7_gauge(self, -25);
        if (kind == 0) {
            pl_act_set_motion_gate(self, 60);
        } else {
            pl_act_set_motion_gate(self, 66);
        }
        eft029_set_scale(self, 11, 1.0f);
        eft029_set_scale(self, 12, 1.0f);
        eft029_set_scale(self, 13, 1.0f);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            self->act_step_0x05++;
            pl_act_motion_row_apply(self, 1, &pl_act_class7_handler_19_rows[kind],
                                    (u32)pl_act_class7_handler_19_steps);
        }
        break;
    case 2:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 3, 4, 0);
        } else {
            if (Pl_frame_check(self, 0, 46.0f, 0.0f) == 1) {
                pl_act_attack_set(self, 24, 0);
            }
            if (Pl_frame_check(self, 0, 96.0f, 0.0f) == 1) {
                pl_act_attack_set(self, 25, 0);
            }
            if (Pl_frame_check(self, 0, 14.0f, 0.0f) == 1) {
                Pl_zanzo_set(self, 6, 4);
            }
            if (Pl_frame_check(self, 0, 34.0f, 0.0f) == 1) {
                Pl_zanzo_set(self, 6, 4);
            }
            if (Pl_frame_check(self, 0, 84.0f, 0.0f) == 1) {
                Pl_zanzo_set(self, 8, 4);
            }
            pl_act_class7_frame_update_0(self, -1.0f);
        }
        break;
    }
}

/* 0x8028AC14 (0x1A4): runs the turning class-7 act on motion 0x469 (`kind` 0) or 0x468: between frames 38 and 68 it
 * turns the player by 0x16C and slides it 10 units sideways along its rotation. */
void pl_act_class7_handler_20(_PLW* self, s32 kind) {
    nw4r::math::VEC3 slide;

    VEC3_ctor(&slide);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 0, 1, 0);
        if (kind == 0) {
            Pl_chr_set_attr_default(self, 0x469, 4, 0);
            pl_act_attack_set(self, 27, 0);
        } else {
            Pl_chr_set_attr_default(self, 0x468, 4, 0);
            pl_act_attack_set(self, 26, 0);
        }
        pl_act_set_motion_gate(self, 60);
        Pl_act_set_step_table(self, (u32)pl_act_class7_handler_20_steps, 0);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 0, 4, 0);
        } else {
            pl_act_class7_frame_update_0(self, -1.0f);
            if (Pl_frame_check(self, 3, 38.0f, 68.0f) == 1) {
                if (kind == 0) {
                    self->field_0x058 -= 0x16C;
                } else {
                    self->field_0x058 += 0x16C;
                }
                self->field_0x0A8 = self->field_0x058;
                slide.x = 10.0f;
                if (kind == 1) {
                    slide.x *= -1.0f;
                }
                slide.y = 0.0f;
                slide.z = 0.0f;
                rotVecXYZ(&slide, &self->rot_0x54);
                addVec3To(&self->vec_0x03C, &slide);
            }
        }
        break;
    }
}

/* 0x8028ADB8 (0x100): runs the gauge-spending class-7 act on motion 0x467 (attack 30, gauge -100) and enters act 0x30
 * at frame 128. */
void pl_act_class7_handler_21(_PLW* self) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 0, 1, 0);
        Pl_chr_set_attr_default(self, 0x467, 4, 0);
        pl_act_attack_set(self, 30, 2);
        pl_act_set_motion_gate(self, 60);
        pl_act_add_class7_gauge(self, -100);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 0, 4, 0);
        } else {
            pl_act_class7_frame_update_1(self, -1.0f);
            if (Pl_frame_check(self, 0, 128.0f, 0.0f) == 1) {
                self->field_0x018 = 0;
                pl_act_enter(self, 4, 0x30, 2);
            }
        }
        break;
    }
}

/* 0x8028AEB8 (0x98): runs the one-motion class-7 act on motion 0x467, handing over at the motion's end. */
void pl_act_class7_handler_22(_PLW* self) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 0, 1, 0);
        Pl_chr_set_attr_default(self, 0x467, 0, 0x80);
        self->field_0x018 = 0;
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 0, 4, 0);
        }
        break;
    }
}

/* 0x8028AF50 (0x1A4): runs the turning class-7 act on motion 0x497 (`kind` 0) or 0x496: between frames 38 and 68 it
 * turns the player by 0x16C and slides it 10 units sideways along its rotation. */
void pl_act_class7_handler_23(_PLW* self, s32 kind) {
    nw4r::math::VEC3 slide;

    VEC3_ctor(&slide);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 3, 1, 0);
        if (kind == 0) {
            Pl_chr_set_attr_default(self, 0x497, 4, 0);
            pl_act_attack_set(self, 29, 0);
        } else {
            Pl_chr_set_attr_default(self, 0x496, 4, 0);
            pl_act_attack_set(self, 28, 0);
        }
        pl_act_set_motion_gate(self, 60);
        Pl_act_set_step_table(self, (u32)pl_act_class7_handler_23_steps, 0);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 3, 4, 0);
        } else {
            pl_act_class7_frame_update_0(self, -1.0f);
            if (Pl_frame_check(self, 3, 38.0f, 68.0f) == 1) {
                if (kind == 0) {
                    self->field_0x058 -= 0x16C;
                } else {
                    self->field_0x058 += 0x16C;
                }
                self->field_0x0A8 = self->field_0x058;
                slide.x = 10.0f;
                if (kind == 1) {
                    slide.x *= -1.0f;
                }
                slide.y = 0.0f;
                slide.z = 0.0f;
                rotVecXYZ(&slide, &self->rot_0x54);
                addVec3To(&self->vec_0x03C, &slide);
            }
        }
        break;
    }
}

/* 0x8028B0F4 (0xA4): runs the class-7 act whose step table is `pl_act_class7_handler_24_steps`, handing over at the
 * motion's end. */
void pl_act_class7_handler_24(_PLW* self) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 3, 0, 0);
        Pl_chr_set_attr_default(self, 0x4F7, 0, 0);
        Pl_act_set_step_table(self, (u32)pl_act_class7_handler_24_steps, 0);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 3, 10, 0);
        }
        break;
    }
}

/* 0x8028B198 (0x100): runs the gauge-spending class-7 act on motion 0x495 (attack 31, gauge -100) and enters act 0x3B
 * at frame 128. */
void pl_act_class7_handler_25(_PLW* self) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 3, 1, 0);
        Pl_chr_set_attr_default(self, 0x495, 4, 0);
        pl_act_attack_set(self, 31, 2);
        pl_act_set_motion_gate(self, 60);
        pl_act_add_class7_gauge(self, -100);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 3, 4, 0);
        } else {
            pl_act_class7_frame_update_1(self, -1.0f);
            if (Pl_frame_check(self, 0, 128.0f, 0.0f) == 1) {
                self->field_0x018 = 0;
                pl_act_enter(self, 4, 0x3B, 2);
            }
        }
        break;
    }
}

/* 0x8028B298 (0x98): runs the one-motion class-7 act on motion 0x495, handing over at the motion's end. */
void pl_act_class7_handler_26(_PLW* self) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 3, 1, 0);
        Pl_chr_set_attr_default(self, 0x495, 0, 0x80);
        self->field_0x018 = 0;
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 3, 4, 0);
        }
        break;
    }
}

/* 0x8028B330 (0x1F4): runs the class-7 act handler `act_no` selects, with the variant the act number implies; an
 * act number past the table hands the motion slot over. */
void pl_act_class7_dispatch(_PLW* self) {
    switch (self->act_no) {
    case 0:
        pl_act_class7_handler_00(self, 0, 0);
        break;
    case 1:
        pl_act_class7_handler_00(self, 0, 1);
        break;
    case 2:
        pl_act_class7_handler_00(self, 1, 0);
        break;
    case 3:
        pl_act_class7_handler_00(self, 2, 0);
        break;
    case 4:
        pl_act_class7_handler_01(self);
        break;
    case 5:
        pl_act_class7_handler_02(self, 0);
        break;
    case 6:
        pl_act_class7_handler_02(self, 1);
        break;
    case 7:
        pl_act_class7_handler_03(self);
        break;
    case 8:
        pl_act_class7_handler_04(self, 0);
        break;
    case 9:
        pl_act_class7_handler_04(self, 1);
        break;
    case 10:
        pl_act_class7_handler_04(self, 2);
        break;
    case 11:
        pl_act_class7_handler_04(self, 3);
        break;
    case 12:
        pl_act_class7_handler_04(self, 4);
        break;
    case 13:
        pl_act_class7_handler_04(self, 5);
        break;
    case 14:
        pl_act_class7_handler_04(self, 6);
        break;
    case 15:
        pl_act_class7_handler_05(self);
        break;
    case 16:
        pl_act_class7_handler_06(self, 0);
        break;
    case 17:
        pl_act_class7_handler_06(self, 1);
        break;
    case 18:
        pl_act_class7_handler_06(self, 2);
        break;
    case 19:
        pl_act_class7_handler_06(self, 3);
        break;
    case 20:
        pl_act_class7_handler_06(self, 4);
        break;
    case 21:
        pl_act_class7_handler_06(self, 5);
        break;
    case 22:
        pl_act_class7_handler_07(self);
        break;
    case 23:
        pl_act_class7_handler_08(self, 0);
        break;
    case 24:
        pl_act_class7_handler_08(self, 1);
        break;
    case 25:
        pl_act_class7_handler_09(self);
        break;
    case 26:
        pl_act_class7_handler_10(self, 0);
        break;
    case 27:
        pl_act_class7_handler_10(self, 1);
        break;
    case 28:
        pl_act_class7_handler_11(self, 0);
        break;
    case 29:
        pl_act_class7_handler_12(self);
        break;
    case 30:
        pl_act_class7_handler_13(self, 0);
        break;
    case 31:
        pl_act_class7_handler_14(self);
        break;
    case 32:
        pl_act_class7_handler_11(self, 1);
        break;
    case 33:
        pl_act_class7_handler_15(self, 0);
        break;
    case 34:
        pl_act_class7_handler_15(self, 1);
        break;
    case 35:
        pl_act_class7_handler_15(self, 2);
        break;
    case 36:
        pl_act_class7_handler_15(self, 3);
        break;
    case 37:
        pl_act_class7_handler_15(self, 4);
        break;
    case 38:
        pl_act_class7_handler_15(self, 5);
        break;
    case 39:
        pl_act_class7_handler_16(self);
        break;
    case 40:
        pl_act_class7_handler_17(self, 0);
        break;
    case 41:
        pl_act_class7_handler_17(self, 1);
        break;
    case 42:
        pl_act_class7_handler_18(self);
        break;
    case 43:
        pl_act_class7_handler_19(self, 0);
        break;
    case 44:
        pl_act_class7_handler_19(self, 1);
        break;
    case 45:
        pl_act_class7_handler_20(self, 0);
        break;
    case 46:
        pl_act_class7_handler_20(self, 1);
        break;
    case 47:
        pl_act_class7_handler_21(self);
        break;
    case 48:
        pl_act_class7_handler_22(self);
        break;
    case 49:
        pl_act_class7_handler_13(self, 1);
        break;
    case 50:
        pl_act_class7_handler_13(self, 2);
        break;
    case 51:
        pl_act_class7_handler_13(self, 3);
        break;
    case 52:
        pl_act_class7_handler_13(self, 4);
        break;
    case 53:
        pl_act_class7_handler_13(self, 5);
        break;
    case 54:
        pl_act_class7_handler_11(self, 2);
        break;
    case 55:
        pl_act_class7_handler_23(self, 0);
        break;
    case 56:
        pl_act_class7_handler_23(self, 1);
        break;
    case 57:
        pl_act_class7_handler_24(self);
        break;
    case 58:
        pl_act_class7_handler_25(self);
        break;
    case 59:
        pl_act_class7_handler_26(self);
        break;
    case 60:
        pl_act_class7_handler_11(self, 3);
        break;
    default:
        Pl_act_set_motion_slot(self, 0, 4, 0);
        break;
    }
}

/* 0x8028B524 (0x584): builds the scene root work for the quest game mode: the move works, the counters, the stage
 * the quest names, the quest work and the SE handles; `kind` 2 asks for a retry, the others set the enemy level. */
void quest_scene_work_init(TaskSlot* task, u8 kind) {
    _PL_ROOT* root;
    s32 work_kind;
    s32 players;
    QuestStage* stage;
    s32 i;

    option_nop();
    system_w.field_0x2d = 0;
    if (isServerSelectState() == 1) {
        allocateDialogRecord();
    }
    system_work_mem_reset();
    if (kind == 0 || kind == 2) {
        if (isServerSelectState() == 1) {
            work_kind = 0;
        } else if (PlayMode_ck() == 6) {
            work_kind = 7;
        } else {
            work_kind = 8;
        }
    } else if (PlayMode_ck() == 3 || PlayMode_ck() == 6) {
        work_kind = 4;
    } else if (isServerSelectState() == 1) {
        work_kind = 0;
    } else {
        work_kind = PlayMode_ck() == 6 ? 7 : 8;
    }
    create_move_work(work_kind);
    if (PlayMode_ck() == 3 || PlayMode_ck() == 6) {
        if (system_w.field_0x7d3 == 0) {
            players = 1;
        } else if (isServerSelectState() == 1) {
            players = 4;
        } else {
            players = 1;
        }
    } else {
        players = 2;
    }
    set_move_work_max(2, players);
    set_max_player(players);
    system_w.field_0x28 = players;
    root = (_PL_ROOT*)get_move_work_adrs(0);
    root->area_slots_0xE0[0] = -1;
    root->area_slots_0xE0[1] = -1;
    root->field_0x225C[0] = -1;
    root->field_0x225C[1] = -1;
    root->field_0x225C[2] = -1;
    root->field_0x225C[3] = -1;
    root->field_0x225C[4] = -1;
    root->field_0x225C[5] = -1;
    root->spawn_work_0x2254 = work_mem_alloc(0x1200);
    for (i = 0; i < 0x20; i++) {
        root->counters_0x18[i] = 0;
    }
    for (i = 0; i < 0x10; i++) {
        root->counters_0x9C[i] = 0;
    }
    if (kind == 2) {
        root->retry_0x22E4 = 1;
        kind = 0;
    } else {
        root->retry_0x22E4 = 0;
    }
    quest_snd_wk_init();
    enemy_control_init();
    if (PlayMode_ck() != 3 && PlayMode_ck() != 6) {
        root->field_0x22DB = kind;
        root->field_0x22DB = kind;
        em_level_set(kind);
    } else if ((system_w.leave_state_0x7d8 & 0xFF000000) != 0) {
        u8 stage_no = quest_stage_no_table[system_w.leave_state_0x7d8 & 0xFFFFFF];
        if (stage_no == 8 || stage_no == 7 || stage_no == 11 || stage_no == 18 || stage_no == 19) {
            root->field_0x22DB = 4;
            em_level_set(4);
        } else {
            root->field_0x22DB = kind;
            em_level_set(kind);
        }
    } else if (system_w.leave_state_0x7d8 == 0) {
        root->field_0x22DB = 1;
        em_level_set(1);
    }
    stage_area_slots_reset();
    em_pop_work_alloc();
    hud_msg_init();
    if (lb_param_w.field_0x00 == 2001) {
        quest_npc_flags_set();
    }
    root->npc_present_0xEF = system_w.field_0x7cc;
    root->field_0x00F0 = system_w.field_0x7cd;
    root->stage_variant_0x144 = 0;
    if ((system_w.leave_state_0x7d8 & 0xFF000000) != 0) {
        root->stage_no_0x120 = quest_stage_no_table[system_w.leave_state_0x7d8 & 0xFFFFFF];
        root->stage_variant_0x144 = quest_stage_variant_table[system_w.leave_state_0x7d8 & 0xFFFFFF];
    } else {
        root->stage_no_0x120 = 1;
    }
    stage = quest_stage_table[root->stage_no_0x120];
    root->area_no_0xE9 = stage->map_no;
    root->area_no_0xEA = stage->area_no;
    root->area_no_0xED = stage->map_no;
    root->field_0x11C = -1;
    root->map_no_0x124 = root->area_no_0xE9;
    if (system_w.field_0x7d3 == 0) {
        my_player_no_set(0);
        player_count_set_c1(get_move_work_max(2));
    }
    if (PlayMode_ck() == 3 || PlayMode_ck() == 6 || PlayMode_ck() == 2) {
        root->mode_work_0xDC = (u8*)quest_init(0);
    } else {
        root->mode_work_0xDC = (u8*)work_mem_alloc(0x6AB8);
        memset(root->mode_work_0xDC, 0, 0x6AB8);
    }
    eft_res_pool_reset();
    se_work_init();
    bgm_ctrl_init();
    root->se_handles_0x138[0] = (u8*)se_entry_request(5, (_ENEMY_WORK*)root, 0);
    root->se_handles_0x138[1] = (u8*)se_entry_request(6, (_ENEMY_WORK*)root, 0);
    root->se_handles_0x138[2] = (u8*)se_entry_request(7, (_ENEMY_WORK*)root, 0);
    gpframe_tex_load();
    menu_tex_load();
    itemicon_tex_load();
    root->unlock_seen_0x148[0] = system_w.unlock_seen_0x888[0];
    root->unlock_seen_0x148[1] = system_w.unlock_seen_0x888[1];
    if (isServerSelectState() == 1) {
        root->server_count_0x22E2 = countOccupiedServerSlots();
        for (i = 0; i < 4; i++) {
            if ((u32)NetCtrlWk::hasServerIndex((u8)i) == 1) {
                root->server_slots_0x22DD[i] = 1;
            }
        }
        root->lowest_server_0x22E1 = NetCtrlWk::getLowestServerIndex();
    }
    pad_warn_mask_clear();
    if (isServerSelectState() == 1) {
        kbd_init(0);
    }
}

/* 0x8028BAA8 (0x24C): builds the scene root work for the versus game mode: one or two players, the counters, the
 * quest work and the SE handles. */
void vs_scene_work_init(TaskSlot* task) {
    _PL_ROOT* root;
    s32 players;
    s32 i;

    for (i = 0; i < 4; i++) {
        system_w.field_0x18[i] = 0x4D48;
    }
    system_work_mem_reset();
    create_move_work(0);
    players = system_w.field_0x8af == 0 ? 1 : 2;
    set_move_work_max(2, players);
    set_max_player(players);
    root = (_PL_ROOT*)get_move_work_adrs(0);
    root->area_slots_0xE0[0] = -1;
    root->area_slots_0xE0[1] = -1;
    root->field_0x225C[0] = -1;
    root->field_0x225C[1] = -1;
    root->field_0x225C[2] = -1;
    root->field_0x225C[3] = -1;
    root->field_0x225C[4] = -1;
    root->field_0x225C[5] = -1;
    root->spawn_work_0x2254 = work_mem_alloc(0x1200);
    for (i = 0; i < 0x20; i++) {
        root->counters_0x18[i] = 0;
    }
    for (i = 0; i < 0x10; i++) {
        root->counters_0x9C[i] = 0;
    }
    enemy_control_init();
    quest_snd_wk_init();
    stage_area_slots_reset();
    em_pop_work_alloc();
    hud_msg_init();
    my_player_no_set(0);
    player_count_set_c1(get_move_work_max(2));
    if (PlayMode_ck() == 3 || PlayMode_ck() == 6 || PlayMode_ck() == 2) {
        root->mode_work_0xDC = (u8*)quest_init(1);
    }
    eft_res_pool_reset();
    se_work_init();
    bgm_ctrl_init();
    root->se_handles_0x138[0] = (u8*)se_entry_request(5, (_ENEMY_WORK*)root, 0);
    root->se_handles_0x138[1] = (u8*)se_entry_request(6, (_ENEMY_WORK*)root, 0);
    root->se_handles_0x138[2] = (u8*)se_entry_request(7, (_ENEMY_WORK*)root, 0);
    gpframe_tex_load();
    menu_tex_load();
    itemicon_tex_load();
    root->unlock_seen_0x148[0] = system_w.unlock_seen_0x888[0];
    root->unlock_seen_0x148[1] = system_w.unlock_seen_0x888[1];
    pad_warn_mask_clear();
}

/* 0x8028BCF4 (0x60): releases the root work's three SE handles. */
void scene_se_handles_release(_PL_ROOT* root) {
    if (root == 0) {
        return;
    }
    if (root->se_handles_0x138[0] != 0) {
        se_handle_clear(root->se_handles_0x138[0]);
    }
    if (root->se_handles_0x138[1] != 0) {
        se_handle_clear(root->se_handles_0x138[1]);
    }
    if (root->se_handles_0x138[2] != 0) {
        se_handle_clear(root->se_handles_0x138[2]);
    }
}

/* 0x8028BD54 - is the root work in mode 2? */
u32 root_mode2_ck(void) {
    _PL_ROOT* root = (_PL_ROOT*)get_move_work_adrs(0);
    if (root == 0) {
        return 0;
    }
    return root->mode_0x00 == 2;
}

/* 0x8028BD98 (0x184): handles player `player` dropping out of the network session: puts its actor into the
 * collapse act, then moves the lowest live server index on and clears the local player's +0x659 latch. */
void net_player_drop_step(_PL_ROOT* root, u8 player) {
    _PLW* players = (_PLW*)get_move_work_adrs(2);
    u8 old_lowest;
    s32 slot;

    if (players == 0) {
        return;
    }
    if (Pl_motion_input_ck(2) == 0) {
        if (Pl_act_ck(&players[player], 9, 5) == 0 && Pl_act_ck(&players[player], 9, 9) == 0 &&
            players[player].field_0x584 == 0 && players[player].field_0x267 != 0) {
            if (player != (s8)my_player_no()) {
                hud_item_msg_push(3, 0x30, player);
            }
            if (players[player].kind_0x09 != 3) {
                pl_act_enter_down(&players[player], 9, 5, 2);
            } else {
                pl_act_enter_down(&players[player], 9, 9, 2);
            }
        }
    }
    countOccupiedServerSlots();
    old_lowest = root->lowest_server_0x22E1;
    for (slot = 0; slot < 4; slot++) {
        if (root->server_slots_0x22DD[slot] == 1) {
            root->lowest_server_0x22E1 = (u8)slot;
            if (old_lowest != (u8)slot && players[(s8)my_player_no()].field_0x659 != 0) {
                players[(s8)my_player_no()].field_0x659 = 0;
            }
            break;
        }
    }
}

/* 0x8028BF1C (0x24C): the quest game mode's task: hands dropped network players to `net_player_drop_step`, builds the scene
 * on its first step and then runs the root work's mode, leaving for the next game mode from mode 4. */
void quest_game_task(TaskSlot* task) {
    _PL_ROOT* root = (_PL_ROOT*)get_move_work_adrs(0);
    s32 i;

    system_stream_count_step();
    if (root != 0) {
        nwMoveStart();
    }
    if (isServerSelectState() == 1 && root != 0 &&
        ((u8)(root->mode_0x00 + 0xFE) <= 1 || (root->mode_0x00 == 1 && root->seq_0x01 >= 3))) {
        NetCtrlWk::flushSession();
        for (i = 0; i < 4; i++) {
            if (root->server_slots_0x22DD[i] == 4 && i != (s8)my_player_no()) {
                net_player_drop_step(root, (u8)i);
            }
        }
    }
    switch (task->step_0x08) {
    case 0:
        rso_overlay_mode1_load();
        GameMode_set(1);
        quest_scene_work_init(task, 0);
        task->step_0x08++;
        break;
    case 1:
        prim_init_all();
        switch (root->mode_0x00) {
        case 0:
            quest_scene_boot(root);
            return;
        case 1:
            quest_scene_load(root);
            break;
        case 2:
            quest_scene_main(root);
            break;
        case 3:
            quest_scene_end(root);
            break;
        case 4:
            if (root->field_0x22DA == 0) {
                system_w.field_0x7d5 = 0;
            }
            system_w.leave_flag_0x7d6 = 1;
            system_w.leave_state_0x7d8 = 0;
            system_w.net_session_0x90f = 0;
            sound_frame_entry();
            bgm_stop_all();
            all_reset();
            scene_se_handles_release(root);
            if (root->field_0x22DA == 1) {
                GameMode_set(2);
                PlayMode_set(4);
                GameModeExec();
            } else if (PlayMode_ck() == 3) {
                GameMode_set(2);
                PlayMode_set(5);
                GameModeExec();
            } else {
                arena_task_start();
            }
            return;
        }
        break;
    }
    nwMoveEnd();
    if (root != 0 && (root->mode_0x00 == 2 || (root->mode_0x00 == 3 && root->field_0x22D7 == 0))) {
        se_frame_step();
        bgm_quest_clear_frame();
    }
}

/* 0x8028C168 (0x300): the quest scene's boot mode: in a network session it seeds the player number and count and
 * plays the opening movie for the first quests, then checks the link and moves on to the load mode. */
void quest_scene_boot(_PL_ROOT* root) {
    switch (root->seq_0x01) {
    case 0:
        if (isServerSelectState() == 0) {
            break;
        }
        if (system_w.net_session_0x90f == 0) {
            s8 server = NetCtrlWk::getSelectedServer();
            s8 count;
            if (server < 0) {
                my_player_no_set(0);
            } else {
                my_player_no_set(server);
            }
            count = countOccupiedServerSlots();
            if (count <= 0) {
                player_count_set_c1(1);
            } else {
                player_count_set_c1(count);
            }
            net_session_close_start();
        }
        if (lb_param_w.field_0x00 == 14002 || lb_param_w.field_0x00 == 16000 || lb_param_w.field_0x00 == 16001) {
            loading_disp_set(0, 0);
            set_str_bgm_main_vol(0.0f);
            set_str_se_main_vol(0.0f);
            demo_bgm_load(39);
            root->seq_0x01++;
            return;
        }
        break;
    case 1:
        if (file_loading_ck(0, 0) != 1 && srt_ready_ck(0) != 0) {
            Movie_open((MovieIndex)10);
            PlayStream(0, 40);
            root->seq_0x01++;
        }
        return;
    case 2:
        if ((u32)(GetStreamStatus(0) - 1) <= 1) {
            PlayStream(0, 2);
            root->seq_0x01++;
        }
        return;
    case 3:
        if (file_loading_ck(0, 0) != 1) {
            if (GetStreamStatus(0) != 3) {
                PlayStream(0, 2);
            } else {
                PlayStream(0, 3);
                set_BGM_volume(get_option_cfg(17));
                set_SE_volume(get_option_cfg(18));
                root->seq_0x01++;
            }
        }
        return;
    case 4:
        if (Check_movie_finish() == 1) {
            Movie_close();
        } else if ((Psw[0].mode == 2 && (Psw[0].field_0xda & 0x8000) != 0 && (u32)Movie_cut() == 1) ||
                   (Psw[0].mode == 1 && (Psw[0].field_0x100 & 0x40) != 0 && (u32)Movie_cut() == 1)) {
        } else {
            subTransSet((u32)Movie_draw_sub, 0, 0);
            return;
        }
        unlock_bit_raise(39);
        bgm_stop_all();
        break;
    case 5:
        break;
    default:
        return;
    }
    if (getLinkStatus() == 1) {
        root->end_kind_0xFA = 8;
        net_session_abort_start();
        pad_input_clear(0);
        root->mode_0x00 = 3;
        root->seq_0x01 = 2;
        root->kind_0x02 = 2;
        root->timer_0x04 = 0;
        root->field_0x22DA = 1;
        system_w.field_0x7d5 = 1;
    } else {
        root->mode_0x00 = 1;
        root->seq_0x01 = 0;
    }
}

/* 0x8028C468 (0x108): loads the quest scene's resources: the player data, the AI NPC, the common effects, the area
 * the quest starts in, and releases the stage's enemy kinds (or the quest's monsters). */
void quest_scene_res_load(_PL_ROOT* root, u8 kind) {
    u8* release = 0;
    LbParamWork* param = &lb_param_w;

    if (param->field_0x00 == 0) {
        release = quest_stage_table[root->stage_no_0x120]->release_kinds;
    }
    player_init_data_load();
    if (root->npc_present_0xEF != 0) {
        ainpc_res_load();
    }
    eft_common_load();
    if (kind == 1) {
        root->area_no_0xEA = param->start_area_0x9A;
    } else {
        root->area_no_0xEA = quest_bonus_pick();
    }
    if (PlayMode_ck() != 3 && PlayMode_ck() != 6) {
        if (release != 0) {
            while (release[0] != 0) {
                em_kind_release(release[1]);
                release += 2;
            }
        }
    } else {
        quest_monsters_release();
    }
    em_pop_res_load(root->area_no_0xED);
    draw_shape_stage_load(root->area_no_0xED);
}

/* 0x8028C570 - the mode object's per-scene player data init. */
void vs_scene_res_load(_PL_ROOT* self) {
    player_init_data_load();
    eft_common_load();
    quest_monsters_release();
    em_pop_res_load(self->area_no_0xED);
    draw_shape_stage_load(self->area_no_0xED);
}
} /* extern "C" */

/* 0x8028C5B4 (0x118): loads the players' motion banks: the common bank the player kind names, then each player's
 * weapon bank. */
void pl_motion_set(void) {
    u32 file[1];
    u32 bank[1];
    u32 weapon[1];
    u8 kind = pl_motion_bank_no_get();
    ResEntry* entry;
    s32 i;
    s32 count;
    _PLW* plw;

    res_file_ctor(file, 0);
    entry = ckResourceName(pl_motion_res_names[kind]);
    if (entry != 0) {
        res_file_assign(file, res_file_ctor(bank, (u32)entry->data));
        mhchar_res_bank_set(0, 0, file);
    }
    plw = (_PLW*)get_move_work_adrs(2);
    count = pl_player_count_get();
    for (i = 0; i < count; i++, plw++) {
        entry = ckResourceName((char*)pl_weapon_motion_res_ids[plw->field_0x002].b_0x04);
        if (entry != 0) {
            res_file_assign(file, res_file_ctor(weapon, (u32)entry->data));
            mhchar_res_bank_set_c1(1, i, file);
        }
    }
}
extern "C" {

/* 0x8028C6CC (0x5B0): the quest scene's load mode: the scene resources, the motion banks, the subsystem inits, the
 * stage and its first area, then (in a network session) the wait for every player before the scene starts. */
void quest_scene_load(_PL_ROOT* root) {
    s32 i;
    u8 seq;

    loading_disp_set(1, 1);
    if (PlayMode_ck() == 3 || PlayMode_ck() == 6) {
    }
    switch (seq = root->seq_0x01) {
    case 0:
        root->loading_0x22E5 = 1;
        quest_snd_wk_init();
        quest_start_load();
        init_player_work();
        enemy_work_reset();
        quest_scene_res_load(root, 0);
        root->seq_0x01++;
        break;
    case 1:
        if (file_loading_ck(0, 0) != 1) {
            pl_motion_set();
            root->seq_0x01++;
        }
        break;
    case 2:
        light_init();
        if (root->npc_present_0xEF != 0) {
            ainpc_init();
        }
        em_prog_work_init();
        em_prog_slots_init();
        eft_control_init();
        shell_work_init();
        camera_work_init();
        stage_work_init();
        cockpit_quest_init();
        menu_item_work_init();
        sound_frame_entry();
        bgm_stop_all();
        root->seq_0x01++;
        break;
    case 3:
        if (player_move_start_ck() != 0) {
            player_move_start(-1);
            if (PlayMode_ck() == 3 || PlayMode_ck() == 6) {
                quest_enter_load();
            } else {
                root->end_kind_0xFA = 2;
            }
            stage_map_set(root->area_no_0xED);
            stage_map_res_load(root->area_no_0xED);
            stage_map_obj_load(root->area_no_0xED);
            stage_area_load(0, root->area_no_0xED, root->area_no_0xEA);
            demo_work_start();
            if (system_w.field_0x7d1 != 1) {
                quest_area_res_load(root->area_no_0xED, root->area_no_0xEA);
            }
            root->seq_0x01++;
        }
        break;
    case 4:
        if (file_loading_ck(0, 0) != 1) {
            root->timer_0x04 = 0;
            root->seq_0x01++;
            stage_area_enter(root->area_no_0xED, root->area_no_0xEA);
            copyVec3(&root->pos_0x128, stage_entry_pos_get(root->area_no_0xED, root->area_no_0xEA));
            root->rot_y_0x134 = stage_entry_angle_get(root->area_no_0xED, root->area_no_0xEA);
            pl_area_entry_set(&root->pos_0x128, root->rot_y_0x134, root->area_no_0xED, root->area_no_0xEA);
            light_point_all_clear();
            camera_area_reset();
            quest_screen_enemy_start((Q_MoveWork*)root);
            if (system_w.field_0x7d1 != 1) {
                em_prog_area_start();
            }
            quest_work_pouch_load();
            em_pop_work_step();
            hud_msg_frame();
            root->area_slots_0xE0[root->area_slot_index_0xE8] = stage_area_slot_get_c1(root->area_no_0xED, root->area_no_0xEA);
            snd_stage_bank_load(root->area_no_0xE9, root->area_no_0xEA, root->field_0x22D8);
            snd_player_banks_load();
            root->kind_0x02 = 0;
        }
        break;
    case 5:
        switch (root->kind_0x02) {
        case 0:
            if (file_loading_ck(0, 0) == 1 || snd_banks_loaded_ck() == 0) {
                break;
            }
            root->kind_0x02++;
        case 1:
            fade_set(0, 2);
            if (pl_players_ready_ck() != 0) {
                if (isServerSelectState() == 1) {
                    root->timer_0x04 = 0;
                    root->timer_0x06 = 0;
                    root->kind_0x02++;
                } else {
                    root->seq_0x01++;
                    root->kind_0x02 = 0;
                }
            }
            break;
        case 2:
            fade_set(0, 2);
            if (isServerSelectState() == 1) {
                if (getLinkStatus() == 1 || system_w.net_result_wait_0x8c1 == 1) {
                    loading_disp_set(0, 1);
                    root->end_kind_0xFA = 8;
                    if (getLinkStatus() == 1) {
                        net_session_abort_start();
                    }
                    pad_input_clear(0);
                    root->mode_0x00 = 3;
                    root->seq_0x01 = 2;
                    root->kind_0x02 = 2;
                    root->timer_0x04 = 0;
                    root->field_0x22DA = 1;
                    system_w.field_0x7d5 = 1;
                } else if (isSessionStartDone() != 0) {
                    if (isReadyCountOne() == 1 && root->timer_0x04 == 0 && (u32)lb_seen_pad_ck(3) == 1) {
                        root->timer_0x04 = 1;
                        lb_sub0b_send(NetCtrlWk::getSelectedServer(), 3);
                    }
                    if ((++root->timer_0x06 & 0x1F) == 0) {
                        lb_sub0a_send(NetCtrlWk::getSelectedServer(), 3);
                    }
                    if (lb_handled_ck(3) != 0) {
                        root->seq_0x01++;
                        root->kind_0x02 = 0;
                        root->timer_0x04 = 0;
                        for (i = 0; i < 4; i++) {
                            if ((u32)NetCtrlWk::hasServerIndex((u8)i) == 1) {
                                root->server_slots_0x22DD[i] = 1;
                            }
                        }
                        root->lowest_server_0x22E1 = NetCtrlWk::getLowestServerIndex();
                    }
                }
            }
            break;
        case 3:
            fade_set(0, 2);
            root->seq_0x01++;
            root->kind_0x02 = 0;
            root->timer_0x04 = 0;
            break;
        }
        break;
    case 6:
        switch ((u32)root->kind_0x02) {
        case 0:
        case 1:
            demo_sound_update();
            root->kind_0x02++;
            break;
        case 2:
            root->seq_0x01 = seq + 1;
            break;
        }
        break;
    case 7:
        system_w.field_0x7d5 = 1;
        if (isServerSelectState() == 1) {
            root->field_0x22DA = 1;
            system_w.hbm_disabled = 1;
            setTransferMode(1);
        } else {
            root->field_0x22DA = 0;
            system_w.hbm_disabled = 0;
        }
        root->loading_0x22E5 = 0;
        root->mode_0x00 = 2;
        root->seq_0x01 = 0;
        root->kind_0x02 = 0;
        GlareFilter_on();
        filter_panel_on();
        root->timer_0x04 = 30;
        fade_set(0, 2);
        loading_disp_set(0, 0);
        movie_change_request();
        kbd_reset_call();
        return;
    }
    player_control_move();
}

/* 0x8028CC7C (0xC0): steps every actor and subsystem for one frame unless the scene is frozen or the pad is held,
 * then updates the HUD and the hit registry. */
void scene_actors_move(_PL_ROOT* root) {
    if (root->frozen_0x112 != 0) {
        return;
    }
    if (pad_hold_ck() != 1) {
        demo_sound_update();
        hit_registry_clear();
        player_control_move();
        enemy_control_move();
        ainpc_frame();
        em_prog_entries_update();
        shell_area_release(1);
        hit_lists_init();
        pl_players_draw();
        enemy_control_post_move();
        ainpc_move();
        yure_move();
        stage_frame_update();
        camera_update();
        light_move();
        shell_area_release(0);
        eft_instances_release();
        em_pop_work_step();
        hud_msg_frame();
        if (root->scene_kind_0x113 == 1 && quest_move_sub_state_4_get() == 1) {
            return;
        }
    }
    scene_hud_update();
    if (pad_hold_ck() != 1) {
        hit_registry_update();
    }
}

/* 0x8028CD3C (0x74): updates the HUD and menus for the screen layout: the NPC entries while player 1 is in the menu,
 * otherwise the single-screen or split-screen cockpit. */
void scene_hud_update(void) {
    ai_npc_reaction_forward();
    if (Pl_motion_input_ck(1) != 0) {
        cockpit_npc_entries_update();
    } else if (screen_split_mode_ck() == 0) {
        ainpc_page_idle();
        ainpc_page_reset();
        system_w.menu_closed_0x8c0 = 0;
        menu_work_update();
        cockpit_frame_update();
        ainpc_page_update();
    } else {
        menu_split_update_all();
        cockpit_split_frame_update();
    }
}

/* 0x8028CDB0 (0x33C): the quest scene's main mode: the quest steps and the end check (which hands over to the end
 * mode with the result message), then the frame or an area change. */
void quest_scene_main(_PL_ROOT* root) {
    get_rcSwitch_on(1);
    if (fade_done_ck(0) != 0) {
        if (root->field_0x22DA == 1 && (getLinkStatus() == 1 || system_w.net_result_wait_0x8c1 == 1)) {
            root->end_kind_0xFA = 8;
        }
        if (PlayMode_ck() == 3 || PlayMode_ck() == 6) {
            if (quest_flag_1000000_ck(0) == 1 && root->frozen_0x112 == 0) {
                if (root->flag_0x114 <= 0) {
                    lb_tutorial_step();
                } else {
                    root->flag_0x114--;
                }
                quest_intro_step((Q_MoveWork*)root);
            }
            if (root->end_kind_0xFA != 8) {
                quest_main_step();
            }
            if (pad_hold_ck() == 0) {
                switch (root->main_step_0x111) {
                case 0:
                    if (quest_sub_state_end_ck(1) == 0) {
                        if (quest_select_ready_ck() == 1) {
                            scene_exit_point_ck(root);
                        }
                    } else {
                        root->main_step_0x111++;
                    }
                    break;
                case 1:
                    if (Pl_motion_input_ck(2) != 0) {
                        pad_input_clear(0);
                        root->mode_0x00 = 3;
                        root->seq_0x01 = 0;
                        root->timer_0x04 = 0;
                        cockpit_notice_hold();
                        scene_actors_move(root);
                        menu_item_close();
                        if (quest_select_ready_ck() == 0) {
                            switch (root->end_kind_0xFA) {
                            case 4:
                                hud_msg_push(0, quest_str_tbl_35_get(13));
                                break;
                            case 6:
                                hud_msg_push(0, quest_str_tbl_35_get(14));
                                break;
                            case 7:
                                hud_msg_push(0, quest_str_tbl_35_get(15));
                                break;
                            case 8:
                                hud_msg_push(0, quest_str_tbl_35_get(36));
                                break;
                            }
                        }
                        if (root->field_0x22DA == 1) {
                            switch (root->end_kind_0xFA) {
                            case 7:
                                net_session_abort_start();
                                break;
                            case 8:
                                if (getLinkStatus() == 1) {
                                    net_session_abort_start();
                                }
                                break;
                            }
                        }
                        return;
                    }
                    break;
                }
            }
        }
    }
    switch (root->seq_0x01) {
    case 0:
        if (root->timer_0x04 > 0) {
            root->timer_0x04--;
            pad_input_clear(0);
        }
        scene_actors_move(root);
        pad_disconnect_notice(0);
        if (root->warp_req_0x22E3 != 0) {
            root->warp_req_0x22E3 = 0;
            quest_entry_point_warp();
        }
        if (quest_select_ready_ck() == 0 && (u8)event_demo_running_ck() == 0) {
            snd_bgm_hold_set();
        }
        if ((u8)event_demo_running_ck() != 0) {
            u8 demo = get_demo_no();
            if ((u8)(demo + 242) <= 4 || demo == 39 || demo == 43) {
                snd_bgm_hold_set();
            }
        }
        break;
    case 1:
        if (PlayMode_ck() == 3 || PlayMode_ck() == 6) {
            fade_set(0, 2);
        }
        scene_area_change_step(0);
        break;
    }
}

/* 0x8028D0EC (0x1F0): starts a player warp to `pos` facing `angle`: in a quest it hands the destination to the
 * network and arms the area change; in the versus mode it toggles the player's alternate entry point, moves the
 * player at once and fades its screen. */
void pl_warp_start(u8 mode, nw4r::math::VEC3* pos, u16 angle) {
    _PL_ROOT* root = (_PL_ROOT*)get_move_work_adrs(0);
    nw4r::math::VEC3 point;
    u32 point_angle;

    VEC3_ctor(&point);
    if (root->end_kind_0xFA == 2 || root->end_kind_0xFA == 3) {
        bgm_main_fade_out();
        if (isServerSelectState() == 1) {
            setTransferMode(0);
        }
        if (PlayMode_ck() != 2) {
            root->area_no_0xEB = mode;
            copyVec3(&root->pos_0x128, pos);
            root->rot_y_0x134 = angle;
            root->seq_0x01 = 1;
            root->kind_0x02 = 0;
            Pl_net_send_hit_kind3(root->area_no_0xEB, &root->pos_0x128, root->rot_y_0x134);
        } else {
            if (mode == 0xFF) {
                root->bytes_0x136[(s8)my_player_no()] = 0;
                mode = 0;
            } else {
                root->bytes_0x136[(s8)my_player_no()] ^= 1;
            }
            if (root->bytes_0x136[(s8)my_player_no()] == 1) {
                stage_area_point_get(mode, root->bytes_0x136[(s8)my_player_no()], &point, &point_angle);
                root->area_no_0xEB = mode;
                copyVec3(&root->pos_0x128, &point);
                root->rot_y_0x134 = point_angle;
            } else {
                root->area_no_0xEB = mode;
                copyVec3(&root->pos_0x128, pos);
                root->rot_y_0x134 = angle;
            }
            pl_area_entry_set(&root->pos_0x128, root->rot_y_0x134, root->area_no_0xED, root->area_no_0xEA);
            camera_area_reset();
            root->seq_0x01 = 0;
            root->kind_0x02 = 0;
            root->hold_timers_0x04[(s8)my_player_no() & 1] = 10;
            fade_set((s8)my_player_no() + 1, 2);
            bgm_quest_pending_frame();
            snd_player_mask_clear(my_player_no());
        }
        system_w.menu_closed_0x8c0 = 0;
    }
}

/* 0x8028D2DC (0x298): steps an area change: releases the old area's effects and slot, loads the destination area
 * into a free slot, enters it and resumes the frame. */
void scene_area_change_step(u8 kind) {
    _PL_ROOT* root = (_PL_ROOT*)get_move_work_adrs(0);
    s8 slot;
    u8 map;

    system_w.menu_closed_0x8c0 = 0;
    kbd_reset_call();
    switch (root->kind_0x02) {
    case 0:
        eft_res_release_all();
        draw_shape_state_clear();
        filter_flag_clear();
        root->kind_0x02++;
        root->frozen_0x112 = 1;
        sound_frame_entry();
        bgm_main_stop();
        loading_disp_flag_set(1);
        break;
    case 1:
        stage_area_slot_release(root->area_slots_0xE0[root->area_slot_index_0xE8]);
        root->area_slots_0xE0[root->area_slot_index_0xE8] = -1;
        enemy_work_store_all();
        eft_instances_release();
        em_prog_support_reset();
        root->kind_0x02++;
        break;
    case 2:
        slot = stage_area_free_slot_get();
        if (slot >= 0) {
            root->area_slots_0xE0[root->area_slot_index_0xE8] = slot;
            root->prev_area_no_0xEC = root->area_no_0xEA;
            root->area_no_0xEA = root->area_no_0xEB;
            stage_area_load((u8)slot, root->area_no_0xED, root->area_no_0xEA);
            if (move_work_state_ck() == 0 && system_w.field_0x7d1 != 1) {
                map = quest_byte_table_get(root->area_no_0xE9);
            } else {
                map = root->area_no_0xE9;
            }
            quest_area_spawn_apply((Q_MoveWork*)root, map, root->area_no_0xEB);
            if (system_w.field_0x7d1 != 1) {
                quest_area_res_load(root->area_no_0xED, root->area_no_0xEB);
            }
            root->kind_0x02++;
        }
        break;
    case 3:
        if (file_loading_ck(0, 0) != 1) {
            stage_area_enter(root->area_no_0xED, root->area_no_0xEA);
            pl_area_entry_set(&root->pos_0x128, root->rot_y_0x134, root->area_no_0xED, root->area_no_0xEA);
            light_point_all_clear();
            camera_area_reset();
            snd_area_bank_load(root->area_no_0xE9, root->area_no_0xEA);
            snd_area_env_bank_load(root->area_no_0xE9, root->area_no_0xEA, root->field_0x22D8);
            em_area_entries_release();
            if (system_w.field_0x7d1 != 1) {
                em_prog_area_start();
            }
            bgm_quest_pending_frame();
            root->kind_0x02++;
            root->sub_step_0x03 = 0;
        }
        break;
    case 4:
        if (isServerSelectState() == 1) {
            setTransferMode(1);
        }
        switch ((u32)root->sub_step_0x03) {
        case 0:
        case 1:
            demo_sound_update();
            root->sub_step_0x03++;
            break;
        case 2:
            loading_disp_flag_set(0);
            root->frozen_0x112 = 0;
            root->timer_0x04 = 10;
            root->flag_0x114 = 30;
            root->seq_0x01 = 0;
            root->kind_0x02 = 0;
            root->sub_step_0x03 = 0;
            break;
        }
        break;
    }
}

/* 0x8028D574 - the `flag_0x114 == 1` predicate, 1 when the work is absent. */
u32 quest_move_idle_ck(void) {
    _PL_ROOT* root = (_PL_ROOT*)get_move_work_adrs(0);
    if (root == 0) {
        return 1;
    }
    return root->flag_0x114 <= 0;
}

/* 0x8028D5B8 - store the flag. */
void root_wait_frames_set(s32 value) {
    _PL_ROOT* root = (_PL_ROOT*)get_move_work_adrs(0);
    if (root != 0) {
        root->flag_0x114 = value;
    }
}

/* 0x8028D5F4 (0x184): draws the network error dialog: the frame, the message, the confirm button and the session
 * error code. */
void net_error_dialog_draw(void) {
    _MH_VEC2 screen;
    char text[32];
    s32 width = 590;
    s32 x;
    s32 code;

    get_ScreenSize(&screen);
    x = 0.5f * (screen.x - width);
    set_zmode(false, 0, false);
    set_blendmode(4, 5, 1);
    put_frame_dialog(x, 40, width, 400, 0, 0x14120AF0);
    MH3DispErrorString(x + 24, 68, (s8*)net_error_msg);
    x += 526;
    if (get_option_cfg(7) != 0) {
        put_button_icon(x, 376, 32, -1, 2, 0);
    } else {
        put_button_icon(x, 376, 32, -1, 0, 0);
    }
    flfntSetSize(22, 22);
    flfntSetColor(-1);
    flfntSetPos((s16)(0.5f * (screen.x - 220.0f)), 360);
    code = MH3GetSessionErrorCode();
    sprintf(text, net_error_code_fmt, MH3GetErrorString2(51), code);
    flfntPrintf(text);
}

/* 0x8028D778 (0x654): the quest scene's end mode: fills the result, shows the end message, tears the quest down and
 * then runs the result screen, the network close and error dialog, or the staff roll after the final quest. */
void quest_scene_end(_PL_ROOT* root) {
    switch (root->seq_0x01) {
    case 0:
        if (root->field_0x22DA == 1) {
            NetCtrlWk::setFetchMode(0);
        }
        if (root->end_kind_0xFA == 8) {
            root->timer_0x04 = 150;
            root->seq_0x01 = 2;
            quest_result_fill((Q_MoveWork*)root);
        } else {
            if (root->timer_0x04 == 60) {
                quest_result_fill((Q_MoveWork*)root);
            }
            if (root->timer_0x04 > 150) {
                quest_fin_tex_load(root->end_kind_0xFA);
                snd_quest_end_stream_load();
                root->seq_0x01++;
            } else {
                root->timer_0x04++;
            }
        }
        scene_actors_move(root);
        break;
    case 1:
        if (file_loading_ck(0, 0) == 1) {
            scene_actors_move(root);
            break;
        }
        if (quest_select_ready_ck() == 1 || PlayMode_ck() != 3) {
            subTransReset();
            root->mode_0x00 = 4;
            root->seq_0x01 = 0;
            break;
        }
        cockpit_event_notice_spawn(root->end_kind_0xFA);
        bgm_mode_jingle_play(root->end_kind_0xFA);
        root->timer_0x04 = 150;
        root->seq_0x01++;
    case 2:
        if (root->timer_0x04 > 0) {
            root->timer_0x04--;
        }
        switch (root->kind_0x02) {
        case 0:
            bgm_ch_fade_out(3);
            root->kind_0x02++;
        case 1:
            if (root->timer_0x04 < 20) {
                fade_set(0, 1);
                root->kind_0x02++;
                root->timer_0x06 = 0;
            }
            scene_actors_move(root);
            break;
        case 2:
            if (root->timer_0x04 > 0) {
                scene_actors_move(root);
                break;
            }
            switch (root->end_kind_0xFA) {
            case 7:
                bgm_env_ch_stop();
                break;
            case 8:
                bgm_stop_all();
                break;
            default:
                if (root->field_0x22DA == 1) {
                    requestLeaveOrAbort();
                }
                bgm_env_ch_stop();
                break;
            }
            root->field_0x22D7 = 1;
            sound_frame_entry();
            eft_res_release_all();
            draw_shape_state_clear();
            filter_flag_clear();
            subTransReset();
            filter_reset();
            fade_reset(0);
            em_kind_res_release(pl_map_kind_get());
            stage_area_slot_release(root->area_slots_0xE0[root->area_slot_index_0xE8]);
            root->area_slots_0xE0[root->area_slot_index_0xE8] = -1;
            em_kind_slot_release_all();
            em_res_buffers_free();
            eft_res_slots_release();
            if (root->end_kind_0xFA == 8) {
                root->kind_0x02 = 100;
                root->timer_0x04 = 60;
                loading_disp_set(1, 0);
                break;
            }
            quest_clear_event_flag_set();
            if (root->stage_no_0x120 >= 1000 && root->mode_work_0xDC != 0) {
                if (root->end_kind_0xFA == 4 && root->stage_no_0x120 == 2005) {
                    bgm_stop_all();
                    root->kind_0x02 = 200;
                    (staff_pc = end_prog_load)();
                    movie_bgm_load();
                    break;
                }
                reward_tex_load();
                snd_result_banks_load();
                q_result_work_clear((Q_MoveWork*)root);
                root->timer_0x04 = 120;
                root->kind_0x02++;
                break;
            }
            if (root->field_0x22DA == 1) {
                root->kind_0x02 = 99;
                break;
            }
            se_work_init();
            root->mode_0x00 = 4;
            root->seq_0x01 = 0;
            break;
        case 3:
            if (q_result_task((Q_MoveWork*)root) == 0) {
                break;
            }
            quest_result_work_flush();
            root->kind_0x02 = 99;
        case 99:
            if (net_session_close_state_get() != 0) {
                se_work_init();
                root->mode_0x00 = 4;
                root->seq_0x01 = 0;
            }
            break;
        case 100:
            if (net_session_close_state_get() == 0) {
                break;
            }
            if (root->timer_0x04 > 0) {
                root->timer_0x04--;
                break;
            }
            se_work_init();
            root->kind_0x02++;
            root->timer_0x04 = 60;
        case 101:
            if (system_w.net_result_wait_0x8c1 != 1 || NetCtrlWk::getLobbyReadyByte() == 2) {
                if ((net_error_msg = get_network_sub_error_msg()) != 0) {
                    subTransSetPrio(9, (u32)net_error_dialog_draw, 0, 0);
                    loading_disp_set(0, 0);
                }
                if (root->timer_0x04 > 0) {
                    root->timer_0x04--;
                    break;
                }
                if ((Psw[0].button_0x2C0.pressed_0x04 & 0x10) != 0) {
                    sysSE_req(0);
                    root->timer_0x04 = 20;
                    root->kind_0x02++;
                    loading_disp_set(0, 0);
                }
            }
            break;
        case 102:
            if ((net_error_msg = get_network_sub_error_msg()) != 0) {
                subTransSetPrio(9, (u32)net_error_dialog_draw, 0, 0);
            }
            if (root->timer_0x04 > 0) {
                root->timer_0x04--;
                break;
            }
            root->mode_0x00 = 4;
            root->seq_0x01 = 0;
            NetCtrlWk::clearSubError();
            break;
        case 200:
            if (file_loading_ck(0, 0) != 1) {
                (staff_pc = end_prog_init)();
                root->kind_0x02++;
            }
            break;
        case 201:
            if (srt_ready_ck(0) != 0) {
                Movie_open((MovieIndex)3);
                PlayStream(0, 13);
                root->kind_0x02++;
            }
            break;
        case 202:
            if (Check_movie_finish() == 1) {
                Movie_close();
                hbm_disable();
                VIEnableDimming(0);
                root->kind_0x02++;
                SetSystemVcnt(1);
            } else {
                subTransSet((u32)Movie_draw_sub, 0, 0);
            }
            break;
        case 203:
            if ((staff_pc2 = end_prog_main)() == 1) {
                hbm_enable();
                VIEnableDimming(1);
                gallery_open(4);
                bgm_stop_all();
                SetSystemVcnt(2);
                root->kind_0x02++;
            }
            break;
        case 204:
            reward_tex_load();
            snd_result_banks_load();
            q_result_work_clear((Q_MoveWork*)root);
            root->timer_0x04 = 120;
            root->kind_0x02 = 3;
            break;
        }
        break;
    }
}

/* 0x8028DDCC (0x158): the field game mode's task: builds the scene on its first step and then runs the root work's
 * mode, leaving for the next game mode from mode 4. */
void field_game_task(TaskSlot* task) {
    _PL_ROOT* root = (_PL_ROOT*)get_move_work_adrs(0);

    system_stream_count_step();
    nwMoveStart();
    switch (task->step_0x08) {
    case 0:
        rso_overlay_mode1_load();
        GameMode_set(1);
        quest_scene_work_init(task, 1);
        task->step_0x08++;
        break;
    case 1:
        prim_init_all();
        switch (root->mode_0x00) {
        case 0:
            field_scene_boot(root);
            break;
        case 1:
            field_scene_load(root);
            break;
        case 2:
            field_scene_main(root);
            break;
        case 3:
            field_scene_end(root);
            break;
        case 4:
            system_w.field_0x7d5 = 0;
            system_w.leave_state_0x7d8 = 0;
            sound_frame_entry();
            bgm_stop_all();
            all_reset();
            scene_se_handles_release(root);
            GameMode_set(2);
            PlayMode_set(5);
            system_w.field_0x2d = 1;
            GameModeExec();
            return;
        }
        break;
    }
    nwMoveEnd();
    if (root != 0 && (u8)(root->mode_0x00 + 0xFE) <= 1) {
        se_frame_step();
        bgm_quest_clear_frame();
    }
}

/* 0x8028DF24 (0x34): the field scene's boot mode: moves straight on to the load mode. */
void field_scene_boot(_PL_ROOT* root) {
    switch (root->seq_0x01) {
    case 0:
        root->seq_0x01++;
    case 1:
        root->mode_0x00 = 1;
        root->seq_0x01 = 0;
        break;
    }
}

/* 0x8028DF58 (0x160): the field scene's load mode: starts the free-hunt monster set, loads the motion banks, sets up
 * the field and enters its first area. */
void field_scene_load(_PL_ROOT* root) {
    loading_disp_set(1, 0);
    PlayMode_ck();
    switch (root->seq_0x01) {
    case 0:
        root->loading_0x22E5 = 1;
        root->scene_kind_0x113 = 1;
        root->seq_0x01++;
    case 1:
        root->seq_0x01++;
    case 2:
        root->seq_0x01++;
        break;
    case 3:
        em_set_start();
        root->seq_0x01++;
        break;
    case 4:
        if (file_loading_ck(0, 0) != 1) {
            root->seq_0x01++;
        }
        break;
    case 5:
        pl_motion_set();
        em_set_field_setup((Q_MoveWork*)root);
        demo_work_start();
        root->seq_0x01++;
        break;
    case 6:
        if ((u32)em_set_move_start((Q_MoveWork*)root) == 1) {
            root->seq_0x01++;
        }
        break;
    case 7:
        if ((u32)em_set_area_enter((Q_MoveWork*)root) == 1) {
            root->seq_0x01++;
        }
        break;
    case 8:
        root->loading_0x22E5 = 0;
        root->mode_0x00 = 2;
        root->seq_0x01 = 0;
        root->kind_0x02 = 0;
        root->timer_0x04 = 0;
        loading_disp_set(0, 0);
        fade_set(0, 2);
        return;
    }
    player_control_move();
}

/* 0x8028E0B8 (0x194): the field scene's main mode: the hunt steps, then the frame; leaving through an exit point
 * stores the free-hunt result and hands over to the end mode, an area change runs `scene_area_change_step`. */
void field_scene_main(_PL_ROOT* root) {
    u8 exit_point;

    get_rcSwitch_on(1);
    if (root->frozen_0x112 == 0 && quest_select_ready_ck() == 1) {
        if (root->flag_0x114 <= 0) {
            lb_tutorial_step();
        } else {
            root->flag_0x114--;
        }
        quest_intro_step((Q_MoveWork*)root);
    }
    if (pad_hold_ck() == 0) {
        em_set_frame_step((Q_MoveWork*)root);
    }
    switch (root->seq_0x01) {
    case 0:
        if (root->timer_0x04 > 0) {
            root->timer_0x04--;
            pad_input_clear(0);
        }
        scene_actors_move(root);
        pad_disconnect_notice(0);
        if (pad_hold_ck() != 0) {
            return;
        }
        if (quest_move_sub_state_4_get() == 1) {
            u8 flag = root->field_0x22D9;
            exit_point = 1;
            if (flag != 0) {
                exit_point = flag;
            }
            se_slot_req(6);
            break;
        } else {
            scene_exit_point_ck(root);
            return;
        }
    case 1:
        if (PlayMode_ck() == 3) {
            fade_set(0, 2);
        }
        scene_area_change_step(1);
        return;
    default:
        return;
    }
    bgm_stop_all();
    root->bgm_volume_0x22E6 = get_option_cfg_c1(17);
    root->se_volume_0x22E7 = get_option_cfg_c1(18);
    set_str_bgm_main_vol(0.0f);
    set_str_se_main_vol(0.0f);
    pad_input_clear(0);
    root->mode_0x00 = 3;
    root->seq_0x01 = 0;
    root->timer_0x04 = 150;
    em_set_result_store_c1(exit_point);
}

/* 0x8028E24C (0x1A0): the field scene's end mode: fades the BGM and SE volumes and the screen out, then restarts the
 * sound engine and leaves for mode 4. */
void field_scene_end(_PL_ROOT* root) {
    pad_input_clear(0);
    switch (root->seq_0x01) {
    case 0:
        root->seq_0x01++;
    case 1:
        if (root->timer_0x04 > 0) {
            root->timer_0x04--;
        }
        scene_actors_move(root);
        if (root->timer_0x04 < 30) {
            if (root->bgm_volume_0x22E6 != 0) {
                set_BGM_volume(--root->bgm_volume_0x22E6);
            }
            if (root->se_volume_0x22E7 != 0) {
                set_SE_volume(--root->se_volume_0x22E7);
            }
            set_stream_main_vol_flag(0, 1);
            set_stream_main_vol_flag(1, 0);
            set_stream_main_vol_flag(2, 0);
        }
        if (root->timer_0x04 < 20) {
            set_str_bgm_main_vol(0.0f);
            set_str_se_main_vol(0.0f);
            fade_set(0, 1);
            bgm_stop_all();
            root->seq_0x01++;
        }
        break;
    case 2:
        if (root->timer_0x04 > 0) {
            root->timer_0x04--;
            scene_actors_move(root);
            break;
        }
        resumeSoundEngine();
        snd_stream_bank_reset();
        se_work_init();
        bgm_ctrl_init();
        subTransReset();
        root->mode_0x00 = 4;
        root->seq_0x01 = 0;
        set_stream_main_vol_flag(0, 1);
        set_stream_main_vol_flag(1, 0);
        set_stream_main_vol_flag(2, 0);
        set_BGM_volume(get_option_cfg(17));
        set_SE_volume(get_option_cfg(18));
        break;
    }
}

/* 0x8028E3EC (0x104): in area 1, sets the root work's exit-point byte when the player stands within 200 units (and
 * the height band) of one of the two exit points. */
void scene_exit_point_ck(_PL_ROOT* root) {
    _PLW* plw;

    get_rcSwitch_on(0);
    plw = (_PLW*)get_move_work_adrs(2);
    root->field_0x22D9 = 0;
    if (root->area_no_0xEA == 1) {
        if (scene_exit_point_a.y >= plw->motion_pos_0x40 - 200.0f && scene_exit_point_a.y < 100.0f + plw->motion_pos_0x40 &&
            calcVecDistXZ(&scene_exit_point_a, &plw->vec_0x03C) <= 200.0f) {
            root->field_0x22D9 = 1;
        }
        if (scene_exit_point_b.y >= plw->motion_pos_0x40 - 200.0f && scene_exit_point_b.y < 100.0f + plw->motion_pos_0x40 &&
            calcVecDistXZ(&scene_exit_point_b, &plw->vec_0x03C) <= 200.0f) {
            root->field_0x22D9 = 2;
        }
    }
}

/* 0x8028E4F0 - the root work's `+0x22D9` byte. */
u8 quest_move_sub_flag_get(void) {
    _PL_ROOT* root = (_PL_ROOT*)get_move_work_adrs(0);
    if (root == 0) {
        return 0;
    }
    return root->field_0x22D9;
}

/* 0x8028E528 (0x16C): the versus game mode's task: builds the scene on its first step and then runs the root work's
 * mode, handing back to the arena task from mode 4. */
void vs_game_task(TaskSlot* task) {
    _PL_ROOT* root = (_PL_ROOT*)get_move_work_adrs(0);

    system_stream_count_step();
    nwMoveStart();
    switch (task->step_0x08) {
    case 0:
        rso_overlay_mode1_load();
        GameMode_set(1);
        vs_scene_work_init(task);
        task->step_0x08++;
        break;
    case 1:
        prim_init_all();
        switch (root->mode_0x00) {
        case 0:
            vs_scene_boot(root);
            break;
        case 1:
            vs_scene_load(root);
            break;
        case 2:
            vs_scene_main(root);
            break;
        case 3:
            vs_scene_end(root);
            break;
        case 4:
            if (root->field_0x22DA == 0) {
                system_w.field_0x7d5 = 0;
            }
            system_w.leave_flag_0x7d6 = 1;
            system_w.leave_state_0x7d8 = 0;
            sound_frame_entry();
            bgm_stop_all();
            all_reset();
            scene_se_handles_release(root);
            arena_task_start();
            return;
        }
        break;
    }
    nwMoveEnd();
    if (root != 0 && (root->mode_0x00 == 2 || (root->mode_0x00 == 3 && root->field_0x22D7 == 0))) {
        se_frame_step();
        bgm_quest_clear_frame();
    }
}

/* 0x8028E694 (0x84): the versus scene's boot mode: shows the pad-connect display for two players, then moves on to
 * the load mode. */
void vs_scene_boot(_PL_ROOT* root) {
    switch (root->seq_0x01) {
    case 0:
        if (system_w.field_0x8af == 0) {
            pad_connect_disp_set(0);
        } else {
            pad_connect_disp_set(1);
        }
        root->seq_0x01++;
    case 1:
        root->mode_0x00 = 1;
        root->seq_0x01 = 0;
        break;
    }
}

/* 0x8028E718 (0x36C): the versus scene's load mode: the scene resources, the motion banks, the subsystem inits, the
 * arena stage, and every player placed at the area's entry point. */
void vs_scene_load(_PL_ROOT* root) {
    nw4r::math::VEC3 point;
    u32 point_angle;
    s32 count;
    s32 i;

    if (system_w.field_0x8af == 0) {
        loading_disp_set(1, 1);
    } else {
        loading_disp_set(1, 4);
    }
    switch (root->seq_0x01) {
    case 0:
        root->loading_0x22E5 = 1;
        root->scene_kind_0x113 = 2;
        quest_snd_wk_init();
        quest_start_load();
        init_player_work();
        enemy_work_reset();
        vs_scene_res_load(root);
        root->seq_0x01++;
        break;
    case 1:
        if (file_loading_ck(0, 0) != 1) {
            pl_motion_set();
            root->seq_0x01++;
        }
        break;
    case 2:
        em_prog_work_init();
        em_prog_slots_init();
        eft_control_init();
        shell_work_init();
        light_init();
        stage_work_init();
        camera_work_init();
        if (screen_split_mode_ck() == 0) {
            cockpit_quest_init();
        } else {
            cockpit_split_init();
        }
        menu_item_work_init();
        root->seq_0x01++;
        break;
    case 3:
        if (player_move_start_ck() != 0) {
            player_move_start(-1);
            quest_enter_load();
            root->area_no_0xEA = 0;
            if (quest_record_state_get(0) == 10) {
                root->area_no_0xEA = 1;
            }
            stage_map_set(root->area_no_0xED);
            stage_map_res_load(root->area_no_0xED);
            stage_map_obj_load(root->area_no_0xED);
            stage_area_load(0, root->area_no_0xED, root->area_no_0xEA);
            quest_area_res_load(root->area_no_0xED, root->area_no_0xEA);
            root->seq_0x01++;
            root->kind_0x02 = 0;
        }
        break;
    case 4:
        switch (root->kind_0x02) {
        case 0:
            if (file_loading_ck(0, 0) == 1) {
                break;
            }
            root->kind_0x02++;
        case 1:
            VEC3_ctor(&point);
            stage_area_enter(root->area_no_0xED, root->area_no_0xEA);
            root->bytes_0x136[0] = 0;
            root->bytes_0x136[1] = 0;
            stage_area_point_get(root->area_no_0xEA, 0, &point, &point_angle);
            copyVec3(&root->pos_0x128, &point);
            root->rot_y_0x134 = point_angle;
            count = pl_player_count_get();
            for (i = 0; i < count; i++) {
                my_player_no_set((u8)i);
                pl_area_entry_set(&root->pos_0x128, root->rot_y_0x134, root->area_no_0xED, root->area_no_0xEA);
            }
            my_player_no_set(0);
            light_point_all_clear();
            camera_area_reset();
            quest_screen_enemy_start((Q_MoveWork*)root);
            em_prog_area_start();
            bgm_quest_pending_frame();
            em_pop_work_step();
            hud_msg_frame();
            root->area_slots_0xE0[root->area_slot_index_0xE8] = stage_area_slot_get_c1(root->area_no_0xED, root->area_no_0xEA);
            GlareFilter_on();
            filter_panel_on();
            snd_stage_bank_load(root->area_no_0xE9, root->area_no_0xEA, 0);
            snd_player_banks_load();
            root->kind_0x02++;
            break;
        case 2:
            if (file_loading_ck(0, 0) != 1) {
                root->loading_0x22E5 = 0;
                root->mode_0x00 = 2;
                root->seq_0x01 = 0;
                root->kind_0x02 = 0;
                fade_reset(1);
                fade_reset(2);
                root->timer_0x04 = 30;
                root->timer_0x06 = 30;
                loading_disp_set(0, 1);
                fade_set(0, 2);
                return;
            }
            break;
        }
        break;
    }
    player_control_move();
}

/* 0x8028EA84 (0x1A4): the versus scene's main mode: the quest steps and the end check (which hands over to the end
 * mode with the result message), otherwise the frame for both players. */
void vs_scene_main(_PL_ROOT* root) {
    get_rcSwitch_on(1);
    switch (root->seq_0x01) {
    case 0:
        if (fade_done_ck(0) != 0) {
            quest_main_step();
            if ((u32)Pl_motion_input_ck(2) == 1) {
                pad_input_clear(0);
                pad_input_clear(1);
                root->mode_0x00 = 3;
                root->seq_0x01 = 0;
                root->timer_0x04 = 0;
                root->timer_0x06 = 0;
                cockpit_notice_hold();
                scene_actors_move(root);
                menu_item_close();
                switch (root->end_kind_0xFA) {
                case 4:
                    hud_msg_push(0, quest_str_tbl_35_get(13));
                    break;
                case 6:
                    hud_msg_push(0, quest_str_tbl_35_get(14));
                    break;
                case 7:
                    hud_msg_push(0, quest_str_tbl_35_get(15));
                    break;
                }
                break;
            }
        }
        if (root->timer_0x04 > 0) {
            root->timer_0x04--;
            pad_input_clear(0);
        }
        if (root->timer_0x06 > 0) {
            root->timer_0x06--;
            pad_input_clear(1);
        }
        scene_actors_move(root);
        snd_bgm_hold_set();
        pad_disconnect_notice(0);
        if (arena_work.mode_0x04 == 1) {
            pad_disconnect_notice(1);
        }
        break;
    case 1:
        pl_area_entry_set(&root->pos_0x128, root->rot_y_0x134, root->area_no_0xED, root->area_no_0xEA);
        light_point_all_clear();
        camera_area_reset();
        scene_actors_move(root);
        root->seq_0x01 = 0;
        break;
    }
}

/* 0x8028EC28 (0x308): the versus scene's end mode: the end message, the teardown, the box result and the arena
 * sub-mode it picks, then mode 4. */
void vs_scene_end(_PL_ROOT* root) {
    s32 i;
    _ENEMY_WORK* enemy;
    u16 max;

    pad_input_clear(0);
    pad_input_clear(1);
    switch (root->seq_0x01) {
    case 0:
        if (root->timer_0x04 == 60) {
            quest_arena_summary_step((Q_MoveWork*)root);
        }
        if (root->timer_0x04 > 150) {
            quest_fin_tex_load(root->end_kind_0xFA);
            snd_quest_end_stream_load();
            root->seq_0x01++;
        } else {
            root->timer_0x04++;
        }
        scene_actors_move(root);
        break;
    case 1:
        if (file_loading_ck(0, 0) == 1) {
            scene_actors_move(root);
            break;
        }
        cockpit_event_notice_spawn(root->end_kind_0xFA);
        bgm_mode_jingle_play(root->end_kind_0xFA);
        root->timer_0x04 = 150;
        root->seq_0x01++;
    case 2:
        if (root->timer_0x04 > 0) {
            root->timer_0x04--;
        }
        switch (root->kind_0x02) {
        case 0:
            bgm_ch_fade_out(3);
            root->kind_0x02++;
        case 1:
            if (root->timer_0x04 < 20) {
                fade_set(0, 1);
                root->kind_0x02++;
                root->timer_0x06 = 0;
            }
            scene_actors_move(root);
            break;
        case 2:
            if (root->timer_0x04 > 0) {
                scene_actors_move(root);
                break;
            }
            root->field_0x22D7 = 1;
            sound_frame_entry();
            eft_res_release_all();
            draw_shape_state_clear();
            filter_flag_clear();
            subTransReset();
            filter_reset();
            fade_reset(0);
            em_kind_res_release(pl_map_kind_get());
            stage_area_slot_release(root->area_slots_0xE0[root->area_slot_index_0xE8]);
            root->area_slots_0xE0[root->area_slot_index_0xE8] = -1;
            em_kind_slot_release_all();
            em_res_buffers_free();
            eft_res_slots_release();
            if (root->mode_work_0xDC != 0) {
                enemy = (_ENEMY_WORK*)get_move_work_adrs(3);
                max = get_move_work_max(3);
                for (i = 0; i < max; i++, enemy++) {
                    if (enemy != 0) {
                        enemy_work_release(enemy);
                    }
                }
                rso_overlay_mode3_load();
                arena_resource_load();
                snd_result_banks_load();
                q_result_work_clear((Q_MoveWork*)root);
                root->timer_0x04 = 120;
                root->kind_0x02++;
                break;
            }
            se_work_init();
            root->mode_0x00 = 4;
            root->seq_0x01 = 0;
            system_w.online_0x8ae = 1;
            break;
        case 3:
            switch (multi_box_result_step((Q_MoveWork*)root)) {
            case 0:
                return;
            case 1:
                arena_sub_mode_set(3);
                break;
            case 2:
                arena_sub_mode_set(1);
                break;
            case 3:
                arena_sub_mode_set(2);
                break;
            default:
                arena_sub_mode_set(0);
                break;
            }
            root->kind_0x02 = 99;
        case 99:
            se_work_init();
            root->mode_0x00 = 4;
            root->seq_0x01 = 0;
            system_w.online_0x8ae = 1;
            break;
        }
        break;
    }
}

/* 0x8028EF30 - the root work's byte array at +0x136, indexed by the caller's byte. */
u8 Pl_area_flag_get(u8 index) {
    _PL_ROOT* root = (_PL_ROOT*)get_move_work_adrs(0);
    if (root == 0) {
        return 0;
    }
    return root->bytes_0x136[index];
}

/* 0x8028EF7C - find the root entry whose key is `key`: the six fixed slots at +0x2274 first, then
 * the area record `quest_arena_item_count_get()` names, then the single extra pointer at +0x2258. */
PlRootEntry* quest_spawn_rec_find(u32 key) {
    _PL_ROOT* root = (_PL_ROOT*)get_move_work_adrs(0);
    PlRootEntry* entry;
    u32 index;
    u32 count;
    int i;

    if (root == 0) {
        return 0;
    }

    if (key < 0x20) {
        PlRootEntry* slot = root->sparse_0x2274;
        for (i = 0; i < 6; i++, slot++) {
            if (slot->key_0x00 == key) {
                return slot;
            }
        }
        return 0;
    }

    index = quest_arena_item_count_get();
    entry = root->recs_0x154[index].entries_0x00;
    count = root->rec_counts_0x2154[index];
    for (i = 0; i < (int)count; i++, entry++) {
        if (entry->key_0x00 == key) {
            return entry;
        }
    }
    if (root->extra_0x2258 != 0) {
        if (root->extra_0x2258->key_0x00 == key) {
            return root->extra_0x2258;
        }
    }
    return 0;
}

/* 0x8028F0B4 - the same search, but the caller names the area index itself. */
PlRootEntry* quest_spawn_rec_find_in(u32 key, u32 index) {
    _PL_ROOT* root = (_PL_ROOT*)get_move_work_adrs(0);
    PlRootEntry* entry;
    u32 count;
    int i;

    if (root == 0) {
        return 0;
    }

    if (key < 0x20) {
        PlRootEntry* slot = root->sparse_0x2274;
        for (i = 0; i < 6; i++, slot++) {
            if (slot->key_0x00 == key) {
                return slot;
            }
        }
        return 0;
    }

    entry = root->recs_0x154[index].entries_0x00;
    count = root->rec_counts_0x2154[index];
    for (i = 0; i < (int)count; i++, entry++) {
        if (entry->key_0x00 == key) {
            return entry;
        }
    }
    if (root->extra_0x2258 != 0) {
        if (root->extra_0x2258->key_0x00 == key) {
            return root->extra_0x2258;
        }
    }
    return 0;
}

/* 0x8028F1E8 - hand the caller's `+0x08` byte to the `arena_user_data_buf` table as a 256-byte stride. */
void arena_userdata_slot_apply(u8* self) {
    arena_userdata_apply(arena_user_data_buf + ((u32)self[8] << 8), self);
}

/* 0x8028F204 - the `+0x22D7` byte == 1 predicate. */
u32 pl_root_lock_ck(void) {
    _PL_ROOT* root = (_PL_ROOT*)get_move_work_adrs(0);
    if (root == 0) {
        return 0;
    }
    return (s8)root->field_0x22D7 == 1;
}

/* 0x8028F24C - the `+0xE9` area byte through `stage_map_kind_get`. */
u32 pl_map_kind_get(void) {
    _PL_ROOT* root = (_PL_ROOT*)get_move_work_adrs(0);
    if (root == 0) {
        return 0;
    }
    return stage_map_kind_get_c1(root->area_no_0xE9);
}

/* 0x8028F288 - the root work's `+0x22DB` byte. */
u8 quest_em_level_get(void) {
    _PL_ROOT* root = (_PL_ROOT*)get_move_work_adrs(0);
    if (root == 0) {
        return 0;
    }
    return root->field_0x22DB;
}
} /* extern "C" */

/* 0x8028F2C0 - the stage's day/night byte (`get_move_work_adrs(0)` + 0x22D8), 0 when absent. */
u8 get_gm_daynight(void) {
    _PL_ROOT* root = (_PL_ROOT*)get_move_work_adrs(0);
    if (root == 0) {
        return 0;
    }
    return root->field_0x22D8;
}
extern "C" {

/* 0x8028F2F8 - arm the mode sequence: 6 for the "no argument" call, 4 otherwise. */
void pl_quest_end_request(u32 arg) {
    _PL_ROOT* root = (_PL_ROOT*)get_move_work_adrs(0);
    if (root == 0) {
        return;
    }
    if (quest_flag_8_ck(0) == 0) {
        return;
    }
    if (arg == 0) {
        root->seq_0xFB = 6;
    } else {
        root->seq_0xFB = 4;
    }
}

/* 0x8028F368 - hand the mode work over to the network move system; 1 on success. */
u32 pl_net_move_handover(void) {
    _PL_ROOT* root = (_PL_ROOT*)get_move_work_adrs(0);
    u8* mode;

    if (root == 0) {
        return 0;
    }
    mode = root->mode_work_0xDC;
    if (mode == 0) {
        return 0;
    }
    if (quest_flag_80_ck(0) == 0) {
        return 0;
    }
    mode[0x6A3E] = 4;
    if (isServerSelectState() != 1) {
        root->seq_0xFC = 4;
    }
    return 1;
}

} /* extern "C" */

/* The two exit points of area 1 `scene_exit_point_ck` measures the player against; MWCC's `__sinit` builds them. */
extern "C" SceneExitPoint scene_exit_point_a(573.0f, -1265.0f, 3392.0f);
extern "C" SceneExitPoint scene_exit_point_b(1413.0f, -571.0f, 1740.0f);
