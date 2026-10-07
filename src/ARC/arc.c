/*
 * ARC/arc.c - the ARC archive library: archive handle init, file open by path or entry number, path lookup and the
 *    directory walk.
 *
 * RANGE. .text 0x8046D9F0-0x8046E3D0 (12 functions, 0x9E0 B); .data 0x8060F868-0x8060F8D8; .sdata
 *    0x80793D08-0x80793D10.  Cut from the old ARC/AX block 0x8046D9F0-0x80474CB0; the left edge is the end of
 *    `AI_SDK/ai.c`, the right edge 0x8046E3D0 is `AXInit`.
 * FLAGS. the `OS` lib's group with `cflags_base` (configure.py `cflags=`): every function start of the band is
 *    16-aligned (`-O4,p` default alignment), not the 4-byte packing `cflags_os` gives.
 * NAMES. the ARC* names are the map's (the dump agrees); the file name `arc.c` is a GUESS from the library.
 * EVIDENCE. `.data` 0x8060F868 (0x22 B) and 0x8060F88C (0x46 B) are the message strings of `ARCInitHandle` and
 *    `ARCOpen`; `.sdata` 0x80793D08 (6 B) is read by `ARCInitHandle` only; the twelve ARC* rows are contiguous
 *    in the map and call only each other, and the next reader of a new data object is `AXInit` (the AX build
 *    string).
 * RESIDUALS. no bodies yet: all 12 functions are unwritten (largest ARCOpen, 0x2AC B).
 * SHAPES. none yet: no body is written, so nothing is measured.
 */
