/*
 * nw4r/math_triangular.cpp - STUB (phase 4, docs/splits/phase4; no bodies yet).
 *
 * `.text` 0x80500E34..0x805012C4.  Sections of the candidate unit: .text 0x80500E34..0x805012C4; .rodata 0x80573CD8..0x80574CE8; .data 0x8062F9C0..0x8062FAC8; .sdata2 0x8079D4A0..0x8079D4C8.
 *
 * WHAT IT IS. `nw4r::math` sine/cosine table lookups (`SinFIdx`, `CosFIdx`) and the sincos index helper; `.rodata` 0x1010 is the lookup table.
 *
 * WHY IT SITS HERE. phase 1 grade medium, class file: SinFIdx/CosFIdx__Q24nw4r4math (nw4r::math triangular roster).
 *
 * UNKNOWN. every body and, for a merged block, the file boundaries between its pieces.
 *
 * FLAGS. the `OS` lib's `cflags_os` (unmeasured until bodies exist).
 *
 * The unit's `.data`/`.sdata`/`.sbss` claims are the candidate's (config/RMHE08/splits.txt); the symbols they hold are
 * in the map (`ledger.py unit nw4r/math_triangular.cpp`), and the pass that writes the bodies defines them.
 */
