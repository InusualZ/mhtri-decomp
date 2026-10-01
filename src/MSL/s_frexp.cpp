/*
 * MSL/s_frexp.cpp - STUB (phase 4, docs/splits/phase4; no bodies yet).
 *
 * `.text` 0x80467B30..0x80467BB8.  Sections of the candidate unit: .text 0x80467B30..0x80467BB8; .sdata2 0x8079CEF8..0x8079CF00.
 *
 * WHAT IT IS. fdlibm `frexp`, one function; its pool is the 2^54 scale constant.
 *   GUESS from the exponent-extraction code and the 2^54 constant.
 *
 * WHY IT SITS HERE. phase 1 grade medium, class file: fdlibm one function per file.
 *
 * UNKNOWN. every body and, for a merged block, the file boundaries between its pieces.
 *
 * FLAGS. the `Runtime.PPCEABI.H` lib's `cflags_ppceabi` (unmeasured until bodies exist).
 *
 * The unit's `.data`/`.sdata`/`.sbss` claims are the candidate's (config/RMHE08/splits.txt); the symbols they hold are
 * in the map (`ledger.py unit MSL/s_frexp.cpp`), and the pass that writes the bodies defines them.
 */
