/*
 * nw4r/fn_805012C4.cpp - STUB (no bodies yet).
 *
 * `.text` 0x805012C4..0x80502828.  Sections of the candidate unit: .text 0x805012C4..0x80502828; .ctors 0x8056F3D8..0x8056F3DC; .data 0x8062FAC8..0x8062FAF0; .bss 0x80760C78..0x80760C98; .sdata2 0x8079D4C8..0x8079D4F0.
 *
 * WHAT IT IS. nw4r math/ut code (`mtx34_rotate_vec3`) followed by the MEM list helpers (`MEMInitList` at 0x80501A4C); several nw4r files merged because their pooled doubles agree.
 *   merged candidate pieces (guess cuts the renderer does not emit): fn_805012C4, mem_list, fn_80501CE8.
 *
 * WHY IT SITS HERE. phase 1 grade medium, class pool: 1 pooled .sdata2 value(s) are held at two pool addresses (one TU never holds a value twice); for the best of them: .sdata2 value 00000000 held at 0x8079D4A4 and again at 0x8079D4C8 (copy 1 last read by 0x80501108, copy 2 first read by 0x805012C4); the other boundaries of the interval are excluded by must-link spans (none needed: the interval is one boundary wide).
 *
 * UNKNOWN. every body and, for a merged block, the file boundaries between its pieces.
 *
 * FLAGS. the `OS` lib's `cflags_os` (unmeasured until bodies exist).
 *
 * The unit's `.data`/`.sdata`/`.sbss` claims are the candidate's (config/RMHE08/splits.txt); the symbols they hold are
 * in the map (`ledger.py unit nw4r/fn_805012C4.cpp`), and the pass that writes the bodies defines them.
 */
