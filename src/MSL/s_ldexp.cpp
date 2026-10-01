/*
 * MSL/s_ldexp.cpp - STUB (phase 4, docs/splits/phase4; no bodies yet).
 *
 * `.text` 0x80467BB8..0x80467D24.  Sections of the candidate unit: .text 0x80467BB8..0x80467D24; .sdata2 0x8079CF00..0x8079CF28.
 *
 * WHAT IT IS. the `scalbn`/`ldexp` scaling routine, one function that calls `__fpclassifyd` and `copysign` and pools huge, tiny and 2^-54.
 *   GUESS: its place in the alphabetical file order (between frexp and modf) says ldexp; its body is fdlibm's scalbn.
 *
 * WHY IT SITS HERE. phase 1 grade medium, class pool: 1 pooled .sdata2 value(s) are held at two pool addresses (one TU never holds a value twice); for the best of them: .sdata2 value 4350000000000000 held at 0x8079CEF8 and again at 0x8079CF08 (copy 1 last read by 0x80467B30, copy 2 first read by 0x80467BB8); the other boundaries of the interval are excluded by must-link spans (none needed: the interval is one boundary wide).
 *
 * UNKNOWN. every body and, for a merged block, the file boundaries between its pieces.
 *
 * FLAGS. the `Runtime.PPCEABI.H` lib's `cflags_ppceabi` (unmeasured until bodies exist).
 *
 * The unit's `.data`/`.sdata`/`.sbss` claims are the candidate's (config/RMHE08/splits.txt); the symbols they hold are
 * in the map (`ledger.py unit MSL/s_ldexp.cpp`), and the pass that writes the bodies defines them.
 */
