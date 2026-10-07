/*
 * OS/OSReset.c - the OS reset: shutdown functions, `OSRestart`, the return-to-menu family and `OSGetResetCode`.
 * RANGE. .text 0x804D21F0-0x804D2B90 (13 functions); .data 0x8061D410-0x8061D678; .sbss 0x80795380-0x80795390.  Cut
 *    from the OS core band 0x804C1760-0x804D9B4C.  Evidence: the `__FILE__` anchor "OSReset.c" at .data
 *    0x8061D410 opens its strings (read by `OSRestart`, `__OSReturnToMenu`, ...); `ShutdownFunctionQueue` (.sbss
 *    0x80795388) is read by the register/call/shutdown trio; `WriteSramCallback` reads the RTC control block and opens
 *    the next unit.
 * FLAGS. `cflags_os` (configure.py), the flags of the unit it was cut from; unchanged by the cut and not measured
 *    with bodies yet.
 * NAMES. map names throughout.
 * RESIDUALS. every body is unwritten.
 * SHAPES. none yet.
 */
