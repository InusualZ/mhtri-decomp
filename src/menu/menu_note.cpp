/* menu/menu_note.cpp - the note-list entry table: which item record a note entry stands for.
 *
 * `.text` 0x8034C0C4..0x8034C1D0, one function (`get_note_item_slot`, 0x10C B) - a switch on a note
 * entry 0..9 that writes the item-table slot and the entry index the caller looks the record up
 * with.
 *
 * NAME (evidence class 1, the `__FILE__` string).  The range's only two `.data` references are
 * `s_menu_note_cpp` = "menu_note.cpp" (0xE B incl. the NUL) and `s_nw4r_assert_failed` =
 * "NW4R:Failed assertion 0" (0x18 B), passed as `Panic(__FILE__, 2301, ...)`.  `menu_note.cpp`
 * occurs exactly ONCE in the DOL (`grep` over `orig/RMHE08/sys/main.dol`), so it is TU-local and the
 * file's original name; the module is `menu` (the `menu_*` family the band's pool carries:
 * `menu_item.cpp` 0x805CDFC8, `menu_infomation.cpp` 0x805DCCDC) and the extension is the name's own
 * suffix.  Both labels are referenced by exactly one object in the whole split tree - this range's
 * object - so no neighbour shares this TU and the 0x8034C1D0 edge is a real one (the seam the pool
 * brief pinned).
 *
 * SYMBOL NAMES (naming pass, 2026-09-26).  The map and the runtime dump had nothing for this
 * range (`dumpmap.py lookup 0x8034C0C4` answers `zz_034c0c4_`; `symedit.py range 0x8034C000
 * 0x8034D000` shows bare stems up to the registered `hud/` and `ef/` units), so the names are
 * derived from the body and the band's scheme.  It reads one note entry and writes the menu
 * item-table slot (0..3 - `fn_8029F818`'s `lbl_806ACF28.array_0x758` run) plus the index into that
 * table, i.e. an item-record location: `get_note_item_slot`, verb-first like the `menu` band's own
 * accessors (`get_menu_tbl_ptr`, `get_menu_lsp_tbl`, `get_lsp_data`).  The two pool strings dropped
 * their `lbl_` names for the same reason - their content is this TU's own `__FILE__` and its
 * `NW4R_ASSERT` message, so each is named for what it holds.  MARKED GUESS: that the argument is a
 * *note-list* entry comes from the file name (the same class-1 string), and that the pair addresses
 * an *item* record is inferred from the callers; a pass with the screen's own symbols can sharpen
 * both, and the switch's ten constants are otherwise unexplained.
 *
 * The two out parameters are inferred, and marked as such: both callers hand the pair to
 * `fn_8034C614`, which switches on the FIRST one (0..3) to pick one of four item tables via
 * `fn_8029F818` and then indexes it with the SECOND, so they are the table slot and the entry index
 * rather than a pair of undifferentiated bytes.  They are `fn_8034C1D0` and `fn_8034C2B8` - the
 * list row's select/update and draw handlers, called from `fn_8033A160` (0x8033A4D4) and
 * `fn_8033A7DC` (0x8033A808); a scan of every `bl` in the DOL finds no other caller.
 *
 * SECTIONS.  `.data` 0x805E9220..0x805E9248 is this unit's own 10-entry jump table, which MWCC emits
 * from the switch below; it is the only data our object emits.  The two string labels above are
 * declared, never defined (playbook 29: a definition emits a second copy and grows `.data`), so the
 * bytes stay where the DOL has them - inside the `auto_07_805E27B0_data` chunk, whose bracketing
 * registered units (`hud/fn_80334568.cpp` below, `fn_80423E74.cpp` above) disagree on a module, so
 * rule 2's home for them is a named gap and the declarations stay here.
 *
 * Flags: the lib's `cflags_menu` (`-O3 -inline noauto -opt nopeephole -Cpp_exceptions on`, mw
 * version Wii/1.3).  No pragma is needed - the body has no fold-shaped pair and no `bl` to a
 * same-file helper.
 *
 * STATUS: `Object(Matching, ...)`.  The object is byte-identical to the target in both sections
 * (`.text` 0x10C, `.data` 0x28; only MWCC's `.comment` version byte differs, `0f` against the
 * target's `0e`, as in every other unit of this lib), objdiff's official metric reads 100.0 % fuzzy
 * / 268 of 268 `.text` bytes / 40 of 40 `.data` bytes, and `flipcheck.py` answers READY.  Proved by
 * the link, not by the score alone: with the unit flipped, a full `ninja` in this branch's worktree
 * ends `build/RMHE08/main.dol: OK` and the built DOL's SHA-1 is the documented
 * `bf4850739478caaedfe675949eb7c28595a7fde9`.  No residual.
 */

#include "types.h"

/* `nw4r::db::Panic`: declaring the owner's real spelling makes the C++ front-end reproduce the map's
 * mangling (`Panic__Q24nw4r2dbFPCciPCce`) exactly; spelling the mangling itself re-mangles it
 * (rule 9).  Its last parameter is variadic - the retail call sets `cr1`'s eq bit, which only the
 * varargs convention does. */
namespace nw4r {
namespace db {
void Panic(const char* file, int line, const char* fmt, ...);
}  // namespace db
}  // namespace nw4r

/* The unit's own two literals, in `.data` (the retail build did not use `-str readonly`, so the
 * pool is `.data` and not `.rodata`); the map names them after what they hold.  Both are owned by
 * the unclaimed `.data` chunk this unit sits in, so they are declared here and never defined. */
extern char s_menu_note_cpp[];      /* "menu_note.cpp"            .data 0x805E91F8 */
extern char s_nw4r_assert_failed[]; /* "NW4R:Failed assertion 0"  .data 0x805E9208 */

/* nw4r's assert; the message is the `NW4R_ASSERT(...)` stringification of the condition (which is
 * why the pooled format string reads "NW4R:Failed assertion 0"), and the file is this unit's
 * `__FILE__`. */
#define NW4R_ASSERT(exp, line, msg) ((exp) ? (void)0 : nw4r::db::Panic(s_menu_note_cpp, line, msg))

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
        NW4R_ASSERT(0, 2301, s_nw4r_assert_failed);
        break;
    }
}
