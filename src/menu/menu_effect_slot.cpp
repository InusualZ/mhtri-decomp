/*
 * menu/menu_effect_slot.cpp - the menu page the top menu opens as kind 3: a four-tab page over one `MenuSlot`
 *   (`menu/menu_item.h`) whose tabs are the item-row list (the save's eight row records), the marked-monster list, the
 *   known-combination list and the three-level note list with its option card, plus each list's row builder and
 *   scroller.
 * RANGE. .text 0x80348A48-0x80349DD8 (16 functions); extab 0x80016F74-0x80016FE4, extabindex 0x80036594-0x8003663C.
 *   No data of its own.
 * FLAGS. `cflags_menu` (configure.py).
 * NAMES. Module `menu` because the band is filed with the menu units that follow it; the file name is a GUESS from the
 *   neighbours, no `__FILE__` string covers it.  GUESS names, from the bodies: `menu_row_list_open`/`_step`/`_build`/
 *   `_scroll`, `menu_monster_list_open`/`_step`/`_build`, `menu_combo_list_build`/`_scroll`, `menu_note_list_open`/
 *   `_step`, `note_slot_pair_copy`; `MenuRowPair`.
 * RESIDUALS.
 *  - `menu_monster_list_build`: `first` and the table pointer swap registers (r23/r26, 9 bytes); no declaration order,
 *    `table[i]` indexing, for-init assignment or split assignment reaches it.
 *   flipcheck: `.text` 9 of 5008 bytes differ (that register pair); nothing else.
 * SHAPES. The tab handlers read the button word as `buttons_trig | (field_0x008 & 0xF)`.
 *  - `MenuSlot` declares +0x016/+0x01E/+0x01F `u8`, which retail reads and writes as `s8`: the `(s8&)` views give
 *    retail's direct `extsb`/`li -1`; an `(s8)` value cast adds a `clrlwi`.
 *  - `menu_row_list_scroll` spells its state test `(u32)state <= 1 || (s32)state == 3` (retail `cmplwi` then `cmpwi`).
 *  - `menu_row_list_build` tests kind 0 before its `switch` (retail's compare chain is 0..6, the kind-0 body last).
 *  - `menu_note_list_step`: one pair temporary per level (stack 0x10/0x0C/0x08) and a function-scope one for level 3
 *    (0x14); the option row's index is `base + (row + rows_per_page * col)`.
 */

#include "types.h"
#include "menu/menu_item.h"
#include "menu/menu_item_page.h"
#include "menu/menu_message.h"
#include "menu/menu_effect_slot.h"
#include "menu/menu_row_clear.h"
#include "menu/note_slot_cursor_bounds.h"
#include "menu/fn_8031A6C0.h"        /* MenuComboRecipe, the combination row view (rule 1) */
#include "hud/sprite_ary_last_frame.h"
#include "hud/monster_mark_ck.h"
#include "hud/cockpit.h"             /* str_tbl_33_get */
#include "lobby/lb_combo_pair_recipe_find.h"
#include "lobby/lb_combo_recipe_known_ck.h"
#include "place_rec_copy.h"
#include "sound/fn_800D7F54.h"       /* sysSE_req */

/* The pair `note_slot_cursor_bounds` packs into its result's high half. */
typedef union MenuRowPairWord {
    /* +0x00 */ u16 raw;
    /* +0x00 */ MenuRowPair pair;
} MenuRowPairWord; /* size: 0x2 */

/* 0x80348A48 (0x30): opens the page with the tab cursor and the sub-state cleared. */
extern "C" void menu_effect_slot_open(MenuSlot* slot)
{
    slot->field_0x014 = 3;
    slot->field_0x001 = 0;
    slot->page_index = 0;
    slot->field_0x01D = 0;
    slot->entry_count_a = 0;
    slot->field_0x01A = 0;
    slot->field_0x23A = 30;
    slot->field_0x23C = 0;
}

/* 0x80348A78 (0x22C): picks a tab, then steps the tab's list until it closes; returns 2 when the page closes. */
extern "C" s32 menu_effect_slot_step(MenuSlot* slot)
{
    s32 result = 0;
    u16 buttons = slot->buttons_trig | (slot->field_0x008 & 0xF);

    ainpc_page_hold_clear();
    switch (slot->field_0x001) {
    case 0:
        if (slot->field_0x23A > 30) {
            slot->field_0x23A--;
        }
        break;
    case 1:
        if (slot->field_0x23A < 41) {
            slot->field_0x23A++;
        }
        break;
    }
    slot->field_0x23C++;
    if (slot->field_0x23C > sprite_ary_last_frame((const u16*)get_menu_lsp_tbl(0x53))) {
        slot->field_0x23C = 0;
    }
    switch (slot->field_0x001) {
    case 0:
        if (buttons & 0x10) {
            switch (slot->page_index) {
            case 0:
                menu_row_list_open(slot);
                break;
            case 1:
                menu_monster_list_open(slot);
                break;
            case 2:
                menu_combo_list_open(slot);
                break;
            case 3:
                menu_note_list_open(slot);
                break;
            }
            slot->scroll.light_colour = 0;
            slot->field_0x23B = 0;
            slot->field_0x001 = 1;
            sysSE_req(26);
        } else if (buttons & 0x20) {
            result = 2;
            sysSE_req(1);
        } else if (buttons & 3) {
            slot->page_index = (s8)menu_cursor_step(slot->page_index, 4, buttons, 1, 2);
        }
        break;
    case 1: {
        s32 state = 2;
        switch (slot->page_index) {
        case 0:
            state = menu_row_list_step(slot);
            break;
        case 1:
            state = menu_monster_list_step(slot);
            break;
        case 2:
            state = menu_combo_list_step(slot);
            break;
        case 3:
            state = menu_note_list_step(slot);
            break;
        }
        if (state == 2) {
            slot->field_0x001 = 0;
            sysSE_req(1);
        }
        break;
    }
    }
    return result;
}

/* 0x80348CA4 (0x50): opens the item-row tab over the eight row records. */
extern "C" void menu_row_list_open(MenuSlot* slot)
{
    menu_scroll_init(&slot->scroll, 0, 8, 8);
    menu_row_list_build(&slot->scroll, slot->entries_b);
    slot->field_0x002 = 0;
}

/* 0x80348CF4 (0x2E0): steps the item-row tab: pick a row, then delete it or move it to another row. */
extern "C" s32 menu_row_list_step(MenuSlot* slot)
{
    s32 result = 0;
    u16 buttons = slot->buttons_trig | (slot->field_0x008 & 0xF);
    MenuScroll* scroll = &slot->scroll;
    MenuEntry* rows = slot->entries_b;

    if (slot->field_0x002 == 0) {
        if (slot->field_0x23B != 0) {
            slot->field_0x23B--;
        }
    } else {
        if (slot->field_0x23B < 5) {
            slot->field_0x23B++;
        }
    }
    switch (slot->field_0x002) {
    case 0:
        if (buttons & 0x20) {
            result = 2;
        } else if (buttons & 0x10) {
            slot->field_0x002 = 1;
            sysSE_req(27);
        } else {
            menu_row_list_scroll(slot, buttons);
        }
        break;
    case 1:
        if (buttons & 0x20) {
            slot->field_0x002 = 0;
            sysSE_req(1);
        } else if (buttons & 0x10) {
            slot->field_0x002 = 2;
            slot->entry_count_a = 2;
            sysSE_req(5);
        } else {
            menu_row_list_scroll(slot, buttons);
        }
        break;
    case 2:
        if (buttons & 0x20) {
            slot->field_0x002 = 1;
            sysSE_req(1);
        } else if (buttons & 0x10) {
            if (slot->field_0x01A == 0) {
                slot->sub_cursor_0x01D = (s8)slot->scroll.row;
                sysSE_req(14);
                slot->field_0x002 = 3;
            } else {
                menu_row_clear(slot->scroll.row);
                menu_row_list_build(scroll, rows);
                sysSE_req(0);
                slot->field_0x002 = 1;
            }
        } else {
            if (buttons & 3) {
                slot->field_0x01A = (s8)menu_cursor_step(slot->field_0x01A, (s8&)slot->entry_count_a, buttons, 1, 2);
            }
            menu_row_list_scroll(slot, buttons);
        }
        break;
    case 3:
        if (buttons & 0x20) {
            slot->scroll.row = slot->field_0x01D;
            menu_row_list_build(scroll, rows);
            sysSE_req(1);
            slot->field_0x002 = 1;
        } else if (buttons & 0x10) {
            MenuRowData* data = userdata_item_rows_get();
            MenuRowData held;

            place_rec_copy(&held, &data[slot->sub_cursor_0x01D]);
            place_rec_copy(&data[slot->sub_cursor_0x01D], &data[slot->scroll.row]);
            place_rec_copy(&data[slot->scroll.row], &held);
            slot->scroll.row = slot->field_0x01D;
            menu_row_list_build(scroll, rows);
            sysSE_req(15);
            slot->field_0x002 = 1;
        } else {
            menu_row_list_scroll(slot, buttons);
        }
        break;
    }
    return result;
}

/* 0x80348FD4 (0xAC): opens the monster tab over the marked monsters of the 35-row table. */
extern "C" void menu_monster_list_open(MenuSlot* slot)
{
    MenuRowRec* rec = (MenuRowRec*)menu_row_table_get();
    u16 count = 0;
    s32 i;

    for (i = 0; i < 35; i++, rec++) {
        if (monster_mark_ck(rec->kind) != 0) {
            count++;
        }
    }
    if (count == 0) {
        count = 1;
    }
    menu_scroll_init(&slot->scroll, 0, count, 8);
    menu_monster_list_build(&slot->scroll, slot->entries_b);
    slot->field_0x002 = 0;
}

/* 0x80349080 (0x104): steps the monster tab. */
extern "C" s32 menu_monster_list_step(MenuSlot* slot)
{
    s32 result = 0;
    u16 buttons = slot->buttons_trig | (slot->field_0x008 & 0xF);

    if (slot->field_0x002 == 0) {
        if (slot->field_0x23B != 0) {
            slot->field_0x23B--;
        }
    } else {
        if (slot->field_0x23B < 5) {
            slot->field_0x23B++;
        }
    }
    switch (slot->field_0x002) {
    case 0:
        if (buttons & 0x20) {
            result = 2;
        } else if (buttons & 0x10) {
            slot->field_0x002 = 1;
            sysSE_req(27);
        } else {
            item_page_scroll_rows(&slot->scroll, slot->entries_b, buttons);
        }
        break;
    case 1:
        if (buttons & 0x20) {
            slot->field_0x002 = 0;
            sysSE_req(1);
        } else {
            item_page_scroll_rows(&slot->scroll, slot->entries_b, buttons);
        }
        break;
    }
    return result;
}

/* 0x80349184 (0xA4): opens the combination tab over the recipes the player knows. */
extern "C" void menu_combo_list_open(MenuSlot* slot)
{
    u16 count = 0;
    s32 i;

    for (i = 0; i < 131; i++) {
        if (lb_combo_recipe_known_ck(lb_combo_pair_recipe_find((u16)i, 0), 0) != 0) {
            count++;
        }
    }
    if (count == 0) {
        count = 1;
    }
    menu_scroll_init(&slot->scroll, 0, count, 8);
    menu_combo_list_build(&slot->scroll, slot->entries_b);
    slot->field_0x002 = 0;
}

/* 0x80349228 (0x104): steps the combination tab. */
extern "C" s32 menu_combo_list_step(MenuSlot* slot)
{
    s32 result = 0;
    u16 buttons = slot->buttons_trig | (slot->field_0x008 & 0xF);

    if (slot->field_0x002 == 0) {
        if (slot->field_0x23B != 0) {
            slot->field_0x23B--;
        }
    } else {
        if (slot->field_0x23B < 5) {
            slot->field_0x23B++;
        }
    }
    switch (slot->field_0x002) {
    case 0:
        if (buttons & 0x20) {
            result = 2;
        } else if (buttons & 0x10) {
            slot->field_0x002 = 1;
            sysSE_req(27);
        } else {
            menu_combo_list_scroll(&slot->scroll, slot->entries_b, buttons);
        }
        break;
    case 1:
        if (buttons & 0x20) {
            slot->field_0x002 = 0;
            sysSE_req(1);
        } else {
            menu_combo_list_scroll(&slot->scroll, slot->entries_b, buttons);
        }
        break;
    }
    return result;
}

/* 0x8034932C (0x6C): opens the note tab on its first level. */
extern "C" void menu_note_list_open(MenuSlot* slot)
{
    slot->field_0x002 = 0;
    slot->sub_cursor_0x01D = -1;
    (s8&)slot->field_0x01E = -1;
    (s8&)slot->field_0x01F = -1;
    slot->kind_row_base[0] = 0;
    slot->kind_row_base[1] = 3;
    slot->field_0x2F5 = 0;
    menu_scroll_init(&slot->scroll, 0, 3, 8);
    item_page_fill_rows(slot);
}

/* 0x80349398 (0x57C): steps the note tab: three list levels, then the option card a leaf row opens. */
extern "C" s32 menu_note_list_step(MenuSlot* slot)
{
    s32 result = 0;
    u16 buttons = slot->buttons_trig | (slot->field_0x008 & 0xF);
    MenuEntry* entry = &slot->entries_b[slot->scroll.row];
    MenuRowPairWord leaf;

    switch (slot->field_0x002) {
    case 0:
        if (buttons & 0x10) {
            if (entry->field_0x02 == 0) {
                sysSE_req(2);
            } else {
                MenuRowPairWord bounds;

                slot->field_0x002 = 1;
                slot->field_0x2F5 = 1;
                sysSE_req(27);
                slot->sub_cursor_0x01D = (s8)(slot->scroll.row + slot->scroll.rows_per_page * slot->scroll.col);
                bounds.raw = (u16)(note_slot_cursor_bounds(0, (u8)(slot->field_0x01D + slot->kind_row_base[0])) >> 16);
                note_slot_pair_copy((MenuRowPair*)&slot->kind_row_base[2], &bounds.pair);
                menu_scroll_init(&slot->scroll, 0, slot->kind_row_base[3], 7);
                item_page_fill_rows(slot);
            }
        } else if (buttons & 0x20) {
            result = 2;
        } else {
            item_page_update_rows(slot, buttons);
        }
        break;
    case 1:
        if (buttons & 0x10) {
            if (entry->field_0x02 == 0) {
                sysSE_req(2);
            } else {
                MenuRowPairWord bounds;

                slot->field_0x002 = 2;
                slot->field_0x2F5 = 2;
                sysSE_req(27);
                (s8&)slot->field_0x01E = (s8)(slot->scroll.row + slot->scroll.rows_per_page * slot->scroll.col);
                bounds.raw = (u16)(note_slot_cursor_bounds(1, (u8)(slot->field_0x01E + slot->kind_row_base[2])) >> 16);
                note_slot_pair_copy((MenuRowPair*)&slot->kind_row_base[4], &bounds.pair);
                menu_scroll_init(&slot->scroll, 0, slot->kind_row_base[5], 7);
                item_page_fill_rows(slot);
            }
        } else if (buttons & 0x20) {
            slot->field_0x002 = 0;
            slot->field_0x2F5 = 0;
            sysSE_req(1);
            menu_scroll_init(&slot->scroll, slot->sub_cursor_0x01D, slot->kind_row_base[1], slot->kind_row_base[1]);
            item_page_fill_rows(slot);
        } else {
            item_page_update_rows(slot, buttons);
        }
        break;
    case 2:
        if (buttons & 0x10) {
            if (entry->field_0x02 == 0) {
                sysSE_req(2);
            } else {
                MenuRowPairWord bounds;
                u8 row;

                sysSE_req(27);
                row = slot->scroll.row + slot->scroll.rows_per_page * slot->scroll.col;
                bounds.raw = (u16)(note_slot_cursor_bounds(2, (u8)(row + slot->kind_row_base[4])) >> 16);
                note_slot_pair_copy((MenuRowPair*)&slot->kind_row_base[6], &bounds.pair);
                if (slot->kind_row_base[6] == 0xFF) {
                    slot->field_0x002 = 10;
                    (s8&)slot->field_0x01F = (s8)row;
                    slot->scroll.light_colour = 0;
                    slot->scroll.dark_colour = (s8)slot->kind_row_base[7];
                } else {
                    slot->field_0x002 = 3;
                    slot->field_0x2F5 = 3;
                    (s8&)slot->field_0x01F = (s8)row;
                    menu_scroll_init(&slot->scroll, 0, slot->kind_row_base[7], 7);
                    item_page_fill_rows(slot);
                }
            }
        } else if (buttons & 0x20) {
            slot->field_0x002 = 1;
            slot->field_0x2F5 = 1;
            sysSE_req(1);
            menu_scroll_init(&slot->scroll, (s8&)slot->field_0x01E, slot->kind_row_base[3], 7);
            item_page_fill_rows(slot);
        } else {
            item_page_update_rows(slot, buttons);
        }
        break;
    case 3:
        if (buttons & 0x10) {
            if (entry->field_0x02 == 0) {
                sysSE_req(2);
            } else {
                u8 row;

                sysSE_req(27);
                row = slot->scroll.row + slot->scroll.rows_per_page * slot->scroll.col;
                leaf.raw = (u16)(note_slot_cursor_bounds(3, (u8)(row + slot->kind_row_base[6])) >> 16);
                if (leaf.pair.first == 0xFF) {
                    slot->field_0x002 = 10;
                    slot->scroll.light_colour = 0;
                    slot->scroll.dark_colour = (s8)leaf.pair.count;
                }
            }
        } else if (buttons & 0x20) {
            slot->field_0x002 = 2;
            slot->field_0x2F5 = 2;
            sysSE_req(1);
            menu_scroll_init(&slot->scroll, (s8&)slot->field_0x01F, slot->kind_row_base[5], 7);
            item_page_fill_rows(slot);
        } else {
            item_page_update_rows(slot, buttons);
        }
        break;
    case 10:
        if (buttons & 0x20) {
            ainpc_entry_kill_kind(1);
            slot->field_0x002 = slot->field_0x2F5;
            sysSE_req(1);
        } else {
            s8 page = slot->scroll.light_colour;
            s8 option;

            if (buttons & 4) {
                page--;
                if (page < 0) {
                    page = 0;
                    sysSE_req(2);
                    slot->field_0x008 &= ~4;
                } else {
                    sysSE_req(6);
                }
            } else if (buttons & 8) {
                page++;
                if (page >= slot->scroll.dark_colour) {
                    page = slot->scroll.dark_colour - 1;
                    sysSE_req(2);
                    slot->field_0x008 &= ~8;
                } else {
                    sysSE_req(6);
                }
            }
            if (page != slot->scroll.light_colour) {
                slot->scroll.light_colour = page;
            }
            option = item_page_option_row_index(slot->field_0x2F5,
                (u8)(slot->kind_row_base[slot->field_0x2F5 * 2] +
                     (slot->scroll.row + slot->scroll.rows_per_page * slot->scroll.col)), page);
            if (option >= 0) {
                item_page_option_available(option);
            }
        }
        break;
    }
    return result;
}

/* 0x80349914 (0x14): copies a row pair byte by byte. */
extern "C" void note_slot_pair_copy(MenuRowPair* dst, const MenuRowPair* src)
{
    dst->first = src->first;
    dst->count = src->count;
}

/* 0x80349928 (0x16C): fills the visible rows of the item-row list from the save's row records. */
extern "C" void menu_row_list_build(MenuScroll* scroll, MenuEntry* rows)
{
    MenuRowData* data;
    s32 index = scroll->rows_per_page * scroll->col;
    s32 i;

    data = userdata_item_rows_get() + index;

    for (i = 0; i < scroll->row_limit; i++, data++) {
        rows->selected = (i == scroll->row);
        rows->field_0x06 = (s16)index;
        rows->field_0x0C = 0;
        if ((u8)data->kind != 0) {
            switch ((u8)data->kind) {
            case 1:
                rows->field_0x02 = 1;
                rows->field_0x08 = ItemName(data->item_id);
                break;
            case 2:
                rows->field_0x02 = 1;
                rows->field_0x08 = ItemName(data->item_id);
                break;
            case 3:
                rows->field_0x02 = 1;
                rows->field_0x08 = ItemName(data->equip_id);
                break;
            case 4:
                rows->field_0x02 = 1;
                rows->field_0x08 = ItemName(data->equip_id);
                break;
            case 5:
                rows->field_0x02 = 1;
                rows->field_0x08 = (u32)get_str_tbl(0x44)[3];
                break;
            case 6:
                rows->field_0x02 = 1;
                rows->field_0x08 = ItemName(data->field_0x0A);
                break;
            }
        } else {
            rows->field_0x02 = 0;
            rows->field_0x08 = ItemName(0);
        }
        rows++;
        index++;
    }
}

/* 0x80349A94 (0x70): scrolls the item-row list and rebuilds it when the cursor moved. */
extern "C" void menu_row_list_scroll(MenuSlot* slot, u16 buttons)
{
    MenuScroll* scroll = &slot->scroll;
    MenuEntry* rows = slot->entries_b;

    if ((u32)slot->field_0x002 <= 1 || (s32)slot->field_0x002 == 3) {
        menu_scroll_step(scroll, buttons, 6);
        if (scroll->move_flags != 0) {
            menu_row_list_build(scroll, rows);
        }
    }
}

/* 0x80349B04 (0x140): fills the visible rows of the combination list with the known recipes' results. */
extern "C" void menu_combo_list_build(MenuScroll* scroll, MenuEntry* rows)
{
    s32 known = 0;
    s32 shown = 0;
    s32 first = scroll->rows_per_page * scroll->col;
    s32 i;

    for (i = 0; i < 131; i++) {
        MenuComboRecipe* recipe = (MenuComboRecipe*)lb_combo_pair_recipe_find((u16)i, 0);

        if (lb_combo_recipe_known_ck((LbComboRecipe*)recipe, 0) != 0) {
            if (first <= known && known - first < scroll->row_limit) {
                rows->field_0x02 = 1;
                rows->field_0x06 = (s16)i;
                rows->field_0x08 = ItemName(recipe->result_0x02);
                rows->selected = (shown == scroll->row);
                rows->field_0x0C = 0;
                rows++;
                shown++;
            }
            known++;
        }
    }
    if (known == 0) {
        rows->field_0x02 = 0;
        rows->field_0x06 = 0;
        rows->field_0x08 = ItemName(0);
        rows->selected = 0;
        rows->field_0x0C = 0;
        rows++;
        shown++;
    }
    while (shown < scroll->row_limit) {
        rows->field_0x02 = 0;
        rows->field_0x06 = 0;
        rows->field_0x08 = ItemName(0);
        rows->selected = 0;
        rows->field_0x0C = 0;
        rows++;
        shown++;
    }
}

/* 0x80349C44 (0x58): scrolls the combination list and rebuilds it when the cursor moved. */
extern "C" void menu_combo_list_scroll(MenuScroll* scroll, MenuEntry* rows, u16 buttons)
{
    menu_scroll_step(scroll, buttons, 6);
    if (scroll->move_flags != 0) {
        menu_combo_list_build(scroll, rows);
    }
}

/* 0x80349C9C (0x13C): fills the visible rows of the monster list with the marked monsters' names. */
extern "C" void menu_monster_list_build(MenuScroll* scroll, MenuEntry* rows)
{
    s32 first = scroll->rows_per_page * scroll->col;
    s32 marked = 0;
    s32 shown = 0;
    s32 i;
    MenuRowRec* rec = (MenuRowRec*)menu_row_table_get();

    for (i = 0; i < 35; i++, rec++) {
        if (monster_mark_ck(rec->kind) != 0) {
            if (first <= marked && marked - first < scroll->row_limit) {
                rows->field_0x02 = 1;
                rows->field_0x08 = (u32)str_tbl_33_get(rec->kind);
                rows->selected = (shown == scroll->row);
                rows->field_0x06 = (s16)i;
                rows->field_0x0C = 0;
                rows++;
                shown++;
            }
            marked++;
        }
    }
    if (marked == 0) {
        rows->field_0x02 = 0;
        rows->field_0x08 = ItemName(0);
        rows->selected = 0;
        rows->field_0x06 = 0;
        rows->field_0x0C = 0;
        rows++;
        shown++;
    }
    while (shown < scroll->row_limit) {
        rows->field_0x02 = 0;
        rows->field_0x08 = ItemName(0);
        rows->selected = 0;
        rows->field_0x06 = 0;
        rows->field_0x0C = 0;
        rows++;
        shown++;
    }
}
