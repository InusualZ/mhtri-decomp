/*
 * AX/AXAlloc.c - the AX voice allocator: the free and callback voice stacks, `AXAcquireVoice` and `AXFreeVoice`.
 *
 * RANGE. .text 0x8046E4F0-0x8046EA90 (10 functions, 0x5A0 B); .bss 0x806F74E0-0x806F75E0; .sbss
 *    0x80794ED0-0x80794ED8.  Cut from the old ARC/AX block between `AX/AX.c` (0x8046E4F0) and `AX/AXAux.c`
 *    (0x8046EA90).
 * FLAGS. the `OS` lib's group with `cflags_base` (configure.py `cflags=`): every function start of the band is
 *    16-aligned (`-O4,p` default alignment), not the 4-byte packing `cflags_os` gives.
 * NAMES. the `__AX*` and `AX*` names are the map's; the map row `PatInterface_VTable` (0x8046E5C0) is a stack helper,
 *    not a vtable (left as is: no source names it); the file name `AXAlloc.c` is a GUESS from the library's
 *    naming.
 * EVIDENCE. `.bss` 0x806F74E0 and 0x806F7560 (0x80 B each) are read only by the stack helpers, `AXFreeVoice` and
 *    `AXAcquireVoice`; `.sbss` 0x80794ED0 (8 B, the stack heads) is read by the same functions; the next
 *    `.bss` object (0x806F75E0) is `__AXAuxInit`'s.
 * RESIDUALS. no bodies yet: all 10 functions are unwritten (largest AXAcquireVoice, 0x184 B).
 * SHAPES. none yet: no body is written, so nothing is measured.
 */
