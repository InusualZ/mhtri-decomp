/*
 * `menu/menu_item_page.cpp` - the menu's item page draw layer: `.text` 0x80349DD8-0x8034C0C4 (22
 * functions, 8940 B).
 *
 * MODULE AND NAME.  Module `menu`: the unit's own fragments sit inside the menu band's data - `.data`
 * 0x805E91E8-0x805E91F8 and `.sdata` 0x807932F0-0x80793308 - immediately below the `menu_note.cpp`
 * `__FILE__` string at 0x805E91F8, which the next TU's single function (0x8034C0C4) is the sole
 * referencer of; its calls are the menu UI helpers (`get_str_tbl`, `get_menu_lsp_tbl`,
 * `ItemName`/`ItemExp`, `font_print_ex`, `draw_sprite_anim_ary`).  No `__FILE__` string covers the
 * range (the only referencer of the string at this data offset is the 0x8034C0C4 TU) and
 * `dumpmap.py lookup` answers `zz_` for every address here, so the file name and the symbol names
 * are the naming pass's own (2026-09-27), each derived from its body and the band's `menu_*` file
 * family (`menu_item.cpp`, `menu_infomation.cpp`, `menu_note.cpp`): the page's scroll/fill/update
 * trio, its per-frame draw, and the per-page column/detail pair the `switch (page_index)` arms of
 * `item_page_draw` select (`item_page_draw_column0`/`item_page_draw_detail0` .. `3`).  MARKED GUESS:
 * what each page number shows (0 = the item's detail panel, 1 = the row's damage/quality block,
 * 2 = the item card, 3 = the option/season card) is read off the bodies' call surface, not off a
 * name; a later pass with the screen's own symbols can sharpen it.
 *
 * THE RECORD IT DRAWS.  `MenuSlot` (`include/menu/menu_item.h`, the landed half's view): callers hand
 * it `&slot->entries_b[0]` (0x6C) or the embedded `MenuScroll` at +0x1A4.  This unit is why
 * `MenuEntry`'s +0x06/+0x0C, `MenuSlot`'s +0x01C..+0x01F, the `MenuScroll` record and the +0x2ED kind
 * table are named there (one definition, rule 1 - the extension was proven inert:
 * `build/RMHE08/src/menu/menu_item.o` is byte-identical before and after it).
 *
 * Naming note: the `fn_XXXXXXXX` names this file carries are all *references* to other units'
 * symbols (`fn_802E0AD4`, `fn_8004EA24`, `fn_8033ADD0`, `get_menu_lsp_tbl`'s band, ...) which the map
 * still spells as stems and which are not this batch's to rename; every one of this file's own 22
 * symbols is named above.  The shape is the one `hud/fn_80334568.cpp` and `menu/fn_8031EA8C.cpp`
 * record for the same case (the gate's `rule7_defer_growth` refuses only a *defined* generated name
 * behind the escape, and this file defines none).
 *
 * SECTIONS.  All four non-`.text` sections are byte-identical to the target object (`datagap.py`
 * reports no data gap): `.data` 0x805E91E8-0x805E91F8 (the four page colour words
 * `item_page_colour_table`, defined here) and `.sdata` 0x807932F0-0x80793308 (the pooled `"%d"`,
 * `"%s"`, `"%6.1f"`, `"%d%"` literals, the `s16` line gap `item_page_line_gap` and their padding -
 * the pool order is the source's function order, which is why the bodies sit in address order).
 * extab/extabindex are claimed with the target's exact ranges.  `.text` is 88 B larger than the
 * target's: `item_page_draw_detail1` +60, `item_page_draw_detail0` +24, `item_page_draw_detail2` +12,
 * `item_page_option_value_string` +8, `item_page_item_price` +4, `item_page_draw_closed_column` +4
 * against `item_page_draw` -20 and `item_page_draw_column3` -4.
 *
 * RESIDUALS (measured with `recompile.py --measure` / the official report, 2026-09-27; the unit's own
 * score is 93.29 fuzzy / 1088 of 8940 code bytes, and the naming pass plus the width fixes below moved
 * it 92.58 -> 93.29 with no row lower):
 *   * 7 functions are byte-identical: `item_page_scroll_rows`, `item_page_fill_rows`,
 *     `item_page_update_rows`, `item_page_draw_page_widget`, `item_page_draw_column1`,
 *     `item_page_draw_column2`, `item_page_option_available` (100.0).
 *   * `item_page_draw_page_arrow` 98.98: two callee-saved registers are swapped against retail (`pos`
 *     r31 vs r30) and the colour `or` keeps retail's other operand order - four source shapes tried
 *     (inline `id - 30`, a `u16`/`u32` `part`, both declaration orders), none flips it.
 *   * `item_page_draw` 95.89 (916/936 B): the `case 1` arm's `else` body and the `page_ofs` test land
 *     in a different block order (5 instructions); three shapes tried.
 *   * `item_page_draw_detail0` 96.49 (1612/1588) - the `MenuRowData` record this unit owns declares
 *     `kind` as `s32` on purpose: retail's `switch (data->kind)` is a signed binary search
 *     (`cmpwi`/`bge`), and with `u32` MWCC emits an unsigned linear chain instead (94.92 with the
 *     fix reverted, and the whole arm bodies shift with it).  `item_page_draw_detail1` 91.12
 *     (1176/1116), `item_page_draw_detail2` 82.89 (604/592), `item_page_draw_column3` 88.22 (516/520):
 *     the arm bodies' reload/step order differs by 1-15 instructions; the call sequences are
 *     identical, so this is in-block scheduling/colouring, not missing code.
 *   * `item_page_draw_rows` 87.58 / `item_page_draw_blank_rows` 93.70: the row draws' argument
 *     narrowing (`(u16)` on the id and the `s16` step) - same size, instruction-order and masking
 *     differences.  `item_page_draw_rows`'s target also narrows its `first` parameter with `extsb`
 *     where our `u8` gives `clrlwi`, and sets `row_id` up *after* the first `fn_802E0AD4` call where
 *     we set it up before: a signed `first` and a reordered declaration are the next two things to
 *     try there (not tried - the row's remaining diff is dominated by the block order).
 *   * `item_page_item_price` 97.5 (260/256 B): the return and callee widths are load-bearing - the
 *     target masks NOTHING on the three `fn_8004B*` results or on `fn_8026FE44`'s compare, so this
 *     unit's view declares `item_page_item_price` and `fn_8004B0A4`/`item_count_find`/`fn_8004B70C` as
 *     `u32` and `fn_8026FE44` as `u32`, and the price chain is two statements
 *     (`base = fn_802731B4(worker, id); base += fn_8004B70C(...);`) so the worker call comes first
 *     like retail's.  That took the row 81.09 -> 97.5; the residual is one instruction, the mask the
 *     third argument's narrowing emits after `fn_8004AE70` instead of before the other two argument
 *     setups (retail masks into r5 immediately, we copy to r0 and mask at the call).  Three shapes
 *     tried on that one (inline call, `u16` and `u32` temporaries), none flips it.
 *   * `item_page_draw_wrapped_text` 96.32 / `item_page_draw_column0` 96.36 /
 *     `item_page_draw_closed_column` 95.57 / `item_page_draw_option_frame` 94.85 /
 *     `item_page_option_row_index` 90.10 / `item_page_option_value_string` 84.46: the same in-block
 *     ordering/masking class.
 */

#include "types.h"
#include "menu/menu_item.h"
#include "menu/menu_item_page.h"

/* This unit's own view of the 0x24-byte sprite-data record `fn_802E0AD4` fills in and the `draw_*`
 * family takes by reference (`_SPR_DATA_` in the map's manglings): `include/hud/layout.h` owns the full
 * definition and `include/unsplit/lobby.h` keeps the tag incomplete, and including the owner's header
 * here collides with the lobby band's `_mh_ivec2_`/`get_lsp_data` views.  The record starts with the
 * anchor the draws read (`&spr.pos` IS the `_SPR_DATA_`'s address) and the row colour is at +0x1C. */
typedef struct SprWork {
    /* +0x00 */ _mh_ivec2_ pos;      /* the record's own anchor, added to the caller's position */
    /* +0x04 */ s16 ofs_x;
    /* +0x06 */ s16 ofs_y;
    /* +0x08 */ s16 width;            /* the vertex rectangle `item_page_draw_wrapped_text` shifts by */
    /* +0x0A */ s16 height;
    /* +0x0C */ u8 unused_0x0C[0x1C - 0x0C];
    /* +0x1C */ u32 colour_0x1C;
    /* +0x20 */ u8 unused_0x20[0x24 - 0x20];
} SprWork; /* size: 0x24 */

/* The four page colour words `item_page_draw_page_widget` tints the widget's rows with.  This TU's own `.data`
 * (0x805E91E8-0x805E91F8, 16 B, claimed in `splits.txt`); `grep -rl item_page_colour_table` over the split
 * objects finds this unit's function as the only referencer. */
u32 item_page_colour_table[4] = {0x704A34E6, 0x787D28E6, 0x467873E6, 0x3C7291E6};

/* 0x80349DD8: re-runs the row build for a scroll record the caller just scrolled, if it moved. */
extern "C" void item_page_scroll_rows(MenuScroll* scroll, MenuEntry* rows, u16 buttons)
{
    fn_802A9368(scroll, buttons, 6);
    if (scroll->move_flags != 0) {
        fn_80349C9C(scroll, rows);
    }
}

/* 0x80349E30: fills the visible item rows (the entry array's first `row_limit` cells) with the string
 * table row each inventory slot names, marking the cursor's row. */
extern "C" void item_page_fill_rows(MenuSlot* slot)
{
    MenuEntry* row = slot->entries_b;
    s32 first;
    u8 kind = slot->field_0x2F5;
    s32 i;

    first = slot->kind_row_base[kind * 2] + slot->scroll.rows_per_page * slot->scroll.col;

    for (i = 0; i < slot->scroll.row_limit; i++) {
        row->selected = (i == slot->scroll.row);
        row->field_0x06 = (s16)first;
        row->field_0x0C = 0;
        if (item_page_option_item_id(kind, (u8)first) > 0) {
            row->field_0x02 = 1;
            switch (kind) {
            case 0:
                row->field_0x08 = (u32)get_str_tbl(0x45)[first];
                break;
            case 1:
                row->field_0x08 = (u32)get_str_tbl(0x46)[first];
                break;
            case 2:
                row->field_0x08 = (u32)get_str_tbl(0x47)[first];
                break;
            case 3:
                row->field_0x08 = (u32)get_str_tbl(0x48)[first];
                break;
            default:
                row->field_0x02 = 0;
                row->field_0x08 = ItemName(0);
                break;
            }
        } else {
            row->field_0x02 = 0;
            row->field_0x08 = ((u32*)get_str_tbl(0x44))[1];
        }
        row++;
        first++;
    }
}

/* 0x80349F8C: the per-frame row build - scroll the record with the frame's button word, then refill
 * the rows when it moved, unless the slot is in the closed phase (0xA). */
extern "C" void item_page_update_rows(MenuSlot* slot, u16 buttons)
{
    MenuScroll* scroll = &slot->scroll;

    if (slot->field_0x002 != 0xA) {
        fn_802A9368(scroll, buttons, 6);
        if (scroll->move_flags != 0) {
            item_page_fill_rows(slot);
        }
    }
}

/* 0x80349FF0: one frame of the item page - the page's own sprite rows, the per-page detail draw, the
 * four arrow sprites with the selected one, and the page widget's number. */
extern "C" void item_page_draw(MenuSlot* slot)
{
    SprWork back;
    SprWork panel;
    SprWork page;
    SprWork nums;
    _mh_ivec2_ posBack;
    _mh_ivec2_ posPanel;
    _mh_ivec2_ posPage;
    _mh_ivec2_ posNums;
    s32 draw = 1;
    s32 arrow = 0;
    u8 page_ofs = slot->page_index;

    set_blendmode(4, 5, 1);
    switch (slot->field_0x001) {
    case 0:
        if (slot->field_0x00F == 1 && chk_pointer() != 0) {
            draw = 0;
        }
        if (chk_pointer() != 0) {
            page_ofs = 0xFF;
        }
        break;
    case 1:
        if ((u32)((s8)slot->page_index - 1) > 1) {
            if ((s8)slot->page_index != 3) {
                break;
            }
            if (slot->field_0x002 == 0xA) {
                item_page_draw_column3(slot);
                draw = 0;
            } else {
                arrow = 1;
            }
        } else {
            arrow = 1;
        }
        break;
    }
    if (draw == 0) {
        return;
    }
    fn_802E0AD4((_SPR_DATA_*)&back, 0x3FA, slot->field_0x23A, &posBack);
    fn_802E0AD4((_SPR_DATA_*)&panel, 0x406, slot->field_0x23A, &posPanel);
    draw_sprite_anim_ary((const u16*)get_menu_lsp_tbl(0x46), slot->field_0x23A, &posBack);
    if (slot->field_0x001 == 1) {
        switch ((s8)slot->page_index) {
        case 0:
            item_page_draw_detail0(slot);
            break;
        case 1:
            item_page_draw_detail1(slot);
            break;
        case 2:
            item_page_draw_detail2(slot);
            break;
        }
    }
    draw_sprite_anim_ary((const u16*)get_menu_lsp_tbl(0x47), slot->field_0x23A, &posBack);
    draw_sprite_anim_idx(0x400, slot->field_0x23A, &posBack);
    item_page_draw_page_arrow((u16*)get_menu_lsp_tbl(0x4B), (u16)slot->field_0x23A, page_ofs - 3 == 0, &posPanel);
    draw_sprite_anim_idx(0x3FF, slot->field_0x23A, &posBack);
    item_page_draw_page_arrow((u16*)get_menu_lsp_tbl(0x4A), (u16)slot->field_0x23A, page_ofs - 2 == 0, &posPanel);
    draw_sprite_anim_idx(0x3FE, slot->field_0x23A, &posBack);
    item_page_draw_page_arrow((u16*)get_menu_lsp_tbl(0x49), (u16)slot->field_0x23A, page_ofs - 1 == 0, &posPanel);
    draw_sprite_anim_idx(0x3FD, slot->field_0x23A, &posBack);
    item_page_draw_page_arrow((u16*)get_menu_lsp_tbl(0x48), slot->field_0x23A, page_ofs == 0, &posPanel);
    if (slot->field_0x23A > 0x1E) {
        fn_802E0AD4((_SPR_DATA_*)&page, 0x41F, slot->field_0x23A, &posPage);
        draw_sprite_anim_ary((const u16*)get_menu_lsp_tbl(0x4C), slot->field_0x23A, &posPage);
        fn_802E0AD4((_SPR_DATA_*)&nums, 0x438, slot->field_0x23A, &posNums);
        item_page_draw_page_widget((s8)slot->page_index, &posNums);
        if (arrow != 0) {
            PutPageArrow((u16*)get_menu_lsp_tbl(0x4F), (s16)slot->scroll.col, (s16)slot->scroll.col_count,
                         slot->scroll.move_flags, &posNums, 1);
        }
    }
    if (slot->field_0x23A > 0x24) {
        switch ((s8)slot->page_index) {
        case 0:
            item_page_draw_column0(slot);
            break;
        case 1:
            item_page_draw_column1(slot);
            break;
        case 2:
            item_page_draw_column2(slot);
            break;
        case 3:
            item_page_draw_column3(slot);
            break;
        }
    }
    get_lsp_data(0x3F1, &posPage);
    draw_sprite_anim_ary((const u16*)get_menu_lsp_tbl(0x45), slot->field_0x23A, &posPage);
}

/* 0x8034A398: draws one of the four page arrows - a plain row when `flag` is clear, else the frame
 * row shifted by the page's own animation phase plus the arrow glyph, tinted. */
extern "C" void item_page_draw_page_arrow(u16* table, u32 id, s32 flag, _mh_ivec2_* pos)
{
    s32 part;
    SprWork spr;

    if (flag == 0) {
        draw_sprite_anim_ary(table, id, pos);
        return;
    }
    part = id - 30;
    draw_sprite_anim_idx(table[0], part, pos);
    fn_802E0AD4((_SPR_DATA_*)&spr, table[1], part, NULL);
    fn_802D9EA8();
    spr.colour_0x1C = (spr.colour_0x1C & 0xFF) | (fn_802E270C(spr.colour_0x1C, 0xFFD246FF) & 0xFFFFFF00);
    draw_sprite(*(_SPR_DATA_*)&spr, pos);
}

/* 0x8034A448: the page widget's row - one sprite per row of the page's own table, tinted with the
 * per-page colour word, then the widget's frame and this page's own id. */
extern "C" void item_page_draw_page_widget(s8 page, _mh_ivec2_* pos)
{
    SprWork spr;
    u16* row = (u16*)get_menu_lsp_tbl(0x4D);

    while (*row != 0xFFFF) {
        fn_801E6850((s16*)&spr, get_lsp_data(*row, NULL));
        spr.colour_0x1C = item_page_colour_table[page];
        draw_sprite(*(_SPR_DATA_*)&spr, pos);
        row++;
    }
    draw_sprite_idx(0x446, pos);
    draw_sprite_idx(((u16*)get_menu_lsp_tbl(0x4E))[page], pos);
}

/* 0x8034A4FC: draws one column of rows - each row's sprite, the selected row's highlight and its text,
 * walking the entry array, the sprite table and the row ids together. */
extern "C" void item_page_draw_rows(MenuEntry* entries, u8 sel, u8 count, u8 id, u8 id2, u8 first, u8 blend)
{
    SprWork row;
    SprWork text;
    _mh_ivec2_ ofs;
    u16* row_id = &((u16*)get_menu_lsp_tbl(0x52))[first];
    s32 i;

    fn_802E0AD4((_SPR_DATA_*)&row, 0x438, id, &ofs);
    for (i = 0; i < count; i++) {
        fn_802E0AD4((_SPR_DATA_*)&row, *row_id, id, NULL);
        row.pos.x += ofs.x;
        row.pos.y += ofs.y;
        draw_sprite_anim_ary((const u16*)get_menu_lsp_tbl(0x50), id, &row.pos);
        if (i == sel && chk_pointer() == 0) {
            draw_sprite_anim_ary((const u16*)get_menu_lsp_tbl(0x53), id2, &row.pos);
        }
        fn_802E0AD4((_SPR_DATA_*)&text, 0x435, id, NULL);
        if (blend == i) {
            text.colour_0x1C = 0xB45511FF;
        }
        if (entries->selected != 0) {
            text.colour_0x1C = 0xBE5F1BFF;
        }
        draw_font(*(_SPR_DATA_*)&text, (s8*)entries->field_0x08, 0, &row.pos);
        row_id++;
        i++;
        entries++;
    }
}

/* 0x8034A65C: the blank-row variant of the column draw above - rows whose entry is empty are skipped
 * (except the first, which prints the table's own text) and the sprite walk stays on the table. */
extern "C" void item_page_draw_blank_rows(MenuEntry* entries, u8 sel, u8 count, u8 id, u8 id2)
{
    SprWork row;
    SprWork text;
    _mh_ivec2_ ofs;
    u16* row_id = (u16*)get_menu_lsp_tbl(0x52);
    s32 i;

    fn_802E0AD4((_SPR_DATA_*)&row, 0x438, id, &ofs);
    for (i = 0; i < count; i++) {
        if (i == 0 || entries->field_0x02 != 0) {
            fn_802E0AD4((_SPR_DATA_*)&row, *row_id, id, NULL);
            row.pos.x += ofs.x;
            row.pos.y += ofs.y;
            draw_sprite_anim_ary((const u16*)get_menu_lsp_tbl(0x50), id, &row.pos);
            if (i == sel && chk_pointer() == 0) {
                draw_sprite_anim_ary((const u16*)get_menu_lsp_tbl(0x53), id2, &row.pos);
            }
            if (i == 0 && entries->field_0x02 == 0) {
                u8** text_tbl = get_str_tbl(0x44);
                fn_802E0AD4((_SPR_DATA_*)&text, 0x435, id, NULL);
                if (entries->selected != 0) {
                    text.colour_0x1C = 0xBE5F1BFF;
                }
                draw_font(*(_SPR_DATA_*)&text, (s8*)*text_tbl, 0, &row.pos);
            } else {
                fn_802E0AD4((_SPR_DATA_*)&text, 0x435, id, NULL);
                if (entries->selected != 0) {
                    text.colour_0x1C = 0xBE5F1BFF;
                }
                draw_font(*(_SPR_DATA_*)&text, (s8*)entries->field_0x08, 0, &row.pos);
            }
        }
        row_id++;
        entries++;
    }
}

/* 0x8034A814: the shown item's price - the player's own start-of-quest money for the two special
 * modes, else the item's shop value plus its two modifiers. */
extern "C" u32 item_page_item_price(MenuSlot* slot, u16 id)
{
    s32 worker = (s32)slot->worker;
    s32 base;

    if (fn_800CF208() == 2) {
        return fn_8004B0A4(id, (void*)lbl_80794880);
    }
    base = fn_802731B4(worker, id);
    base += fn_8004B70C(id, (void*)&lbl_80794880->field_0x0180, fn_8004AE70((void*)lbl_80794880));
    if (fn_8026FE44(worker) == 1) {
        return base + item_count_find(id, fn_8004AF60((void*)lbl_80794880, 0), fn_8004AF0C(0));
    }
    return base + item_count_find(id, fn_8004AF60((void*)lbl_80794880, 1), fn_8004AF0C(1));
}

/* 0x8034A914: the page-0 detail panel - the panel frame, then the row record's own fields rendered
 * as the name/quality/price block the item kind selects. */
extern "C" void item_page_draw_detail0(MenuSlot* slot)
{
    SprWork spr;
    MenuRowData* data;
    _mh_ivec2_ pos;
    s8 text[0x10];
    u8 id;

    if (slot->field_0x002 != 0 || slot->field_0x23B != 0) {
        id = slot->field_0x23B;
        fn_802E0AD4((_SPR_DATA_*)&spr, 0x4D9, id, &pos);
        draw_sprite_anim_ary((const u16*)get_menu_lsp_tbl(0x57), id, &pos);
        draw_sprite_anim_ary((const u16*)get_menu_lsp_tbl(0x58), id, &pos);
        data = &fn_8004EA24()[slot->scroll.row];
        switch (data->kind) {
        case 1:
        case 2:
            fn_802E23D0(0x4EA, id, (s8*)ItemName(data->item_id), 4, &pos);
            fn_802E23D0(0x4FC, id, (s8*)GetEquipName((u8)data->equip_kind, data->equip_id), 0, &pos);
            sprintf(text, "%d", data->field_0x0A);
            fn_802E23D0(0x4F7, id, text, 2, &pos);
            sprintf(text, "%d", item_page_item_price(slot, data->item_id));
            fn_802E23D0(0x4F8, id, text, 2, &pos);
            fn_802E0AD4((_SPR_DATA_*)&spr, 0x4F0, id, NULL);
            font_set_size(spr.width, spr.height);
            flfntSetColor(spr.colour_0x1C);
            font_print_ex(pos.x + spr.pos.x, pos.y + spr.pos.y, -1, "%s", ItemExp(data->item_id));
            break;
        case 3:
            fn_802E23D0(0x4EA, id, (s8*)ItemName(data->equip_id), 4, &pos);
            fn_802E23D0(0x4FC, id, (s8*)ItemName(data->equip_kind), 0, &pos);
            sprintf(text, "%d", data->item_id);
            fn_802E23D0(0x4F7, id, text, 2, &pos);
            sprintf(text, "%d", item_page_item_price(slot, data->equip_id));
            fn_802E23D0(0x4F8, id, text, 2, &pos);
            fn_802E0AD4((_SPR_DATA_*)&spr, 0x4F0, id, NULL);
            font_set_size(spr.width, spr.height);
            flfntSetColor(spr.colour_0x1C);
            font_print_ex(pos.x + spr.pos.x, pos.y + spr.pos.y, -1, "%s", ItemExp(data->equip_id));
            break;
        case 4:
            fn_802E23D0(0x4EA, id, (s8*)ItemName(data->equip_id), 4, &pos);
            fn_802E23D0(0x4FC, id, (s8*)((u8**)get_str_tbl(0x44))[5], 0, &pos);
            sprintf(text, "%d", data->item_id);
            fn_802E23D0(0x4F7, id, text, 2, &pos);
            sprintf(text, "%d", item_page_item_price(slot, data->equip_id));
            fn_802E23D0(0x4F8, id, text, 2, &pos);
            fn_802E0AD4((_SPR_DATA_*)&spr, 0x4F0, id, NULL);
            font_set_size(spr.width, spr.height);
            flfntSetColor(spr.colour_0x1C);
            font_print_ex(pos.x + spr.pos.x, pos.y + spr.pos.y, -1, "%s", ItemExp(data->equip_id));
            break;
        case 5:
            fn_802E23D0(0x4EA, id, (s8*)((u8**)get_str_tbl(0x44))[3], 4, &pos);
            fn_802E23D0(0x4FC, id, (s8*)((u8**)get_str_tbl(0x44))[5], 0, &pos);
            sprintf(text, "%d", data->item_id);
            fn_802E23D0(0x4F7, id, text, 2, &pos);
            sprintf(text, "%d", lbl_80794880->field_0x3F04);
            fn_802E23D0(0x4F8, id, text, 2, &pos);
            fn_802E23D0(0x4F0, id, (s8*)((u8**)get_str_tbl(0x44))[7], 0, &pos);
            break;
        case 6:
            fn_802E23D0(0x4EA, id, (s8*)ItemName(data->field_0x0A), 4, &pos);
            fn_802E23D0(0x4FC, id, (s8*)((u8**)get_str_tbl(0x44))[6], 0, &pos);
            sprintf(text, "%d", data->field_0x0C);
            fn_802E23D0(0x4F7, id, text, 2, &pos);
            sprintf(text, "%d", item_page_item_price(slot, data->field_0x0A));
            fn_802E23D0(0x4F8, id, text, 2, &pos);
            fn_802E0AD4((_SPR_DATA_*)&spr, 0x4F0, id, NULL);
            font_set_size(spr.width, spr.height);
            flfntSetColor(spr.colour_0x1C);
            font_print_ex(pos.x + spr.pos.x, pos.y + spr.pos.y, -1, "%s", ItemExp(data->field_0x0A));
            break;
        default:
            fn_802E23D0(0x4EA, id, (s8*)((u8**)get_str_tbl(0x44))[1], 4, &pos);
            fn_802E23D0(0x4FC, id, (s8*)((u8**)get_str_tbl(0x44))[1], 0, &pos);
            break;
        }
        font_flush();
    }
}

/* 0x8034AF48: the page-0 column draw, then - when the slot is in its "item picked" phase - the item
 * icon's own sprite shifted by the page anchor and the item name put at it. */
extern "C" void item_page_draw_column0(MenuSlot* slot)
{
    SprWork spr;
    _mh_ivec2_ pos;
    _mh_ivec2_* ofs;
    u8 blend = 0xFF;

    if (slot->page_index == 0 && slot->field_0x002 == 3) {
        blend = slot->field_0x01D;
    }
    item_page_draw_rows(slot->entries_b, slot->scroll.row, slot->scroll.row_limit, slot->field_0x23A, slot->field_0x23C,
                0, blend);
    if (slot->field_0x002 == 2) {
        fn_802E0AD4((_SPR_DATA_*)&spr, 0x4D9, slot->field_0x23B, &pos);
        ofs = (_mh_ivec2_*)get_lsp_data(0x4DA, NULL);
        pos.x += ofs->x;
        pos.y += ofs->y;
        fn_802A9F48(slot->field_0x01A, slot->entry_count_a, (s8*)fn_8029F7C4(12), &pos, 0);
    }
}

/* The line gap `item_page_draw_wrapped_text` advances each wrapped line by: `.sdata` 0x807932F8-0x807932FA (2 B),
 * this TU's own object, claimed in `splits.txt`.  `volatile` because the retail code loads it from
 * memory instead of using the immediate (the compiler propagates the initialiser otherwise). */
volatile s16 item_page_line_gap = 2;

/* 0x8034B024: puts a text with embedded newlines at the given sprite, shifting each following line
 * down by the sprite's own height plus the line gap. */
extern "C" void item_page_draw_wrapped_text(u16 id, u16 part, s8* text, const _mh_ivec2_* pos)
{
    SprWork spr;
    s8 buffer[0x200];
    s8* line = buffer;

    fn_802E0AD4((_SPR_DATA_*)&spr, id, part, NULL);
    strcpy(buffer, text);
    for (;;) {
        s8* brk = (s8*)flfntStrChr((char*)line, 10);
        if (brk == NULL) {
            draw_font(*(_SPR_DATA_*)&spr, line, 0, pos);
            break;
        } else {
            *brk = 0;
            draw_font(*(_SPR_DATA_*)&spr, line, 0, pos);
            line = brk + 1;
            spr.pos.y += spr.height + item_page_line_gap;
        }
    }
}

/* 0x8034B104: the page-0/1/2/3 list - the column's sprite rows and, when the row has a record, its
 * damage/quality block.  The three list widths share the row layout below the data switch. */
extern "C" void item_page_draw_detail1(MenuSlot* slot)
{
    SprWork spr;
    SprWork bar;
    MenuEntry* entry;
    MenuRowRec* data;
    _mh_ivec2_ pos;
    _mh_ivec2_ posBar;
    f32 value_a;
    f32 value_b;
    s8 text[0x10];
    u8 id;
    u16 limit_a;
    u16 limit_b;
    s32 i;

    if (slot->field_0x002 != 0 || slot->field_0x23B != 0) {
        id = slot->field_0x23B;
        entry = &slot->entries_b[slot->scroll.row];
        data = &((MenuRowRec*)fn_8029F808())[entry->field_0x06];
        fn_802E0AD4((_SPR_DATA_*)&spr, 0x46C, id, &pos);
        draw_sprite_anim_ary((const u16*)get_menu_lsp_tbl(0x54), id, &pos);
        if (entry->selected != 0) {
            draw_sprite_anim_ary((const u16*)((u32*)get_menu_lsp_tbl(0x55))[data->mode], id, &pos);
            limit_a = quest_record_a_count_get_wide(data->kind);
            limit_b = quest_record_b_count_get_wide(data->kind);
            u16* user = (u16*)get_userdata();
            u16 id_a = user[data->kind * 2 + 0x1DE1];
            u16 id_b = user[data->kind * 2 + 0x1DE0];
            fn_800CEE74(data->kind, id_a, &value_a);
            fn_800CEE74(data->kind, id_b, &value_b);
            u8 res_a = fn_8004E634(data->kind, id_a);
            u8 res_b = fn_8004E634(data->kind, id_b);
            fn_802E23D0(0x493, id, (s8*)entry->field_0x08, 4, &pos);
            fn_802E23D0(0x4A9, id, fn_802DFB18(data->field_0x02), 0, &pos);
            fn_802E1A7C(0x48E, data->kind, id, &pos);
            item_page_draw_wrapped_text(0x483, id, (s8*)fn_8029F7E0(0, entry->field_0x06), &pos);
            switch (data->mode) {
            case 1:
                if (limit_a > 0x3E7) {
                    limit_a = 0x3E7;
                }
                if (limit_b > 0x3E7) {
                    limit_b = 0x3E7;
                }
                sprintf(text, "%d", limit_a);
                fn_802E23D0(0x4AA, id, text, 0, &pos);
                sprintf(text, "%d", limit_b);
                fn_802E23D0(0x4AC, id, text, 0, &pos);
                if (id_a != 0) {
                    sprintf(text, "%6.1f", value_a);
                    fn_802E23D0(0x4AE, id, text, 2, &pos);
                    if ((u8)(res_a + 0xFF) <= 1) {
                        fn_802E0AD4((_SPR_DATA_*)&bar, 0x4A6, id, NULL);
                        if (res_a == 2) {
                            bar.pos.x += 0x14;
                            bar.pos.y += 0x14;
                        }
                        draw_sprite(*(_SPR_DATA_*)&bar, &pos);
                    }
                } else {
                    fn_802E23D0(0x4AE, id, (s8*)((u8**)get_str_tbl(0x44))[2], 2, &pos);
                }
                if (id_b != 0) {
                    sprintf(text, "%6.1f", value_b);
                    fn_802E23D0(0x4AF, id, text, 2, &pos);
                    if (res_b == 3) {
                        draw_sprite_anim_idx(0x4A7, id, &pos);
                    }
                } else {
                    fn_802E23D0(0x4AF, id, (s8*)((u8**)get_str_tbl(0x44))[2], 2, &pos);
                }
                /* fallthrough */
            case 0:
            case 2:
                sprintf(text, "%d", limit_a);
                fn_802E23D0(0x4AA, id, text, 0, &pos);
                if (data->field_0x03 != 0) {
                    fn_802E0AD4((_SPR_DATA_*)&bar, 0x49F, id, NULL);
                    for (i = 0; i < data->field_0x03; i++) {
                        draw_sprite(*(_SPR_DATA_*)&bar, &pos);
                        bar.pos.x += bar.ofs_x;
                    }
                }
                break;
            default:
                break;
            }
        } else {
            draw_sprite_anim_ary((const u16*)*get_menu_lsp_tbl(0x55), id, &pos);
            u8** blank = get_str_tbl(0x44);
            fn_802E23D0(0x493, id, (s8*)blank[1], 4, &pos);
            fn_802E23D0(0x4A9, id, (s8*)blank[1], 0, &pos);
            fn_802E23D0(0x4AA, id, (s8*)blank[1], 0, &pos);
            fn_802E23D0(0x4A8, id, (s8*)blank[1], 0, &pos);
        }
    }
    font_flush();
}

/* 0x8034B560: the page-2 column draw plus the text flush. */
extern "C" void item_page_draw_column1(MenuSlot* slot)
{
    item_page_draw_blank_rows(slot->entries_b, slot->scroll.row, slot->scroll.row_limit, slot->field_0x23A, slot->field_0x23C);
    font_flush();
}

/* 0x8034B59C: the page-1 detail panel - the panel frame and the selected row's item card (its icon,
 * the three names and the two value lines). */
extern "C" void item_page_draw_detail2(MenuSlot* slot)
{
    SprWork spr;
    MenuEntry* entry;
    _mh_ivec2_ pos;
    s8 text[0x10];
    u8 id;
    u16 item_a;
    u16 item_b;
    ItemRec* item;

    if (slot->field_0x002 != 0 || slot->field_0x23B != 0) {
        id = slot->field_0x23B;
        entry = &slot->entries_b[slot->scroll.row];
        fn_802E0AD4((_SPR_DATA_*)&spr, 0x4B0, id, &pos);
        draw_sprite_anim_ary((const u16*)get_menu_lsp_tbl(0x56), id, &pos);
        if (entry->field_0x02 != 0) {
            item = (ItemRec*)fn_8033ADD0(entry->field_0x06, &item_a, &item_b, 0);
            if (item != NULL) {
                fn_802E0AD4((_SPR_DATA_*)&spr, 0x4C4, id, NULL);
                draw_itemicon_item_id(*(_SPR_DATA_*)&spr, item->id, &pos);
                fn_802E23D0(0x4C8, id, (s8*)ItemName(item->id), 4, &pos);
                fn_802E23D0(0x4C9, id, (s8*)ItemName(item_a), 0, &pos);
                fn_802E23D0(0x4CA, id, (s8*)ItemName(item_b), 0, &pos);
                sprintf(text, "%d%", (s8)fn_8033AED0(item, 3));
                fn_802E23D0(0x4D4, id, text, 1, &pos);
                fn_8033B67C(text, item);
                fn_802E23D0(0x4D5, id, text, 1, &pos);
                fn_802E0AD4((_SPR_DATA_*)&spr, 0x4D7, id, NULL);
                font_set_size(spr.width, spr.height);
                flfntSetColor(spr.colour_0x1C);
                font_print_ex(pos.x + spr.pos.x, pos.y + spr.pos.y, -1, "%s",
                              ItemExp(item->id));
            }
        } else {
            u8** blank = get_str_tbl(0x44);
            fn_802E23D0(0x4C8, id, (s8*)blank[1], 4, &pos);
            fn_802E23D0(0x4C9, id, (s8*)blank[1], 0, &pos);
            fn_802E23D0(0x4CA, id, (s8*)blank[1], 0, &pos);
        }
    }
    font_flush();
}

/* 0x8034B7EC: the page-3 column draw plus the text flush. */
extern "C" void item_page_draw_column2(MenuSlot* slot)
{
    item_page_draw_blank_rows(slot->entries_b, slot->scroll.row, slot->scroll.row_limit, slot->field_0x23A, slot->field_0x23C);
    font_flush();
}

/* 0x8034B828: the page-2/3 detail draw - the empty-column variant when the slot is closed, else the
 * column's own rows plus the quality glyph of the first row that has one. */
extern "C" void item_page_draw_column3(MenuSlot* slot)
{
    SprWork spr;
    SprWork glyph;
    _mh_ivec2_ pos;
    _mh_ivec2_ posGlyph;
    s32 has_glyph;
    u8* str;
    u8 base;
    u8 kind;

    fn_802E0AD4((_SPR_DATA_*)&spr, 0x438, slot->field_0x23A, &posGlyph);
    if (slot->field_0x002 != 0xA) {
        has_glyph = (slot->field_0x2F5 != 0);
        if (has_glyph == 1) {
            fn_802E0AD4((_SPR_DATA_*)&glyph, *(u16*)get_menu_lsp_tbl(0x52), slot->field_0x23A, NULL);
            glyph.pos.x += posGlyph.x;
            glyph.pos.y += posGlyph.y;
            draw_sprite_anim_ary((const u16*)get_menu_lsp_tbl(0x51), slot->field_0x23A, &glyph.pos);
            draw_sprite_anim_ary((const u16*)get_menu_lsp_tbl(0x50), slot->field_0x23A, &glyph.pos);
            kind = slot->field_0x2F5;
            base = slot->kind_row_base[(kind - 1) * 2];
            switch (kind) {
            case 1:
                str = (u8*)get_str_tbl(0x45)[(u8)(base + slot->field_0x01D)];
                break;
            case 2:
                str = (u8*)get_str_tbl(0x46)[(u8)(base + slot->field_0x01E)];
                break;
            case 3:
                str = (u8*)get_str_tbl(0x47)[(u8)(base + slot->field_0x01F)];
                break;
            default:
                str = NULL;
                break;
            }
            if (str != NULL) {
                draw_font_idx(0x439, (s8*)str, 5, &glyph.pos);
            }
        }
        item_page_draw_rows(slot->entries_b, slot->scroll.row, slot->scroll.row_limit, slot->field_0x23A,
                    slot->field_0x23C, has_glyph, 0xFF);
        return;
    }
    kind = slot->field_0x2F5;
    item_page_draw_closed_column(kind, (u8)(slot->scroll.rows_per_page * slot->scroll.col +
                          (slot->kind_row_base[kind * 2] + slot->scroll.row)),
                slot->scroll.light_colour, slot->scroll.dark_colour, slot->field_0x23A, slot->field_0x008, 2);
}

/* 0x8034BA30: the "closed" page column - the option's own value rows, its name/effect text and the
 * two value strings, all walked by the record `item_page_option_row_index` selected. */
extern "C" void item_page_draw_closed_column(u8 kind, u8 index, s8 light, s8 dark, u8 id, u16 arg5, s32 flags)
{
    SprWork spr;
    _mh_ivec2_ pos;
    _mh_ivec2_ posText;
    u8** values = NULL;
    s8* value;
    s8 selected;
    u8 blend;

    selected = item_page_option_row_index(kind, index, light);
    fn_802E0AD4((_SPR_DATA_*)&spr, 0x438, id, &pos);
    if (selected >= 0 && get_option_cfg(7) == 0) {
        blend = flags & 0xFE;
    } else {
        blend = flags | 1;
    }
    item_page_draw_option_frame(light, dark, arg5, 5, blend, &posText);
    switch (kind) {
    case 2:
        values = get_str_tbl(0x47);
        break;
    case 3:
        values = get_str_tbl(0x48);
        break;
    }
    if (values != NULL) {
        draw_font_idx(0x514, (s8*)values[index], 4, &posText);
    }
    value = item_page_option_value_string(kind, index, light, NULL);
    if (value != NULL) {
        if (selected >= 0 && get_option_cfg(7) == 0) {
            draw_font_idx(0x515, value, 0, &posText);
            return;
        }
        draw_font_idx(0x516, value, 0, &posText);
    }
}

/* 0x8034BBB0: the record index the option's value table selects, or -1 when the kind has none. */
extern "C" s8 item_page_option_row_index(u8 kind, u8 index, s8 light)
{
    s32 table = 0;
    u8 row = index;

    switch (kind) {
    case 2:
        if (index >= 0x0C && index < 0x1A) {
            row = index - 0x0C;
            table = fn_8029F818(4);
        }
        break;
    case 3:
        table = fn_8029F818(5);
        break;
    }
    if (table != 0) {
        return ((s8*)table)[row * 4 + light];
    }
    return -1;
}

/* 0x8034BC58: the value string the option card shows for one row - the row's own table, the variant
 * table the option config selects and the two "same as the player's" comparisons, with the status
 * code the card colours by written back through `out`. */
extern "C" s8* item_page_option_value_string(u8 kind, u8 index, s8 light, s8* out)
{
    u8** head = get_str_tbl(0x49);
    u8** table = NULL;
    u8** variant_a = NULL;
    u8** variant_b = NULL;
    u8** chosen;
    s8* value;
    u8 row = index;
    u8 status;
    u32 cfg;

    if (head == NULL) {
        if (out != NULL) {
            *out = -1;
        }
        return NULL;
    }
    if (kind > 1) {
        switch (kind) {
        case 2:
            table = get_str_tbl(0x4A);
            if (index >= 0x0C && index < 0x1A) {
                row = index - 0x0C;
                variant_a = get_str_tbl(0x4B);
                variant_b = get_str_tbl(0x4C);
            }
            break;
        case 3:
            table = get_str_tbl(0x4D);
            variant_a = get_str_tbl(0x4E);
            variant_b = get_str_tbl(0x4F);
            break;
        default:
            if (out != NULL) {
                *out = -1;
            }
            return NULL;
        }
        cfg = get_option_cfg(7);
        switch (cfg) {
        case 0:
            chosen = (u8**)table[index];
            status = 1;
            break;
        case 2:
            if (variant_a == NULL) {
                chosen = (u8**)table[index];
                status = 1;
            } else {
                chosen = (u8**)variant_a[row];
                value = (s8*)chosen[light];
                if (strcmp(value, (const s8*)head[0]) == 0) {
                    chosen = (u8**)table[index];
                    status = 1;
                } else if (strcmp(value, (const s8*)head[1]) == 0) {
                    chosen = NULL;
                    status = 0;
                } else {
                    status = 2;
                }
            }
            break;
        case 1:
            if (variant_b == NULL) {
                chosen = (u8**)table[index];
                status = 1;
            } else {
                chosen = (u8**)variant_b[row];
                value = (s8*)chosen[light];
                if (strcmp(value, (const s8*)head[0]) == 0) {
                    chosen = (u8**)table[index];
                    status = 1;
                } else if (strcmp(value, (const s8*)head[1]) == 0) {
                    chosen = NULL;
                    status = 0;
                } else {
                    status = 3;
                }
            }
            break;
        default:
            chosen = NULL;
            status = 0xFF;
            break;
        }
        if (out != NULL) {
            *out = status;
        }
        return (chosen != NULL) ? (s8*)chosen[light] : NULL;
    }
    if (out != NULL) {
        *out = 4;
    }
    return NULL;
}

/* 0x8034BEF4: the "closed" season card - the frame row, the two arrow columns and the value glyph. */
extern "C" void item_page_draw_option_frame(s8 a, s8 b, u16 c, u16 d, u8 flags, _mh_ivec2_* pos)
{
    SprWork spr;
    u32 glyph;
    u8 arrow = 3;

    fn_802E0AD4((_SPR_DATA_*)&spr, 0x4FE, d, pos);
    draw_sprite_anim_ary((const u16*)get_menu_lsp_tbl(0x5A), d, pos);
    if ((flags & 8) == 0) {
        if ((flags & 4) != 0) {
            arrow = 3 | 0x80;
        }
        PutPageArrow((u16*)get_menu_lsp_tbl(0x5B), a, b, c, pos, arrow);
    }
    glyph = (flags & 1) ? 0x51E : 0x51B;
    draw_sprite_anim_idx(glyph, d, pos);
}

/* 0x8034BFC8: whether the season card may be shown at all - the option gate, the index's sign and the
 * three task predicates the season table answers. */
extern "C" u32 item_page_option_available(s8 index)
{
    u32 result = 0;

    if (get_option_cfg(7) != 0) {
        return 0;
    }
    if (index < 0) {
        return 0;
    }
    if (fn_802DA20C(1, (u8)index) == 1) {
        return 0;
    }
    fn_802DA2D4(1);
    if (fn_802DA454(1, (u8)index, 0, 0, 1, 1, 0) != 0) {
        result = 1;
    }
    if (index == 0xB && result == 1) {
        result = 0;
        if (fn_802DA454(1, 0x15, 0, 0, 1, 1, 0) != 0) {
            result = 1;
        }
    }
    return result;
}
