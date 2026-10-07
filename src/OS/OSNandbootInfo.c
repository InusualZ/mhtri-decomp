/*
 * OS/OSNandbootInfo.c - the OS NAND boot info: `__OSCreateNandbootInfo` and `__OSWriteNandbootInfo`.
 * RANGE. .text 0x804D6800-0x804D6A10 (2 functions); .data 0x806297B8-0x806297D8.  Cut from the OS core band
 *    0x804C1760-0x804D9B4C.  Evidence: the NANDBOOTINFO path (.data 0x806297B8) is read only by these two
 *    functions.
 * FLAGS. `cflags_os` (configure.py), the flags of the unit it was cut from; unchanged by the cut and not measured
 *    with bodies yet.
 * NAMES. map names throughout.
 * RESIDUALS. every body is unwritten.
 * SHAPES. none yet.
 */
