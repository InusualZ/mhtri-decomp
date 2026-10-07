/*
 * AX/AXAux.c - the AX auxiliary effect buses: aux buffer init and quit, the A/B/C input and output getters, aux
 *    processing and the aux-A callback.
 *
 * RANGE. .text 0x8046EA90-0x8046F310 (21 functions, 0x880 B); .bss 0x806F75E0-0x806FA760; .sbss
 *    0x80794ED8-0x80794F20.  Cut from the old ARC/AX block between `AX/AXAlloc.c` (0x8046EA90) and `AX/AXCL.c`
 *    (0x8046F310).
 * FLAGS. the `OS` lib's group with `cflags_base` (configure.py `cflags=`): every function start of the band is
 *    16-aligned (`-O4,p` default alignment), not the 4-byte packing `cflags_os` gives.
 * NAMES. the `__AXGetAux*`, `__AXProcessAux` and `AXRegisterAuxACallback` names are the map's; the file name
 *    `AXAux.c` is a GUESS.
 * EVIDENCE. `.bss` 0x806F75E0 (0xF0E B), 0x806F87E0 (0x1200 B) and 0x806F99E0 (0xD80 B) are the aux A, B and C
 *    buffers, read by `__AXAuxInit` and the getters; `.sbss` 0x80794ED8..0x80794F20 are the aux pointers and
 *    callbacks, first read by `__AXAuxInit`. 0x80794F38 (the mode word) is defined in `AX/AXCL.c` and read
 *    here.
 * RESIDUALS. no bodies yet: all 21 functions are unwritten (largest __AXProcessAux, 0x3AC B).
 * SHAPES. none yet: no body is written, so nothing is measured.
 */
