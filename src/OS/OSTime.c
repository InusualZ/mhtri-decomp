/*
 * OS/OSTime.c - the OS time base: `OSGetTime`/`OSGetTick`, system time and the calendar conversions.
 * RANGE. .text 0x804D4D50-0x804D5420 (6 functions); .data 0x8061D678-0x8061D6D8.  Cut from the OS core band
 *    0x804C1760-0x804D9B4C.  Evidence: the two month tables (.data 0x8061D678, 0x8061D6A8) are read only by
 *    `OSTicksToCalendarTime` and `OSCalendarTimeToTicks`.
 * FLAGS. `cflags_os` (configure.py), the flags of the unit it was cut from; unchanged by the cut and not measured
 *    with bodies yet.
 * NAMES. map names throughout.
 * RESIDUALS. every body is unwritten.
 * SHAPES. none yet.
 */
