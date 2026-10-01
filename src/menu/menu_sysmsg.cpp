/*
 * menu/menu_sysmsg.cpp - STUB (phase 4, docs/splits/phase4; no bodies yet).
 *
 * `.text` 0x804513FC..0x804565C0.  Sections of the candidate unit: extab 0x8001E18C..0x8001E3B0; extabindex 0x8003EF88..0x8003F144; .text 0x804513FC..0x804565C0; .ctors 0x8056F3D4..0x8056F3D8; .rodata 0x80572240..0x80572418; .data 0x80607E50..0x8060E7FC; .bss 0x806F4658..0x806F4B48; .sdata 0x80793C80..0x80793CB8; .sbss 0x80794D64..0x80794DF8; .sdata2 0x8079C998..0x8079C9A8.
 *
 * WHAT IT IS. a menu band whose `.data` (0x80607E50..0x8060E7FC, 419 symbols) is the Wii system message and error text (the disc could not be
 *   read, controller / Wii Remote errors, save and data-management messages) and the dialog code that shows it (87 functions, none named in the map;
 *   `.sbss` holds 50 symbols, `.ctors` one word).
 *
 * WHY IT SITS HERE. phase 3 cut 0x804513FC (medium: `.data` vtable-then-string seam 0x80607E50, the last read of the previous half at 0x80451364,
 *   the first read of this half at 0x80451600); the right edge is the `.ctors` closure of the band, ending at 0x804565C0.
 *   The candidate carried the placeholder name `menu/fn_804513FC`; the name here is a GUESS from the text the `.data` holds
 *   (strings read with `tools/units/m2cinput.py`'s DOL reader at 0x80607E50), no `__FILE__` string names the TU.
 *
 * UNKNOWN. every body; the real file name.
 *
 * FLAGS. `cflags_menu`, the `menu` lib's group (unmeasured).
 *
 * The unit's `.data`/`.sdata`/`.sbss` claims are the candidate's (config/RMHE08/splits.txt); the symbols they hold are
 * in the map (`ledger.py unit menu/menu_sysmsg.cpp`), and the pass that writes the bodies defines them.
 */
