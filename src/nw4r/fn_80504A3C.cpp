/*
 * nw4r/fn_80504A3C.cpp - STUB (no bodies yet).
 *
 * `.text` 0x80504A3C..0x8050661C.  Sections of the candidate unit: .text 0x80504A3C..0x8050661C; .ctors 0x8056F3DC..0x8056F3E0; .bss 0x80760CB8..0x80760CD0; .sbss 0x80795770..0x80795780; .sdata2 0x8079D510..0x8079D528.
 *
 * WHAT IT IS. nw4r code between a pool cut (0x80504A3C) and the `.ctors` cut at 0x8050661C; no named function.
 *
 * WHY IT SITS HERE. phase 1 grade medium, class pool: 2 pooled .sdata2 value(s) are held at two pool addresses (one TU never holds a value twice); for the best of them: .sdata2 value 4330000080000000 held at 0x8079D4F8 and again at 0x8079D518 (copy 1 last read by 0x805041F4, copy 2 first read by 0x80504A3C); address order tightens it to cuts 0x80504A3C..0x80504A3C (the literal at 0x8079D4F0, at or below copy 1, is last read by 0x805046F0; the literal at 0x8079D518, at or above copy 2, is first read by 0x80504A3C); the other boundaries of the interval are excluded by must-link spans (none needed: the interval is one boundary wide); 1 more distinct values pin the same cut.
 *
 * UNKNOWN. every body and, for a merged block, the file boundaries between its pieces.
 *
 * FLAGS. the `OS` lib's `cflags_os` (unmeasured until bodies exist).
 *
 * The unit's `.data`/`.sdata`/`.sbss` claims are the candidate's (config/RMHE08/splits.txt); the symbols they hold are
 * in the map (`ledger.py unit nw4r/fn_80504A3C.cpp`), and the pass that writes the bodies defines them.
 */
