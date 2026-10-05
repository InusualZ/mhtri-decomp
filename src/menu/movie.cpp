/*
 * menu/movie.cpp - STUB (no bodies yet).
 *
 * `.text` 0x8043D524..0x804459DC.  Sections of the candidate unit: extab 0x8001DBEC..0x8001DE9C; extabindex 0x8003E718..0x8003EB20; .text 0x8043D524..0x804459DC; .ctors 0x8056F3CC..0x8056F3D0; .data 0x806047D0..0x80604D2C; .bss 0x806E26E0..0x806E3E10; .sdata 0x80793A80..0x80793B28; .sbss 0x80794D08..0x80794D18; .sdata2 0x8079C8D0..0x8079C960.
 *
 * WHAT IT IS. movie playback (`Movie_open`, `Movie_draw_sub`, `Movie_close`, `Check_movie_finish`, `Movie_cut`, `THPSimpleGetTotalFrame`),
 *   the character-make screen (`charmake_init`, `charmake_move`, `get_charmake_param`, `set_charmake_r_no`) and the demo
 *   equipment (`set_demo_equip_data`, `demoEquip`); 112 functions, 11 named.
 *
 * WHY IT SITS HERE. phase 1 grade strong: the `.ctors` closure plus the `__FILE__` anchor `movie.cpp` starts at the cut 0x8043D524; the right edge
 *   is the registered `quest/arenatask.cpp` (moved to 0x804459DC).  Phase 1 notes the run holds at least three TUs (movie,
 *   `menu_friendlist.cpp` at 0x8043F398 and the character make code).
 *
 * UNKNOWN. every body and the internal seams.
 *
 * FLAGS. `cflags_menu`, the `menu` lib's group (unmeasured).
 *
 * The unit's `.data`/`.sdata`/`.sbss` claims are the candidate's (config/RMHE08/splits.txt); the symbols they hold are
 * in the map (`ledger.py unit menu/movie.cpp`), and the pass that writes the bodies defines them.
 */
