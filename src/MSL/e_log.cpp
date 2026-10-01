/*
 * MSL/e_log.cpp - STUB (phase 4, docs/splits/phase4; no bodies yet).
 *
 * `.text` 0x80464B38..0x80464DEC.  Sections of the candidate unit: .text 0x80464B38..0x80464DEC; .sbss 0x80794E28..0x80794E30; .sdata2 0x8079CB98..0x8079CC18.
 *
 * WHAT IT IS. fdlibm `__ieee754_log` (named in the map), one function.
 *
 * WHY IT SITS HERE. phase 1 grade medium, class file: __ieee754_log (fdlibm e_log.c).
 *
 * UNKNOWN. every body and, for a merged block, the file boundaries between its pieces.
 *
 * FLAGS. the `Runtime.PPCEABI.H` lib's `cflags_ppceabi` (unmeasured until bodies exist).
 *
 * The unit's `.data`/`.sdata`/`.sbss` claims are the candidate's (config/RMHE08/splits.txt); the symbols they hold are
 * in the map (`ledger.py unit MSL/e_log.cpp`), and the pass that writes the bodies defines them.
 */
