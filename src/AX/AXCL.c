/*
 * AX/AXCL.c - the AX command list: the per-frame list builder `__AXNextFrame`, list cycle and address queries, the
 *    output mode and the aux return volumes.
 *
 * RANGE. .text 0x8046F310-0x8046FE20 (15 functions, 0xB10 B); .data 0x8060F920-0x80610980; .bss
 *    0x806FA760-0x806FA860; .sbss 0x80794F20-0x80794F48.  Cut from the old ARC/AX block between `AX/AXAux.c`
 *    (0x8046F310) and `AX/AXOut.c` (0x8046FE20).
 * FLAGS. the `OS` lib's group with `cflags_base` (configure.py `cflags=`): every function start of the band is
 *    16-aligned (`-O4,p` default alignment), not the 4-byte packing `cflags_os` gives.
 * NAMES. `__AXClInit`, `__AXNextFrame`, `__AXGetCommandList*` and the `AXGet/SetAux*ReturnVolume` names are the
 *    map's; `AXGetMode` (0x8046FD80, reads the mode word 0x80794F38; renamed from the generated name, the reverb
 *    effect compares it with 2 and `sound/snd_stream_reloc.cpp` stores 0..2 into the same word through
 *    0x8046FD70) is a GUESS; the file name `AXCL.c` is a GUESS.
 * EVIDENCE. `.data` 0x8060F920 (0x80 B), 0x8060F9A0 (0x20 B) are tables read by `__AXSyncPBs` through an extern and
 *    0x8060F9C0 (0xFC0 B) by `__AXClInit`: three consecutive objects after the AX build string and before the
 *    output unit's 0x2000 B buffer; `.sbss` 0x80794F20..0x80794F48 holds the return volumes, mode and
 *    compressor words, read by `__AXNextFrame` and `__AXClInit`.
 * RESIDUALS. no bodies yet: all 15 functions are unwritten (largest __AXNextFrame, 0x9BC B).
 * SHAPES. none yet: no body is written, so nothing is measured.
 */
