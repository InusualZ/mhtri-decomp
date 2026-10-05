/*
 * MSL/e_atan2.cpp - STUB (no bodies yet).
 *
 * `.text` 0x80464560..0x804647B8.  Sections of the candidate unit: .text 0x80464560..0x804647B8; .sdata2 0x8079CB40..0x8079CB98.
 *
 * WHAT IT IS. fdlibm `__ieee754_atan2`, one function that calls the `atan` routine at 0x80467570 twice.
 *   GUESS from the two atan calls.
 *
 * WHY IT SITS HERE. phase 1 grade medium, class pool: 2 pooled .sdata2 value(s) are held at two pool addresses (one TU never holds a value twice); for the best of them: .sdata2 value 3ff921fb54442d18 held at 0x8079CAB8 and again at 0x8079CB58 (copy 1 last read by 0x804642C8, copy 2 first read by 0x80464560); the other boundaries of the interval are excluded by must-link spans (none needed: the interval is one boundary wide); 1 more distinct values pin the same cut.
 *
 * UNKNOWN. every body and, for a merged block, the file boundaries between its pieces.
 *
 * FLAGS. the `Runtime.PPCEABI.H` lib's `cflags_ppceabi` (unmeasured until bodies exist).
 *
 * The unit's `.data`/`.sdata`/`.sbss` claims are the candidate's (config/RMHE08/splits.txt); the symbols they hold are
 * in the map (`ledger.py unit MSL/e_atan2.cpp`), and the pass that writes the bodies defines them.
 */
