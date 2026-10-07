/*
 * OS/OSMessage.c - the OS message queue: init, send, receive and jam.
 * RANGE. .text 0x804D1460-0x804D1740 (4 functions).  Cut from the OS core band 0x804C1760-0x804D9B4C.  Evidence: four
 *    functions that call the thread sleep/wakeup pair (0x804D4A30, 0x804D4B20) and the interrupt pair; no data.
 * FLAGS. `cflags_os` (configure.py), the flags of the unit it was cut from; unchanged by the cut and not measured
 *    with bodies yet.
 * NAMES. map names throughout.
 * RESIDUALS. every body is unwritten.
 * SHAPES. none yet.
 */
