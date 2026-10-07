/*
 * OS/OSMemory.c - the OS memory layout: physical/simulated sizes, the memory-protection interrupt handler, BAT setup
 *    and `__OSInitMemoryProtection`.
 * RANGE. .text 0x804D1740-0x804D1EE0 (15 functions); .data 0x8061D400-0x8061D410; .sbss 0x80795370-0x80795378.  Cut
 *    from the OS core band 0x804C1760-0x804D9B4C.  Evidence: its `ShutdownFunctionInfo` (.data 0x8061D400) is
 *    address-taken by `__OSInitMemoryProtection` (0x804D1E94, the OSAlarm one is 0x8061C0C8) and its
 *    `initialized` flag (.sbss 0x80795370) by the same function.
 * FLAGS. `cflags_os` (configure.py), the flags of the unit it was cut from; unchanged by the cut and not measured
 *    with bodies yet.
 * NAMES. `OSGetPhysicalMem2Size`, `BATConfig`, `__OSInitMemoryProtection` are the map's names; `MEMIntrruptHandler`
 *    is the map's (sic).
 * RESIDUALS. every body is unwritten.
 * SHAPES. none yet.
 */
