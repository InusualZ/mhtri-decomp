/*
 * NAND/nand.c - the Revolution SDK NAND library: create/delete/read/write/seek/open/close/status, path conversion,
 *    home directory, check and the error log.
 * RANGE. .text 0x804C70E0-0x804C9F00 (58 functions); .rodata 0x80573A10-0x80573B58; .data 0x8061B9A0-0x8061BC20; .bss
 *    0x8074CF40-0x8074D280; .sdata 0x80793F08-0x80793F50; .sbss 0x807952B8-0x807952C8.  Cut from the OS core band
 *    0x804C1760-0x804D9B4C.  Evidence: the build string "<< RVL_SDK - NAND ... (0x4302_145) >>" opens .data at
 *    0x8061B9A0 and the OS one opens the next library at 0x8061BC20; every NAND function reads only .data
 *    0x8061B9A0..0x8061BC20, .bss 0x8074CF40.., .sdata 0x80793F08..0x80793F50 and .sbss 0x807952B8..0x807952C8;
 *    `__OSFPRInit` (0x804C9F00) is the first function that reads OS-owned data (.sbss `ZeroPS`/`ZeroF`).
 * FLAGS. `cflags_os` (configure.py), the flags of the unit it was cut from; unchanged by the cut and not measured
 *    with bodies yet.
 * NAMES. the `NAND*`/`nand*` names are the map's (dump); the SDK splits the library over several source files (nand,
 *    NANDOpenClose, NANDCore, NANDCheck, NANDLogging) but the image carries no `__FILE__` string for it and its
 *    `.data` strings are ordered by first use across the whole run, so no seam inside is proven and the library
 *    is one unit.
 * RESIDUALS. every body is unwritten.
 * SHAPES. none yet.
 */
