/*
 * menu/movie.cpp - movie playback (`Movie_open`, `Movie_draw_sub`, `Movie_close`, `Check_movie_finish`, `Movie_cut`,
 *   `THPSimpleGetTotalFrame`), the character-make screen (`charmake_init`, `charmake_move`, `get_charmake_param`,
 *   `set_charmake_r_no`) and the demo equipment (`set_demo_equip_data`, `demoEquip`).  No bodies yet.
 * RANGE. .text 0x8043D524-0x804459DC (112 functions, 11 named); extab 0x8001DBEC-0x8001DE9C, extabindex
 *   0x8003E718-0x8003EB20, .ctors 0x8056F3CC, .data 0x806047D0-0x80604D2C, .bss 0x806E26E0-0x806E3E10, .sdata
 *   0x80793A80-0x80793B28, .sbss 0x80794D08-0x80794D18, .sdata2 0x8079C8D0-0x8079C960.  The left edge is the `.ctors`
 *   closure and the `movie.cpp` `__FILE__` anchor, the right one `quest/arenatask.cpp`.
 * FLAGS. `cflags_menu` (configure.py; unmeasured).
 * NAMES. Module `menu` and the file from the `__FILE__` string "movie.cpp".
 * RESIDUALS. every body (112 rows score zero) and the internal seams: the run holds at least three TUs (movie,
 *   `menu_friendlist.cpp` at 0x8043F398, the character make code).  The symbols are in the map
 *   (`python tools/units/ledger.py unit menu/movie.cpp`).
 */
