/*
 * OS/OSError.c - the OS error reporting: `OSReport`, `OSPanic`, the error-handler table and `__OSUnhandledException`.
 * RANGE. .text 0x804CD620-0x804CDD70 (5 functions); .data 0x8061C570-0x8061C850; .bss 0x8074D2F0-0x8074D340; .sdata
 *    0x80793F90-0x80793F98.  Cut from the OS core band 0x804C1760-0x804D9B4C.  Evidence: .data 0x8061C570 (" in
 *    \"%s\" on line %d") is read by `OSPanic` and `__OSUnhandledException`, and the table of 0x44 bytes
 *    `__OSErrorTable` (.bss 0x8074D2F0) by `OSSetErrorHandler` and `__OSUnhandledException`; the argument-packing
 *    helper at 0x804CDD70 (strlen/strcpy/memset) is not error code: it has a twin at 0x804D71F0 and heads the
 *    exec unit.
 * FLAGS. `cflags_os` (configure.py), the flags of the unit it was cut from; unchanged by the cut and not measured
 *    with bodies yet.
 * NAMES. map names throughout.
 * RESIDUALS. every body is unwritten.
 * SHAPES. none yet.
 */
