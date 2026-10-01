/*
 * MSL/s_atan.cpp - STUB (phase 4, docs/splits/phase4; no bodies yet).
 *
 * `.text` 0x80467570..0x804677A0.  Sections of the candidate unit: .text 0x80467570..0x804677A0; .rodata 0x80573410..0x805734A8; .sdata2 0x8079CEA8..0x8079CED0.
 *
 * WHAT IT IS. fdlibm `atan`, one function; `.rodata` 0x98 is atanhi[4] / atanlo[4] / aT[11].
 *   GUESS from the 19-double table.
 *
 * WHY IT SITS HERE. phase 1 grade medium, class pool: 3 pooled .sdata2 value(s) are held at two pool addresses (one TU never holds a value twice); for the best of them: .sdata2 value bff0000000000000 held at 0x8079CE78 and again at 0x8079CEC8 (copy 1 last read by 0x80467320, copy 2 first read by 0x80467570); the other boundaries of the interval are excluded by must-link spans (none needed: the interval is one boundary wide); 2 more distinct values pin the same cut.
 *
 * UNKNOWN. every body and, for a merged block, the file boundaries between its pieces.
 *
 * FLAGS. the `Runtime.PPCEABI.H` lib's `cflags_ppceabi` (unmeasured until bodies exist).
 *
 * The unit's `.data`/`.sdata`/`.sbss` claims are the candidate's (config/RMHE08/splits.txt); the symbols they hold are
 * in the map (`ledger.py unit MSL/s_atan.cpp`), and the pass that writes the bodies defines them.
 */
