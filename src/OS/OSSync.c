/*
 * OS/OSSync.c - the OS system-call vector and its installer, `__OSInitSystemCall`.
 * RANGE. .text 0x804D3640-0x804D36D0 (3 functions).  Cut from the OS core band 0x804C1760-0x804D9B4C.  Evidence:
 *    `__OSSystemCallVectorStart`/`End` labels bracket the 0x20-byte vector body at 0x804D3640 and
 *    `__OSInitSystemCall` copies it; a 4-byte `blr` follows; `__OSThreadInit` (0x804D36D0) opens the thread unit.
 * FLAGS. `cflags_os` (configure.py), the flags of the unit it was cut from; unchanged by the cut and not measured
 *    with bodies yet.
 * NAMES. map names throughout.
 * RESIDUALS. every body is unwritten.
 * SHAPES. none yet.
 */
