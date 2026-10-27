/*
 * menu/menu_message.cpp - the menu band's list and cursor layer (the selection list `MenuListWork`, its cursor and
 *   scroll steps, the item pairs), its item-row and list-page draws, and its dialog frame / message window helpers.
 *   C++; a bare map row is defined `extern "C"`, a mangled one at global scope (`put_message`, `put_frame_dialog`,
 *   `put_message_sys`, `GetMenuFontColor*`).
 * RANGE. .text 0x802A6624-0x802AA6A8 (70 functions, all written); extab, extabindex, .data 0x805CE00C-0x805CE198,
 *   .sdata 0x807922A0-0x807922B0 (`"%d"`, `"%s"`, `"%s"`), .sdata2 0x8079A3F8 (120.0f, `draw_sprite_blink`).  The right
 *   edge is `stage/shell.cpp`'s `shell_work_init` (0x802AA6A8).  The range is two TUs, seam at 0x802A7C44
 *   (`draw_dialog_piece`): the `"%s"` copies (0x807922A4 read from 0x802A78F4/0x802A7B0C, 0x807922A8 from
 *   0x802AA078-0x802AA234) cannot share a TU under `-str reuse`, and the `.data` tiles as a 4-aligned 0x34-byte first
 *   object (0x805CE00C, `menu_item_row_draw_values`'s) and an 8-aligned rest from 0x805CE040 (`menu_pad_map_set`'s,
 *   `draw_dialog_piece`'s and `menu_yes_no_draw`'s tables), which one object cannot lay out.
 * FLAGS. `cflags_menu` (configure.py); `#pragma pool_data off` for the file: retail addresses each `.data` table with
 *   its own `lis`/`addi` pair (`menu_yes_no_draw` reads three; with pooling on it is 81.99 against 100).
 * NAMES. Module `menu` from the left neighbour `menu/menu_item.cpp` and the range's own entry points
 *   (`put_message__FPScsUl`, `put_frame_dialog__FssssUlUl`, `GetMenuFontColor__Fbbbb`).  The file name is a GUESS from
 *   what the range does; the DOL's one `menu_message.cpp` string (0x80607870) is cited only by `plsearch_step`
 *   (0x8044E94E), inside `menu/menu_plsearch.cpp`'s registered range, so the retail `menu_message.cpp` is not this range.
 *   GUESSes from the bodies: `menu_page_count` (a ceil-divide, a 0 divisor read as 1), `menu_text_*center_x*`,
 *   `menu_item_pick_seek`, `item_pair*`/`item_pages_*`, `menu_slot_*`, `get_group2_name_str`, `menu_item_row_sprite_ids`,
 *   the `.data` tables marked in place, the cursor page move's 0x04/0x08 bits as "first"/"last", `put_frame_dialog_title`,
 *   `put_frame_header`, `put_frame_label`, `put_frame_separator`, `put_frame_side_arrows`, `put_frame_cursor_bar`,
 *   `put_message_window`, `menu_select_window_draw`, `menu_choice_window_draw`, `menu_entry_window_draw`,
 *   `menu_pad_map_set`, `menu_yes_no_draw`, `draw_sprite_blink` (alpha scaled by `cockpit_blink_get`),
 *   `menu_item_pages_resort` (re-sorts the pages and re-points the pick and the selected slot), `MenuRowLayout`.
 * RESIDUALS. 10 rows below 100 %, register allocation or branch layout only:
 *  - `menu_page_count`: retail `beqlr ... blr`, ours `bnelr / mr r3,r5 / blr` (60 vs 64 B);
 *  - `menu_list_fill`: `self` and the record pointer in r29/r30 swapped, each `id` extension before the record test;
 *  - `menu_scroll_init`: retail `blt`+`b` around the `rows` copy, ours one `bge` (116 vs 120 B; if, ternary and
 *    empty-then forms measured the same);
 *  - `GetMenuFontColor`: retail keeps the inner `if (b == 1)` as `bne` + `blr`, MWCC folds it into a conditional return;
 *  - `menu_choice_window_draw`, `menu_entry_window_draw`: retail tests the entry colour with `beq` over a `b`, ours one
 *    `bne` (4 B each), plus register order;
 *  - `draw_sprite_blink`: retail loads the colour word into r3 and masks it after the multiply, ours masks first (the
 *    operand order and a separate local measured the same);
 *  - `item_pairs_shell_sort`, `put_message`, `put_message_window`: register allocation only.
 *  - `.data` is 0x190 against 0x18C and `.sdata` 7 against 16 B: one object cannot reproduce the two TUs above (the
 *    second `"%s"` copy and the 4-mod-8 start of the first `.data` object).
 *  - `menu_select_window_draw` is defined with the types its body uses (`s8** texts`, `s16* flags`); its leaf
 *    `menu/menu_select_window_draw.h` still carries `menu/menu_combo.cpp`'s call-site view and is not included here.
 *   `menu/menu_item.cpp` and `menu/menu_combo.cpp` still declare `menu_slot_*`, `menu_item_slot_accepts`, `menu_list_*`,
 *   `menu_cursor_column_step`, `menu_choice_window_draw` and `fn_802A9BCC` (`menu_item_pages_resort`) themselves.
 * SHAPES. `++x`/`x--` instead of `x = x + 1` keeps a narrow local in place (`menu_cursor_move`,
 *   `menu_frame_entries_build`, `menu_cursor_page_move`).
 *  - A local declared first and assigned later, in the target's register order, picks its callee-saved register
 *    (`item_pairs_compact_sort`, `menu_item_pick_seek`, `menu_text_block_center_x`, `menu_frame_row_draw`,
 *    `put_frame_dialog_title`'s and `put_frame_header`'s `left`).
 *  - `menu_frame_entries_build_row` is a four-iteration `for`, not four blocks; `menu_slot_find`'s pooled `u16` result
 *    needs a `u16` variable, not a cast.
 *  - The draw helpers' default colours are `if (c == 0) c = ...;` on the parameter; a value `x = a ? b : c` is a
 *    ternary where retail loads the else value first (`menu_frame_row_draw`); a `switch` on `flags & 3` needs the
 *    `u32` parameter for its `cmplwi` chain (`menu_frame_draw_blocks`); `put_message` keeps the text y in `size`.
 */

#include "types.h"

#include "Runtime.PPCEABI.H/memcpy.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "ef/fn_800CDB2C.h"
#include "menu/menu_message.h"
#include "menu/menu_item.h"
#include "font/flfnt.h"
#include "fn_80047398.h"
#include "hud/layout.h"
#include "menu/get_pop_dat_ptr.h"
#include "menu/put_frame_dialog_title.h"
#include "sound/set_zmode__FbUcb.h"
#include "hud/cockpit_blink_get.h"
#include "hud/cockpit.h"
#include "hud/draw_window_box.h"
#include "mh3_pad/Screen_w.h"
#include "menu/put_message_sys.h"
#include "menu/put_frame_side_arrows.h"
#include "menu/menu_choice_window_draw.h"
#include "menu/menu_entry_window_draw.h"
#include "menu/menu_pad_map_set.h"
#include "Pl/pl_item_slot_item_get.h"
#include "Pl/pl_act.h"
#include "pad_wait_table_set.h"
#include "MSL_C/printf.h"
#include "hud/draw_sprite_blink_idx.h"
#include "hud/put_button_guide_b.h"

extern "C" char* strcpy(char* dst, const char* src);

/* Retail addresses each `.data` table with its own `lis`/`addi` pair (`menu_yes_no_draw` reads three). */
#pragma pool_data off

/* ---------------------------------------------------------------------------------------------------
 * The range's own entry points and the callees its written bodies call.  A `fn_XXXXXXXX` stem is the
 * map's placeholder, so those are declared `extern "C"` (the map's spelling is what objdiff pairs);
 * a mangled map name (`get_move_work_adrs__FUc`) is declared as the function it spells.
 * --------------------------------------------------------------------------------------------------- */

extern "C" u32 quest_move_state_valid_ck(void);

/* The HUD's 2D element library (`hud/layout.h`, its owner's header) supplies the `_mh_ivec2_` /
 * `_SPR_DATA_` records and every `draw_*`/`put_*` entry this range calls, so none of them is
 * re-declared here (docs/plan.md 6.5 rule 2).  This unit's own `MenuLspPos` is the same 4-byte
 * record; the call sites cast to `_mh_ivec2_*`, which is a no-op. */

void sysSE_req(long id);
u8** get_str_tbl(long index);
u8* get_move_work_adrs(u8 kind);
s32 get_move_work_max(u8 kind);
extern "C" void menu_frame_page_draw(u32* dst, MenuListEntry* entries, s8 a, s8 b, u16 c, u32 d, u8 e);

/* The source table the list is built from: `lobby/lb_companion_ui.cpp`'s `.bss`, declared here. */
extern MenuSourceRecord lobby_hunter_cards[10];   /* 2 x 5 records: validity byte + two names */

/* `hud/cockpit.cpp`'s page-arrow helper (0x802DAF48), declared in this unit's call sites' shape; its owner's
 * declaration in `ai/fn_802D44F4.h` is not included here. */
void PutPageArrow(u16* table, s16 a, s16 b, u16 flags, const struct _mh_ivec2_* pos, u8 mode);

/* The sprite-id run (terminated by 0xFFFF) `menu_item_row_draw_values` hands to `draw_sprite_ary` (`.data` 0x805CE00C). */
u16 menu_item_row_sprite_ids[26] = {
    0x00A9, 0x00AA, 0x0089, 0x00A4, 0x008B, 0x00A5, 0x00A6, 0x00A7,
    0x008F, 0x00A8, 0x0093, 0x00AB, 0x00AC, 0x00B2, 0x00B3, 0x00B4,
    0x00B5, 0x00B6, 0x00B7, 0x00B8, 0x00B9, 0x009D, 0x00AD, 0x00AE,
    0xFFFF, 0x0000
};

/* Four per-slot value runs (17 values of 1..3; the compiler pads each to 0x14) and the pointer table over them, which the
 * tail helper at 0x802AA68C (index by the second argument) hands to the pad layer's setter (`.data` 0x805CE040-0x805CE0A0).  GUESS: names, from the reader. */
u8 menu_pad_slot_map_0[17] = {1, 1, 1, 1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3};
u8 menu_pad_slot_map_1[17] = {1, 1, 1, 1, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3};
u8 menu_pad_slot_map_2[17] = {1, 1, 1, 1, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3};
u8 menu_pad_slot_map_3[17] = {1, 1, 1, 1, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3};
u8* menu_pad_slot_map_tbl[4] = {menu_pad_slot_map_0, menu_pad_slot_map_1, menu_pad_slot_map_2, menu_pad_slot_map_3};

/* The dialog frame's piece rectangles, read by `draw_dialog_piece` and the frame builder (`.data` 0x805CE0A0).  GUESS: each row's
 * four halfwords are a texture rectangle (left/top/right/bottom). */
_mh_tex_uv_ dialog_piece_uv_tbl[24][2] = {
    {{0x0067, 0x0083}, {0x0069, 0x0085}}, {{0x0060, 0x0060}, {0x0080, 0x0061}}, {{0x0080, 0x0060}, {0x00A0, 0x0061}},
    {{0x0080, 0x0060}, {0x0081, 0x0080}}, {{0x0060, 0x0040}, {0x0080, 0x0060}}, {{0x0080, 0x0040}, {0x00A0, 0x0060}},
    {{0x0060, 0x0060}, {0x0080, 0x0080}}, {{0x0080, 0x0060}, {0x00A0, 0x0080}}, {{0x00A2, 0x004C}, {0x00B2, 0x006C}},
    {{0x00B2, 0x004C}, {0x00BA, 0x006C}}, {{0x0095, 0x006E}, {0x00A9, 0x007E}}, {{0x00A9, 0x006E}, {0x00B1, 0x007E}},
    {{0x01C5, 0x005F}, {0x01FB, 0x008B}}, {{0x01FA, 0x005F}, {0x01FB, 0x008B}}, {{0x01B9, 0x00D7}, {0x01C2, 0x00EB}},
    {{0x01BF, 0x00D7}, {0x01C2, 0x00EB}}, {{0x00B7, 0x0000}, {0x00C5, 0x0017}}, {{0x00B7, 0x0000}, {0x00BE, 0x0017}},
    {{0x00B7, 0x0000}, {0x00B8, 0x0017}}, {{0x00A4, 0x0000}, {0x00AB, 0x001B}}, {{0x00AB, 0x0000}, {0x00AD, 0x001B}},
    {{0x00A5, 0x000B}, {0x00A6, 0x0011}}, {{0x0093, 0x002C}, {0x00A3, 0x003E}}, {{0x00A3, 0x002C}, {0x0093, 0x003E}},
};

/* Three sprite-id runs (terminated by 0xFFFF, the last word padding) the dialog's sprite draw passes to `draw_sprite_ary`
 * (`.data` 0x805CE160-0x805CE198).  GUESS: names, from the reader's order. */
u16 dialog_sprite_ids_frame[16] = {
    0x01C9, 0x01C1, 0x01C2, 0x01C3, 0x01C4, 0x01C5, 0x01C6, 0x01C7,
    0x01C8, 0x01AF, 0x01B0, 0x01B1, 0x01B7, 0x01B8, 0x01B9, 0xFFFF
};
u16 dialog_sprite_ids_a[6] = {0x01B5, 0x01B2, 0x01B3, 0x01B4, 0xFFFF, 0x0000};
u16 dialog_sprite_ids_b[6] = {0x01BD, 0x01BA, 0x01BB, 0x01BC, 0xFFFF, 0x0000};

/* One row style of the item list's layout table (`get_menu_lsp_tbl(18)`, indexed by style): the sprite-id runs
 * `menu_frame_row_draw` draws.  size: 0x10 */
typedef struct MenuRowLayout {
    /* +0x00 */ u16* frame_ids;   /* the row frame's sprite run */
    /* +0x04 */ u16* icon_ids;    /* [0] the item icon's layout, [1]/[2] the icon cell when selected/not */
    /* +0x08 */ u16* text_ids;    /* [0] the name's layout, [1]/[2] the count's for one/two digits */
    /* +0x0C */ u16* cursor_ids;  /* the cursor sprite run */
} MenuRowLayout;

/* This unit's own bodies, in address order. */
extern "C" void menu_frame_row_draw(s32 select, IdValue* item, MenuListEntry* entry, s32 flags, struct _mh_ivec2_* pos);
extern "C" void menu_item_row_draw(u16* item, struct _mh_ivec2_* pos);
extern "C" void menu_item_row_draw_values(u16* item, struct _mh_ivec2_* pos, s32 a, s32 b);
extern "C" void eft052_page_counts_get(u16 id, s32* a, s32* b, s32* c);

/* The list length the active kind's table yields for the selection (kind 1 the move table, kind 2 the item table's two
 * blocks of five; 0x80 = no selection, the table's label into both names); returns how many ids were written. */
extern "C" u8 menu_list_mode_get(MenuListWork* self)
{
    u8 result = 0;
    switch (self->kind_0x00F) {
    case 1:
        if (game_ready_ck() == 1) {
            result = 1;
        } else if (quest_move_state_valid_ck() == 1) {
            result = 2;
        } else if (player_count_get() > 1) {
            result = 3;
        } else {
            result = 0;
        }
        break;
    case 2:
        result = (game_ready_ck() - 1) == 0;
        break;
    }
    return result;
}

/* Clears the list count to its three-entry default, then subtracts the entries the finished
 * pad/task state has already consumed; an unknown kind reports 0 without touching the record. */
extern "C" u8 menu_list_count_update(MenuListWork* self)
{
    self->count_0x016 = 3;
    switch (self->kind_0x00F) {
    default:
        return 0;
    case 1:
        if (game_ready_ck() == 0 && quest_move_state_valid_ck() == 0) {
            self->count_0x016--;
        }
        break;
    case 2:
        if (game_ready_ck() == 0) {
            self->count_0x016--;
        }
        break;
    }
    return self->count_0x016;
}

/* Fills the list at +0x1BE from the active kind's table (kind 2 the item table's two blocks of five, kind 1 the move
 * table), skipping the selected index (negative = the current selection), and stores the count. */
extern "C" u8 menu_list_fill(MenuListWork* self, s32 index)
{
    s8* dst = self->list_0x1BE;

    self->list_count_0x1BB = 0;
    if (self->kind_0x00F == 2) {
        u8* rec = (u8*)&lobby_hunter_cards[0];
        s8 pick;
        s8 id;
        s32 block;

        if ((s8)index < 0) {
            index = my_player_no();
        }
        id = 0;
        pick = index;
        for (block = 0; block < 2; block++) {
            if (rec[0] != 0 && id != pick) {
                self->list_count_0x1BB++;
                *dst++ = id;
            }
            id++;
            if (rec[0x130] != 0 && id != pick) {
                self->list_count_0x1BB++;
                *dst++ = id;
            }
            id++;
            if (rec[0x260] != 0 && id != pick) {
                self->list_count_0x1BB++;
                *dst++ = id;
            }
            id++;
            if (rec[0x390] != 0 && id != pick) {
                self->list_count_0x1BB++;
                *dst++ = id;
            }
            id++;
            if (rec[0x4C0] != 0 && id != pick) {
                self->list_count_0x1BB++;
                *dst++ = id;
            }
            id++;
            rec += 0x5F0;
        }
    } else if (self->kind_0x00F == 1) {
        u8* rec = get_move_work_adrs(2);
        s8 count;
        s8 pick;
        s32 i;

        if ((s8)index < 0) {
            index = my_player_no();
        }
        count = get_move_work_max(2);
        pick = index;
        for (i = 0; i < count; i++) {
            if (rec[0] != 0 && rec[8] != pick) {
                self->list_count_0x1BB++;
                *dst++ = rec[8];
            }
            rec += 0xB20;
        }
    }
    return self->list_count_0x1BB;
}

/* Copies the active kind's record names (item or move table) into the two name buffers, the table's own label for index
 * 0x80 (no selection); reports whether a record was found. */
extern "C" s32 menu_list_names_set(MenuListWork* self, u8 index)
{
    s32 found = 0;

    if (index == 0x80) {
        u8** tbl = get_str_tbl(0x55);

        strcpy(self->long_name_0x1D2, (char*)*tbl);
        strcpy(self->short_name_0x1C8, (char*)*tbl);
        return 1;
    }
    if (self->kind_0x00F == 2) {
        MenuSourceRecord* rec = &lobby_hunter_cards[index];

        if (rec->valid_0x000 != 0) {
            strcpy(self->long_name_0x1D2, rec->long_name_0x00D);
            strcpy(self->short_name_0x1C8, rec->short_name_0x003);
            found = 1;
        }
    } else if (self->kind_0x00F == 1) {
        MenuMoveRecord* rec = &((MenuMoveRecord*)get_move_work_adrs(2))[index];

        if (rec->valid_0x000 != 0) {
            strcpy(self->long_name_0x1D2, rec->long_name_0x5CA);
            strcpy(self->short_name_0x1C8, rec->short_name_0x5DB);
            found = 1;
        }
    }
    return found;
}

/* Steps the cursor one row up/down (bits 2-3 of the input word) or one column left/right (bits 0-1),
 * wrapping at the page bounds, and plays the move SE only when the row actually changed. */
extern "C" void menu_cursor_move(MenuListWork* self, s8 step)
{
    u16 input = self->input_0x000;

    if ((input & 0xC) != 0) {
        s8 cursor = self->cursor_0x004;

        if ((input & 4) != 0) {
            self->step_0x002 = 4;
            cursor--;
            if (cursor < 0) {
                cursor = self->rows_0x005 - 1;
            }
        } else {
            self->step_0x002 = 8;
            cursor++;
            if (cursor >= self->rows_0x005) {
                cursor = 0;
            }
        }
        if (cursor != self->cursor_0x004) {
            self->cursor_0x004 = cursor;
            sysSE_req(3);
        }
    } else if ((input & 3) != 0) {
        self->column_0x006 = menu_cursor_step(self->column_0x006, step, input, 1, 2);
    } else if (step < self->column_0x006) {
        self->column_0x006 = step - 1;
    }
}

/* Moves the cursor to a flat entry index: inside the current row it is a plain column step, past it
 * the row is advanced first and the column then set from the remainder. */
extern "C" void menu_cursor_seek(MenuListWork* self, s8 index)
{
    s8 columns = self->columns_0x007;

    if (index < (self->cursor_0x004 + 1) * columns) {
        menu_cursor_move(self, (s8)(index % columns));
        return;
    }
    menu_cursor_move(self, columns);
    if (index < (self->cursor_0x004 + 1) * self->columns_0x007) {
        self->column_0x006 = (s8)(index % self->columns_0x007 - 1);
    }
}

/* Steps the cursor one column, i.e. wraps the flat index back into the current row. */
extern "C" void menu_cursor_column_step(MenuListWork* self)
{
    menu_cursor_move(self, self->columns_0x007);
}

/* Builds the two 0x60-byte entry blocks the message frame draws (sorted by `item_pages_sort`, each entry marked with its
 * ordinal and slot use) and hands them to `menu_frame_draw_blocks`; `mode` 1 adds draw flag 0x800. */
extern "C" void menu_frame_entries_build(u16* src_a, u16* src_b, s8 kind, u16 lsp_index, u8 mode)
{
    s16 lsp[2];
    u16 mask[0x10];
    u16 work[0x30];
    MenuListEntry entries[8];
    u16* pair;
    MenuListEntry* entry;
    u16* p;
    s32 index;
    s32 block;
    s32 flags;

    flags = 0x20;
    if (mode == 1) {
        flags |= 0x800;
    }
    memcpy(work, src_a, 0x60);
    p = src_b;
    if (src_b != NULL) {
        memcpy(mask, src_b, 0x20);
        p = mask;
    }
    item_pages_sort((IdValue*)work, (IdValue*)p);
    pair = work;
    entry = entries;
    index = 0;
    for (block = 0; block < 2; block++) {
        entry[0].index_0x06 = index;
        entry[0].field_0x01 = 0;
        if (pair[0] != 0) {
            entry[0].present_0x02 = 1;
        } else {
            entry[0].present_0x02 = 0;
        }
        entry[1].index_0x06 = ++index;
        entry[1].field_0x01 = 0;
        if (pair[2] != 0) {
            entry[1].present_0x02 = 1;
        } else {
            entry[1].present_0x02 = 0;
        }
        entry[2].index_0x06 = ++index;
        entry[2].field_0x01 = 0;
        if (pair[4] != 0) {
            entry[2].present_0x02 = 1;
        } else {
            entry[2].present_0x02 = 0;
        }
        entry[3].index_0x06 = ++index;
        entry[3].field_0x01 = 0;
        if (pair[6] != 0) {
            entry[3].present_0x02 = 1;
        } else {
            entry[3].present_0x02 = 0;
        }
        index++;
        pair += 8;
        entry += 4;
    }
    get_lsp_data(lsp_index, (struct _mh_ivec2_*)lsp);
    menu_frame_draw_blocks((IdValue*)work, entries, 1, kind, 0, flags, (struct _mh_ivec2_*)lsp);
}

/* The one-block form: fills four entries (ordinals 0..3) from the 0x60-byte source and the optional 0x20-byte mask and
 * hands them to `menu_frame_page_draw`. */
extern "C" void menu_frame_entries_build_row(void* unused, u16* src_a, u16* src_b, s8 kind, u16 lsp_index, u8 mode)
{
    s16 lsp[2];
    u16 mask[0x10];
    u16 work[0x30];
    MenuListEntry entries[4];
    u16* p;
    s32 i;

    memcpy(work, src_a, 0x60);
    p = src_b;
    if (src_b != NULL) {
        memcpy(mask, src_b, 0x20);
        p = mask;
    }
    item_pages_sort((IdValue*)work, (IdValue*)p);
    for (i = 0; i < 4; i++) {
        entries[i].index_0x06 = i;
        entries[i].field_0x01 = 0;
        if (work[i * 2] != 0) {
            entries[i].present_0x02 = 1;
        } else {
            entries[i].present_0x02 = 0;
        }
    }
    get_lsp_data(lsp_index, (struct _mh_ivec2_*)lsp);
    menu_frame_page_draw((u32*)work, entries, 0, kind, 0, 0x20, mode);
}

/* Reads the LSP position the constant id 628 names and forwards the call unchanged. */
extern "C" void menu_frame_draw_page(void* a, void* b, s8 c, s8 d, u16 e, s32 f)
{
    s16 lsp[2];

    get_lsp_data(628, (struct _mh_ivec2_*)lsp);
    menu_frame_draw_blocks((IdValue*)a, (MenuListEntry*)b, c, d, e, f, (struct _mh_ivec2_*)lsp);
}

/* Draws a block-framed item page at `pos`: the frame in the style `flags & 3` picks (3 drops the page counter), the
 * blinking page arrows and the page `row` of `rows`, the eight entry rows, then the footer `flags` selects (the
 * button guide, the selected item's row, or the frame message). */
extern "C" void menu_frame_draw_blocks(IdValue* items, MenuListEntry* entries, s8 row, s8 rows, u16 timer, u32 flags,
                                       struct _mh_ivec2_* pos)
{
    _mh_ivec2_ at;
    s16 dx;
    s16 dy;
    s32 counter;
    s32 style;
    s32 footer;
    u16* frame;
    u16 arrows;
    _SPR_DATA_* lsp;
    IdValue* picked;
    s32 i;

    counter = 1;
    style = 0;
    lsp = get_lsp_data(628, NULL);
    dx = pos->x - lsp->pos.x;
    dy = pos->y - lsp->pos.y;
    switch (flags & 3) {
    case 0:
        frame = (u16*)get_menu_lsp_tbl(40);
        break;
    case 1:
        frame = (u16*)get_menu_lsp_tbl(41);
        style = 1;
        break;
    case 2:
        frame = (u16*)get_menu_lsp_tbl(42);
        style = 1;
        break;
    case 3:
        frame = (u16*)get_menu_lsp_tbl(68);
        counter = 0;
        break;
    default:
        return;
    }
    draw_sprite_anim_ary(frame, timer, pos);
    if (counter != 0) {
        arrows = 0;
        if ((flags & 8) != 0) {
            arrows |= 4;
        }
        if ((flags & 0x10) != 0) {
            arrows |= 8;
        }
        if ((flags & 0x20) == 0 && timer == 0) {
            draw_sprite_blink_idx(641, 642, 0xFFE347FF, arrows, pos);
        }
        draw_number_anim(648, timer, row, (flags & 0x18) != 0, pos);
        draw_number_anim(647, timer, rows, 0, pos);
    }
    footer = 0;
    if ((flags & 0x800) == 0) {
        if ((flags & 0x20) != 0) {
            footer = 2;
        } else if ((flags & 0x40) != 0) {
            footer = 1;
        }
    }
    lsp = get_lsp_data(657, NULL);
    at.x = lsp->pos.x + dx;
    at.y = lsp->pos.y + dy;
    picked = NULL;
    if (timer != 0) {
        flags |= 0x400;
    }
    i = 0;
    do {
        menu_frame_row_draw(style, &items[entries->index_0x06], entries, flags, &at);
        if (entries->field_0x01 != 0) {
            picked = &items[entries->index_0x06];
        }
        at.y += 26;
        i++;
        entries++;
    } while (i < 8);
    switch (footer) {
    case 0:
        if ((flags & 0x180) != 0 && chk_pointer() == 0) {
            lsp = get_lsp_data(734, NULL);
            at.x = lsp->pos.x + dx;
            at.y = lsp->pos.y + dy;
            if (get_cfg(0, 7) == 0) {
                put_button_guide_b(5, &at);
            } else {
                put_button_guide_b(11, &at);
            }
        }
        break;
    case 1:
        if (picked != NULL) {
            menu_item_row_draw_by_lsp(735, (u16*)picked);
            if ((flags & 0x180) != 0 && chk_pointer() == 0) {
                ainpc_page_hold_clear();
                lsp = get_lsp_data(733, NULL);
                at.x = lsp->pos.x + dx;
                at.y = lsp->pos.y + dy;
                if (get_cfg(0, 7) == 0) {
                    put_button_guide_b(4, &at);
                } else {
                    put_button_guide_b(10, &at);
                }
            }
        }
        break;
    case 2:
        lsp = get_lsp_data(732, NULL);
        at.x = lsp->pos.x + dx;
        at.y = lsp->pos.y + dy;
        draw_window_frame_preset(at.x, at.y, 0);
        font_set_size(16, 16);
        font_print_ex(at.x + 24, at.y + 16, 0, (s8*)get_menu_msg(37, 7));
        break;
    }
    font_flush();
}

/* Draws a four-row page at the page's anchor plus the per-kind offset, 26 pixels per row, from the block
 * `menu_frame_entries_build`/`menu_frame_entries_build_row` filled; `mode` 0..2 picks the style, anything else is a no-op. */
extern "C" void menu_frame_page_draw(u32* dst, MenuListEntry* entries, s8 a, s8 b, u16 c, u32 flags, u8 mode)
{
    _mh_ivec2_ pos;
    _mh_ivec2_ at;
    s16 dx;
    s16 dy;
    s32 select;
    u16* frame;
    u16 arrows;
    _SPR_DATA_* lsp;
    s32 i;

    get_lsp_data(((u16*)get_menu_lsp_tbl(361))[mode], &pos);
    lsp = get_lsp_data(3936, NULL);
    dx = pos.x - lsp->pos.x;
    dy = pos.y - lsp->pos.y;
    switch (flags & 3) {
    case 0:
        frame = (u16*)get_menu_lsp_tbl(362);
        select = 2;
        break;
    case 1:
        frame = (u16*)get_menu_lsp_tbl(363);
        select = 3;
        break;
    case 2:
        frame = (u16*)get_menu_lsp_tbl(364);
        select = 3;
        break;
    default:
        return;
    }
    draw_sprite_anim_ary(frame, c, &pos);
    arrows = 0;
    if ((flags & 8) != 0) {
        arrows |= 4;
    }
    if ((flags & 0x10) != 0) {
        arrows |= 8;
    }
    PutPageArrow((u16*)get_menu_lsp_tbl(365), a, b, arrows, &pos, 0);
    lsp = get_lsp_data(3961, NULL);
    at.x = dx + lsp->pos.x;
    at.y = dy + lsp->pos.y;
    if (c != 0) {
        flags |= 0x400;
    }
    i = 0;
    do {
        menu_frame_row_draw(select, (IdValue*)&dst[entries->index_0x06], entries, flags, &at);
        at.y += 26;
        i++;
        entries++;
    } while (i < 4);
    font_flush();
}

/* Draws one item-list row in style `select` at `pos`: the row frame, the item's name and count (in the entry's own
 * colours, or by its pickable/selected bytes and `flags`), the item icon and its cell, and the cursor. */
extern "C" void menu_frame_row_draw(s32 select, IdValue* item, MenuListEntry* entry, s32 flags, struct _mh_ivec2_* pos)
{
    char buf[4];
    _SPR_DATA_ spr;
    MenuRowLayout* layout;
    u32 text_color;
    u32 count_color;
    u32 icon_color;
    u16 id;
    s32 font_flags;
    s16 max;

    layout = ((MenuRowLayout**)get_menu_lsp_tbl(18))[(u8)select];
    draw_sprite_ary(layout->frame_ids, pos);
    id = item->id;
    if ((flags & 4) != 0 && (text_color = entry->text_color_0x0C) != 0) {
        count_color = entry->count_color_0x10;
        icon_color = entry->icon_color_0x14;
    } else if (entry->present_0x02 != 0) {
        if ((flags & 0x400) == 0) {
            text_color = 0xC3C3C3FF;
            count_color = 0xDC2323FF;
            icon_color = 0xDFDFDFFF;
            if (entry->field_0x01 != 0) {
                text_color = (flags & 0x200) != 0 ? 0xE8A40FFF : 0xF0F0F0FF;
                count_color = 0xF53737FF;
                icon_color = 0xFFFFFFFF;
            }
        } else {
            text_color = entry->field_0x01 != 0 ? 0xE8A40FFF : 0x878787FF;
            count_color = 0xAA1414FF;
            icon_color = 0xAAAAAAFF;
        }
    } else {
        text_color = 0x646464FF;
        count_color = 0x870F0FFF;
        icon_color = 0x8A8A8AFF;
    }
    font_flags = entry->field_0x01 != 0 ? 4 : 0;
    spr_data_copy(&spr, get_lsp_data(layout->text_ids[0], NULL));
    spr.color = text_color;
    draw_font(spr, id != 0 ? (s8*)ItemName(id) : (s8*)get_menu_msg(37, 9), font_flags, pos);
    if (id != 0) {
        max = GetItemData(id)->max_num_0x003;
        if (max > 1 || (flags & 0x1000) != 0) {
            sprintf(buf, "%d", item->value);
            spr_data_copy(&spr, get_lsp_data(layout->text_ids[item->value < 10 ? 1 : 2], NULL));
            if (item->value < max) {
                count_color = text_color;
            }
            spr.color = count_color;
            draw_font(spr, (s8*)buf, font_flags | 1, pos);
        }
    }
    spr_data_copy(&spr, get_lsp_data(layout->icon_ids[entry->field_0x01 != 0 ? 1 : 2], NULL));
    spr.color = color_mult(spr.color, icon_color);
    draw_sprite(spr, pos);
    if (id != 0) {
        spr_data_copy(&spr, get_lsp_data(layout->icon_ids[0], NULL));
        spr.color = icon_color;
        draw_itemicon_item_id(spr, id, pos);
    }
    if ((flags & 0x200) == 0 && entry->field_0x01 != 0) {
        put_menu_cursor(layout->cursor_ids, 0, pos);
    }
}

/* Draws an item row at `pos`: the row frame, then for a non-empty `item` its icon, name, description and the
 * rarity-coloured rank badge. */
extern "C" void menu_item_row_draw(u16* item, struct _mh_ivec2_* pos)
{
    _SPR_DATA_ spr;
    _SPR_DATA_* lsp;
    ItemDataRecord* data;
    u32 color;

    draw_sprite_ary((u16*)get_menu_lsp_tbl(46), pos);
    if (item != NULL && item[0] != 0) {
        draw_itemicon_anim_idx(158, 0, item[0], pos);
        draw_font_idx(162, (s8*)ItemName(item[0]), 1, pos);
        lsp = get_lsp_data(163, NULL);
        font_set_size(lsp->width, lsp->height);
        font_print_ex(pos->x + lsp->pos.x, pos->y + lsp->pos.y, 0, (s8*)"%s", (s8*)ItemExp(item[0]));
        data = GetItemData(item[0]);
        color = get_rare_color(data->level_0x01);
        spr_data_copy(&spr, get_lsp_data(159, NULL));
        spr.color = color;
        draw_sprite(spr, pos);
        draw_number_idx(160, data->level_0x01 + 1, color, pos);
    }
}

/* Reads the position the caller's LSP id names and draws one item row at it. */
extern "C" void menu_item_row_draw_by_lsp(u16 id, u16* item)
{
    MenuLspPos pos;

    get_lsp_data(id, (struct _mh_ivec2_*)&pos);
    menu_item_row_draw(item, (struct _mh_ivec2_*)&pos);
}

/* Draws an item row with its two counts at `pos`: the row frame, then for a non-empty `item` its icon, name, the
 * `count` (warning red at the item's cap) and the `stock` (capped at 999), the description and the rank badge. */
extern "C" void menu_item_row_draw_values(u16* item, struct _mh_ivec2_* pos, s32 count, s32 stock)
{
    char buf[16];
    _SPR_DATA_ spr;
    _SPR_DATA_* lsp;
    ItemDataRecord* data;
    u32 color;

    draw_sprite_ary(menu_item_row_sprite_ids, pos);
    if (item != NULL && item[0] != 0) {
        data = GetItemData(item[0]);
        draw_itemicon_anim_idx(158, 0, item[0], pos);
        draw_font_idx(175, (s8*)ItemName(item[0]), 1, pos);
        spr_data_copy(&spr, get_lsp_data(176, NULL));
        if (count >= data->max_num_0x003) {
            spr.color = 0xFF435DFF;
        }
        sprintf(buf, "%d", count);
        draw_font(spr, (s8*)buf, 1, pos);
        if (stock > 999) {
            stock = 999;
        }
        sprintf(buf, "%d", stock);
        draw_font_idx(177, (s8*)buf, 2, pos);
        lsp = get_lsp_data(163, NULL);
        font_set_size(lsp->width, lsp->height);
        font_print_ex(pos->x + lsp->pos.x, pos->y + lsp->pos.y, 0, (s8*)"%s", (s8*)ItemExp(item[0]));
        color = get_rare_color(data->level_0x01);
        spr_data_copy(&spr, get_lsp_data(159, NULL));
        spr.color = color;
        draw_sprite(spr, pos);
        draw_number_idx(160, data->level_0x01 + 1, color, pos);
    }
}

/* Draws the item row at the caller's position: `eft052_page_counts_get`'s two value words through
 * `menu_item_row_draw_values`; a null or empty item is skipped. */
extern "C" void menu_item_row_draw_pages(u16* item, struct _mh_ivec2_* pos)
{
    s32 value;
    s32 low;
    s32 high;
    s32 sum;

    if (item != NULL) {
        if (item[0] != 0) {
            eft052_page_counts_get(item[0], &value, &low, &high);
            sum = low + high;
        } else {
            value = 0;
            sum = 0;
        }
        menu_item_row_draw_values(item, pos, value, sum);
    }
}

/* Reads the position the caller's LSP id names and draws one item row at it. */
extern "C" void menu_hold_row_draw_by_lsp(u16 id, u16* item)
{
    MenuLspPos pos;

    get_lsp_data(id, (struct _mh_ivec2_*)&pos);
    menu_item_row_draw_pages(item, (struct _mh_ivec2_*)&pos);
}

/* Places the dialog piece `index` of the frame atlas at (`x`, `y`) relative to `pos`, `width` x `height`, and draws it. */
extern "C" void draw_dialog_piece(_SPR_DATA_* spr, s16 x, s16 y, s16 width, s16 height, u16 index,
                                  const _mh_ivec2_* pos)
{
    _mh_tex_uv_* uv;

    spr->pos.x = x;
    spr->pos.y = y;
    spr->width = width;
    spr->height = height;
    uv = dialog_piece_uv_tbl[index];
    tex_uv_copy(&spr->uv0, &uv[0]);
    tex_uv_copy(&spr->uv1, &uv[1]);
    draw_sprite(*spr, pos);
}

/* Draws a titled dialog frame `width` x `height` at (`x` widened by `wide`, `y`): the backdrop in `frame_color`, the
 * border in `color`, the title plate, and `title` in `title_color`; returns the widened x. */
extern "C" s32 put_frame_dialog_title(s16 x, s16 y, s16 width, s16 height, u32 color, u32 frame_color,
                                      u32 title_color, s8* title, u32 wide)
{
    s16 left;
    _mh_ivec2_ pos;
    _SPR_DATA_ spr;

    left = x + get_wide_offset(wide);
    pos.x = left;
    pos.y = y;
    memset(&spr, 0, sizeof(spr));
    spr.u_scale = spr.v_scale = 100;
    if (color == 0) {
        color = 0x965A3CFF;
    }
    if (frame_color == 0) {
        frame_color = 0x14120AB2;
    }
    if (title_color == 0) {
        title_color = 0xEFF095FF;
    }
    spr.tex_flag = 5;
    spr.color = frame_color;
    draw_dialog_piece(&spr, 6, 8, width - 13, height - 20, 0, &pos);
    memset(&spr, 0, sizeof(spr));
    spr.u_scale = spr.v_scale = 100;
    spr.tex_flag = 5;
    spr.color = color;
    draw_dialog_piece(&spr, -8, 24, 32, height - 48, 1, &pos);
    draw_dialog_piece(&spr, width - 24, 24, 32, height - 48, 2, &pos);
    draw_dialog_piece(&spr, 24, height - 24, width - 48, 32, 3, &pos);
    draw_dialog_piece(&spr, -8, -8, 32, 32, 4, &pos);
    draw_dialog_piece(&spr, width - 24, -8, 32, 32, 5, &pos);
    draw_dialog_piece(&spr, -8, height - 24, 32, 32, 6, &pos);
    draw_dialog_piece(&spr, width - 24, height - 24, 32, 32, 7, &pos);
    draw_dialog_piece(&spr, 18, -14, 16, 32, 8, &pos);
    draw_dialog_piece(&spr, 34, -14, width - 69, 32, 9, &pos);
    draw_dialog_piece(&spr, width - 19, -14, -16, 32, 8, &pos);
    spr.tex_flag = 7;
    draw_dialog_piece(&spr, 26, -4, 20, 16, 10, &pos);
    draw_dialog_piece(&spr, 46, -4, width - 91, 16, 11, &pos);
    draw_dialog_piece(&spr, width - 25, -4, -20, 16, 10, &pos);
    if (title != NULL) {
        pos.x += 40;
        pos.y -= 10;
        font_set_size(18, 18);
        font_print_ex(pos.x + 2, pos.y + 1, 1, title);
        flfntSetColor(title_color);
        font_locate(pos.x, pos.y);
        font_print(title);
    }
    return left;
}

/* Draws a plain dialog frame `width` x `height` at (`x`, `y`): the backdrop in `frame_color`, the border in `color`. */
void put_frame_dialog(s16 x, s16 y, s16 width, s16 height, u32 color, u32 frame_color)
{
    _mh_ivec2_ pos;
    _SPR_DATA_ spr;

    pos.x = x;
    pos.y = y;
    memset(&spr, 0, sizeof(spr));
    spr.u_scale = spr.v_scale = 100;
    if (color == 0) {
        color = 0x965A3CFF;
    }
    if (frame_color == 0) {
        frame_color = 0x14120AB2;
    }
    spr.tex_flag = 5;
    spr.color = frame_color;
    draw_dialog_piece(&spr, 6, 8, width - 13, height - 20, 0, &pos);
    memset(&spr, 0, sizeof(spr));
    spr.u_scale = spr.v_scale = 100;
    spr.tex_flag = 5;
    spr.color = color;
    draw_dialog_piece(&spr, 24, -2, width - 48, 32, 3, &pos);
    draw_dialog_piece(&spr, -8, 24, 32, height - 48, 1, &pos);
    draw_dialog_piece(&spr, width - 24, 24, 32, height - 48, 2, &pos);
    draw_dialog_piece(&spr, 24, height - 24, width - 48, 32, 3, &pos);
    draw_dialog_piece(&spr, -8, -8, 32, 32, 4, &pos);
    draw_dialog_piece(&spr, width - 24, -8, 32, 32, 5, &pos);
    draw_dialog_piece(&spr, -8, height - 24, 32, 32, 6, &pos);
    draw_dialog_piece(&spr, width - 24, height - 24, 32, 32, 7, &pos);
}

/* Draws the screen header plate sized to `text` at (`x` widened by `wide`, `y`) - the default corner when both are 0 -
 * and `text` in `color` inside it. */
extern "C" s16 put_frame_header(s16 x, s16 y, u32 color, s8* text, u32 wide)
{
    s16 left;
    _mh_ivec2_ pos;
    _SPR_DATA_ spr;
    s32 len;
    s32 width;

    if (x == 0 && y == 0) {
        pos.x = get_wide_offset(2) + 16;
        pos.y = 13;
    } else {
        pos.x = x + get_wide_offset(wide);
        pos.y = y;
    }
    if (color == 0) {
        color = 0xF3E8ABFF;
    }
    left = pos.x;
    memset(&spr, 0, sizeof(spr));
    spr.u_scale = spr.v_scale = 100;
    spr.tex_flag = 28;
    spr.color = 0xFFFFFFFF;
    len = flfntStrLen((char*)text);
    draw_dialog_piece(&spr, 0, 0, 54, 44, 12, &pos);
    width = ((len + 1) >> 1) * 26;
    draw_dialog_piece(&spr, 54, 0, width, 44, 13, &pos);
    draw_dialog_piece(&spr, width + 108, 0, -54, 44, 12, &pos);
    if (text != NULL) {
        pos.x += 54;
        pos.y += 8;
        font_set_size(26, 26);
        font_print_ex(pos.x + 2, pos.y + 1, 1, text);
        flfntSetColor(color);
        font_locate(pos.x, pos.y);
        font_print(text);
    }
    return left;
}

/* Draws a label plate `width` wide (sized to `text` when 0) at (`x`, `y`) in `color`, and `text` in `text_color`. */
extern "C" void put_frame_label(s16 x, s16 y, s16 width, u32 color, s16 text_color, s8* text)
{
    _mh_ivec2_ pos;
    _SPR_DATA_ spr;
    s32 len = flfntStrLen((char*)text);

    pos.x = x;
    pos.y = y;
    if (width == 0) {
        width = ((len >> 1) + 1) * 18;
    }
    if (color == 0) {
        color = 0x3C4100FF;
    }
    if (text_color == 0) {
        text_color = (s16)0xB3FF;
    }
    memset(&spr, 0, sizeof(spr));
    spr.u_scale = spr.v_scale = 100;
    spr.tex_flag = 27;
    spr.color = color;
    draw_dialog_piece(&spr, 0, 0, 9, 23, 14, &pos);
    draw_dialog_piece(&spr, 9, 0, width - 18, 23, 15, &pos);
    draw_dialog_piece(&spr, width, 0, -9, 23, 14, &pos);
    if (text != NULL) {
        pos.x += 9;
        pos.y += 2;
        font_set_size(18, 18);
        font_print_ex(pos.x + 2, pos.y + 1, 1, text);
        flfntSetColor(text_color);
        font_locate(pos.x, pos.y);
        font_print(text);
    }
}

/* Draws a list row's separator line `width` x `height` at (`x`, `y`). */
extern "C" void put_frame_separator(s16 x, s16 y, s16 width, s16 height, u32 flags)
{
    _mh_ivec2_ pos;
    _SPR_DATA_ spr;

    pos.x = x;
    pos.y = y;
    memset(&spr, 0, sizeof(spr));
    spr.u_scale = spr.v_scale = 100;
    spr.tex_flag = 7;
    spr.color = 0xFFFFFFFF;
    draw_dialog_piece(&spr, 18, 2, -14, height - 4, 17, &pos);
    draw_dialog_piece(&spr, 18, 2, width - 32, height - 4, 18, &pos);
    draw_dialog_piece(&spr, width - 18, 2, 14, height - 4, 17, &pos);
}

/* Draws the two page arrows beside a `width` x `height` row at (`x`, `y`), each in `hi_color` while its `moved` bit
 * (0x04 left, 0x08 right) is set and in `color` otherwise; nothing while the pointer is in use. */
extern "C" void put_frame_side_arrows(s16 x, s16 y, s16 width, s16 height, u16 moved, u32 color, u32 hi_color)
{
    _mh_ivec2_ pos;
    _SPR_DATA_ spr;
    s32 arrow_y;

    if ((u32)chk_pointer() != 1) {
        pos.x = pos.y = 0;
        memset(&spr, 0, sizeof(spr));
        spr.u_scale = spr.v_scale = 100;
        spr.tex_flag = 7;
        if (color == 0) {
            color = 0x507C2FFF;
        }
        if (hi_color == 0) {
            hi_color = 0x7BCA60FF;
        }
        if ((moved & 4) != 0) {
            spr.color = hi_color;
        } else {
            spr.color = color;
        }
        arrow_y = y + (height >> 1) - 9;
        draw_dialog_piece(&spr, x - 18, arrow_y, 16, 18, 22, &pos);
        if ((moved & 8) != 0) {
            spr.color = hi_color;
        } else {
            spr.color = color;
        }
        draw_dialog_piece(&spr, x + width, arrow_y, 16, 18, 23, &pos);
    }
}

/* Draws the cursor bar `width` x `height` at (`x`, `y`): a blinking glow in `glow_color` under the bar in `color`;
 * nothing while the pointer is in use. */
extern "C" void put_frame_cursor_bar(s16 x, s16 y, s16 width, s16 height, u32 color, u32 glow_color)
{
    _mh_ivec2_ pos;
    _SPR_DATA_ spr;

    if ((u32)chk_pointer() != 1) {
        pos.x = x;
        pos.y = y;
        if (glow_color == 0) {
            glow_color = 0xC88C3750;
        }
        if (color == 0) {
            color = 0xFAA014FF;
        }
        memset(&spr, 0, sizeof(spr));
        spr.u_scale = spr.v_scale = 100;
        spr.tex_flag = 7;
        spr.color = 0xFFFFFFFF;
        spr.pos.x = 3;
        spr.pos.y = 1;
        spr.width = width - 6;
        spr.height = height - 3;
        tex_uv_copy(&spr.uv0, &dialog_piece_uv_tbl[21][0]);
        tex_uv_copy(&spr.uv1, &dialog_piece_uv_tbl[21][1]);
        spr.color = glow_color;
        draw_sprite_blink(&spr, &pos);
        spr.color = color;
        draw_dialog_piece(&spr, 0, 0, 7, height, 19, &pos);
        draw_dialog_piece(&spr, 7, 0, width - 14, height, 20, &pos);
        draw_dialog_piece(&spr, width, 0, -7, height, 19, &pos);
    }
}

/* Draws the message `msg` (glyph size `size`, 18 when 0) centred on screen inside a dialog frame, in `color`. */
void put_message(s8* msg, s16 size, u32 color)
{
    s16 lines;
    s16 center_x;
    s16 center_y;
    s16 text_x;
    s16 text_y;

    font_flush();
    if (size == 0) {
        size = 18;
    }
    if (color == 0) {
        color = 0xFFFFFFFF;
    }
    font_set_size(size, size);
    center_x = get_wide_offset(3) + 320;
    center_y = (s32)Screen_w.height_f >> 1;
    text_x = menu_text_block_center_x((char*)msg, center_x, size, &lines);
    size = center_y - lines * (size >> 1);
    put_frame_dialog(text_x - 40, size - 30, (center_x - (text_x - 40)) * 2, (center_y - (size - 30)) * 2, 0, 0);
    flfntSetColor(color);
    font_locate(text_x, size);
    font_print(msg);
}

/* Draws the yes/no question `msg` centred on screen in a window, with the `yes`/`no` answers below it and the cursor
 * bar on answer `sel` (0 = yes). */
extern "C" void put_message_window(s8* msg, s8* yes, s8* no, s32 sel, s16 size, u32 color)
{
    s16 lines;
    s16 center_x;
    s16 center_y;
    s16 text_x;
    s16 text_y;
    s32 left;
    s32 top;
    s16 width;
    s16 height;
    u32 yes_len;
    u32 no_len;
    u32 len;
    s16 pad;
    s32 bar_w;
    s32 bottom;
    s32 bar_y;
    s32 yes_x;
    s32 no_x;
    s32 label_y;

    font_flush();
    if (size == 0) {
        size = 18;
    }
    if (color == 0) {
        color = 0xFFFFFFFF;
    }
    font_set_size(size, size);
    center_x = get_wide_offset(3) + 320;
    center_y = (s32)Screen_w.height_f >> 1;
    text_x = menu_text_block_center_x((char*)msg, center_x, size, &lines);
    text_y = center_y - lines * (size >> 1) - 20;
    left = text_x - 40;
    width = (center_x - left) * 2;
    top = text_y - 30;
    height = (center_y - top) * 2;
    if (width < 340) {
        draw_window_box(center_x - 170, top, 340, height, 0x926239FF, 0x1C190FFF);
    } else {
        draw_window_box(left, top, width, height, 0x926239FF, 0x1C190FFF);
    }
    yes_len = flfntStrLen((char*)yes);
    no_len = flfntStrLen((char*)no);
    len = yes_len < no_len ? no_len : yes_len;
    if (len > 6) {
        pad = (len - 6) * 9;
    } else {
        pad = 0;
    }
    bar_w = pad + 74;
    bottom = center_y + (height >> 1);
    bar_y = bottom - 50;
    yes_x = center_x - 80 - pad;
    put_frame_separator(yes_x, bar_y, bar_w, 30, 0);
    no_x = center_x + 6;
    put_frame_separator(no_x, bar_y, bar_w, 30, 0);
    if (sel == 0) {
        put_frame_cursor_bar(yes_x, bar_y, bar_w, 30, 0, 0);
    } else {
        put_frame_cursor_bar(no_x, bar_y, bar_w, 30, 0, 0);
    }
    flfntSetColor(color);
    font_locate(text_x, text_y);
    font_print(msg);
    font_set_size(18, 18);
    flfntSetColor(0xFFFFFFFF);
    label_y = bottom - 44;
    font_locate(yes_x + ((u32)(bar_w - yes_len * 9) >> 1), label_y);
    font_print(yes);
    font_locate(no_x + ((u32)(bar_w - no_len * 9) >> 1), label_y);
    font_print(no);
}

/* Draws the system message `text` in `text_color` inside the fixed system dialog frame (`color`, `frame_color`). */
void put_message_sys(s8* text, u32 text_color, u32 color, u32 frame_color)
{
    s32 x;

    font_flush();
    if (text_color == 0) {
        text_color = 0xFFFFFFFF;
    }
    font_set_size(18, 18);
    x = get_wide_offset(3) + 120;
    if (frame_color == 0) {
        frame_color = 0x14120AE6;
    }
    put_frame_dialog(x, 120, 400, 198, color, frame_color);
    flfntSetColor(text_color);
    font_locate(x + 15, 150);
    font_print(text);
}

/* Draws the system dialog's OK button: the icon index is 2 when the option row 7 is set and 0 when it
 * is not, at the dialog's own button x. */
void put_message_sys_ok_button(void)
{
    s32 x = get_wide_offset(3) + 470;

    if (get_option_cfg(7) != 0) {
        put_button_icon(x, 268, 32, -1, 2, 0);
    } else {
        put_button_icon(x, 268, 32, -1, 0, 0);
    }
}

/* Steps `value` down when `keys` holds the `dec_mask` bits and up when it holds `inc_mask`, wrapping inside
 * [0, count); plays the SE (unless it is -1) and reports the mask that moved it through `*moved`. */
extern "C" s32 menu_cursor_wrap(s32 value, s32 count, u16 keys, u16 dec_mask, u16 inc_mask, s32 sfx, u16* moved)
{
    u16 mask = 0;

    if (count > 1) {
        if ((keys & dec_mask) != 0) {
            if (sfx != -1) {
                sysSE_req(sfx);
            }
            mask = dec_mask;
            value--;
            if (value < 0) {
                value = count - 1;
            }
        } else if ((keys & inc_mask) != 0) {
            if (sfx != -1) {
                sysSE_req(sfx);
            }
            mask = inc_mask;
            value++;
            if (value >= count) {
                value = 0;
            }
        }
    }
    if (moved != NULL) {
        *moved = mask;
    }
    return value;
}

/* The four forwarders onto `menu_cursor_wrap`'s cursor step, each supplying a different tail of the
 * argument list; the wrapped index they report is what the menu's column cursor stores. */
extern "C" s32 menu_cursor_step_fixed_tail(s32 a, s32 b, u16 c, u16 d, u16 e, u16* moved)
{
    return menu_cursor_wrap(a, b, c, d, e, 3, moved);
}

/* Same step with the sixth argument zeroed. */
extern "C" s32 menu_cursor_step_open_last(s32 a, s32 b, u16 c, u16 d, u16 e, s32 f)
{
    return menu_cursor_wrap(a, b, c, d, e, f, 0);
}

/* Same step, both tail arguments passed through. */
extern "C" s32 menu_cursor_step_forward(s32 a, s32 b, u16 c, u16 d, u16 e, s32 f, u16* moved)
{
    return menu_cursor_wrap(a, b, c, d, e, f, moved);
}

/* Same step with the two fixed tail values 3 and 0 - the entry point the cursor call sites use. */
extern "C" s32 menu_cursor_step(s32 a, s32 b, u16 c, u16 d, u16 e)
{
    return menu_cursor_wrap(a, b, c, d, e, 3, 0);
}

/* Ceiling of `a / b`, with b == 0 reported as 1. */
extern "C" s16 menu_page_count(s16 a, s16 b)
{
    s16 q;

    if (b == 0) {
        return 1;
    }
    q = a / b;
    if (a % b == 0) {
        return q;
    }
    return q + 1;
}

/* Steps a two-state selector (`*state` is 0 or 1) from the pad word `keys`: 0x10 confirms (1 when the state was
 * 0, else 2) and 0x20 selects state 1 (2 when it already was), while `dec_mask` / `inc_mask` move it to 0 / 1.
 * The move SE is skipped for `sfx` 0xFFFE.  Returns 0 when it only moved. */
/* untyped: caller-owned payload - the callers pass their own `s32 stepper_*` / `u32` state word */
extern "C" s32 toggle_word_step(void* state, u16 keys, u16 dec_mask, u16 inc_mask, u16 sfx)
{
    s32* word = (s32*)state;

    if ((keys & 0x10) != 0) {
        if (*word == 0) {
            if (sfx < 0xFFFE) {
                sysSE_req(0);
            }
            return 1;
        }
        if (sfx != 0xFFFE) {
            sysSE_req(0);
        }
        return 2;
    }
    if ((keys & 0x20) != 0) {
        if (*word != 1) {
            *word = 1;
            sysSE_req(1);
        } else {
            if (sfx != 0xFFFE) {
                sysSE_req(1);
            }
            return 2;
        }
    } else if ((keys & dec_mask) != 0) {
        if (*word != 0) {
            *word = 0;
            sysSE_req(3);
        }
    } else if ((keys & inc_mask) != 0) {
        if (*word != 1) {
            *word = 1;
            sysSE_req(3);
        }
    }
    return 0;
}

/* The pad-only form of `toggle_word_step`: no SE override. */
/* untyped: caller-owned payload - the callers pass their own `s32 stepper_*` / `u32` state word */
extern "C" s32 toggle_word_step_dpad(void* state, u16 keys, u16 dec_mask, u16 inc_mask)
{
    return toggle_word_step(state, keys, dec_mask, inc_mask, 0);
}

/* Draws the yes/no dialog at layout `id`: its frame, the two answers (the chosen one highlighted) and the cursor on
 * answer `sel` (0 = yes). */
extern "C" void menu_yes_no_draw(u16 id, s32 sel)
{
    _mh_ivec2_ pos;
    _SPR_DATA_ spr;
    u8** strs;
    bool yes_on;
    bool no_on;

    get_lsp_data(id, &pos);
    draw_sprite_ary(dialog_sprite_ids_frame, &pos);
    if (sel == 0) {
        yes_on = true;
        no_on = false;
    } else {
        yes_on = false;
        no_on = true;
    }
    strs = get_str_tbl(67);
    spr_data_copy(&spr, get_lsp_data(430, NULL));
    spr.color = GetMenuFontColor(true, yes_on, true, false);
    draw_font(spr, (s8*)strs[0], 1, &pos);
    spr_data_copy(&spr, get_lsp_data(438, NULL));
    spr.color = GetMenuFontColor(true, no_on, true, false);
    draw_font(spr, (s8*)strs[1], 1, &pos);
    put_menu_cursor(sel == 0 ? dialog_sprite_ids_a : dialog_sprite_ids_b, 0, &pos);
}

/* Moves the 1-based cursor inside [1, max] (`keys` 0x04/0x08 first/last, `held` 0x01/0x02 up/down), plays the move SE
 * and reports the moving bit through `*moved`; returns 1 for button 0x10, 2 for 0x20. */
extern "C" s32 menu_cursor_page_move(s16* cursor, s16 max, u16 keys, u16 held, u16* moved)
{
    s32 result = 0;

    *moved = 0;
    if ((keys & 0x10) != 0) {
        *moved = 0x10;
        result = 1;
    } else if ((keys & 0x20) != 0) {
        *moved = 0x20;
        result = 2;
    } else if ((keys & 4) != 0) {
        if (*cursor != 1) {
            *cursor = 1;
            *moved = 4;
            sysSE_req(3);
        }
    } else if ((keys & 8) != 0) {
        if (max != *cursor) {
            *cursor = max;
            *moved = 8;
            sysSE_req(3);
        }
    } else if ((held & 1) != 0) {
        if (*cursor < max) {
            (*cursor)++;
            *moved = 1;
            sysSE_req(3);
        }
    } else if ((held & 2) != 0) {
        if (*cursor > 1) {
            (*cursor)--;
            *moved = 2;
            sysSE_req(3);
        }
    }
    return result;
}

/* Sets the scroll record up for `total` entries at `rows_per_page` rows a page with the cursor on the flat
 * entry `index`: the page count, the last page's row count, and the cursor's page/row split. */
extern "C" void menu_scroll_init(MenuScroll* scroll, u16 index, u16 total, u8 rows_per_page)
{
    s32 rows;
    s32 pages;
    s32 page;
    u8 limit;

    scroll->index = index;
    scroll->rows_per_page = rows_per_page;
    rows = rows_per_page;
    pages = (u8)((total + rows - 1) / rows);
    scroll->col_count = pages;
    limit = total - rows * (pages - 1);
    scroll->field_0x007 = limit;
    page = index / rows;
    scroll->col = page;
    scroll->row = index % rows;
    if (page < pages - 1) {
        limit = rows;
    }
    scroll->row_limit = limit;
    scroll->move_flags = 0;
}

/* Moves the scroll cursor from `buttons`: 0x04/0x08 turn the page (SE `sfx`), 0x01/0x02 move the row (SE 3), both
 * wrapping; the move bits go to `move_flags` and the flat index is recomputed. */
extern "C" void menu_scroll_step(MenuScroll* scroll, u16 buttons, s32 sfx)
{
    s16 col;
    s16 row;
    u8 limit;

    scroll->move_flags = 0;
    row = scroll->row;
    col = scroll->col;
    if ((buttons & 0xC) != 0 && scroll->col_count > 1) {
        if ((buttons & 4) != 0) {
            col--;
            if (col < 0) {
                col = scroll->col_count - 1;
            }
            scroll->move_flags |= 4;
        } else {
            col++;
            if (col >= scroll->col_count) {
                col = 0;
            }
            scroll->move_flags |= 8;
        }
        if (col >= scroll->col_count - 1) {
            limit = scroll->field_0x007;
        } else {
            limit = scroll->rows_per_page;
        }
        scroll->row_limit = limit;
        if (row >= limit) {
            row = limit - 1;
            scroll->row = row;
        }
        scroll->col = col;
        sysSE_req(sfx);
    } else if ((buttons & 3) != 0) {
        limit = scroll->row_limit;
        if (limit > 1) {
            if ((buttons & 1) != 0) {
                row--;
                if (row < 0) {
                    row = limit - 1;
                }
                scroll->move_flags |= 1;
            } else {
                row++;
                if (row >= limit) {
                    row = 0;
                }
                scroll->move_flags |= 2;
            }
            sysSE_req(3);
        }
        scroll->row = row;
    }
    scroll->index = row + scroll->rows_per_page * col;
}

/* The x a single line of text starts at so that it is centred on `center`: half the line's width, where
 * a glyph is `size / 2` wide. */
extern "C" s16 menu_text_center_x(char* text, s32 center, s16 size)
{
    return center - (size * flfntStrLen(text)) / 4;
}

/* The x a block of newline-separated lines starts at so that its widest line is centred on `center`;
 * stores the number of lines in `*out_lines` when the caller wants it. */
extern "C" s16 menu_text_block_center_x(char* text, s32 center, s16 size, s16* out_lines)
{
    s32 lines;
    s32 widest;
    char* start;

    start = text;
    lines = 0;
    widest = 0;
    while (*text != 0) {
        s32 width;

        text = flfntStrChr(text, 10);
        lines++;
        if (text != NULL) {
            width = text - start;
            start = text;
        } else {
            width = flfntStrLen(start);
        }
        if (width < 0) {
            width = -width;
        }
        if (widest < width) {
            widest = width;
        }
        if (text == NULL) {
            break;
        }
        text++;
    }
    if (out_lines != NULL) {
        *out_lines = lines;
    }
    return center - (widest >> 1) * (size >> 1);
}

/* The x a line starts at when its glyph cells are `size` apart, so a line of n glyphs spans n - 1 gaps. */
extern "C" s16 menu_text_center_x_by_gaps(char* text, s32 center, s16 size)
{
    return center - (size * (flfntStrLen(text) - 1)) / 2;
}

/* Stores as the new pick the next of the 26 slots after the current one whose item is pickable and not kind 1;
 * returns -1 when the list is empty. */
extern "C" s32 menu_item_pick_seek(_PLW* plw, IdValue* pairs)
{
    s32 left;
    u32 slot = plw->field_0x304;
    s32 filled;

    if (pairs[slot].id != 0) {
        return (s16)slot;
    }
    left = 0x1A;
    filled = 0;
    do {
        if (pairs[slot].id != 0) {
            ItemDataRecord* record;

            filled++;
            record = GetItemData(pairs[slot].id);
            if ((record->field_0x002 & 8) != 0 && record->kind_0x00 != 1) {
                plw->field_0x304 = slot;
                return (s16)slot;
            }
        }
        slot++;
        if (slot >= 0x1A) {
            slot = 0;
        }
        left--;
    } while (left != 0);
    return -(filled == 0);
}

/* The sort key of one list slot: 0 for an empty slot, else the item record's sort word in the high half
 * over the item id. */
extern "C" u32 item_pair_sort_key(IdValue* pair)
{
    u32 key = 0;

    if (pair->id != 0) {
        u32 hi = GetItemData(pair->id)->sort_order_0x006 << 16;

        key = hi + pair->id;
    }
    return key;
}

/* Shell-sorts the first `count` slots by their sort key, ties broken by the larger count first. */
extern "C" void item_pairs_shell_sort(IdValue* pairs, u32 count)
{
    s32 gap = 1;
    IdValue pivot;

    while (gap < count) {
        gap = gap * 3 + 1;
    }
    gap /= 3;
    while (gap >= 1) {
        u32 i;

        for (i = gap; i < count; i++) {
            u32 key;
            s32 j;

            item_pair_copy(&pivot, &pairs[i]);
            key = item_pair_sort_key(&pivot);
            for (j = i - gap; j >= 0; j -= gap) {
                u32 other = item_pair_sort_key(&pairs[j]);

                if (other > key || (other == key && pivot.value > pairs[j].value)) {
                    item_pair_copy(&pairs[j + gap], &pairs[j]);
                } else {
                    break;
                }
            }
            item_pair_copy(&pairs[j + gap], &pivot);
        }
        gap /= 3;
    }
}

/* Compacts the first `count` slots so every filled one comes first (each hole takes the next filled slot's
 * place), then orders the filled run with `item_pairs_shell_sort`. */
extern "C" void item_pairs_compact_sort(IdValue* pairs, s32 count)
{
    IdValue* slot = pairs;
    IdValue* next;
    s32 left;
    s32 filled;

    filled = 0;
    left = count;
    while (left != 0) {
        if (slot->id != 0) {
            filled++;
        } else {
            s32 remaining;

            next = slot + 1;
            remaining = left - 1;
            for (; remaining != 0; remaining--, next++) {
                if (next->id != 0) {
                    item_pair_copy(slot, next);
                    next->id = 0;
                    next->value = 0;
                    filled++;
                    break;
                }
            }
        }
        left--;
        slot++;
    }
    item_pairs_shell_sort(pairs, filled);
}

/* Inserts `*pair` into the 8-slot ordered `table` (an empty slot, or before the first it sorts ahead of, rippling the
 * rest down); `*pair` is then cleared, or holds the pair pushed off a full table. */
extern "C" void item_pair_table_insert(IdValue* pair, IdValue* table)
{
    IdValue carry;
    IdValue next;
    IdValue* slot = table;
    s32 i = 0;
    s32 j;

    for (; i < 8; i++, slot++) {
        if (slot->id == 0) {
            item_pair_copy(slot, pair);
            pair->id = 0;
            pair->value = 0;
            return;
        }
        if (pair->id <= slot->id && (pair->id != slot->id || pair->value >= slot->value)) {
            item_pair_copy(&carry, slot);
            item_pair_copy(slot, pair);
            j = i + 1;
            slot++;
            for (; j < 8; j++, slot++) {
                if (slot->id == 0) {
                    item_pair_copy(slot, &carry);
                    pair->id = 0;
                    pair->value = 0;
                    return;
                }
                item_pair_copy(&next, slot);
                item_pair_copy(slot, &carry);
                item_pair_copy(&carry, &next);
            }
            item_pair_copy(pair, &carry);
            return;
        }
    }
}

/* Folds the 8-slot `page` and the kind-1 items of the 24-slot `pool` into one ordered page, writes that back
 * to `page`, and compacts the pool. */
extern "C" void item_pages_merge(IdValue* pool, IdValue* page)
{
    IdValue merged[8];
    IdValue* slot;
    s32 i;

    merged[0].id = 0;
    merged[0].value = 0;
    merged[1].id = 0;
    merged[1].value = 0;
    merged[2].id = 0;
    merged[2].value = 0;
    merged[3].id = 0;
    merged[3].value = 0;
    merged[4].id = 0;
    merged[4].value = 0;
    merged[5].id = 0;
    merged[5].value = 0;
    merged[6].id = 0;
    merged[6].value = 0;
    merged[7].id = 0;
    merged[7].value = 0;
    slot = page;
    for (i = 0; i < 8; i++, slot++) {
        if (slot->id != 0) {
            item_pair_table_insert(slot, merged);
        }
    }
    slot = pool;
    for (i = 0; i < 24; i++, slot++) {
        if (slot->id != 0 && GetItemData(slot->id)->kind_0x00 == 1) {
            item_pair_table_insert(slot, merged);
        }
    }
    memcpy(page, merged, 0x20);
    item_pairs_compact_sort(pool, 24);
}

/* Orders a 24-slot item pool: on its own when there is no second page, else together with that page. */
extern "C" void item_pages_sort(IdValue* pool, IdValue* page)
{
    if (page == NULL) {
        item_pairs_compact_sort(pool, 24);
    } else {
        item_pages_merge(pool, page);
    }
}

/* Re-sorts the item pool and its second page, then points the player's pick and selected item slot back at the items
 * they named before the sort. */
extern "C" void menu_item_pages_resort(_PLW* plw, IdValue* pool, IdValue* page)
{
    u8 saved;
    IdValue picked;
    IdValue selected;
    s32 has_selected;
    s32 index;

    menu_item_pick_seek(plw, pool);
    item_pair_copy(&picked, &pool[plw->field_0x304]);
    has_selected = 0;
    if (page != NULL) {
        plw->item_slot_sel = pl_item_slot_next_get(plw, plw->item_slot_sel, 2);
        saved = plw->field_0x269;
        pl_held_item_refresh(plw);
        plw->field_0x269 = saved;
        if (plw->item_slot_sel != 0xFF) {
            if ((plw->item_slot_sel & 0x80) != 0) {
                item_pair_copy(&selected, &page[plw->item_slot_sel & 0x7F]);
            } else {
                item_pair_copy(&selected, &pool[plw->item_slot_sel]);
            }
            has_selected = 1;
        }
    }
    item_pages_sort(pool, page);
    index = item_pair_index_find(picked.id, pool, 26);
    if (index >= 0) {
        plw->field_0x304 = index;
    }
    if (has_selected != 0) {
        plw->item_slot_sel = item_slot_code_find(pool, page, selected.id);
    }
}

/* The slot code of `id` in the item lists: the index of its slot in the 9-slot `page` with the 0x80 bit set
 * when it is there, else its index in the 24-slot `pool`, else 0xFF. */
extern "C" u16 item_slot_code_find(IdValue* pool, IdValue* page, u16 id)
{
    s32 index = item_pair_index_find(id, page, 9);

    if (index >= 0) {
        return (u16)index | 0x80;
    }
    index = item_pair_index_find(id, pool, 24);
    if (index >= 0) {
        return (u16)index;
    }
    return 0xFF;
}

/* Whether slot `index` addresses the second page (only a two-page list has one, from slot 24 on). */
extern "C" s32 menu_slot_index_is_page_b(MenuListWork* self, s16 index)
{
    if (self->two_page_0x010 != 0 && index >= 24) {
        return 1;
    }
    return 0;
}

/* Whether the item may go in slot `index`: anything but a kind-1 item is refused on the second page. */
extern "C" u32 menu_item_slot_accepts(MenuListWork* self, u16 id, s16 index)
{
    if (GetItemData(id)->kind_0x00 != 1 && menu_slot_index_is_page_b(self, index) != 0) {
        return 0;
    }
    return 1;
}

/* The list slot `index` names: the first page's, or the second page's from slot 24 on. */
extern "C" IdValue* menu_slot_get(MenuListWork* self, s16 index)
{
    if (self->two_page_0x010 != 0 && index >= 24) {
        return &self->page_b_0x198[index - 24];
    }
    return &self->page_a_0x194[index];
}

/* The slot index of item `id` (first page 0-23, second page 24-31), or 0xFF when it is in neither. */
extern "C" s16 menu_slot_find(MenuListWork* self, u16 id)
{
    IdValue* slot = self->page_a_0x194;
    u16 index = 0;
    s32 group;

    for (group = 0; group < 3; group++) {
        if (slot[0].id == id) {
            return index;
        }
        index++;
        if (slot[1].id == id) {
            return index;
        }
        index++;
        if (slot[2].id == id) {
            return index;
        }
        index++;
        if (slot[3].id == id) {
            return index;
        }
        index++;
        if (slot[4].id == id) {
            return index;
        }
        index++;
        if (slot[5].id == id) {
            return index;
        }
        index++;
        if (slot[6].id == id) {
            return index;
        }
        index++;
        if (slot[7].id == id) {
            return index;
        }
        index++;
        slot += 8;
    }
    if (self->two_page_0x010 != 0) {
        s32 i;

        slot = self->page_b_0x198;
        for (i = 0; i < 8; i++, slot++) {
            if (slot->id == id) {
                return i + 24;
            }
        }
    }
    return 0xFF;
}

/* Draws a window of `count` text rows `texts` at `pos`, the cursor on row `sel`; each row is printed with its entry of
 * `flags` (all 0 when `flags` is NULL). */
extern "C" void menu_select_window_draw(u8 sel, u8 count, s8** texts, const _mh_ivec2_* pos, s16* flags)
{
    s16 no_flags;
    _mh_ivec2_ row;
    _SPR_DATA_* lsp;
    s32 step;
    s32 i;
    s16 x;
    s16 y;

    draw_window_box(pos->x, pos->y, 112, (count + 1) * 30, 0, 0);
    row.x = pos->x + 9;
    row.y = pos->y + 17;
    lsp = get_lsp_data(740, NULL);
    font_set_size(lsp->width, lsp->height);
    if (flags != NULL) {
        step = 1;
    } else {
        flags = &no_flags;
        no_flags = 0;
        step = 0;
    }
    for (i = 0; i < count; i++) {
        draw_sprite_ary((u16*)get_menu_lsp_tbl(43), &row);
        x = menu_text_center_x((char*)*texts, (s16)(row.x + lsp->pos.x), lsp->width);
        y = row.y + lsp->pos.y;
        if (i == sel) {
            put_menu_cursor((u16*)get_menu_lsp_tbl(44), 0, &row);
            font_print_ex(x + 2, y + 1, 1, (s8*)"%s", *texts);
        }
        font_print_ex(x, y, *flags, (s8*)"%s", *texts);
        flags += step;
        row.y += 30;
        texts++;
    }
}

/* Draws a window of `count` menu entries at `pos`: each entry's text in its own colour (or grey/white by its
 * pickable/selected bytes), the selected one with a shadow and the cursor. */
extern "C" void menu_choice_window_draw(MenuEntry* entries, u8 count, _mh_ivec2_* pos)
{
    _mh_ivec2_ row;
    _SPR_DATA_* lsp;
    s32 i;
    s16 x;
    s16 y;
    u32 color;

    draw_window_box(pos->x, pos->y, 112, (count + 1) * 30, 0, 0);
    row.x = pos->x + 9;
    row.y = pos->y + 17;
    lsp = get_lsp_data(740, NULL);
    font_set_size(lsp->width, lsp->height);
    for (i = 0; i < count; i++) {
        draw_sprite_ary((u16*)get_menu_lsp_tbl(43), &row);
        x = menu_text_center_x((char*)entries->field_0x08, (s16)(row.x + lsp->pos.x), lsp->width);
        y = row.y + lsp->pos.y;
        color = entries->field_0x0C;
        if (color == 0) {
            if (entries->field_0x02 != 0) {
                if (entries->selected != 0) {
                    font_print_ex(x + 2, y + 1, 1, (s8*)"%s", (s8*)entries->field_0x08);
                    color = 0xF0F0F0FF;
                } else {
                    color = 0xC3C3C3FF;
                }
            } else {
                color = 0x646464FF;
            }
        }
        font_locate(x, y);
        flfntSetColor(color);
        font_print((s8*)"%s", (s8*)entries->field_0x08);
        if (entries->selected != 0) {
            put_menu_cursor((u16*)get_menu_lsp_tbl(44), 0, &row);
        }
        row.y += 30;
        entries++;
    }
}

/* Draws the window at layout `id` with `count` entry rows: the row frame `frame_ids` (its first two words the row and
 * text layout ids), each entry's text in its colour, and the cursor sprites `cursor_ids` on the selected one. */
extern "C" void menu_entry_window_draw(u16 id, u32* frame_ids, u32* cursor_ids, MenuEntry* rows, u8 count)
{
    _mh_ivec2_ box;
    _mh_ivec2_ row;
    _SPR_DATA_ spr;
    _SPR_DATA_* lsp;
    u16* frame = (u16*)frame_ids;
    s32 i;
    u32 color;
    u32 flags;

    lsp = get_lsp_data(id, &box);
    draw_window_box(box.x, box.y, lsp->width, lsp->height, 0, 0);
    get_lsp_data(frame[0], &row);
    for (i = 0; i < count; i++) {
        draw_sprite_ary(&frame[2], &row);
        flags = 1;
        color = rows->field_0x0C;
        if (color == 0) {
            if (rows->field_0x02 != 0) {
                if (rows->selected != 0) {
                    flags |= 4;
                    color = 0xF0F0F0FF;
                } else {
                    color = 0xC3C3C3FF;
                }
            } else {
                color = 0x646464FF;
            }
        }
        spr_data_copy(&spr, get_lsp_data(frame[1], NULL));
        spr.color = color;
        draw_font(spr, (s8*)rows->field_0x08, flags, &row);
        if (rows->selected != 0) {
            put_menu_cursor((u16*)cursor_ids, 0, &row);
        }
        row.y += 30;
        rows++;
    }
}

/* The menu text colour (RGBA8888): `d == 1` first, `a == 0` (disabled) second, then `c`/`b` pick one of four grey/amber
 * shades; the arms are in the order the target branches on them. */
s32 GetMenuFontColor(bool a, bool b, bool c, bool d)
{
    if (d == 1) {
        return 0xF3D73EFF;
    }
    if (a == 0) {
        return 0x646464FF;
    }
    if (c == 1) {
        if (b == 1) {
            return 0xF0F0F0FF;
        } else {
            return 0xC3C3C3FF;
        }
    }
    if (b == 1) {
        return 0xE8A40FFF;
    }
    return 0x878787FF;
}

/* The same palette's red variant: `a == 0` first, then the `c`/`(d | b)` pair and the last shade. */
s32 GetMenuFontColorRed(bool a, bool b, bool c, bool d)
{
    if (a == 0) {
        return 0x870F0FFF;
    }
    if (c == 1) {
        if (d == 1 || b == 1) {
            return 0xF53737FF;
        }
        return 0xDC2323FF;
    }
    return 0xAA1414FF;
}

/* The same palette's icon variant: same arm order as the red one, with white where the "selected"
 * combination lands. */
s32 GetMenuIconColor(bool a, bool b, bool c, bool d)
{
    if (a == 0) {
        return 0x8A8A8AFF;
    }
    if (c == 1) {
        if (d == 1 || b == 1) {
            return 0xFFFFFFFF;
        }
        return 0xDFDFDFFF;
    }
    return 0xAAAAAAFF;
}

/* Draws `spr` additively with its alpha scaled by the cockpit blink factor (136..256 of 256). */
extern "C" void draw_sprite_blink(_SPR_DATA_* spr, const _mh_ivec2_* pos)
{
    u16 alpha = (s32)(120.0f * cockpit_blink_get()) + 136;

    set_blendmode(4, 1, 1);
    spr->color = (spr->color & 0xFFFFFF00) + (u16)(((u8)spr->color * alpha) >> 8);
    draw_sprite(*spr, pos);
    set_blendmode(4, 5, 1);
}

/* The string-table accessors: the `index`-th string of `get_str_tbl(group)`, group 0 the item names, 1 the player-slot
 * names, 3 the digit glyphs; group 2's content is unpinned, a GUESS, so its accessor keeps an index-based name. */
extern "C" s8* get_item_name_str(u8 index)
{
    return (s8*)get_str_tbl(0)[index];
}

/* The `index`-th name of the player-slot string group. */
extern "C" s8* get_player_name_str(u8 index)
{
    return (s8*)get_str_tbl(1)[index];
}

/* The `index`-th name of string group 2. */
extern "C" s8* get_group2_name_str(u8 index)
{
    return (s8*)get_str_tbl(2)[index];
}

/* The `index`-th name of the digit-glyph string group. */
extern "C" s8* get_digit_str(u8 index)
{
    return (s8*)get_str_tbl(3)[index];
}

/* Hands the pad layer channel `chan` the slot map `map` of the four per-slot value runs. */
extern "C" void menu_pad_map_set(s32 chan, s32 map)
{
    pad_wait_table_set((u8)chan, menu_pad_slot_map_tbl[(u8)map]);
}
