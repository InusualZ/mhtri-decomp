/*
 * menu/menu_message.cpp - the menu band's list/cursor layer and its message helpers.
 *
 * `.text` 0x802A6624-0x802AD9C0 (139 functions, 29596 B), extab 0x800137D4-0x80013B0C (103 8-byte
 * records) and extabindex 0x8003123C-0x80031710 (103 12-byte records, one per framed function), from
 * `proposal/802A6624_fn_802A6624.cpp`.  `.ctors` 0x8056F374-0x8056F37C (two words) is dtk's own
 * addition and the two functions it names are this unit's constructors.
 *
 * Module `menu`: the left neighbour is `menu/menu_item.cpp`, the range calls into it
 * (`GetItemData__FUs`, `get_menu_lsp_tbl__FUs`) and its own entry points are the menu's
 * (`put_message`, `put_frame_dialog`, `GetMenuFontColor`, `pull_shell_work`).  File name: derived
 * (class 3 + class 4).  Class 1 and class 2 are both empty - no `__FILE__` string covers the range
 * (the only one in this `.data` run is `menu_item.cpp` at 0x805CDFC8, referenced from
 * 0x802A5444/0x802A579C/0x802A64B0, i.e. the range before this one) and the runtime dump answers
 * `zz_XXXXXXXX_` for every unnamed row here - so the name comes from what the range does and is
 * MARKED GUESS: the selection list and its cursor, and the message/frame dialog the window puts up.
 * The unit was registered at the map's stem and re-homed to `menu/menu_message.cpp` +
 * `include/menu/menu_message.h` on 2026-09-29 by `worker/menu-num-8725` (the gate refuses a unit
 * registered at a generated file name).
 *
 * Seam: unproven.  Discovery's own warning says the edge is `--max-bytes`'s cap, not a TU boundary,
 * and the two neighbouring proposals (0x802A5444 and my start) are one run.  The band's first block
 * is the *same object* `menu_item.h` calls `MenuSlot` (the kind byte at +0x0F, the count at +0x16),
 * and the `.bss` table `lbl_806BE340` is referenced from both this range and 0x802A5E64 in the
 * proposal before it - so the cut is very likely through one translation unit.  This unit keeps the
 * proposal's extent because a worker may not re-cut a claim, and reuses the offsets rather than
 * inventing a type (`include/menu/menu_message.h` says which).
 *
 * Second seam, unproven too, and stronger: the trailing block (`pull_shell_work`, 0x802AA764, to
 * `fn_802AD9A8`) is **not the menu module**.  It is the player's gunner-shell pool and actor layer:
 * `_SHELL_W` (the 0x10C record `include/ef.h` documents; `push_shell_work` `memset`s exactly 0x10C),
 * pool `lbl_806AD698`, and `yure_move` is called from `Pl/fn_80288CEC.cpp`, `charmake_move` and
 * `arena_game_task`.  12892 of the 29596 bytes (44 %, 70 of the 139 rows) may belong to another TU.
 * No `__FILE__` string and no pooled literal covers the range, so the boundary cannot be settled from
 * the data here - `tudiscover.py at 0x802AA764` reports only weak signals.  The evidence is in this
 * header for the orchestrator; nothing in the block was written this pass.
 *
 * Naming note: the symbol map has only `fn_XXXXXXXX` for 99 of this range's 139 symbols (the 40
 * named rows are the `put_message`/`put_frame_dialog`/`GetMenuFontColor`/`shell_*`/`serial_*`/
 * `niku_*`/`yure_move` family the runtime dump carries).  2026-09-29: no `fn_XXXXXXXX` this file
 * **defines** is left generated.  The list/cursor block became `menu_list_mode_get`,
 * `menu_list_count_update`, `menu_list_fill`, `menu_list_names_set`, `menu_cursor_move`,
 * `menu_cursor_seek`, `menu_cursor_column_step`; the frame/page draw block `menu_frame_entries_build`,
 * `menu_frame_entries_build_row`, `menu_frame_draw_page`, `menu_frame_page_draw`,
 * `menu_frame_row_draw`, `menu_frame_draw_blocks`; the item-row draw block `menu_item_row_draw`,
 * `menu_item_row_draw_by_lsp`, `menu_item_row_draw_values`, `menu_item_row_draw_pages`,
 * `menu_hold_row_draw_by_lsp`; the pad-driven walk `menu_cursor_wrap`,
 * `menu_cursor_step_fixed_tail`/`_open_last`/`_forward` (an arity family that differs only in the
 * tail arguments it fixes - MARKED GUESS), `toggle_word_step`, `toggle_word_step_dpad`.  The rows
 * added with this pass's bodies are named from their callers: `get_item_name_str`,
 * `get_player_name_str` and `get_digit_str` are the item / player-slot / digit-glyph groups
 * `ef/eft050.cpp` indexes, while `get_group2_name_str`'s group is not pinned and keeps an index name
 * (MARKED GUESS); `draw_dialog_piece` + `dialog_piece_uv_tbl` are the dialog's frame-piece draw and
 * its uv table, and the bodies are pending the `.data` claim below.  `fn_802A8F14` (the `s16`
 * ceiling divide) KEEPS its generated name on purpose - `include/Pl/pl_skill.h` +
 * `src/Pl/pl_skill.cpp` call it and `Pl/` is outside this lane's write scope.
 *
 * Reconstructed: the menu list/cursor layer, the palette getters, the four string-table accessors
 * and the system dialog's OK-button row - 19 of the 139 rows are byte-identical, 1596 of 29596
 * `.text` bytes (5.39262 %), unit metric 12.13434 %.  Rows this pass added: `GetMenuFontColorRed`
 * 0 -> 100, `GetMenuIconColor` 0 -> 100, `get_item_name_str`/`get_player_name_str`/
 * `get_group2_name_str`/`get_digit_str` 0 -> 100 each, `put_message_sys_ok_button__Fv` 0 -> 100,
 * `GetMenuFontColor` 0 -> 95.59.  Every pre-existing row measures exactly what it did before.
 *
 * Residuals, biggest first.
 *   * The `.data` blocker this pass found - `draw_dialog_piece` (132 B) and `put_frame_dialog`
 *     (504 B) are derived but NOT in the source: their bodies read `dialog_piece_uv_tbl` at
 *     0x805CE0A0, which no registered `splits.txt` range covers (only an analyzer `auto_*_data` unit
 *     does), so an `extern` for it would be a rule-12 finding.  The fix is to claim the `.data` run
 *     in this unit's own `splits.txt` and define the 0xC0 bytes - measured before and after, because
 *     a `.data` claim can drop the target's pool relocations (playbook 23/78).  The same run holds
 *     `lbl_805CE160`/`lbl_805CE180`/`lbl_805CE18C` (`fn_802A907C`, 304 B), `lbl_805CE090`
 *     (`fn_802AA68C`, 28 B) and the 0x805CE040..0x805CE07C blobs those pointers name, so one claim
 *     unblocks ~1.3 KB of bodies - it is the first follow-up.
 *   * `GetMenuFontColor` 95.59259: the target keeps the inner `if (b == 1)` arm as a plain
 *     `bne <else>` + `blr` where MWCC folds the same source into a conditional return, one
 *     instruction shorter than the target.  Four shapes were measured (`if`/`else`, `if` + `return`,
 *     a ternary, and the negated `if (b != 1)`) - all four fold; the negated one scores highest
 *     (95.59) because it puts the two colour bodies in the target's order.  Flag-shaped or a
 *     block-ordering pass, not reachable from the source side with this unit's flags so far.
 *   * `put_message` (284 B) + `put_message_sys` (180 B) need the font cluster
 *     (`font_print`/`font_locate`/`flfntSetColor`), whose band home `include/unsplit/menu.h` spells
 *     `font_print(char*)` while the map's own name is the variadic `font_print__FPSce` and the target
 *     call site carries `crclr 4*cr1+eq`.  That one declaration is consumed by every unit including
 *     the band header, so fixing it is its own change (the relocation-name class).
 *   * `toggle_word_step` (280 B) + `toggle_word_step_dpad` (20 B) are fully derived but unwritten:
 *     `toggle_word_step`'s declaration reaches 49 call sites in `lobby/` units through
 *     `include/unsplit/lobby.h`, and the retail call sites narrow three of its five parameters to
 *     `u16` where the header spells them `s32` - writing it means re-measuring those units.
 *   * rule-2 debt left: `src/lobby/fn_801EC9F8.cpp`, `include/lobby/fn_8020C588.h` and
 *     `src/menu/menu_item.cpp` still declare `toggle_word_step`/`toggle_word_step_dpad` locally
 *     rather than including this unit's header (swept to the new names, and given rule-11 markers for
 *     the shared untyped state word).  `src/hud/layout.cpp` + `src/hud/move_work_update.cpp` name the
 *     unit by its old path in prose; both are outside this lane's write scope.
 *
 * Declaration linkage (2026-09-29): every callee the written bodies call is declared the way the map
 * spells it - `extern "C"` for a bare row (`strcpy`, `fn_802A9BB8`, `eft052_page_counts_get` and the
 * two `menu_frame_*` writers, whose forward declarations must also carry the definition's parameter
 * list) and global C++ scope for a mangled one (`PutPageArrow`, whose owner's signature
 * `include/ai/fn_802D44F4.h` carries, and `get_lsp_data`/`get_wide_offset`, which moved out of
 * `include/hud/layout.h`'s `extern "C"` block).  A declaration in that block emits the bare stem,
 * which no link input defines; the object's relocations now spell the target's names name for name,
 * and the change is codegen-neutral (0 of 2189 unit rows moved, DOL hash unchanged).
 *
 * The 120 unwritten bodies, by block:
 *   * `fn_802A6F64`-`fn_802A7CC8` (0x408/0x1B8/0x314/0x140/0x40/0x1C8/0x84/0x40/0x84/0x358 B): the
 *     frame/message page writers.  `fn_802A6F64` and `fn_802A7524` are half-written as declarations
 *     only - their bodies are the two biggest in the block and need the `_SPR_DATA_` record (the
 *     `draw_dialog_piece` + `.data` claim item above).
 *   * `put_frame_dialog`-`fn_802AA6A8` (0x1F8..0xBC B, 20 rows): the `put_message`/`put_frame_dialog`
 *     page layer.  Written call sites are needed first: their bodies build `_SPR_DATA_` copies and call
 *     the variadic `font_print_ex(s16, s16, s16, char*, ...)`, whose `crclr 4*cr1+eq` the target
 *     carries.
 *   * `pull_shell_work`-`yure_move` (0x10..0x12B4 B, ~60 rows): the shell/serial/niku layer.  Every
 *     one of them takes `_SHELL_W` (include/ef.h's union view, whose +0x030..+0x10C run is one pad
 *     block); naming the fields they touch means extending that shared header, which is a separate
 *     change from this registration.  `fn_802AC484` (0x12B4 B) alone is 16 % of the range.
 *   * known non-byte-identical rows: `menu_list_fill` 82.76 (the two-block list fill - the retail loop
 *     is unrolled five-fold with constant displacements and our `for` over `rec[0x130 * j]` is not),
 *     `fn_802A8F14` 76.67 (the ceiling-divide helper: same instruction sequence, the allocator swaps
 *     the two parameters' registers), `menu_cursor_move` 97.58, `menu_frame_page_draw` 97.57,
 *     `menu_frame_entries_build_row` 89.14, `menu_item_row_draw_pages` 90.91.
 *   * rule-2 note: `include/unsplit/lobby.h` and `include/lobby/fn_801F3294.h` declare
 *     `menu_cursor_step` as a C++ five-argument function; this range owns 0x802A8EFC and the target
 *     object's relocations spell it bare, so both declarations want moving to this unit's header
 *     (they cannot be included here - `(10597) illegal function overloading`).
 *
 * Inventory and evidence: `python tools/units/ledger.py unit menu/menu_message.cpp`.
 */

#include "types.h"

#include "Runtime.PPCEABI.H/memcpy.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "ef/fn_800CDB2C.h"
#include "menu/menu_message.h"
#include "hud/layout.h"
#include "menu/get_pop_dat_ptr.h"

extern "C" char* strcpy(char* dst, const char* src);

/* ---------------------------------------------------------------------------------------------------
 * The range's own entry points and the callees its written bodies call.  A `fn_XXXXXXXX` stem is the
 * map's placeholder, so those are declared `extern "C"` (the map's spelling is what objdiff pairs);
 * a mangled map name (`get_move_work_adrs__FUc`) is declared as the function it spells.
 * --------------------------------------------------------------------------------------------------- */

extern "C" u32 quest_move_state_valid_ck(void);
extern "C" s32 menu_cursor_wrap(s32 a, s32 b, u16 c, u16 d, u16 e, s32 f, s32 g);

/* The HUD's 2D element library (`hud/layout.h`, its owner's header) supplies the `_mh_ivec2_` /
 * `_SPR_DATA_` records and every `draw_*`/`put_*` entry this range calls, so none of them is
 * re-declared here (docs/plan.md 6.5 rule 2).  This unit's own `MenuLspPos` is the same 4-byte
 * record; the call sites cast to `_mh_ivec2_*`, which is a no-op. */

void sysSE_req(long id);
u8** get_str_tbl(long index);
u16* get_menu_lsp_tbl(u16 idx);
u8* get_move_work_adrs(u8 kind);
s32 get_move_work_max(u8 kind);
extern "C" void fn_802A9BB8(void* dst, const void* src);
extern "C" void menu_frame_draw_blocks(void* dst, void* src, s8 a, s8 b, u16 c, s32 d, struct _mh_ivec2_* e);
extern "C" void menu_frame_page_draw(u32* dst, MenuListEntry* entries, s8 a, s8 b, u16 c, s32 d, u8 e);

/* The two source tables the list is built from (map symbols, unclaimed - playbook 29). */
extern MenuSourceRecord lbl_806BE340[10];   /* 2 x 5 records: validity byte + two names */

/* The two draw helpers that sit in no registered range, so the declaration is this unit's call
 * sites' (rule 2's unsplit case - the address bands interleave, so `include/unsplit/<module>.h` has
 * no sound module to move them to). */
void PutPageArrow(u16* table, s16 a, s16 b, u16 flags, const struct _mh_ivec2_* pos, u8 mode);
void font_flush(void);

/* This unit's own bodies, in address order. */
extern "C" void menu_frame_row_draw(s32 select, u32* dst, void* entry, s32 flags, struct _mh_ivec2_* pos);
extern "C" s32 menu_cursor_step(s32 a, s32 b, u16 c, u16 d, u16 e);
extern "C" s16 fn_802A8F14(s16 a, s16 b);
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
        } else if (fn_800CF3C4() > 1) {
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
extern "C" u8 menu_list_fill(MenuListWork* self, s8 index_)
{
    s8* dst = self->list_0x1BE;
    s8 index = index_;

    self->list_count_0x1BB = 0;
    if (self->kind_0x00F == 2) {
        u8* rec = (u8*)&lbl_806BE340[0];
        int block;
        s8 id;

        if (index_ < 0) {
            index = my_player_no();
        }
        id = 0;
        for (block = 0; block < 2; block++) {
            if (rec[0] != 0 && id != index) {
                self->list_count_0x1BB++;
                *dst++ = id;
            }
            id++;
            if (rec[0x130] != 0 && id != index) {
                self->list_count_0x1BB++;
                *dst++ = id;
            }
            id++;
            if (rec[0x260] != 0 && id != index) {
                self->list_count_0x1BB++;
                *dst++ = id;
            }
            id++;
            if (rec[0x390] != 0 && id != index) {
                self->list_count_0x1BB++;
                *dst++ = id;
            }
            id++;
            if (rec[0x4C0] != 0 && id != index) {
                self->list_count_0x1BB++;
                *dst++ = id;
            }
            id++;
            rec += 0x5F0;
        }
    } else if (self->kind_0x00F == 1) {
        u8* rec = get_move_work_adrs(2);
        s8 count;
        s32 i;

        if (index_ < 0) {
            index = my_player_no();
        }
        count = get_move_work_max(2);
        for (i = 0; i < count; i++) {
            if (rec[0] != 0 && rec[8] != index) {
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
            cursor = cursor - 1;
            if (cursor < 0) {
                cursor = self->rows_0x005 - 1;
            }
        } else {
            self->step_0x002 = 8;
            cursor = cursor + 1;
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
 * source into a work buffer, folds it through `fn_802A9BB8`, marks each of the eight entries with its
 * ordinal and whether its source slot is in use, then hands the lot to `menu_frame_draw_blocks` with the LSP
 * position the `flags` word selects.  `mode` 1 adds the 0x800 bit to the draw flags. */
extern "C" void menu_frame_entries_build(u16* src_a, u16* src_b, s8 kind, u16 lsp_index, u8 mode)
{
    s16 lsp[2];
    u16 mask[0x10];
    u16 work[0x30];
    MenuListEntry entries[8];
    MenuListEntry* entry = entries;
    u16* p;
    s32 flags = 0x20;
    s16 index = 0;
    int block;

    if (mode == 1) {
        flags |= 0x800;
    }
    memcpy(work, src_a, 0x60);
    p = src_b;
    if (src_b != NULL) {
        memcpy(mask, src_b, 0x20);
        p = mask;
    }
    fn_802A9BB8(work, p);
    for (block = 0; block < 2; block++) {
        entry[0].index_0x06 = index;
        entry[0].field_0x01 = 0;
        if (p[0] != 0) {
            entry[0].present_0x02 = 1;
        } else {
            entry[0].present_0x02 = 0;
        }
        index++;
        entry[1].index_0x06 = index;
        entry[1].field_0x01 = 0;
        if (p[2] != 0) {
            entry[1].present_0x02 = 1;
        } else {
            entry[1].present_0x02 = 0;
        }
        index++;
        entry[2].index_0x06 = index;
        entry[2].field_0x01 = 0;
        if (p[4] != 0) {
            entry[2].present_0x02 = 1;
        } else {
            entry[2].present_0x02 = 0;
        }
        index++;
        entry[3].index_0x06 = index;
        entry[3].field_0x01 = 0;
        if (p[6] != 0) {
            entry[3].present_0x02 = 1;
        } else {
            entry[3].present_0x02 = 0;
        }
        index++;
        entry += 4;
        p += 8;
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
    MenuListEntry entries[4];
    u16 work[0x30];
    u16* p;

    memcpy(work, src_a, 0x60);
    p = src_b;
    if (src_b != NULL) {
        memcpy(mask, src_b, 0x20);
        p = mask;
    }
    fn_802A9BB8(work, p);
    entries[0].index_0x06 = 0;
    entries[0].field_0x01 = 0;
    if (p[0] != 0) {
        entries[0].present_0x02 = 1;
    } else {
        entries[0].present_0x02 = 0;
    }
    entries[1].index_0x06 = 1;
    entries[1].field_0x01 = 0;
    if (p[2] != 0) {
        entries[1].present_0x02 = 1;
    } else {
        entries[1].present_0x02 = 0;
    }
    entries[2].index_0x06 = 2;
    entries[2].field_0x01 = 0;
    if (p[4] != 0) {
        entries[2].present_0x02 = 1;
    } else {
        entries[2].present_0x02 = 0;
    }
    entries[3].index_0x06 = 3;
    entries[3].field_0x01 = 0;
    if (p[6] != 0) {
        entries[3].present_0x02 = 1;
    } else {
        entries[3].present_0x02 = 0;
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
extern "C" void menu_frame_page_draw(u32* dst, MenuListEntry* entries, s8 a, s8 b, u16 c, s32 flags_in, u8 mode)
{
    MenuLspPos pos;
    MenuLspPos rows;
    MenuLspPos* base;
    u16 flags;
    u16* tbl;
    s32 select;
    s16 dx;
    s16 dy;
    s32 flags_ = flags_in;
    s32 i;

    get_lsp_data(get_menu_lsp_tbl(361)[mode], (struct _mh_ivec2_*)&pos);
    base = (MenuLspPos*)get_lsp_data(3936, 0);
    dx = pos.x - base->x;
    dy = pos.y - base->y;
    switch (flags_in & 3) {
    case 0:
        tbl = get_menu_lsp_tbl(362);
        select = 2;
        break;
    case 1:
        tbl = get_menu_lsp_tbl(363);
        select = 3;
        break;
    case 2:
        tbl = get_menu_lsp_tbl(364);
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
    PutPageArrow(get_menu_lsp_tbl(365), a, b, flags, (struct _mh_ivec2_*)&pos, 0);
    base = (MenuLspPos*)get_lsp_data(3961, 0);
    rows.x = base->x + dx;
    rows.y = base->y + dy;
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

/* The four forwarders onto `menu_cursor_wrap`'s cursor step, each supplying a different tail of the
 * argument list; the wrapped index they report is what the menu's column cursor stores. */
extern "C" s32 menu_cursor_step_fixed_tail(s32 a, s32 b, u16 c, u16 d, u16 e, s32 f)
{
    return menu_cursor_wrap(a, b, c, d, e, 3, f);
}

/* Same step with the sixth argument zeroed. */
extern "C" s32 menu_cursor_step_open_last(s32 a, s32 b, u16 c, u16 d, u16 e, s32 f)
{
    return menu_cursor_wrap(a, b, c, d, e, f, 0);
}

/* Same step, both tail arguments passed through. */
extern "C" s32 menu_cursor_step_forward(s32 a, s32 b, u16 c, u16 d, u16 e, s32 f, s32 g)
{
    return menu_cursor_wrap(a, b, c, d, e, f, g);
}

/* Same step with the two fixed tail values 3 and 0 - the entry point the cursor call sites use. */
extern "C" s32 menu_cursor_step(s32 a, s32 b, u16 c, u16 d, u16 e)
{
    return menu_cursor_wrap(a, b, c, d, e, 3, 0);
}

/* Ceiling of `a / b`, with b == 0 reported as 1. */
extern "C" s16 fn_802A8F14(s16 a, s16 b)
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

/* The item row at the position the caller supplies: `eft052_page_counts_get` yields the row's two value words
 * (the second already summed), which the `menu_item_row_draw_values` page routine draws.  A null or empty item is
 * skipped. */
extern "C" void menu_item_row_draw_pages(u16* item, struct _mh_ivec2_* pos)
{
    s32 value = 0;
    s32 low;
    s32 high;
    s32 sum = 0;

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
