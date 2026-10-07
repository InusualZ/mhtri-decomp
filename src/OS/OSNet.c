/*
 * OS/OSNet.c - the OS network shutdown hook, `__OSInitNet`.
 * RANGE. .text 0x804D6740-0x804D6800 (1 function); .data 0x80629650-0x806297B8.  Cut from the OS core band
 *    0x804C1760-0x804D9B4C.  Evidence: the NWC24/kd device strings (.data 0x80629650..0x806297B8, one 0x168-byte
 *    label) are read only by `__OSInitNet`.
 * FLAGS. `cflags_os` (configure.py), the flags of the unit it was cut from; unchanged by the cut and not measured
 *    with bodies yet.
 * NAMES. map name.
 * RESIDUALS. the body is unwritten.
 * SHAPES. none yet.
 */
