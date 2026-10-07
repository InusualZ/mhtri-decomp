/*
 * AX/AXProf.c - the AX profiling: the current profile accessor.
 *
 * RANGE. .text 0x80471960-0x804719A0 (1 functions, 0x40 B); .sbss 0x80794FF8-0x80795008.  Cut from the old ARC/AX
 *    block between `AX/AXVPB.c` (0x80471960) and `DSPADPCM/dspadpcm.c` (0x804719A0).
 * FLAGS. the `OS` lib's group with `cflags_base` (configure.py `cflags=`): every function start of the band is
 *    16-aligned (`-O4,p` default alignment), not the 4-byte packing `cflags_os` gives.
 * NAMES. `__AXGetCurrentProfile` is the map's name; the file name `AXProf.c` is a GUESS.
 * EVIDENCE. `.sbss` 0x80794FF8..0x80795008 (four words) is read by `__AXGetCurrentProfile` alone; it is the last
 *    `.sbss` of the old block.
 * RESIDUALS. no bodies yet: all 1 functions are unwritten (largest __AXGetCurrentProfile, 0x40 B).
 * SHAPES. none yet: no body is written, so nothing is measured.
 */
