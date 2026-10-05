/*
 * MSL/e_asin.cpp - STUB (no bodies yet).
 *
 * `.text` 0x804642C8..0x80464560.  Sections of the candidate unit: .text 0x804642C8..0x80464560; .sdata2 0x8079CAB8..0x8079CB40.
 *
 * WHAT IT IS. fdlibm arc-sine kernel (`__ieee754_asin`), one function; its `.sdata2` pool is pio2_hi, pio2_lo, huge, one, the pS0..pS5 / qS1..qS4 coefficients, half, two and pio4, and it calls the sqrt thunk 0x804681C8.
 *   GUESS: asin rather than acos because the pool carries `pio4` and no `pi`.
 *
 * WHY IT SITS HERE. phase 1 grade medium, class pool: 15 pooled .sdata2 value(s) are held at two pool addresses (one TU never holds a value twice); for the best of them: .sdata2 value 4000000000000000 held at 0x8079CAB0 and again at 0x8079CB30 (copy 1 last read by 0x80463FFC, copy 2 first read by 0x804642C8); the other boundaries of the interval are excluded by must-link spans (none needed: the interval is one boundary wide); 14 more distinct values pin the same cut.
 *
 * UNKNOWN. every body and, for a merged block, the file boundaries between its pieces.
 *
 * FLAGS. the `Runtime.PPCEABI.H` lib's `cflags_ppceabi` (unmeasured until bodies exist).
 *
 * The unit's `.data`/`.sdata`/`.sbss` claims are the candidate's (config/RMHE08/splits.txt); the symbols they hold are
 * in the map (`ledger.py unit MSL/e_asin.cpp`), and the pass that writes the bodies defines them.
 */
