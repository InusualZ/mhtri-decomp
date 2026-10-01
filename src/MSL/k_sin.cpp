/*
 * MSL/k_sin.cpp - STUB (phase 4, docs/splits/phase4; no bodies yet).
 *
 * `.text` 0x80467260..0x80467320.  Sections of the candidate unit: .text 0x80467260..0x80467320; .sdata2 0x8079CE38..0x8079CE70.
 *
 * WHAT IT IS. fdlibm `__kernel_sin` (named in the map), one function.
 *
 * WHY IT SITS HERE. phase 1 grade strong, class file: __kernel_sin (fdlibm k_sin.c).
 *
 * UNKNOWN. every body and, for a merged block, the file boundaries between its pieces.
 *
 * FLAGS. the `Runtime.PPCEABI.H` lib's `cflags_ppceabi` (unmeasured until bodies exist).
 *
 * The unit's `.data`/`.sdata`/`.sbss` claims are the candidate's (config/RMHE08/splits.txt); the symbols they hold are
 * in the map (`ledger.py unit MSL/k_sin.cpp`), and the pass that writes the bodies defines them.
 */
