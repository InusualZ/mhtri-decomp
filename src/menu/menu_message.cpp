/*
 * menu/menu_message.cpp - the menu band's list/cursor layer and its message helpers.
 *
 * `.text` 0x802A6624-0x802AA764 (71 functions, 16704 B), extab 0x800137D4-0x8001398C and extabindex
 * 0x8003123C-0x800314D0 (55 records each, one per framed function), `.data` 0x805CE00C-0x805CE040.
 * 48 of 71 rows written, 42 byte-identical, 23 unwritten; objdiff unit metric 44.78 %; still `NonMatching`.
 * The object emits no `.sdata`: the target's `.sdata` reads (`"%d"` 0x807922A0, `"%s"` 0x807922A4/0x807922A8)
 * belong to bodies that are not written yet.
 *
 * Provenance: registered from `proposal/802A6624_fn_802A6624.cpp` and re-homed here on 2026-09-29; the name is
 * a GUESS from what the range does (the selection list and its cursor, the message/frame dialog) - no
 * `__FILE__` string covers it.  The right edge was cut at 0x802AA764 (the tail is the gunner-shell layer, see
 * `.pi/notes/menu-message-4d56.md` for the cut, the unwind-run partition and the tail's recipe).
 *
 * Seams, both open: the left edge is the proposal's own (`lbl_806BE340` is also read from the range before it,
 * and the first block is `menu_item.h`'s `MenuSlot`); the two `"%s"` copies (0x807922A4 read from 0x802A78F4 and
 * 0x802A7B0C, 0x807922A8 read only at 0x802AA078..0x802AA234) cannot both exist in one TU under `-str reuse`, so
 * a boundary very likely lies somewhere in 0x802A7B80..0x802A9F48.  Not acted on; only data whose referrers all
 * lie below 0x802A7B80 is claimed here (the `.data` table above), and the `"%d"`/`"%s"` `.sdata` claim was tried
 * and dropped (it reclassifies `menu_infomation.cpp`'s three `lbl_80792BE*` externs from rule 12 to rule 2).
 *
 * GUESS names (from the body, not pinned by a caller): `menu_page_count` (ceil-divide of a count by a page
 * size, 0 divisor reported as 1), `menu_text_*center_x*`, `menu_item_pick_seek`, `item_pair*`/`item_pages_*`,
 * `menu_slot_*`, `get_group2_name_str`, `menu_item_row_sprite_ids`; the cursor page move's 0x04/0x08 bits as
 * "first"/"last" is a guess too.
 *
 * Load-bearing source shapes (MWCC 1.3 allocation, measured): a `++x` / `x--` statement instead of
 * `x = x + 1` keeps a narrow local in place (`menu_cursor_move`, `menu_frame_entries_build`,
 * `menu_cursor_page_move`); declaring a local and assigning it later, in the target's register order, decides
 * which callee-saved register it gets (`item_pairs_compact_sort`, `menu_item_pick_seek`,
 * `menu_text_block_center_x`); `menu_frame_entries_build_row` is a four-iteration `for`, not four unrolled
 * blocks; a pooled `u16` result needs the `u16` variable, not a cast (`menu_slot_find`).
 *
 * Residuals (non-identical written rows).
 *   * `menu_page_count` 76.67: target `beqlr ... blr`, ours `bnelr / mr r3,r5 / blr` (60 vs 64 B); the
 *     parameter copies land in other registers (five shapes measured).
 *   * `menu_list_fill` 90.91: register-only - the target keeps `self` in r30 and the record pointer in r29
 *     (ours the other way) and computes each `id` extension before the record test; 520 vs 528 B.
 *   * `menu_scroll_init` 95.00: an inverted branch and a dropped `b` in the target's order, 116 vs 120 B.
 *   * `GetMenuFontColor` 95.59: the target keeps the inner `if (b == 1)` arm as a plain `bne` + `blr` where MWCC
 *     folds the same source into a conditional return, one instruction shorter (four shapes measured, all fold).
 *   * `item_pairs_shell_sort` 98.06 and `menu_frame_page_draw` 98.75: allocation-only (the latter keeps two
 *     locals in the highest callee-saved registers where the target keeps parameters).
 *   * Unwritten bodies are blocked by (a) `.data` reads no claim covers yet (`dialog_piece_uv_tbl` 0x805CE0A0,
 *     `lbl_805CE160`/`lbl_805CE180`/`lbl_805CE18C`, `lbl_805CE090`, the 0x805CE040..0x805CE07C blobs - second half,
 *     unproven), (b) `.sdata2` 120.0f at 0x8079A3F8 (sole referrer `fn_802AA4F4`), (c) callees no header declares
 *     with a usable signature (`fn_802E3D18`, `fn_802D9EA8`, `fn_802E1320`, `get_rare_color`, `draw_number_idx`,
 *     `fn_8027993C`/`fn_80279B84`, the font cluster: `font_print` is `char*` in `unsplit/menu.h` where the map's
 *     variadic `font_print__FPSce` has `crclr`).
 *   * rule-2 debt: `menu/menu_item.cpp`, `menu/fn_8031A6C0.cpp` and `lobby/fn_801EC9F8.cpp` still declare the
 *     record-typed entry points (`menu_slot_*`, `menu_item_slot_accepts`, `menu_list_*`, `menu_cursor_column_step`)
 *     locally because they view the record as `MENU_ITEM_W` / `MenuSel`, not `MenuListWork`; folding the views is a
 *     separate change.  `include/unsplit/lobby.h` and `include/lobby/fn_801F3294.h` declare `menu_cursor_step` as
 *     a C++ five-argument function while this range owns 0x802A8EFC and the target spells it bare.
 *
 * Declaration linkage: every callee the written bodies call is declared the way the map spells it -
 * `extern "C"` for a bare row and global C++ scope for a mangled one (`PutPageArrow`, `get_lsp_data`,
 * `get_wide_offset`, `flfntStrLen`); a declaration in an `extern "C"` block emits the bare stem.
 *
 * Inventory and evidence: `python tools/units/ledger.py unit menu/menu_message.cpp`.
 */

#include "types.h"

#include "Runtime.PPCEABI.H/memcpy.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "ef/fn_800CDB2C.h"
#include "menu/menu_message.h"
#include "menu/menu_item.h"
#include "g3d/g3d_anmchr.h"
#include "fn_80047398.h"
#include "hud/layout.h"
#include "menu/get_pop_dat_ptr.h"

extern "C" char* strcpy(char* dst, const char* src);

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
extern "C" void menu_frame_draw_blocks(void* dst, void* src, s8 a, s8 b, u16 c, s32 d, struct _mh_ivec2_* e);
extern "C" void menu_frame_page_draw(u32* dst, MenuListEntry* entries, s8 a, s8 b, u16 c, u32 d, u8 e);

/* The two source tables the list is built from (map symbols, unclaimed - playbook 29). */
extern MenuSourceRecord lbl_806BE340[10];   /* 2 x 5 records: validity byte + two names */

/* The draw helper that sits in no registered range, so the declaration is this unit's call
 * sites' (rule 2's unsplit case - the address bands interleave, so `include/unsplit/<module>.h` has
 * no sound module to move them to). */
void PutPageArrow(u16* table, s16 a, s16 b, u16 flags, const struct _mh_ivec2_* pos, u8 mode);

/* The sprite-id run (terminated by 0xFFFF) `menu_item_row_draw_values` hands to `draw_sprite_ary` (`.data` 0x805CE00C). */
u16 menu_item_row_sprite_ids[26] = {
    0x00A9, 0x00AA, 0x0089, 0x00A4, 0x008B, 0x00A5, 0x00A6, 0x00A7,
    0x008F, 0x00A8, 0x0093, 0x00AB, 0x00AC, 0x00B2, 0x00B3, 0x00B4,
    0x00B5, 0x00B6, 0x00B7, 0x00B8, 0x00B9, 0x009D, 0x00AD, 0x00AE,
    0xFFFF, 0x0000
};

/* This unit's own bodies, in address order. */
extern "C" void menu_frame_row_draw(s32 select, u32* dst, void* entry, s32 flags, struct _mh_ivec2_* pos);
extern "C" void menu_item_row_draw(u16* item, struct _mh_ivec2_* pos);
extern "C" void menu_item_row_draw_values(u16* item, struct _mh_ivec2_* pos, s32 a, s32 b);
extern "C" void eft052_page_counts_get(u16 id, s32* a, s32* b, s32* c);

/* The list length the active kind's table yields for the current selection: kind 1 walks the move
 * table's records, kind 2 the item-record table's two blocks of five.  An index of 0x80 is the "no
 * selection" case and copies the table's own label into both name buffers.  Returns how many list
 * ids were written. */
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

/* Fills the list at +0x1BE from the active kind's table, skipping the currently selected index, and
 * stores the count.  Kind 2 walks two blocks of five records of the item table, kind 1 the move
 * table's variable-length block.  A negative index means "the current selection". */
extern "C" u8 menu_list_fill(MenuListWork* self, s32 index)
{
    s8* dst = self->list_0x1BE;

    self->list_count_0x1BB = 0;
    if (self->kind_0x00F == 2) {
        u8* rec = (u8*)&lbl_806BE340[0];
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

/* Copies the active kind's record names into the two name buffers: the item table's record for the
 * index, or the move table's.  Index 0x80 is the "no selection" case, which copies the table's own
 * label over both.  Reports whether a record was found. */
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
        MenuSourceRecord* rec = &lbl_806BE340[index];

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

/* Builds the list of two 0x60-byte entry blocks the message frame draws from: copies the 0x60-byte
 * source into a work buffer, folds it through `item_pages_sort`, marks each of the eight entries with its
 * ordinal and whether its source slot is in use, then hands the lot to `menu_frame_draw_blocks` with the LSP
 * position the `flags` word selects.  `mode` 1 adds the 0x800 bit to the draw flags. */
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
    menu_frame_draw_blocks(work, entries, 1, kind, 0, flags, (struct _mh_ivec2_*)lsp);
}

/* The one-block form: fills a single block of four entries from the 0x60-byte source `src` and the
 * optional 0x20-byte mask, then hands it to the `menu_frame_page_draw` draw path.  The ordinals are the
 * constants 0..3, so the block is the frame's own row. */
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
    menu_frame_draw_blocks(a, b, c, d, e, f, (struct _mh_ivec2_*)lsp);
}

/* The menu's own LSP position rows.  `menu_frame_page_draw` draws a four-row page: it takes the page's
 * anchor position, the per-kind offset and a row stride of 26 pixels, then walks the four entries
 * of the block `menu_frame_entries_build`/`menu_frame_entries_build_row` filled.  `mode` 0..2 selects the page style; anything
 * else is a no-op. */
extern "C" void menu_frame_page_draw(u32* dst, MenuListEntry* entries, s8 a, s8 b, u16 c, u32 flags_in, u8 mode)
{
    MenuLspPos pos;
    MenuLspPos rows;
    MenuLspPos* base;
    u16 flags;
    u16* tbl;
    s32 select;
    u32 flags_ = flags_in;
    s32 i;
    s16 dy;
    s16 dx;

    get_lsp_data(((u16*)get_menu_lsp_tbl(361))[mode], (struct _mh_ivec2_*)&pos);
    base = (MenuLspPos*)get_lsp_data(3936, 0);
    dx = pos.x - base->x;
    dy = pos.y - base->y;
    switch (flags_in & 3) {
    case 0:
        tbl = (u16*)get_menu_lsp_tbl(362);
        select = 2;
        break;
    case 1:
        tbl = (u16*)get_menu_lsp_tbl(363);
        select = 3;
        break;
    case 2:
        tbl = (u16*)get_menu_lsp_tbl(364);
        select = 3;
        break;
    default:
        return;
    }
    draw_sprite_anim_ary(tbl, c, (struct _mh_ivec2_*)&pos);
    flags = 0;
    if ((flags_in & 8) != 0) {
        flags |= 4;
    }
    if ((flags_in & 0x10) != 0) {
        flags |= 8;
    }
    PutPageArrow((u16*)get_menu_lsp_tbl(365), a, b, flags, (struct _mh_ivec2_*)&pos, 0);
    base = (MenuLspPos*)get_lsp_data(3961, 0);
    rows.x = dx + base->x;
    rows.y = dy + base->y;
    if (c != 0) {
        flags_ |= 1024;
    }
    for (i = 0; i < 4; i++) {
        menu_frame_row_draw(select, &dst[entries[i].index_0x06], &entries[i], flags_,
                    (struct _mh_ivec2_*)&rows);
        rows.y += 26;
    }
    font_flush();
}

/* Reads the position the caller's LSP id names and draws one item row at it. */
extern "C" void menu_item_row_draw_by_lsp(u16 id, u16* item)
{
    MenuLspPos pos;

    get_lsp_data(id, (struct _mh_ivec2_*)&pos);
    menu_item_row_draw(item, (struct _mh_ivec2_*)&pos);
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
extern "C" s32 toggle_word_step(void* state, u16 keys, s32 dec_mask, s32 inc_mask, s32 sfx)
{
    s32* word = (s32*)state;

    if ((keys & 0x10) != 0) {
        if (*word == 0) {
            if ((u16)sfx < 0xFFFE) {
                sysSE_req(0);
            }
            return 1;
        }
        if ((u16)sfx != 0xFFFE) {
            sysSE_req(0);
        }
        return 2;
    }
    if ((keys & 0x20) != 0) {
        if (*word != 1) {
            *word = 1;
            sysSE_req(1);
        } else {
            if ((u16)sfx != 0xFFFE) {
                sysSE_req(1);
            }
            return 2;
        }
    } else if ((keys & (u16)dec_mask) != 0) {
        if (*word != 0) {
            *word = 0;
            sysSE_req(3);
        }
    } else if ((keys & (u16)inc_mask) != 0) {
        if (*word != 1) {
            *word = 1;
            sysSE_req(3);
        }
    }
    return 0;
}

/* The pad-only form of `toggle_word_step`: no SE override. */
/* untyped: caller-owned payload - the callers pass their own `s32 stepper_*` / `u32` state word */
extern "C" s32 toggle_word_step_dpad(void* state, u16 keys, s32 dec_mask, s32 inc_mask)
{
    return toggle_word_step(state, keys, (u16)dec_mask, (u16)inc_mask, 0);
}

/* The item row at the position the caller supplies: `eft052_page_counts_get` yields the row's two value words
 * (the second already summed), which the `menu_item_row_draw_values` page routine draws.  A null or empty item is
 * skipped. */
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

/* Moves the 1-based cursor `*cursor` inside [1, max] from the pad word `keys` (bits 0x04 / 0x08 jump to the
 * first / last entry) and the held word `held` (bits 0x01 / 0x02 step up / down), plays the move SE and
 * reports the bit that moved it through `*moved`.  Returns 1 for the 0x10 button, 2 for the 0x20 one. */
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

/* Moves the scroll cursor from the pad word `buttons`: bits 0x04 / 0x08 turn the page (wrapping, playing the
 * `sfx` SE), bits 0x01 / 0x02 move the row inside the page (wrapping, SE 3); the move bits go to
 * `move_flags` and the flat index is recomputed. */
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

/* ---------------------------------------------------------------------------------------------------
 * The text-centring helpers and the item-pair list layer (0x802A9508..0x802A9E20).
 * --------------------------------------------------------------------------------------------------- */

/* The x a single line of text starts at so that it is centred on `center`: half the line's width, where
 * a glyph is `size / 2` wide. */
extern "C" s16 menu_text_center_x(char* text, s32 center, s16 size)
{
    return center - (size * flfntStrLen(text)) / 4;
}

/* The x a line starts at when its glyph cells are `size` apart, so a line of n glyphs spans n - 1 gaps. */
extern "C" s16 menu_text_center_x_by_gaps(char* text, s32 center, s16 size)
{
    return center - (size * (flfntStrLen(text) - 1)) / 2;
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

/* The next list slot from the player's current pick that holds an item the pick may land on (one whose
 * record has the pickable bit and is not kind 1), walking the 26 slots once; stores it as the new pick.
 * Returns -1 when the list is empty. */
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

/* Inserts `*pair` into the 8-slot ordered `table`: the first empty slot takes it, otherwise it displaces the
 * first slot it sorts before and the displaced pairs ripple down.  `*pair` is cleared when it found a place
 * and holds the pair pushed off the end when the table was full. */
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

/* ---------------------------------------------------------------------------------------------------
 * The dialog/page draw layer and the palette getters (0x802A7C44, 0x802A8020, 0x802AA3EC..0x802AA4F4).
 * --------------------------------------------------------------------------------------------------- */

/* The palette the menu's own text is drawn in, as RGBA8888 words: `d == 1` is answered first, `a == 0`
 * (the disabled case) second, then `c`/`b` select one of the four grey/amber shades.  The arms are
 * written in the order the target branches on them. */
s32 GetMenuFontColor(bool a, bool b, bool c, bool d)
{
    if (d == 1) {
        return 0xF3D73EFF;
    }
    if (a == 0) {
        return 0x646464FF;
    }
    if (c == 1) {
        if (b != 1) {
            return 0xC3C3C3FF;
        }
        return 0xF0F0F0FF;
    }
    if (b == 1) {
        return 0x878787FF;
    }
    return 0xE8A40FFF;
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

/* The string-table accessors the HUD/cockpit rows read their text from: each picks the `index`-th
 * string of one of the master string table's first groups (`get_str_tbl(group)`).  Group 0 is the
 * item-name group (`ef/eft050.cpp` indexes it by a hold slot's item id), group 1 the player-slot
 * names, group 3 the digit glyphs.  MARKED GUESS: group 2's content is not pinned by this range (its
 * only two call sites are `lobby/lb_companion_ui.cpp`'s), so it keeps an index-based name. */
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
