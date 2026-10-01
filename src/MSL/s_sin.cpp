/*
 * MSL/s_sin.cpp - STUB (phase 4, docs/splits/phase4; no bodies yet).
 *
 * `.text` 0x80467E20..0x80467EEC.  Sections of the candidate unit: .text 0x80467E20..0x80467EEC; .sdata2 0x8079CF28..0x8079CF30.
 *
 * WHAT IT IS. fdlibm `sin`, one function that calls `__kernel_sin`, `__ieee754_rem_pio2` and `__kernel_cos`.
 *   GUESS from those callees.
 *
 * WHY IT SITS HERE. phase 1 grade medium, class file: fdlibm one function per file.
 *
 * UNKNOWN. every body and, for a merged block, the file boundaries between its pieces.
 *
 * FLAGS. the `Runtime.PPCEABI.H` lib's `cflags_ppceabi` (unmeasured until bodies exist).
 *
 * The unit's `.data`/`.sdata`/`.sbss` claims are the candidate's (config/RMHE08/splits.txt); the symbols they hold are
 * in the map (`ledger.py unit MSL/s_sin.cpp`), and the pass that writes the bodies defines them.
 */
