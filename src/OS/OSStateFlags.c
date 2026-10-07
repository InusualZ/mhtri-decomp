/*
 * OS/OSStateFlags.c - the OS state flags: `__OSWriteStateFlags` and `__OSReadStateFlags`.
 * RANGE. .text 0x804D6520-0x804D6740 (2 functions); .data 0x80629628-0x80629650; .bss 0x8074E360-0x8074E380.  Cut
 *    from the OS core band 0x804C1760-0x804D9B4C.  Evidence: the state.dat path (.data 0x80629628) and the
 *    0x20-byte buffer (.bss 0x8074E360) are read only by these two functions.
 * FLAGS. `cflags_os` (configure.py), the flags of the unit it was cut from; unchanged by the cut and not measured
 *    with bodies yet.
 * NAMES. map names throughout.
 * RESIDUALS. every body is unwritten.
 * SHAPES. none yet.
 */
