/*
 * menu/menu_placeinfo.cpp - STUB (phase 4, docs/splits/phase4; no bodies yet).
 *
 * `.text` 0x80437270..0x8043D524.  Sections of the candidate unit: extab 0x8001DA94..0x8001DBEC; extabindex 0x8003E514..0x8003E718; .text 0x80437270..0x8043D524; .ctors 0x8056F3C8..0x8056F3CC; .data 0x80603E90..0x806047D0; .bss 0x806E1B38..0x806E26E0; .sdata 0x80793A10..0x80793A80; .sdata2 0x8079C8A8..0x8079C8D0.
 *
 * WHAT IT IS. the place-info menu (`draw_search_city_dialog`, 0x80439894, and its city/room dialog state; 60 functions, 1 named), with
 *   28 `.data` and 17 `.sdata` symbols and a `.ctors` word.
 *
 * WHY IT SITS HERE. phase 1 grade medium: the `.ctors` closure ends at 0x8043D524 and the name is the `__FILE__` string `menu_placeinfo.cpp`
 *   in the range's data; the left edge is the end of `Network/net_session_close`.
 *
 * UNKNOWN. every body; the unit may hold more than one TU (phase1-e.md open question 6).
 *
 * FLAGS. `cflags_menu`, the `menu` lib's group (unmeasured).
 *
 * The unit's `.data`/`.sdata`/`.sbss` claims are the candidate's (config/RMHE08/splits.txt); the symbols they hold are
 * in the map (`ledger.py unit menu/menu_placeinfo.cpp`), and the pass that writes the bodies defines them.
 */
