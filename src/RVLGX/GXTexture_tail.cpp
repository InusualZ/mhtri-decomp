/*
 * RVLGX/GXTexture_tail.cpp - STUB (no bodies yet).
 *
 * `.text` 0x804B8020..0x804C1760.  Sections of the candidate unit: .text 0x804B8020..0x804C1760; .data 0x8061ABE4..0x8061AE00; .bss 0x80747300..0x80748B90; .sdata 0x80793E60..0x80793F00; .sbss 0x80795228..0x80795298; .sdata2 0x8079D160..0x8079D270.
 *
 * WHAT IT IS. the tail of the GX library from `GXInitTexObj` (GXTexture, GXBump, GXTev, GXPixel, GXDisplayList, GXTransform, GXPerf), then the IPC library (`ipcMain`, `ipcclt`, memory, `ipcProfile`) and the KPAD run up to 0x804C1760.
 *   merged candidate pieces (guess cuts the renderer does not emit): GXTexture_tail, GXBump, GXTev, GXPixel, GXDisplayList, GXTransform, GXPerf, ipcMain, ipcclt, memory, ipcProfile, fn_804BD780.
 *
 * WHY IT SITS HERE. phase 1 grade medium, class inherited: 0x804B8020 follows the registered unit EXI/ProbeBarnacle.c with no function in between (<= 16 B pad); baseline order/coverage/text-cut PASS.
 *
 * UNKNOWN. every body and, for a merged block, the file boundaries between its pieces.
 *
 * FLAGS. the `OS` lib's `cflags_os` (unmeasured until bodies exist).
 *
 * The unit's `.data`/`.sdata`/`.sbss` claims are the candidate's (config/RMHE08/splits.txt); the symbols they hold are
 * in the map (`ledger.py unit RVLGX/GXTexture_tail.cpp`), and the pass that writes the bodies defines them.
 */
