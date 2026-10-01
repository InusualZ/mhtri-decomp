/*
 * MSL/e_fmod.cpp - STUB (phase 4, docs/splits/phase4; no bodies yet).
 *
 * `.text` 0x804647B8..0x80464B38.  Sections of the candidate unit: .text 0x804647B8..0x80464B38; .rodata 0x80573190..0x805731A0.
 *
 * WHAT IT IS. fdlibm `__ieee754_fmod`, one function; its `.rodata` 0x10 is fdlibm's `Zero[2] = {0.0, -0.0}`.
 *   GUESS from the Zero[] table.
 *
 * WHY IT SITS HERE. phase 1 grade medium, class file: fdlibm e_*.c: one function per file.
 *
 * UNKNOWN. every body and, for a merged block, the file boundaries between its pieces.
 *
 * FLAGS. the `Runtime.PPCEABI.H` lib's `cflags_ppceabi` (unmeasured until bodies exist).
 *
 * The unit's `.data`/`.sdata`/`.sbss` claims are the candidate's (config/RMHE08/splits.txt); the symbols they hold are
 * in the map (`ledger.py unit MSL/e_fmod.cpp`), and the pass that writes the bodies defines them.
 */
