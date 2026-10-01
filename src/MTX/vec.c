/*
 * MTX/vec.c - STUB (phase 4, docs/splits/phase4; no bodies yet).
 *
 * `.text` 0x804C6B60..0x804C6D70.  Sections of the candidate unit: .text 0x804C6B60..0x804C6D70; .sdata2 0x8079D2B0..0x8079D2C0.
 *
 * WHAT IT IS. the Revolution SDK VEC library (`PSVECNormalize`, `PSVECMag`, `PSVECDotProduct`, `PSVECCrossProduct`, `PSVECSquareDistance`), hand-written paired-single code.
 *
 * WHY IT SITS HERE. phase 1 grade medium, class recut: PSVECNormalize..PSVECSquareDistance: tudiscover finds the vec cluster certainly one TU.
 *
 * UNKNOWN. every body and, for a merged block, the file boundaries between its pieces.
 *
 * FLAGS. the `OS` lib's `cflags_os` (unmeasured until bodies exist).
 *
 * The unit's `.data`/`.sdata`/`.sbss` claims are the candidate's (config/RMHE08/splits.txt); the symbols they hold are
 * in the map (`ledger.py unit MTX/vec.c`), and the pass that writes the bodies defines them.
 */
