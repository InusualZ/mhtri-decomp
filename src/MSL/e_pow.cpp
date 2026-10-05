/*
 * MSL/e_pow.cpp - STUB (no bodies yet).
 *
 * `.text` 0x80464F00..0x80465714.  Sections of the candidate unit: .text 0x80464F00..0x80465714; .rodata 0x805731A0..0x805731D0; .sdata2 0x8079CC48..0x8079CD58.
 *
 * WHAT IT IS. fdlibm `__ieee754_pow`, one function; `.rodata` 0x30 is the bp / dp_h / dp_l tables and it calls `scalbn` and the sqrt thunk.
 *   GUESS from the three 2-entry tables.
 *
 * WHY IT SITS HERE. phase 1 grade medium, class pool: 1 pooled .sdata2 value(s) are held at two pool addresses (one TU never holds a value twice); for the best of them: .sdata2 value 4330000080000000 held at 0x8079CC40 and again at 0x8079CD50 (copy 1 last read by 0x80464DEC, copy 2 first read by 0x80464F00); the other boundaries of the interval are excluded by must-link spans (none needed: the interval is one boundary wide).
 *
 * UNKNOWN. every body and, for a merged block, the file boundaries between its pieces.
 *
 * FLAGS. the `Runtime.PPCEABI.H` lib's `cflags_ppceabi` (unmeasured until bodies exist).
 *
 * The unit's `.data`/`.sdata`/`.sbss` claims are the candidate's (config/RMHE08/splits.txt); the symbols they hold are
 * in the map (`ledger.py unit MSL/e_pow.cpp`), and the pass that writes the bodies defines them.
 */
