/*
 * menu/menu_item_sub.cpp - the menu's lobby sub-pages that `menu/menu_item.cpp`'s page dispatcher opens, steps and
 *   draws: page 16 (the recent hunters and their friend requests), page 15 (the name-tag icon), page 14 (the guild
 *   card's title and comment), page 9 (the arena team names) and page 10 (the quest info pages), plus the quest-info
 *   helpers `enemy/em020_prog.cpp`'s info sheets call.
 * RANGE. .text 0x8036A828-0x8036CF64 (23 functions); extab 0x80017844-0x800178E4, extabindex 0x800372CC-0x800373BC,
 *   .data 0x805EE078-0x805EE098 (`chara_icon_tex_tbl`), .bss 0x806BF440-0x806BF530 (the two keyboard buffers).  The
 *   left edge is `ef/eft053.cpp`'s end; the right edge is `enemy/em020_prog.cpp`'s registration, whose head
 *   (0x8036CF64-0x8036E26C, the quest info page draws) calls only menu/hud code and reads as this TU's tail.
 * FLAGS. `cflags_menu` (configure.py).
 * NAMES. The file name is a GUESS from the band's role and neighbours; no `__FILE__` string covers it.  GUESS (from
 *   each body and its callers): `menu_item_sub_page14_cursor_set`, `menu_item_sub_page14_card_draw`,
 *   `menu_item_sub_page14_title_draw`, `menu_item_sub_page09_rows_build`, `menu_item_sub_comment_text`,
 *   `menu_item_sub_team_text`, and the registry names `lb_title_owned_ck`, `lb_title_range_find`, `net_entry_post`,
 *   `menu_entry_window_draw` (lane menu-item-sub).
 * RESIDUALS. `menu_item_sub_page14_draw`: retail tests 0..1 unsigned (`cmplwi`) and 2/3 signed (`cmpwi`); an `int` switch
 *   gives signed tests throughout, a `u32` switch unsigned ones.  `menu_item_sub_page14_title_draw`: `i`, `marked` and
 *   `picked` take other registers (declaration orders searched).  `menu_item_sub_page15_draw`: `marked`/`current`
 *   swap registers.  `menu_item_sub_page09_step`: the `(s8)entry_count_a` argument loads through r0.  The four
 *   foreign callees are spelled by their registry names until the map carries them.  `.bss`: the buffers are 0xAA and
 *   0x3E bytes (the `memset` lengths); the map's 0xAC/0x44 include alignment padding.
 * SHAPES. Flags handed to `GetMenuFontColor`'s `bool` parameters are `bool` locals; the quest-mode range test is
 *   spelled `(u32)(mode - 3) > 3`; page 14's owned-title test is an inline helper (the `neg/or/srwi` retest).
 */

#include "types.h"
#include "menu/menu_item.h"
#include "menu/menu_message.h"
#include "menu/menu_cursor_column_step.h"
#include "menu/menu_cursor_seek.h"
#include "menu/menu_entry_window_draw.h"
#include "hud/layout.h"
#include "hud/draw_font_align.h"
#include "hud/draw_layout_window.h"
#include "hud/put_lsp_anchor_offset.h"
#include "hud/cockpit.h"
#include "font/flfnt.h"
#include "fn_80040598.h"
#include "sound/fn_800D7F54.h"
#include "ef/fn_800CDB2C.h"
#include "MSL_C/string.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "fn_80047398/lobby_world_block.h"
#include "quest/quest_types.h"
#include "quest/quest_entry.h"
#include "quest/quest_item_slot.h"
#include "enemy/em_pop.h"
#include "enemy/lobby_state_block.h"
#include "enemy/lobby_friend_accept.h"
#include "enemy/net_entry_post.h"
#include "lobby/lb_quest_screen.h"
#include "lobby/hud_key_lookup.h"
#include "lobby/lobby_hunter_cards.h"
#include "lobby/lb_title_owned_ck.h"
#include "mh3_pad/lb_param_w.h"
#include "userdata_arena_rows_export.h"
#include "menu/chara_icon_tex_tbl.h"
#include "menu/quest_info_text_get.h"
#include "menu/menu_item_sub_page09_open.h"
#include "menu/menu_item_sub_page09_step.h"
#include "menu/menu_item_sub_page09_draw.h"
#include "menu/menu_item_sub_page10_open.h"
#include "menu/menu_item_sub_page10_step.h"
#include "menu/menu_item_sub_page10_draw.h"
#include "menu/menu_item_sub_page14_open.h"
#include "menu/menu_item_sub_page14_step.h"
#include "menu/menu_item_sub_page14_draw.h"
#include "menu/menu_item_sub_page15_open.h"
#include "menu/menu_item_sub_page15_step.h"
#include "menu/menu_item_sub_page15_draw.h"
#include "menu/menu_item_sub_page16_open.h"
#include "menu/menu_item_sub_page16_step.h"
#include "menu/menu_item_sub_page16_draw.h"

/* The callees this unit reads with another return width than their owners declare (the same direct call). */
#define quest_field32C_ck_c1 ((u8 (*)(QuestRecord*))quest_field32C_ck)
#define lb_title_owned_ck_c1 ((u32 (*)(LbEquipWork*, s16))lb_title_owned_ck)

/* The user-data block behind `lobby_world_block`. */
#define USERDATA ((Q_UserData*)lobby_world_block)

/* The two keyboard buffers: the guild card comment (page 14) and an arena team name (page 9). */
char menu_item_sub_comment_text[0xAA];
char menu_item_sub_team_text[0x3E];

/* The texture index of each name-tag icon kind (kind 0 has none). */
u32 chara_icon_tex_tbl[8] = { 0xFFFFFFFF, 0x28, 0x29, 0x2C, 0x2B, 0x27, 0x2A, 0x31 };

extern "C" {

static void menu_item_sub_page14_cursor_set(MenuSlot* slot, u8 title);
static void menu_item_sub_page14_card_draw(MenuSlot* slot);
static void menu_item_sub_page14_title_draw(MenuSlot* slot);
static void menu_item_sub_page09_rows_build(MenuSlot* slot, s16 cursor);

/* Whether the save owns title `title`. */
inline bool menu_item_sub_title_owned(s8 title) {
    return lb_title_owned_ck_c1((LbEquipWork*)lobby_world_block, title) != 0;
}

/* 0x8036A828 (0x68): opens page 16, the recent hunters list (eight rows a column). */
void menu_item_sub_page16_open(MenuSlot* slot) {
    slot->field_0x19E = 0;
    slot->field_0x1A3 = 8;
    slot->field_0x1A0 = 0;
    slot->field_0x1A1 = menu_page_count(16, 8);
    slot->field_0x1A2 = 0;
    slot->field_0x23A = 0;
    slot->field_0x001 = 0;
    slot->field_0x002 = 0;
}

/* 0x8036A890 (0x284): steps page 16: picks a recent hunter, confirms the friend request and waits for its answer. */
s32 menu_item_sub_page16_step(MenuSlot* slot) {
    s32 ret;
    u16 input;
    LobbyTimerBlock* block;

    ret = 0;
    input = slot->buttons_trig | (slot->field_0x008 & 0xF);
    block = &lobby_state_block;
    slot->field_0x19C = input;
    slot->field_0x19E = 0;
    switch (slot->field_0x001) {
    case 0:
        if (input & 0x10) {
            slot->page_index = slot->field_0x1A2 + slot->field_0x1A3 * slot->field_0x1A0;
            if (slot->page_index < block->recent_0x1DDC.count_0x000) {
                sysSE_req(0);
                slot->field_0x001 = 1;
            } else {
                sysSE_req(2);
            }
        } else if (input & 0x20) {
            sysSE_req(1);
            ret = 2;
        } else {
            menu_cursor_column_step((MenuListWork*)&slot->field_0x19C);
            slot->page_index = slot->field_0x1A2 + slot->field_0x1A3 * slot->field_0x1A0;
        }
        break;
    case 1:
        if (input & 0x10) {
            slot->sub_cursor_0x01D = 1;
            sysSE_req(0);
            slot->field_0x001 = 2;
        } else if (input & 0x20) {
            sysSE_req(1);
            slot->field_0x001 = 0;
        }
        break;
    case 2:
        if (input & 0x10) {
            if (slot->sub_cursor_0x01D == 1) {
                sysSE_req(0);
                slot->field_0x001 = 1;
            } else {
                switch ((u8)lobby_friend_accept(slot->page_index)) {
                case 1:
                    menu_slot0_lock_set(1);
                    sysSE_req(0);
                    slot->field_0x001 = 3;
                    break;
                case 0:
                case 2:
                    sysSE_req(2);
                    slot->field_0x001 = 0;
                    break;
                }
            }
        } else if (input & 0x20) {
            sysSE_req(1);
            slot->field_0x001 = 1;
        } else if (input & 0xC) {
            slot->sub_cursor_0x01D = menu_cursor_step(slot->sub_cursor_0x01D, 2, input, 4, 8);
        }
        break;
    case 3:
        if (block->result_0x1695 != 0) {
            if (block->result_0x1695 == 1) {
                lobby_recent_drop(slot->page_index);
            }
            menu_slot0_lock_set(0);
            slot->field_0x001 = 0;
        }
        break;
    }
    return ret;
}

/* 0x8036AB14 (0x66C): draws page 16: the recent hunters, the page arrows, the request dialog and the help line. */
void menu_item_sub_page16_draw(MenuSlot* slot) {
    _mh_ivec2_ base;
    _mh_ivec2_ pos;
    _SPR_DATA_ spr;
    u16* ids;
    LobbyRecentList* recent;
    s32 help;
    u32 dialog;
    u32 confirm;
    u32 listing;
    bool pointer_off;
    s32 i;
    s32 j;
    bool marked;
    bool picked;
    s32 index;

    recent = &lobby_state_block.recent_0x1DDC;
    help = -1;
    dialog = 0;
    confirm = 0;
    listing = 0;
    pointer_off = chk_pointer() == 0;
    switch (slot->field_0x001) {
    case 0:
        help = 0;
        listing = 1;
        break;
    case 1:
        help = 1;
        dialog = 1;
        break;
    case 2:
        confirm = 1;
        break;
    }
    get_lsp_data(0x2AE2, &base);
    draw_sprite_ary((const u16*)get_menu_lsp_tbl(0x15C), &base);
    draw_sprite_idx(0x2AEF, &base);
    get_lsp_data(0x2B14, &base);
    draw_sprite_ary((const u16*)get_menu_lsp_tbl(0xE), &base);
    for (i = 0, j = 0; i < slot->field_0x1A3; j++, i++) {
        get_lsp_data(0x2B14, &base);
        ids = (u16*)get_menu_lsp_tbl(0xF);
        put_lsp_anchor_offset(ids[j], &pos, base);
        draw_sprite_ary((const u16*)get_menu_lsp_tbl(0x10), &pos);
        marked = 0;
        picked = 0;
        if (i == slot->field_0x1A2) {
            if (listing == 0) {
                picked = 1;
            }
            if (pointer_off == 1) {
                draw_sprite_ary((const u16*)get_menu_lsp_tbl(0x11), &pos);
                marked = 1;
            }
        }
        index = i + slot->field_0x1A3 * slot->field_0x1A0;
        if (index < recent->count_0x000) {
            spr_data_copy(&spr, get_lsp_data(0x1DC, NULL));
            spr.color = GetMenuFontColor(1, marked, pointer_off, picked);
            draw_font_align(&spr, (s8*)recent->rows_0x004[index].name_0x00.text_0x00, 0, &pos);
            spr_data_copy(&spr, get_lsp_data(0x1DD, NULL));
            spr.color = GetMenuFontColor(1, marked, pointer_off, picked);
            draw_font(spr, (s8*)recent->rows_0x004[index].id_0x14.text_0x00, 0, &pos);
        } else {
            spr_data_copy(&spr, get_lsp_data(0x1DC, NULL));
            spr.color = GetMenuFontColor(0, marked, pointer_off, 0);
            draw_font(spr, (s8*)get_menu_msg(0x32, 6), 0, &pos);
            spr_data_copy(&spr, get_lsp_data(0x1DD, NULL));
            spr.color = GetMenuFontColor(0, marked, pointer_off, 0);
            draw_font(spr, (s8*)get_menu_msg(0x32, 6), 0, &pos);
        }
    }
    get_lsp_data(0x2AE1, &base);
    PutPageArrow((u16*)get_menu_lsp_tbl(1), slot->field_0x1A0, slot->field_0x1A1, slot->field_0x19E, &base, 0);
    if (dialog == 1) {
        get_lsp_data(0x2B21, &base);
        draw_sprite_ary((const u16*)get_menu_lsp_tbl(0x162), &base);
        put_lsp_anchor_offset(0x2B2B, &pos, base);
        draw_sprite_ary((const u16*)get_menu_lsp_tbl(4), &pos);
        if (pointer_off == 1) {
            draw_sprite_ary((const u16*)get_menu_lsp_tbl(5), &pos);
        }
        spr_data_copy(&spr, get_lsp_data(0x80, NULL));
        spr.color = GetMenuFontColor(1, pointer_off, pointer_off, 0);
        draw_font(spr, (s8*)get_menu_msg(0x32, 2), 1, &pos);
    }
    if (confirm == 1) {
        font_flush();
        spr_data_copy(&spr, get_lsp_data(0x2B17, &base));
        draw_layout_window(base.x, base.y, spr.width, spr.height);
        get_lsp_data(0x2B19, &base);
        draw_font_idx(0x2B1A, (s8*)get_menu_msg(0x32, 3), 6, &base);
        draw_font_idx(0x2B1B, (s8*)get_menu_msg(0x32, 4), 6, &base);
        draw_font_align_idx(0x2B1C, (s8*)recent->rows_0x004[slot->page_index].name_0x00.text_0x00, 4, &base);
        draw_font_idx(0x2B1D, (s8*)recent->rows_0x004[slot->page_index].id_0x14.text_0x00, 4, &base);
        draw_font_idx(0x2B1E, (s8*)get_menu_msg(0x32, 5), 0, &base);
        put_lsp_anchor_offset(0x2B1F, &pos, base);
        draw_sprite_ary((const u16*)get_menu_lsp_tbl(0xA), &pos);
        if (slot->sub_cursor_0x01D == 0 && pointer_off == 1) {
            draw_sprite_ary((const u16*)get_menu_lsp_tbl(0xD), &pos);
        }
        spr_data_copy(&spr, get_lsp_data(0x1AE, NULL));
        spr.color = GetMenuFontColor(1, slot->sub_cursor_0x01D == 0 && pointer_off, pointer_off, 0);
        draw_font(spr, (s8*)get_menu_msg(0xD, 0), 5, &pos);
        draw_sprite_ary((const u16*)get_menu_lsp_tbl(0xB), &pos);
        if (slot->sub_cursor_0x01D == 1 && pointer_off == 1) {
            draw_sprite_ary((const u16*)get_menu_lsp_tbl(0xC), &pos);
        }
        spr_data_copy(&spr, get_lsp_data(0x1B6, NULL));
        spr.color = GetMenuFontColor(1, slot->sub_cursor_0x01D == 1 && pointer_off, pointer_off, 0);
        draw_font(spr, (s8*)get_menu_msg(0xD, 1), 5, &pos);
    }
    if (help >= 0) {
        get_lsp_data(0x2B16, &base);
        draw_window_frame_preset(base.x, base.y, 0);
        font_set_size(16, 16);
        font_print_ex(base.x + 24, base.y + 16, 0, (s8*)get_menu_msg(0x32, help));
    }
}

/* 0x8036B180 (0x24): opens page 15 with the cursor on the current name-tag icon. */
void menu_item_sub_page15_open(MenuSlot* slot) {
    slot->page_index = USERDATA->net_level_0x3F0C;
    slot->field_0x23A = 0;
    slot->field_0x001 = 0;
    slot->field_0x002 = 0;
}

/* 0x8036B1A4 (0x11C): steps page 15: picks a name-tag icon and copies it to this hunter's card. */
s32 menu_item_sub_page15_step(MenuSlot* slot) {
    s32 ret;
    u16 input;
    u8 found;
    s8 level;
    LbQuestPage* card;

    ret = 0;
    input = slot->buttons_trig | (slot->field_0x008 & 0xF);
    switch (slot->field_0x001) {
    case 0:
        if (input & 0x10) {
            level = slot->page_index;
            if (USERDATA->net_level_0x3F0C != level) {
                USERDATA->net_level_0x3F0C = level;
                found = hud_key_lookup((u8*)slot->worker->hunter_0x5CA.id_0x5DB);
                if (found != 0xFF) {
                    card = &lobby_hunter_cards[found];
                    if (card->active_0x000 != 0) {
                        card->card_0x024[0xF3] = slot->page_index;
                    }
                }
                slot->field_0x31E = 1;
            }
            sysSE_req(0);
            ret = 2;
        } else if (input & 0x20) {
            sysSE_req(1);
            ret = 2;
        } else if (input & 3) {
            slot->page_index = menu_cursor_step(slot->page_index, 8, input, 1, 2);
        }
        break;
    }
    return ret;
}

/* 0x8036B2C0 (0x35C): draws page 15: the eight icon rows, the picked icon's description and the help line. */
void menu_item_sub_page15_draw(MenuSlot* slot) {
    _mh_ivec2_ base;
    _mh_ivec2_ pos;
    _SPR_DATA_ spr;
    u16* ids;
    bool pointer_off;
    s32 i;
    s32 j;
    bool marked;
    bool current;

    pointer_off = chk_pointer() == 0;
    get_lsp_data(0x2B2E, &base);
    draw_sprite_ary((const u16*)get_menu_lsp_tbl(0x163), &base);
    i = 0;
    j = 0;
    do {
        get_lsp_data(0x2B40, &base);
        ids = (u16*)get_menu_lsp_tbl(0x164);
        put_lsp_anchor_offset(ids[j], &pos, base);
        current = i == USERDATA->net_level_0x3F0C;
        if (i == 0) {
            draw_sprite_ary((const u16*)get_menu_lsp_tbl(8), &pos);
            if (i == slot->page_index && pointer_off == 1) {
                draw_sprite_ary((const u16*)get_menu_lsp_tbl(9), &pos);
                marked = 1;
            } else {
                marked = 0;
            }
            spr_data_copy(&spr, get_lsp_data(0x171, NULL));
            spr.color = GetMenuFontColor(1, marked, pointer_off, current);
            draw_font(spr, (s8*)get_menu_msg(0x30, i), 0, &pos);
        } else {
            draw_sprite_ary((const u16*)get_menu_lsp_tbl(6), &pos);
            if (i == slot->page_index && pointer_off == 1) {
                draw_sprite_ary((const u16*)get_menu_lsp_tbl(7), &pos);
                marked = 1;
            } else {
                draw_sprite_idx(0x141, &pos);
                marked = 0;
            }
            spr_data_copy(&spr, get_lsp_data(0x13C, NULL));
            spr.color = GetMenuFontColor(1, marked, pointer_off, current);
            draw_font(spr, (s8*)get_menu_msg(0x30, i), 0, &pos);
            spr_data_copy(&spr, get_lsp_data(0x13D, NULL));
            if (marked == 1) {
                spr.color = 0xFFE040FF;
            }
            pos.x += spr.pos.x;
            pos.y += spr.pos.y;
            draw_tex_icon_square(pos.x, pos.y, spr.width, spr.color, chara_icon_tex_tbl[i]);
        }
        j++;
        i++;
    } while (i < 8);
    if (slot->page_index != 0) {
        get_lsp_data(0x2B49, &base);
        draw_sprite_ary((const u16*)get_menu_lsp_tbl(0x160), &base);
        get_lsp_data(0x2B4A, &base);
        draw_sprite_ary((const u16*)get_menu_lsp_tbl(0x161), &base);
        draw_font_idx(0x2B10, (s8*)get_menu_msg(0x30, slot->page_index), 4, &base);
        draw_font_idx(0x2B11, (s8*)get_menu_msg(0x31, slot->page_index - 1), 0, &base);
    }
    get_lsp_data(0x2B4B, &base);
    draw_window_frame_preset(base.x, base.y, 0);
    font_set_size(16, 16);
    font_print_ex(base.x + 24, base.y + 16, 0, (s8*)get_menu_msg(0x30, 8));
}

/* 0x8036B61C (0x84): opens page 14, the guild card, with the title list on the current title. */
void menu_item_sub_page14_open(MenuSlot* slot) {
    slot->page_index = 0;
    slot->sub_cursor_0x01D = USERDATA->net_rank_0x3F0B;
    slot->field_0x19E = 0;
    slot->field_0x1A3 = 8;
    slot->field_0x1A1 = menu_page_count(0x75, 8);
    menu_item_sub_page14_cursor_set(slot, USERDATA->net_rank_0x3F0B);
    slot->field_0x23A = 0;
    slot->field_0x001 = 0;
    slot->field_0x002 = 0;
}

/* 0x8036B6A0 (0x48): puts the title list's cursor on title `title` (0 is the top). */
static void menu_item_sub_page14_cursor_set(MenuSlot* slot, u8 title) {
    if (title == 0) {
        slot->field_0x1A0 = 0;
        slot->field_0x1A2 = 0;
        return;
    }
    slot->field_0x1A0 = (title - 1) / slot->field_0x1A3;
    slot->field_0x1A2 = (title - 1) % slot->field_0x1A3;
}

/* 0x8036B6E8 (0x434): steps page 14: the title list and the comment keyboard. */
s32 menu_item_sub_page14_step(MenuSlot* slot) {
    s32 ret;
    u16 input;
    u8 found;
    s8 title;
    LbQuestPage* card;
    s8* src;
    u8* dst;
    s32 room;
    s32 size;
    s32 k;

    ret = 0;
    input = slot->buttons_trig | (slot->field_0x008 & 0xF);
    slot->field_0x19C = input;
    slot->field_0x19E = 0;
    switch (slot->field_0x001) {
    case 0:
        menu_item_sub_page14_cursor_set(slot, slot->field_0x01D);
    case 1:
        if (input & 0x10) {
            sysSE_req(0);
            if (slot->page_index == 0 && slot->sub_cursor_0x01D == 0) {
                slot->sub_cursor_0x01D = 1;
            }
            slot->field_0x001 = slot->page_index + 2;
            slot->field_0x002 = 0;
        } else if (input & 0x20) {
            ret = 2;
            sysSE_req(1);
        } else if (input & 3) {
            slot->page_index = menu_cursor_step(slot->page_index, 2, input, 1, 2);
        }
        break;
    case 2:
        if (input & 0x10) {
            if (menu_item_sub_title_owned(slot->sub_cursor_0x01D)) {
                title = slot->sub_cursor_0x01D;
                if (USERDATA->net_rank_0x3F0B != title) {
                    USERDATA->net_rank_0x3F0B = title;
                    found = hud_key_lookup((u8*)slot->worker->hunter_0x5CA.id_0x5DB);
                    if (found != 0xFF) {
                        card = &lobby_hunter_cards[found];
                        if (card->active_0x000 != 0) {
                            card->card_0x024[0xF2] = slot->sub_cursor_0x01D;
                        }
                    }
                    slot->field_0x31E = 1;
                }
                sysSE_req(0);
                slot->field_0x001 = 0;
            } else {
                sysSE_req(2);
            }
        } else if (input & 0x20) {
            slot->sub_cursor_0x01D = USERDATA->net_rank_0x3F0B;
            sysSE_req(1);
            slot->field_0x001 = 0;
        } else {
            menu_cursor_seek((MenuListWork*)&slot->field_0x19C, 0x75);
            slot->sub_cursor_0x01D = slot->field_0x1A2 + slot->field_0x1A3 * slot->field_0x1A0 + 1;
        }
        break;
    case 3:
        switch (slot->field_0x002) {
        case 0:
            if (kbd_open(0) == 1) {
                slot->field_0x32D = 1;
                memset(menu_item_sub_comment_text, 0, sizeof(menu_item_sub_comment_text));
                strcpy(menu_item_sub_comment_text, (char*)USERDATA->comment_0x528C);
                set_kbd_param(menu_item_sub_comment_text, 0xA8);
                menu_slot0_lock_set(1);
                slot->field_0x002 = 1;
            }
            break;
        case 1:
            switch (kbd_move()) {
            case 1:
                src = (s8*)menu_item_sub_comment_text;
                dst = USERDATA->comment_0x528C;
                room = 0x54;
                memset(dst, 0, sizeof(USERDATA->comment_0x528C));
                for (;;) {
                    size = utf82unicode2((u8*)src);
                    if (size == 1 && *src == 0) {
                        break;
                    }
                    if (room - size < 0) {
                        break;
                    }
                    for (k = 0; k < size; k++) {
                        *dst++ = *src++;
                    }
                    room -= size;
                }
                slot->field_0x32D = 0;
                menu_slot0_lock_set(0);
                slot->field_0x002++;
                break;
            case -1:
                slot->field_0x32D = 0;
                menu_slot0_lock_set(0);
                slot->field_0x001 = 0;
                slot->field_0x002 = 0;
                break;
            }
            break;
        case 2:
            slot->field_0x31E = 1;
            slot->field_0x001 = 0;
            slot->field_0x002 = 0;
            break;
        }
        break;
    }
    return ret;
}

/* 0x8036BB1C (0x30): draws page 14: the title list while it is open, else the card. */
void menu_item_sub_page14_draw(MenuSlot* slot) {
    switch ((u32)slot->field_0x001) {
    case 0:
    case 1:
        menu_item_sub_page14_card_draw(slot);
        break;
    case 2:
        menu_item_sub_page14_title_draw(slot);
        break;
    case 3:
        menu_item_sub_page14_card_draw(slot);
        break;
    }
}

/* 0x8036BB4C (0x2A4): draws the guild card: the title, the comment, the cursor and the help line. */
static void menu_item_sub_page14_card_draw(MenuSlot* slot) {
    _mh_ivec2_ base;
    _mh_ivec2_ pos;
    _SPR_DATA_ spr;
    s8 text[0xAC];
    bool active;
    bool marked;
    u8** titles;

    active = 0;
    if (slot->field_0x001 != 3 && (u32)chk_pointer() != 1) {
        active = 1;
    }
    get_lsp_data(0x2AA7, &base);
    draw_sprite_ary((const u16*)get_menu_lsp_tbl(0x158), &base);
    get_lsp_data(0x2AB7, &base);
    draw_sprite_ary((const u16*)get_menu_lsp_tbl(0x159), &base);
    draw_font_idx(0x2AB8, (s8*)get_menu_msg(0x2F, 0), 4, &base);
    draw_font_idx(0x2AB9, (s8*)get_menu_msg(0x2F, 1), 4, &base);
    marked = 0;
    if (slot->page_index == 0 && active == 1) {
        marked = 1;
    }
    titles = get_str_tbl(0x56);
    spr_data_copy(&spr, get_lsp_data(0x2ABA, NULL));
    spr.color = GetMenuFontColor(1, marked, active, 0);
    draw_font(spr, (s8*)titles[slot->sub_cursor_0x01D], 0, &base);
    knj_msg_str_build((s8*)USERDATA->comment_0x528C, text, 0x1C, 3);
    marked = 0;
    if (slot->page_index == 1 && active == 1) {
        marked = 1;
    }
    spr_data_copy(&spr, get_lsp_data(0x2ABB, NULL));
    spr.color = GetMenuFontColor(1, marked, active, 0);
    draw_font(spr, text, 0, &base);
    if (active == 1) {
        get_lsp_data(0x2ADD, &base);
        if (slot->page_index == 0) {
            put_lsp_anchor_offset(0x2ADE, &pos, base);
            draw_sprite_ary((const u16*)get_menu_lsp_tbl(0x15A), &pos);
        } else {
            put_lsp_anchor_offset(0x2ADF, &pos, base);
            draw_sprite_ary((const u16*)get_menu_lsp_tbl(0x15B), &pos);
        }
    }
    get_lsp_data(0x2AE0, &base);
    draw_window_frame_preset(base.x, base.y, 0);
    font_set_size(16, 16);
    font_print_ex(base.x + 24, base.y + 16, 0, (s8*)get_menu_msg(0x2F, 2));
}

/* 0x8036BDF0 (0x2E8): draws the title list: eight titles a column (unowned ones greyed), the page arrows, the picked
 * title's description and the help line. */
static void menu_item_sub_page14_title_draw(MenuSlot* slot) {
    bool active;
    u16* ids;
    u8** notes;
    _mh_ivec2_ base;
    _SPR_DATA_ spr;
    bool marked;
    s8 picked;
    s32 title;
    _mh_ivec2_ pos;
    u8** titles;
    s32 i;
    s32 j;

    active = chk_pointer() != 1;
    get_lsp_data(0x2AE2, &base);
    draw_sprite_ary((const u16*)get_menu_lsp_tbl(0x15C), &base);
    draw_sprite_idx(0x2AEE, &base);
    i = 0;
    j = 0;
    do {
        title = i + slot->field_0x1A3 * slot->field_0x1A0 + 1;
        if (title < 0x76) {
            get_lsp_data(0x2AFC, &base);
            ids = (u16*)get_menu_lsp_tbl(0x15F);
            put_lsp_anchor_offset(ids[j], &pos, base);
            draw_sprite_ary((const u16*)get_menu_lsp_tbl(0x15D), &pos);
            if (i == slot->field_0x1A2 && active == 1) {
                draw_sprite_ary((const u16*)get_menu_lsp_tbl(0x15E), &pos);
                marked = 1;
            } else {
                marked = 0;
            }
            titles = get_str_tbl(0x56);
            spr_data_copy(&spr, get_lsp_data(0x2AF4, NULL));
            if (lb_title_owned_ck_c1((LbEquipWork*)lobby_world_block, title) != 0) {
                spr.color = GetMenuFontColor(1, marked, active, 0);
                draw_font(spr, (s8*)titles[title], 1, &pos);
            } else {
                spr.color = GetMenuFontColor(0, marked, active, 0);
                draw_font(spr, (s8*)titles[0], 1, &pos);
            }
        }
        j++;
        i++;
    } while (i < 8);
    get_lsp_data(0x2AE1, &base);
    PutPageArrow((u16*)get_menu_lsp_tbl(1), slot->field_0x1A0, slot->field_0x1A1, slot->field_0x19E, &base, 1);
    picked = slot->sub_cursor_0x01D;
    if (lb_title_owned_ck_c1((LbEquipWork*)lobby_world_block, picked) != 0) {
        get_lsp_data(0x2B05, &base);
        draw_sprite_ary((const u16*)get_menu_lsp_tbl(0x160), &base);
        get_lsp_data(0x2B0F, &base);
        draw_sprite_ary((const u16*)get_menu_lsp_tbl(0x161), &base);
        draw_font_idx(0x2B10, (s8*)get_str_tbl(0x56)[picked], 4, &base);
        notes = get_str_tbl(0x57);
        draw_font_idx(0x2B11, (s8*)notes[lb_title_range_find(picked)], 0, &base);
    }
    get_lsp_data(0x2B16, &base);
    draw_window_frame_preset(base.x, base.y, 0);
    font_set_size(16, 16);
    font_print_ex(base.x + 24, base.y + 16, 0, (s8*)get_menu_msg(0x2F, 3));
}

/* 0x8036C0D8 (0x108): fills page 9's two command rows (`entries_a`) with the cursor on row `cursor`. */
static void menu_item_sub_page09_rows_build(MenuSlot* slot, s16 cursor) {
    MenuEntry* row;
    s32 i;
    bool active;
    bool picked;

    row = slot->entries_a;
    for (i = 0; i < (s8)slot->entry_count_a; i++, row++) {
        row->field_0x02 = 1;
        row->field_0x08 = get_menu_msg(0x2A, i);
        active = chk_pointer() != 1;
        picked = 0;
        if (slot->field_0x001 < 10 || slot->field_0x001 == 22) {
            if (i == cursor && chk_pointer() == 0) {
                row->selected = 1;
            } else {
                row->selected = 0;
            }
        } else {
            row->selected = 0;
            active = 0;
            if (i == cursor) {
                picked = 1;
            }
        }
        row->field_0x0C = GetMenuFontColor(1, row->selected != 0, active, picked);
    }
}

/* 0x8036C1E0 (0xA4): opens page 9, the arena team names (six rows a column). */
void menu_item_sub_page09_open(MenuSlot* slot) {
    userdata_arena_rows_export(lobby_state_block.rows_0x13E6[0]);
    slot->field_0x014 = 9;
    slot->field_0x19E = 0;
    slot->entry_count_b = 6;
    slot->field_0x1A3 = 6;
    slot->field_0x1A0 = 0;
    slot->field_0x1A1 = menu_page_count(12, 6);
    slot->field_0x1A2 = 0;
    slot->field_0x23A = 0;
    slot->field_0x01A = 0;
    slot->entry_count_a = 2;
    menu_slot_entry_b_select(slot, 0);
    menu_item_sub_page09_rows_build(slot, slot->field_0x01A);
}

/* 0x8036C284 (0x464): steps page 9: picks a team row, then posts it as the entry or renames it on the keyboard. */
s32 menu_item_sub_page09_step(MenuSlot* slot) {
    s32 ret;
    u16 input;
    u8 index;
    s8* src;
    u8* dst;
    s32 room;
    s32 size;
    s32 k;

    ret = 0;
    input = slot->buttons_trig | (slot->field_0x008 & 0xF);
    slot->field_0x19C = input;
    slot->field_0x19E = 0;
    switch (slot->field_0x001) {
    case 0:
        if (slot->field_0x23A != 0) {
            slot->field_0x23A--;
        }
        if (input & 0x10) {
            slot->field_0x01A = 0;
            slot->field_0x001 = 1;
            sysSE_req(0);
            menu_item_sub_page09_rows_build(slot, slot->field_0x01A);
        } else if (input & 0x20) {
            ret = 2;
            sysSE_req(1);
        } else {
            menu_cursor_column_step((MenuListWork*)&slot->field_0x19C);
        }
        menu_slot_entry_b_select(slot, slot->field_0x1A2);
        break;
    case 1:
        if (slot->field_0x23A < 2) {
            slot->field_0x23A++;
        }
        if (input & 0x10) {
            switch (slot->field_0x01A) {
            case 0:
                slot->field_0x001 = 10;
                break;
            case 1:
                sysSE_req(0);
                slot->field_0x001 = 20;
                break;
            }
        } else if (input & 0x20) {
            slot->field_0x001 = 0;
            sysSE_req(1);
        } else {
            slot->field_0x01A = menu_cursor_step(slot->field_0x01A, (s8)slot->entry_count_a, input, 1, 2);
            if (input & 3) {
                menu_item_sub_page09_rows_build(slot, slot->field_0x01A);
            }
        }
        break;
    case 10:
        menu_item_sub_page09_rows_build(slot, slot->field_0x01A);
        slot->field_0x001++;
    case 11:
        if (net_entry_post(slot->worker, em020_row_ptr(slot->field_0x1A2 + slot->field_0x1A3 * slot->field_0x1A0)) == 1) {
            sysSE_req(0);
            ret = 4;
        } else {
            ret = 4;
        }
        break;
    case 20:
        menu_item_sub_page09_rows_build(slot, slot->field_0x01A);
        slot->field_0x001++;
    case 21:
        if (kbd_open(0) == 1) {
            slot->field_0x32D = 1;
            index = slot->field_0x1A2 + slot->field_0x1A3 * slot->field_0x1A0;
            memset(menu_item_sub_team_text, 0, sizeof(menu_item_sub_team_text));
            strcpy(menu_item_sub_team_text, (char*)lobby_state_block.rows_0x13E6[index]);
            set_kbd_param(menu_item_sub_team_text, 0x3C);
            menu_slot0_lock_set(1);
            slot->field_0x001++;
        }
        break;
    case 22:
        switch (kbd_move()) {
        case 1:
            src = (s8*)menu_item_sub_team_text;
            dst = lobby_state_block.rows_0x13E6[0];
            index = slot->field_0x1A2 + slot->field_0x1A3 * slot->field_0x1A0;
            dst += index * sizeof(lobby_state_block.rows_0x13E6[0]);
            room = 0x1E;
            memset(dst, 0, sizeof(lobby_state_block.rows_0x13E6[0]));
            for (;;) {
                size = utf82unicode2((u8*)src);
                if (size == 1 && *src == 0) {
                    break;
                }
                if (room - size < 0) {
                    break;
                }
                for (k = 0; k < size; k++) {
                    *dst++ = *src++;
                }
                room -= size;
            }
        case -1:
            userdata_arena_rows_import(lobby_state_block.rows_0x13E6[0]);
            menu_slot0_lock_set(0);
            slot->field_0x32D = 0;
            slot->field_0x001 = 0;
            break;
        }
        break;
    }
    return ret;
}

/* 0x8036C6E8 (0x2A0): draws page 9: the team rows, the page arrows, the help line and the command window. */
void menu_item_sub_page09_draw(MenuSlot* slot) {
    s32 i;
    s32 index;
    bool active;
    s32 commands;
    s32 listing;
    _mh_ivec2_ pos;
    s8* help;
    s8 text[0x20];
    _SPR_DATA_ spr;

    if (slot->field_0x001 == 0) {
        listing = 1;
        commands = 0;
    } else {
        commands = 1;
        listing = 0;
    }
    get_lsp_data(0xAD2, &pos);
    draw_sprite_anim_ary((const u16*)get_menu_lsp_tbl(0x153), slot->field_0x23A, &pos);
    get_lsp_data(0xAFC, &pos);
    PutPageArrow((u16*)get_menu_lsp_tbl(0), slot->field_0x1A0, slot->field_0x1A1, slot->field_0x19E, &pos, 0);
    get_lsp_data(0xAE8, &pos);
    index = slot->field_0x1A3 * slot->field_0x1A0;
    for (i = 0; i < slot->entry_count_b; i++, index++) {
        draw_sprite_ary((const u16*)get_menu_lsp_tbl(0x154), &pos);
        active = 0;
        if (slot->field_0x001 == 0 && chk_pointer() == 0) {
            active = 1;
        }
        spr_data_copy(&spr, get_lsp_data(0xAF1, NULL));
        spr.color = GetMenuFontColor(1, i == slot->field_0x1A2, active, 0);
        str_tbl_utf16_build((s8*)em020_row_ptr(index), text, 9);
        draw_font_align(&spr, text, i == slot->field_0x1A2 ? 4 : 0, &pos);
        if (listing != 0 && i == slot->field_0x1A2 && chk_pointer() == 0) {
            put_menu_cursor((u16*)get_menu_lsp_tbl(0x155), 0, &pos);
        }
        pos.y += 26;
    }
    get_lsp_data(0xAF6, &pos);
    draw_window_frame_preset(pos.x, pos.y, 0);
    if (slot->field_0x001 == 0) {
        help = (s8*)get_menu_msg(0x2B, 2);
    } else {
        help = (s8*)get_menu_msg(0x2B, slot->field_0x01A);
    }
    if (help != NULL) {
        font_set_size(16, 16);
        font_print_ex(pos.x + 24, pos.y + 16, 0, help);
    }
    if (commands != 0) {
        menu_entry_window_draw(0xAF7, get_menu_lsp_tbl(0x156), get_menu_lsp_tbl(0x157), slot->entries_a,
                               slot->entry_count_a);
    }
}

/* 0x8036C988 (0xE8): the quest info mode: 1 none posted, 2 the lobby's quest (3 when one is picked), 3-6 the posted
 * quest's kind. */
s32 quest_info_mode_get(void) {
    s32 mode;

    mode = 0;
    switch (GameMode_ck()) {
    case 2:
        if (lb_param_w.field_0x00 == 0) {
            mode = 1;
        } else {
            mode = 2;
        }
        break;
    case 1:
        if (move_work_state_ck() == 1) {
            mode = 1;
        } else if (quest_flag_100000_ck(NULL) == 1) {
            mode = 6;
        } else if (quest_flag_10_ck(NULL) == 1) {
            mode = 5;
        } else if (quest_flag_2000000_ck(NULL) == 1 || quest_flag_80000000_ck(NULL) == 1 ||
                   quest_flag_4000000_ck(NULL) == 1) {
            mode = 4;
        } else {
            mode = 3;
        }
        break;
    }
    return mode;
}

/* 0x8036CA70 (0x54): text field `line` of the quest mode `mode` shows. */
s8* quest_info_text_get(s32 mode, s32 line) {
    s8* text;

    if ((u32)(mode - 3) > 3) {
        text = (s8*)quest_result_field_text_get(quest_record_find(lb_param_w.field_0x00), line);
    } else {
        text = (s8*)quest_result_field_text_cur_get(line);
    }
    return text;
}

/* 0x8036CAC4 (0x40): whether the quest mode `mode` shows has a reward flag set. */
u8 quest_info_reward_ck(s32 mode) {
    QuestRecord* record;

    switch (mode) {
    case 3:
    case 4:
    case 5:
    case 6:
        record = NULL;
        break;
    default:
        record = quest_record_find(lb_param_w.field_0x00);
        break;
    }
    return quest_field32C_ck_c1(record);
}

/* 0x8036CB04 (0x7C): opens page 10, the quest info pages. */
void menu_item_sub_page10_open(MenuSlot* slot) {
    slot->field_0x014 = 10;
    slot->scroll.info_page = 0;
    switch (quest_info_mode_get()) {
    case 1:
        slot->scroll.info_page_count = 1;
        break;
    case 2:
        slot->scroll.info_page_count = 3;
        break;
    case 3:
    case 4:
    case 5:
    case 6:
        slot->scroll.info_page_count = 4;
        break;
    }
}

/* 0x8036CB80 (0xC4): steps page 10: turns the info pages. */
s32 menu_item_sub_page10_step(MenuSlot* slot) {
    s32 ret;
    u16 input;

    ret = 0;
    input = slot->buttons_trig | (slot->field_0x008 & 0xF);
    if (input & 0x20) {
        ret = 2;
        sysSE_req(1);
    } else if (quest_info_mode_get() == 1) {
        slot->scroll.info_page = 0;
        slot->scroll.info_page_count = 1;
    } else if (input & 0xC) {
        slot->scroll.info_page =
            menu_cursor_step(slot->scroll.info_page, slot->scroll.info_page_count, input, 4, 8);
    }
    return ret;
}

/* 0x8036CC44 (0x234): draws page 10: the frame, the page arrows, the quest's name line and the page shown. */
void menu_item_sub_page10_draw(MenuSlot* slot) {
    _mh_ivec2_ pos;
    s32 mode;
    u16** frames;

    mode = quest_info_mode_get();
    get_lsp_data(0x9FC, &pos);
    frames = (u16**)get_menu_lsp_tbl(0xF5);
    if (mode == 2) {
        draw_sprite_ary((frames + slot->scroll.info_page)[1], &pos);
    } else {
        draw_sprite_ary(frames[slot->scroll.info_page], &pos);
    }
    draw_sprite_ary((const u16*)get_menu_lsp_tbl(0xF6), &pos);
    if (mode != 1) {
        if (slot->scroll.info_page == 0) {
            draw_sprite_idx(0xA12, &pos);
        } else if (slot->scroll.info_page == 1 && (mode < 4 || mode > 6)) {
            draw_sprite_idx(0xA12, &pos);
        }
        PutPageArrow((u16*)get_menu_lsp_tbl(0xF7), slot->scroll.info_page, slot->scroll.info_page_count,
                     slot->field_0x008, &pos, 0);
        get_lsp_data(0xA19, &pos);
        draw_sprite_ary((const u16*)get_menu_lsp_tbl(0xF8), &pos);
        draw_font_idx(0xA1C, quest_info_text_get(mode, 0), 5, &pos);
    }
    switch (slot->scroll.info_page) {
    case 0:
        if (mode == 1) {
            get_lsp_data(0x9FC, &pos);
            if (quest_select_ready_ck() == 0) {
                draw_font_idx(0x9FD, (s8*)get_str_tbl(0x2B)[0], 5, &pos);
            } else {
                quest_info_name_draw(quest_id_low_get(), &pos);
            }
        } else if (mode == 2) {
            quest_info_detail_draw(mode);
        } else {
            quest_info_sheet_draw(mode);
        }
        break;
    case 1:
        if (mode != 2) {
            quest_info_detail_draw(mode);
        } else {
            quest_info_summary_draw(mode);
        }
        break;
    case 2:
        if (mode != 2) {
            quest_info_summary_draw(mode);
        } else {
            quest_info_objective_draw(mode);
        }
        break;
    case 3:
        quest_info_objective_draw(mode);
        break;
    }
}

/* 0x8036CE78 (0xEC): whether the posted quest's entry is paid: the quest is ready, or its required items are in. */
u32 quest_info_paid_ck(void) {
    QuestWork* work;

    work = (QuestWork*)move_work_item_work_get();
    if (work == NULL) {
        return 0;
    }
    if (quest_entry_ready_ck() == 1) {
        return 1;
    }
    if (quest_flag_2000000_ck(NULL) == 1 || quest_flag_80000000_ck(NULL) == 1) {
        if (quest_element_pick_ck(work, 0, 1) == 1 && quest_element_pick_ck(work, 1, 1) == 1) {
            return 1;
        }
        return 0;
    }
    if (quest_flag_4000000_ck(NULL) == 1) {
        return quest_item_work_notify(2) == 1;
    }
    return quest_item_work_notify(0) == 1;
}

}
