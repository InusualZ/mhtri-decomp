/*
 * menu/menu_plsearch.cpp - STUB (phase 4, docs/splits/phase4; no bodies yet).
 *
 * `.text` 0x80448404..0x804513FC.  Sections of the candidate unit: extab 0x8001DF24..0x8001E18C; extabindex 0x8003EBEC..0x8003EF88; .text 0x80448404..0x804513FC; .data 0x806073F0..0x80607E50; .bss 0x806E40C0..0x806F4658; .sdata 0x80793B88..0x80793C0C; .sbss 0x80794D50..0x80794D64.
 *
 * WHAT IT IS. the player search menu and the save-data code (`createSystemFile`, `createDataFile`, `readDataFile`, `game_save_wait`,
 *   `startAccountLoad`, `startBootLoad`, `setPlayerSave2Userdata`; 118 functions, 15 named) with the 58 `.data` symbols of its menu
 *   tables and 7 `.bss` objects (0x10598 B).
 *
 * WHY IT SITS HERE. phase 1 grade strong: the closure lands exactly on the registered end of `quest/arenatask.cpp` (0x80448404) and the name is
 *   the `__FILE__` string `menu_plsearch.cpp`; the right edge 0x804513FC is the phase 3 cut `menu/menu_sysmsg` (a `.data`
 *   vtable-then-string seam at 0x80607E50).  Phase 1 notes at least three TUs inside (`menu_plsearch.cpp`, `menu_message.cpp` at
 *   0x8044E340, the save-data code).
 *
 * UNKNOWN. every body and the internal seams.
 *
 * FLAGS. `cflags_menu`, the `menu` lib's group (unmeasured).
 *
 * The unit's `.data`/`.sdata`/`.sbss` claims are the candidate's (config/RMHE08/splits.txt); the symbols they hold are
 * in the map (`ledger.py unit menu/menu_plsearch.cpp`), and the pass that writes the bodies defines them.
 */
