/*
 * MSL/e_log10.cpp - STUB (no bodies yet).
 *
 * `.text` 0x80464DEC..0x80464F00.  Sections of the candidate unit: .text 0x80464DEC..0x80464F00; .sbss 0x80794E30..0x80794E38; .sdata2 0x8079CC18..0x8079CC48.
 *
 * WHAT IT IS. fdlibm `__ieee754_log10`, one function that calls `__ieee754_log`.
 *   GUESS from the call to `__ieee754_log`.
 *
 * WHY IT SITS HERE. phase 1 grade medium, class pool: 3 pooled .sdata2 value(s) are held at two pool addresses (one TU never holds a value twice); for the best of them: .sdata2 value 4330000080000000 held at 0x8079CC10 and again at 0x8079CC40 (copy 1 last read by 0x80464B38, copy 2 first read by 0x80464DEC); the other boundaries of the interval are excluded by must-link spans (none needed: the interval is one boundary wide); 2 more distinct values pin the same cut.
 *
 * UNKNOWN. every body and, for a merged block, the file boundaries between its pieces.
 *
 * FLAGS. the `Runtime.PPCEABI.H` lib's `cflags_ppceabi` (unmeasured until bodies exist).
 *
 * The unit's `.data`/`.sdata`/`.sbss` claims are the candidate's (config/RMHE08/splits.txt); the symbols they hold are
 * in the map (`ledger.py unit MSL/e_log10.cpp`), and the pass that writes the bodies defines them.
 */
