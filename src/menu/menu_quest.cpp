/*
 * menu/menu_quest.cpp - the lobby quest board screens: the quest list builders (`quest_list_tbl`, the per-group seed
 *   draws), the quest accept/join screens on `lobby_w.menu_0xAC` (`MenuQuestWork`), their detail and keyboard pages
 *   and the quest id tests.  C++ callees (the `draw_*` layout calls, `ck_WideMode`, `sysSE_req`), `extern "C"` bodies.
 * RANGE. .text 0x8031F77C-0x8032422C (45 functions); extab 0x800160BC-0x800161EC, extabindex 0x80034F80-0x80035148,
 *   .data 0x805DD598-0x805DD8D0, .bss 0x806BE120-0x806BE2C8, .sdata 0x80792C50-0x80792C78, .sdata2
 *   0x8079AEF8-0x8079AF10.  One TU: its pool (the two int-to-float doubles, 60.0f) is loaded only by the quest detail
 *   draws, the extabindex records 0x80034F80 and 0x80035148 name 0x8031F77C and 0x8032422C, and the `.data`/`.sdata`
 *   runs end at the next unit's first table (`hud_key_frame_spr`) and string ("HD-00", `lb_npc_move_work_draw`).
 *   The edge with `hud/lobby_hud.cpp` is 0x8032422C or 0x80324304: `lobby_hud_reset`/`lobby_hud_frame_update` load
 *   no data and have no unwind-visible difference, so they sit with the HUD calls they make.
 * FLAGS. `cflags_menu` (configure.py); file-scope `#pragma fp_contract off` (retail keeps every multiply and add
 *   apart), `#pragma pool_data off` (each table addressed by its own lis/addi) and `#pragma optimization_level 4`
 *   (retail's switches test case ranges first with subi + cmplwi, idea 108).
 * NAMES. Module `menu` from the menu/HUD 2D callees; no `__FILE__` string covers the range and the dump answers `zz_`;
 *   the file name is a GUESS (retail menu files are `menu_<topic>.cpp`; the range is the quest board).
 *   GUESS: `quest_party_state_reset`, `quest_detail_label_draw`, `quest_list_unlocked_count`,
 *   `quest_group_bonus_pick`, `quest_seed_walk`, `quest_group_draw`, `quest_event_slot_id_get`,
 *   `quest_list_tbl_build`, `quest_list_tbl`, `lb_quest_join_screen_open`, `lb_quest_accept_screen_open`,
 *   `lb_quest_screen_close`, `quest_list_select_step`, `lb_quest_join_screen_draw`, the quest screen pages and draws,
 *   the table names; foreign (registry): `pl_se_pos_req`, `lb_talk_page_value_set`, `lobby_kujira_level_get`,
 *   `draw_tex_icon`.
 * RESIDUALS. Every row is written.  Register allocation residuals: `quest_group_bonus_pick` (two volatile backend
 *   temporaries of the shift loop swap r10/r11), `quest_comment_wrap` (a callee-saved permutation no declaration
 *   order reaches); `quest_group_id_ck` (retail tests 1000 and 9000 apart and branches to the final `return 0`; the
 *   nested, `&&`, `do { break; } while (0)` and empty-branch spellings fold into one `addis`/`subi`/`cmplwi` range
 *   test, the `else if (id < 1000) return 0; else if (id < 9000)` chain keeps the separate `li`/`b`, and a
 *   per-function `optimization_level 3` changes nothing; the file needs level 4 (115 B lower without it).
 *   flipcheck: `.text` 8 bytes over the claim (`quest_group_id_ck`) and the three rows above; data byte-identical.
 */

#include "menu/menu_quest.h"
#include "types.h"
#include "lobby/lobby_w.h"
#include "mh3_pad/lb_param_w.h"
#include "fn_80047398/lobby_world_block.h"
#include "lobby/lb_npc.h" /* lb_party_state_reset (rule 2: its owner) */
#include "enemy/em_pop.h" /* quest_flag_2000000_ck (rule 2: its owner) */
#include "fn_8004CAD8.h" /* calcVecAngXY / MTX34_ctor / vec3_scale_in_place / userdata_flag_ck (rule 2: its owner) */
#include "ef/system_core.h" /* rand_lcg_step / work_mem_alloc */
#include "ef/work_mem_free.h"
#include "quest/quest_slot_count_get.h"
#include "quest/quest_slot_items_get.h"
#include "Network/network_pat_control.h" /* net_ctrl_wk */
#include "Network/net_session_close.h" /* requestSessionJoin / getSessionJoinResult / the profile checks */
#include "camera/camera.h" /* camera_talk_lock_set */
#include "lobby/lb_menu_step.h"
#include "lobby/lb_talk_page_open.h"
#include "lobby/lb_talk_page_value_set.h"
#include "lobby/lb_list_init.h" /* lb_list_init / lb_choice_step */
#include "lobby/lb_unlock_cond_ck.h"
#include "userdata_hunter_rank_info_get.h"
#include "hud/layout.h" /* the draw_* sprite and font calls */
#include "font/flfnt.h" /* font_set_size */
#include "menu/menu_message.h" /* menu_cursor_step / menu_page_count / toggle_word_step */
#include "lobby/lb_cmd_pressed_ck.h"
#include "lobby/lb_quest_board_data.h"
#include "lobby/lb_unlock_cond_last_ck.h"
#include "lobby/LbCheckKujiraEvent.h"
#include "userdata_rank_band_get.h"
#include "enemy/quest_board_flags_send.h"
#include "menu/quest_group_clear_count.h"
#include "fn_80040598.h" /* kbd_open / kbd_move / set_kbd_param */
#include "hud/cockpit.h" /* chk_pointer / PutPageArrow / ainpc_page_hold_set / setCockpitTransferMode */
#include "lobby/LbStr.h"
#include "mh3_pad/Screen_w.h"
#include "menu/get_pop_dat_ptr.h" /* get_option_cfg */
#include "sound/set_zmode.h"
#include "sound/fn_800E3CBC.h" /* set_blendmode */
#include "lobby/lb_cursor_prompt_draw.h"
#include "lobby/lb_eqshop_menu_frame.h"
#include "font/flKnjMsgNum.h"
#include "quest/quest_entry.h" /* quest_record_find / quest_result_field_text_get */
/* signatures the calls below use, when they differ from the owner header's: a cast call is the same direct call. */
#define rand_lcg_step_c1 ((s32 (*)(u16))rand_lcg_step)
#define toggle_word_step_c1 ((s32 (*)(u32*, u16, s32, s32, u16))toggle_word_step)
#define quest_field32C_ck_c1 ((u8 (*)(QuestRecord*))quest_field32C_ck)
#define get_wide_offset_c1 ((s32 (*)(u8))get_wide_offset)

/* retail addresses each table of this file with its own lis/addi pair, and keeps every float multiply and add
 * apart (fmuls + fadds, no fmadds) */
#pragma pool_data off
#pragma fp_contract off
#pragma optimization_level 4

/* Callees other units own; their owners' headers do not declare them yet. */
extern "C" {

void* memset(void* dst, int value, u32 size);
void  sysSE_stop(s32 value);
u32   game_ready_ck(void);

/* This unit's own forward declarations (defined below). */
void  quest_list_draw(MenuQuestWork* self, u8 mode);
void  quest_detail_draw(MenuQuestWork* self, u16 a, s16 b);
void  quest_list_page_set(MenuQuestWork* self, s16 page, u8 mode);
u32   quest_group_id_ck(u16 id, u8 group);
s32   quest_group_open_count(void);
u16   quest_urgent_id_get(void);
u32   quest_dual_target_ck(QuestRecord* rec);
void  lb_quest_accept_screen_draw(void);
void quest_join_settings_draw(MenuQuestWork* self, s32 value);
s32 quest_list_select_step(MenuQuestWork* self, u8 mode);
void quest_list_open(MenuQuestWork* self, u8 value);
void quest_join_params_reset(MenuQuestWork* self, u8 mode);
void lb_quest_screen_close(void);
void lb_quest_join_screen_draw(void);
s32 quest_fee_get(QuestRecord* rec);

} /* extern "C" */

/* The mangled screen callees (real C++ signatures - rule 9). */
s32 ck_WideMode(void);
s32 sysSE_req(s32 value);


/* The quest lists per board group (.bss 0x806BE120): eight rows of sixteen quest ids and a terminator; row 7 is the
 * screen's label row. */
u16 quest_list_tbl[8][17];

/* 0x8031F77C - count the non-zero `u16` entries of a string table whose value `userdata_flag_ck`
 * accepts. */
extern "C" s16 quest_list_unlocked_count(u16* table) {
    u16* p = table;
    s16 count = 0;

    while (*p != 0) {
        if ((u32)userdata_flag_ck(*p) == 1) {
            count++;
        }
        p++;
    }
    return count;
}

/* 0x8031F7DC (0x3A8): picks one quest of `ids` flagged 0x40000000 into `out[0]` and, on an odd roll, a second
 * different one into `out[1]`; the rolls come from the lobby's quest seed chain. */
extern "C" void quest_group_bonus_pick(s16 group, u16* ids, u16* out) {
    u16 list[10];
    s16 n;
    s16 i;
    s32 seed;
    s32 roll_a;
    s32 roll_b;
    s32 roll_c;
    s32 pick;

    for (i = 0; i < 10; i++) {
        list[i] = 0;
    }
    out[0] = 0;
    out[1] = 0;
    n = 0;
    while (*ids != 0) {
        if (quest_flag_40000000_ck(quest_record_find(*ids)) == 1) {
            list[n] = *ids;
            n++;
            if (n >= 10) {
                break;
            }
        }
        ids++;
    }
    if (n <= 0) {
        return;
    }
    for (i = 0; i < n; i++) {
        userdata_flag_ck(list[i]);
    }
    seed = 9;
    switch (i) {
    case 2:
        seed = 10;
        break;
    case 3:
        seed = 11;
        break;
    case 4:
        seed = 12;
        break;
    }
    roll_a = rand_lcg_step_c1((u16)lobby_w.field_0x036[seed]);
    roll_b = rand_lcg_step_c1((u16)roll_a);
    roll_c = rand_lcg_step_c1((u16)roll_b);
    pick = (u16)roll_b % n;
    out[0] = list[pick];
    if ((u16)roll_a & 1) {
        for (i = pick; i < n; i++) {
            list[i] = list[i + 1];
        }
        list[i] = 0;
        out[1] = list[(u16)roll_c % (s16)(n - 1)];
    }
}

/* 0x8031FB84 - walk `n` links of the `lobby_w + 0x48` chain, returning the last value. */
extern "C" s32 quest_seed_walk(s16 n) {
    s32 value = (u16)lobby_w.field_0x036[9];
    s16 i = 0;

    while (i < n) {
        value = rand_lcg_step_c1((u16)value);
        i++;
    }
    return value;
}

/* 0x8031FBE8 (0x7B8): draws `count` quests out of `ids` with the group's seed chain and, when every still-locked
 * quest was drawn, swaps one unlocked pick for a locked one so the board always offers something new. */
extern "C" void quest_group_draw(s16 group, u16* ids, s16 count) {
    u16 picks[10];
    u16 unlocked[30];
    u16 locked[30];
    s16 i;
    u16 n_locked;
    u16* p;
    s32 seed;
    s16 k;
    s16 j;
    s16 m;
    u16 total;
    u16 n_unlocked;
    u16 index;
    s16 dup;
    u16 swap_in;
    u16 swap_out;

    for (k = 0; k < 30; k++) {
        unlocked[k] = 0;
        locked[k] = 0;
    }
    for (k = 0; k < 10; k++) {
        picks[k] = 0;
    }
    total = 0;
    n_unlocked = 0;
    n_locked = 0;
    for (p = ids; *p != 0; p++) {
        if (userdata_flag_ck(*p) == 0) {
            locked[n_locked] = *p;
            n_locked++;
        } else {
            unlocked[n_unlocked] = *p;
            n_unlocked++;
        }
        total++;
    }
    seed = quest_seed_walk(group * 3);
    for (k = 0; k < count; k++) {
        if (total == 0) {
            return;
        }
        seed = rand_lcg_step_c1((u16)seed);
        index = (u16)seed % total;
        picks[k] = ids[index];
        for (j = index; j < total; j++) {
            *(ids + j) = *(ids + j + 1);
        }
        total--;
    }
    p = ids;
    dup = 0;
    for (; *p != 0; p++) {
        for (k = 0; k < n_locked; k++) {
            if (*p == locked[k]) {
                dup++;
            }
        }
    }
    if (n_locked != 0 && n_locked <= count && dup == 0) {
        for (k = 0; k < count; k++) {
            for (j = 0; j < n_unlocked; j++) {
                if (unlocked[j] == picks[k]) {
                    for (m = j; m < n_unlocked; m++) {
                        unlocked[m] = unlocked[m + 1];
                    }
                    unlocked[m] = 0;
                    n_unlocked--;
                    break;
                }
            }
        }
        for (p = ids; *p != 0; p++) {
            for (k = 0; k < n_locked; k++) {
                if (*p == locked[k]) {
                    locked[k] = 0;
                }
            }
        }
        index = 0;
        for (i = 0; i < 29; i++) {
            if (locked[i] == 0) {
                s16 n;

                for (n = i; n < 29; n++) {
                    locked[n] = locked[n + 1];
                }
            } else {
                index++;
            }
        }
        seed = rand_lcg_step_c1((u16)seed);
        swap_in = locked[(u16)seed % index];
        swap_out = unlocked[(u16)seed % n_unlocked];
        for (m = 0; m < total; m++) {
            if (ids[m] == swap_out) {
                ids[m] = swap_in;
                return;
            }
        }
    }
}

/* 0x803203A0 (0xAC): the quest id of downloaded event quest `slot`, or 0 when the slot is empty or its 0x10 flag
 * does not suit `mode` (0: unflagged only, 1: flagged only). */
extern "C" u16 quest_event_slot_id_get(u8 slot, u8 mode) {
    QuestRecord* rec;

    if (net_ctrl_wk != NULL) {
        if (net_ctrl_wk->file_sums_0xC3C4[slot] == 0xFFFFFFFF) {
            return 0;
        }
        rec = (QuestRecord*)net_ctrl_wk->file_buffers_0xC39C[slot];
        if (quest_flag_10_ck(rec) == 1) {
            if (mode == 0) {
                return 0;
            }
        } else if (mode == 1) {
            return 0;
        }
        return rec->field_0x02C;
    }
    return 0;
}

/* 0x8032044C (0x3FC): rebuilds `quest_list_tbl`: for each board group, the open quests first, then the cleared
 * ones the group's draw keeps, plus up to two bonus picks. */
extern "C" void quest_list_tbl_build(void) {
    u16 bonus[2];
    u16 list[30];
    s32 i;
    s16 g;
    u16 n;
    u16* buf;
    u16 first;
    s32 count;
    u16* top;
    s16 k;

    for (g = 0; g < 8; g++) {
        for (k = 0; k < 17; k++) {
            quest_list_tbl[g][k] = 0;
        }
    }
    for (g = 0; g <= 5; g++) {
        n = 0;
        count = quest_slot_count_get((u8)(g + 1));
        buf = (u16*)work_mem_alloc(count * 2 + 1);
        memset(buf, 0, (count + 1) * 2);
        quest_slot_items_get((u8)(g + 1), buf);
        top = buf;
        for (i = 0; i < 30; i++) {
            list[i] = 0;
        }
        bonus[0] = 0;
        bonus[1] = 0;
        switch (g) {
        case 2:
        case 3:
        case 4:
            if (quest_list_unlocked_count(buf) > count / 2 - 2 && (u16)quest_seed_walk(g * 4) % 100 <= 30) {
                quest_group_bonus_pick(g, buf, bonus);
            }
            break;
        }
        for (; *buf != 0; buf++) {
            if (*buf < 50000 && quest_record_find(*buf)->field_0x348 == 0) {
                list[n] = *buf;
                n++;
            }
        }
        first = n;
        buf = top;
        for (; *buf != 0; buf++) {
            if (*buf < 50000 && (u32)(*buf - 16000) > 1) {
                QuestRecord* rec = quest_record_find(*buf);

                if (rec->field_0x348 > 0 && quest_flag_40000000_ck(rec) == 0) {
                    if ((u16)(*buf / 100) == 140 || (u16)(*buf / 100) == 190) {
                        if ((u32)userdata_flag_ck(*buf) == 1) {
                            list[n] = *buf;
                            n++;
                        }
                    } else {
                        list[n] = *buf;
                        n++;
                    }
                }
            }
        }
        if (g != 5) {
            quest_group_draw(g, &list[first], 3);
        }
        for (i = 0; i < 2; i++) {
            if (bonus[i] != 0) {
                s16 j;

                for (j = 0; j < 30; j++) {
                    if (list[j] == 0) {
                        list[j] = bonus[i];
                        break;
                    }
                }
            }
        }
        for (k = 0; k < n; k++) {
            if (k < 16) {
                quest_list_tbl[g][k] = list[k];
            }
        }
        work_mem_free(top);
    }
}

/* 0x80320848 (0x80): opens the quest join screen (lobby screen 0x1D) for the player's board data and the NPC. */
extern "C" void lb_quest_join_screen_open(LbQuestBoardData* board, _LB_NPC* npc) {
    MenuQuestWork* w = lobby_w.quest_0x0AC;
    s32 next_points;
    s32 points;

    memset(w, 0, 0x2000);
    w->board_0x16C = board;
    w->npc_0x170 = npc;
    lobby_w.state_0x000 = 0x1D;
    lobby_w.active_0x008 = 1;
    lb_talk_page_open(0);
    userdata_hunter_rank_info_get(&w->rank_0x19A, &points, &next_points);
}

/* 0x803208C8 (0x38C): steps the quest join screen: the talk menu, the yes/no list, the name/comment keyboard,
 * the session join request and the closing talk pages. */
extern "C" void lb_quest_join_screen_step(void) {
    MenuQuestWork* w = lobby_w.quest_0x0AC;

    w->no_draw_0x198 = 0;
    w->field_0x00A = 0;
    switch (w->state_0x000) {
    case 0:
        if ((u32)(lb_menu_step(1, -1, 0, 1) - 1) <= 1) {
            camera_talk_lock_set(1);
            if (lb_param_w.field_0x00 == 0) {
                w->state_0x000 = 2;
                w->state_0x001 = 0;
                w->field_0x020 = 7;
                quest_list_open(w, 1);
                lb_list_init(&w->list_0x178, 1, 0, 0x1A4, -1, 0, 0, 3, 0);
                sysSE_stop(41);
            } else {
                w->state_0x000 = 4;
                w->accept_anim_0x006 = 8;
                lb_talk_page_open(9);
                lb_talk_page_value_set(1);
                sysSE_req(27);
            }
        } else {
            w->no_draw_0x198 = 1;
        }
        break;
    case 1:
        switch (lb_choice_step(&w->list_0x178)) {
        case 1:
            if (w->list_0x178.cursor_0x00 == 0) {
                w->state_0x000 = 2;
                w->state_0x001 = 0;
                quest_list_open(w, 1);
            } else {
                w->talking_0x009 = 1;
                w->state_0x000 = 6;
                lb_talk_page_open(1);
                camera_talk_lock_set(0);
            }
            break;
        case 2:
            w->talking_0x009 = 1;
            w->state_0x000 = 6;
            lb_talk_page_open(1);
            camera_talk_lock_set(0);
            break;
        }
        break;
    case 2:
        switch (quest_list_select_step(w, 1)) {
        case 1:
            w->state_0x000 = 3;
            quest_join_params_reset(w, 1);
            requestSessionJoin((NetJoinParams*)&w->field_0x040);
            break;
        case 2:
            w->talking_0x009 = 1;
            w->state_0x000 = 6;
            lb_talk_page_open(1);
            camera_talk_lock_set(0);
            break;
        }
        break;
    case 3:
        switch (getSessionJoinResult()) {
        case 1:
            w->talking_0x009 = 1;
            w->state_0x000 = 6;
            quest_board_accept(w->board_0x16C, w->field_0x036, 1);
            lb_talk_page_open(7);
            lb_npc_talk_mode_set(w->npc_0x170, 1);
            break;
        case -1:
            w->state_0x001 = 2;
            sysSE_req(1);
            break;
        }
        break;
    case 6:
        if ((u32)(lb_talk_page_mode_reset() - 1) <= 1) {
            lb_quest_screen_close();
            return;
        }
        break;
    case 4:
        if (isProfileUnselected() == 1) {
            w->talking_0x009 = 0;
            quest_party_state_reset((_PLW*)w->board_0x16C);
            lb_quest_screen_close();
            return;
        }
        if ((u32)(lb_menu_step(1, -1, 0, 1) - 1) <= 1) {
            if (lb_talk_page_value_get() == 0) {
                w->talking_0x009 = 0;
                w->state_0x000 = 5;
                runPendingAction6();
            } else {
                w->talking_0x009 = 1;
                w->state_0x000 = 6;
                lb_talk_page_open(7);
                sysSE_req(1);
                lb_npc_talk_mode_set(w->npc_0x170, 1);
            }
        }
        break;
    case 5:
        if (hasSelectedProfile() != 1) {
            w->talking_0x009 = 0;
            w->state_0x000 = 6;
            quest_party_state_reset((_PLW*)w->board_0x16C);
            lb_talk_page_open(10);
        }
        break;
    }
    if (w->no_draw_0x198 == 0) {
        subTransSet((u32)lb_quest_join_screen_draw, 0, 0);
    }
}

/* 0x80320C54 (0xCC): opens the quest accept screen (lobby screen 0x0C); online it rebuilds the quest lists first. */
extern "C" void lb_quest_accept_screen_open(LbQuestBoardData* board, _LB_NPC* npc) {
    MenuQuestWork* w = lobby_w.quest_0x0AC;
    s32 next_points;
    s32 points;

    memset(w, 0, 0x2000);
    w->board_0x16C = board;
    w->npc_0x170 = npc;
    w->unlocked_0x008 = 0;
    if (game_ready_ck() == 0 && lb_unlock_cond_ck(0x1A) != 0) {
        w->unlocked_0x008 = 1;
    }
    lobby_w.state_0x000 = 0xC;
    lobby_w.active_0x008 = 1;
    lb_talk_page_open(0);
    if (game_ready_ck() == 1) {
        quest_list_tbl_build();
    }
    userdata_hunter_rank_info_get(&w->rank_0x19A, &points, &next_points);
}

/* 0x80320D20 - the screen's teardown tail. */
extern "C" void lb_quest_screen_close(void) {
    lb_panel_close();
}

/* 0x80320D24 (0x2AC): fills the group menu: the board groups up to the last one with quests, the tab row, the
 * special row when it is open, and online the event row. */
extern "C" void quest_group_menu_init(MenuQuestWork* w) {
    s16 last = quest_group_open_count() - 1;
    s16 tab = 0;
    s16 tabs;

    w->group_cursor_0x00C = 0;
    memset(w->groups_0x010, 0xFF, 8);
    memset(w->tabs_0x018, 0xFF, 8);
    for (w->group_count_0x00E = 0; w->group_count_0x00E <= last; w->group_count_0x00E++) {
        w->groups_0x010[w->group_count_0x00E] = w->group_count_0x00E;
    }
    if (game_ready_ck() == 0) {
        tabs = 4;
    } else {
        tabs = 5;
    }
    for (tab = 0; tab <= tabs; tab++) {
        w->tabs_0x018[tab] = tab;
    }
    if (quest_urgent_id_get() != 0) {
        w->groups_0x010[w->group_count_0x00E] = 6;
        w->tabs_0x018[tab] = 6;
        w->group_count_0x00E++;
        tab++;
    } else if (w->unlocked_0x008 != 0) {
        w->groups_0x010[w->group_count_0x00E] = 6;
        w->tabs_0x018[tab] = 6;
        w->group_count_0x00E++;
        tab++;
    }
    w->open_frame_0x004 = 6;
    w->group_cursor_0x00C = w->group_count_0x00E - 1;
    if (game_ready_ck() == 1) {
        w->groups_0x010[w->group_count_0x00E] = 7;
        w->tabs_0x018[tab] = 7;
        w->group_count_0x00E++;
    }
}

/* 0x80320FD0 (0xE8): steps the group menu; returns 1 when a group is chosen, 2 when the menu is cancelled. */
extern "C" s32 quest_group_menu_step(MenuQuestWork* w) {
    s32 result = 0;

    if (w->open_frame_0x004 < 6) {
        w->open_frame_0x004++;
    }
    if (lb_cmd_pressed_ck(0x20) != 0) {
        result = 2;
        sysSE_req(1);
    } else if (lb_cmd_pressed_ck(0x10) != 0) {
        w->field_0x020 = w->groups_0x010[w->group_cursor_0x00C];
        result = 1;
        sysSE_req(0);
    } else if (lb_cmd_repeat_ck(3) != 0) {
        w->group_cursor_0x00C = menu_cursor_step(w->group_cursor_0x00C, w->group_count_0x00E, lb_cmd_repeat_get(), 1, 2);
        if (w->group_count_0x00E > 1) {
            w->open_frame_0x004 = 1;
        }
    }
    return result;
}

extern "C" u16 quest_label_row_add(u16 index, u16 value) {
    quest_list_tbl[7][index] = value;
    return index + 1;
}

/* 0x803210DC - clear the label row of `quest_list_tbl`. */
extern "C" void quest_label_row_clear(void) {
    s32 i;

    for (i = 0; i < 17; i++) {
        quest_list_tbl[7][i] = 0;
    }
}

/* 0x80321130 (0x314): fills page `page` of the quest list for the selected group: the event quests (`mode` 1, or
 * the online event row), the urgent quest (group 6), or the group's quests from `quest_list_tbl` online and the
 * board slot offline. */
extern "C" void quest_list_page_set(MenuQuestWork* w, s16 page, u8 mode) {
    s32 size;
    s32 first;
    u16 n;
    s32 last;
    s32 i;
    u8 group;
    s32 index;
    u16* top;
    u16* list;
    QuestRecord* rec;

    if (game_ready_ck() == 0 && w->unlocked_0x008 == 1 && w->field_0x020 == 6) {
        group = 6;
    } else if (mode == 1) {
        w->pages_0x03A = 1;
        w->ids_0x026[0] = 0;
        w->count_0x024 = 0;
        w->page_0x038 = 0;
        n = 0;
        quest_label_row_clear();
        for (i = 0; i < 10; i++) {
            u16 id = quest_event_slot_id_get(i, 1);

            if (id != 0) {
                n = quest_label_row_add(n, id);
            }
        }
        if (n == 0) {
            return;
        }
        group = 8;
    } else if (w->field_0x020 == 7) {
        w->pages_0x03A = 1;
        w->ids_0x026[0] = 0;
        w->count_0x024 = 0;
        w->page_0x038 = 0;
        n = 0;
        group = w->field_0x020 + 1;
        if (game_ready_ck() == 0) {
            return;
        }
        quest_label_row_clear();
        if (LbCheckKujiraEvent() == 1) {
            n = quest_label_row_add(0, 16000);
            if (userdata_rank_band_get() >= 2) {
                n = quest_label_row_add(n, 16001);
            }
        }
        for (i = 0; i < 10; i++) {
            u16 id = quest_event_slot_id_get(i, 0);

            if (id != 0) {
                n = quest_label_row_add(n, id);
            }
        }
        if (n == 0) {
            return;
        }
    } else if (w->field_0x020 == 6) {
        w->pages_0x03A = 1;
        w->ids_0x026[0] = quest_urgent_id_get();
        w->count_0x024 = 1;
        w->page_0x038 = 0;
        return;
    } else {
        group = w->field_0x020 + 1;
    }
    if (game_ready_ck() == 1) {
        list = quest_list_tbl[group - 1];
    } else {
        size = (quest_slot_count_get(group) + 1) * 2;
        list = (u16*)work_mem_alloc(size);
        top = list;
        memset(list, 0, size);
        quest_slot_items_get(group, list);
    }
    first = page * 8;
    last = first + 7;
    w->count_0x024 = 0;
    memset(w->ids_0x026, 0, 16);
    index = 0;
    for (; *list != 0; list++) {
        rec = quest_record_find(*list);
        if (quest_group_id_ck(*list, w->field_0x020) == 1 && (mode != 1 || quest_flag_10_ck(rec) == 1)) {
            if (index >= first && index <= last) {
                w->ids_0x026[w->count_0x024] = *list;
                w->count_0x024++;
            }
            index++;
        }
    }
    if (w->value_0x022 >= w->count_0x024) {
        w->value_0x022 = w->count_0x024 - 1;
    }
    w->page_0x038 = page;
    w->pages_0x03A = menu_page_count(index, 8);
    if (game_ready_ck() == 0) {
        work_mem_free(top);
    }
}

/* 0x80321444 - reset the row counter and hand `value` to the list advance. */
extern "C" void quest_list_open(MenuQuestWork* self, u8 value) {
    self->value_0x022 = 0;
    quest_list_page_set(self, 0, value);
}

/* 0x8032145C (0x3D4): steps the quest list: pick a quest and a detail tab, confirm it, check its condition and
 * fee and accept it (60000 and up: download it first); returns 1 once accepted, 2 when the list is left. */
extern "C" s32 quest_list_select_step(MenuQuestWork* w, u8 mode) {
    s32 result = 0;

    if (w->accept_anim_0x006 == 7) {
        w->accept_anim_0x006 = 8;
    }
    switch (w->state_0x001) {
    case 0:
        if (lb_cmd_pressed_ck(0x10) != 0) {
            if (w->count_0x024 != 0) {
                w->state_0x001 = 1;
                w->field_0x03C = 0;
                w->tab_count_0x03E = 3;
                w->field_0x036 = w->ids_0x026[w->value_0x022];
                w->rec_0x174 = quest_record_find(w->field_0x036);
                sysSE_req(27);
            } else {
                sysSE_req(2);
            }
        } else if (lb_cmd_pressed_ck(0x20) != 0) {
            result = 2;
            sysSE_req(1);
        } else if (lb_cmd_repeat_ck(0xC) != 0) {
            if (w->pages_0x03A > 1) {
                w->page_0x038 = menu_cursor_step_forward(w->page_0x038, w->pages_0x03A, lb_cmd_repeat_get(), 4, 8, 6,
                                                         &w->field_0x00A);
                quest_list_page_set(w, w->page_0x038, mode);
            }
        } else if (lb_cmd_repeat_ck(3) != 0) {
            w->value_0x022 = menu_cursor_step(w->value_0x022, w->count_0x024, lb_cmd_repeat_get(), 1, 2);
        }
        break;
    case 1:
        if (lb_cmd_pressed_ck(0x10) != 0) {
            w->state_0x001 = 2;
            w->field_0x168 = 0;
            sysSE_req(0);
        } else if (lb_cmd_pressed_ck(0x20) != 0) {
            w->state_0x001 = 0;
            sysSE_req(1);
        } else if (lb_cmd_repeat_ck(0xC) != 0) {
            w->field_0x03C = menu_cursor_step_forward(w->field_0x03C, w->tab_count_0x03E, lb_cmd_repeat_get(), 4, 8, 6,
                                                      &w->field_0x00A);
        }
        break;
    case 2:
        switch (toggle_word_step_c1(&w->field_0x168, lobby_cmd_trig_get(), 4, 8, 0xFFFF)) {
        case 1: {
            LbQuestBoardData* board = w->board_0x16C;

            if (quest_ex_condition_show_ck(quest_record_find(w->field_0x036), w->rank_0x19A, board->field_0x014,
                                           board) == 0) {
                w->state_0x001 = 4;
                lb_talk_page_open(6);
                sysSE_req(2);
            } else if (((MenuWorldBlockView*)lobby_world_block)->money_0x018 >= quest_fee_get(w->rec_0x174)) {
                if (w->field_0x036 >= 60000) {
                    w->state_0x001 = 5;
                } else {
                    w->state_0x001 = 3;
                    w->wait_0x007 = 3;
                    w->accept_anim_0x006 = 7;
                    sysSE_stop(0);
                    if (game_ready_ck() == 0) {
                        quest_board_accept(w->board_0x16C, w->field_0x036, 1);
                    }
                }
            } else {
                w->state_0x001 = 4;
                lb_talk_page_open(5);
                sysSE_req(2);
            }
            break;
        }
        case 2:
            w->state_0x001 = 1;
            break;
        }
        break;
    case 3:
        if (--w->wait_0x007 <= 0) {
            result = 1;
        }
        break;
    case 4:
        if ((u32)(lb_talk_page_mode_reset() - 1) <= 1) {
            w->state_0x001 = 1;
        }
        break;
    case 5: {
        s32 ret = stepStagingDownloadForVersion(w->field_0x036);

        if (ret != 0) {
            if (ret < 0) {
                w->state_0x001 = 1;
                sysSE_req(2);
            } else {
                w->state_0x001 = 3;
                w->wait_0x007 = 3;
                w->accept_anim_0x006 = 7;
                sysSE_stop(0);
            }
        }
        break;
    }
    }
    return result;
}

/* 0x80321830 (0x11C): accepts quest `quest_id` for the player: records it in the lobby, sets the board flags
 * (`mode` 1 pays the fee from the lobby money) and sends them with the objective kind. */
extern "C" void quest_board_accept(LbQuestBoardData* plw, u16 quest_id, u8 mode) {
    QuestRecord* rec = quest_record_find(quest_id);

    lb_param_w.field_0x00 = quest_id;
    lobby_w.page_0x052 = mode;
    lobby_w.field_0x050 = quest_id;
    if (mode == 1) {
        plw->flags_0xB01 |= 1;
        lb_param_w.field_0x04 = quest_fee_get(rec);
        score_add_clamped(-(s32)lb_param_w.field_0x04, &((MenuWorldBlockView*)lobby_world_block)->money_0x018);
        sysSE_req(9);
    } else {
        plw->flags_0xB01 |= 2;
        lb_param_w.field_0x04 = 0;
    }
    if (quest_flag_10_ck(rec) == 1) {
        plw->param_0xB02 = 3;
    } else {
        switch (quest_objective_get(rec)) {
        case 1:
            plw->param_0xB02 = 1;
            break;
        case 4:
            plw->param_0xB02 = 2;
            break;
        default:
            plw->param_0xB02 = 0;
            break;
        }
    }
    quest_board_flags_send(plw->flags_0xB01, plw->param_0xB02);
}

/* 0x8032194C - when the option block is live, play the confirm sound and run the two post steps. */
extern "C" void quest_party_state_reset(_PLW* plw) {
    if (lb_param_w.field_0x04 != 0) {
        sysSE_req(9);
    }
    lb_party_state_reset();
    sysSE_stop(1);
}

/* 0x80321990 - reset the edit screen for `mode`: seed the row count, mirror the label, clear both
 * editable buffers. */
extern "C" void quest_join_params_reset(MenuQuestWork* self, u8 mode) {
    if (mode == 0) {
        self->field_0x166 = 4;
    } else {
        self->field_0x166 = 2;
    }
    self->row_0x042 = (s8)self->field_0x166;
    self->field_0x040 = self->field_0x036;
    self->field_0x164 = 0;
    memset(self->name_0x043, 0, 5);
    memset(self->text_0x048, 0, 0x91);
}

/* The comment the keyboard edits (.bss 0x806BE230): the join comment is copied in, edited and cut to 72
 * characters on the way back. */
char quest_comment_kbd_buf[0x98];

/* 0x80321A14 (0x384): steps the join settings: the party size, the session name and the comment keyboards and the
 * join request; returns 1 once the join is accepted. */
extern "C" s32 lb_quest_join_settings_step(MenuQuestWork* w) {
    s32 result = 0;

    switch (w->state_0x001) {
    case 0:
        if (lb_cmd_pressed_ck(0x10) != 0) {
            switch (w->field_0x164) {
            case 1:
                w->state_0x001 = 1;
                w->submode_0x002 = 0;
                strcpy(w->edit_name_0x19C, w->name_0x043);
                sysSE_req(5);
                break;
            case 2:
                w->state_0x001 = 2;
                w->submode_0x002 = 0;
                strcpy(w->edit_text_0x1A6, w->text_0x048);
                sysSE_req(5);
                break;
            case 3:
                w->state_0x001 = 3;
                requestSessionJoin((NetJoinParams*)&w->field_0x040);
                sysSE_req(0);
                break;
            }
        } else if (lb_cmd_pressed_ck(0x20) == 0) {
            if (lb_cmd_repeat_ck(3) != 0) {
                w->field_0x164 = menu_cursor_step(w->field_0x164, 4, lb_cmd_repeat_get(), 1, 2);
            } else if (lb_cmd_repeat_ck(0xC) != 0 && w->field_0x164 == 0) {
                if (lb_cmd_repeat_ck(4) != 0) {
                    if (--w->row_0x042 <= 0) {
                        w->row_0x042 = 1;
                    } else {
                        sysSE_req(3);
                    }
                    w->field_0x00A = 4;
                }
                if (lb_cmd_repeat_ck(8) != 0) {
                    if (++w->row_0x042 > 4) {
                        w->row_0x042 = 4;
                    } else {
                        sysSE_req(3);
                    }
                    w->field_0x00A = 8;
                }
            }
        }
        break;
    case 1:
        switch (w->submode_0x002) {
        case 0:
            if (kbd_open(4) == 1) {
                menu_work.slot[0].field_0x32D = 1;
                set_kbd_param(w->edit_name_0x19C, 8);
                w->submode_0x002 = 1;
            }
            break;
        case 1:
            switch (kbd_move()) {
            case 1:
                strcpy(w->name_0x043, w->edit_name_0x19C);
            case -1:
                menu_work.slot[0].field_0x32D = 0;
                w->state_0x001 = 0;
                break;
            }
            break;
        }
        break;
    case 2:
        switch (w->submode_0x002) {
        case 0:
            if (kbd_open(0) == 1) {
                menu_work.slot[0].field_0x32D = 1;
                memset(quest_comment_kbd_buf, 0, 0x91);
                strcpy(quest_comment_kbd_buf, w->edit_text_0x1A6);
                set_kbd_param(quest_comment_kbd_buf, 0x90);
                w->submode_0x002 = 1;
            }
            break;
        case 1:
            switch (kbd_move()) {
            case 1: {
                char* cut = flKnjMsgNumPtr(quest_comment_kbd_buf, 72);

                if (cut != NULL) {
                    *cut = 0;
                }
                strcpy(w->text_0x048, quest_comment_kbd_buf);
            }
            case -1:
                menu_work.slot[0].field_0x32D = 0;
                w->state_0x001 = 0;
                break;
            }
            break;
        }
        break;
    case 3:
        switch (getSessionJoinResult()) {
        case 1:
            result = 1;
            break;
        case -1:
            w->state_0x001 = 0;
            sysSE_req(1);
            break;
        }
        break;
    }
    return result;
}

/* 0x80321D98 (0x3B4): steps the quest accept screen: the talk menu, the group menu, the quest list, the join
 * settings (online) and the closing talk pages. */
extern "C" void lb_quest_accept_screen_step(void) {
    MenuQuestWork* w = lobby_w.quest_0x0AC;

    w->no_draw_0x198 = 0;
    w->field_0x00A = 0;
    switch (w->state_0x000) {
    case 0:
        if ((u32)(lb_menu_step(1, -1, 0, 1) - 1) <= 1) {
            camera_talk_lock_set(1);
            if (lb_param_w.field_0x00 == 0) {
                w->state_0x000 = 1;
                w->state_0x001 = 0;
                quest_group_menu_init(w);
                sysSE_stop(41);
            } else {
                if (game_ready_ck() == 1) {
                    w->state_0x000 = 5;
                } else {
                    w->state_0x000 = 4;
                }
                w->accept_anim_0x006 = 8;
                lb_talk_page_open(9);
                lb_talk_page_value_set(1);
                sysSE_req(27);
            }
        } else {
            w->no_draw_0x198 = 1;
        }
        break;
    case 1:
        switch (quest_group_menu_step(w)) {
        case 1:
            w->state_0x000 = 2;
            w->state_0x001 = 0;
            quest_list_open(w, 0);
            break;
        case 2:
            w->talking_0x009 = 1;
            w->state_0x000 = 7;
            lb_talk_page_open(1);
            camera_talk_lock_set(0);
            break;
        }
        break;
    case 2:
        ainpc_page_hold_set();
        switch (quest_list_select_step(w, 0)) {
        case 1:
            if (game_ready_ck() == 0) {
                w->talking_0x009 = 1;
                w->state_0x000 = 7;
                lb_talk_page_open(7);
                lb_npc_talk_mode_set(w->npc_0x170, 1);
            } else {
                w->state_0x000 = 3;
                w->state_0x001 = 0;
                quest_join_params_reset(w, 0);
            }
            break;
        case 2:
            w->state_0x000 = 1;
            break;
        }
        break;
    case 3:
        ainpc_page_hold_set();
        switch (lb_quest_join_settings_step(w)) {
        case 1:
            w->talking_0x009 = 1;
            w->state_0x000 = 7;
            quest_board_accept(w->board_0x16C, w->field_0x036, 1);
            lb_talk_page_open(7);
            lb_npc_talk_mode_set(w->npc_0x170, 1);
            break;
        case 2:
            w->state_0x000 = 2;
            break;
        }
        break;
    case 7:
        if ((u32)(lb_talk_page_mode_reset() - 1) <= 1) {
            lb_quest_screen_close();
            return;
        }
        break;
    case 4:
        if ((u32)(lb_menu_step(1, -1, 0, 1) - 1) <= 1) {
            if (lb_talk_page_value_get() == 0) {
                w->talking_0x009 = 0;
                w->state_0x000 = 7;
                quest_party_state_reset((_PLW*)w->board_0x16C);
                lb_talk_page_open(10);
            } else {
                w->talking_0x009 = 0;
                w->state_0x000 = 7;
                lb_talk_page_open(7);
                sysSE_req(1);
                lb_npc_talk_mode_set(w->npc_0x170, 1);
            }
        }
        break;
    case 5:
        if (isProfileUnselected() == 1) {
            w->talking_0x009 = 0;
            quest_party_state_reset((_PLW*)w->board_0x16C);
            lb_quest_screen_close();
            return;
        }
        if ((u32)(lb_menu_step(1, -1, 0, 1) - 1) <= 1) {
            if (lb_talk_page_value_get() == 0) {
                w->talking_0x009 = 0;
                w->state_0x000 = 6;
                runPendingAction6();
            } else {
                w->talking_0x009 = 0;
                w->state_0x000 = 7;
                lb_talk_page_open(7);
                sysSE_req(1);
                lb_npc_talk_mode_set(w->npc_0x170, 1);
            }
        }
        break;
    case 6:
        if (hasSelectedProfile() != 1) {
            w->talking_0x009 = 0;
            w->state_0x000 = 7;
            quest_party_state_reset((_PLW*)w->board_0x16C);
            lb_talk_page_open(10);
        }
        break;
    }
    if (w->no_draw_0x198 == 0) {
        subTransSet((u32)lb_quest_accept_screen_draw, 0, 0);
    }
}

/* The group menu's sprites: one tab per board group (`quest_group_tab_spr_tbl`), the tabs' frame animations, and
 * the cursor. */
u16 quest_group_tab_spr_1[8] = {0x1518, 0x151A, 0x1519, 0x151B, 0x151C, 0x151D, 0x151E, 0xFFFF};
u16 quest_group_tab_spr_2[8] = {0x1527, 0x1529, 0x1528, 0x151B, 0x151C, 0x151D, 0x152A, 0xFFFF};
u16 quest_group_tab_spr_3[8] = {0x152C, 0x152E, 0x152D, 0x151B, 0x151C, 0x151D, 0x152F, 0xFFFF};
u16 quest_group_tab_spr_4[8] = {0x1531, 0x1533, 0x1532, 0x151B, 0x151C, 0x151D, 0x1534, 0xFFFF};
u16 quest_group_tab_spr_5[8] = {0x1536, 0x1538, 0x1537, 0x151B, 0x151C, 0x151D, 0x1539, 0xFFFF};
u16 quest_group_tab_spr_6[8] = {0x153B, 0x153D, 0x153C, 0x151B, 0x151C, 0x151D, 0x153E, 0xFFFF};
u16 quest_group_tab_spr_7[8] = {0x1541, 0x1543, 0x1542, 0x151B, 0x151C, 0x151D, 0x1544, 0xFFFF};
u16 quest_group_tab_spr_8[8] = {0x1546, 0x1548, 0x1547, 0x151B, 0x151C, 0x151D, 0x1549, 0xFFFF};
u16* quest_group_tab_spr_tbl[8] = {
    quest_group_tab_spr_1, quest_group_tab_spr_2, quest_group_tab_spr_3, quest_group_tab_spr_4,
    quest_group_tab_spr_5, quest_group_tab_spr_6, quest_group_tab_spr_7, quest_group_tab_spr_8,
};
u16 quest_group_tab_anim_ids[8] = {0x1517, 0x1526, 0x152B, 0x1530, 0x1535, 0x153A, 0x154B, 0x1545};
u16 quest_group_cursor_spr[6] = {0x1520, 0x1521, 0x1522, 0x1523, 0xFFFF, 0x0};

/* 0x8032214C (0x264): draws the group menu: each tab with its frame animation, its label (greyed when the group is
 * not open), the cursor, and the mark of a group with nothing cleared. */
extern "C" void quest_group_menu_draw(MenuQuestWork* w) {
    u16 frame;
    _mh_ivec2_ pos;
    _SPR_DATA_ frame_spr;
    _SPR_DATA_ spr;
    s32 i;
    s32 open;

    i = 0;
    open = 0;
    while (w->tabs_0x018[i] != 0xFF) {
        if (w->tabs_0x018[i] == w->groups_0x010[w->group_cursor_0x00C]) {
            frame = w->open_frame_0x004;
        } else {
            frame = 0;
        }
        sprite_frame_apply(&frame_spr, quest_group_tab_anim_ids[i], frame, &pos);
        draw_sprite_anim_ary(quest_group_tab_spr_tbl[i], frame, &pos);
        if (w->tabs_0x018[i] == 6) {
            draw_font_anim_idx(5439, frame, (s8*)LbStr(1, 27), 1, &pos);
        } else if (w->tabs_0x018[i] == 7) {
            sprite_frame_apply(&spr, 5439, frame, NULL);
            spr.width = 16;
            spr.height = 18;
            spr.pos.y++;
            draw_font(spr, (s8*)LbStr(1, 28), 1, &pos);
        } else {
            sprite_frame_apply(&spr, 5412, frame, NULL);
            if (w->groups_0x010[open] != w->tabs_0x018[i]) {
                spr.color = 0x404040FF;
            }
            draw_font(spr, (s8*)LbStr(1, 20), 0, &pos);
            sprite_frame_apply(&spr, 5413, frame, NULL);
            if (w->groups_0x010[open] != w->tabs_0x018[i]) {
                spr.color = 0x404040FF;
            }
            draw_font(spr, (s8*)LbStr(1, i + 21), 0, &pos);
        }
        if (chk_pointer() == 0 && frame != 0) {
            draw_sprite_ary(quest_group_cursor_spr, &pos);
        }
        if (w->tabs_0x018[i] != 6 && w->tabs_0x018[i] != 7 && quest_group_clear_count(NULL, w->tabs_0x018[i] + 1) == 0) {
            draw_sprite_anim_idx(5407, frame, &pos);
        }
        if (w->groups_0x010[open] == w->tabs_0x018[i]) {
            open++;
        }
        if (++i >= 8) {
            break;
        }
    }
}

/* The quest list's sprites: the frame, the title per group pair, the row positions and the page arrows. */
u16 quest_list_frame_spr[6] = {0x1561, 0x1562, 0x1563, 0x1564, 0xFFFF, 0x0};
u16 quest_list_title_spr[12] = {0x1557, 0x1559, 0x1558, 0x155A, 0x155B, 0x155C, 0x155D, 0x155F, 0x155E, 0xFFFF, 0x0, 0x0};
u16 quest_list_row_lsp_ids[8] = {0x1565, 0x1566, 0x1567, 0x1568, 0x1569, 0x156A, 0x156B, 0x156C};
u16 quest_list_page_arrow_spr[6] = {0x1552, 0x1550, 0x1551, 0x154E, 0x154F, 0x0};

/* 0x803223B0 (0x2A0): draws the quest list page: the frame, the group title (the event title for `mode` 1), the page
 * arrows and the rows, with the cleared mark. */
extern "C" void quest_list_draw(MenuQuestWork* w, u8 mode) {
    _mh_ivec2_ row_pos;
    _mh_ivec2_ mark_pos;
    _mh_ivec2_ pos;
    _SPR_DATA_ spr;
    s32 i;
    QuestRecord* rec;
    s32 sel;

    get_lsp_data(5472, &pos);
    draw_sprite_ary(quest_list_frame_spr, &pos);
    get_lsp_data(5459, &pos);
    draw_sprite_anim_ary(quest_list_title_spr, w->field_0x020 / 2, &pos);
    if (mode == 0) {
        switch (w->field_0x020) {
        case 6:
            draw_font_idx(5462, (s8*)LbStr(1, 27), 1, &pos);
            break;
        case 7:
            spr_data_copy(&spr, get_lsp_data(5462, NULL));
            spr.width = 16;
            spr.height = 18;
            spr.pos.y++;
            draw_font(spr, (s8*)LbStr(1, 28), 1, &pos);
            break;
        default:
            draw_font_idx(5460, (s8*)LbStr(1, 20), 0, &pos);
            draw_font_idx(5461, (s8*)LbStr(1, w->field_0x020 + 21), 0, &pos);
            break;
        }
    } else {
        draw_font_idx(5462, (s8*)LbStr(1, 29), 1, &pos);
    }
    get_lsp_data(5453, &pos);
    if (w->pages_0x03A != 0) {
        PutPageArrow(quest_list_page_arrow_spr, w->page_0x038, w->pages_0x03A, w->field_0x00A, &pos, 0);
    }
    get_lsp_data(5472, &pos);
    for (i = 0; i < w->count_0x024; i++) {
        rec = quest_record_find(w->ids_0x026[i]);
        if (rec != NULL) {
            _SPR_DATA_* lsp = get_lsp_data(quest_list_row_lsp_ids[i], NULL);

            row_pos.x = pos.x + lsp->pos.x;
            row_pos.y = pos.y + lsp->pos.y;
            if (i == w->value_0x022) {
                sel = 1;
            } else {
                sel = 0;
            }
            lb_list_row_draw((s8*)quest_result_field_text_get(rec, 22), sel, &row_pos, 0, 1);
            if (userdata_flag_ck(rec->field_0x02C) != 0) {
                get_lsp_data(5485, &mark_pos);
                mark_pos.y = row_pos.y;
                draw_sprite_idx(5486, &mark_pos);
            }
        }
    }
}

/* The quest detail page's sprites: the target frames (one target, or the two monster slots of a dual-target quest),
 * their label rows, the target icon frame and the monster slot frame. */
u16 quest_detail_target_spr[18] = {0x15C9, 0x15CA, 0x15CB, 0x15CC, 0x15CD, 0x15CE, 0x15B3, 0x15B4, 0x15B5,
                                   0x15B6, 0x15B7, 0x15B8, 0x15B9, 0x15BA, 0x15C2, 0x15C3, 0xFFFF, 0x0};
u16 quest_detail_dual_spr[8] = {0x15D0, 0x15D1, 0x15D2, 0x15D3, 0x15D4, 0x15D5, 0xFFFF, 0x0};
u16 quest_detail_target_labels[6] = {0x15BB, 0x15BC, 0x15BD, 0x15BE, 0x15BF, 0x0};
u16 quest_detail_dual_labels[6] = {0x15D6, 0x15D7, 0x15D8, 0x15D9, 0x15DA, 0x0};
u16 quest_detail_icon_spr[10] = {0x1590, 0x1591, 0x1592, 0x1593, 0x1594, 0x1595, 0x1596, 0x158E, 0x158F, 0xFFFF};
u16 quest_detail_monster_spr[10] = {0x159B, 0x159C, 0x159D, 0x159E, 0x159F, 0x15A0, 0x15A1, 0x1599, 0x159A, 0xFFFF};

/* The item icon's size and offset on the detail page (.sdata). */
u8 quest_detail_icon_size[2] = {0x44, 0x44};
u8 quest_detail_icon_ofs[2] = {0x12, 0x0D};

/* 0x80322650 (0x548): draws the detail page's target block: the two monster slots of a dual-target quest, or the
 * one target with its item, monster or material icon; then the five label rows and the dual-target extra rows. */
extern "C" void quest_detail_target_draw(QuestRecord* rec) {
    _mh_ivec2_ slot_pos;
    _mh_ivec2_ pos;
    _mh_ivec2_ icon_pos;
    u16* labels;
    u8 big;

    get_lsp_data(5554, &pos);
    big = quest_field32C_ck_c1(rec);
    if (quest_dual_target_ck(rec) == 1) {
        _SPR_DATA_* lsp;

        draw_sprite_ary(quest_detail_dual_spr, &pos);
        labels = quest_detail_dual_labels;
        if (big != 0) {
            draw_sprite_idx(5596, &pos);
        } else {
            draw_sprite_idx(5595, &pos);
        }
        lsp = get_lsp_data(5597, NULL);
        slot_pos.x = pos.x + lsp->pos.x;
        slot_pos.y = pos.y + lsp->pos.y;
        draw_sprite_ary(quest_detail_monster_spr, &slot_pos);
        draw_monstericon_idx(5528, quest_slot_byte_get(rec, 0), &slot_pos);
        lsp = get_lsp_data(5598, NULL);
        slot_pos.x = pos.x + lsp->pos.x;
        slot_pos.y = pos.y + lsp->pos.y;
        draw_sprite_ary(quest_detail_monster_spr, &slot_pos);
        draw_monstericon_idx(5528, quest_slot_byte_get(rec, 1), &slot_pos);
    } else {
        draw_sprite_ary(quest_detail_target_spr, &pos);
        if (big != 0) {
            draw_sprite_idx(5569, &pos);
        } else {
            draw_sprite_idx(5568, &pos);
        }
        get_lsp_data(5516, &icon_pos);
        draw_sprite_ary(quest_detail_icon_spr, &icon_pos);
        switch (quest_objective_get(rec)) {
        case 1: {
            u16 item = quest_slot_word_get(rec, 0);

            if (item != 0) {
                _SPR_DATA_ item_spr = *get_lsp_data(5516, NULL);

                item_spr.tex_flag = 0;
                item_spr.width = (s16)(s32)quest_detail_icon_size[0];
                item_spr.height = (s16)(s32)quest_detail_icon_size[1];
                icon_pos.x = (f32)(quest_detail_icon_ofs[0] + (s16)get_wide_offset_c1(item_spr.wide_idx));
                icon_pos.y = (f32)quest_detail_icon_ofs[1];
                draw_itemicon_item_id(item_spr, item, &icon_pos);
            }
            break;
        }
        case 9: {
            _SPR_DATA_ kind_spr = *get_lsp_data(5516, NULL);

            kind_spr.tex_flag = 0;
            kind_spr.width = (s16)(s32)quest_detail_icon_size[0];
            kind_spr.height = (s16)(s32)quest_detail_icon_size[1];
            icon_pos.x = (f32)(quest_detail_icon_ofs[0] + (s16)get_wide_offset_c1(kind_spr.wide_idx));
            icon_pos.y = (f32)quest_detail_icon_ofs[1];
            draw_tex_icon(&kind_spr, 34, item_kind_color_by_kind(0), &icon_pos);
            break;
        }
        default: {
            u8 monster = quest_slot_byte_get(rec, 0);

            if (monster != 0) {
                draw_monstericon_idx(5517, monster, &icon_pos);
            }
            break;
        }
        }
        labels = quest_detail_target_labels;
    }
    draw_font_idx(labels[0], (s8*)quest_result_field_text_get(rec, 9), 0, &pos);
    draw_font_idx(labels[1], (s8*)quest_result_field_text_get(rec, 10), 0, &pos);
    draw_font_idx(labels[2], (s8*)quest_result_field_text_get(rec, 11), 0, &pos);
    draw_font_idx(labels[3], (s8*)quest_result_field_text_get(rec, 12), 0, &pos);
    draw_font_idx(labels[4], (s8*)quest_result_field_text_get(rec, 6), 0, &pos);
    if (quest_dual_target_ck(rec) == 0) {
        draw_font_idx(5573, (s8*)quest_result_field_text_get(rec, 2), 0, &pos);
        draw_font_idx(5574, (s8*)quest_result_field_text_get(rec, 3), 0, &pos);
        draw_font_idx(5575, (s8*)quest_result_field_text_get(rec, 13), 0, &pos);
        draw_font_idx(5576, (s8*)quest_result_field_text_get(rec, 14), 0, &pos);
    }
}

/* The detail page's reward frame. */
u16 quest_detail_reward_spr[14] = {0x15E6, 0x15E7, 0x15E8, 0x15E0, 0x15E1, 0x15E2, 0x15E3,
                                   0x15E4, 0x15E5, 0x15E9, 0x15EC, 0x15ED, 0xFFFF, 0x0};

/* 0x80322B98 (0xD0): draws the detail page's reward block: the reward, the fee and the two reward rows. */
extern "C" void quest_detail_reward_draw(QuestRecord* rec) {
    _mh_ivec2_ pos;

    get_lsp_data(5599, &pos);
    draw_sprite_ary(quest_detail_reward_spr, &pos);
    draw_sprite_idx(5613, &pos);
    draw_font_idx(5610, (s8*)quest_result_field_text_get(rec, 30), 0, &pos);
    draw_font_idx(5611, (s8*)quest_result_field_text_get(rec, 31), 0, &pos);
    draw_font_idx(5614, (s8*)quest_result_field_text_get(rec, 4), 0, &pos);
    draw_font_idx(5615, (s8*)quest_result_field_text_get(rec, 5), 0, &pos);
}

/* The detail page's label-tab frame. */
u16 quest_detail_label_spr[10] = {0x15F1, 0x15F2, 0x15F3, 0x15F4, 0x15F5, 0x15F6, 0x15F7, 0x15F9, 0xFFFF, 0x0};

/* 0x80322C68 - draw the two label rows of one entry. */
extern "C" void quest_detail_label_draw(MenuQuestWork* self) {
    _mh_ivec2_ pos;
    char* text;

    get_lsp_data(0x15F0, &pos);
    draw_sprite_ary(quest_detail_label_spr, &pos);
    text = quest_result_field_text_get((QuestRecord*)self, 7);
    draw_font_idx(0x15F8, (s8*)text, 1, &pos);
    text = quest_result_field_text_get((QuestRecord*)self, 8);
    draw_font_idx(0x15FA, (s8*)text, 0, &pos);
}

/* The detail page's header frames. */
u16 quest_detail_header_spr[26] = {0x1580, 0x1581, 0x1582, 0x1583, 0x1584, 0x1585, 0x1586, 0x1587, 0x1588,
                                   0x1589, 0x158A, 0x158B, 0x1575, 0x1576, 0x1577, 0x1578, 0x1579, 0x157A,
                                   0x157B, 0x157C, 0x157D, 0x157E, 0x157F, 0x1573, 0x1574, 0xFFFF};
u16 quest_detail_title_spr[14] = {0x15A9, 0x15AA, 0x15AB, 0x15AC, 0x15AD, 0x15AE, 0x15AF,
                                  0x15B0, 0x15A6, 0x15A4, 0xFFFF, 0x0, 0x0, 0x0};

/* 0x80322CEC (0xF8): draws the detail page's header: the frames, the title and client rows, the dual-target mark
 * and row, and the cleared mark. */
extern "C" void quest_detail_header_draw(QuestRecord* rec) {
    _mh_ivec2_ pos;

    get_lsp_data(5490, &pos);
    draw_sprite_ary(quest_detail_header_spr, &pos);
    get_lsp_data(5538, &pos);
    draw_sprite_ary(quest_detail_title_spr, &pos);
    draw_font_idx(5541, (s8*)quest_result_field_text_get(rec, 0), 5, &pos);
    draw_font_idx(5543, (s8*)quest_result_field_text_get(rec, 16), 1, &pos);
    if (quest_dual_target_ck(rec) == 1) {
        draw_sprite_idx(5553, &pos);
        draw_font_idx(5544, (s8*)quest_result_field_text_get(rec, 17), 1, &pos);
    }
    if (userdata_flag_ck(rec->field_0x02C) != 0) {
        draw_sprite_idx(5539, &pos);
    }
}

/* The arena page's frames. */
u16 quest_arena_frame_spr[24] = {0x287A, 0x287B, 0x287C, 0x287D, 0x287E, 0x287F, 0x2880, 0x2881,
                                 0x2882, 0x2883, 0x2884, 0x2871, 0x2872, 0x2873, 0x2874, 0x2875,
                                 0x2876, 0x2877, 0x2878, 0x2879, 0x286E, 0x286F, 0x2870, 0xFFFF};
u16 quest_arena_record_spr[24] = {0x2852, 0x2853, 0x2854, 0x2855, 0x2856, 0x2857, 0x2858, 0x2859,
                                  0x285A, 0x285B, 0x285C, 0x285D, 0x285E, 0x285F, 0x2860, 0x2861,
                                  0x2862, 0x2863, 0x2864, 0x2867, 0x2868, 0x2869, 0x284D, 0xFFFF};

/* The arena page's time row frame (.sdata). */
u16 quest_arena_time_spr[4] = {0x289C, 0x289D, 0x289E, 0xFFFF};

/* 0x80322DE4 (0x420): draws the arena page of an arena quest: the best time and its holder, and the three time
 * limits. */
extern "C" void quest_detail_footer_draw(QuestRecord* rec) {
    _mh_ivec2_ pos;
    _mh_ivec2_ row_pos;
    char time[16];
    _SPR_DATA_ spr;
    _SPR_DATA_ unit_spr;
    Q_UserData* ud = get_userdata();
    char* holder;
    u8 index;

    if (quest_flag_10_ck(rec) == 1) {
        get_lsp_data(10348, &pos);
        draw_sprite_ary(quest_arena_frame_spr, &pos);
        draw_font_idx(10349, (s8*)LbStr(0, 422), 1, &pos);
        row_pos.x = pos.x;
        row_pos.y = pos.y + 150;
        draw_sprite_ary(quest_arena_record_spr, &row_pos);
        draw_font_idx(10318, (s8*)LbStr(0, 426), 2, &row_pos);
        index = rec->field_0x02C - 60000;
        if (index < 12) {
            s32 best = (s32)ud->arena_best_0x5334[index];

            if (best == 0) {
                sprintf(time, "--:--");
            } else {
                s16 min = best / (s32)(60.0f * Screen_w.frame_scale);
                s16 rest = best - min * (s32)(60.0f * Screen_w.frame_scale);
                s16 sec = rest / (s32)Screen_w.frame_scale;

                sprintf(time, "%02d:%02d", min, sec);
            }
            draw_font_idx(10320, (s8*)time, 0, &row_pos);
        }
        draw_font_idx(10319, (s8*)LbStr(0, 427), 2, &row_pos);
        holder = ud->arena_0x42D0[index].name_a;
        if (holder[0] != 0) {
            spr_data_copy(&spr, get_lsp_data(10321, NULL));
            spr.color = 0x505050FF;
            if (get_option_cfg(12) == 0) {
                draw_font(spr, (s8*)holder, 0, &row_pos);
            } else {
                draw_font(spr, (s8*)ud->arena_0x42D0[index].name_b, 0, &row_pos);
            }
        }
        get_lsp_data(10390, &pos);
        draw_sprite_ary(quest_arena_time_spr, &pos);
        draw_font_idx(10391, (s8*)LbStr(0, 423), 0, &pos);
        spr_data_copy(&spr, get_lsp_data(10392, NULL));
        spr_data_copy(&unit_spr, get_lsp_data(10395, NULL));
        unit_spr.color = 0x3C3C3CFF;
        spr.color = 0x3C3C3CFF;
        draw_font(spr, (s8*)quest_arena_time_text_get(rec, 0), 0, &pos);
        draw_font(unit_spr, (s8*)LbStr(1, 54), 0, &pos);
        get_lsp_data(10399, &row_pos);
        row_pos.x += pos.x;
        row_pos.y += pos.y;
        draw_sprite_ary(quest_arena_time_spr, &row_pos);
        draw_font_idx(10391, (s8*)LbStr(0, 424), 0, &row_pos);
        draw_font(spr, (s8*)quest_arena_time_text_get(rec, 1), 0, &row_pos);
        draw_font(unit_spr, (s8*)LbStr(1, 54), 0, &row_pos);
        get_lsp_data(10400, &row_pos);
        row_pos.x += pos.x;
        row_pos.y += pos.y;
        draw_sprite_ary(quest_arena_time_spr, &row_pos);
        draw_font_idx(10391, (s8*)LbStr(0, 425), 0, &row_pos);
        draw_font(spr, (s8*)quest_arena_time_text_get(rec, 2), 0, &row_pos);
        draw_font(unit_spr, (s8*)LbStr(1, 54), 0, &row_pos);
    }
}

/* 0x80323204 (0x114): draws quest `quest_id`'s detail page on tab `tab` (target, reward or labels) with its tab
 * arrows, the accept stamp and the arena page. */
extern "C" void quest_detail_draw(MenuQuestWork* w, u16 quest_id, s16 tab) {
    QuestRecord* rec = quest_record_find(quest_id);
    _mh_ivec2_ pos;
    s32 active;

    if (rec == NULL) {
        return;
    }
    quest_detail_header_draw(rec);
    active = w->state_0x001 == 1;
    switch (w->state_0x000) {
    case 4:
    case 5:
        break;
    case 7:
        if (w->talking_0x009 == 0) {
            break;
        }
    default:
        lb_page_arrow_draw(5487, w->field_0x03C, w->tab_count_0x03E, w->field_0x00A, active);
        break;
    }
    switch (tab) {
    case 0:
        quest_detail_target_draw(rec);
        break;
    case 1:
        quest_detail_reward_draw(rec);
        break;
    case 2:
        quest_detail_label_draw((MenuQuestWork*)rec);
        break;
    }
    if (w->accept_anim_0x006 != 0) {
        get_lsp_data(5488, &pos);
        draw_sprite_anim_idx(5489, w->accept_anim_0x006, &pos);
    }
    quest_detail_footer_draw(rec);
    font_flush();
}

/* 0x80323318 - the entry draw pass: a per-state body then the shared frame row. */
extern "C" void quest_list_page_draw(MenuQuestWork* self, u8 arg) {
    font_set_size(0x12, 0x12);
    switch (self->state_0x001) {
    case 0:
        quest_list_draw(self, arg);
        return;
    case 2:
        lb_panel_msg_draw(0x1877, 0x15B);
        lb_panel_yes_no_draw(0x1879, self->field_0x168);
        break;
    }
    quest_detail_draw(self, self->field_0x036, self->field_0x03C);
}

/* The quest screens' backdrop: the wide and the 4:3 frame, and the accept screen's title. */
u16 lb_quest_accept_title_spr[15] = {0x1503, 0x1504, 0x1505, 0x1506, 0x1500, 0x1501, 0x1502, 0x1507,
                                     0x1508, 0x1509, 0x150A, 0x150B, 0x14FE, 0x14FF, 0xFFFF};
u16 lb_quest_backdrop_spr[6] = {0x150F, 0x1510, 0x150D, 0x150E, 0xFFFF, 0x0};
u16 lb_quest_backdrop_wide_spr[6] = {0x1514, 0x1515, 0x1512, 0x1513, 0xFFFF, 0x0};

/* 0x803233A8 - the two-mode backdrop. */
extern "C" void lb_quest_accept_backdrop_draw(void) {
    _mh_ivec2_ pos;

    if (ck_WideMode() != 0) {
        get_lsp_data(0x1511, &pos);
        draw_sprite_ary(lb_quest_backdrop_wide_spr, &pos);
    } else {
        get_lsp_data(0x150C, &pos);
        draw_sprite_ary(lb_quest_backdrop_spr, &pos);
    }
    get_lsp_data(0x14FD, &pos);
    draw_sprite_ary(lb_quest_accept_title_spr, &pos);
}

/* The join settings page's sprites. */
u16 quest_join_comment_spr[19] = {0x162C, 0x1624, 0x1625, 0x1626, 0x1627, 0x1628, 0x1629, 0x162D, 0x162E, 0x162F,
                                  0x1630, 0x1631, 0x1632, 0x1633, 0x1634, 0x1635, 0x1636, 0x1623, 0xFFFF};
u16 quest_join_frame_spr[14] = {0x1619, 0x161A, 0x161B, 0x1618, 0x161C, 0x1611, 0x1610,
                                0x1612, 0x1613, 0x1614, 0x1615, 0x1616, 0x1617, 0xFFFF};
u16 quest_join_cursor_spr[6] = {0x1600, 0x15FD, 0x15FE, 0x15FF, 0xFFFF, 0x0};
u16 quest_join_comment_cursor_spr[10] = {0x160D, 0x1605, 0x1606, 0x1607, 0x1608, 0x1609, 0x160A, 0x160B, 0x160C,
                                         0xFFFF};

/* The join settings page's value frame (.sdata). */
u16 quest_join_value_spr[4] = {0x1601, 0x1602, 0x1603, 0xFFFF};

/* 0x80323428 (0xC4): wraps `text` into `out`, 24 characters a line. */
extern "C" void quest_comment_wrap(char* text, char* out) {
    char line[0x98];
    s32 n = flKnjMsgNum(text);

    *out = 0;
    for (;;) {
        char* cut;

        if (n <= 24) {
            strcat(out, text);
            break;
        }
        line[0] = 0;
        strcpy(line, text);
        cut = flKnjMsgNumPtr(line, 24);
        if (cut != NULL) {
            *cut = 0;
        }
        strcat(out, line);
        strcat(out, "\n");
        text = flKnjMsgNumPtr(text, 24);
        n -= 24;
    }
}

/* 0x803234EC (0x388): draws the join settings page: the comment, the party size with its arrows, the session name,
 * the cursor on row `field_0x164` while `active`, and the join prompt. */
extern "C" void quest_join_settings_draw(MenuQuestWork* w, s32 active) {
    _mh_ivec2_ pos;
    _mh_ivec2_ row_pos;
    char size[16];
    _SPR_DATA_ spr;
    char comment[0x99];
    u32 color;
    u8 prompt;

    get_lsp_data(5646, &pos);
    draw_sprite_ary(quest_join_frame_spr, &pos);
    draw_font_idx(5647, (s8*)LbStr(0, 419), 0, &pos);
    get_lsp_data(5661, &pos);
    draw_sprite_ary(quest_join_comment_spr, &pos);
    quest_comment_wrap(w->text_0x048, comment);
    draw_font_idx(5662, (s8*)LbStr(0, 413), 0, &pos);
    draw_font_idx(5663, (s8*)LbStr(0, 414), 0, &pos);
    draw_font_idx(5664, (s8*)LbStr(0, 415), 0, &pos);
    draw_font_idx(5665, (s8*)w->name_0x043, 1, &pos);
    draw_font_idx(5666, (s8*)comment, 0, &pos);
    spr_data_copy(&spr, get_lsp_data(5687, NULL));
    uv_pair_copy(&row_pos, &spr);
    row_pos.x += pos.x;
    row_pos.y += pos.y;
    draw_sprite_ary(quest_join_value_spr, &row_pos);
    sprintf(size, "%d", w->row_0x042);
    draw_font_idx(5628, (s8*)size, 1, &row_pos);
    if (w->field_0x164 == 0 && active != 0) {
        put_menu_cursor(quest_join_cursor_spr, 0, &row_pos);
        if (chk_pointer() == 0) {
            sprite_frame_apply(&spr, 5675, 1, NULL);
            color = spr.color;
            if (w->row_0x042 > 1) {
                spr_data_copy(&spr, get_lsp_data(5674, NULL));
                if (w->field_0x00A & 4) {
                    spr.color = color;
                }
                draw_sprite(spr, &pos);
            }
            if (w->row_0x042 < w->field_0x166) {
                spr_data_copy(&spr, get_lsp_data(5675, NULL));
                if (w->field_0x00A & 8) {
                    spr.color = color;
                }
                draw_sprite(spr, &pos);
            }
        }
    }
    spr_data_copy(&spr, get_lsp_data(5688, NULL));
    uv_pair_copy(&row_pos, &spr);
    row_pos.x += pos.x;
    row_pos.y += pos.y;
    draw_sprite_ary(quest_join_value_spr, &row_pos);
    draw_font_idx(5628, (s8*)LbStr(0, 416), 1, &row_pos);
    if (w->field_0x164 == 1 && active != 0) {
        put_menu_cursor(quest_join_cursor_spr, 0, &row_pos);
    }
    if (w->field_0x164 == 2 && active != 0) {
        get_lsp_data(5636, &row_pos);
        put_menu_cursor(quest_join_comment_cursor_spr, 0, &row_pos);
    }
    if (w->field_0x164 == 3 && active != 0) {
        prompt = 1;
    } else {
        prompt = 0;
    }
    get_lsp_data(5691, &row_pos);
    lb_cursor_prompt_draw(prompt, (s16*)&row_pos);
}

/* 0x80323874 - hand `state_0x01 == 0` to `quest_join_settings_draw`. */
extern "C" void quest_join_settings_page_draw(MenuQuestWork* self) {
    quest_join_settings_draw(self, self->state_0x001 == 0 ? 1 : 0);
}

/* 0x80323884 (0x120): draws the quest accept screen for its state. */
extern "C" void lb_quest_accept_screen_draw(void) {
    MenuQuestWork* w = lobby_w.quest_0x0AC;

    set_zmode(false, 0, false);
    set_blendmode(4, 5, 1);
    if (w->state_0x000 != 0 && w->state_0x000 != 7) {
        lb_quest_accept_backdrop_draw();
        menu_money_draw(5398);
    }
    switch (w->state_0x000) {
    case 1:
        quest_group_menu_draw(w);
        break;
    case 2:
        quest_list_page_draw(w, 0);
        break;
    case 3:
        quest_join_settings_page_draw(w);
        break;
    case 4:
    case 5:
        if (lobby_w.field_0x050 != 0) {
            quest_detail_draw(w, lobby_w.field_0x050, w->field_0x03C);
        }
        break;
    case 7:
        if (game_ready_ck() == 0 && lobby_w.field_0x050 != 0) {
            quest_detail_draw(w, lobby_w.field_0x050, w->field_0x03C);
        }
        break;
    }
    font_flush();
}

/* The join screen's title. */
u16 lb_quest_join_title_spr[16] = {0x27FD, 0x27FE, 0x27FF, 0x2800, 0x2801, 0x2802, 0x2803, 0x2804,
                                   0x2805, 0x2806, 0x2807, 0x2808, 0x27FB, 0x27FC, 0xFFFF, 0x0};

/* 0x803239A4 - the same backdrop for the second screen's frame. */
extern "C" void lb_quest_join_backdrop_draw(void) {
    _mh_ivec2_ pos;

    if (ck_WideMode() != 0) {
        get_lsp_data(0x1511, &pos);
        draw_sprite_ary(lb_quest_backdrop_wide_spr, &pos);
    } else {
        get_lsp_data(0x150C, &pos);
        draw_sprite_ary(lb_quest_backdrop_spr, &pos);
    }
    get_lsp_data(0x27FA, &pos);
    draw_sprite_ary(lb_quest_join_title_spr, &pos);
}

/* 0x80323A24 (0x124): draws the quest join screen for its state. */
extern "C" void lb_quest_join_screen_draw(void) {
    MenuQuestWork* w = lobby_w.quest_0x0AC;

    set_zmode(false, 0, false);
    set_blendmode(4, 5, 1);
    if (w->state_0x000 != 0 && w->state_0x000 != 6) {
        lb_quest_join_backdrop_draw();
        menu_money_draw(5398);
    }
    switch (w->state_0x000) {
    case 1:
        lb_choice_draw(&w->list_0x178, 6362, lb_eqshop_menu_frame, lb_eqshop_menu_rows, 6263, 0);
        break;
    case 2:
    case 3:
        quest_list_page_draw(w, 1);
        break;
    case 4:
        if (lobby_w.field_0x050 != 0) {
            quest_detail_draw(w, lobby_w.field_0x050, w->field_0x03C);
        }
        break;
    case 6:
        if (game_ready_ck() == 0 && lobby_w.field_0x050 != 0) {
            quest_detail_draw(w, lobby_w.field_0x050, w->field_0x03C);
        }
        break;
    }
}

/* 0x80323B48 (0x104): the number of board groups open, from the unlock conditions of the current mode. */
extern "C" s32 quest_group_open_count(void) {
    if (game_ready_ck() == 0) {
        if (lb_unlock_cond_ck(0x14) == 1) {
            return 5;
        }
        if (lb_unlock_cond_ck(0x10) == 1) {
            return 4;
        }
        if (lb_unlock_cond_ck(0xC) == 1) {
            return 3;
        }
        return (lb_unlock_cond_ck(9) == 1) + 1;
    }
    if (lb_unlock_cond_ck(0x27) == 1) {
        return 6;
    }
    if (lb_unlock_cond_ck(0x25) == 1) {
        return 5;
    }
    if (lb_unlock_cond_ck(0x23) == 1) {
        return 4;
    }
    if (lb_unlock_cond_ck(0x21) == 1) {
        return 3;
    }
    return (lb_unlock_cond_ck(0x1F) == 1) + 1;
}

/* 0x80323C4C - whether the id names a displayable entry in the current mode. */
extern "C" s32 quest_id_shown_ck(s32 id) {
    if ((u32)((u16)id - 0x3E80) <= 1) {
        return 0;
    }
    if (game_ready_ck() == 1) {
        if ((u16)id >= 0x2710) {
            if ((u16)id < 0xEA60) {
                return 1;
            }
        }
    } else {
        if ((u16)id >= 0x3E8) {
            if ((u16)id < 0x2328) {
                return 1;
            }
        }
    }
    return 0;
}

/* 0x80323CD4 (0xDC): counts the cleared quests of board slot `group` into `*count` and returns the last quest of
 * the slot shown and not cleared yet (0 when there is none). */
extern "C" u16 quest_group_clear_count(u16* count, u8 group) {
    s32 size;
    u16 open;
    u16 cleared;
    u16* top;
    u16* list;

    open = 0;
    cleared = 0;
    size = (quest_slot_count_get(group) + 1) * 2;
    list = (u16*)work_mem_alloc(size);
    top = list;

    memset(list, 0, size);
    quest_slot_items_get(group, list);
    for (; *list != 0; list++) {
        if ((u32)quest_id_shown_ck(*list) == 1) {
            s32 done = (u32)userdata_flag_ck(*list) == 1;

            if (done) {
                cleared++;
            }
            if (!done) {
                open = *list;
            }
        }
    }
    if (count != NULL) {
        *count = cleared;
    }
    work_mem_free(top);
    return open;
}

/* 0x80323DB0 (0x158): the urgent quest the latest unlock condition calls for, or 0 when there is none or it is
 * already cleared. */
extern "C" u16 quest_urgent_id_get(void) {
    u16 id = 0;

    if (game_ready_ck() == 1) {
        if (lb_unlock_cond_last_ck(0x1E) == 1) {
            id = 14000;
        } else if (lb_unlock_cond_last_ck(0x20) == 1) {
            id = 14001;
        } else if (lb_unlock_cond_last_ck(0x22) == 1) {
            id = 14002;
        } else if (lb_unlock_cond_last_ck(0x24) == 1) {
            id = 19000;
        } else if (lb_unlock_cond_last_ck(0x26) == 1) {
            id = 19001;
        }
    } else if (lb_unlock_cond_last_ck(8) == 1) {
        id = 2000;
    } else if (lb_unlock_cond_last_ck(0xB) == 1) {
        id = 2001;
    } else if (lb_unlock_cond_last_ck(0xF) == 1) {
        id = 2002;
    } else if (lb_unlock_cond_last_ck(0x11) == 1) {
        id = 2003;
    } else if (lb_unlock_cond_last_ck(0x13) == 1) {
        id = 2004;
    } else if (lb_unlock_cond_last_ck(0x19) == 1) {
        id = 2005;
    }
    if (id != 0 && (u32)userdata_flag_ck(id) == 1) {
        id = 0;
    }
    return id;
}

/* 0x80323F08 (0x4): whether the quest has two targets (its 0x2000000 flag). */
extern "C" u32 quest_dual_target_ck(QuestRecord* rec) {
    return quest_flag_2000000_ck(rec);
}

/* 0x80323F0C (0x318): whether quest `id` is listed under board group `group`: online every quest from 10000 (the
 * event range only on the event row), offline the quests whose rank's unlock condition (or urgent quest) is met. */
extern "C" u32 quest_group_id_ck(u16 id, u8 group) {
    if (game_ready_ck() == 1) {
        if (id >= 10000 && (id < 60000 || group == 7)) {
            return 1;
        }
    } else if (id >= 1000) {
        if (id >= 9000) {
            return 0;
        }
        switch (id) {
        case 1000:
            if (lb_unlock_cond_ck(6) == 0) {
                return 0;
            }
            break;
        case 1001:
        case 1002:
        case 1003:
        case 1004:
        case 1005:
            if (lb_unlock_cond_ck(7) == 0) {
                return 0;
            }
            break;
        case 1010:
        case 1011:
        case 1012:
        case 1013:
            if (lb_unlock_cond_ck(9) == 0) {
                return 0;
            }
            break;
        case 1014:
            if (lb_unlock_cond_ck(10) == 0) {
                return 0;
            }
            break;
        case 2001:
            if ((u32)userdata_flag_ck(2001) == 1) {
                return 0;
            }
            break;
        case 1020:
        case 1021:
        case 1022:
        case 1023:
        case 1024:
            if (lb_unlock_cond_ck(12) == 0) {
                return 0;
            }
            break;
        case 1025:
        case 1026:
            if (lb_unlock_cond_ck(13) == 0) {
                return 0;
            }
            break;
        case 1027:
        case 1028:
            if (lb_unlock_cond_ck(14) == 0) {
                return 0;
            }
            break;
        case 1030:
        case 1031:
        case 1032:
        case 1033:
        case 1034:
        case 1035:
            if (lb_unlock_cond_ck(16) == 0) {
                return 0;
            }
            break;
        case 2003:
            if ((u32)userdata_flag_ck(2003) == 1) {
                return 0;
            }
            break;
        case 1036:
        case 1037:
            if (lb_unlock_cond_ck(18) == 0) {
                return 0;
            }
            break;
        case 1038:
        case 1039:
            if (lb_unlock_cond_ck(19) == 0) {
                return 0;
            }
            break;
        case 1040:
        case 1041:
        case 1042:
        case 1043:
            if (lb_unlock_cond_ck(20) == 0) {
                return 0;
            }
            break;
        case 1044:
        case 1045:
        case 1046:
            if (lb_unlock_cond_ck(22) == 0) {
                return 0;
            }
            break;
        case 1047:
        case 1049:
        case 1050:
            if (lb_unlock_cond_ck(24) == 0) {
                return 0;
            }
            break;
        case 2005:
            if ((u32)userdata_flag_ck(2005) == 1) {
                return 0;
            }
            break;
        case 1048:
            if (lb_unlock_cond_ck(25) == 0) {
                return 0;
            }
            break;
        case 1060:
        case 1061:
        case 1062:
        case 1063:
        case 1064:
        case 1065:
        case 2006:
            if (lb_unlock_cond_ck(26) == 0) {
                return 0;
            }
            break;
        }
        return 1;
    }
    return 0;
}

/* 0x80324224 (0x8): the quest record's +0x348 word (the entry fee `quest_board_accept` charges). */
extern "C" s32 quest_fee_get(QuestRecord* rec) {
    return rec->field_0x348;
}
