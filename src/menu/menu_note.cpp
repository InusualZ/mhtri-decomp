/*
 * menu/menu_note.cpp - the note-list entry table: `get_note_item_slot` maps a note entry 0..9 to the item table's slot
 *   (0..3, `fn_8029F818`'s `lbl_806ACF28.array_0x758` run) and the index into it.
 * RANGE. .text 0x8034C0C4-0x8034C1D0 (1 function); .data 0x805E91F8-0x805E9248: `s_menu_note_cpp`, `s_nw4r_assert_failed`
 *   and the switch's 10-entry jump table.
 * FLAGS. `cflags_menu` (configure.py); no pragma.
 * NAMES. Module and file from the `__FILE__` string "menu_note.cpp" (one copy in the DOL, referenced by this object
 *   only, so the 0x8034C1D0 edge is real).  `get_note_item_slot` is a GUESS from the body and its callers
 *   (`note_slot_cursor_step`, `note_slot_draw_row`, which hand the pair to `note_slot_cursor_bounds`); that the argument
 *   is a note-list entry comes from the file name, and the switch's ten constants are otherwise unexplained.
 * RESIDUALS. none: `Object(Matching)`, flipcheck READY (`.text` 0x10C and `.data` 0x50 byte-identical).
 */

#include "types.h"

#include "nw4r/db_assert.h" /* nw4r::db::Panic and NW4R_ASSERT (rule 2) */

/* The unit's own two literals, in `.data` (the retail build did not use `-str readonly`, so the
 * pool is `.data` and not `.rodata`); the map names them after what they hold.  The unit claims
 * them (0x805E91F8..0x805E9220), so they are defined here, ahead of the jump table. */
char s_menu_note_cpp[] = "menu_note.cpp";                /* .data 0x805E91F8 */
char s_nw4r_assert_failed[] = "NW4R:Failed assertion 0"; /* .data 0x805E9208 */

/* Maps a note-list entry to the item table's slot and the index into it. */
extern "C" void get_note_item_slot(u8 note_entry, u8* p_slot_tbl, u8* p_slot_idx) {
    switch (note_entry) {
    case 0:
        *p_slot_tbl = 2;
        *p_slot_idx = 0;
        break;
    case 1:
        *p_slot_tbl = 2;
        *p_slot_idx = 18;
        break;
    case 2:
        *p_slot_tbl = 2;
        *p_slot_idx = 12;
        break;
    case 3:
        *p_slot_tbl = 3;
        *p_slot_idx = 0;
        break;
    case 4:
        *p_slot_tbl = 2;
        *p_slot_idx = 22;
        break;
    case 5:
        *p_slot_tbl = 2;
        *p_slot_idx = 19;
        break;
    case 6:
        *p_slot_tbl = 2;
        *p_slot_idx = 15;
        break;
    case 7:
        *p_slot_tbl = 2;
        *p_slot_idx = 21;
        break;
    case 8:
        *p_slot_tbl = 2;
        *p_slot_idx = 25;
        break;
    case 9:
        *p_slot_tbl = 2;
        *p_slot_idx = 20;
        break;
    default:
        NW4R_ASSERT(0, s_menu_note_cpp, 2301, s_nw4r_assert_failed);
        break;
    }
}
