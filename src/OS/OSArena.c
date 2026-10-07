/*
 * OS/OSArena.c - the OS arena bounds: the MEM1/MEM2 arena Hi/Lo getters and setters and `OSAllocFromMEM1ArenaLo`.
 * RANGE. .text 0x804CC030-0x804CC130 (13 functions); .sdata 0x80793F88-0x80793F90; .sbss 0x80795328-0x80795330.  Cut
 *    from the OS core band 0x804C1760-0x804D9B4C.  Evidence: the four arena words (.sdata 0x80793F88/0x80793F8C,
 *    .sbss 0x80795328/0x8079532C) are read only by these 13 functions.
 * FLAGS. `cflags_os` (configure.py), the flags of the unit it was cut from; unchanged by the cut and not measured
 *    with bodies yet.
 * NAMES. map names throughout.
 * RESIDUALS. every body is unwritten.
 * SHAPES. none yet.
 */
