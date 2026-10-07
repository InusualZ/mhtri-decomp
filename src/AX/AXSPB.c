/*
 * AX/AXSPB.c - the AX studio parameter block: the studio getter and print, the main and remote depop fades, the SPB
 *    init and quit and the voice depop.
 *
 * RANGE. .text 0x80470570-0x804709B0 (7 functions, 0x440 B); .bss 0x806FB660-0x806FB6E0; .sbss 0x80794F88-0x80794FD8.
 *    Cut from the old ARC/AX block between `AX/AXOut.c` (0x80470570) and `AX/AXVPB.c` (0x804709B0).
 * FLAGS. the `OS` lib's group with `cflags_base` (configure.py `cflags=`): every function start of the band is
 *    16-aligned (`-O4,p` default alignment), not the 4-byte packing `cflags_os` gives.
 * NAMES. the `__AX*` names are the map's; the file name `AXSPB.c` is a GUESS.
 * EVIDENCE. `.bss` 0x806FB660 (0x80 B) is read by `__AXGetStudio` and `__AXPrintStudio` only; `.sbss`
 *    0x80794F88..0x80794FD8 (twenty words) is read by `__AXPrintStudio`, `__AXSPBInit` and `__AXDepopVoice`
 *    and by nothing else; `__AXGetNumVoices` reads the next group (0x80794FE8).
 * RESIDUALS. no bodies yet: all 7 functions are unwritten (largest __AXPrintStudio, 0x178 B).
 * SHAPES. none yet: no body is written, so nothing is measured.
 */
