/*
 * hud/lobby_hud.cpp - the lobby HUD: the per-frame HUD step and its reset, the move-work markers (a player's and an
 *   NPC's), the keyed table and the animated sprite runs the move-work draw task calls.
 * RANGE. .text 0x8032422C-0x80324F7C (6 functions); extab 0x800161EC-0x8001621C, extabindex 0x80035148-0x80035190,
 *   .data 0x805DD8D0-0x805DD9C8, .sdata 0x80792C78-0x80792CC8, .sdata2 0x8079AF10-0x8079AF48.  One TU: its pool
 *   (0.0f, 1.0f, 12.0f, 1.0714f, 24.0f, the int-to-float double, 200.0f, 1600.0f, 2.1f, 6.0f, 50.0f, 0.5f) is
 *   loaded only by `move_work_draw`, `lb_npc_move_work_draw` and `draw_lsp_anim_ary`; the `.sdata` run opens with
 *   `lb_npc_move_work_draw`'s "HD-00" literal and the `.data` run with `hud_key_frame_spr`.  The start edge is
 *   0x8032422C or 0x80324304 (see `menu/menu_quest.cpp`): `lobby_hud_reset`/`lobby_hud_frame_update` load no data.
 * FLAGS. `cflags_menu` (configure.py, the group of the range it was cut from); file-scope `#pragma pool_data off`,
 *   `#pragma fp_contract off` and `#pragma optimization_level 4`; `infer.py` on the target: `draw_lsp_anim_ary`
 *   keeps 0 record forms with 2 fold-shaped pairs.
 * NAMES. Module `hud` from the HUD callees and the HUD units that follow (`hud/move_work_update.cpp` calls these
 *   draws); no `__FILE__` string covers the range; the file name is a GUESS.  GUESS: `lobby_hud_frame_update`,
 *   `lobby_hud_reset`, the table names; foreign (registry): `mhchar_node_id_get`, `lb_npc_mark_state_get`,
 *   `hud_info_toggle_step`.
 * RESIDUALS. Register allocation residuals: `hud_key_table_draw` (the `ids` walker and the strength-reduced
 *   `cockpit_state` pointer swap r27/r28), `draw_lsp_anim_ary` (a callee-saved permutation no declaration order
 *   reaches).  `.sdata2`: `draw_lsp_anim_ary` also loads 2.0f at 0x8079AF48, the first word of
 *   `enemy/em022_prog.cpp`'s claim; our pool is 0x3C (with the 2.0f) and em022's 4 bytes short, but a seam at
 *   0x8079AF4C does not link (`ELF_gen.c` 2802: em022's 8-byte entries would start 4 mod 8, also with `align:4`).
 *   flipcheck: `.text` (the two rows above) and `.sdata2` (0x3C against the 0x38 claim).
 */

#include "types.h"
#include "lobby/lobby_w.h"
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "ai/fn_802D44F4.h" /* ai_slots_clear (rule 2: its owner) */
#include "lobby/lb_npc.h" /* lb_party_state_reset (rule 2: its owner) */
#include "fn_8004CAD8.h" /* calcVecAngXY / MTX34_ctor / vec3_scale_in_place / userdata_flag_ck (rule 2: its owner) */
#include "Network/network_pat_control.h" /* net_ctrl_wk */
#include "Network/net_session_close.h" /* requestSessionJoin / getSessionJoinResult / the profile checks */
#include "camera/camera.h" /* camera_talk_lock_set */
#include "hud/layout.h" /* the draw_* sprite and font calls */
#include "hud/cockpit.h" /* chk_pointer / PutPageArrow / ainpc_page_hold_set / setCockpitTransferMode */
#include "menu/get_pop_dat_ptr.h" /* get_option_cfg */
#include "hud/cockpit_notice_update.h" /* cockpit_notice_init / cockpit_notice_update */
#include "hud/move_work_update.h"
#include "hud/cockpit_state.h"
#include "lobby/lb_item_list_depth_get.h"
#include "lobby/lb_npc_talk_mark_ck.h"
#include "lobby/lobby_hunter_cards.h"
#include "lobby/lb_menu_pos_tbl.h" /* getItemListSelection */
#include "sound/mhchar_node_id_get.h"
#include "ef/lb_npc_mark_state_get.h"
#include "ef/pRoot.h"
#include "enemy/hud_info_toggle_step.h"
#include "hud/chara_icon_pos_get.h"
#include "lobby/lb_equip_page.h" /* pl_eft_draw_ck */
#include "menu/chara_icon_tex_tbl.h"
#include "g3d/g3d_camera.h" /* camera_world_to_screen */
#include "g3d/g3d_scnroot.h" /* scn_root_get_current_camera */
#include "hud/hud_screen_width_get.h"
#include "quest/quest_entry.h" /* quest_record_find / quest_result_field_text_get */
#include "stage/stg_w.h" /* get_now_areano */

/* retail addresses each table of this file with its own lis/addi pair, and keeps every float multiply and add
 * apart (fmuls + fadds, no fmadds) */
#pragma pool_data off
#pragma fp_contract off
#pragma optimization_level 4

/* 0x8032422C - reset the ten slots and run the screen's two post steps. */
extern "C" void lobby_hud_reset(void) {
    s32 i;

    for (i = 0; i < 10; i++) {
        setCockpitTransferMode((u8)i, 0);
    }
    cockpit_notice_init();
    ai_slots_clear();
}

/* 0x80324274 (0x90): the lobby's per-frame HUD step: the info popup, the notices, the NPC entries and the menu
 * frame, then the move-work, NPC, menu panel and parts draw tasks. */
extern "C" void lobby_hud_frame_update(void) {
    hud_info_toggle_step();
    cockpit_notice_update();
    ainpc_entry_update_all();
    menu_slot_anim_update(0);
    subTransSetPrio(1, (u32)move_work_update, 0, NULL);
    subTransSetPrio(6, (u32)ainpc_entry_draw_all, 0, NULL);
    subTransSetPrio(6, (u32)menu_slot_panel_draw, 0, NULL);
    subTransSetPrio(7, (u32)draw_lsp_parts, 0, NULL);
}

/* 0x80324304 (0x240): draws move work `mw`'s marker: projected from its position in its own area (`mode` 1), or at
 * `pos`; coloured by its board objective and blinking while it is away. */
extern "C" void move_work_draw(_PLW* mw, _mh_ivec2_* pos, u32 mode) {
    u32 camera;
    s32 camera_id;
    nw4r::math::VEC3 scr;
    nw4r::math::VEC3 wpos;
    nw4r::math::VEC3 icon;
    s16 size;
    u32 color;
    s32 tex;

    camera_id = scn_root_get_current_camera(pRoot);
    word_copy_return_dst(&camera, &camera_id);
    VEC3_ctor(&scr);
    assignVec3((Vec*)&wpos, (Vec*)&mw->field_0x03C);
    if ((mw->field_0xB01 & 4) && (lobby_w.counter_0x030 & 0x10)) {
        return;
    }
    if (mode == 1) {
        if (mw->area_0x16 != get_now_areano()) {
            return;
        }
        if (pl_eft_draw_ck(mw) == 0) {
            return;
        }
    }
    if (mode == 1) {
        chara_icon_pos_get(mw, 2, &wpos);
        camera_world_to_screen((nw4r::g3d::Camera*)&camera, (u8*)&scr, &wpos);
        if (scr.z < 0.0f || scr.z > 1.0f) {
            return;
        }
        icon.x = scr.x * hud_screen_width_get() - 12.0f;
        icon.y = 1.0714285f * scr.y + pos->y - 24.0f;
        size = 24;
    } else {
        icon.x = pos->x;
        icon.y = pos->y;
        size = 20;
    }
    tex = (mw->field_0xB01 & 1) ? 29 : 30;
    switch (mw->field_0xB02) {
    case 1:
        color = 0x10F010FF;
        break;
    case 2:
        color = 0xB4B4B4FF;
        break;
    case 3:
        color = 0x4040F0FF;
        break;
    default:
        color = 0xF01010FF;
        break;
    }
    draw_tex_icon_square(icon.x, icon.y, size, color, tex);
}

/* 0x80324544 (0x390): draws NPC `npc`'s talk mark over its head, scaled with the camera distance and coloured by its
 * mark state. */
extern "C" void lb_npc_move_work_draw(_LB_NPC* npc) {
    u32 camera;
    s32 camera_id;
    nw4r::math::VEC3 scr;
    nw4r::math::VEC3 eye;
    nw4r::math::VEC3 dir;
    nw4r::math::VEC3 joint;
    nw4r::math::VEC3 wpos;
    nw4r::math::VEC3 icon;
    s16 mark;
    s32 size;
    u32 node;
    f32 raw;
    f32 scale;

    camera_id = scn_root_get_current_camera(pRoot);
    word_copy_return_dst(&camera, &camera_id);
    VEC3_ctor(&scr);
    VEC3_ctor(&eye);
    VEC3_ctor(&dir);
    VEC3_ctor(&joint);
    assignVec3((Vec*)&wpos, (Vec*)&npc->pos_0x10);
    mark = -1;
    if (lb_npc_id_ck(npc, 0) == 1) {
        if (lb_npc_talk_mark_ck(npc) == 0) {
            mark = lb_npc_mark_state_get();
            if (mark < 0) {
                return;
            }
        }
    } else if (lb_npc_talk_mark_ck(npc) == 0) {
        return;
    }
    if (event_demo_ck() == 1) {
        return;
    }
    if (npc->field_0x256 == 1) {
        return;
    }
    if (npc->field_0x001 == 0) {
        return;
    }
    node = mhchar_node_id_get((MHchar*)npc->body_0x058, "HD-00");
    if (node != 0) {
        ((MHchar*)npc->body_0x058)->get_joint_wpos(node, &joint);
        wpos.y = joint.y;
    } else {
        wpos.y += 200.0f;
    }
    {
        const nw4r::math::VEC3& eye_now = get_camera_pos();
        nw4r::math::VEC3 delta;

        copyVec3(&eye, &eye_now);
        subVec3(&delta, &eye, &wpos);
        copyVec3(&dir, &delta);
    }
    scale = 1600.0f / vec3_len((f32*)&dir);
    raw = scale;
    if (scale < 1.0f) {
        scale = 1.0f;
    }
    if (scale > 2.1f) {
        scale = 2.1f;
    }
    size = 24.0f * scale;
    copyVec3(&wpos, &npc->pos_0x10);
    wpos.x += 6.0f;
    node = mhchar_node_id_get((MHchar*)npc->body_0x058, "HD-00");
    if (node != 0) {
        f32 height;

        ((MHchar*)npc->body_0x058)->get_joint_wpos(node, &joint);
        height = joint.y - npc->pos_0x10.y;
        if (height < 0.0f) {
            height = 0.0f;
        }
        wpos.y += 6.0f + (24.0f + 50.0f * (1.0f + height / 50.0f) + 24.0f * (0.5f * (scale - raw)));
    } else {
        wpos.y += 200.0f;
    }
    camera_world_to_screen((nw4r::g3d::Camera*)&camera, (u8*)&scr, &wpos);
    if (scr.z < 0.0f || scr.z > 1.0f) {
        return;
    }
    icon.x = scr.x * hud_screen_width_get();
    icon.y = 1.0714285f * scr.y;
    switch (mark) {
    case 0:
        draw_tex_icon_square(icon.x, icon.y, size, 0xE1505CFF, 49);
        break;
    case 1:
        draw_tex_icon_square(icon.x, icon.y, size, 0x69BF83FF, 49);
        break;
    default:
        draw_tex_icon_square(icon.x, icon.y, size, 0xFFFF66FF, 49);
        break;
    }
}

/* The hunter key table's sprites: the frame, its row positions and the row frame (.sdata). */
u16 hud_key_frame_spr[18] = {0x189F, 0x1898, 0x1899, 0x189A, 0x189B, 0x189C, 0x189D, 0x189E, 0x1895,
                             0x1896, 0x1897, 0x18A1, 0x18A2, 0x18A3, 0x18A0, 0x18A4, 0x18A5, 0xFFFF};
u16 hud_key_row_lsp_ids[10] = {0x18A6, 0x18A7, 0x18A8, 0x18A9, 0x18AA, 0x18AB, 0x18AC, 0x18AD, 0x18AE, 0x18AF};
u16 hud_key_row_spr[4] = {0x18B2, 0x18B3, 0x18B4, 0xFFFF};

/* 0x803248D4 (0x3F0): draws the hunter key table: a row per hunter card with its name (or id), its colour mark, its
 * player's marker, its level icon and its transfer icon. */
extern "C" void hud_key_table_draw(void) {
    _mh_ivec2_ pos;
    _mh_ivec2_ row_pos;
    _SPR_DATA_ spr;
    u16* ids;
    s32 i;
    LbQuestPage* card;
    s16 friend_row;
    s16 n;
    _PLW* mw;
    u8 level;
    s16 k;

    n = 0;
    for (k = 0; k < 10; k++) {
        if (lobby_hunter_cards[k].active_0x000 != 0) {
            n++;
        }
    }
    if (n == 0) {
        return;
    }
    sprite_frame_apply(&spr, 6292, n, &pos);
    draw_sprite_anim_ary(hud_key_frame_spr, n, &pos);
    card = lobby_hunter_cards;
    for (i = 0, ids = hud_key_row_lsp_ids; i < 10; i++, card++) {
        if (card->active_0x000 == 0) {
            continue;
        }
        mw = card->player_0x124;
        get_lsp_data(*ids, &row_pos);
        row_pos.x += pos.x;
        row_pos.y += pos.y;
        draw_sprite_ary(hud_key_row_spr, &row_pos);
        spr_data_copy(&spr, get_lsp_data(6325, NULL));
        if ((u32)isSessionMemberId((NetId*)card->id_0x03) == 1) {
            spr.color = 0xFFA461FF;
        }
        friend_row = 0;
        if ((u32)NetCtrlWk::hasFriendDetail((NetId*)card->id_0x03) == 1) {
            spr.color = 0x808080FF;
            friend_row = 1;
        }
        if (get_option_cfg(12) == 0) {
            draw_font(spr, (s8*)card->name_0x0D, 0, &row_pos);
        } else {
            draw_font(spr, (s8*)card->id_0x03, 0, &row_pos);
        }
        spr_data_copy(&spr, get_lsp_data(6326, NULL));
        draw_sprite_uv_color_idx(6326, mw->field_0x002, spr.color, &row_pos);
        if (mw->field_0xB01 != 0 && friend_row == 0) {
            get_lsp_data(*ids, &row_pos);
            row_pos.x += pos.x;
            row_pos.y += pos.y;
            {
                _mh_ivec2_ marker_pos = row_pos;

                move_work_draw(mw, &marker_pos, 0);
            }
        }
        level = ((NetUserProfile*)card->card_0x024)->level_0xF3;
        if (level != 0) {
            spr_data_copy(&spr, get_lsp_data(*ids, &row_pos));
            row_pos.x += (s16)(pos.x + spr.width - 22);
            row_pos.y += pos.y;
            spr_data_copy(&spr, get_lsp_data(317, NULL));
            draw_tex_icon_square(row_pos.x, row_pos.y, 20, spr.color, chara_icon_tex_tbl[level]);
        }
        spr_data_copy(&spr, get_lsp_data(*ids, &row_pos));
        row_pos.x += (s16)(pos.x + spr.width - 44);
        row_pos.y += pos.y;
        switch ((u8)cockpit_state.transfer_mode_0x07C[i]) {
        case 3:
            if ((lobby_w.counter_0x030 & 0x10) == 0) {
                draw_tex_icon_square(row_pos.x, row_pos.y, 20, 0xFFFFFFFF, 34);
            }
            break;
        case 1:
            draw_tex_icon_square(row_pos.x, row_pos.y, 20, 0xFFFFFFFF, 34);
            break;
        case 2:
            draw_tex_icon_square(row_pos.x, row_pos.y, 20, 0xFFFFFFFF, 35);
            break;
        }
        ids++;
    }
}

/* The item list's slide animations: the left and right pages (.data) and their arrows and bars (.sdata). */
u16 lsp_slide_left_spr[16] = {0x277D, 0x277E, 0x2781, 0x2782, 0x2783, 0x2784, 0x2785, 0x2786,
                              0x2787, 0x2788, 0x2789, 0x278A, 0x278B, 0x278C, 0x278D, 0xFFFF};
u16 lsp_slide_right_spr[16] = {0x2792, 0x2793, 0x2796, 0x2797, 0x2798, 0x2799, 0x279A, 0x279B,
                               0x279C, 0x279D, 0x279E, 0x279F, 0x27A0, 0x27A1, 0x27A2, 0xFFFF};
u16 lsp_list_a_spr[16] = {0x27A7, 0x27A8, 0x27AB, 0x27AC, 0x27AD, 0x27AE, 0x27AF, 0x27B0,
                          0x27B1, 0x27B2, 0x27B3, 0x27B4, 0x27B5, 0x27B6, 0x27B7, 0xFFFF};
u16 lsp_list_a_over_spr[16] = {0x27BC, 0x27BD, 0x27C0, 0x27C1, 0x27C2, 0x27C3, 0x27C4, 0x27C5,
                               0x27C6, 0x27C7, 0x27C8, 0x27C9, 0x27CA, 0x27CB, 0x27CC, 0xFFFF};
u16 lsp_list_b_over_spr[16] = {0x27D1, 0x27D2, 0x27D5, 0x27D6, 0x27D7, 0x27D8, 0x27D9, 0x27DA,
                               0x27DB, 0x27DC, 0x27DD, 0x27DE, 0x27DF, 0x27E0, 0x27E1, 0xFFFF};
u16 lsp_list_b_spr[16] = {0x27E6, 0x27E7, 0x27EA, 0x27EB, 0x27EC, 0x27ED, 0x27EE, 0x27EF,
                          0x27F0, 0x27F1, 0x27F2, 0x27F3, 0x27F4, 0x27F5, 0x27F6, 0xFFFF};
u16 lsp_slide_left_arrow_spr[3] = {0x277F, 0x2780, 0xFFFF};
u16 lsp_slide_right_arrow_spr[3] = {0x2794, 0x2795, 0xFFFF};
u16 lsp_list_a_arrow_spr[3] = {0x27A9, 0x27AA, 0xFFFF};
u16 lsp_list_a_tab_spr[3] = {0x27BE, 0x27BF, 0xFFFF};
u16 lsp_list_b_tab_spr[3] = {0x27D3, 0x27D4, 0xFFFF};
u16 lsp_list_b_arrow_spr[3] = {0x27E8, 0x27E9, 0xFFFF};
u16 lsp_slide_left_bar_ids[3] = {0x278E, 0x278F, 0x2790};
u16 lsp_slide_right_bar_ids[3] = {0x27A3, 0x27A4, 0x27A5};

/* 0x80324CC4 (0x2B8): draws the item list's page slide: the slide animation while the list moves, else the current
 * page with its tab and arrows, and the four depth bars while it slides. */
extern "C" void draw_lsp_anim_ary(void) {
    _mh_ivec2_ pos;
    _SPR_DATA_ spr;
    u16* arrows;
    u16* bars;
    u16* tab;
    u16* page;
    u16* main_spr;
    s32 sel;
    u16* bar_id;
    s16 bar;
    s16 frame;
    u32 rot;
    s32 i;
    f32 depth;

    tab = NULL;
    page = NULL;
    bars = NULL;
    sel = getItemListSelection();
    get_lsp_data(10108, &pos);
    if (lobby_w.slide_0x034 >= 60 && sel != 1) {
        arrows = lsp_slide_left_arrow_spr;
        main_spr = lsp_slide_left_spr;
        bars = lsp_slide_left_bar_ids;
        frame = (u32)lobby_w.counter_0x030 % 60;
    } else if (lobby_w.slide_0x034 <= -60 && sel != 0) {
        arrows = lsp_slide_right_arrow_spr;
        main_spr = lsp_slide_right_spr;
        bars = lsp_slide_right_bar_ids;
        frame = (u32)lobby_w.counter_0x030 % 60;
    } else if (getItemListSelection() == 1) {
        arrows = lsp_list_a_arrow_spr;
        main_spr = lsp_list_a_spr;
        tab = lsp_list_a_tab_spr;
        page = lsp_list_a_over_spr;
        if (lobby_w.slide_0x034 > 0) {
            lobby_w.slide_0x034 = 0;
        }
        frame = -lobby_w.slide_0x034;
    } else {
        arrows = lsp_list_b_arrow_spr;
        main_spr = lsp_list_b_spr;
        tab = lsp_list_b_tab_spr;
        page = lsp_list_b_over_spr;
        if (lobby_w.slide_0x034 < 0) {
            lobby_w.slide_0x034 = 0;
        }
        frame = lobby_w.slide_0x034;
    }
    draw_sprite_anim_ary(arrows, frame, &pos);
    if (tab != NULL) {
        draw_sprite_anim_ary(tab, frame, &pos);
    }
    if (bars != NULL) {
        depth = lb_item_list_depth_get();
        depth *= 2.0f;
        if (getItemListSelection() == 1) {
            depth -= 1.0f;
        }
        if (depth < 0.0f) {
            depth = 0.0f;
        } else if (depth > 1.0f) {
            depth = 1.0f;
        }
        bar = 12.0f * depth;
        i = 0;
        bar_id = &bars[bar];
        rot = 0;
        for (; i < 4; i++) {
            if (bar < 3) {
                spr_data_copy(&spr, get_lsp_data(bar >= 0 ? *bar_id : bars[0], NULL));
                spr.tex_id = rot;
                draw_sprite(spr, &pos);
            }
            bar_id -= 3;
            bar -= 3;
            rot += 0x4000;
        }
    }
    draw_sprite_anim_ary(main_spr, frame, &pos);
    if (page != NULL) {
        draw_sprite_anim_ary(page, frame, &pos);
    }
}
