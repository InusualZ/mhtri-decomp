/* lobby/lb_menu_pos_tbl.cpp - the offline lobby's game-mode task, the item list's clock and selection, the player
 *   placement, the lobby exit positions and the lobby work block.
 * RANGE. .text 0x8021F020-0x80220038 (20 functions); .ctors 0x8056F364-0x8056F368, .data 0x805BA650-0x805BA750, .bss
 *   0x806AAA88-0x806AACC0 (`lb_item_list_state`, `lb_page_state_0/1`, `lb_menu_pos_tbl`, `lb_menu_pos_extra`,
 *   `lobby_w`, whose record `LbLobbyWork` is in `lobby/lobby_work.h`), .sdata2 0x80799C78-0x80799CD8, extab,
 *   extabindex.  Between `lobby/lb_menu_scratch.cpp` and `lobby/lb_equip_page.cpp`; the three share
 *   `lobby/fn_8021E1EC.h`.
 * FLAGS. `cflags_lobby`; file-scope `#pragma peephole off` (retail keeps the narrowing `clrlwi`s; docs/lobby.md) and
 *   `#pragma pool_data off` (retail's `__sinit` addresses each `.bss` object with its own `lis`/`addi`, playbook 43).
 * NAMES. `lobby_local_task` is the task `GameModeExec` starts for game mode 2 when `system_w` +0x7D3 is clear (beside
 *   `field_game_task`/`quest_game_task`); `__sinit_\lb_menu_pos_tbl_cpp` and `__ct__11LbLobbyWorkFv` are the
 *   compiler's names for the `.ctors` entry and the work's constructor.
 *   GUESS (from each body and its callers): lb_event_schedule_step, lb_item_list_step_get, lb_item_list_depth_get,
 *   lb_item_list_select, lobby_local_init, lobby_local_task, lobby_local_opening_step, lobby_local_load_start,
 *   lobby_local_player_send, lobby_local_main_step, lobby_local_area_change_step, lobby_local_exit_set,
 *   lb_menu_pos_tbl, lb_menu_pos_extra, lb_menu_pos_radius, lb_page_state_0/1, lb_unused_zero.
 * RESIDUALS. `lobby_local_main_step` case 2's else arm: retail schedules `addi r30,r31,lb_param_w@l` before `li r0,1`,
 *   ours the reverse (a pointer local, `TRUE`, the swapped if/else and playbook 189's dead predecessor measured; none
 *   moves it) - 8 bytes, the one `.text` blocker of a flip.
 *  - flipcheck: the calls to `field_game_task_start`/`lb_pl_act_idle_ck`/`lobby_enter_seq_step` resolve once the
 *    registry names reach the map; `.sdata2` is 0x5C against the 0x60 claim (the trailing alignment word).
 * SHAPES. `syncItemListClock` takes `count` as `u32`: retail converts it unsigned (no `xoris`).  The `frsp` before the
 *   0.5 compare (and before the depth store of `lb_event_schedule_step`) is a `(f32)(f64)` round trip on an `f32`.
 *   The constructor is an `inline` member defined after `lobby_w` with `__declspec(noinline)`, so MWCC emits it after
 *   `__sinit` as retail does; `lb_unused_zero` is an exported `const` so the pool keeps its 0.0 between 1.0 and 200.0.
 */
#include "types.h"
#include "nw4r/math.h"

#include "Runtime.PPCEABI.H/memset.h"

#include "lobby/fn_8021E1EC.h"
#include "lobby/lb_menu_pos_tbl.h"
#include "mh3_pad.h"                   /* VEC3_ctor / copyVec3 / setVec3, GameModeExec, setSoftresetFlag (rule 2) */
#include "mh3_pad/system_w.h"
#include "mh3_pad/lb_param_w.h"
#include "mh3_pad/Psw.h"
#include "mh3_pad/control.h"
#include "Network/network_pat_control.h" /* NetCtrlWk::getSchedulePhase */
#include "ef/fn_800CDB2C.h"
#include "ef/system_core.h"
#include "ef/eft_res.h"
#include "ef/get_move_work_adrs.h"
#include "ef/quest_npc_flags_set.h"
#include "fn_80056F24.h"
#include "fn_8004CAD8.h"
#include "fn_80047398.h"
#include "draw_shape.h"
#include "gallery_open.h"
#include "get_userdata.h"
#include "nw_resource.h"
#include "stage_map_set.h"
#include "field_game_task_start.h"
#include "camera/camera_work_init.h"
#include "hud/Pl_net_send.h"
#include "menu/Movie_open.h"
#include "menu/menu_item.h"
#include "Pl/plw.h"
#include "Pl/pl_act_stage_latch_set.h"
#include "quest/quest_entry.h"
#include "sound/fn_800D7F54.h"
#include "sound/fn_800E3CBC.h"
#include "sound/fn_800E46E8.h"
#include "sound/fn_800EF7D8.h"
#include "sound/fn_800F2A94.h"
#include "sound/snd_bank_loader.h"
#include "stage/get_now_mapno.h"
#include "stage/stg_w.h"
#include "lobby/demo_work_update.h"
#include "lobby/lb_menu_scratch_init.h"
#include "lobby/lb_npc.h"
#include "lobby/lb_npc_func.h"
#include "lobby/lb_pl_act_idle_ck.h"
#include "lobby/lb_unlock_rank_get.h"
#include "lobby/lobby_enter_seq_step.h"

/* Retail keeps the narrowing `clrlwi` the peephole pass folds away. */
#pragma peephole off
/* Retail addresses each `.bss` object with its own `lis`/`addi` (playbook 43). */
#pragma pool_data off

/* One lobby exit position: a vector built through `setVec3`, so the file-scope ones are constructed by the unit's
 * `__sinit`.  size: 0xC */
struct LbMenuPos : public nw4r::math::VEC3 {
    /* Builds (x, y, z). */
    LbMenuPos(f32 x, f32 y, f32 z)
    {
        setVec3(this, x, y, z);
    }
};

/* .bss 0x806AAB08 - the four lobby exits the player walks into (the fourth is never taken), and 0x806AAB38, a fifth
 * position built with them; defined at the foot of the file. */
extern "C" LbMenuPos lb_menu_pos_tbl[4];
extern "C" LbMenuPos lb_menu_pos_extra;

extern "C" {

void lobby_local_opening_step(TaskSlot* self);
void lobby_local_load_start(TaskSlot* self);
void lobby_local_player_send(void);
void lobby_local_main_step(TaskSlot* self);
void lobby_local_area_change_step(TaskSlot* self);
void lobby_local_exit_set(TaskSlot* self);

/* 0x8021F020 (0xBC): takes the network schedule's clock: the stamp, the row count and its per-tick step, and the
 * scroll depth; selects row 1 once the depth reaches one half. */
void syncItemListClock(u32 stamp, u32 count, f32 depth)
{
    LbMenuItemState* state = &lb_item_list_state;

    state->count_0x14 = count;
    state->step_0x18 = 30.0f * count;
    state->scroll_0x30 = depth;
    state->depth_0x34 = depth;
    frame_counter = stamp;
    state->stamp_0x04 = stamp;
    if ((f32)(f64)depth >= 0.5f) {
        state->selected_0x38 = 1;
    } else {
        state->selected_0x38 = 0;
    }
    state->changed_0x3C = 0;
    state->events_0x48 = (s8)CalculateEvents();
    state->active_0x00 = 0;
}

/* 0x8021F0DC (0x13C): advances the item list's depth by the frames since the last stamp (wrapping past one), and on a
 * selection change refreshes the item database, the area objects and the lobby music. */
void lb_event_schedule_step(void)
{
    LbMenuItemState* state = &lb_item_list_state;
    s32 prev;
    f32 phase;
    u32 now;
    f32 elapsed;

    state->events_0x48 = (s8)CalculateEvents();
    prev = state->selected_0x38;
    phase = NetCtrlWk::getSchedulePhase();
    state->scroll_0x30 = phase;
    if (lb_item_list_state.active_0x00 == 0) {
        state->depth_0x34 = (f32)(f64)phase;
        state->active_0x00++;
        state->stamp_0x04 = frame_counter;
    } else {
        now = frame_counter;
        if (now > state->stamp_0x04) {
            elapsed = now - state->stamp_0x04;
            state->stamp_0x04 = now;
            state->depth_0x34 += elapsed / state->step_0x18;
            while (state->depth_0x34 > 1.0f) {
                state->depth_0x34 -= 1.0f;
            }
            if (state->depth_0x34 < state->scroll_0x30) {
                state->depth_0x34 = state->scroll_0x30;
            }
        }
    }
    if (state->scroll_0x30 >= 0.5f) {
        state->selected_0x38 = 1;
    } else {
        state->selected_0x38 = 0;
    }
    if (prev != state->selected_0x38) {
        state->changed_0x3C = 1;
        lb_item_db_toggle();
        stage_area_objects_refresh();
        bgm_lobby_ch1_stop();
    }
}

/* .sdata2 0x80799C8C - a zero constant nothing in the unit reads (it sits in the pool between the functions that
 * use 1.0f and 200.0f); exported so the link keeps it, as the force-active retail object does. */
__declspec(export) extern const f32 lb_unused_zero = 0.0f;

/* 0x8021F218 (0x10): the item list's per-tick row step. */
f32 lb_item_list_step_get(void)
{
    return lb_item_list_state.step_0x18;
}

/* 0x8021F228 (0x10): the item list's scroll depth. */
f32 lb_item_list_depth_get(void)
{
    return lb_item_list_state.depth_0x34;
}

/* 0x8021F238 (0x10): the selected row; also published to the sound unit through `unsplit/lobby.h`. */
s32 getItemListSelection(void)
{
    return lb_item_list_state.selected_0x38;
}

/* 0x8021F248 (0x34): selects a row and remembers whether the selection actually moved. */
void lb_item_list_select(s32 selected)
{
    LbMenuItemState* state = &lb_item_list_state;
    s32 prev = state->selected_0x38;

    state->selected_0x38 = (u8)selected;
    if (prev == state->selected_0x38) {
        state->changed_0x3C = 0;
    } else {
        state->changed_0x3C = 1;
    }
}

/* 0x8021F27C (0x1C): true on the frame the selection changed. */
s32 lb_item_list_done_ck(void)
{
    return lb_item_list_state.changed_0x3C - 1 == 0;
}

/* 0x8021F298 (0x14): acknowledges the selection change. */
void lb_item_list_done_clear(void)
{
    lb_item_list_state.changed_0x3C = 0;
}

/* 0x8021F2AC (0xFC): clears the lobby work and parameters, points `lobby_wp` at the work, allocates the menu work,
 * restores the item list's row and loads the lobby map. */
void lobby_local_init(void)
{
    Q_UserData* user = get_userdata();

    memset(&lobby_w, 0, sizeof(LbLobbyWork));
    memset(&lb_param_w, 0, sizeof(LbParamWork));
    system_w.leave_state_0x7d8 = 0;
    system_w.field_0x7d2 = 0;
    system_w.field_0x7d1 = 0;
    lobby_wp = &lobby_w;
    lobby_wp->menu_0xAC = (struct LbMenuWork*)work_mem_alloc(0x2000);
    memset(lobby_wp->menu_0xAC, 0, 0x2000);
    lobby_wp->slots_0x00C[0] = -1;
    lobby_wp->slots_0x00C[1] = -1;
    lobby_wp->talk_area_0x055 = 2;
    lobby_wp->kitchen_busy_0x054 = 0;
    lb_item_list_select(user->item_list_sel_0x3E00);
    memset(lobby_wp->place_ids_0x134, 0xFF, sizeof(lobby_wp->place_ids_0x134));
    rso_overlay_release_all();
    rso_overlay_mode2_load();
    stage_map_set(22);
}

/* 0x8021F3A8 (0x220): the offline lobby's game-mode task: the opening movie, the load, the lobby itself and the exit
 * into the quest or the field. */
void lobby_local_task(TaskSlot* self)
{
    s32 i;

    system_stream_count_step();
    nwMoveStart();
    switch (self->step_0x08) {
    case 0:
        self->step_0x08++;
        self->sub_0x09 = 0;
        self->step_0x0A = 0;
        break;
    case 1:
        switch (self->sub_0x09) {
        case 0:
            lobby_local_opening_step(self);
            break;
        case 1:
            lobby_local_load_start(self);
            break;
        case 2:
            lobby_local_main_step(self);
            break;
        case 3:
            lobby_local_exit_set(self);
            break;
        case 4:
            for (i = 0; i < 3; i++) {
                if (lobby_w.se_0x0B4[i] != NULL) {
                    se_handle_clear((u8*)lobby_w.se_0x0B4[i]);
                }
            }
            game_reset_to_title();
            return;
        case 5:
            switch (self->step_0x0A) {
            case 0:
                self->step_0x0A = 2;
                break;
            case 1:
                self->step_0x0A = 2;
            case 2:
                for (i = 0; i < 3; i++) {
                    if (lobby_w.se_0x0B4[i] != NULL) {
                        se_handle_clear((u8*)lobby_w.se_0x0B4[i]);
                    }
                }
                lb_npc_model_release_all();
                system_full_reset();
                clear_qResult_work();
                PlayMode_set(3);
                GameMode_set(1);
                if (lb_param_w.field_0x00 == 2005) {
                    lb_param_w.entry_0x08 |= 0x80;
                }
                if (lb_param_w.entry_0x08 & 0x80) {
                    quest_npc_flags_set();
                }
                if (system_w.leave_state_0x7d8 == 0 || lb_param_w.flag_0x0b == 1) {
                    field_game_task_start();
                } else {
                    if (lb_param_w.field_0x00 == 0) {
                        system_w.field_0x7d2 = 1;
                    } else {
                        system_w.field_0x7d2 = 0;
                    }
                    PlayMode_set(6);
                    GameModeExec();
                }
                return;
            }
            break;
        }
        break;
    }
    nwMoveEnd();
}

/* 0x8021F5C8 (0x148): plays the opening movie once (when `system_w` asks for it), then resets the game into the lobby
 * mode and initialises the offline lobby. */
void lobby_local_opening_step(TaskSlot* self)
{
    SystemWork* sys = &system_w;

    if (sys->opening_movie_0x887 == 0) {
        self->sub_0x09 = 1;
        self->step_0x0A = 3;
    } else {
        switch (self->step_0x0A) {
        case 0:
            movie_bgm_load();
            self->step_0x0A++;
            break;
        case 1:
            if (srt_ready_ck(0) != 0) {
                Movie_open((MovieIndex)2);
                PlayStream(0, 12);
                self->step_0x0A++;
            }
            break;
        case 2:
            if (Check_movie_finish() == 1) {
                Movie_close();
                gallery_open(3);
                bgm_stop_all();
                self->step_0x0A++;
                sys->opening_movie_0x887 = 0;
            } else {
                subTransSet((u32)Movie_draw_sub, 0, NULL);
            }
            break;
        }
    }
    if (self->step_0x0A >= 3) {
        all_reset();
        GameMode_set(2);
        system_w.field_0x884 = 0;
        system_w.field_0x7d3 = 0;
        lobby_local_init();
        snd_bank_layout(0);
        quest_init(0);
    }
}

/* 0x8021F710 (0x68): moves the task to the lobby step and starts the single-player load. */
void lobby_local_load_start(TaskSlot* self)
{
    self->sub_0x09 = 2;
    self->step_0x0A = 0;
    self->sub_step_0x0B = 0;
    lobby_wp->field_0x007 = 0;
    lobby_wp->field_0x006 = 0;
    my_player_no_set(0);
    player_count_set(1);
    loading_disp_set(1, 0);
    gpframe_tex_load();
    menu_tex_load();
    itemicon_tex_load();
}

/* 0x8021F778 (0x64): sends the local player's state (kind 2) unless `system_w` +0x0C bit 5 is set. */
void lobby_local_player_send(void)
{
    _PLW* players;

    if ((system_w.field_0x0c & 0x20) == 0) {
        players = (_PLW*)get_move_work_adrs(2);
        Pl_net_send(players + (s8)my_player_no(), 2, 0);
    }
}

/* 0x8021F7DC (0x4F4): the lobby step: the entry sequence, the frame (and the walk into an exit), the exit into a
 * quest, an area change and the fade out. */
void lobby_local_main_step(TaskSlot* self)
{
    u16 pressed = Psw[0].button_0x2C0.pressed_0x04;
    _PLW* player;
    f32 limit;
    s32 i;
    u8 area;
    u16 rank;

    if (lobby_wp->field_0x0B0 == 1 && self->step_0x0A != 7) {
        self->step_0x0A = 7;
        self->timer_0x0E = 20;
        fade_set(0, 1);
    }
    if (lobby_wp->area_change_0x026 == 1) {
        self->step_0x0A = 4;
        self->sub_step_0x0B = 0;
        lobby_wp->area_change_0x026 = 0;
    }
    prim_init_all();
    switch (self->step_0x0A) {
    case 0:
        loading_disp_set(1, 0);
        self->flag_0x14 = 1;
        if ((u8)lobby_enter_seq_step((struct LbChgSeqWork*)self) == 0) {
            loading_disp_set(1, 0);
        } else {
            loading_disp_set(0, 0);
        }
        if (self->step_0x0A != 0) {
            PlayMode_ck();
            se_work_init();
            bgm_ctrl_init();
            snd_bank_layout(0);
            scene_se_bank_load(get_now_mapno(), 0);
            snd_player_banks_load();
            lobby_w.se_0x0B4[0] = (struct _se_w*)se_entry_request(12, (struct _ENEMY_WORK*)&lobby_w, lb_npc_func.se_func_0x00);
            lobby_w.se_0x0B4[1] = (struct _se_w*)se_entry_request(13, (struct _ENEMY_WORK*)&lobby_w, NULL);
            lobby_w.se_0x0B4[2] = (struct _se_w*)se_entry_request(14, (struct _ENEMY_WORK*)&lobby_w, NULL);
            bgm_behind_flag_clear();
            lobby_w.field_0x12C = 1;
            lobby_w.busy_0x172 = 10;
            loading_disp_set(0, 0);
            GlareFilter_on();
            filter_panel_on();
        }
        break;
    case 1:
        if (lobby_w.field_0x12C == 1) {
            fade_set(0, 2);
            pad_input_clear(0);
        } else if (lobby_w.busy_0x172 != 0) {
            lobby_w.busy_0x172--;
            pad_input_clear(0);
        }
        lobby_frame_update();
        bgm_ctrl_frame();
        se_frame_step();
        lobby_w.sub_0x0B1 = 0;
        if (self->flag_0x14 == 0) {
            break;
        }
        get_rcSwitch_on(0);
        player = (_PLW*)get_move_work_adrs(2);
        for (i = 0; i < 4; i++) {
            if (i == 3) {
                continue;
            }
            area = get_now_areano();
            system_w.field_0x7cf = get_now_mapno();
            system_w.area_no_0x7d0 = area;
            if (lb_pl_act_idle_ck(player) == 0) {
                continue;
            }
            if (!((lb_param_w.field_0x00 != 0 && area == 0 && i == 1) ||
                  (lb_param_w.field_0x00 == 0 && !(area == 0 && i >= 2) && !(area == 2 && i != 2)))) {
                continue;
            }
            if (!(lb_menu_pos_tbl[i].y >= player->motion_pos_0x40 - 200.0f &&
                  lb_menu_pos_tbl[i].y < 100.0f + player->motion_pos_0x40)) {
                continue;
            }
            limit = lb_menu_pos_radius[i];
            if (!(calcVecDistXZ(&lb_menu_pos_tbl[i], &player->vec_0x03C) <= limit)) {
                continue;
            }
            if ((u32)i <= 2) {
                lobby_w.sub_0x0B1 = i + 1;
            }
            if (((lb_param_w.field_0x00 != 0 && i == 1) || i == 0 || i == 2) && (pressed & 0x10)) {
                if (menu_busy_ck() == 0) {
                    self->exit_sel_0x15 = i & 1;
                    self->exit_state_0x16 = 0;
                    self->exit_wait_0x17 = 0;
                    self->step_0x0A = 2;
                    if (i == 3) {
                        self->step_0x0A = 3;
                        self->sub_step_0x0B = 0;
                        self->wait_0x0C = 0;
                    }
                }
            }
            break;
        }
        break;
    case 2:
        if (lb_param_w.field_0x00 != 0) {
            self->exit_sel_0x15 = 0;
            system_w.leave_state_0x7d8 = lb_param_w.field_0x00;
            lb_param_w.rank_sel_0x09 = getItemListSelection();
        } else {
            lb_param_w.flag_0x0b = 1;
            lb_param_w.rank_sel_0x09 = getItemListSelection();
            rank = lb_unlock_rank_get();
            if (rank == 0) {
                system_w.leave_state_0x7d8 = 0;
            } else {
                self->exit_sel_0x15 = 0;
                lb_param_w.field_0x00 = rank;
                system_w.leave_state_0x7d8 = rank;
            }
        }
        prim_init_all();
        filter_reset();
        self->sub_0x09 = 5;
        self->step_0x0A = 0;
        break;
    case 4:
        lobby_local_area_change_step(self);
        break;
    case 7:
        if (self->timer_0x0E > 0) {
            self->timer_0x0E--;
            lobby_frame_update();
            bgm_ctrl_frame();
            se_frame_step();
        } else {
            self->sub_0x09 = 3;
            filter_reset();
        }
        break;
    }
}

/* 0x8021FCD0 (0x10): places the player at `pos` facing `angle` in area `mode`, with the default stage latch. */
void lobby_player_place(u8 mode, nw4r::math::VEC3* pos, u16 angle)
{
    lobby_player_place_ex(mode, pos, angle, 1);
}

/* 0x8021FCE0 (0x80): records the area change to `mode` with the spawn position and angle, latches the player's stage
 * and fades out. */
void lobby_player_place_ex(u8 mode, nw4r::math::VEC3* pos, u16 angle, s32 kind)
{
    lobby_wp->area_next_0x015 = mode;
    copyVec3(&lobby_wp->spawn_pos_0x018, pos);
    lobby_wp->spawn_id_0x024 = angle;
    lobby_wp->area_change_0x026 = 1;
    lobby_wp->busy_0x172 = 10;
    pl_act_stage_latch_set(my_player_work_get(), kind);
    fade_set(0, 1);
}

/* 0x8021FD60 (0x1F0): the area change: releases the old area's slot, loads the next area, then enters it and places
 * the player, the NPCs and the camera. */
void lobby_local_area_change_step(TaskSlot* self)
{
    s8 slot;

    switch (self->sub_step_0x0B) {
    case 0:
        setSoftresetFlag(false);
        eft_res_release_all();
        filter_flag_clear();
        filter_panel_flag_clear();
        stage_area_slot_release(lobby_wp->slots_0x00C[lobby_wp->field_0x014]);
        lobby_wp->slots_0x00C[lobby_wp->field_0x014] = -1;
        self->sub_step_0x0B++;
        break;
    case 1:
        slot = stage_area_free_slot_get();
        if (slot >= 0) {
            lobby_wp->slots_0x00C[lobby_wp->field_0x014] = slot;
            lobby_wp->area_prev_0x016 = lobby_wp->field_0x002;
            lobby_wp->field_0x002 = lobby_wp->area_next_0x015;
            stage_area_load((u8)slot, lobby_wp->field_0x001, lobby_wp->field_0x002);
            if (lobby_wp->area_prev_0x016 == 0 && lobby_wp->field_0x002 == 2) {
                bgm_stop_all();
            }
            if (lobby_wp->area_prev_0x016 == 2 && lobby_wp->field_0x002 == 0) {
                bgm_stop_all();
            }
            self->sub_step_0x0B++;
        }
        break;
    case 2:
        if (file_loading_ck(NULL, NULL) != 1) {
            stage_area_enter(lobby_wp->field_0x001, lobby_wp->field_0x002);
            demo_work_update();
            lb_player_spawn_set(lobby_wp->spawn_0x018, lobby_wp->spawn_id_0x024, lobby_wp->field_0x001,
                                lobby_wp->field_0x002);
            lb_npc_model_release_all();
            lb_npc_map_setup();
            camera_area_reset();
            lb_menu_scratch_init(1);
            snd_area_bank_load(lobby_wp->field_0x001, lobby_wp->field_0x002);
            if (system_w.field_0x7d3 == 1 && system_w.field_0x86A == 1) {
                lobby_local_player_send();
            }
            self->step_0x0A = 1;
            self->sub_step_0x0B = 0;
            fade_set(0, 2);
            filter_panel_on();
            setSoftresetFlag(true);
        }
        break;
    }
}

/* 0x8021FF50 (0xC): moves the task to the title reset. */
void lobby_local_exit_set(TaskSlot* self)
{
    self->sub_0x09 = 4;
}

/* The unit's `.data`: the exit radii and the trade tables. */
f32 lb_menu_pos_radius[4] = { 300.0f, 500.0f, 300.0f, 300.0f };

u16 lb_trade_price_rows[58] = {
    0x0097, 0x001E, 0x0000, 0x010C, 0x0032, 0x0000, 0x010B, 0x0064, 0x0000, 0x010A, 0x0096, 0x0000,
    0x010D, 0x0096, 0x0000, 0x0096, 0x0064, 0x0000, 0x025A, 0x00BE, 0x0000, 0x0122, 0x0140, 0x0000,
    0x01EA, 0x03E8, 0x0000, 0x00D0, 0x000A, 0x0000, 0x00CF, 0x000F, 0x0000, 0x00D2, 0x000F, 0x0000,
    0x00D3, 0x0014, 0x0000, 0x00D1, 0x0014, 0x0000, 0x00D4, 0x001E, 0x0000, 0x00D5, 0x0028, 0x0000,
    0x00D7, 0x0028, 0x0000, 0x00D6, 0x0019, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
};

u16 lb_trade_flag_rows[34] = {
    0x0024, 0x0003, 0x0000, 0x0021, 0x0001, 0x0000, 0x0022, 0x0002, 0x0000, 0x001B, 0x0001, 0x0000,
    0x001A, 0x0003, 0x0000, 0x0020, 0x0003, 0x0000, 0x0017, 0x0005, 0x0000, 0x000A, 0x0005, 0x0000,
    0x000B, 0x000A, 0x0000, 0x0011, 0x000D, 0x0009, 0x0000, 0x0000, 0x0000, 0x0000,
};

u16 lb_trade_pair_rows[28] = {
    0x000C, 0x0096, 0x00C8, 0x000A, 0x0003, 0x012C, 0x0190, 0x000D, 0x0010, 0x01C2, 0x0258, 0x000E,
    0x0001, 0x0320, 0x03E8, 0x0013, 0x000F, 0x03E8, 0x0514, 0x0015, 0x0002, 0x03E8, 0x0514, 0x0017,
    0x0000, 0x0000, 0x0000, 0x0000,
};

/* The unit's `.bss`, after every use. */
LbMenuItemState lb_item_list_state;  /* .bss 0x806AAA88 */
LbPageState lb_page_state_0;         /* .bss 0x806AAAD8 */
LbPageState lb_page_state_1;         /* .bss 0x806AAAF0 */

/* The lobby exits, built by `__sinit`. */
LbMenuPos lb_menu_pos_tbl[4] = {
    LbMenuPos(2623.0f, 455.0f, -650.0f),
    LbMenuPos(-2460.0f, 179.0f, -770.0f),
    LbMenuPos(983.0f, 461.0f, 49.0f),
    LbMenuPos(-210.0f, 0.0f, -127.0f),
};
LbMenuPos lb_menu_pos_extra(-946.0f, 474.0f, -1848.0f);

LbLobbyWork lobby_w;

}  /* extern "C" */

/* 0x8021FFFC (0x3C): constructs the work's two position vectors. */
__declspec(noinline) inline LbLobbyWork::LbLobbyWork()
{
    VEC3_ctor(&spawn_pos_0x018);
    VEC3_ctor(&npc_pos_0x058);
}
