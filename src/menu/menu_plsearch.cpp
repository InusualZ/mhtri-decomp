/*
 * menu/menu_plsearch.cpp - the player search menu and the save-data code (`createSystemFile`, `createDataFile`,
 *   `readDataFile`, `game_save_wait`, `startAccountLoad`, `startBootLoad`, `setPlayerSave2Userdata`), with the 58 `.data`
 *   symbols of its menu tables and 7 `.bss` objects.  No bodies yet.
 * RANGE. .text 0x80448404-0x804513FC (118 functions, 15 named); extab 0x8001DF24-0x8001E18C, extabindex
 *   0x8003EBEC-0x8003EF88, .data 0x806073F0-0x80607E50, .bss 0x806E40C0-0x806F4658, .sdata 0x80793B88-0x80793C10, .sbss
 *   0x80794D50-0x80794D64.  The left edge is `quest/arenatask.cpp`'s end, the right one the `.data` vtable-then-string
 *   seam at 0x80607E50 (`menu/menu_sysmsg.cpp`).
 * FLAGS. `cflags_menu` (configure.py; unmeasured).
 * NAMES. Module and file from the `__FILE__` string "menu_plsearch.cpp".  `sPatTerms` (.sbss 0x80794D58) is a GUESS:
 *   the terms object's constructor publishes itself there.
 *   GUESS (from each body and its callers): game_data_file_create_start, game_system_file_create_start
 *   GUESS: game_system_file_create_wait
 * RESIDUALS. every body (118 rows score zero) and the internal seams: at least three TUs (`menu_plsearch.cpp`,
 *   `menu_message.cpp` at 0x8044E340, the save-data code).  The symbols are in the map
 *   (`python tools/units/ledger.py unit menu/menu_plsearch.cpp`).
 */
