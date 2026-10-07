/*
 * AX/AXOut.c - the AX output stage: the per-frame output buffer swap, the DSP bring-up, the output callback
 *    registration.
 *
 * RANGE. .text 0x8046FE20-0x80470570 (9 functions, 0x750 B); .data 0x80610980-0x80612980; .bss 0x806FA860-0x806FB660;
 *    .sdata 0x80793D18-0x80793D20; .sbss 0x80794F48-0x80794F88.  Cut from the old ARC/AX block between
 *    `AX/AXCL.c` (0x8046FE20) and `AX/AXSPB.c` (0x80470570).
 * FLAGS. the `OS` lib's group with `cflags_base` (configure.py `cflags=`): every function start of the band is
 *    16-aligned (`-O4,p` default alignment), not the 4-byte packing `cflags_os` gives.
 * NAMES. the `__AXOut*` and `AXRegisterCallback` names are the map's; the file name `AXOut.c` is a GUESS.
 * EVIDENCE. `.data` 0x80610980 (0x2000 B, the last object of the old block's `.data`) is read by `__AXOutInitDSP`;
 *    `.bss` 0x806FA860 (0x8E0 B), 0x806FB140, 0x806FB5C0 and 0x806FB620 are read by `__AXOutNewFrame`,
 *    `__AXOutInit` and `__AXOutQuit`; `.sdata` 0x80793D18 (three halfwords) by `__AXOutInitDSP`;
 *    `AXRegisterCallback` reads `.sbss` 0x80794F74, a word of the output group.
 * RESIDUALS. no bodies yet: all 9 functions are unwritten (largest __AXOutNewFrame, 0x260 B).
 * SHAPES. none yet: no body is written, so nothing is measured.
 */
