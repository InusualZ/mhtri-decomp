/*
 * AX/AXVPB.c - the AX voice parameter blocks: the voice table, the per-frame voice service and PB sync, and the
 *    AXSetVoice* setters.
 *
 * RANGE. .text 0x804709B0-0x80471960 (17 functions, 0xFB0 B); .bss 0x806FB6E0-0x8070C414; .sbss
 *    0x80794FD8-0x80794FF8; .sdata2 0x8079CF48-0x8079CF68.  Cut from the old ARC/AX block between `AX/AXSPB.c`
 *    (0x804709B0) and `AX/AXProf.c` (0x80471960).
 * FLAGS. the `OS` lib's group with `cflags_base` (configure.py `cflags=`): every function start of the band is
 *    16-aligned (`-O4,p` default alignment), not the 4-byte packing `cflags_os` gives.
 * NAMES. the `__AXVPB*`, `__AXServiceVPB`, `__AXSyncPBs`, `__AXGetPBs` and `__AXSetPBDefault` names are the map's;
 *    the setters are unnamed; the file name `AXVPB.c` is a GUESS.
 * EVIDENCE. `.bss` 0x806FB6E0..0x8070C414 (0x10D34 B: the PB arrays and voice table; the tail after 0x80702EE0 has no
 *    direct reader and is assigned by order) is read by `__AXVPBInit`; `.sbss` 0x80794FD8..0x80794FF8 by
 *    `__AXServiceVPB`, `__AXSyncPBs` and `__AXVPBInit`; `.sdata2` 0x8079CF48..0x8079CF68 by the setters at
 *    0x804716E0 and 0x80471890. The setters at 0x80471550..0x80471830 write PB fields under
 *    `OSDisableInterrupts`.
 * RESIDUALS. no bodies yet: all 17 functions are unwritten (largest __AXServiceVPB, 0x550 B).
 * SHAPES. none yet: no body is written, so nothing is measured.
 */
