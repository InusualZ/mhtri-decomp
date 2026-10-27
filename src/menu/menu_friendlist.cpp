/*
 * menu/menu_friendlist.cpp - the friend picker (`friend_pick_*`, menu page 18), the hunter card view
 *   (`menu_friend_list_*`, `friend_card_*`) and the option pages 4 and 17 with the split-screen page 4.
 * RANGE. .text 0x8043F2FC-0x80443554; extab 0x8001DCC4-0x8001DE04, extabindex 0x8003E85C-0x8003EA3C, .data
 *   0x806048B0-0x806049D8, .bss 0x806E28B0-0x806E3AC8, .sdata 0x80793AF0-0x80793B08, .sdata2 0x8079C900-0x8079C930.
 *   The left edge is `menu/movie.cpp`'s end (the `.data` V->S seam at 0x806048B0 after `NetworkFriendInfo`'s table,
 *   the second "%s" in `.sdata`, 1.0f/0.0f/the int-to-float double re-pooled at 0x8079C900, the `extabindex` record
 *   of `friend_pick_init` at 0x8003E85C); the right edge is the character make (`menu/charmake.cpp`), whose
 *   `extabindex` run starts at 0x8003EA3C and whose pool repeats the same constants at 0x8079C930.
 * FLAGS. `cflags_menu` (configure.py); file-scope `#pragma pool_data off` (MWCC otherwise addresses the strings and
 *   tables through one pooled base); `#pragma optimization_level 4` over the whole file (its range compares and
 *   register colouring are -O4's: `friend_pick_draw` matches only under it); `#pragma opt_strength_reduction off`
 *   over `page17_rows_build` alone (see RESIDUALS).
 * NAMES. Module `menu` and the file from the one-copy `__FILE__` string "menu_friendlist.cpp" (DOL offset 0x600990,
 *   cited by `friend_pick_step`).  GUESS (from each body and its callers): `menu_friend_list_init`,
 *   `menu_friend_list_step`, `menu_friend_list_draw`, `page18_pick_work`, `friend_pick_*`, `friend_pick_list_draw`,
 *   `friend_card_*`, `card_equip_ids_build`, `page04_*`, `page17_*`, `option_value_step`, `option_value_sprite_draw`,
 *   `option_cursor_draw`, `arena_value_step`, `terms_progress_draw`, the types `FriendPickWork`, `FriendCardView`,
 *   and every foreign name this file spells from the name registry.
 * RESIDUALS.
 *  - `friend_pick_step`: retail's friend-request step jumps from sub-steps 0 and 2 into the tails of sub-steps 1 and 2;
 *    the source repeats the tails (rule 8), so the function is 0x80 bytes longer.
 *  - `movie_page04_draw`: retail reloads `row->page_kind` after each call in the value switch; MWCC keeps it in a
 *    saved register.
 *  - `page17_rows_build` matches only with `#pragma opt_strength_reduction off` (retail indexes the option tables with
 *    the sign-extended row index each pass; the pragma over the whole file costs three rows); the
 *    `page17_value_max` address takes its `add` operands in the other order.
 *  - `page17_transfer_get`: retail keeps the 0/1 returns as branches where MWCC folds them branch-free.
 *  - `movie_page04_draw`, `movie_page04_split_draw`, `movie_page17_draw`, `friend_card_equip_page_draw`,
 *    `friend_card_confirm_draw`: retail clears the loop counter between the table call and the copy of its result
 *    (`bl`, `li i,0`, `mr`); MWCC puts the `li` before the call or after the copy, whichever statement order is used.
 *  - `movie_page17_draw`, `friend_pick_step`: register colouring of the page count and the list locals; no declaration
 *    order among those tried reaches retail's.
 *  - `option_cursor_draw`: retail compares the value state before materialising 0xFFFF (`cmpwi` first); MWCC schedules
 *    the compare after it in every spelling tried (if/else, ternary, int locals).
 */

#include "types.h"
#include "gx.h"
#include "gx/GXRenderModeObj.h"
#include "MTX/mtx.h"
#include "MTX/mtx44.h"
#include "OS/OSError.h"
#include "OS/OSInterrupt.h"
#include "OS/OSReset.h"
#include "OS/DCInvalidateRange.h"
#include "DVD/dvd.h"
#include "DVD/dvdfs.h"
#include "VI/vi.h"
#include "AI_SDK/ai.h"
#include "MIX/mix.h"
#include "SEQ/seq.h"
#include "SYN/syn.h"
#include "WPAD/wpad.h"
#include "RVLGX/GXAttr.h"
#include "RVLGX/GXGeometry.h"
#include "RVLGX/GXLight.h"
#include "RVLGX/GXMisc.h"
#include "RVLGX/GXPixel.h"
#include "RVLGX/GXFrameBuf.h"
#include "RVLGX/GXTev.h"
#include "RVLGX/GXTexture.h"
#include "RVLGX/GXTransform.h"
#include "THP/THPDec.h"
#include "MSL_C/string.h"
#include "MSL_C/strcat.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "Runtime.PPCEABI.H/memcpy.h"
#include "main.h"
#include "mh3_pad.h"
#include "mh3_pad/system_w.h"
#include "mh3_pad/lb_param_w.h"
#include "ef/system_core.h"
#include "ef/fn_800CDB2C.h"
#include "font/flfnt.h"
#include "fn_8004CAD8.h"
#include "menu/menu_item.h"
#include "lobby/lb_quest_screen.h"
#include "Network/network_pat_control.h"
#include "sound/fn_800D7F54.h"
#include "sound/fn_800F2A94.h"
#include "menu/Movie_open.h"
#include "dvd_drive_status.h"
#include "Rmode.h"
#include "sound/snd_master_volume_set.h"
#include "sound/snd_track_pause.h"
#include "pmic_stop.h"
#include "font/flfnt_frame_reset.h"
#include "font/flfntGetSizeX.h"
#include "font/dbg_drawgraph.h"
#include "draw_rect_set.h"
#include "draw_rect_fill.h"
#include "ef/dvd_read_wait.h"
#include "ef/hbm_enable.h"
#include "kbd_close_call.h"
#include "enemy/lobby_state_block.h"
#include "enemy/em020_prog.h"
#include "Network/net_session_close.h"
#include "hud/layout.h"
#include "hud/put_lsp_anchor_offset.h"
#include "hud/cockpit.h"
#include "menu/menu_message.h"
#include "menu/get_pop_dat_ptr.h"
#include "lobby/lb_npc.h"
#include "mh3_pad/Psw.h"
#include "nw4r/db_assert.h"
#include "MSL_C/printf.h"
#include "menu/menu_friend_list_init.h"
#include "menu/friend_card_view.h"
#include "menu/movie_page18_open.h"
#include "menu/movie_page18_step.h"
#include "menu/movie_page18_draw.h"
#include "menu/plsearch_work_init.h"
#include "lobby/lb_player_menu_area_ck.h"
#include "enemy/lobby_friend_remove.h"
#include "menu/info_gun_parts_draw.h"
#include "Pl/pl_skill_tree_get.h"
#include "menu/placeinfo_server_rows_draw.h"
#include "menu/chara_icon_tex_tbl.h"
#include "menu/card_equip_ids_build.h"
#include "menu/put_menu_cursor.h"
#include "menu/Put_equip_dtl_syo_item1.h"
#include "menu/Set_equip_column_arrangement.h"
#include "Pl/fn_8027D684.h"
#include "Pl/pl_status_sheet_get.h"
#include "userdata_item.h"
#include "hud/draw_window_box.h"
#include "hud/draw_layout_window.h"
#include "ef/get_move_work_adrs.h"
#include "menu/ck_cfg.h"
#include "menu/option_cfg_max.h"
#include "hud/draw_sprite_blink_idx.h"
#include "disp_beta.h"
#include "color_black.h"
#include "mh3_pad/control.h"
#include "menu/movie_page04_open.h"
#include "menu/movie_page04_step.h"
#include "menu/movie_page04_draw.h"
#include "menu/movie_page04_split_open.h"
#include "menu/movie_page04_split_step.h"
#include "menu/movie_page04_split_draw.h"
#include "menu/movie_page17_open.h"
#include "menu/movie_page17_step.h"
#include "menu/movie_page17_draw.h"
#include "quest/quest_types.h"
#include "quest/quest_entry.h"
#include "unsplit/stage.h"
#include "stage/stg_w.h"
#include "light/light.h"
#include "g3d/g3d_scnroot.h"
#include "g3d/g3d_camera_types.h"
#include "g3d/g3d_anmscn.h"
#include "ef/pRoot.h"
#include "ef/eft_rot_vec_copy.h"
#include "ef/work_mem_free.h"
#include "mh3_pad/Screen_w.h"
#include "Pl/fn_802693C4.h"
#include "Pl/fn_80273B14.h"
#include "Pl/fn_80262940.h"
#include "Pl/fn_80288CEC.h"
#include "Pl/pl_yure.h"
#include "Pl/pl_skill.h"
#include "Pl/pl_net_slot_ready_ck.h"
#include "sound/snd_bank_loader.h"
#include "sound/fn_800EF7D8.h"
#include "sound/fn_800E3CBC.h"
#include "sound/fn_800E46E8.h"
#include "nw_resource.h"
#include "fn_80056F24.h"
#include "fn_80040598.h"
#include "fn_80047398.h"
#include "MSL/strlen.h"
#include "face_work_init.h"
#include "userdata_clear.h"
#include "g3d/camera_posture_get.h"
#include "sound/pl_voice_bank_load.h"
#include "Pl/pl_obj_ready_ck.h"
#include "Pl/pl_parts_set.h"
#include "sound/snd_stream_reloc_busy_ck.h"
#include "sound/se_bank_req.h"
#include "lobby/lb_cursor_box_step.h"
#include "lobby/get_change_hair_color.h"
#include "menu/movie.h"
#include "menu/menu_friendlist.h"

#pragma pool_data off

/* ---------------------------------------------------------------------------------------------------------------- */
/* The friend picker (`menu_friendlist.cpp`). */

#pragma optimization_level 4
/* The picker menu page 18 shows. */
FriendPickWork page18_pick_work;

/* 0x8043F2FC (0x18): opens menu page 18 on the friend picker. */
void movie_page18_open(MenuSlot* slot) {
    slot->field_0x014 = 18;
    friend_pick_init(&page18_pick_work, 0);
}

/* 0x8043F314 (0x78): clears `work`, stores `mode` and points it at the lobby's friend list. */
void friend_pick_init(FriendPickWork* work, u8 mode) {
    memset(work, 0, sizeof(FriendPickWork));
    work->mode_0x003 = mode;
    work->list_0x014 = &lobby_state_block.friends_0x16D0;
    work->step_0x000 = 0;
    work->unused_0x001 = 0;
    menu_scroll_init(&work->scroll_0x008, 0, 50, 8);
    work->moved_0x006 = 0;
}

/* 0x8043F38C (0xC): steps menu page 18's picker. */
s32 movie_page18_step(MenuSlot* slot) {
    return friend_pick_step(&page18_pick_work);
}

/* 0x8043F398 (0x6CC): fetches the page's cards one by one, lets the player pick a friend, shows its card and sends
 * a friend request; 1 picked (mode 1), 2 back, 3 nothing to fetch or the request failed, 4 closed. */
s32 friend_pick_step(FriendPickWork* work) {
    FriendCardView* view;
    s32 ret;
    MenuScroll* scroll;
    PlayerPad* pad;
    LobbyFriendList* list;
    u8 index;
    NetPeerCard* card;
    u8 col;
    s32 count;
    u8 fetched;
    s32 row;
    const char* id;
    char* args;

    ret = 0;
    scroll = &work->scroll_0x008;
    view = &friend_card_view;
    pad = Psw;
    if (chk_pointer() == 0) {
        work->input_0x004 = pad->button_0x2C0.pressed_0x04 | (pad->button_0x2C0.held_0x14 & 0xF);
    } else {
        work->input_0x004 = 0;
    }
    work->moved_0x006 = 0;
    switch (work->step_0x000) {
    case 0:
        menu_slot0_lock_set(1);
        work->page_top_0x10AC = scroll->rows_per_page * scroll->col;
        work->fetched_0x10AD = 0;
        work->step_0x000 = 1;
    case 1:
        list = work->list_0x014;
        fetched = work->fetched_0x10AD;
        index = work->page_top_0x10AC + fetched;
        if (index >= list->count_0x000) {
            menu_slot0_lock_set(0);
            work->step_0x000 = 3;
            break;
        }
        id = list->rows_0x004[index].id_0x14;
        if (*id != 0) {
            card = &work->cards_0x01C[fetched];
            memset(card, 0, sizeof(NetPeerCard));
            sprintf(card->id_0x014, "%s", id);
            requestPeerProfileById(id, &work->result_0x10AE, 1, index);
            work->step_0x000 = 2;
        } else {
            ret = 3;
        }
        break;
    case 2:
        if (work->result_0x10AE == 0) {
            break;
        }
        list = work->list_0x014;
        index = work->page_top_0x10AC + work->fetched_0x10AD;
        if (work->result_0x10AE == 1) {
            row = index;
            list->rows_0x004[row].state_0x1E = 1;
        } else {
            row = index;
            list->rows_0x004[row].state_0x1E = 0;
        }
        card = &work->cards_0x01C[work->fetched_0x10AD];
        if (strcmp(card->id_0x014, list->rows_0x004[row].id_0x14) == 0) {
            sprintf(card->name_0x000, "%s", list->rows_0x004[row].name_0x00);
            if (work->result_0x10AE == 1) {
                copyPeerProfileCard(card);
            }
            if (++work->fetched_0x10AD >= scroll->rows_per_page) {
                menu_slot0_lock_set(0);
                work->step_0x000 = 3;
            } else {
                work->step_0x000 = 1;
            }
        } else {
            work->step_0x000 = 0;
        }
        break;
    case 3:
        list = work->list_0x014;
        col = scroll->col;
        count = list->count_0x000;
        if (count < col * 8) {
            fetched = 0;
        } else if (count - (col + 1) * 8 < 0) {
            fetched = count % 8;
        } else {
            fetched = 8;
        }
        if (work->fetched_0x10AD != fetched) {
            work->step_0x000 = 0;
            break;
        }
        if (work->input_0x004 & 0x10) {
            index = scroll->row + scroll->rows_per_page * col;
            if (index < count) {
                sysSE_req(5);
                work->picked_0x018 = &list->rows_0x004[index];
                if (work->mode_0x003 == 1) {
                    ret = 1;
                } else {
                    requestPeerProfileById(list->rows_0x004[index].id_0x14, &work->result_0x10AE, 2, index);
                    work->step_0x000 = 4;
                }
            } else {
                sysSE_req(2);
            }
        } else if (work->input_0x004 & 0x20) {
            ret = 2;
            sysSE_req(1);
        } else {
            menu_scroll_step(scroll, work->input_0x004, 6);
            work->moved_0x006 = scroll->move_flags;
            if (col != scroll->col) {
                work->step_0x000 = 0;
            }
        }
        break;
    case 4:
        if (work->result_0x10AE == 0) {
            break;
        }
        card = &work->cards_0x01C[scroll->row];
        memset(card, 0, sizeof(NetPeerCard));
        sprintf(card->name_0x000, "%s", work->picked_0x018->name_0x00);
        sprintf(card->id_0x014, "%s", work->picked_0x018->id_0x14);
        if (work->result_0x10AE != 1) {
            work->picked_0x018->state_0x1E = 0;
        } else {
            work->picked_0x018->state_0x1E = 1;
            copyPeerProfileCard(card);
        }
        menu_friend_list_init(view, card, 0, work->picked_0x018->state_0x1E);
        work->step_0x000 = 5;
        break;
    case 5:
        switch (menu_friend_list_step(view)) {
        case 4:
        case 5:
            ret = 4;
            break;
        case 2:
            strcpy(work->id_0x10B2, work->cards_0x01C[scroll->row].id_0x014);
            strcpy(work->name_0x10BC, work->cards_0x01C[scroll->row].name_0x000);
            menu_slot0_lock_set(1);
            work->step_0x000 = 6;
            work->request_step_0x002 = 0;
            break;
        case 1:
            work->step_0x000 = 0;
            break;
        case 3:
            work->step_0x000 = 3;
            break;
        }
        break;
    case 6:
        lobby_hidden_flag = 1;
        switch (work->request_step_0x002) {
        case 0:
            work->request_step_0x002 = 1;
            if (menu_work.slot[0].field_0x31F != get_option_cfg(27)) {
                requestFriendSync(&work->result_0x10AE);
                break;
            }
            work->request_step_0x002 = 2;
            if (menu_work.slot[0].field_0x31E != 0) {
                resendProfileRecord(&work->result_0x10AE);
                break;
            }
            args = work->id_0x10B2;
            if (NetCtrlWk::requestCommand18((const s32*)&args, &work->result_0x10AE) == 0) {
                ret = 3;
            } else {
                work->request_step_0x002 = 3;
            }
            break;
        case 1:
            if (work->result_0x10AE == 0) {
                break;
            }
            if (work->result_0x10AE != 1) {
                set_option_cfg(27, menu_work.slot[0].field_0x31F);
            }
            work->request_step_0x002 = 2;
            if (menu_work.slot[0].field_0x31E != 0) {
                resendProfileRecord(&work->result_0x10AE);
                break;
            }
            args = work->id_0x10B2;
            if (NetCtrlWk::requestCommand18((const s32*)&args, &work->result_0x10AE) == 0) {
                ret = 3;
            } else {
                work->request_step_0x002 = 3;
            }
            break;
        case 2:
            if (work->result_0x10AE == 0) {
                break;
            }
            menu_work.slot[0].field_0x31E = 0;
            args = work->id_0x10B2;
            if (NetCtrlWk::requestCommand18((const s32*)&args, &work->result_0x10AE) == 0) {
                ret = 3;
            } else {
                work->request_step_0x002 = 3;
            }
            break;
        case 3:
            if (work->result_0x10AE == 0) {
                break;
            }
            if (lobby_net_leave_select(work->result_0x10AE) == 0 && lobby_net_jump_msg_get(work->result_0x10AE) != NULL) {
                work->request_step_0x002 = 4;
                work->timer_0x10B0 = 15;
            } else {
                menu_slot0_lock_set(0);
                menu_item_close();
                ret = 4;
            }
            break;
        case 4:
            if (work->timer_0x10B0 > 0) {
                work->timer_0x10B0--;
            } else if (work->input_0x004 & 0x10) {
                sysSE_req(0);
                menu_slot0_lock_set(0);
                menu_item_close();
                ret = 4;
            }
            break;
        }
        break;
    default:
        nw4r::db::Panic("menu_friendlist.cpp", 516, "NW4R:Failed assertion 0");
        menu_slot0_lock_set(0);
        menu_item_close();
        ret = 4;
        break;
    }
    return ret;
}

/* 0x8043FA64 (0xC): draws menu page 18's picker. */
void movie_page18_draw(MenuSlot* slot) {
    friend_pick_draw(&page18_pick_work);
}

static void friend_pick_list_draw(FriendPickWork* work, u32 filled);

/* 0x8043FA70 (0x9C): draws the picker, the friend's card or the request's message. */
void friend_pick_draw(FriendPickWork* work) {
    FriendCardView* view;

    view = &friend_card_view;
    switch (work->step_0x000) {
    case 0:
        friend_pick_list_draw(work, 0);
        break;
    case 1:
    case 2:
    case 3:
    case 4:
        friend_pick_list_draw(work, 1);
        break;
    case 5:
        menu_friend_list_draw(view);
        break;
    case 6:
        if (work->request_step_0x002 == 4) {
            put_message((s8*)lobby_net_jump_msg_get(work->result_0x10AE), 20, 0xC0C0C0FF);
        }
        break;
    }
}

/* 0x8043FB0C (0x420): draws the picker's page: one row per friend (name, id and online state) once its card is
 * fetched (`filled`), the page arrows and the title window. */
static void friend_pick_list_draw(FriendPickWork* work, u32 filled) {
    _SPR_DATA_ spr;
    _mh_ivec2_ pos;
    _mh_ivec2_ at;
    _mh_ivec2_ icon;
    LobbyFriendList* list;
    u16* rows;
    s32 ofs;
    s32 i;
    bool lit;
    bool hot;
    u8 index;
    LobbyTimerRow* row;
    u8 state;

    list = work->list_0x014;
    get_lsp_data(0x28A6, &pos);
    draw_sprite_ary((const u16*)get_menu_lsp_tbl(0x12F), &pos);
    get_lsp_data(0x28B6, &pos);
    draw_sprite_ary((const u16*)get_menu_lsp_tbl(0xE), &pos);
    get_lsp_data(0x28B8, &pos);
    draw_sprite_ary((const u16*)get_menu_lsp_tbl(0x130), &pos);
    lit = chk_pointer() != 1;
    for (i = 0, ofs = 0; i < work->scroll_0x008.row_limit; ofs += 2, i++) {
        if (i == work->scroll_0x008.row) {
            hot = lit == 1;
        } else {
            hot = 0;
        }
        get_lsp_data(0x28B6, &pos);
        rows = (u16*)get_menu_lsp_tbl(0xF);
        put_lsp_anchor_offset(rows[i], &at, pos);
        get_lsp_data(0x28B8, &pos);
        rows = (u16*)get_menu_lsp_tbl(0x131);
        put_lsp_anchor_offset(rows[i], &icon, pos);
        draw_sprite_ary((const u16*)get_menu_lsp_tbl(0x10), &at);
        index = i + work->scroll_0x008.rows_per_page * work->scroll_0x008.col;
        if (index < list->count_0x000 && filled == 1 && i < work->fetched_0x10AD) {
            row = &list->rows_0x004[index];
            spr_data_copy(&spr, get_lsp_data(0x1DC, NULL));
            spr.color = GetMenuFontColor(1, hot, lit, 0);
            draw_font(spr, (s8*)row->name_0x00, 0, &at);
            spr_data_copy(&spr, get_lsp_data(0x1DD, NULL));
            spr.color = GetMenuFontColor(1, hot, lit, 0);
            draw_font(spr, (s8*)row->id_0x14, 0, &at);
            state = row->state_0x1E;
            switch (state) {
            case 1:
                draw_sprite_idx(0x28C8, &icon);
                break;
            case 0:
                draw_sprite_idx(0x28C9, &icon);
                break;
            }
            draw_font_idx(0x28C6, (s8*)get_menu_msg(0x33, state), 5, &icon);
        } else {
            spr_data_copy(&spr, get_lsp_data(0x1DC, NULL));
            spr.color = GetMenuFontColor(0, hot, lit, 0);
            draw_font(spr, (s8*)get_menu_msg(0x32, 6), 0, &at);
            spr_data_copy(&spr, get_lsp_data(0x1DD, NULL));
            spr.color = GetMenuFontColor(0, hot, lit, 0);
            draw_font(spr, (s8*)get_menu_msg(0x32, 6), 0, &at);
            draw_sprite_idx(0x28C9, &icon);
        }
        if (hot == 1 && lit == 1 && filled == 1) {
            draw_sprite_ary((const u16*)get_menu_lsp_tbl(0x134), &at);
        }
    }
    get_lsp_data(0x28A5, &pos);
    PutPageArrow((u16*)get_menu_lsp_tbl(1), work->scroll_0x008.col, work->scroll_0x008.col_count, work->moved_0x006,
                 &pos, 0);
    get_lsp_data(0x28D0, &pos);
    draw_window_frame_preset(pos.x, pos.y, 0);
    font_set_size(16, 16);
    switch (work->mode_0x003) {
    case 0:
        font_print_ex(pos.x + 24, pos.y + 16, 0, (s8*)get_menu_msg(0x33, 2));
        break;
    case 1:
        font_print_ex(pos.x + 24, pos.y + 16, 0, (s8*)get_menu_msg(0x38, 9));
        break;
    }
}

/* ---------------------------------------------------------------------------------------------------------------- */
/* The hunter card view: a friend's card with its three pages and the command rows (mail, remove). */

/* size: 0x148 (the clear `menu_friend_list_init` runs) - the card view: its step, the command rows, the page scroll,
 * the card shown, the mail the message command opens and whether the card's owner is online. */
struct FriendCardView {
    /* +0x000 */ u8 step;
    /* +0x001 */ u8 unused_0x001;
    /* +0x002 */ u8 row_count;            /* the command rows (3 from the friend list, else 2) */
    /* +0x003 */ u8 page_count;           /* the card pages (3 while the owner is online, else 1) */
    /* +0x004 */ u8 mode;                 /* 0 the friend list, 1-3 the other callers */
    /* +0x005 */ u8 confirm;              /* the confirmation the step shows (0 mail, 6 remove) */
    /* +0x006 */ s8 confirm_cursor;
    /* +0x007 */ u8 confirm_choices;      /* 1 (OK) or 2 (yes/no) */
    /* +0x008 */ u16 input;               /* the pad's pressed word plus the held d-pad bits */
    /* +0x00A */ u16 moved;               /* the scroll's move flags of the frame */
    /* +0x00C */ MenuScroll scroll;       /* the command row (`row`) and the page (`col`) */
    /* +0x018 */ MenuEntry rows[3];
    /* +0x060 */ NetPeerCard* card;
    /* +0x064 */ LobbyMailRow mail;       /* the mail the message command writes to the card's owner */
    /* +0x144 */ u8 online;
    /* +0x145 */ u8 pad_0x145[3];
};

/* The hunter card view the friend picker, the player search and the lobby card menu share. */
FriendCardView friend_card_view;

static void friend_card_rows_build(FriendCardView* view, s8 cursor);
static s8 friend_card_row_enabled(FriendCardView* view, s8 row);
static void friend_card_rows_draw(FriendCardView* view);
static void friend_card_profile_draw(FriendCardView* view, bool lit);
static void friend_card_place_draw(FriendCardView* view);
static void friend_card_equip_draw(FriendCardView* view);
static void friend_card_equip_page_draw(FriendCardView* view, _EQUIP* equip, u8 dye);
static void friend_card_skills_draw(u16 id, u16* skills);
static void friend_card_confirm_draw(FriendCardView* view, u8 choices, u16 kind);

/* 0x8043FF2C (0x110): opens the view on `card` for caller `mode`; `online` adds the card's other pages. */
void menu_friend_list_init(FriendCardView* view, NetPeerCard* card, u8 mode, u8 online) {
    memset(view, 0, sizeof(FriendCardView));
    view->step = 0;
    view->unused_0x001 = 0;
    view->input = 0;
    view->moved = 0;
    view->mode = mode;
    switch (mode) {
    case 0:
        view->row_count = 3;
        break;
    case 1:
    case 2:
    case 3:
        view->row_count = 2;
        break;
    }
    view->online = online;
    view->card = card;
    if (online == 0) {
        view->page_count = 1;
    } else {
        view->page_count = 3;
    }
    menu_scroll_init(&view->scroll, 0, view->page_count * view->row_count, view->row_count);
    if (view->online == 0 && view->mode == 0) {
        view->scroll.row = 2;
    }
    friend_card_rows_build(view, view->scroll.row);
}

/* 0x8044003C (0x14C): fills the three command rows: their message, whether they can be picked and their colour. */
static void friend_card_rows_build(FriendCardView* view, s8 cursor) {
    MenuEntry* row;
    s32 i;
    u32 lit;
    s8 hot;
    s32 dim;

    i = 0;
    row = view->rows;
    do {
        if (i < view->row_count) {
            row->page_kind = i;
            row->field_0x02 = friend_card_row_enabled(view, i);
            row->field_0x08 = get_menu_msg(0x34, i);
            lit = 0;
            if (view->step < 10 && chk_pointer() == 0) {
                lit = 1;
            }
            if (i == cursor) {
                if (lit == 1) {
                    hot = 1;
                    dim = 0;
                } else if (chk_pointer() == 0) {
                    hot = 0;
                    dim = 1;
                } else {
                    hot = 0;
                    dim = 0;
                }
            } else {
                hot = 0;
                dim = 0;
            }
            row->selected = hot;
            row->field_0x0C = GetMenuFontColor(row->field_0x02 != 0, hot != 0, lit != 0, dim != 0);
        } else {
            row->page_kind = -1;
            row->selected = 0;
            row->field_0x02 = 0;
            row->field_0x08 = 0;
            row->field_0x0C = 0;
        }
        row++;
        i++;
    } while (i < 3);
}

/* 0x80440188 (0xE0): 1 when command `row` can be picked (not in the arena while the hunter is busy). */
static s8 friend_card_row_enabled(FriendCardView* view, s8 row) {
    u8 enabled;
    _PLW* plw;

    enabled = 1;
    switch (view->mode) {
    case 0:
        if (view->online == 0 && row != 2) {
            enabled = 0;
        }
        break;
    case 1:
        if (row != 1) {
            enabled = 0;
        }
        break;
    }
    if (row == 0 && GameMode_ck() == 2) {
        plw = (_PLW*)get_move_work_adrs(2);
        if (plw != NULL) {
            plw = &plw[(s8)my_player_no()];
            if (plw->field_0xB01 & 7) {
                enabled = 0;
            }
            if (lb_player_menu_area_ck(plw) == 1) {
                enabled = 0;
            }
        }
    }
    return enabled;
}

/* 0x80440268 (0x428): steps the view: the pages and commands, the mail page, the confirmation and the friend's
 * removal; 0 busy, 1 removed, 2 a request was picked, 3 back, 4 failed or unknown step. */
s32 menu_friend_list_step(FriendCardView* view) {
    PlMessageWork* message;
    s32 ret;
    MenuScroll* scroll;
    PlayerPad* pad;
    NetPeerCard* card;
    LobbyMailRow* mail;

    ret = 0;
    scroll = &view->scroll;
    message = &plsearch_message_work;
    pad = Psw;
    if (chk_pointer() == 0) {
        view->input = pad->button_0x2C0.pressed_0x04 | (pad->button_0x2C0.held_0x14 & 0xF);
    } else {
        view->input = 0;
    }
    view->moved = 0;
    switch (view->step) {
    case 0:
        friend_card_rows_build(view, scroll->row);
        view->step = 1;
        break;
    case 1:
        if (view->input & 0x10) {
            if (view->rows[scroll->row].field_0x02 == 0) {
                sysSE_req(2);
            } else {
                switch (scroll->row) {
                case 0:
                    sysSE_req(5);
                    view->step = 10;
                    break;
                case 1:
                    sysSE_req(27);
                    view->step = 20;
                    break;
                case 2:
                    sysSE_req(0);
                    view->step = 40;
                    break;
                }
                view->unused_0x001 = 0;
                friend_card_rows_build(view, scroll->row);
            }
        } else if (view->input & 0x20) {
            sysSE_req(1);
            ret = 3;
        } else {
            menu_scroll_step(scroll, view->input, 6);
            view->moved = scroll->move_flags;
            friend_card_rows_build(view, scroll->row);
        }
        break;
    case 10:
        view->confirm = 0;
        view->step = 50;
        break;
    case 20:
        mail = &view->mail;
        card = view->card;
        sprintf(mail->name_0x00.text_0x00, "%s", card->name_0x000);
        sprintf(mail->id_0x14.text_0x00, "%s", card->id_0x014);
        plsearch_work_init(message, 1, mail);
        view->step++;
        break;
    case 21:
        switch (plsearch_step(message)) {
        case 2:
            view->step = 0;
            break;
        case 4:
            ret = 3;
            break;
        }
        break;
    case 40:
        view->confirm = 6;
        view->step = 50;
        break;
    case 50:
        switch (view->confirm) {
        case 1:
        case 3:
        case 4:
        case 5:
            sysSE_req(2);
            view->confirm_cursor = 0;
            view->confirm_choices = 1;
            break;
        case 0:
        case 2:
        case 6:
            view->confirm_cursor = 1;
            view->confirm_choices = 2;
            break;
        }
        view->step++;
        break;
    case 51:
        if (view->input & 0x10) {
            sysSE_req(0);
            if (view->confirm_cursor == 0) {
                view->step = 52;
            } else {
                view->step = 0;
            }
        } else if (view->input & 0x20) {
            sysSE_req(1);
            view->step = 0;
        } else {
            view->confirm_cursor = menu_cursor_step(view->confirm_cursor, view->confirm_choices, view->input, 4, 8);
        }
        break;
    case 52:
        switch (view->confirm) {
        case 1:
        case 3:
        case 4:
        case 5:
            view->step = 0;
            break;
        case 0:
        case 2:
            ret = 2;
            break;
        case 6:
            switch ((u8)lobby_friend_remove((LobbyRecentRow*)view->card)) {
            case 1:
                menu_slot0_lock_set(1);
                view->step++;
                break;
            case 0:
            case 2:
                sysSE_req(2);
                ret = 3;
                break;
            }
            break;
        }
        break;
    case 53:
        if (lobby_state_block.result_0x1695 != 0) {
            if (lobby_state_block.result_0x1695 == 1) {
                lobby_friend_drop((LobbyRecentRow*)view->card);
                sysSE_req(0);
                ret = 1;
            } else {
                sysSE_req(2);
                ret = 4;
            }
            menu_slot0_lock_set(0);
        }
        break;
    default:
        ret = 4;
        break;
    }
    return ret;
}

/* 0x80440690 (0x108): draws the view: the mail page, or the card page, the command rows and the page arrows, then
 * the confirmation window. */
void menu_friend_list_draw(FriendCardView* view) {
    s32 drawn;
    u8 page;
    _mh_ivec2_ pos;

    drawn = 0;
    page = view->scroll.col;
    switch (view->step) {
    case 21:
        plsearch_draw(&plsearch_message_work);
        drawn = 1;
        break;
    }
    if (drawn == 0) {
        switch (page) {
        case 0:
            friend_card_profile_draw(view, 1);
            break;
        case 1:
            friend_card_place_draw(view);
            break;
        case 2:
            friend_card_equip_draw(view);
            break;
        }
        friend_card_rows_draw(view);
        if (view->page_count > 1) {
            get_lsp_data(0x2920, &pos);
            PutPageArrow((u16*)get_menu_lsp_tbl(1), view->scroll.col, view->page_count, view->moved, &pos, 0);
        }
        font_flush();
    }
    switch (view->step) {
    case 51:
        friend_card_confirm_draw(view, view->confirm_choices, view->confirm);
        break;
    }
}

/* 0x80440798 (0x188): draws the command rows' window, rows and cursor. */
static void friend_card_rows_draw(FriendCardView* view) {
    u16 window;
    u16* anchors;
    _SPR_DATA_ spr;
    _mh_ivec2_ pos;
    _mh_ivec2_ at;
    MenuEntry* row;
    s32 i;

    anchors = NULL;
    window = 0xFFFF;
    switch (view->mode) {
    case 0:
        window = 0x296A;
        anchors = (u16*)get_menu_lsp_tbl(0x13F);
        break;
    case 1:
    case 2:
    case 3:
        window = 0x2A68;
        anchors = (u16*)get_menu_lsp_tbl(0x136);
        break;
    }
    if (anchors == NULL || window == 0xFFFF) {
        return;
    }
    spr_data_copy(&spr, get_lsp_data(window, &pos));
    spr.color = 0x926239FF;
    draw_window_box(pos.x, pos.y, spr.width, spr.height, 0x926239FF, 0);
    for (i = 0, row = view->rows; i < view->row_count; row++, anchors++, i++) {
        put_lsp_anchor_offset(*anchors, &at, pos);
        draw_sprite_ary((const u16*)get_menu_lsp_tbl(2), &at);
        spr_data_copy(&spr, get_lsp_data(0x77, NULL));
        spr.color = row->field_0x0C;
        draw_font(spr, (s8*)row->field_0x08, 1, &at);
        if (row->selected != 0 && chk_pointer() == 0) {
            put_menu_cursor((u16*)get_menu_lsp_tbl(3), 0, &at);
        }
    }
}

/* 0x80440920 (0x45C): draws the card's first page: name and id, then for an online owner the weapon kind, the id
 * number, the rank, the hunter rank and the comment, and the active skills. */
static void friend_card_profile_draw(FriendCardView* view, bool lit) {
    _mh_ivec2_ pos;
    char number[16];
    _SPR_DATA_ spr;
    s8 comment[0xAC];
    NetPeerCard* card;
    u16* anchors;
    s32 color;
    u8 kind;
    u8** names;
    u8 rank;

    card = view->card;
    anchors = NULL;
    color = GetMenuFontColor(1, lit, lit, 0);
    switch (view->mode) {
    case 0:
        anchors = (u16*)get_menu_lsp_tbl(0x140);
        break;
    case 1:
        anchors = (u16*)get_menu_lsp_tbl(0x137);
        break;
    case 2:
    case 3:
        anchors = (u16*)get_menu_lsp_tbl(0x135);
        break;
    }
    get_lsp_data(anchors[1], &pos);
    draw_sprite_idx(0x292C, &pos);
    draw_sprite_ary((const u16*)get_menu_lsp_tbl(0x138), &pos);
    get_lsp_data(anchors[2], &pos);
    draw_sprite_ary((const u16*)get_menu_lsp_tbl(0x13B), &pos);
    draw_font_idx(0x2937, (s8*)get_menu_msg(0x35, 0), 4, &pos);
    draw_font_idx(0x2939, (s8*)get_menu_msg(0x35, 1), 4, &pos);
    spr_data_copy(&spr, get_lsp_data(0x2938, NULL));
    spr.color = color;
    draw_font(spr, (s8*)card->name_0x000, 0, &pos);
    spr_data_copy(&spr, get_lsp_data(0x293A, NULL));
    spr.color = color;
    draw_font(spr, (s8*)card->id_0x014, 0, &pos);
    if (view->online == 0) {
        get_lsp_data(anchors[2], &pos);
        draw_sprite_idx(0x2942, &pos);
        draw_font_idx(0x2941, (s8*)get_menu_msg(0x35, 7), 5, &pos);
    } else {
        get_lsp_data(anchors[3], &pos);
        draw_sprite_ary((const u16*)get_menu_lsp_tbl(0x13C), &pos);
        draw_font_idx(0x2944, (s8*)get_menu_msg(0x35, 2), 4, &pos);
        draw_font_idx(0x2946, (s8*)get_menu_msg(0x35, 3), 4, &pos);
        draw_font_idx(0x2948, (s8*)get_menu_msg(0x35, 4), 4, &pos);
        kind = 0;
        if (card->equip_0x022[6][0] != 0) {
            kind = Get_pl_type((_EQUIP*)card->equip_0x022[6], (_EQUIP*)card->equip_0x022[7]);
        }
        names = get_str_tbl(0x2C);
        spr_data_copy(&spr, get_lsp_data(0x2945, NULL));
        spr.color = color;
        draw_font(spr, (s8*)names[kind], 0, &pos);
        sprintf(number, "%d", card->id_0x01E);
        spr_data_copy(&spr, get_lsp_data(0x2947, NULL));
        spr.color = color;
        draw_font(spr, (s8*)number, 0, &pos);
        kind = card->level_0x020;
        if (kind != 0) {
            draw_sprite_uv_idx(0x294A, chara_icon_tex_tbl[kind], &pos);
            spr_data_copy(&spr, get_lsp_data(0x2949, NULL));
            spr.color = color;
            draw_font(spr, (s8*)get_menu_msg(0x30, kind), 0, &pos);
        }
        get_lsp_data(anchors[4], &pos);
        draw_sprite_ary((const u16*)get_menu_lsp_tbl(0x13D), &pos);
        draw_font_idx(0x2955, (s8*)get_menu_msg(0x35, 5), 4, &pos);
        draw_font_idx(0x2957, (s8*)get_menu_msg(0x35, 6), 4, &pos);
        rank = card->rank_0x021;
        names = get_str_tbl(0x56);
        spr_data_copy(&spr, get_lsp_data(0x2956, NULL));
        spr.color = color;
        draw_font(spr, (s8*)names[rank], 0, &pos);
        knj_msg_str_build((s8*)card->comment_0x09E, comment, 28, 3);
        spr_data_copy(&spr, get_lsp_data(0x2958, NULL));
        spr.color = color;
        draw_font(spr, comment, 0, &pos);
    }
    if (anchors[6] != 0xFFFF && view->online != 0) {
        friend_card_skills_draw(anchors[6], card->equip_ids_0x08E);
    }
}

/* 0x80440D7C (0x80): draws the card's second page: the owner's server and settings rows. */
static void friend_card_place_draw(FriendCardView* view) {
    _mh_ivec2_ pos;

    get_lsp_data(0x2921, &pos);
    draw_sprite_ary((const u16*)get_menu_lsp_tbl(0x139), &pos);
    placeinfo_server_rows_draw(0x2971, 1, view->card);
    placeinfo_settings_rows_draw(0x2972, 1, view->card);
    get_lsp_data(0x2921, &pos);
    draw_sprite_ary((const u16*)get_menu_lsp_tbl(0x138), &pos);
}

/* 0x80440DFC (0xBC): copies the card's nine equipment records and draws its equipment page. */
static void friend_card_equip_draw(FriendCardView* view) {
    _EQUIP equip[9];
    NetPeerCard* card;
    u8 dye;

    card = view->card;
    dye = card->status_0x211;
    equip_record_copy(&equip[0], (_EQUIP*)card->equip_0x022[0]);
    equip_record_copy(&equip[1], (_EQUIP*)card->equip_0x022[1]);
    equip_record_copy(&equip[2], (_EQUIP*)card->equip_0x022[2]);
    equip_record_copy(&equip[3], (_EQUIP*)card->equip_0x022[3]);
    equip_record_copy(&equip[4], (_EQUIP*)card->equip_0x022[4]);
    equip_record_copy(&equip[5], (_EQUIP*)card->equip_0x022[5]);
    equip_record_copy(&equip[6], (_EQUIP*)card->equip_0x022[6]);
    equip_record_copy(&equip[7], (_EQUIP*)card->equip_0x022[7]);
    equip_record_copy(&equip[8], (_EQUIP*)card->equip_0x022[8]);
    friend_card_equip_page_draw(view, equip, dye);
}

/* 0x80440EB8 (0x250): draws the equipment page: the gun parts, the seven pieces and, when some piece cannot be
 * dyed, the dye colour. */
static void friend_card_equip_page_draw(FriendCardView* view, _EQUIP* equip, u8 dye) {
    _mh_ivec2_ pos;
    union {
        s32 word;
        GXColor rgba;
    } color;
    _SPR_DATA_ spr;
    u16* anchors;
    s32 dyeable;
    _SPR_DATA_* frame;
    s32 i;
    _EQUIP* piece;

    dyeable = 1;
    get_lsp_data(0x2981, &pos);
    draw_sprite_ary((const u16*)get_menu_lsp_tbl(0x13A), &pos);
    draw_sprite_ary((const u16*)get_menu_lsp_tbl(0x138), &pos);
    info_gun_parts_draw(0x2982, &equip[6], &equip[7], &equip[8]);
    frame = get_lsp_data(0x2983, &pos);
    draw_sprite_ary((const u16*)get_menu_lsp_tbl(0x141), &pos);
    anchors = (u16*)get_menu_lsp_tbl(0x142);
    i = 0;
    do {
        piece = NULL;
        switch (i) {
        case 0:
            piece = &equip[6];
            break;
        case 1:
            piece = &equip[4];
            break;
        case 2:
            piece = &equip[0];
            break;
        case 3:
            piece = &equip[1];
            break;
        case 4:
            piece = &equip[2];
            break;
        case 5:
            piece = &equip[3];
            break;
        case 6:
            piece = &equip[5];
            break;
        }
        if (piece != NULL) {
            if (i == 0) {
                if (piece->kind != 0) {
                    Put_equip_dtl_syo_item1(piece, &equip[7], 0, 0, 0, frame->pos, *anchors);
                }
            } else if (piece->kind != 0) {
                Put_equip_dtl_syo_item1(piece, NULL, 0, 0, 0, frame->pos, *anchors);
            }
            if (i - 1 <= 4U && EnableChangeColor(piece) == 0) {
                dyeable = 0;
            }
        }
        anchors++;
        i++;
    } while (i < 7);
    if (dyeable == 0 && dye != 0) {
        get_lsp_data(0x299A, &pos);
        draw_sprite_ary((const u16*)get_menu_lsp_tbl(0x83), &pos);
        color.word = equip_color_rgba_get(dye);
        spr_data_copy(&spr, get_lsp_data(0x7B9, NULL));
        spr.color = (color.rgba.r << 24) | (color.rgba.g << 16) | (color.rgba.b << 8) | 0xFF;
        draw_sprite(spr, &pos);
    }
}

/* 0x80441108 (0x1F8): draws the active-skill box at layout `id`: one row per skill (red when the tree has no
 * points, grey when the skill is invalid), or the "no skills" text. */
static void friend_card_skills_draw(u16 id, u16* skills) {
    _mh_ivec2_ origin;
    _mh_ivec2_ pos;
    s32 count;
    s32 i;
    s32 ofs;
    u16 skill;
    u32 color;
    u16* ids;

    ids = skills;
    count = 0;
    if (skills[0] != 0) {
        count = 1;
    }
    if (skills[1] != 0) {
        count++;
    }
    if (skills[2] != 0) {
        count++;
    }
    if (skills[3] != 0) {
        count++;
    }
    if (skills[4] != 0) {
        count++;
    }
    if (skills[5] != 0) {
        count++;
    }
    if (skills[6] != 0) {
        count++;
    }
    if (skills[7] != 0) {
        count++;
    }
    get_lsp_data(0x677, &origin);
    get_lsp_data(id, &pos);
    pos.x -= origin.x;
    pos.y -= origin.y;
    if (count != 0) {
        get_lsp_data(id, &origin);
        draw_sprite_anim_ary((const u16*)get_menu_lsp_tbl(0x80), 7, &origin);
        for (i = 0, ofs = 0; i < 8; ids++, ofs += 2, i++) {
            skill = *ids;
            if (skill != 0) {
                color = 0;
                if (skill & 0x4000) {
                    color = 0xE63737FF;
                } else if (skill & 0x8000) {
                    color = 0x878787FF;
                }
                skill_name_row_draw(((u16*)get_menu_lsp_tbl(0x19A))[i], skill & 0x3FFF, color, &pos);
            } else {
                skill_name_row_draw(((u16*)get_menu_lsp_tbl(0x19A))[i], 0xFFFF, 0, &pos);
            }
        }
    } else {
        get_lsp_data(id, &origin);
        draw_sprite_anim_ary((const u16*)get_menu_lsp_tbl(0x80), 7, &origin);
        draw_font_idx(0x678, (s8*)*get_str_tbl(0x28), 1, &origin);
    }
}

/* 0x80441300 (0xB8): writes the eight active-skill ids of `plw` to `ids`, flagged 0x4000 when the skill's tree has
 * no points and 0x8000 when the skill is not valid for the hunter. */
void card_equip_ids_build(_PLW* plw, u16* ids) {
    _EQUIP columns[9];
    s32 i;
    u16 flags;

    Set_equip_column_arrangement(plw, NULL, columns);
    for (i = 0; i < 8; i++) {
        flags = 0;
        if (plw->active_skill_id[i] != 0) {
            flags = (equip_columns_tree_points_get(columns, pl_skill_tree_get(plw->active_skill_id[i])) >> 31) & 0x4000;
            if (pl_skill_id_valid_ck(plw, plw->active_skill_id[i]) == 0) {
                flags = 0x8000;
            }
        }
        ids[i] = flags | plw->active_skill_id[i];
    }
}

/* 0x804413B8 (0x264): draws the confirmation window for `kind` with `choices` buttons and the cursor. */
static void friend_card_confirm_draw(FriendCardView* view, u8 choices, u16 kind) {
    _mh_ivec2_ pos;
    _mh_ivec2_ at;
    _SPR_DATA_ spr;
    _SPR_DATA_* rect;
    u16* anchors;
    s32 i;

    rect = get_lsp_data(0x299C, &pos);
    draw_layout_window(pos.x, pos.y, rect->width, rect->height);
    get_lsp_data(0x299E, &pos);
    if (kind == 2) {
        spr_data_copy(&spr, get_lsp_data(0x299F, NULL));
        draw_font(spr, (s8*)get_menu_msg(0x36, 0), 1, &pos);
        spr.color = 0xE63737FF;
        spr.pos.y += spr.height;
        draw_font(spr, (s8*)get_menu_msg(0x36, kind), 1, &pos);
    } else {
        draw_font_idx(0x299F, (s8*)get_menu_msg(0x36, kind), 8, &pos);
    }
    switch (choices) {
    case 1:
        rect = get_lsp_data(0x29A2, NULL);
        at.x = pos.x + rect->pos.x;
        at.y = pos.y + rect->pos.y;
        draw_sprite_ary((const u16*)get_menu_lsp_tbl(0xE6), &at);
        draw_font_idx(0xAB9, (s8*)get_menu_msg(13, 0), 1, &at);
        if (chk_pointer() == 0) {
            put_menu_cursor((u16*)get_menu_lsp_tbl(0xE7), 0, &at);
        }
        break;
    case 2:
        anchors = (u16*)get_menu_lsp_tbl(0x143);
        i = 0;
        do {
            rect = get_lsp_data(*anchors, NULL);
            at.x = pos.x + rect->pos.x;
            at.y = pos.y + rect->pos.y;
            draw_sprite_ary((const u16*)get_menu_lsp_tbl(0xE6), &at);
            draw_font_idx(0xAB9, (s8*)get_menu_msg(13, i), 1, &at);
            if (i == view->confirm_cursor && chk_pointer() == 0) {
                put_menu_cursor((u16*)get_menu_lsp_tbl(0xE7), 0, &at);
            }
            anchors++;
            i++;
        } while (i < 2);
        break;
    }
}

/* ---------------------------------------------------------------------------------------------------------------- */
/* The option pages: page 4 (the game options), its split-screen form (one per player) and page 17 (the network
 * options with the terms progress bar). */

/* The option ids page 4's rows set, in row order. */
u8 page04_option_ids[16] = { 20, 4, 17, 18, 13, 14, 15, 3, 8, 9, 2, 11, 6, 10, 0, 1 };
/* The option ids the split-screen page's rows set, in row order. */
u8 page04_split_option_ids[13] = { 20, 13, 14, 15, 3, 8, 9, 2, 11, 6, 10, 0, 1 };

/* Page 17's rows: the option ids of its first three rows, the system message ids of the transfer rows' titles and
 * values, and the highest value of each transfer row. */
u8 page17_option_ids[3] = { 12, 26, 27 };
u8 page17_title_msg[3] = { 0x2D, 0x30, 0x32 };
u8 page17_value_msg[3] = { 0x2E, 0x31, 0x34 };
u8 page17_value_max[3] = { 1, 7, 1 };

static void page04_rows_build(MenuSlot* slot);
static void page04_cursor_step(MenuSlot* slot, u16 input);
static s16 option_value_step(u16 input, u8 option);
static s16 page17_transfer_step(u16 input, u8 row);
static u32 page04_control_row_ok(MenuSlot* slot);
static u32 page04_pointer_row_ok(MenuSlot* slot);
static void option_value_sprite_draw(u16 id, u32 color, _mh_ivec2_* pos);
static void option_cursor_draw(MenuSlot* slot, MenuEntry* row, _mh_ivec2_* pos);
static void page04_split_scroll_init(MenuSlot* slot);
static s16 arena_value_step(u16 input, u8 option, u8 player);
static void page04_split_rows_build(MenuSlot* slot);
static void page04_split_cursor_step(MenuSlot* slot, u16 input);
static void page17_scroll_init(MenuSlot* slot);
static u8 page17_transfer_get(u8 row);
static void page17_transfer_set(u8 row, u8 value);
static void page17_rows_build(MenuSlot* slot);
static void page17_cursor_step(MenuSlot* slot, u16 input);
static void terms_progress_draw(_mh_ivec2_* pos);
static void page04_scroll_init(MenuSlot* slot);

/* 0x8044161C (0x14): opens menu page 4. */
void movie_page04_open(MenuSlot* slot) {
    slot->field_0x014 = 4;
    slot->field_0x001 = 0;
    page04_scroll_init(slot);
}

/* 0x80441630 (0x44): sets page 4's scroll (16 rows, 7 a page) and fills its rows. */
static void page04_scroll_init(MenuSlot* slot) {
    menu_scroll_init(&slot->scroll, 0, 16, 7);
    page04_rows_build(slot);
}

/* 0x80441674 (0x1D4): steps page 4: pick a row, then change its value until confirmed (A) or cancelled (B). */
s32 movie_page04_step(MenuSlot* slot) {
    s32 ret;
    u16 input;

    ret = 0;
    input = slot->buttons_trig | (slot->field_0x008 & 0xF);
    switch (slot->field_0x001) {
    case 0:
        if (input & 0x20) {
            ret = 2;
            sysSE_req(1);
        } else if (input & 0x10) {
            if (slot->entries_b[slot->scroll.row].field_0x02 != 0) {
                menu_slot0_lock_set(1);
                slot->option_value_0x234 = get_cfg(slot->worker->chunk_ofs, page04_option_ids[slot->scroll.index]);
                slot->field_0x001 = 1;
                sysSE_req(0);
            } else {
                sysSE_req(2);
            }
        } else {
            page04_cursor_step(slot, input);
        }
        break;
    case 1:
        slot->option_move_0x236 = 0;
        if (input & 0x20) {
            if (system_w.vs_mode_0x8b0 == 0) {
                set_option_cfg(page04_option_ids[slot->scroll.index], slot->option_value_0x234);
            } else {
                arena_cfg_set(slot->worker->chunk_ofs, page04_option_ids[slot->scroll.index], slot->option_value_0x234);
            }
            page04_rows_build(slot);
            slot->field_0x001 = 0;
            sysSE_req(1);
            menu_slot0_lock_set(0);
        } else if (input & 0x10) {
            slot->field_0x001 = 0;
            sysSE_req(0);
            menu_slot0_lock_set(0);
        } else if (input & 0xC) {
            slot->option_move_0x236 = option_value_step(input, page04_option_ids[slot->scroll.index]);
            page04_rows_build(slot);
        }
        break;
    }
    return ret;
}

/* 0x80441848 (0x58): moves page 4's cursor and refills the rows when it moved. */
static void page04_cursor_step(MenuSlot* slot, u16 input) {
    MenuScroll* scroll;

    scroll = &slot->scroll;
    menu_scroll_step(scroll, input, 5);
    if (scroll->move_flags != 0) {
        page04_rows_build(slot);
    }
}

/* 0x804418A0 (0x16C): raises (right) or lowers (left) option `option` by one; returns the arrow that moved it. */
static s16 option_value_step(u16 input, u8 option) {
    s16 moved;
    u8 value;
    u8 next;

    moved = 0;
    if (system_w.vs_mode_0x8b0 == 0) {
        value = get_option_cfg(option);
    } else {
        value = get_arena_cfg(0, option);
    }
    next = value;
    if (input & 8) {
        next++;
        moved = 8;
    } else if (input & 4) {
        if (value != 0) {
            next = value - 1;
        }
        moved = 4;
    }
    if (system_w.vs_mode_0x8b0 == 0) {
        if (next != ck_option_cfg(option)) {
            set_option_cfg(option, next);
            if (next == get_option_cfg(option)) {
                sysSE_req(3);
                return moved;
            }
        }
    } else if (next != ck_arena_cfg(0, option)) {
        arena_cfg_set(0, option, next);
        if (next == get_arena_cfg(0, option)) {
            sysSE_req(3);
            return moved;
        }
    }
    return 0;
}

/* 0x80441A0C (0xE0): raises or lowers transfer row `row` by one; returns the arrow that moved it. */
static s16 page17_transfer_step(u16 input, u8 row) {
    s16 moved;
    u8 value;
    u8 next;

    moved = 0;
    value = page17_transfer_get(row);
    next = value;
    if (input & 8) {
        next++;
        moved = 8;
    } else if (input & 4) {
        if (value != 0) {
            next = value - 1;
        }
        moved = 4;
    }
    if (next != page17_transfer_get(row)) {
        page17_transfer_set(row, next);
        if (next == page17_transfer_get(row)) {
            sysSE_req(3);
            return moved;
        }
    }
    return 0;
}

/* 0x80441AEC (0x78): 1 when the slot's controller can use the controller-type row. */
static u32 page04_control_row_ok(MenuSlot* slot) {
    u32 ok;

    ok = 0;
    switch ((u8)get_ControlType(slot->field_0x011)) {
    case 0:
        ok = 1;
        break;
    case 1:
        ok = 1;
        break;
    case 2:
        ok = 0;
        break;
    case 0xFF:
        ok = 1;
        break;
    }
    return ok;
}

/* 0x80441B64 (0x78): 1 when the slot's controller can use the pointer rows. */
static u32 page04_pointer_row_ok(MenuSlot* slot) {
    u32 ok;

    ok = 0;
    switch ((u8)get_ControlType(slot->field_0x011)) {
    case 0:
        ok = 1;
        break;
    case 1:
        ok = 1;
        break;
    case 2:
        ok = 0;
        break;
    case 0xFF:
        ok = 1;
        break;
    }
    return ok;
}

/* 0x80441BDC (0x1A4): fills page 4's rows of the page: enabled, colour, and whether the value is at an end. */
static void page04_rows_build(MenuSlot* slot) {
    u8* option;
    MenuEntry* row;
    s16 index;
    s32 i;
    u8 value;
    u32 color;

    row = slot->entries_b;
    index = slot->scroll.rows_per_page * slot->scroll.col;
    i = 0;
    option = &page04_option_ids[index];
    for (; i < slot->scroll.row_limit; option++, index++, row++, i++) {
        row->field_0x02 = 1;
        row->page_kind = index;
        row->selected = i == slot->scroll.row;
        switch (index) {
        case 6:
            if (ck_cfg(slot->worker->chunk_ofs, 14) == 1) {
                row->field_0x02 = 0;
            }
            break;
        case 0:
            if (page04_control_row_ok(slot) == 0) {
                row->field_0x02 = 0;
            }
            break;
        case 14:
        case 15:
            if (page04_pointer_row_ok(slot) == 0) {
                row->field_0x02 = 0;
            }
            break;
        }
        if (row->field_0x02 == 0) {
            row->field_0x0C = 0x8C8C8CFF;
        } else {
            if (row->selected == 1) {
                color = 0x7DE6BEFF;
            } else {
                color = 0x6E8C7EFF;
            }
            row->field_0x0C = color;
        }
        value = get_cfg(slot->worker->chunk_ofs, *option);
        if (value == 0) {
            row->field_0x06 = 1;
        } else if (value >= option_cfg_max[*option]) {
            row->field_0x06 = 2;
        } else {
            row->field_0x06 = 0;
        }
    }
}

/* 0x80441D80 (0x3CC): draws page 4: the arrows, each row's name and value, the cursor and the help text. */
void movie_page04_draw(MenuSlot* slot) {
    _mh_ivec2_ pos;
    _mh_ivec2_ at;
    _SPR_DATA_ spr;
    u8 arrows;
    u16* names;
    u16* values;
    s32 i;
    s8* text;
    MenuEntry* row;
    s32 color;
    u16* sprites;
    u8** msgs;
    u16 index;

    get_lsp_data(0x74C, &pos);
    draw_sprite_ary((const u16*)get_menu_lsp_tbl(0xD7), &pos);
    arrows = slot->field_0x001 == 0 ? 0 : 0x80;
    PutPageArrow((u16*)get_menu_lsp_tbl(0xD9), slot->scroll.col, slot->scroll.col_count, slot->scroll.move_flags, &pos,
                 arrows);
    row = slot->entries_b;
    names = (u16*)get_menu_lsp_tbl(0xDA);
    values = (u16*)get_menu_lsp_tbl(0xDB);
    for (i = 0; i < slot->scroll.row_limit; values++, i++, row++) {
        if (names[row->page_kind] == 0xFFFF) {
            continue;
        }
        spr_data_copy(&spr, get_lsp_data(names[row->page_kind], NULL));
        spr.color = row->field_0x0C;
        text = (s8*)get_menu_msg(0x1E, row->page_kind);
        draw_font(spr, text, 4, &pos);
        get_lsp_data(*values, &at);
        at.x += pos.x;
        at.y += pos.y;
        draw_sprite_ary((const u16*)get_menu_lsp_tbl(0xDC), &at);
        color = GetMenuFontColor(row->field_0x02 == 1, row->selected == 1, 1, 0);
        if (row->selected != 0) {
            option_cursor_draw(slot, row, &at);
        }
        switch (row->page_kind) {
        default:
            sprites = ((u16**)get_menu_lsp_tbl(0xE2))[row->page_kind];
            if (sprites != NULL) {
                option_value_sprite_draw(sprites[ck_cfg(slot->worker->chunk_ofs, page04_option_ids[row->page_kind])],
                                         color, &at);
            }
            break;
        case 6:
        case 7:
        case 11:
        case 13:
            sprites = ((u16**)get_menu_lsp_tbl(0xE2))[row->page_kind];
            if (sprites != NULL) {
                option_value_sprite_draw(sprites[ck_cfg(slot->worker->chunk_ofs, page04_option_ids[row->page_kind])],
                                         color, &at);
            }
            break;
        case 2:
        case 3:
            switch (row->field_0x06) {
            case 0:
                draw_number_idx(0x78B, ck_cfg(slot->worker->chunk_ofs, page04_option_ids[row->page_kind]), color, &at);
                break;
            case 1:
                option_value_sprite_draw(0x789, color, &at);
                break;
            case 2:
                option_value_sprite_draw(0x78A, color, &at);
                break;
            }
            break;
        }
    }
    get_lsp_data(0x794, &pos);
    draw_window_frame_preset(pos.x, pos.y, 0);
    font_set_size(16, 16);
    switch (slot->field_0x001) {
    case 0:
        text = (s8*)get_menu_msg(0x21, slot->scroll.index);
        break;
    case 1:
        index = slot->scroll.index;
        msgs = ((u8***)get_menu_msg_tbl(0x23))[index];
        if ((u16)(index - 2) <= 1) {
            text = (s8*)msgs[0];
        } else {
            text = (s8*)msgs[ck_cfg(slot->worker->chunk_ofs, page04_option_ids[index])];
        }
        break;
    }
    font_print_ex(pos.x + 24, pos.y + 16, 0, text);
}

/* 0x8044214C (0x98): draws sprite record `id` in `color` at `pos`. */
static void option_value_sprite_draw(u16 id, u32 color, _mh_ivec2_* pos) {
    _SPR_DATA_ spr = *get_lsp_data(id, NULL);

    spr.color = color;
    draw_sprite(spr, pos);
}

/* 0x804421E4 (0xB0): draws the row cursor and, while a value is being changed, its blinking arrows. */
static void option_cursor_draw(MenuSlot* slot, MenuEntry* row, _mh_ivec2_* pos) {
    s16 state;
    u16 left;
    u16 right;

    if (chk_pointer() == 0) {
        put_menu_cursor((u16*)get_menu_lsp_tbl(0xDD), 0, pos);
        if (slot->field_0x001 == 1) {
            state = row->field_0x06;
            left = 0xFFFF;
            if (state != 1) {
                left = 0x776;
            }
            right = 0xFFFF;
            if (state != 2) {
                right = 0x777;
            }
            draw_sprite_blink_idx(left, right, 0xFFFFC8FF, slot->option_move_0x236, pos);
        }
    }
}

/* 0x80442294 (0x14): opens the split-screen option page. */
void movie_page04_split_open(MenuSlot* slot) {
    slot->field_0x014 = 4;
    slot->field_0x001 = 0;
    page04_split_scroll_init(slot);
}

/* 0x804422A8 (0x44): sets the split page's scroll (13 rows, 5 a page) and fills its rows. */
static void page04_split_scroll_init(MenuSlot* slot) {
    menu_scroll_init(&slot->scroll, 0, 13, 5);
    page04_split_rows_build(slot);
}

/* 0x804422EC (0xE4): raises or lowers player `player`'s option `option` by one; returns the arrow that moved it. */
static s16 arena_value_step(u16 input, u8 option, u8 player) {
    s16 moved;
    u8 value;
    u8 next;

    moved = 0;
    value = get_arena_cfg(player, option);
    next = value;
    if (input & 8) {
        next++;
        moved = 8;
    } else if (input & 4) {
        if (value != 0) {
            next = value - 1;
        }
        moved = 4;
    }
    if (next != ck_arena_cfg(player, option)) {
        arena_cfg_set(player, option, next);
        if (next == get_arena_cfg(player, option)) {
            sysSE_req(3);
            return moved;
        }
    }
    return 0;
}

/* 0x804423D0 (0x19C): fills the split page's rows of the page. */
static void page04_split_rows_build(MenuSlot* slot) {
    u8* option;
    MenuEntry* row;
    s16 index;
    s32 i;
    u8 value;
    u32 color;

    row = slot->entries_b;
    index = slot->scroll.rows_per_page * slot->scroll.col;
    i = 0;
    option = &page04_split_option_ids[index];
    for (; i < slot->scroll.row_limit; option++, index++, row++, i++) {
        row->field_0x02 = 1;
        row->page_kind = index;
        row->selected = i == slot->scroll.row;
        switch (index) {
        case 3:
            if (ck_arena_cfg(slot->field_0x011, 14) == 1) {
                row->field_0x02 = 0;
            }
            break;
        case 0:
            if (page04_control_row_ok(slot) == 0) {
                row->field_0x02 = 0;
            }
            break;
        case 11:
        case 12:
            if (page04_pointer_row_ok(slot) == 0) {
                row->field_0x02 = 0;
            }
            break;
        }
        if (row->field_0x02 == 0) {
            row->field_0x0C = 0x8C8C8CFF;
        } else {
            if (row->selected == 1) {
                color = 0x7DE6BEFF;
            } else {
                color = 0x6E8C7EFF;
            }
            row->field_0x0C = color;
        }
        value = get_arena_cfg(slot->field_0x011, *option);
        if (value == 0) {
            row->field_0x06 = 1;
        } else if (value >= option_cfg_max[*option]) {
            row->field_0x06 = 2;
        } else {
            row->field_0x06 = 0;
        }
    }
}

/* 0x8044256C (0x1B0): steps the split option page for the slot's player. */
s32 movie_page04_split_step(MenuSlot* slot) {
    s32 ret;
    u16 input;

    ret = 0;
    input = slot->buttons_trig | (slot->field_0x008 & 0xF);
    switch (slot->field_0x001) {
    case 0:
        if (input & 0x20) {
            ret = 2;
            sysSE_req(1);
        } else if (input & 0x10) {
            if (slot->entries_b[slot->scroll.row].field_0x02 != 0) {
                menu_slot_lock_set(slot->pad_no, 1);
                slot->option_value_0x234 = get_arena_cfg(slot->field_0x011, page04_split_option_ids[slot->scroll.index]);
                slot->field_0x001 = 1;
                sysSE_req(0);
            } else {
                sysSE_req(2);
            }
        } else {
            page04_split_cursor_step(slot, input);
        }
        break;
    case 1:
        slot->option_move_0x236 = 0;
        if (input & 0x20) {
            arena_cfg_set(slot->field_0x011, page04_split_option_ids[slot->scroll.index], slot->option_value_0x234);
            page04_split_rows_build(slot);
            slot->field_0x001 = 0;
            sysSE_req(1);
            menu_slot_lock_set(slot->pad_no, 0);
        } else if (input & 0x10) {
            slot->field_0x001 = 0;
            sysSE_req(0);
            menu_slot_lock_set(slot->pad_no, 0);
        } else if (input & 0xC) {
            slot->option_move_0x236 =
                arena_value_step(input, page04_split_option_ids[slot->scroll.index], slot->field_0x011);
            page04_split_rows_build(slot);
        }
        break;
    }
    return ret;
}

/* 0x8044271C (0x58): moves the split page's cursor and refills the rows when it moved. */
static void page04_split_cursor_step(MenuSlot* slot, u16 input) {
    MenuScroll* scroll;

    scroll = &slot->scroll;
    menu_scroll_step(scroll, input, 5);
    if (scroll->move_flags != 0) {
        page04_split_rows_build(slot);
    }
}

/* 0x80442774 (0x268): draws the split option page in the slot's half of the screen. */
void movie_page04_split_draw(MenuSlot* slot) {
    _mh_ivec2_ pos;
    _SPR_DATA_ spr;
    _mh_ivec2_ at;
    u16* names;
    u16* values;
    s32 i;
    _SPR_DATA_* anchor;
    MenuEntry* row;
    s32 color;
    u16** sprites;

    get_lsp_data(((u16*)get_menu_lsp_tbl(0x18A))[slot->pad_no], &pos);
    draw_sprite_ary((const u16*)get_menu_lsp_tbl(0x18B), &pos);
    PutPageArrow((u16*)get_menu_lsp_tbl(0x18D), slot->scroll.col, slot->scroll.col_count, slot->scroll.move_flags, &pos,
                 0);
    row = slot->entries_b;
    names = (u16*)get_menu_lsp_tbl(0x18E);
    values = (u16*)get_menu_lsp_tbl(0x18F);
    for (i = 0; i < slot->scroll.row_limit; values++, i++, row++) {
        spr_data_copy(&spr, get_lsp_data(names[row->page_kind], NULL));
        spr.color = row->field_0x0C;
        draw_font(spr, (s8*)get_menu_msg(0x1F, row->page_kind), 4, &pos);
        anchor = get_lsp_data(*values, NULL);
        at.x = pos.x + anchor->pos.x;
        at.y = pos.y + anchor->pos.y;
        draw_sprite_ary((const u16*)get_menu_lsp_tbl(0xDC), &at);
        if (row->selected != 0) {
            option_cursor_draw(slot, row, &at);
        }
        color = GetMenuFontColor(row->field_0x02 == 1, row->selected == 1, 1, 0);
        switch (row->page_kind) {
        default:
            sprites = (u16**)get_menu_lsp_tbl(0x190);
            option_value_sprite_draw(
                sprites[row->page_kind][ck_arena_cfg(slot->field_0x011, page04_split_option_ids[row->page_kind])],
                color, &at);
            break;
        case 3:
        case 4:
        case 8:
        case 10:
            sprites = (u16**)get_menu_lsp_tbl(0x190);
            option_value_sprite_draw(
                sprites[row->page_kind][ck_arena_cfg(slot->field_0x011, page04_split_option_ids[row->page_kind])],
                color, &at);
            break;
        }
    }
}

/* 0x804429DC (0x14): opens menu page 17. */
void movie_page17_open(MenuSlot* slot) {
    slot->field_0x014 = 17;
    slot->field_0x001 = 0;
    page17_scroll_init(slot);
}

/* 0x804429F0 (0x7C): sets page 17's scroll (three rows, six with the network option on) and fills its rows. */
static void page17_scroll_init(MenuSlot* slot) {
    MenuScroll* scroll;

    scroll = &slot->scroll;
    if (get_option_cfg(28) == 0) {
        menu_scroll_init(scroll, 0, 3, 3);
    } else {
        menu_scroll_init(scroll, 0, 6, 3);
    }
    page17_rows_build(slot);
}

/* 0x80442A6C (0x80): the value of transfer row `row` (3 transfer on/off, 4 level, 5 flag). */
static u8 page17_transfer_get(u8 row) {
    switch (row) {
    case 3:
        if (system_w.transfer_mode_0xa50 == 1) {
            return 0;
        }
        return 1;
    case 4:
        return system_w.transfer_level_0xa52;
    case 5:
        if (system_w.transfer_flag_0xa51 == 1) {
            return 0;
        }
        return 1;
    }
    return 0;
}

/* 0x80442AEC (0xBC): stores `value` (clamped) as transfer row `row` and applies it. */
static void page17_transfer_set(u8 row, u8 value) {
    u8 max;

    max = page17_value_max[row - 3];
    if (value >= max) {
        value = max;
    }
    switch (row) {
    case 3:
        if (value == 0) {
            system_w.transfer_mode_0xa50 = 1;
        } else {
            system_w.transfer_mode_0xa50 = 0;
        }
        applyTransferSettings();
        break;
    case 4:
        system_w.transfer_level_0xa52 = value;
        applyTransferLevel();
        break;
    case 5:
        if (value == 0) {
            system_w.transfer_flag_0xa51 = 1;
        } else {
            system_w.transfer_flag_0xa51 = 0;
        }
        syncNetTransferFlag();
        break;
    }
}

/* 0x80442BA8 (0x1B8): fills page 17's rows of the page. */
#pragma opt_strength_reduction off
static void page17_rows_build(MenuSlot* slot) {
    MenuEntry* row;
    s16 index;
    s32 i;
    u8 value;
    u32 color;

    row = slot->entries_b;
    index = slot->scroll.rows_per_page * slot->scroll.col;
    i = 0;
    for (; i < slot->scroll.row_limit; index++, row++, i++) {
        row->page_kind = index;
        row->selected = i == slot->scroll.row;
        if (i == 2 && slot->field_0x00F == 1) {
            row->field_0x02 = 0;
        } else {
            row->field_0x02 = 1;
        }
        switch (index) {
        case 4:
        case 5:
            if (page17_transfer_get(3) == 1) {
                row->field_0x02 = 0;
            } else {
                row->field_0x02 = 1;
            }
            break;
        }
        if (row->field_0x02 == 0) {
            row->field_0x0C = 0x8C8C8CFF;
        } else {
            if (row->selected == 1) {
                color = 0x7DE6BEFF;
            } else {
                color = 0x6E8C7EFF;
            }
            row->field_0x0C = color;
        }
        if (index < 3) {
            value = get_option_cfg(page17_option_ids[index]);
            if (value == 0) {
                row->field_0x06 = 1;
            } else if (value >= option_cfg_max[page17_option_ids[index]]) {
                row->field_0x06 = 2;
            } else {
                row->field_0x06 = 0;
            }
        } else {
            value = page17_transfer_get(index);
            if (value == 0) {
                row->field_0x06 = 1;
            } else if (value >= page17_value_max[index - 3]) {
                row->field_0x06 = 2;
            } else {
                row->field_0x06 = 0;
            }
        }
    }
}

#pragma opt_strength_reduction reset
/* 0x80442D60 (0x1D4): steps page 17: the option rows and the transfer rows. */
s32 movie_page17_step(MenuSlot* slot) {
    s32 ret;
    u16 input;

    ret = 0;
    input = slot->buttons_trig | (slot->field_0x008 & 0xF);
    switch (slot->field_0x001) {
    case 0:
        if (input & 0x20) {
            ret = 2;
            sysSE_req(1);
        } else if (input & 0x10) {
            if (slot->entries_b[slot->scroll.row].field_0x02 != 0) {
                if (slot->scroll.index <= 2) {
                    slot->option_value_0x234 = get_option_cfg(page17_option_ids[slot->scroll.index]);
                } else {
                    slot->option_value_0x234 = page17_transfer_get(slot->scroll.index);
                }
                slot->field_0x001 = 1;
                sysSE_req(0);
                menu_slot0_lock_set(1);
            } else {
                sysSE_req(2);
            }
        } else {
            page17_cursor_step(slot, input);
        }
        break;
    case 1:
        slot->option_move_0x236 = 0;
        if (input & 0x20) {
            if (slot->scroll.index <= 2) {
                set_option_cfg(page17_option_ids[slot->scroll.index], slot->option_value_0x234);
            } else {
                page17_transfer_set(slot->scroll.index, slot->option_value_0x234);
            }
            page17_rows_build(slot);
            slot->field_0x001 = 0;
            sysSE_req(1);
            menu_slot0_lock_set(0);
        } else if (input & 0x10) {
            slot->field_0x001 = 0;
            sysSE_req(0);
            menu_slot0_lock_set(0);
        } else if (input & 0xC) {
            if (slot->scroll.index <= 2) {
                slot->option_move_0x236 = option_value_step(input, page17_option_ids[slot->scroll.index]);
            } else {
                slot->option_move_0x236 = page17_transfer_step(input, slot->scroll.index);
            }
            page17_rows_build(slot);
        }
        break;
    }
    return ret;
}

/* 0x80442F34 (0x58): moves page 17's cursor and refills the rows when it moved. */
static void page17_cursor_step(MenuSlot* slot, u16 input) {
    MenuScroll* scroll;

    scroll = &slot->scroll;
    menu_scroll_step(scroll, input, 5);
    if (scroll->move_flags != 0) {
        page17_rows_build(slot);
    }
}

/* 0x80442F8C (0x194): draws the terms progress bar at `pos`: its frame, its black well and the filled part. */
static void terms_progress_draw(_mh_ivec2_* pos) {
    Vec at;
    Vec2 size;
    GXColor color;
    f32 rate;

    at.x = 8.0f + pos->x;
    at.y = 5.0f + pos->y;
    at.z = 0.0f;
    size.x = 206.0f;
    size.y = 11.0f;
    color.r = 0x5F;
    color.g = 0x55;
    color.b = 0x4B;
    color.a = 0xFF;
    disp_beta(&at, &size, color);
    at.x += 3.0f;
    at.y += 3.0f;
    size.x = 200.0f;
    size.y = 5.0f;
    disp_beta(&at, &size, color_black);
    rate = getNetTermsProgressLevel();
    rate *= 0.0625f;
    if (rate > 1.0f) {
        rate = 1.0f;
    } else if (rate < 0.0f) {
        rate = 0.0f;
    }
    size.x = 200.0f * rate;
    color.r = 0x57;
    color.g = 0xC0;
    color.b = 0x91;
    color.a = 0xFF;
    disp_beta(&at, &size, color);
}

/* 0x80443120 (0x434): draws page 17: the rows, their values, the cursor, the progress bar and the help text. */
void movie_page17_draw(MenuSlot* slot) {
    _mh_ivec2_ pos;
    _mh_ivec2_ at;
    _SPR_DATA_ spr;
    u8 pages;
    u8 arrows;
    MenuEntry* row;
    u16* names;
    u16* values;
    s32 i;
    s8* text;
    s32 color;
    u16* sprites;
    u8** msgs;
    u16 index;
    u8 value;
    u8*** msg_tables;

    get_lsp_data(0x74C, &pos);
    draw_sprite_ary((const u16*)get_menu_lsp_tbl(0xD7), &pos);
    pages = slot->scroll.col_count;
    if (pages >= 2) {
        arrows = slot->field_0x001 == 0 ? 0 : 0x80;
        PutPageArrow((u16*)get_menu_lsp_tbl(0xD9), slot->scroll.col, pages, slot->scroll.move_flags, &pos, arrows);
    }
    row = slot->entries_b;
    names = (u16*)get_menu_lsp_tbl(0xDA);
    values = (u16*)get_menu_lsp_tbl(0xDB);
    for (i = 0; i < slot->scroll.row_limit; names++, values++, i++, row++) {
        if (row->page_kind == -1 || *names == 0xFFFF) {
            continue;
        }
        spr_data_copy(&spr, get_lsp_data(*names, NULL));
        spr.color = row->field_0x0C;
        if (row->page_kind <= 2) {
            text = (s8*)get_menu_msg(0x20, row->page_kind);
        } else {
            text = (s8*)MH3GetErrorString2(row->page_kind + 0x27);
        }
        draw_font(spr, text, 4, &pos);
        get_lsp_data(*values, &at);
        at.x += pos.x;
        at.y += pos.y;
        draw_sprite_ary((const u16*)get_menu_lsp_tbl(0xDC), &at);
        color = GetMenuFontColor(row->field_0x02 == 1, row->selected == 1, 1, 0);
        if (row->selected != 0) {
            option_cursor_draw(slot, row, &at);
        }
        if (row->page_kind == 4) {
            switch (row->field_0x06) {
            case 0:
                draw_number_idx(0x78B, page17_transfer_get(row->page_kind), color, &at);
                break;
            case 1:
                option_value_sprite_draw(0x789, color, &at);
                break;
            case 2:
                option_value_sprite_draw(0x78A, color, &at);
                break;
            }
        } else {
            sprites = ((u16**)get_menu_lsp_tbl(0xE3))[row->page_kind];
            if (sprites != NULL) {
                if (row->page_kind <= 2) {
                    option_value_sprite_draw(sprites[ck_option_cfg(page17_option_ids[row->page_kind])], color, &at);
                } else {
                    option_value_sprite_draw(sprites[page17_transfer_get(row->page_kind)], color, &at);
                }
            }
        }
    }
    if (slot->scroll.col == 1) {
        get_lsp_data(0x762, &at);
        at.x += pos.x;
        at.y += pos.y;
        terms_progress_draw(&at);
    }
    get_lsp_data(0x794, &pos);
    draw_window_frame_preset(pos.x, pos.y, 0);
    font_set_size(16, 16);
    switch (slot->field_0x001) {
    case 0:
        index = slot->scroll.index;
        if (index <= 2) {
            text = (s8*)get_menu_msg(0x22, index);
        } else {
            text = (s8*)MH3GetErrorString2(page17_title_msg[index - 3]);
        }
        break;
    case 1:
        msg_tables = (u8***)get_menu_msg_tbl(0x24);
        msgs = msg_tables[slot->scroll.index];
        if (slot->scroll.index <= 2) {
            text = (s8*)msgs[ck_option_cfg(page17_option_ids[slot->scroll.index])];
        } else {
            switch (slot->scroll.index) {
            case 3:
            case 5:
                value = page17_transfer_get(slot->scroll.index);
                text = (s8*)MH3GetErrorString2(value + page17_value_msg[slot->scroll.index - 3]);
                break;
            default:
                text = (s8*)MH3GetErrorString2(page17_value_msg[slot->scroll.index - 3]);
                break;
            }
        }
        break;
    }
    font_print_ex(pos.x + 24, pos.y + 16, 0, text);
}
