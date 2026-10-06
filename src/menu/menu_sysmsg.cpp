/*
 * menu/menu_sysmsg.cpp - the Wii system message and error text (the disc could not be read, controller / Wii Remote
 *   errors, save and data-management messages: 419 `.data` symbols) and the dialog code that shows it.  No bodies yet.
 * RANGE. .text 0x804513FC-0x804565C0 (87 functions, none named in the map); extab 0x8001E18C-0x8001E3B0, extabindex
 *   0x8003EF88-0x8003F144, .ctors 0x8056F3D4, .rodata 0x80572240-0x80572418, .data 0x80607E50-0x8060E7FC, .bss
 *   0x806F4658-0x806F4B48, .sdata 0x80793C80-0x80793CB8, .sbss 0x80794D64-0x80794DF8 (50 symbols), .sdata2
 *   0x8079C998-0x8079C9A8.  The left edge is the `.data` vtable-then-string seam 0x80607E50 (the previous half's last
 *   read 0x80451364, this half's first 0x80451600); the right edge is the band's `.ctors` closure.
 * FLAGS. `cflags_menu` (configure.py; unmeasured).
 * NAMES. The file name is a GUESS from the text the `.data` holds (read with `tools/units/m2cinput.py`'s DOL reader at
 *   0x80607E50); no `__FILE__` string names the TU.
 * RESIDUALS. every body (87 rows score zero) and the real file name.  The symbols are in the map
 *   (`python tools/units/ledger.py unit menu/menu_sysmsg.cpp`).
 */
