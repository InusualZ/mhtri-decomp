/*
 * OS/OSLink.c - the one OS module-list initialiser kept by the linker, `__OSModuleInit`.
 * RANGE. .text 0x804D1440-0x804D1460 (1 function).  Cut from the OS core band 0x804C1760-0x804D9B4C.  Evidence:
 *    `__OSModuleInit` (24 bytes, no data) lies between `ExternalInterruptHandler` (interrupt code) and
 *    `OSInitMessageQueue`; the name is the SDK's module-linking file.
 * FLAGS. `cflags_os` (configure.py), the flags of the unit it was cut from; unchanged by the cut and not measured
 *    with bodies yet.
 * NAMES. GUESS: the file name (from the function it holds); the function name is the map's.
 * RESIDUALS. the body is unwritten; the seam is a one-function guess.
 * SHAPES. none yet.
 */
