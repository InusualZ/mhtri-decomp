/*
 * AX/AX.c - the AX audio library core: init, the init flag and the version registration.
 *
 * RANGE. .text 0x8046E3D0-0x8046E4F0 (4 functions, 0x120 B); .data 0x8060F8D8-0x8060F920; .sdata
 *    0x80793D10-0x80793D18; .sbss 0x80794EC8-0x80794ED0.  Cut from the old ARC/AX block; the left edge is
 *    `ARCCloseDir`'s end, the right edge 0x8046E4F0 is `__AXGetStackHead`, the first reader of the voice stacks.
 * FLAGS. the `OS` lib's group with `cflags_base` (configure.py `cflags=`): every function start of the band is
 *    16-aligned (`-O4,p` default alignment), not the 4-byte packing `cflags_os` gives.
 * NAMES. `AXInit` and `AXIsInit` are the map's names; the other two rows (0x8046E430, 0x8046E490) are unnamed.
 * EVIDENCE. `.data` 0x8060F8D8 is the build string `<< RVL_SDK - AX release build ... (0x4302_145) >>`, reached
 *    through `.sdata` 0x80793D10 by `AXInit` and one unnamed neighbour; `.sbss` 0x80794EC8 (8 B) is read by
 *    these four functions only.
 * RESIDUALS. no bodies yet: all 4 functions are unwritten (largest fn_8046E430, 0x60 B).
 * SHAPES. none yet: no body is written, so nothing is measured.
 */
