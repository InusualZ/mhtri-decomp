/* enemy/em_prog_support.cpp - the lobby support block before the note-pane program tail: the spot departure screen
 *   (lobby screen 0x10), the lobby chat log with its HUD box and its menu page (25), and the quest NPC records
 *   (allocation, resource slots, per-frame step) the program tail drives.
 * RANGE. .text 0x80383148-0x80385EE0 (75 functions); .ctors 0x8056F3AC-0x8056F3B0 (`__sinit_\em_prog_support_cpp`),
 *   .data 0x805EF8C0-0x805EF990, .bss 0x806C2418-0x806C5528, .sdata 0x807933C0-0x807933D8, .sbss 0x80794C00-0x80794C08,
 *   .sdata2 0x8079BF60-0x8079BF90, extab, extabindex.
 * SEAM. The left edge closes `enemy/em019_ai.cpp`, whose static initializer `em019_se_shift_init` ends there.  The
 *   right edge is the static initializer and the record constructor emitted after it; `enemy/em_prog_tail.cpp`
 *   follows (the pool repeats 41c00000 at 0x8079BFAC there).
 * NAMES. `em_prog_support` and the function names below are GUESSES from the bodies and their callers:
 *   GUESS: lb_spot_screen_close, lb_screen16_step, lb_spot_save_step, lb_spot_depart_step, lb_spot_area_refresh,
 *   lb_spot_save_window_draw, lb_spot_screen_draw, chat_log_ring_reset, chat_log_show_mode, chat_log_line_add,
 *   chat_log_scroll_get, chat_log_scroll, chat_log_lines_draw, chat_log_sender_print, chat_log_page_open,
 *   chat_log_page_open_ck, chat_log_line_get, chat_log_page_draw, note_pane_spot_ck, note_npc_spot_ck,
 *   note_slot_init, note_slot_find, note_work_alloc, note_work_clear, note_work_model_load, note_work_create,
 *   note_npc_create, note_work_release, note_slot_release, note_slots_release, note_work_find, note_work_step,
 *   note_work_init, note_work_frame, note_work_motion_post, note_work_pose_sync, note_pane_joint_wpos_get,
 *   note_pane_motion_res_get, qn_chr_flag_clear, note_work_alpha_apply, note_work_wall_slide, qnpc_load_ck,
 *   qnpc_res_load_done, qnpc_snd_func_get, note_pane_*, qn_chr_flag_set, em_prog_slots_init, em_prog_work_init,
 *   em_prog_support_reset; data: chat_log, note_works, note_slots, note_*_motion_tbl, note_motion_tbls,
 *   lb_spot_choice_tmpl/_sprites/_rows, chat_log_notice_time.  `qn_get_motion_no` is the map's mangled name.
 *   Callees named through the registry (GUESS): lb_ship_marks_refresh (0x801FC2F0), note_pane_npc_init (0x80385EEC).
 * FLAGS. `cflags_main` (Wii/1.3) with a file-wide `#pragma peephole off` (retail keeps `clrlwi`/`extsb` before every
 *   narrow compare and call argument: 13 rows gained when it was turned off).
 * RESIDUALS.
 *  - `lb_spot_save_step`: retail shares one "finish" block (system flag 0, step 3, `lb_npc_act_set`) between the
 *    create-start failure, the system-file wait and step 4 - a jump into another case only a `goto` spells (rule 8);
 *    the duplicated block is 584 B against 488 B.
 *  - `note_page_draw`, `lb_screen16_step`: retail lowers the 0..1 / 0..2 case ranges to one `cmplwi` and the other
 *    cases to `cmpwi`; ours either keeps `cmpwi 0; blt` (int switch) or `cmplwi` throughout (the `(u32)` switch kept,
 *    95.00 against 92.78); `lb_screen16_step` keeps a branch where retail if-converts `off_mask |= 2`.
 *  - `chat_log_lines_draw`, `chat_log_scroll_get`, `note_slot_find`, `chat_log_page_draw`: register colouring only
 *    (declaration orders searched with `permdecl`/`hill`).
 *  - `note_work_frame`: retail stores the ground height to +0x1A4 and then `frsp`s it for +0x174 (an f64 view puts
 *    the `frsp` first: 97.69 against 98.79).
 *  - `note_page_step`: retail computes `lines - 1` after the cursor store.
 *  - `.sbss`/`.bss` end 4 bytes short of the claim (alignment padding).
 * SHAPES. A `case` range of a byte field is spelled `if ((u32)(x - lo) > n) switch (x) ... else ...` where retail
 *   lowers it to `addi`+`cmplwi` before the chain; `++`/`--` on the page cursor read through `(s8)(u8)`.
 */

#include "types.h"
#include "hud/cockpit.h"
#include "enemy/em_prog_support.h"
#include "enemy/em_prog_tail.h"
#include "stage/stg_w.h"
#include "stage/get_now_mapno.h"
#include "nw4r/math.h"
#include "sound/mhchar.h"
#include "enemy/ENEMY_WORK.h"
#include "sound/fn_800DD1F0.h"
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "enemy/note_work.h" /* NoteWork and the pane/slot types (rule 1) */
#include "lobby/lb_quest_screen.h" /* note_pane_get_motion, note_pane_init, note_pane_step (the owner's header) */
#include "lobby/note_talk_init.h"
#include "enemy/note_pane_npc_init.h"
#include "ef/pRoot.h"
#include "g3d/g3d_root_model_bind.h"
#include "lobby/lb_server_sel_trans.h" /* npc_talk_init / npc_talk_update / npc_talk_active_ck / event_demo_ck */
#include "Runtime.PPCEABI.H/memset.h"
#include "MSL_C/string.h"
#include "MSL_C/printf.h"
#include "g3d/g3d_resmat.h"
#include "sound/mhchar_mat_num_get.h"
#include "sound/mhchar_mat_alpha_set.h"
#include "sound/mhchar_model_mtx_update.h"
#include "sound/mhchar_model_create.h"
#include "sound/mhchar_draw_mode_set.h"
#include "Pl/coll_wall_slide.h"
#include "Pl/pl_coll.h"
#include "draw_shape.h"
#include "nw_resource.h"
#include "nw_res_entry_get.h"
#include "sound/fn_800D7F54.h"
#include "sound/fn_800E3CBC.h"
#include "sound/set_zmode.h"
#include "quest/quest_item_slot.h"
#include "lobby/lobby_w.h"
#include "lobby/lb_list_init.h"
#include "lobby/lb_cmd_pressed_ck.h"
#include "lobby/lb_menu_pos_tbl.h"
#include "lobby/lb_npc.h"
#include "lobby/LbStr.h"
#include "lobby/lb_ship_marks_refresh.h"
#include "lobby/lb_quest_ui.h"
#include "fn_80047398/lobby_world_block.h"
#include "mh3_pad/system_w.h"
#include "mh3_pad/Psw.h"
#include "mh3_pad/Screen_w.h"
#include "menu/menu_item.h"
#include "menu/menu_message.h"
#include "menu/put_frame_dialog_title.h"
#include "menu/put_message_sys.h"
#include "menu/menu_plsearch.h"
#include "menu/plsearch_work_init.h"
#include "menu/get_pop_dat_ptr.h"
#include "sound/fn_800F2A94.h"
#include "fn_80056F24.h"
#include "fn_8004CAD8.h"
#include "userdata_item.h"
#include "Pl/fn_802693C4.h"
#include "Network/network_pat_control.h"
#include "hud/layout.h"
#include "hud/put_lsp_anchor_offset.h"
#include "hud/put_button_guide_c.h"
#include "font/flfnt.h"
#include "ef/system_core.h"
#include "ef/get_move_work_adrs.h"
#include "ef/fn_800CDB2C.h"
#include "enemy/em020_prog.h"
#include "enemy/lobby_state_block.h"
#include "enemy/postChatLogLine.h"
#include "enemy/net_entry_post.h"
#include "enemy/note_page_open.h"
#include "enemy/note_page_step.h"
#include "enemy/note_page_draw.h"

/* The entry post with the sender byte read signed, as retail passes it (the owner declares a `u8`). */
#define net_post_entry_c1 ((BOOL (*)(u8, char*, u32, s8))NetCtrlWk::postEntry)
/* The wall slide with the probe height retail passes in f1, loaded right after the second pointer argument (the
 * owner's declaration stops at the area byte). */
#define coll_wall_slide_c1 ((s32 (*)(VEC3*, VEC3*, f32, VEC3*, s32, s32, s32, u8))coll_wall_slide)

struct LbSpotWork;
struct ChatLog;
struct ChatLogLine;

/* This unit's own functions, declared ahead of their first use. */
extern "C" {
void lb_spot_screen_close(void);
s32 lb_spot_save_step(LbSpotWork* work);
s32 lb_spot_depart_step(LbSpotWork* work);
void lb_spot_area_refresh(void);
void lb_spot_screen_draw(void);
void chat_log_ring_reset(void);
u8 chat_log_show_mode(void);
void chat_log_line_add(u8 kind, s8 member, s32 color, const char* name, const char* id_text, const char* text);
s16 chat_log_scroll_get(ChatLog* log, s16 delta);
void chat_log_scroll(ChatLog* log, s16 delta);
void chat_log_lines_draw(ChatLog* log);
void chat_log_sender_print(ChatLogLine* line);
void chat_log_page_open(void);
u32 chat_log_page_open_ck(void);
ChatLogLine* chat_log_line_get(ChatLog* log, s8 row);
void chat_log_page_draw(MenuSlot* slot, ChatLog* log);
u32 note_pane_spot_ck(u8 map, u8 area);
u32 note_npc_spot_ck(u8 map, u8 area);
void note_slot_init(u8 idx);
u8 note_slot_find(u8 kind);
NoteWork* note_work_alloc(void);
void note_work_clear(NoteWork* self);
void note_work_model_load(NoteWork* self, u8 kind);
NoteWork* note_work_create(u8 kind);
void note_npc_create(u8 id);
void note_work_release(NoteWork* self);
void note_slot_release(u8 idx);
void note_slots_release(void);
void note_work_step(NoteWork* self);
void note_work_init(NoteWork* self);
void note_work_frame(NoteWork* self);
void note_work_motion_post(NoteWork* self);
void note_work_pose_sync(NoteWork* self);
u32 note_pane_motion_res_get(NoteWork* self, u16 index);
void qn_chr_flag_clear(_QNPC_W* self);
void note_work_alpha_apply(NoteWork* self);
void note_work_wall_slide(NoteWork* self);
}

#pragma peephole off

/* ---------- the spot departure screen (lobby screen 0x10) ---------- */

/* The spot departure screen's work, `lobby_w.spot_0x0AC` (0x2000 bytes, cleared by `lb_spot_screen_open`).
 * size: 0x34 (approximate: the bytes this screen touches) */
struct LbSpotWork {
    /* +0x00 */ u8 state_0x00;       /* 0 opens the choice, 1 the choice, 2 the save, 3 the departure */
    /* +0x01 */ u8 step_0x01;        /* the save/departure sub-step */
    /* +0x02 */ s16 timer_0x02;      /* the departure fade countdown */
    /* +0x04 */ s32 yes_no_0x04;     /* the yes/no cursor `lb_yes_no_step` and the save toggle move */
    /* +0x08 */ _PLW* player_0x08;   /* the player who walks to the spot */
    /* +0x0C */ LbSpot* spot_0x0C;   /* the spot the player walks to */
    /* +0x10 */ LbList choice_0x10;  /* the two-row choice (save first / depart) */
    /* +0x30 */ s32 nand_msg_0x30;   /* the system message of a failed save (`chg_nand_err2msgcode`) */
};

/* The spot screen's choice: two rows of string group 0 from 0x1AC, no help line, rows greyed by the caller. */
LbListTmpl lb_spot_choice_tmpl = {2, 0, 0, 0x1AC, -1, NULL, 0, 3, {0, 0, 0, 0}};
/* The choice's frame sprites and its row sprites. */
u16 lb_spot_choice_sprites[12] = {0x16A2, 0x16A3, 0x16A4, 0x16A0, 0x16A1, 0x169A, 0x169B, 0x169C, 0x169D, 0x169E,
                                  0x169F, 0xFFFF};
u16 lb_spot_choice_rows[2] = {0x16A5, 0x16A6};

/* 0x80383148 (0x68): opens the spot departure screen for `player` walking to `spot`. */
extern "C" void lb_spot_screen_open(_PLW* player, LbSpot* spot) {
    LbSpotWork* work = lobby_w.spot_0x0AC;
    memset(work, 0, 0x2000);
    work->player_0x08 = player;
    work->spot_0x0C = spot;
    lobby_w.state_0x000 = 16;
    lobby_w.active_0x008 = 1;
}

/* 0x803831B0 (0x4): closes the screen. */
extern "C" void lb_spot_screen_close(void) {
    lb_panel_close();
}

/* 0x803831B4 (0x1BC): the spot screen's frame: the choice, the save and the departure, then queues the draw. */
extern "C" void lb_screen16_step(void) {
    LbSpotWork* work = lobby_w.spot_0x0AC;
    u16 off_mask = 0;

    switch (work->state_0x00) {
    case 0:
        work->state_0x00 = 1;
        if (getItemListSelection() == 0) {
            off_mask |= 2;
        }
        lb_choice_init(&work->choice_0x10, &lb_spot_choice_tmpl, off_mask, 0);
        if (game_ready_ck() == 1) {
            work->yes_no_0x04 = 0;
            work->step_0x01 = 0;
            work->state_0x00 = 2;
        }
        sysSE_req(5);
        break;
    case 1:
        ainpc_page_hold_set();
        switch (lb_choice_step(&work->choice_0x10)) {
        case 1:
            work->yes_no_0x04 = 0;
            work->step_0x01 = 0;
            switch (work->choice_0x10.cursor_0x00) {
            case 0:
                work->state_0x00 = 2;
                work->nand_msg_0x30 = 0;
                break;
            case 1:
                work->state_0x00 = 3;
                break;
            }
            break;
        case 2:
            lb_spot_screen_close();
            break;
        }
        break;
    case 2:
        ainpc_page_hold_set();
        switch (lb_spot_save_step(work)) {
        case 1:
            lb_spot_screen_close();
            break;
        case 2:
            if (game_ready_ck() == 1) {
                lb_spot_screen_close();
            } else {
                work->state_0x00 = 1;
            }
            break;
        }
        break;
    case 3:
        ainpc_page_hold_set();
        switch (lb_spot_depart_step(work)) {
        case 1:
            lb_spot_screen_close();
            break;
        case 2:
            work->state_0x00 = 1;
            break;
        }
        break;
    }
    subTransSet((u32)lb_spot_screen_draw, 0, NULL);
}

/* 0x80383370 (0x1E8): the save branch: asks, saves the data and system files, then walks the player to the spot;
 * returns 1 once the player arrives, 2 when the save is declined. */
extern "C" s32 lb_spot_save_step(LbSpotWork* work) {
    s32 result = 0;

    switch (work->step_0x01) {
    case 0:
        switch (lobby_cmd_toggle_step_raw(&work->yes_no_0x04)) {
        case 1:
            if (game_data_file_create_start() != 0) {
                system_w.field_0x865 = 1;
                work->step_0x01++;
                lb_player_spot_walk_act6(work->player_0x08, work->spot_0x0C);
                bgm_lobby_depart_set();
                break;
            }
        case 2:
            result = 2;
            break;
        }
        break;
    case 1:
        ainpc_page_hold_clear();
        if (Get_motion_no(work->player_0x08) != 326) {
            s32 saved = game_save_wait();
            switch (saved) {
            case 1:
                if (game_system_file_create_start() != 0) {
                    system_w.field_0x865 = 1;
                    work->step_0x01++;
                } else {
                    system_w.field_0x865 = 0;
                    work->step_0x01 = 3;
                    lb_npc_act_set(work->player_0x08, 0, 7, 0);
                }
                break;
            case -2:
            case -1:
                system_w.field_0x865 = 0;
                work->nand_msg_0x30 = chg_nand_err2msgcode();
                work->step_0x01 = 4;
                break;
            }
        }
        break;
    case 2:
        ainpc_page_hold_clear();
        switch (game_system_file_create_wait()) {
        case -2:
        case -1:
            system_w.field_0x865 = 0;
            work->nand_msg_0x30 = chg_nand_err2msgcode();
            work->step_0x01 = 4;
            break;
        case 1:
            system_w.field_0x865 = 0;
            work->step_0x01 = 3;
            lb_npc_act_set(work->player_0x08, 0, 7, 0);
            break;
        }
        break;
    case 3:
        ainpc_page_hold_clear();
        if (work->player_0x08->act_no == 0) {
            result = 1;
        }
        break;
    case 4:
        ainpc_page_hold_clear();
        if (lb_cmd_pressed_ck(0x10) == 1) {
            system_w.field_0x865 = 0;
            work->step_0x01 = 3;
            lb_npc_act_set(work->player_0x08, 0, 7, 0);
        }
        break;
    }
    return result;
}

/* 0x80383558 (0x194): the departure branch: asks, walks the player to the spot under two fades and refreshes the
 * area; returns 1 once the player arrives, 2 when the departure is declined. */
extern "C" s32 lb_spot_depart_step(LbSpotWork* work) {
    s32 result = 0;

    switch (work->step_0x01) {
    case 0:
        switch (lb_yes_no_step(&work->yes_no_0x04)) {
        case 1:
            work->step_0x01++;
            work->timer_0x02 = 240;
            lb_player_spot_walk_act6(work->player_0x08, work->spot_0x0C);
            bgm_lobby_depart_set();
            break;
        case 2:
            result = 2;
            break;
        }
        break;
    case 1:
        ainpc_page_hold_clear();
        if (--work->timer_0x02 <= 0) {
            work->step_0x01++;
            fade_set(0, 1);
            work->timer_0x02 = 20;
        }
        break;
    case 2:
        ainpc_page_hold_clear();
        if (--work->timer_0x02 <= 0) {
            work->step_0x01++;
            lb_spot_area_refresh();
            fade_set(0, 2);
            bgm_lobby_ch1_stop();
            work->timer_0x02 = 20;
        }
        break;
    case 3:
        ainpc_page_hold_clear();
        if (--work->timer_0x02 <= 0) {
            work->step_0x01++;
            lb_npc_act_set(work->player_0x08, 0, 7, 0);
        }
        break;
    case 4:
        ainpc_page_hold_clear();
        if (work->player_0x08->act_no == 0) {
            result = 1;
        }
        break;
    }
    return result;
}

/* 0x803836EC (0x34): rebuilds the lobby's item lists and refreshes the area objects after the departure. */
extern "C" void lb_spot_area_refresh(void) {
    lb_item_db_toggle();
    lb_item_list_select(lobby_world_block[0x3E00]);
    stage_area_objects_refresh();
    lb_ship_marks_refresh();
}

/* 0x80383720 (0x7C): puts the save question window. */
extern "C" void lb_spot_save_window_draw(LbSpotWork* work) {
    s8* yes;
    s8* no = (s8*)LbStr(1, 46);
    yes = (s8*)LbStr(1, 45);
    put_message_window((s8*)LbStr(3, 355), yes, no, work->yes_no_0x04, 0, 0);
}

/* 0x8038379C (0x190): the spot screen's draw. */
extern "C" void lb_spot_screen_draw(void) {
    s8 text[0x100];
    LbSpotWork* work = lobby_w.spot_0x0AC;

    set_zmode(false, 0, false);
    set_blendmode(4, 5, 1);
    switch (work->state_0x00) {
    case 1:
        lb_choice_draw(&work->choice_0x10, 0x1699, lb_spot_choice_sprites, lb_spot_choice_rows, 0x1877, 0);
        lb_panel_msg_draw(0x1877, work->choice_0x10.cursor_0x00 + 352);
        if (work->choice_0x10.cursor_0x00 == 1) {
            s32 line = -1;
            if (getItemListSelection() == 0) {
                line = 356;
            }
            if (line != -1) {
                lb_panel_line_draw(0x1877, line, 2, 2);
            }
        } else {
            lb_panel_line_draw(0x1877, 357, 2, 2);
        }
        break;
    case 3:
        if (work->step_0x01 == 0) {
            lb_choice_draw(&work->choice_0x10, 0x1699, lb_spot_choice_sprites, lb_spot_choice_rows, 0x1877, 1);
            lb_panel_msg_draw(0x1877, 354);
            lb_panel_yes_no_draw(0x1879, work->yes_no_0x04);
        }
        break;
    case 2:
        switch (work->step_0x01) {
        case 0:
            lb_spot_save_window_draw(work);
            break;
        case 4:
            wii_sysmsg_gen(work->nand_msg_0x30, (char*)text, 0);
            put_message_sys(text, 0, 0, 0);
            put_message_sys_ok_button();
            break;
        }
        break;
    }
}

/* ---------- the lobby chat log ---------- */

/* One line of the chat log: the message (cut to 0x3D characters), the sender and its colour.
 * size: 0x64 */
struct ChatLogLine {
    /* +0x00 */ char text_0x00[0x3D];
    /* +0x3D */ s8 member_0x3D;      /* the sender's player number (`player_color_get`) */
    /* +0x3E */ u8 kind_0x3E;        /* 1: a system line in its own colour */
    /* +0x3F */ char id_0x3F[0xA];   /* the sender's id text */
    /* +0x49 */ char name_0x49[0x17]; /* the sender's name */
    /* +0x60 */ s32 color_0x60;      /* the line's colour (0: white) */
};

/* The lobby chat log at `.bss` 0x806C2418: the notice timer, the 96-line ring and the friend-request row the log
 * page fills from a picked line.
 * size: 0x266C */
struct ChatLog {
    /* +0x000 */ u16 notice_timer_0x00;  /* frames the newest line stays up in the HUD box */
    /* +0x002 */ u8 notice_on_0x02;
    /* +0x003 */ u8 notice_se_0x03;     /* 1: the box sounds its chime on the next frame */
    /* +0x004 */ u8 write_0x04;         /* the ring's next slot */
    /* +0x005 */ u8 count_0x05;         /* lines in the ring (up to 96) */
    /* +0x006 */ u8 scroll_0x06;        /* the newest line shown */
    /* +0x007 */ u8 page_on_0x07;       /* 1 while the log page is open */
    /* +0x008 */ u8 lines_0x08;         /* lines the box shows (4, or 12 opened) */
    /* +0x009 */ u8 page_mode_0x09;
    /* +0x00A */ u8 pad_0x00A[0x2];
    /* +0x00C */ ChatLogLine lines_0x0C[96];
    /* +0x258C */ union {
        LobbyMailRow mail_0x258C;     /* the picked sender as the message page's draft */
        LobbyRecentRow recent_0x258C; /* the same name and id as a friend request */
    };
};

ChatLog chat_log;

/* 0x8038392C (0x18): whether the chat notice box is up. */
extern "C" u32 em_prog_flag_ck(void) {
    return chat_log.notice_timer_0x00 != 0;
}

/* 0x80383944 (0x7C): clears the chat log, keeping the lines while a session runs. */
extern "C" void enemy_net_work_reset(void) {
    if ((u32)NetCtrlWk::isSessionFlagSet() != 1 && system_w.field_0x8b1 == 0) {
        memset(&chat_log, 0, 0x266C);
        chat_log_ring_reset();
    } else {
        chat_log.notice_se_0x03 = 0;
        chat_log.notice_on_0x02 = 0;
        chat_log.notice_timer_0x00 = 0;
        chat_log.page_mode_0x09 = 0;
        chat_log.lines_0x08 = 4;
    }
}

/* 0x803839C0 (0x2C): empties the ring. */
extern "C" void chat_log_ring_reset(void) {
    chat_log.write_0x04 = 0;
    chat_log.count_0x05 = 0;
    chat_log.scroll_0x06 = 0;
    chat_log.page_on_0x07 = 0;
    chat_log.lines_0x08 = 4;
    chat_log.page_mode_0x09 = 0;
}

/* 0x803839EC (0xF8): counts the notice box down and toggles its size on the 0x100 button. */
extern "C" void hud_info_toggle_step(void) {
    ChatLog* log = &chat_log;
    u16 trig = Psw[0].button_0x2C0.pressed_0x04 | (Psw[0].button_0x2C0.held_0x14 & 0xF);

    if (log->notice_timer_0x00 != 0) {
        if (log->notice_se_0x03 != 0) {
            sysSE_req(24);
            log->notice_se_0x03 = 0;
        }
        log->notice_timer_0x00--;
        if (chat_log_show_mode() != 2) {
            log->notice_timer_0x00 = 0;
        }
        if ((trig & 0x100) != 0 && GameMode_ck() == 2 && menu_closing_ck() == 0 && chat_log_page_open_ck() == 0) {
            sysSE_req(5);
            if (log->lines_0x08 == 4) {
                log->lines_0x08 = 12;
            } else {
                log->lines_0x08 = 4;
            }
        }
    }
}

/* 0x80383AE4 (0xC4): draws the chat notice box: its window, its title and the lines. */
extern "C" void note_box_draw(void) {
    ChatLog* log = &chat_log;
    _mh_ivec2_ pos;
    u8** title;

    if (log->notice_timer_0x00 != 0 || log->page_on_0x07 == 1) {
        note_box_window_draw(log->page_on_0x07, log->lines_0x08);
        note_box_pos_upper(&pos, log->lines_0x08);
        draw_tex_icon_square(pos.x, pos.y, 16, 0xFFFFFFFF, 34);
        title = get_str_tbl(88);
        font_set_size(15, 15);
        font_print_ex(pos.x + 18, pos.y, 0, (s8*)*title);
        chat_log_lines_draw(log);
    }
}

/* The notice box's time in frame-scaled units (an 8-byte pool object: the word after it is padding). */
__declspec(section ".sdata2") f32 chat_log_notice_time[2] = {10.0f, 0.0f};

/* 0x80383BA8 (0xEC): posts one chat line unless the sender is muted, then sounds it or raises the box. */
extern "C" void postChatLogLine(u8 kind, s8 member, s32 color, const char* text, const char* id_text,
                                const char* name) {
    ChatLog* log = &chat_log;

    if (lobby_recent_has(id_text) != 1) {
        chat_log_line_add(kind, member, color, name, id_text, text);
        switch (chat_log_show_mode()) {
        case 0:
            sysSE_req(24);
            break;
        case 1:
            cockpit_notice_quest_show();
            sysSE_req(24);
            break;
        case 2:
            log->notice_on_0x02 = 1;
            log->notice_timer_0x00 = chat_log_notice_time[0] * Screen_w.frame_scale;
            log->notice_se_0x03 = 1;
            cockpit_notice_quest_clear();
            break;
        }
    }
}

/* 0x80383C94 (0x140): how a new line shows: 0 silent, 1 a quest notice, 2 the box (0xFF outside the game). */
extern "C" u8 chat_log_show_mode(void) {
    u8 mode = 0xFF;
    u8* players;

    switch (GameMode_ck()) {
    case 1:
        mode = 2;
        if (npc_talk_active_ck() == 1) {
            mode = 1;
        }
        players = (u8*)get_move_work_adrs(2);
        if (players == NULL) {
            return 0;
        }
        if (((_PLW*)(players + (s8)my_player_no() * 0xB20))->field_0x5BC != 0) {
            mode = 1;
        }
        if (menu_top_busy_ck() == 1) {
            if (chat_log_page_open_ck() == 1) {
                mode = 0;
            } else {
                mode = 1;
            }
        }
        break;
    case 2:
        mode = 2;
        switch (lobby_w.state_0x000) {
        case 0:
        case 36:
        case 10:
        case 25:
        case 16:
            break;
        case 17:
            if (lb_kitchen_idle_ck() == 0) {
                mode = 1;
            }
            break;
        default:
            mode = 1;
            break;
        }
        if (menu_top_busy_ck() == 1) {
            mode = chat_log_page_open_ck() != 1;
        }
        break;
    }
    return mode;
}

/* 0x80383DD4 (0x138): writes one line into the ring and follows it when the log is at its newest line. */
extern "C" void chat_log_line_add(u8 kind, s8 member, s32 color, const char* name, const char* id_text,
                                  const char* text) {
    ChatLog* log = &chat_log;
    ChatLogLine* line;
    char* cut;

    if ((s8)*text != 0) {
        line = &log->lines_0x0C[log->write_0x04];
        log->write_0x04 = (u8)(log->write_0x04 + 1) % 96;
        log->count_0x05++;
        if (log->count_0x05 > 96) {
            log->count_0x05 = 96;
        }
        chat_log_scroll(log, 1);
        line->kind_0x3E = kind;
        line->member_0x3D = member;
        line->color_0x60 = color;
        if (name != NULL) {
            strcpy(line->name_0x49, name);
        } else {
            line->name_0x49[0] = 0;
        }
        if (id_text != NULL) {
            strcpy(line->id_0x3F, id_text);
        } else {
            line->id_0x3F[0] = 0;
        }
        cut = flKnjMsgNumPtr((char*)text, 61);
        if (cut != NULL) {
            *cut = 0;
        }
        strcpy(line->text_0x00, text);
    }
}

/* 0x80383F0C (0xF8): the scroll position `delta` lines from the current one, or -1 when it cannot move. */
extern "C" s16 chat_log_scroll_get(ChatLog* log, s16 delta) {
    s16 pos = -1;
    s16 base;
    u8 count = log->count_0x05;
    u8 lines;

    if (count == 96) {
        base = (count - 1) - log->write_0x04;
        pos = log->scroll_0x06 + base;
        if (pos >= count) {
            pos -= count;
        }
        if (delta < 0) {
            pos += delta;
            if (pos < log->lines_0x08) {
                pos = log->lines_0x08 - 1;
            }
        } else {
            pos += delta;
            if (pos >= count) {
                pos = count - 1;
            }
        }
        pos -= base;
        if (pos < 0) {
            pos += count;
        }
    } else {
        lines = log->lines_0x08;
        if (lines < count) {
            pos = log->scroll_0x06 + delta;
            if (pos < lines) {
                pos = lines;
            } else if (pos >= count) {
                pos = count;
            }
        } else if (delta > 0) {
            pos = log->scroll_0x06 + delta;
            if (pos >= count) {
                pos = count;
            }
        }
    }
    return pos;
}

/* 0x80384004 (0x44): scrolls the log by `delta` lines when it can move. */
extern "C" void chat_log_scroll(ChatLog* log, s16 delta) {
    s16 pos = chat_log_scroll_get(log, delta);
    if (pos >= 0) {
        log->scroll_0x06 = pos;
    }
}

/* 0x80384048 (0xAC): posts `text` to the session's entry list; 1 when it went out. */
extern "C" u32 net_entry_post(_PLW* plw, u8* text) {
    switch (GameMode_ck()) {
    case 1:
        if (isServerSelectState() == 0) {
            return 0;
        }
        net_post_entry_c1(0, (char*)text, 0, plw->chunk_ofs);
        break;
    case 2:
        return NetCtrlWk::postEntryWithText(0, (char*)text, 0xF0F0F0FF, 0, NULL);
    default:
        return 0;
    }
    return 1;
}

/* 0x803840F4 (0x1B0): draws the log's lines, newest at the bottom, each with its sender. */
extern "C" void chat_log_lines_draw(ChatLog* log) {
    _mh_ivec2_ pos;
    s16 x;
    u8 shown;
    s32 idx;
    s16 y;
    s16 text_x;
    s32 i;
    ChatLogLine* line;
    s32 color;

    if (log->count_0x05 != 0) {
        note_box_pos_lower(&pos, log->lines_0x08);
        font_set_size(16, 16);
        x = pos.x;
        text_x = x + 76;
        shown = log->count_0x05 < log->lines_0x08 ? log->count_0x05 : log->lines_0x08;
        idx = log->scroll_0x06;
        y = pos.y + (log->lines_0x08 - 1) * 16;
        for (i = 0; i < shown; i++) {
            idx--;
            if (idx < 0) {
                line = &log->lines_0x0C[idx + 96];
            } else {
                line = &log->lines_0x0C[idx];
            }
            if (line->kind_0x3E == 1) {
                color = line->color_0x60;
                if ((color & 0xF) == 0) {
                    flfntSetColor(color | 0xFF);
                } else {
                    flfntSetColor(0xF0F0F0FF);
                }
            } else if (line->color_0x60 == 0xFFA461FF) {
                flfntSetColor(0xF0F0F0FF);
            } else {
                flfntSetColor(player_color_get(line->member_0x3D));
            }
            font_locate(x, y);
            chat_log_sender_print(line);
            color = line->color_0x60;
            if (color == 0) {
                flfntSetColor(0xF0F0F0FF);
            } else if ((color & 0xF) == 0) {
                flfntSetColor(color | 0xFF);
            } else {
                flfntSetColor(color);
            }
            font_locate(text_x, y);
            flfntSetAttr(1);
            flfntFontPuts(line->text_0x00);
            flfntSetAttr(0);
            y -= 16;
        }
        font_flush();
    }
}

/* 0x803842A4 (0x60): prints a line's sender, by name or by id as the option says. */
extern "C" void chat_log_sender_print(ChatLogLine* line) {
    if (get_option_cfg(12) == 0) {
        font_print((s8*)"%-8.8s:", line->name_0x49);
    } else {
        font_print((s8*)"%-8.8s:", line->id_0x3F);
    }
}

/* 0x80384304 (0x20): opens the log page: the twelve-line box, marked open. */
extern "C" void chat_log_page_open(void) {
    chat_log.page_mode_0x09 = 1;
    chat_log.page_on_0x07 = 1;
    chat_log.lines_0x08 = 12;
}

/* 0x80384324 (0x5C): raises the notice box for the screen's ten-frame notice. */
extern "C" void note_pen_seed(void) {
    chat_log.lines_0x08 = 4;
    chat_log.notice_timer_0x00 = chat_log_notice_time[0] * Screen_w.frame_scale;
    chat_log.notice_se_0x03 = 0;
    cockpit_notice_quest_clear();
}

/* 0x80384380 (0x28): closes the log page and snaps the log back to its newest line. */
extern "C" void note_pane_reset(void) {
    chat_log.page_mode_0x09 = 0;
    chat_log.page_on_0x07 = 0;
    chat_log.lines_0x08 = 4;
    chat_log.scroll_0x06 = chat_log.write_0x04;
}

/* 0x803843A8 (0x30): whether the log page is open. */
extern "C" u32 chat_log_page_open_ck(void) {
    if (chat_log.page_mode_0x09 != 0) {
        if (chat_log.page_on_0x07 == 1) {
            return 1;
        }
    }
    return 0;
}

/* 0x803843D8 (0x5C): menu page 25's opener: opens the log page with the cursor on its last row. */
extern "C" void note_page_open(MenuSlot* slot, s8 kind) {
    slot->field_0x014 = kind;
    slot->field_0x19E = 0;
    slot->page_index = 0;
    chat_log_page_open();
    slot->page_index = (s16)(chat_log.lines_0x08 - 1);
    cockpit_notice_quest_clear();
}

/* 0x80384434 (0x700): menu page 25's frame: scrolls the log and opens a message or a friend request to the
 * sender of the picked line. */
extern "C" s32 note_page_step(MenuSlot* slot) {
    s32 result = 0;
    u16 trig = slot->buttons_trig | (slot->field_0x008 & 0xF);
    ChatLog* log = &chat_log;
    LobbyMailRow* row = &log->mail_0x258C;
    u32 pad_ok = slot->field_0x00F == 2;
    u32 other;
    u8* players;
    s8 top;
    s8 lines;
    u8 scroll;
    s32 edge;
    ChatLogLine* line;

    slot->move_result_0x238 = 0;
    switch (slot->field_0x001) {
    case 0:
        if (log->page_mode_0x09 == 1) {
            log->page_on_0x07 = 1;
        }
        cockpit_notice_quest_clear();
        slot->field_0x001 = 1;
        slot->field_0x002 = 0;
        slot->step_0x003 = 0;
    case 1:
        if ((trig & 0x20) != 0) {
            sysSE_req(1);
            result = 2;
        } else if (log->count_0x05 != 0) {
            other = 1;
            players = (u8*)get_move_work_adrs(2);
            if (players != NULL &&
                strcmp(chat_log_line_get(log, slot->page_index)->id_0x3F, ((_PLW*)players)->hunter_0x5CA.id_0x5DB) == 0) {
                other = 0;
            }
            if ((trig & 0x4000) != 0) {
                if (pad_ok == 1) {
                    if (other == 1) {
                        sysSE_req(0);
                        slot->field_0x001 = 2;
                        slot->field_0x002 = 0;
                    } else {
                        sysSE_req(2);
                    }
                }
            } else if ((trig & 0x2000) != 0) {
                if (pad_ok == 1) {
                    if (other == 1) {
                        sysSE_req(0);
                        slot->field_0x001 = 3;
                        slot->field_0x002 = 0;
                        slot->step_0x003 = 0;
                        slot->field_0x18C = 0;
                    } else {
                        sysSE_req(2);
                    }
                }
            } else {
                top = log->count_0x05 < log->lines_0x08 ? log->lines_0x08 - log->count_0x05 : 0;
                lines = log->lines_0x08;
                scroll = log->scroll_0x06;
                if ((trig & 1) != 0) {
                    if ((s8)(u8)--slot->page_index < 1) {
                        chat_log_scroll(log, -1);
                        if (scroll != log->scroll_0x06) {
                            sysSE_req(3);
                            slot->page_index++;
                        } else if (slot->page_index < top) {
                            sysSE_req(2);
                            slot->page_index = top;
                        } else {
                            sysSE_req(3);
                        }
                    } else if (slot->page_index < top) {
                        slot->page_index = top;
                        sysSE_req(2);
                    } else {
                        sysSE_req(3);
                    }
                } else if ((trig & 2) != 0) {
                    if ((s8)(u8)++slot->page_index == lines - 1) {
                        chat_log_scroll(log, 1);
                        if (scroll != log->scroll_0x06) {
                            slot->page_index--;
                        }
                        sysSE_req(3);
                    } else if (slot->page_index >= lines) {
                        slot->page_index = lines - 1;
                        sysSE_req(2);
                    } else {
                        sysSE_req(3);
                    }
                } else if ((trig & 8) != 0) {
                    edge = top + 1;
                    if (slot->page_index <= edge) {
                        chat_log_scroll(log, -log->lines_0x08);
                        if (scroll != log->scroll_0x06) {
                            if (log->scroll_0x06 == chat_log_scroll_get(log, -1)) {
                                slot->page_index = top;
                            } else {
                                slot->page_index = edge;
                            }
                            sysSE_req(6);
                        } else {
                            sysSE_req(2);
                        }
                    } else {
                        slot->page_index = edge;
                        sysSE_req(6);
                    }
                } else if ((trig & 4) != 0) {
                    edge = lines - 2;
                    if (slot->page_index >= edge) {
                        chat_log_scroll(log, log->lines_0x08);
                        if (scroll != log->scroll_0x06) {
                            if (log->scroll_0x06 == chat_log_scroll_get(log, 1)) {
                                slot->page_index = lines - 1;
                            } else {
                                slot->page_index = edge;
                            }
                            sysSE_req(6);
                        } else {
                            sysSE_req(2);
                        }
                    } else {
                        slot->page_index = edge;
                        sysSE_req(6);
                    }
                }
            }
        }
        break;
    case 2: {
        PlMessageWork* msg = &plsearch_message_work;
        switch (slot->field_0x002) {
        case 0:
            log->page_on_0x07 = 0;
            line = chat_log_line_get(log, slot->page_index);
            sprintf(row->name_0x00.text_0x00, "%s", line->name_0x49);
            sprintf(row->id_0x14.text_0x00, "%s", line->id_0x3F);
            plsearch_work_init(msg, 1, row);
            slot->field_0x002 = 1;
            break;
        case 1:
            switch (plsearch_step(msg)) {
            case 2:
                slot->field_0x001 = 0;
                slot->field_0x002 = 0;
                break;
            case 4:
                result = 2;
                break;
            }
            break;
        }
        break;
    }
    case 3: {
        u8 sub = slot->field_0x002;
        if ((u32)(sub - 3) > 3) {
        switch (sub) {
        case 0:
            log->page_on_0x07 = 0;
            line = chat_log_line_get(log, slot->page_index);
            sprintf(row->name_0x00.text_0x00, "%s", line->name_0x49);
            sprintf(row->id_0x14.text_0x00, "%s", line->id_0x3F);
            slot->field_0x002 = 1;
        case 1:
            if ((trig & 0x10) != 0) {
                sysSE_req(0);
                if (slot->field_0x18C == 0) {
                    slot->field_0x002 = 2;
                    slot->step_0x003 = 0;
                } else {
                    slot->field_0x001 = 0;
                }
            } else if ((trig & 0x20) != 0) {
                sysSE_req(1);
                slot->field_0x001 = 0;
            } else {
                slot->field_0x18C = (s8)menu_cursor_step(slot->field_0x18C, 2, trig, 4, 8);
            }
            break;
        case 2:
            switch (slot->step_0x003) {
            case 0:
                switch ((u8)lobby_friend_request_send(&log->recent_0x258C)) {
                case 0:
                    menu_slot0_lock_set(1);
                    slot->step_0x003++;
                    break;
                case 1:
                    slot->field_0x002 = 4;
                    break;
                case 2:
                    slot->field_0x002 = 5;
                    break;
                case 3:
                    slot->field_0x002 = 6;
                    break;
                }
                break;
            case 1:
                if (lobby_state_block.result_0x1695 != 0) {
                    if (lobby_state_block.result_0x1695 == 1) {
                        lobby_recent_add(&log->recent_0x258C);
                        slot->field_0x002 = 3;
                    } else {
                        slot->field_0x002 = 6;
                    }
                    slot->step_0x003 = 0;
                    menu_slot0_lock_set(0);
                }
                break;
            }
            break;
        }
        } else if ((trig & 0x10) != 0) {
            sysSE_req(0);
            slot->field_0x001 = 0;
        } else if ((trig & 0x20) != 0) {
            sysSE_req(1);
            slot->field_0x001 = 0;
        }
        break;
    }
    default:
        result = 4;
        break;
    }
    if (result != 0) {
        note_pane_reset();
        slot->field_0x002 = 0;
    }
    return result;
}

/* 0x80384B34 (0x6C): the log line on page row `row`. */
extern "C" ChatLogLine* chat_log_line_get(ChatLog* log, s8 row) {
    s8 idx;

    if (log->lines_0x08 < log->count_0x05) {
        idx = (s8)log->lines_0x08 - row - 1;
        idx = (s8)log->scroll_0x06 - idx;
        idx--;
        if (idx < 0) {
            idx += 96;
        }
    } else {
        idx = row - (s8)(log->lines_0x08 - log->count_0x05);
    }
    return &log->lines_0x0C[idx];
}

/* 0x80384BA0 (0x120): menu page 25's draw. */
extern "C" void note_page_draw(MenuSlot* slot) {
    PlMessageWork* msg = &plsearch_message_work;
    ChatLog* log = &chat_log;
    LobbyMailRow* row = &log->mail_0x258C;

    switch ((u32)slot->field_0x001) {
    case 0:
    case 1:
        chat_log_page_draw(slot, log);
        break;
    case 2:
        if (slot->field_0x002 != 0) {
            plsearch_draw(msg);
        }
        break;
    case 3:
        switch ((u32)slot->field_0x002) {
        case 0:
        case 1:
        case 2:
            mailbox_mail_draw(row, 17, (s8)slot->field_0x18C);
            break;
        case 3:
            plsearch_window_draw((s8*)get_menu_msg(57, 13));
            break;
        case 4:
            plsearch_window_draw((s8*)get_menu_msg(57, 15));
            plsearch_choice_draw(1, 0);
            break;
        case 5:
            plsearch_window_draw((s8*)get_menu_msg(57, 14));
            plsearch_choice_draw(1, 0);
            break;
        case 6:
            plsearch_window_draw((s8*)get_menu_msg(57, 18));
            break;
        }
        break;
    }
}

/* 0x80384CC0 (0x20C): draws the log page's arrows, its button guide and the cursor bar. */
extern "C" void chat_log_page_draw(MenuSlot* slot, ChatLog* log) {
    _mh_ivec2_ pos;
    _mh_ivec2_ extent;
    u32 pad_ok = slot->field_0x00F == 2;
    u32 any = log->count_0x05 != 0;
    _SPR_DATA_ bar;
    s16 next;

    note_box_size(&extent, log->lines_0x08);
    extent.y -= 24;
    get_lsp_data(0xB8C, &pos);
    next = chat_log_scroll_get(log, 1);
    if (log->scroll_0x06 != next && next >= 0) {
        draw_sprite_idx(0xB8D, &pos);
    }
    pos.y += extent.y;
    next = chat_log_scroll_get(log, -1);
    if (log->scroll_0x06 != next && next >= 0) {
        draw_sprite_idx(0xB8E, &pos);
    }
    if (pad_ok == 1 && any == 1) {
        put_lsp_anchor_offset(0xB8A, &pos, extent);
        put_button_guide_c(16, &pos);
        if (get_option_cfg(7) == 0) {
            put_lsp_anchor_offset(0xB8B, &pos, extent);
            put_button_guide_c(17, &pos);
        } else {
            put_lsp_anchor_offset(0xB8B, &pos, extent);
            put_button_guide_c(18, &pos);
        }
    }
    if (any == 1) {
        note_box_pos_lower(&pos, log->lines_0x08);
        pos.x -= 8;
        pos.y += (s16)(slot->page_index * 16);
        spr_data_copy(&bar, get_lsp_data(0xB89, NULL));
        put_frame_cursor_bar(pos.x, pos.y, bar.width - 20, 16, 0, 0);
    }
}

/* ---------- the quest NPC records ---------- */

/* The motion tables of the three record kinds (talk NPC, quest pane, lobby NPC); the last two are filled at run time
 * and are defined after the records, where `.bss` keeps them. */
extern u32 note_pane_motion_tbl[20];
extern u32 note_npc_motion_tbl[20];
u32 note_talk_motion_tbl[20] = {0, 0, 150};
u32* note_motion_tbls[3] = {note_talk_motion_tbl, note_pane_motion_tbl, note_npc_motion_tbl};

NoteWork note_works[5];
NoteSlot note_slots[3];
u32 note_pane_motion_tbl[20];
u32 note_npc_motion_tbl[20];
QnpcSndFunc q_npc_snd_func;

/* 0x80384ECC (0x7C): whether the quest pane stands in (`map`, `area`). */
extern "C" u32 note_pane_spot_ck(u8 map, u8 area) {
    switch (map) {
    case 1:
    case 12:
        if (area == 11) {
            return 1;
        }
        break;
    case 3:
    case 14:
        if (area == 8) {
            return 1;
        }
        break;
    case 2:
    case 4:
    case 13:
    case 15:
        if (area == 0) {
            return 1;
        }
        break;
    case 5:
    case 16:
        if (area == 10) {
            return 1;
        }
        break;
    }
    return 0;
}

/* 0x80384F48 (0x38): whether the lobby NPCs stand in (`map`, `area`). */
extern "C" u32 note_npc_spot_ck(u8 map, u8 area) {
    if (area == 0) {
        return 1;
    }
    if (map == 9 && area == 1) {
        return 1;
    }
    return 0;
}

/* 0x80384F80 (0x78): whether the quest NPC of (`map`, `area`) needs its models loaded. */
extern "C" u32 qnpc_load_ck(u8 map, u8 area) {
    if (map >= 21) {
        return 0;
    }
    if (note_pane_spot_ck(map, area) == 1) {
        return 1;
    }
    return note_npc_spot_ck(map, area) == 1;
}

/* 0x80384FF8 (0x70): clears resource slot `idx`. */
extern "C" void note_slot_init(u8 idx) {
    u32 file;
    NoteSlot* slot = &note_slots[idx];
    res_file_ctor(&file, 0);
    slot->index_0x00 = idx;
    slot->state_0x01 = 0;
    slot->kind_0x02 = 0xFF;
    slot->handle_0x04 = -1;
    slot->handle_0x08 = -1;
}

/* 0x80385068 (0x3C): clears the three resource slots. */
extern "C" void em_prog_slots_init(void) {
    for (u8 i = 0; i < 3; i++) {
        note_slot_init(i);
    }
}

/* 0x803850A4 (0x74): the loaded resource slot of record kind `kind`, or 0xFF. */
extern "C" u8 note_slot_find(u8 kind) {
    NoteSlot* slots = note_slots;
    u32 want = kind;
    if (slots[0].state_0x01 != 0 && slots[0].kind_0x02 == want) {
        return 0;
    }
    if (slots[1].state_0x01 != 0 && slots[1].kind_0x02 == want) {
        return 1;
    }
    if (slots[2].state_0x01 != 0 && slots[2].kind_0x02 == want) {
        return 2;
    }
    return 0xFF;
}

/* 0x80385118 (0xBC): the `load_file_req` completion of one quest NPC model file: registers the read file in the
 * resource slot `*ctx` names. */
extern "C" void qnpc_res_load_done(u32 data, s32 size, s32 flag, u32* ctx) {
    NoteSlot* slot = &note_slots[*ctx];
    u32 file;
    res_file_ctor(&file, 0);
    nw4r::g3d::ResMdl mdl(NULL);
    slot->handle_0x04 = nw_res_entry_add((char*)data, (void*)size);
    slot->kind_0x02 = *ctx;
    if (slot->handle_0x04 != -1) {
        if (nw_res_entry_get(slot->handle_0x04) != NULL) {
            slot->state_0x01 = 2;
        } else {
            slot->state_0x01 = 1;
        }
    }
}

/* 0x803851D4 (0x98): takes a free record, or NULL. */
extern "C" NoteWork* note_work_alloc(void) {
    NoteWork* work = note_works;
    for (u8 i = 0; i < 5; i++, work++) {
        if (work->field_0x000 == 0) {
            work->field_0x002 = i;
            return work;
        }
    }
    return NULL;
}

/* 0x8038526C (0x4C): resets a record's model and state bytes. */
extern "C" void note_work_clear(NoteWork* self) {
    mhchar_reset(&self->model);
    self->field_0x000 = 0;
    self->field_0x001 = 0;
    self->field_0x168 = 0;
    self->field_0x169 = 0;
    self->field_0x16A = 0;
    self->field_0x16B = 0;
}

/* 0x803852B8 (0x54): resets every record and the NPC talk. */
extern "C" void em_prog_work_init(void) {
    u32 i;
    NoteWork* work = note_works;
    for (i = 0; i < 5; i++, work++) {
        note_work_clear(work);
    }
    npc_talk_init();
}

/* 0x8038530C (0xBC): makes `self` a record of kind `kind` and builds its model from the kind's resource slot. */
extern "C" void note_work_model_load(NoteWork* self, u8 kind) {
    u32 file;
    u8 slot;
    ResEntry* entry;

    res_file_ctor(&file, 0);
    slot = note_slot_find(kind);
    if (slot != 0xFF) {
        self->field_0x000 = 1;
        self->field_0x003 = kind;
        entry = nw_res_entry_get(note_slots[slot].handle_0x04);
        if (entry != NULL) {
            u32 data;
            res_file_assign(&file, res_file_ctor(&data, (u32)entry->data));
            mhchar_model_create(&self->model, &file, 0x4F, 0);
        }
        mhchar_draw_mode_set(&self->model, 2);
    }
}

/* 0x803853C8 (0x4C): takes a free record and makes it a record of kind `kind`. */
extern "C" NoteWork* note_work_create(u8 kind) {
    NoteWork* work = note_work_alloc();
    if (work != NULL) {
        note_work_model_load(work, kind);
    }
    return work;
}

/* 0x80385414 (0x8): the NPC sound callback the quest NPC sets up. */
extern "C" QnpcSndFunc qnpc_snd_func_get(void) {
    return q_npc_snd_func;
}

/* 0x8038541C (0x5C): creates lobby NPC `id` with its sound entry. */
extern "C" void note_npc_create(u8 id) {
    NoteWork* work = note_work_create(2);
    work->field_0x198 = id;
    work->field_0x1F4 = (s32)se_entry_request(10, (_ENEMY_WORK*)work, qnpc_snd_func_get());
}

/* 0x80385478 (0x6C): releases a record's model, scene work and sound. */
extern "C" void note_work_release(NoteWork* self) {
    if (self->field_0x000 != 0) {
        mhchar_model_release(&self->model);
        if (self->view.g3d_0x110 != 0) {
            push_g3d_wk(self->view.g3d_0x110);
            self->view.g3d_0x110 = 0;
        }
        if (self->field_0x1F4 != 0) {
            se_handle_clear((u8*)self->field_0x1F4);
        }
        note_work_clear(self);
    }
}

/* 0x803854E4 (0x54): releases every record and resource slot. */
extern "C" void em_prog_support_reset(void) {
    NoteWork* work = note_works;
    for (u32 i = 0; i < 5; i++, work++) {
        note_work_release(work);
    }
    note_slots_release();
}

/* 0x80385538 (0x60): releases resource slot `idx`. */
extern "C" void note_slot_release(u8 idx) {
    NoteSlot* slot = &note_slots[idx];
    if (slot->state_0x01 != 0) {
        slot->index_0x00 = idx;
        slot->state_0x01 = 0;
        nw_res_entry_clear(slot->handle_0x04);
        slot->handle_0x04 = -1;
    }
}

/* 0x80385598 (0x3C): releases the three resource slots. */
extern "C" void note_slots_release(void) {
    for (u8 i = 0; i < 3; i++) {
        note_slot_release(i);
    }
}

/* 0x803855D4 (0xC4): creates the records of the current area: the lobby NPCs, the quest pane and the talk NPC. */
extern "C" void em_prog_area_start(void) {
    u8 map = get_now_mapno();
    u8 area = get_now_areano();

    if (note_npc_spot_ck(map, area) == 1) {
        for (u8 i = 0; i < 4; i++) {
            note_npc_create(i);
        }
    }
    if (note_pane_spot_ck(map, area) == 1) {
        note_work_create(1);
    }
    if (stage_map_kind_get(map) == 1 && area == 0 && (u8)quest_id_low_get() == 1) {
        note_work_create(0);
    }
}

/* 0x80385698 (0xC4): the live record of kind `kind`, or NULL. */
extern "C" NoteWork* note_work_find(u8 kind) {
    NoteWork* work = note_works;
    for (u32 i = 0; i < 5; i++, work++) {
        if (work->field_0x000 != 0 && work->field_0x003 == kind) {
            return work;
        }
    }
    return NULL;
}

/* 0x8038575C (0x60): steps every live record, then the NPC talk. */
extern "C" void em_prog_entries_update(void) {
    NoteWork* work = note_works;
    for (u32 i = 0; i < 5; i++, work++) {
        if (work->field_0x000 != 0) {
            note_work_step(work);
        }
    }
    npc_talk_update();
}

/* 0x803857BC (0x6C): a record's step: its init on the first frame, its frame and model binding after. */
extern "C" void note_work_step(NoteWork* self) {
    u8 step = self->field_0x168;
    if ((u32)(step - 2) > 1) {
        switch (step) {
        case 0:
            self->field_0x168++;
            note_work_init(self);
            break;
        case 1:
            note_work_frame(self);
            g3d_root_model_bind(pRoot, self->model.field_0x118);
            break;
        }
    }
}

/* 0x80385828 (0xD0): sets a record up at the current area and runs its kind's init. */
extern "C" void note_work_init(NoteWork* self) {
    self->field_0x16C = 0;
    self->map_0x194 = get_now_mapno();
    self->field_0x195 = get_now_areano();
    setVector3(&self->vec_0x170, 0.0f, 0.0f, 0.0f);
    setVector3(&self->vec_0x17C, 0.0f, 0.0f, 0.0f);
    self->field_0x19E = 0;
    self->field_0x19D = 0;
    self->field_0x1A0 = 0;
    self->field_0x19F = 0;
    self->field_0x1A1 = 0;
    self->field_0x1B0 = 1.0f;
    self->flag_0x1AC = 0;
    switch (self->field_0x003) {
    case 0:
        note_talk_init(self);
        break;
    case 1:
        note_pane_init(self);
        break;
    case 2:
        note_pane_npc_init(self);
        break;
    }
}

/* 0x803858F8 (0x15C): a record's frame: its kind's program, the fade, the move, the ground and the pose. */
extern "C" void note_work_frame(NoteWork* self) {
    switch (self->field_0x003) {
    case 0:
        note_talk_frame(self);
        break;
    case 1:
        if (event_demo_ck() == 1) {
            return;
        }
        note_pane_step(self);
        break;
    case 2:
        note_pane_npc_frame(self);
        break;
    }
    self->model.field_0x34 = self->field_0x001;
    mhchar_mat_alpha_set(&self->model, 4, 255.0f * self->field_0x1B0, 0);
    note_work_alpha_apply(self);
    if (0.0f == self->field_0x1B0) {
        self->model.field_0x34 = 0;
    } else if (1.0f != self->field_0x1B0) {
        self->model.ready = 1;
    } else {
        self->model.ready = 0;
    }
    note_work_pose_sync(self);
    self->model.move(0);
    copyVec3(&self->vec_0x170, &self->model.pos_0x04);
    if (self->flag_0x1AC != 0) {
        note_work_wall_slide(self);
    }
    self->vec_0x170.y = self->effect_y_ofs_0x1A4 = GetGroundHit(&self->vec_0x170, 0xFFFF, self->field_0x195);
    copyVec3(&self->vec_0x17C, &self->vec_0x170);
    note_work_pose_sync(self);
    mhchar_model_mtx_update(&self->model);
    note_work_motion_post(self);
}

/* 0x80385A54 (0x24): the kind's motion follow-up: the quest pane's motion pick or the lobby NPC's effects. */
extern "C" void note_work_motion_post(NoteWork* self) {
    switch (self->field_0x003) {
    case 1:
        note_pane_get_motion(self);
        return;
    case 2:
        note_pane_motion_effects(self);
        return;
    }
}

/* 0x80385A78 (0x28): copies the record's rotation and position into its model. */
extern "C" void note_work_pose_sync(NoteWork* self) {
    self->model.field_0x28 = self->field_0x188;
    self->model.field_0x2C = self->field_0x18C;
    self->model.field_0x30 = self->field_0x190;
    copyVec3(&self->model.pos_0x04, &self->vec_0x170);
}

/* 0x80385AA0 (0x8): stores the pane's mode byte. */
extern "C" void note_pane_mode_set(NoteWork* self, u8 mode) {
    self->field_0x19C = mode;
}

/* 0x80385AA8 (0x2C): clears the sub-state bytes, saves the old animation pair and enters (a, b). */
extern "C" void note_pane_anim_pair_enter(NoteWork* self, u8 a, u8 b) {
    self->field_0x169 = 0;
    self->field_0x16A = 0;
    self->field_0x16B = 0;
    self->field_0x19E = self->field_0x19D;
    self->field_0x1A0 = self->field_0x19F;
    self->field_0x19D = a;
    self->field_0x19F = b;
    qn_chr_flag_clear((_QNPC_W*)self);
}

/* 0x80385AD4 (0x14): arms the pane's animation pair and runs the motion reset. */
extern "C" void note_pane_set_anim_pair(NoteWork* self, u32 a, u32 b) {
    self->field_0x1A1 = 1;
    note_pane_anim_pair_enter(self, a, b);
}

/* 0x80385AE8 (0x30): whether the pane's animation pair is (a, b). */
extern "C" u32 note_pane_anim_pair_ck(NoteWork* self, u8 a, u8 b) {
    if (self->field_0x19D == a && self->field_0x19F == b) {
        return 1;
    }
    return 0;
}

/* 0x80385B18 (0x8): the world matrix of joint `joint` of the pane's model. */
extern "C" void note_pane_joint_mtx_get(NoteWork* self, u32 joint, nw4r::math::MTX34* out) {
    mhchar_joint_mtx_get(&self->model, joint, out);
}

/* 0x80385B20 (0x8): the world position of joint `joint` of the pane's model. */
extern "C" void note_pane_joint_wpos_get(NoteWork* self, u32 joint, nw4r::math::VEC3* out) {
    self->model.get_joint_wpos(joint, out);
}

/* 0x80385B28 (0x34): the motion resource of motion `index` of the record's kind. */
extern "C" u32 note_pane_motion_res_get(NoteWork* self, u16 index) {
    if (index > 20) {
        return 0;
    }
    return note_motion_tbls[self->field_0x003][index];
}

/* 0x80385B5C (0x98): starts motion `motion` (up to 16) on the pane's model for `frames` frames. */
extern "C" void note_pane_motion_start(NoteWork* self, u16 motion, u32 blend, s32 frames) {
    if (motion <= 16) {
        mhchar_motion_start_no(&self->model, 0, motion, blend, (f32)frames, note_pane_motion_res_get(self, motion),
                               0, 1.0f);
    }
}

/* 0x80385BF4 (0x70): starts motion `motion` unless the record is already in it. */
extern "C" void note_pane_motion_set(NoteWork* self, u16 motion, u32 blend, u32 frames) {
    if ((u16)qn_get_motion_no((_QNPC_W*)self) != motion) {
        note_pane_motion_start(self, motion, blend, frames);
    }
}

/* 0x80385C64 (0xC): whether the pane model's motion has ended. */
extern "C" u32 note_pane_motion_end_ck(NoteWork* self) {
    return mhchar_motion_end_ck(&self->model, 0);
}

/* 0x80385C70 (0x10): whether the pane model's motion `a` is at the frame pair passed through in f1/f2. */
extern "C" u32 note_pane_motion_frame_ck(NoteWork* self, u32 a, f32 frame, f32 range) {
    return mhchar_frame_check(&self->model, (u16)a, 0);
}

/* 0x80385C80 (0x10): the mode-1 twin of `note_pane_motion_frame_ck`. */
extern "C" u32 note_pane_motion_frame_pass_ck(NoteWork* self, u32 a) {
    return mhchar_frame_check(&self->model, (u16)a, 1);
}

/* 0x80385C90 (0x8): the quest NPC's motion number. */
u16 qn_get_motion_no(_QNPC_W* self) {
    return (*self).motion_no_0x54;
}

/* 0x80385C98 (0x8): stores the quest NPC model's flag byte. */
extern "C" void qn_chr_flag_set(_QNPC_W* self, u8 flag) {
    (*self).field_0xF6 = flag;
}

/* 0x80385CA0 (0xC): clears the quest NPC model's flag byte. */
extern "C" void qn_chr_flag_clear(_QNPC_W* self) {
    (*self).field_0xF6 = 0;
}

/* 0x80385CAC (0x140): scales every material's alpha-compare references by the record's fade. */
extern "C" void note_work_alpha_apply(NoteWork* self) {
    nw4r::g3d::ResMatPix pix(NULL);
    MHchar* model = &self->model;
    u32 num = mhchar_mat_num_get(model);
    u32 i;
    _GXCompare comp0;
    u8 ref0;
    _GXCompare comp1;
    _GXAlphaOp op;
    u8 ref1;

    for (i = 0; i < num; i++) {
        pix = ((nw4r::g3d::ResMdl*)&model->field_0x114)->GetResMat(i).GetResMatPix();
        pix.GXGetAlphaCompare(&comp0, &ref0, &op, &comp1, &ref1);
        if (ref0 != 0 || ref1 != 0xFF) {
            mhchar_mat_tev_set(model, i, comp0, (u8)((f32)ref0 * self->field_0x1B0), op, comp1,
                               (u8)((f32)ref1 * self->field_0x1B0), self->field_0x1B0);
        }
    }
}

/* 0x80385DEC (0x90): slides the record along the walls from its last position to its new one. */
extern "C" void note_work_wall_slide(NoteWork* self) {
    nw4r::math::VEC3 from;
    nw4r::math::VEC3 to;
    nw4r::math::VEC3 unused;

    VEC3_ctor(&from);
    VEC3_ctor(&to);
    VEC3_ctor(&unused);
    copyVec3(&from, &self->vec_0x17C);
    from.y += 24.0f;
    copyVec3(&to, &self->vec_0x170);
    coll_wall_slide_c1(&from, &to, 48.0f, &self->vec_0x170, 0xFFFF, 0, 1, self->field_0x195);
}

/* 0x80385E9C (0x44): constructs a record's model and its two positions. */
inline NoteWork::NoteWork() {
    mhchar_construct(&model);
    VEC3_ctor(&vec_0x170);
    VEC3_ctor(&vec_0x17C);
}
