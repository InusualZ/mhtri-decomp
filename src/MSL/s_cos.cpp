/*
 * MSL/s_cos.cpp - STUB (no bodies yet).
 *
 * `.text` 0x80467918..0x804679E0.  Sections of the candidate unit: .text 0x80467918..0x804679E0; .sdata2 0x8079CEE0..0x8079CEE8.
 *
 * WHAT IT IS. fdlibm `cos`, one function that calls `__kernel_cos`, `__ieee754_rem_pio2` and `__kernel_sin`.
 *   GUESS from those three callees.
 *
 * WHY IT SITS HERE. phase 1 grade medium, class file: fdlibm one function per file.
 *
 * UNKNOWN. every body and, for a merged block, the file boundaries between its pieces.
 *
 * FLAGS. the `Runtime.PPCEABI.H` lib's `cflags_ppceabi` (unmeasured until bodies exist).
 *
 * The unit's `.data`/`.sdata`/`.sbss` claims are the candidate's (config/RMHE08/splits.txt); the symbols they hold are
 * in the map (`ledger.py unit MSL/s_cos.cpp`), and the pass that writes the bodies defines them.
 */
