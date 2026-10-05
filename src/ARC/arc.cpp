/*
 * ARC/arc.cpp - STUB (phase 4, docs/splits/phase4; no bodies yet).
 *
 * `.text` 0x8046D9F0..0x80474CB0.  Sections of the candidate unit: .text 0x8046D9F0..0x80474CB0; .data 0x8060F868..0x80612980; .bss 0x806F74E0..0x8070C414; .sdata 0x80793D08..0x80793D20; .sbss 0x80794EC8..0x80795008; .sdata2 0x8079CF48..0x8079CFF0.
 *
 * WHAT IT IS. the ARC archive library (`ARCInitHandle`, `ARCOpen`, `ARCConvertPathToEntrynum`, ...) plus the AX (audio) library that follows it up to 0x80474CB0 (AX, AXAux, AXCL, AXOut, AXSPB, AXVPB, AXProf).
 *   merged candidate pieces (guess cuts the renderer does not emit): arc, AX, AXAux, AXCL, AXOut, AXSPB, AXVPB, fn_80471550, AXProf, getBytesForAdpcmSamples (0x804719A0).
 *
 * WHY IT SITS HERE. phase 1 grade medium, class anchor: ARCInitHandle holds the `arc.c` __FILE__ anchor; ARC* roster.
 *
 * UNKNOWN. every body and, for a merged block, the file boundaries between its pieces.
 *
 * FLAGS. the `OS` lib's `cflags_os` (unmeasured until bodies exist).
 *
 * The unit's `.data`/`.sdata`/`.sbss` claims are the candidate's (config/RMHE08/splits.txt); the symbols they hold are
 * in the map (`ledger.py unit ARC/arc.cpp`), and the pass that writes the bodies defines them.
 */
