/*
 * MSL/s_modf.cpp - STUB (phase 4, docs/splits/phase4; no bodies yet).
 *
 * `.text` 0x80467D24..0x80467E20.  Sections of the candidate unit: .text 0x80467D24..0x80467E20.
 *
 * WHAT IT IS. fdlibm `modf`, one function (integer-part pointer in r3, no data).
 *   GUESS from the code shape: split of the mantissa by the exponent, `fsub` for the fraction.
 *
 * WHY IT SITS HERE. phase 1 grade medium, class file: fdlibm one function per file.
 *
 * UNKNOWN. every body and, for a merged block, the file boundaries between its pieces.
 *
 * FLAGS. the `Runtime.PPCEABI.H` lib's `cflags_ppceabi` (unmeasured until bodies exist).
 *
 * The unit's `.data`/`.sdata`/`.sbss` claims are the candidate's (config/RMHE08/splits.txt); the symbols they hold are
 * in the map (`ledger.py unit MSL/s_modf.cpp`), and the pass that writes the bodies defines them.
 */
