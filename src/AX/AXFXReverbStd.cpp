/*
 * AX/AXFXReverbStd.cpp - STUB (no bodies yet).
 *
 * `.text` 0x80475E30..0x804770E0.  Sections of the candidate unit: .text 0x80475E30..0x804770E0; .data 0x80612B20..0x80612CE0; .sdata 0x80793D20..0x80793D28; .sdata2 0x8079D040..0x8079D080.
 *
 * WHAT IT IS. the AXFX effect run that follows `AXFXReverbHiExp`: an init (0x80475E30), a settings routine, a shutdown (0x804760C0, still called `AXFXReverbHiExpShutdown` in the map before this change), the effect callback and delay-line helpers, then the two small `AXFXSetHooks`/`AXFXGetHooks` accessors and thunks that end the AX library.
 *   GUESS: the next effect of the AXFX family is `AXFXReverbStd`; the caller `fn_80474E80` (in AXFXReverbHi.c) releases this unit's object through 0x804760C0.
 *
 * WHY IT SITS HERE. phase 1 grade strong, class pool: 9 pooled .sdata2 value(s) are held at two pool addresses (one TU never holds a value twice); for the best of them: .sdata2 value 4330000080000000 held at 0x8079D018 and again at 0x8079D058 (copy 1 last read by 0x80475200, copy 2 first read by 0x80476120); address order tightens it to cuts 0x80475E30..0x80476120 (the literal at 0x8079D000, at or below copy 1, is last read by 0x80475B00; the literal at 0x8079D058, at or above copy 2, is first read by 0x80476120); the other boundaries of the interval are excluded by must-link spans (0x80475FF0 inside lbl_8079D040 [.sdata2 pool, value copied elsewhere, span 4020 B]; 0x804760C0 inside lbl_8079D040 [.sdata2 pool, value copied elsewhere, span 4020 B]; 0x80476120 inside lbl_8079D040 [.sdata2 pool, value copied elsewhere, span 4020 B]); 8 more distinct values pin the same cut.
 *
 * UNKNOWN. every body and, for a merged block, the file boundaries between its pieces.
 *
 * FLAGS. the `OS` lib's `cflags_os` (unmeasured until bodies exist).
 *
 * The unit's `.data`/`.sdata`/`.sbss` claims are the candidate's (config/RMHE08/splits.txt); the symbols they hold are
 * in the map (`ledger.py unit AX/AXFXReverbStd.cpp`), and the pass that writes the bodies defines them.
 */
