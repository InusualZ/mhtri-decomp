/*
 * TRK/TRK_flush_cache.cpp - STUB (no bodies yet).
 *
 * `.text` 0x80467EEC..0x8046D9F0.  Sections of the candidate unit: .text 0x80467EEC..0x8046D9F0; .rodata 0x805734A8..0x80573530; .data 0x8060F538..0x8060F868; .bss 0x806F5020..0x806F74E0; .sdata 0x80793D00..0x80793D08; .sbss 0x80794E38..0x80794EC8; .sdata2 0x8079CF30..0x8079CF40.
 *
 * WHAT IT IS. a merged guess block: `s_tan` (medium pool cut at its start), the libm wrapper thunks (`log10`, `pow`), the MSL `extras` string routines (`stricmp`, `strupr`), `TRKAccessFile`, the `gdev_cc_*` debugger channel, the circle buffer, the TRK flush-cache run and the AI (audio interface) library.
 *   merged candidate pieces (guess cuts the renderer does not emit): fn_80467EEC, fn_80467F64, extras_str, TRKAccessFile, gdev_cc, CircleBuffer, TRK_flush_cache, ai.
 *
 * WHY IT SITS HERE. phase 1 grade medium, class pool: 1 pooled .sdata2 value(s) are held at two pool addresses (one TU never holds a value twice); for the best of them: .sdata2 value 0000000000000000 held at 0x8079CF28 and again at 0x8079CF30 (copy 1 last read by 0x80467E20, copy 2 first read by 0x80467EEC); the other boundaries of the interval are excluded by must-link spans (none needed: the interval is one boundary wide).
 *
 * UNKNOWN. every body and, for a merged block, the file boundaries between its pieces.
 *
 * FLAGS. the `OS` lib's `cflags_os` (unmeasured until bodies exist).
 *
 * The unit's `.data`/`.sdata`/`.sbss` claims are the candidate's (config/RMHE08/splits.txt); the symbols they hold are
 * in the map (`ledger.py unit TRK/TRK_flush_cache.cpp`), and the pass that writes the bodies defines them.
 */
