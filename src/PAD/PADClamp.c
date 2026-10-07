/*
 * PAD/PADClamp.c - the PAD stick clamp: the stick clamp helper and `PADClamp`.
 * RANGE. .text 0x804D80B0-0x804D82D0 (2 functions).  Cut from the OS core band 0x804C1760-0x804D9B4C.  Evidence:
 *    `PADClamp` is the map's name for the 0xF0-byte function at 0x804D81E0 and `fn_804D80B0` is its only callee;
 *    neither reads PAD state (.bss 0x8074E3B0, first read by `fn_804D82D0`).
 * FLAGS. `cflags_os` (configure.py), the flags of the unit it was cut from; unchanged by the cut and not measured
 *    with bodies yet.
 * NAMES. GUESS: the file name (from `PADClamp`); medium evidence for the right edge.
 * RESIDUALS. every body is unwritten.
 * SHAPES. none yet.
 */
