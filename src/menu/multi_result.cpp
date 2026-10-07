/*
 * menu/multi_result.cpp - the lobby and game-mode flow: the game-mode task and its lobby sub-states, the area change,
 *   the Poogie and name menus, the note pane's proximity test and the NPC trade and gift rolls.  C++.
 * RANGE. .text 0x803A12D4-0x803A3A50 (27 functions); extab 0x8001877C-0x8001882C, extabindex 0x800389A0-0x80038AA8,
 *   .data 0x805F1E18-0x805F2038 (the Poogie/name menu id tables, the note pane's home spots, the trade and gift tables),
 *   .sdata 0x80793520-0x80793530, .sbss 0x80794C10-0x80794C18, .sdata2 0x8079C438-0x8079C448.  Left edge:
 *   `enemy/em029_prog.cpp` ends with `em_area_team_ck`, and `lobby_flow_init` opens this TU's extab records; right edge:
 *   `lobby/lb_quest_screen.cpp`.  The range is a sequence of objects, not one TU: the flow and menus, then a note pane
 *   and NPC band.  The dump's SDK names inside the range are linker-folded duplicates of tiny bodies, not evidence.
 * FLAGS. `cflags_menu` (configure.py).
 * NAMES. Module `menu`: no `__FILE__` string reaches the range, so the file name and every function name are GUESSes
 *   from the bodies.
 *   GUESS (from each body and its callers): note_pane_player_near_ck, note_pane_home_table
 *   GUESS (from each body and its callers): npc_trade_special_ck, npc_trade_pick, npc_gift_roll, npc_trade_special_tbl
 *   GUESS (the tables by map kind): npc_trade_tbl_1, npc_trade_tbl_2, npc_trade_tbl_3, npc_trade_tbl_4, npc_trade_tbl_5
 *   GUESS (the tables by map kind): npc_gift_tbl_1, npc_gift_tbl_2, npc_gift_tbl_3, npc_gift_tbl_4, npc_gift_tbl_5
 *   GUESS (from each body and its callers): lobby_flow_init, lobby_flow_boot_step, lobby_flow_main_step
 *   GUESS: lobby_area_change_net_req, lobby_area_change_req, lobby_area_change_step, lobby_flow_leave_step
 *   GUESS: lobby_area_change_mode, lobby_area_prev
 *   GUESS (from each body and its callers): lobby_pig_dress_view_set, pig_dress_npc_set, pig_menu_open
 *   GUESS: pig_menu_close, pig_menu_step, pig_dress_list_build, pig_dress_list_draw, pig_menu_draw, name_menu_open
 *   GUESS: name_menu_close, name_menu_page_build, name_menu_step, name_menu_list_draw, name_menu_draw
 *   GUESS: pig_dress_flag_tbl, pig_dress_sprite_tbl, pig_dress_row_tbl, name_menu_sprite_tbl, name_menu_row_tbl
 *   GUESS (from each body and its callers): game_mode_sub_state_set1, game_mode_flow_task
 * RESIDUALS. No row is unwritten; 12 are partial:
 *  - `lobby_flow_boot_step`: retail reaches the leave request and the lobby entry from two places each through one
 *    copy (a jump into the shared block); the source repeats both blocks (81.8; a flag variable that merges the leave
 *    paths measures 76.2); its relocations therefore run twice where retail's run once (the leave block's `isSessionFlagSet`,
 *    `system_w`, `lobby_w`, `PlayStream` and `loading_disp_set`, and the entry block's `stage_map_set`, `lobby_wp`,
 *    `my_player_no_set` and `updateLobbyEventFlags`);
 *  - `game_mode_flow_task`: retail keeps a separate `nwMoveEnd` for the arena and the quest exits, ours shares one;
 *  - `lobby_flow_leave_step`: retail calls `pollAccountLoad` with r3 unset, its declaration takes a frame (`li r3,0`);
 *  - `npc_trade_pick`: retail keeps the list start in a second register (`mr r3,r24` per case);
 *  - `pig_dress_list_build`, `name_menu_page_build`, `pig_dress_list_draw`, `name_menu_list_draw`, `name_menu_open`,
 *    `name_menu_step`, `pig_menu_step` (`toggle_word_step`'s last argument: retail masks it to 16 bits, the shared
 *    declaration takes an `s32`), `npc_gift_roll`: register choice and operand order.
 *  - `npc_gift_roll` stops its walk at a weight of 0xFF (the tables end at 0xFFFF; the weights reach 100 first).
 *   Data: the tables are defined after the bodies (the target relocates each by name, which MWCC only does for an
 *   object not yet defined); `.data`/`.sdata` emission order is not checked against the target.
 *   flipcheck: `.text` 0x281C against the claimed 0x277C (the duplicated blocks of `lobby_flow_boot_step`),
 *   extab/extabindex differing; the `.sdata`/`.sdata2` pools are partial (a candidate fold with `lobby/lb_quest_screen`,
 *   which owns the `.sdata2` word 0x8079C448 that `note_pane_player_near_ck` reads).
 * SHAPES. None.
 */

#include "types.h"
#include "menu/multi_result.h"
#include "menu/menu_message.h"
#include "sound/fn_800D7F54.h"
#include "userdata_item.h"
#include "stage/stg_w.h"
#include "stage/get_now_mapno.h"
#include "mh3_pad.h"
#include "Pl/pl_act_stage_latch_set.h"
#include "Pl/pl_act.h"
#include "ef/fn_800CDB2C.h"
#include "lobby/fn_8021E1EC.h"
#include "mh3_pad/lb_param_w.h"
#include "mh3_pad/Psw.h"
#include "Network/network_pat_control.h"
#include "fn_80040598.h"
#include "draw_shape.h"
#include "fn_80056F24.h"
#include "ef/system_core.h"
#include "sound/snd_bank_loader.h"
#include "sound/fn_800EF7D8.h"
#include "sound/fn_800E46E8.h"
#include "sound/fn_800E3CBC.h"
#include "sound/fn_800F2A94.h"
#include "enemy/em020_prog.h"
#include "menu/menu_plsearch.h"
#include "menu/Movie_open.h"
#include "lobby/lobby_scene_setup_step.h"
#include "lobby/lb_npc.h"
#include "lobby/lb_npc_func.h"
#include "lobby/lb_menu_pos_tbl.h"
#include "lobby/lb_menu_scratch_init.h"
#include "ef/eft_res.h"
#include "stage_map_set.h"
#include "gallery_open.h"
#include "camera/camera_work_init.h"
#include "Network/net_session_close.h"
#include "nw_resource.h"
#include "quest/quest_entry.h"
#include "lobby/lb_cmd_pressed_ck.h"
#include "lobby/lb_menu_open.h"
#include "lobby/lb_talk_page_open.h"
#include "lobby/LbStr.h"
#include "lobby/lb_quest_board_flash_spawn.h"
#include "hud/cockpit.h"
#include "hud/draw_font_idx.h"
#include "sound/set_zmode__FbUcb.h"
#include "fn_8004CAD8.h"
#include "Pl/fn_802693C4.h"

/* Sets the flow object's sub-state to 1 (called from the game-mode dispatcher's state machine). */
void game_mode_sub_state_set1(TaskSlot* self) {
    self->sub_0x09 = 1;
}

/* The note pane's home spots, by map kind. */
extern Vec note_pane_home_table[6];

/* The two `NoteWork` fields this unit reads, as a prefix view: `enemy/note_work.h` embeds the `MHchar` of
 * `sound/mhchar.h`, which cannot be included beside `pl.h`'s.  size: 0x196 (a view of the 0x1F8-byte record) */
struct NotePaneSpotView {
    /* +0x000 */ u8 unused_0x000[0x170];
    /* +0x170 */ nw4r::math::VEC3 pos_0x170;
    /* +0x17C */ u8 unused_0x17C[0x195 - 0x17C];
    /* +0x195 */ u8 area_0x195;
};

/* 0x803A357C (0xA0): whether the local player is up, in the pane's area, within 500 units of it and ready to talk. */
extern "C" u32 note_pane_player_near_ck(struct NoteWork* self) {
    NotePaneSpotView* pane = (NotePaneSpotView*)self;
    _PLW* me = my_player_work_get();

    if (me->slot_active == 0 || me->field_0x001 == 0) {
        return 0;
    }
    if (pane->area_0x195 != me->area_0x16) {
        return 0;
    }
    if (vec3_dist_sq(&me->vec_0x03C, &pane->pos_0x170) > 250000.0f) {
        return 0;
    }
    return me->talk_wait_0x668 != 0;
}

/* The area change's mode (1 local, 2 networked) and the area it left (0xFF when none). */
u8 lobby_area_change_mode;
u8 lobby_area_prev = 0xFF;

/* 0x803A12D4 (0xE0): resets the lobby work, its parameter block and the system's leave state, allocates the 0x2000-byte
 * screen block and loads the overlay set the lobby needs (the network one when the network control is up). */
extern "C" void lobby_flow_init(void) {
    rso_overlay_release_all();
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
    lobby_wp->kitchen_busy_0x054 = 0;
    if (system_w.net_active_0x7d4 == 1) {
        rso_overlay_mode4_load();
        initNetworkPatControl();
    } else {
        rso_overlay_mode2_load();
        allocateDialogRecord();
    }
}

/* 0x803A13B4 (0x2CC): the game-mode task while the lobby runs: sets the lobby mode up, then dispatches its sub-states
 * (boot, lobby, leave), and on leaving starts the quest or arena mode or goes back to the title. */
extern "C" void game_mode_flow_task(TaskSlot* task) {
    s32 i;

    system_stream_count_step();
    nwMoveStart();
    switch (task->step_0x08) {
    case 0:
        hbm_disable();
        GameMode_set(2);
        lobby_flow_init();
        snd_bank_layout(0);
        quest_init(0);
        kbd_init(0);
        task->step_0x08++;
        task->sub_0x09 = 0;
        break;
    case 1:
        switch (task->sub_0x09) {
        case 0:
            game_mode_sub_state_set1(task);
            break;
        case 1:
            lobby_flow_boot_step(task);
            break;
        case 2:
            lobby_flow_main_step(task);
            break;
        case 3:
            lobby_flow_leave_step(task);
            break;
        case 4:
            if (lobby_w.field_0x0B0 == 3 && NetCtrlWk::getLobbyReadyByte() != 0) {
                task->sub_0x09 = 3;
                task->step_0x0A = 100;
                break;
            }
            for (i = 0; i < 3; i++) {
                if (lobby_w.se_0x0B4[i] != NULL) {
                    se_handle_clear((u8*)lobby_w.se_0x0B4[i]);
                }
            }
            if (lobby_w.field_0x0B0 == 3) {
                prim_init_all();
                system_scene_reset();
                lobby_flow_init();
                snd_bank_layout(0);
                quest_init(0);
                task->sub_0x09 = 1;
                task->step_0x0A = 0;
                NetCtrlWk::raiseSecondRequestFlag();
                rso_overlay_mode4_load();
                clear_qResult_work();
                return;
            }
            kbd_exit();
            resetControlFields();
            game_reset_to_title();
            system_w.title_req_0x86d = 0;
            system_w.reset_func_0x8c4();
            return;
        case 6:
            switch (task->step_0x0A) {
            case 0:
                task->step_0x0A = 2;
            case 1:
                task->step_0x0A = 2;
            case 2:
                for (i = 0; i < 3; i++) {
                    if (lobby_w.se_0x0B4[i] != NULL) {
                        se_handle_clear((u8*)lobby_w.se_0x0B4[i]);
                    }
                }
                setTransferMode(0);
                lb_npc_model_release_all();
                system_full_reset();
                clear_qResult_work();
                system_w.leave_state_0x7d8 = lb_param_w.field_0x00;
                lb_param_w.rank_sel_0x09 = lobby_w.item_list_selection_0x162;
                if ((u16)(lb_param_w.field_0x00 + 0x15A0) <= 11) {
                    system_w.net_session_0x90f = 1;
                    PlayMode_set(6);
                    GameMode_set(3);
                    ArenaSelExec();
                } else {
                    PlayMode_set(6);
                    GameMode_set(1);
                    GameModeExec();
                }
                nwMoveEnd();
                return;
            }
            break;
        }
        break;
    }
    nwMoveEnd();
}

/* Asks the game-mode flow to leave the lobby: the leave kind by the session's state, then sub-state 3. */
static inline void lobby_flow_leave_req(TaskSlot* task) {
    if ((u32)NetCtrlWk::isSessionFlagSet() == 1) {
        lobby_w.field_0x0B0 = 4;
    } else {
        lobby_w.field_0x0B0 = 2;
    }
    system_w.field_0x7ce = 3;
    lobby_w.leave_wait_0x174 = 15;
    task->sub_0x09 = 3;
    task->step_0x0A = 0;
    PlayStream(1, 1);
    loading_disp_set(1, 0);
}

/* 0x803A168C (0x478): game-mode flow sub-state 1, booting the lobby: loads the textures and the lobby BGM, waits for
 * the network or the opening movie, then enters the lobby map. */
extern "C" void lobby_flow_boot_step(TaskSlot* task) {
    prim_init_all();
    switch (task->step_0x0A) {
    case 0:
        gpframe_tex_load();
        menu_tex_load();
        itemicon_tex_load();
        lobby_w.net_wait_0x164 = 0;
        if (system_w.field_0x7d5 == 0) {
            lobby_bgm_load();
            task->step_0x0A++;
        } else {
            my_player_no_set(0);
            em020_quest_pages_clear();
            NetCtrlWk::restartSession();
            system_w.field_0x8b1 = 1;
            task->flag_0x14 = 1;
            system_w.field_0x7d5 = 0;
            task->step_0x0A = 5;
            if (NetCtrlWk::getLobbyReadyByte() == 0) {
                loading_disp_set(1, 2);
            }
        }
        break;
    case 1:
        if (file_loading_ck(NULL, NULL) != 1) {
            PlayStream(1, 90);
            task->flag_0x14 = 0;
            task->step_0x0A++;
            system_w.leave_flag_0x7d6 = 0;
            system_w.field_0x8b1 = 0;
        }
        break;
    case 2:
        if ((u32)isCityMode() == 1) {
            task->step_0x0A = 3;
            task->sub_step_0x0B = 0;
            system_w.field_0x7d5 = 0;
            PlayStream(1, 1);
            break;
        }
        invokeResultCallback();
        lobby_w.field_0x163 = NetCtrlWk::getLobbyReadyByte();
        em020_unknown_flag_set(0);
        switch (lobby_w.field_0x163) {
        case 1:
            lobby_w.net_err_0x168 = NULL;
            subTransSetPrio(9, (u32)lobby_net_err_draw, 0, NULL);
            break;
        case 2:
            if (lobby_w.net_wait_0x164 < 60) {
                lobby_w.net_wait_0x164++;
            } else if (system_w.field_0x865 == 0) {
                em020_unknown_flag_set(1);
                if (Psw[0].button_0x2C0.pressed_0x04 & 0x10) {
                    sysSE_req(0);
                    lobby_flow_leave_req(task);
                    return;
                }
            }
            lobby_w.net_err_0x168 = NetCtrlWk::getErrorMessage();
            if (lobby_w.net_err_0x168 != NULL) {
                subTransSetPrio(9, (u32)lobby_net_err_draw, 0, NULL);
            }
            break;
        }
        if (system_w.field_0x865 != 1 && NetCtrlWk::isRequestStateTwo() == 1) {
            lobby_flow_leave_req(task);
        }
        break;
    case 3:
        switch (task->sub_step_0x0B) {
        case 0:
            if (isOnlineFlagClear() != 0) {
                movie_bgm_load();
                task->sub_step_0x0B++;
            } else {
                stage_map_set(0x15);
                lobby_w.leave_wait_0x174 = 0;
                task->sub_0x09 = 2;
                task->step_0x0A = 0;
                task->sub_step_0x0B = 0;
                lobby_wp->field_0x007 = 0;
                lobby_wp->field_0x006 = 0;
                my_player_no_set(0);
                NetCtrlWk::updateLobbyEventFlags();
            }
            break;
        case 1:
            if (srt_ready_ck(0) != 0) {
                Movie_open(MOVIE_INDEX_OPENING);
                PlayStream(0, 14);
                task->sub_step_0x0B++;
            }
            break;
        case 2:
            if (Check_movie_finish() == 1) {
                Movie_close();
                userdata_opening_seen_set();
                gallery_open(5);
                bgm_stop_all();
                task->step_0x0A++;
                task->sub_step_0x0B = 0;
            } else {
                subTransSet((u32)Movie_draw_sub, 0, NULL);
            }
            break;
        }
        break;
    case 4:
        stage_map_set(0x15);
        lobby_w.leave_wait_0x174 = 0;
        task->sub_0x09 = 2;
        task->step_0x0A = 0;
        task->sub_step_0x0B = 0;
        lobby_wp->field_0x007 = 0;
        lobby_wp->field_0x006 = 0;
        my_player_no_set(0);
        NetCtrlWk::updateLobbyEventFlags();
        break;
    case 5:
        lobby_w.field_0x163 = NetCtrlWk::getLobbyReadyByte();
        if (lobby_w.field_0x163 != 0) {
            loading_disp_set(0, 0);
        } else if (NetCtrlWk::isConnectionSettled() == 0) {
            break;
        }
        stage_map_set(0x15);
        lobby_w.leave_wait_0x174 = 0;
        task->sub_0x09 = 2;
        task->step_0x0A = 0;
        task->sub_step_0x0B = 0;
        lobby_wp->field_0x007 = 0;
        lobby_wp->field_0x006 = 0;
        NetCtrlWk::updateLobbyEventFlags();
        break;
    }
}

/* Shows the loading screen kind the lobby's map state asks for (2 for map 1, 3 for map 2, else 0). */
static inline void lobby_flow_loading_show(void) {
    if (system_w.field_0x8b1 == 1) {
        if (lobby_w.field_0x002 == 1) {
            loading_disp_set(1, 2);
        } else if (lobby_w.field_0x002 == 2) {
            loading_disp_set(1, 3);
        } else {
            loading_disp_set(1, 0);
        }
    } else {
        loading_disp_set(1, 0);
    }
}

/* Leaves the lobby towards the game-mode flow's leave sub-state with a fade. */
static inline void lobby_flow_fade_leave(TaskSlot* task) {
    lobby_w.leave_wait_0x174 = 15;
    fade_set(0, 5);
    resumeSoundEngine();
}

/* 0x803A1B04 (0x518): game-mode flow sub-state 2, the lobby itself: sets the scene up, then runs the lobby's frame
 * until a leave, a server change or an area change is asked for. */
extern "C" void lobby_flow_main_step(TaskSlot* task) {
    prim_init_all();
    if (lobby_wp->field_0x0B0 == 1) {
        lobby_flow_fade_leave(task);
        NetCtrlWk::setRequestStateOne();
        task->sub_0x09 = 3;
        task->step_0x0A = 0;
        loading_disp_set(1, 0);
        return;
    }
    if (lobby_w.field_0x0B0 == 2) {
        lobby_flow_fade_leave(task);
        task->sub_0x09 = 3;
        task->step_0x0A = 0;
        loading_disp_set(1, 0);
        return;
    }
    if (lobby_w.field_0x0B0 == 3) {
        lobby_flow_fade_leave(task);
        task->sub_0x09 = 3;
        task->step_0x0A = 0;
        loading_disp_set(1, 0);
        return;
    }
    if (system_w.field_0x7d3 != 1 || isServerSelectState() != 1) {
        if (lobby_wp->area_change_0x026 == 1) {
            task->step_0x0A = 4;
            task->sub_step_0x0B = 0;
            lobby_wp->area_change_0x026 = 0;
        }
        switch (task->step_0x0A) {
        case 0:
            if (lobby_scene_setup_step(task) == 0) {
                if (system_w.leave_flag_0x7d6 == 0) {
                    loading_disp_set(1, 0);
                } else {
                    loading_disp_set(1, 3);
                }
            } else if (em020_quest_active_ck() == 0) {
                if (system_w.leave_flag_0x7d6 == 0) {
                    loading_disp_set(1, 0);
                } else {
                    loading_disp_set(1, 3);
                }
            } else {
                loading_disp_set(0, 0);
            }
            if (task->step_0x0A != 0) {
                se_work_init();
                bgm_ctrl_init();
                scene_se_bank_load(get_now_mapno(), 0);
                snd_player_banks_load();
                lobby_w.se_0x0B4[0] = (struct _se_w*)se_entry_request(12, (struct _ENEMY_WORK*)&lobby_w,
                                                                      lb_npc_func.se_func_0x00);
                lobby_w.se_0x0B4[1] = (struct _se_w*)se_entry_request(13, (struct _ENEMY_WORK*)&lobby_w, NULL);
                lobby_w.se_0x0B4[2] = (struct _se_w*)se_entry_request(14, (struct _ENEMY_WORK*)&lobby_w, NULL);
                bgm_behind_flag_clear();
                lobby_w.field_0x12C = 1;
                lobby_w.busy_0x172 = 10;
                GlareFilter_on();
                filter_panel_on();
                task->sub_step_0x0B = 0;
            }
            break;
        case 1:
            if ((u32)NetCtrlWk::isServerSelectSubState() == 1) {
                loading_disp_set(1, 0);
                if (lobby_w.field_0x163 != 0 || lobby_w.field_0x16F != 0) {
                    lobby_w.field_0x0B0 = 3;
                    system_w.field_0x7ce = 3;
                    lobby_w.leave_wait_0x174 = 15;
                    task->sub_0x09 = 3;
                    task->step_0x0A = 0;
                    PlayStream(1, 1);
                }
                break;
            }
            if (lobby_w.field_0x12C == 1) {
                if (task->sub_step_0x0B == 0) {
                    lobby_flow_loading_show();
                } else {
                    fade_set(0, 2);
                }
                pad_input_clear(0);
            } else if (lobby_w.field_0x163 != 0 || lobby_w.field_0x16F != 0) {
                loading_disp_set(0, 0);
                fade_reset(0);
                task->sub_step_0x0B = 1;
            } else if (em020_quest_active_ck() == 0) {
                lobby_flow_loading_show();
                pad_input_clear(0);
            } else if (task->sub_step_0x0B == 0) {
                loading_disp_set(0, 0);
                fade_set(0, 2);
                task->sub_step_0x0B = 1;
            }
            if (lobby_w.busy_0x172 != 0) {
                lobby_w.busy_0x172--;
                pad_input_clear(0);
            }
            lobby_frame_update();
            bgm_ctrl_frame();
            se_frame_step();
            lb_event_schedule_step();
            break;
        case 4:
            lobby_area_change_step(task);
            break;
        }
    } else {
        em020_quest_active_clear();
        system_w.field_0x7d5 = 1;
        system_w.field_0x7cf = 0x15;
        prim_init_all();
        filter_reset();
        task->sub_0x09 = 6;
        task->step_0x0A = 0;
    }
}

/* 0x803A201C (0x10): asks for a networked area change. */
extern "C" void lobby_area_change_net_req(void) {
    lobby_area_change_mode = 2;
    setTransferMode(0);
}

/* 0x803A202C (0x10): asks for a local area change. */
extern "C" void lobby_area_change_req(void) {
    lobby_area_change_mode = 1;
    setTransferMode(0);
}

/* 0x803A203C (0x20C): the area change: releases the current area, loads the next one into a free stage slot, then
 * enters it, places the player and the NPCs and brings the lobby back. */
extern "C" void lobby_area_change_step(TaskSlot* task) {
    s8 slot;

    if (lobby_area_change_mode == 2) {
        loading_disp_set(1, 2);
    }
    switch (task->sub_step_0x0B) {
    case 0:
        setSoftresetFlag(false);
        eft_res_release_all();
        filter_flag_clear();
        filter_panel_flag_clear();
        stage_area_slot_release((s8)lobby_wp->slots_0x00C[lobby_wp->field_0x014]);
        lobby_wp->slots_0x00C[lobby_wp->field_0x014] = -1;
        lobby_area_prev = lobby_wp->area_0x002;
        task->sub_step_0x0B++;
        kbd_close_call();
        break;
    case 1:
        slot = stage_area_free_slot_get();
        if (slot >= 0) {
            lobby_wp->slots_0x00C[lobby_wp->field_0x014] = slot;
            lobby_wp->area_prev_0x016 = lobby_wp->area_0x002;
            lobby_wp->area_0x002 = lobby_wp->area_next_0x015;
            stage_area_load((u8)slot, lobby_wp->field_0x001, lobby_wp->area_0x002);
            task->sub_step_0x0B++;
        }
        break;
    case 2:
        if (file_loading_ck(NULL, NULL) != 1 &&
            (lobby_area_change_mode != 2 || NetCtrlWk::getLobbyReadyByte() != 0 ||
             NetCtrlWk::isConnectionSettled() != 0) &&
            NetCtrlWk::pollBigDataFetch() != 0) {
            stage_area_enter(lobby_wp->field_0x001, lobby_wp->area_0x002);
            lb_player_spawn_set(lobby_wp->spawn_0x018, lobby_wp->spawn_id_0x024, lobby_wp->field_0x001,
                                lobby_wp->area_0x002);
            lb_npc_model_release_all();
            lb_npc_map_setup();
            camera_area_reset();
            lb_menu_scratch_init(1);
            snd_area_bank_load(lobby_wp->field_0x001, lobby_wp->area_0x002);
            task->step_0x0A = 1;
            task->sub_step_0x0B = 0;
            if (lobby_area_change_mode != 2) {
                fade_set(0, 2);
            }
            filter_panel_on();
            lobby_area_prev = 0xFF;
            lobby_area_change_mode = 0;
            setSoftresetFlag(true);
            setTransferMode(1);
            kbd_reset_call();
        }
        break;
    }
}

/* 0x803A2248 (0x38C): game-mode flow sub-state 3, leaving the lobby: waits out the leave delay, then saves (account
 * and system files) or waits for the friend sync, and on a dropped session offers the error window. */
extern "C" void lobby_flow_leave_step(TaskSlot* task) {
    switch (task->step_0x0A) {
    case 0:
        if (lobby_w.leave_wait_0x174 > 0) {
            lobby_w.leave_wait_0x174--;
            break;
        }
        if (lobby_w.field_0x0B0 < 3) {
            if (NetCtrlWk::isRequestStateTwo() == 0) {
                break;
            }
            loading_disp_set(0, 0);
        } else if (lobby_w.field_0x0B0 == 3) {
            if (NetCtrlWk::getLobbyReadyByte() != 0) {
                task->step_0x0A = 100;
                loading_disp_set(0, 0);
            } else {
                requestFriendSync(&lobby_w.friend_sync_0x16D);
                task->step_0x0A = 4;
            }
            break;
        } else if (lobby_w.field_0x0B0 == 4) {
            task->step_0x0A = 10;
            system_w.field_0x865 = 1;
            break;
        }
        system_w.field_0x865 = 1;
        task->step_0x0A++;
        break;
    case 1:
        startAccountLoad();
        task->step_0x0A++;
        break;
    case 2:
        if (pollAccountLoad(0) != 0) {
            game_system_file_create_start();
            task->step_0x0A++;
        }
        break;
    case 3:
        if (game_system_file_create_wait() != 0) {
            system_w.field_0x865 = 0;
            task->sub_0x09 = 4;
            task->step_0x0A = 0;
            loading_disp_set(0, 0);
        }
        break;
    case 4:
        if (NetCtrlWk::getLobbyReadyByte() != 0) {
            task->step_0x0A = 100;
        } else if (lobby_w.friend_sync_0x16D != 0) {
            task->sub_0x09 = 4;
            task->step_0x0A = 0;
            loading_disp_set(0, 0);
        }
        break;
    case 10:
        game_data_file_create_start();
        task->step_0x0A++;
        break;
    case 11:
        if (game_save_wait() != 0) {
            task->step_0x0A = 1;
        }
        break;
    case 100:
        prim_init_all();
        filter_reset();
        fade_reset(0);
        loading_disp_set(0, 0);
        task->step_0x0A++;
        break;
    case 101:
        prim_init_all();
        em020_unknown_flag_set(0);
        lobby_w.field_0x163 = NetCtrlWk::getLobbyReadyByte();
        switch (lobby_w.field_0x163) {
        case 1:
            lobby_w.net_err_0x168 = NULL;
            subTransSetPrio(9, (u32)lobby_net_err_draw, 0, NULL);
            break;
        case 2:
            if (lobby_w.net_wait_0x164 < 60) {
                lobby_w.net_wait_0x164++;
            } else if (system_w.field_0x865 == 0) {
                em020_unknown_flag_set(1);
                if (Psw[0].button_0x2C0.pressed_0x04 & 0x10) {
                    sysSE_req(0);
                    lobby_w.field_0x0B0 = 2;
                    system_w.field_0x7ce = 3;
                    lobby_w.leave_wait_0x174 = 15;
                    task->sub_0x09 = 3;
                    task->step_0x0A = 0;
                    PlayStream(1, 1);
                    break;
                }
            }
            lobby_w.net_err_0x168 = NetCtrlWk::getErrorMessage();
            if (lobby_w.net_err_0x168 != NULL) {
                subTransSetPrio(9, (u32)lobby_net_err_draw, 0, NULL);
            }
            break;
        }
        break;
    }
}

/* The Poogie menu's costume unlock flags (0xFFFF: always worn), its list sprites and rows, and the name menu's frame
 * sprites and rows. */
extern u16 pig_dress_flag_tbl[8];
extern u16 pig_dress_sprite_tbl[10];
extern u16 pig_dress_row_tbl[8];
extern u16 name_menu_sprite_tbl[11];
extern u16 name_menu_row_tbl[5];

/* 0x803A25D4 (0x10): stores the Poogie costume the lobby shows. */
extern "C" void lobby_pig_dress_view_set(u8 dress) {
    lobby_w.pig_dress_0x078 = dress;
}

/* 0x803A25E4 (0x40): passes costume `dress` to the Poogie NPC, when it is in the lobby. */
extern "C" void pig_dress_npc_set(u8 dress) {
    struct _LB_NPC* npc = lb_npc_find(0x49);

    if (npc != NULL) {
        lb_npc_event_set(npc, dress);
    }
}

/* 0x803A2624 (0x84): opens the Poogie menu for `player` at `npc`: clears the screen block, waits 300 frames for the
 * player to come close and puts the player in act 17. */
extern "C" void pig_menu_open(struct _PLW* player, struct _LB_NPC* npc) {
    LbPigMenuWork* w = lobby_w.pig_menu_0x0AC;

    memset(w, 0, 0x2000);
    w->player_0x58 = player;
    w->npc_0x5C = npc;
    w->wait_0x06 = 300;
    lobby_w.state_0x000 = 0x15;
    lobby_w.active_0x008 = 1;
    lb_npc_act_set(player, 0, 17, 0);
}

/* 0x803A26A8 (0x24): closes the Poogie menu: sends the profile and refreshes the NPCs. */
extern "C" void pig_menu_close(void) {
    em020_profile_send();
    lb_panel_close();
}

/* 0x803A26CC (0x4EC): one step of the Poogie menu: the walk-up, the top menu (pet, costume), the petting motion and
 * the costume list with its confirmation. */
extern "C" void pig_menu_step(void) {
    LbPigMenuWork* w = lobby_w.pig_menu_0x0AC;
    LbActRow* row;
    s16 choice;

    switch (w->state_0x00) {
    case 0:
        w->player_0x58->field_0x0A8 = lb_player_angle_to(w->player_0x58, &w->npc_0x5C->pos_0x10);
        if (calcDistanceSqXZ(&w->player_0x58->vec_0x03C, &w->npc_0x5C->pos_0x10) < 10000.0f &&
            Pl_frame_check(w->player_0x58, 1, 120.0f, 0.0f) == 1) {
            w->state_0x00++;
            if (game_ready_ck() == 1) {
                lb_list_init(&w->menu_0x0C, 2, 0, 457, 429, 0, 0, 3, 0);
            } else {
                lb_list_init(&w->menu_0x0C, 3, 0, 456, 428, 0, 0, 3, 0);
            }
        } else if (w->wait_0x06 <= 0) {
            lb_npc_act_set(w->player_0x58, 0, 18, 0);
            pig_menu_close();
        } else {
            w->wait_0x06--;
        }
        break;
    case 1:
        ainpc_page_hold_set();
        switch (lb_choice_step(&w->menu_0x0C)) {
        case 1:
            if (game_ready_ck() == 1) {
                choice = w->menu_0x0C.cursor_0x00 + 1;
            } else {
                choice = w->menu_0x0C.cursor_0x00;
            }
            switch (choice) {
            case 0:
                w->state_0x00 = 4;
                lb_player_act_latch(w->player_0x58);
                w->wait_0x06 = 30;
                lb_npc_act_set(w->player_0x58, 12, 7, 0);
                break;
            case 1:
                w->state_0x00 = 3;
                w->petted_0x02 = 0;
                w->pet_row_0x54 = (LbActRow*)pl_act_name_row_get(w->player_0x58, 0, 1);
                w->frame_0x04 = 0;
                se_ch2_req(0);
                break;
            case 2:
                w->state_0x00 = 2;
                w->sub_0x01 = 0;
                pig_dress_list_build(w);
                break;
            }
            break;
        case 2:
            lb_npc_act_set(w->player_0x58, 0, 18, 0);
            w->state_0x00 = 5;
            lb_talk_page_open(0);
            break;
        }
        break;
    case 3:
        row = w->pet_row_0x54;
        if (++w->frame_0x04 >= row->start_0x02 + row->param_0x04) {
            w->state_0x00 = 4;
            w->petted_0x02 = 1;
            w->wait_0x06 = 20;
            sysSE_bank32_req(27);
            lb_npc_motion_restart(w->npc_0x5C, 49);
        } else {
            row = w->pet_row_0x54;
            if (w->frame_0x04 == row->start_0x02) {
                lb_quest_board_flash_spawn(w->npc_0x5C, row->param_0x04, row->param_0x06, row->param_0x08);
            }
            if (lb_cmd_pressed_ck(0x10) != 0) {
                if (w->frame_0x04 < w->pet_row_0x54->start_0x02) {
                    w->state_0x00 = 5;
                    lb_npc_act_set(w->player_0x58, 0, 18, 0);
                    lb_talk_page_open(0);
                    sysSE_bank32_req(26);
                    se_ch2_req(2);
                } else {
                    lb_npc_act_set(w->player_0x58, 0, 18, 0);
                    lb_npc_motion_restart(w->npc_0x5C, 51);
                    se_ch2_req(1);
                    pig_menu_close();
                }
            }
        }
        break;
    case 2:
        ainpc_page_hold_set();
        switch (w->sub_0x01) {
        case 0:
            switch (lb_choice_step(&w->dress_0x2C)) {
            case 1:
                w->sub_0x01++;
                w->confirm_0x08 = 0;
                break;
            case 2:
                w->state_0x00 = 1;
                break;
            }
            break;
        case 1:
            switch (toggle_word_step(&w->confirm_0x08, lobby_cmd_trig_get(), 4, 8, 0xFFFF)) {
            case 1:
                lb_npc_act_set(w->player_0x58, 0, 18, 0);
                w->state_0x00 = 5;
                ((Q_UserData*)lobby_world_block)->pig_dress_0x483B = w->dress_ids_0x4C[w->dress_0x2C.cursor_0x00];
                lobby_pig_dress_view_set(((Q_UserData*)lobby_world_block)->pig_dress_0x483B);
                lb_npc_event_set(w->npc_0x5C, ((Q_UserData*)lobby_world_block)->pig_dress_0x483B);
                lb_talk_page_open(3);
                sysSE_stop(42);
                break;
            case 2:
                w->sub_0x01 = 0;
                break;
            }
            break;
        }
        break;
    case 4:
        if (--w->wait_0x06 <= 0) {
            if (w->petted_0x02 != 0) {
                lb_npc_act_set(w->player_0x58, 0, 25, 0);
            }
            pig_menu_close();
        }
        break;
    case 5:
        if ((u32)(lb_talk_page_mode_reset() - 1) <= 1) {
            pig_menu_close();
        }
        break;
    }
    subTransSet((u32)pig_menu_draw, 0, NULL);
}

/* 0x803A2BB8 (0x100): builds the costume list: every costume whose unlock flag is up (or that needs none), with the
 * one the Poogie wears greyed out. */
extern "C" void pig_dress_list_build(LbPigMenuWork* w) {
    u16 worn;
    u8 i;
    u8 count;

    memset(w->dress_ids_0x4C, 0xFF, sizeof(w->dress_ids_0x4C));
    count = 0;
    worn = 0;
    i = 0;
    do {
        if (i == ((Q_UserData*)lobby_world_block)->pig_dress_0x483B) {
            worn |= (u16)(1 << count);
        }
        if (pig_dress_flag_tbl[i] == 0xFFFF) {
            w->dress_ids_0x4C[count] = i;
            count++;
        } else if (userdata_progress_flag_ck(pig_dress_flag_tbl[i]) != 0) {
            w->dress_ids_0x4C[count] = i;
            count++;
        }
        i++;
    } while (i < 8);
    lb_list_init(&w->dress_0x2C, (s16)count, 0, 459, -1, worn, 0, 3, 0);
}

/* 0x803A2CB8 (0x118): draws the costume list and the Poogie's name. */
extern "C" void pig_dress_list_draw(LbPigMenuWork* w, bool active) {
    _mh_ivec2_ pos;
    s32 i;
    const u16* row;
    bool sel;
    bool cur;
    u32 color;

    get_lsp_data(9387, &pos);
    draw_sprite_anim_ary(pig_dress_sprite_tbl, w->dress_0x2C.count_0x06, &pos);
    draw_font_idx(9388, (s8*)lb_pig_name_get(((Q_UserData*)lobby_world_block)->pig_name_0x483A), 4, &pos);
    for (i = 0, row = pig_dress_row_tbl; i < w->dress_0x2C.count_0x06; row++, i++) {
        if (w->dress_0x2C.cursor_0x00 == i) {
            sel = active;
            cur = true;
        } else {
            sel = false;
            cur = false;
        }
        color = GetMenuFontColor((w->dress_0x2C.off_mask_0x02 & (1 << i)) == 0, cur, active, 0);
        lb_frame_draw_at(9387, *row, (s8*)LbStr(0, (u16)(w->dress_0x2C.str_base_0x0A + w->dress_ids_0x4C[i])), sel, color,
                         w->dress_0x2C.kind_0x08);
    }
}

/* 0x803A2DD0 (0x140): draws the Poogie menu: the top list or the costume list with its help line and the yes/no. */
extern "C" void pig_menu_draw(void) {
    LbPigMenuWork* w = lobby_w.pig_menu_0x0AC;
    _mh_ivec2_ pos;

    set_zmode(false, 0, false);
    set_blendmode(4, 5, 1);
    switch (w->state_0x00) {
    case 1:
        lb_choice_draw(&w->menu_0x0C, 9387, pig_dress_sprite_tbl, pig_dress_row_tbl, 6263, 0);
        get_lsp_data(9387, &pos);
        draw_font_idx(9388, (s8*)lb_pig_name_get(((Q_UserData*)lobby_world_block)->pig_name_0x483A), 4, &pos);
        break;
    case 2:
        switch (w->sub_0x01) {
        case 0:
            pig_dress_list_draw(w, 1);
            lb_panel_msg_draw(6263, 431);
            if (((Q_UserData*)lobby_world_block)->pig_dress_0x483B == w->dress_ids_0x4C[w->dress_0x2C.cursor_0x00]) {
                lb_panel_str_print(6263, 432, 2);
            }
            break;
        case 1:
            pig_dress_list_draw(w, 0);
            lb_panel_msg_draw(6263, 433);
            lb_panel_yes_no_draw(6265, w->confirm_0x08);
            break;
        }
        break;
    }
}

/* 0x803A2F10 (0xC4): opens the name menu for `player` at `npc` (a kitchen Felyne or the Poogie) on the name it has. */
extern "C" void name_menu_open(struct _PLW* player, struct _LB_NPC* npc) {
    LbNameMenuWork* w = lobby_w.name_menu_0x0AC;
    u32 cat;

    memset(w, 0, 0x2000);
    w->player_0x24 = player;
    w->npc_0x28 = npc;
    lobby_w.state_0x000 = 0x16;
    lobby_w.active_0x008 = 1;
    cat = npc->field_0x002 - 0x14;
    if (cat > 2) {
        if ((s32)npc->field_0x002 == 0x17 || (s32)npc->field_0x002 == 0x49) {
            w->current_0x0E = ((Q_UserData*)lobby_world_block)->pig_name_0x483A;
        }
    } else {
        w->current_0x0E = ((Q_UserData*)lobby_world_block)->cat_name_0x3F95[cat];
    }
    name_menu_page_build(w, 0);
    lb_menu_open(npc);
    lb_talk_page_open(2);
}

/* 0x803A2FD4 (0x4): closes the name menu. */
extern "C" void name_menu_close(void) {
    lb_panel_close();
}

/* 0x803A2FD8 (0x108): fills page `page` of the name menu (five names a page) from the NPC kind's name table. */
extern "C" void name_menu_page_build(LbNameMenuWork* w, s32 page) {
    s16 first = (s16)page * 5;
    s16 last = first + 4;
    u8 kind;
    s16 i;
    s32 ofs;

    w->rows_0x0A = 0;
    kind = w->npc_0x28->field_0x002;
    if ((u32)(kind - 0x14) > 2) {
        if ((s32)kind == 0x17 || (s32)kind == 0x49) {
            w->names_0x20 = lb_pig_name;
        }
    } else {
        w->names_0x20 = lb_cat_name;
    }
    for (i = 0, ofs = 0; *(s32*)((u8*)w->names_0x20 + ofs) != 0; ofs += 4, i++) {
        if (first <= i && i <= last) {
            w->row_ids_0x10[w->rows_0x0A] = i;
            w->rows_0x0A++;
        }
    }
    if (w->cursor_0x02 >= w->rows_0x0A) {
        w->cursor_0x02 = w->rows_0x0A - 1;
    }
    w->total_0x04 = i;
    w->page_0x06 = page;
    w->pages_0x08 = menu_page_count(i, 5);
}

/* 0x803A30E0 (0x294): one step of the name menu: the greeting, the list (pages, rows, decide, cancel), the yes/no and
 * the closing message. */
extern "C" void name_menu_step(void) {
    LbNameMenuWork* w = lobby_w.name_menu_0x0AC;
    u8 kind;

    w->moved_0x0C = 0;
    switch (w->state_0x00) {
    case 0:
        if ((u32)(lb_talk_page_mode_reset() - 1) <= 1) {
            if (lb_talk_page_value_get() == 0) {
                w->state_0x00 = 1;
                sysSE_req(0);
            } else {
                w->state_0x00 = 3;
                sysSE_req(1);
                lb_talk_page_open(0);
            }
        }
        break;
    case 1:
        ainpc_page_hold_set();
        if (lb_cmd_pressed_ck(0x10) != 0) {
            if (w->current_0x0E != w->row_ids_0x10[w->cursor_0x02]) {
                w->state_0x00 = 2;
                w->confirm_0x1C = 0;
                sysSE_req(0);
            } else {
                sysSE_req(2);
            }
        } else if (lb_cmd_pressed_ck(0x20) != 0) {
            w->state_0x00 = 3;
            lb_talk_page_open(0);
            sysSE_req(1);
        } else if (lb_cmd_repeat_ck(0xC) != 0) {
            w->page_0x06 = menu_cursor_step_fixed_tail(w->page_0x06, w->pages_0x08, lb_cmd_repeat_get(), 4, 8,
                                                       &w->moved_0x0C);
            name_menu_page_build(w, w->page_0x06);
        } else if (lb_cmd_repeat_ck(3) != 0) {
            w->cursor_0x02 = menu_cursor_step(w->cursor_0x02, w->rows_0x0A, lb_cmd_repeat_get(), 1, 2);
        }
        break;
    case 2:
        ainpc_page_hold_set();
        switch (lb_yes_no_step(&w->confirm_0x1C)) {
        case 1:
            w->state_0x00 = 3;
            lb_talk_page_open(3);
            kind = w->npc_0x28->field_0x002;
            if ((u32)(kind - 0x14) > 2) {
                if ((s32)kind == 0x17 || (s32)kind == 0x49) {
                    ((Q_UserData*)lobby_world_block)->pig_name_0x483A = w->row_ids_0x10[w->cursor_0x02];
                    lb_npc_motion_restart(w->npc_0x28, 50);
                }
            } else {
                ((Q_UserData*)lobby_world_block)->cat_name_0x3F95[kind - 0x14] = w->row_ids_0x10[w->cursor_0x02];
            }
            break;
        case 2:
            w->state_0x00 = 1;
            break;
        }
        break;
    case 3:
        if ((u32)(lb_talk_page_mode_reset() - 1) <= 1) {
            name_menu_close();
        }
        break;
    }
    subTransSet((u32)name_menu_draw, 0, NULL);
}

/* 0x803A3374 (0x130): draws the name menu's page: its frame, the page arrows and the page's names. */
extern "C" void name_menu_list_draw(LbNameMenuWork* w, bool active) {
    _mh_ivec2_ base;
    _mh_ivec2_ pos;
    s16 i;
    const u16* row;
    bool sel;
    u32 color;

    get_lsp_data(9369, &base);
    draw_sprite_ary(name_menu_sprite_tbl, &base);
    lb_page_arrow_draw(9368, w->page_0x06, w->pages_0x08, w->moved_0x0C, active);
    for (i = 0, row = name_menu_row_tbl; i < w->rows_0x0A; row++, i++) {
        get_lsp_data(*row, &pos);
        pos.x += base.x;
        pos.y += base.y;
        color = GetMenuFontColor(w->current_0x0E != w->row_ids_0x10[i], sel = i == w->cursor_0x02, active, 0);
        if (!active) {
            sel = false;
        }
        lb_list_row_draw((s8*)w->names_0x20[w->row_ids_0x10[i]], sel, &pos, color, 0);
    }
}

/* 0x803A34A4 (0xD8): draws the name menu: the list with its help line (and the current name's mark) or the yes/no. */
extern "C" void name_menu_draw(void) {
    LbNameMenuWork* w = lobby_w.name_menu_0x0AC;

    set_zmode(false, 0, false);
    set_blendmode(4, 5, 1);
    switch (w->state_0x00) {
    case 1:
        name_menu_list_draw(w, 1);
        lb_panel_msg_draw(6263, 434);
        if (w->current_0x0E == w->row_ids_0x10[w->cursor_0x02]) {
            lb_panel_line_draw(6263, 435, 2, 2);
        }
        break;
    case 2:
        name_menu_list_draw(w, 0);
        lb_panel_msg_draw(6263, 436);
        lb_panel_yes_no_draw(6265, w->confirm_0x1C);
        break;
    }
}

/* One swap the quest NPC offers: the item it wants and the item it gives for it; 0xFFFF ends a list.
 * size: 0x4 */
struct NpcTrade {
    /* +0x0 */ u16 want;
    /* +0x2 */ u16 give;
};

/* One weighted gift roll: its weight out of 100 and the item; 0xFFFF ends a list.  size: 0x4 */
struct NpcGift {
    /* +0x0 */ u16 weight;
    /* +0x2 */ u16 item;
};

/* The NPC's swap lists per map kind (1..5), the special swaps of the carried quest items, and the gift rolls per map
 * kind. */
extern NpcTrade npc_trade_tbl_1[9];
extern NpcTrade npc_trade_tbl_2[5];
extern NpcTrade npc_trade_tbl_3[6];
extern NpcTrade npc_trade_tbl_4[4];
extern NpcTrade npc_trade_special_tbl[5];
extern NpcGift npc_gift_tbl_1[13];
extern NpcGift npc_gift_tbl_2[13];
extern NpcGift npc_gift_tbl_3[13];
extern NpcGift npc_gift_tbl_4[13];
extern NpcGift npc_gift_tbl_5[13];
extern NpcTrade npc_trade_tbl_5[2];

/* 0x803A361C (0x98): when the player carries a quest item, settles its special swap (the first one the player has
 * the item for). */
extern "C" u8 npc_trade_special_ck(_PLW* me) {
    NpcTrade* trade;

    if ((u16)pl_carry_item_get(me) == 0xFFFF) {
        return 0;
    }
    for (trade = npc_trade_special_tbl; trade->want != 0xFFFF; trade++) {
        if (Pl_item_timer_get(me, trade->want) >= 1) {
            me->trade_want_0x66C = trade->want;
            me->trade_count_0x66E = -1;
            me->trade_give_0x670 = trade->give;
            return 1;
        }
    }
    return 0;
}

/* 0x803A3904 (0x14C): rolls the NPC's gift for the current map kind and reports whether the player has room for
 * it. */
extern "C" u32 npc_gift_roll(_PLW* me) {
    NpcGift* gift;
    u16 roll;
    u16 sum;

    switch (stage_map_kind_get(get_now_mapno())) {
    case 1:
        gift = npc_gift_tbl_1;
        break;
    case 2:
        gift = npc_gift_tbl_2;
        break;
    case 3:
        gift = npc_gift_tbl_3;
        break;
    case 4:
        gift = npc_gift_tbl_4;
        break;
    case 5:
        gift = npc_gift_tbl_5;
        break;
    default:
        return 0;
    }
    if (gift == NULL) {
        return 0;
    }
    roll = ran_suu(0) % 100;
    for (sum = 0; gift->weight != 0xFF; gift++) {
        sum += gift->weight;
        if (roll < sum) {
            me->trade_give_0x670 = gift->item;
            return pl_item_room_get(me, gift->item, 1) >= 1;
        }
    }
    return 0;
}

/* 0x803A36B4 (0x250): picks the swap the NPC offers on this map: from a random start, the first swap whose wanted
 * item the player has and whose reward fits (0), else remembers the first one the player could do but has no room
 * for (1), 2 when the player has none of the wanted items, 3/4 for a special swap (whether the reward fits), 0xFF on a
 * map without swaps. */
extern "C" s32 npc_trade_pick(_PLW* me) {
    s32 result = 2;
    u16 give = 0;
    u16 want = 0;
    s16 pending = 0;
    NpcTrade* start;
    NpcTrade* trade;
    s32 count;
    s32 i;
    s32 held;

    if (npc_trade_special_ck(me) == 1) {
        if (pl_item_room_get(me, me->trade_give_0x670, 1) >= 1) {
            return 3;
        }
        return (Pl_item_timer_get(me, me->trade_give_0x670) == 0) ? 3 : 4;
    }
    switch (stage_map_kind_get(get_now_mapno())) {
    case 1:
        start = npc_trade_tbl_1;
        break;
    case 2:
        start = npc_trade_tbl_2;
        break;
    case 3:
        start = npc_trade_tbl_3;
        break;
    case 4:
        start = npc_trade_tbl_4;
        break;
    case 5:
        start = npc_trade_tbl_5;
        break;
    default:
        return 0xFF;
    }
    if (start == NULL) {
        return 0xFF;
    }
    for (count = 0, trade = start; trade->want != 0xFFFF; trade++) {
        count++;
    }
    trade = start + ran_suu(1) % count;
    for (i = 0; i < count; i++) {
        held = Pl_item_timer_get(me, trade->want);
        if (held >= 1) {
            result = 1;
            if (pl_item_room_get(me, trade->give, 1) >= 1) {
                me->trade_want_0x66C = trade->want;
                me->trade_count_0x66E = -1;
                me->trade_give_0x670 = trade->give;
                return 0;
            }
            if (held == 1 && Pl_item_timer_get(me, trade->give) == 0) {
                me->trade_want_0x66C = trade->want;
                me->trade_count_0x66E = -1;
                me->trade_give_0x670 = trade->give;
                return 0;
            }
            if (pending == 0) {
                want = trade->want;
                give = trade->give;
                pending = -1;
            }
        }
        trade++;
        if (trade->want == 0xFFFF) {
            trade = start;
        }
    }
    if (pending != 0) {
        me->trade_want_0x66C = want;
        me->trade_give_0x670 = give;
        me->trade_count_0x66E = pending;
    }
    return result;
}

/* The tables above are defined after the bodies: the target relocates each by name, which MWCC only does for an
 * object it has not seen defined yet. */
u16 pig_dress_flag_tbl[8] = {0xFFFF, 0xFFFF, 0xFFFF, 0x46, 0x47, 0x48, 0x49, 0x4A};
u16 pig_dress_sprite_tbl[10] = {0x24B3, 0x24B4, 0x24B5, 0x24AD, 0x24AE, 0x24B1, 0x24B2, 0x24AF, 0x24B0, 0xFFFF};
u16 pig_dress_row_tbl[8] = {0x24B6, 0x24B7, 0x24B8, 0x24B9, 0x24BA, 0x24BB, 0x24BC, 0x24BD};
u16 name_menu_sprite_tbl[11] = {0x24A1, 0x24A2, 0x24A3, 0x249B, 0x249C, 0x249D, 0x249E, 0x249F, 0x24A0, 0x249A, 0xFFFF};
u16 name_menu_row_tbl[5] = {0x24A4, 0x24A5, 0x24A6, 0x24A7, 0x24A8};
Vec note_pane_home_table[6] = {
    {0.0f, 0.0f, 0.0f},         {4840.0f, 420.0f, 6350.0f},   {-200.0f, 330.0f, -750.0f},
    {-1250.0f, 1500.0f, -200.0f}, {2753.0f, -1085.0f, 272.0f}, {358.0f, -557.0f, 2274.0f},
};
NpcTrade npc_trade_tbl_1[9] = {
    {0x90, 0xC2}, {0x8F, 0x23}, {0x192, 0x184}, {0xD8, 0x16E}, {0xD9, 0x31}, {0x1AE, 0x26}, {0x1AD, 0x2C},
    {0x118, 0xE}, {0xFFFF, 0},
};
NpcTrade npc_trade_tbl_2[5] = {{0x18C, 0x19}, {0x18D, 0x7}, {0x1AD, 0x18}, {0x118, 0xE}, {0xFFFF, 0}};
NpcTrade npc_trade_tbl_3[6] = {{0x7A, 0x17A}, {0xD8, 0x16E}, {0xD9, 0x31}, {0x1AD, 0x2D}, {0x118, 0xE}, {0xFFFF, 0}};
NpcTrade npc_trade_tbl_4[4] = {{0x1AB, 0x2D}, {0x21F, 0x19}, {0x118, 0xE}, {0xFFFF, 0}};
NpcTrade npc_trade_special_tbl[5] = {{0x8B, 0xF}, {0x17D, 0xF}, {0x17E, 0x18}, {0x18B, 0xF}, {0xFFFF, 0}};
NpcGift npc_gift_tbl_1[13] = {
    {15, 0x17A}, {15, 0xC2}, {15, 0x15F}, {15, 0x108}, {15, 0x2C}, {5, 0x185}, {5, 0x26},
    {5, 0x15A}, {3, 0x14}, {3, 0x17}, {3, 0x163}, {1, 0x31}, {0xFFFF, 0},
};
NpcGift npc_gift_tbl_2[13] = {
    {15, 0x17A}, {15, 0x15F}, {15, 0x108}, {15, 0x2D}, {15, 0x2C}, {5, 0x26}, {5, 0x23},
    {5, 0x177}, {3, 0x14}, {3, 0x17}, {3, 0x163}, {1, 0x31}, {0xFFFF, 0},
};
NpcGift npc_gift_tbl_3[13] = {
    {15, 0x17A}, {15, 0x2C}, {15, 0xC2}, {15, 0x161}, {15, 0x108}, {5, 0x26}, {5, 0x15A},
    {5, 0xCE}, {3, 0x14}, {3, 0x17}, {3, 0x163}, {1, 0x31}, {0xFFFF, 0},
};
NpcGift npc_gift_tbl_4[13] = {
    {15, 0x17A}, {15, 0x11}, {15, 0x2D}, {15, 0x161}, {15, 0x108}, {5, 0x19}, {5, 0x177},
    {5, 0x1A}, {3, 0x14}, {3, 0x17}, {3, 0x24}, {1, 0x31}, {0xFFFF, 0},
};
NpcGift npc_gift_tbl_5[13] = {
    {15, 0x17A}, {15, 0x15F}, {15, 0x62}, {15, 0x108}, {15, 0x2C}, {5, 0x18}, {5, 0x14E},
    {5, 0x23}, {3, 0x14}, {3, 0x17}, {3, 0x24}, {1, 0x31}, {0xFFFF, 0},
};
NpcTrade npc_trade_tbl_5[2] = {{0x118, 0x24}, {0xFFFF, 0}};
