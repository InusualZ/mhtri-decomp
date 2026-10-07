/*
 * OS/OSMutex.c - the OS mutex: init, lock, unlock, `__OSUnlockAllMutex` and the two thread-queue branch thunks that
 *    end it.
 * RANGE. .text 0x804D1EE0-0x804D2160 (6 functions).  Cut from the OS core band 0x804C1760-0x804D9B4C.  Evidence: four
 *    functions that call the thread layer; the two 4-byte `b OSInitThreadQueue` / `b OSWakeupThread` thunks at
 *    0x804D2140/0x804D2150 follow them before `__OSReboot`.
 * FLAGS. `cflags_os` (configure.py), the flags of the unit it was cut from; unchanged by the cut and not measured
 *    with bodies yet.
 * NAMES. map names throughout; the two thunks keep the map's GUESS names
 *    `OSInitThreadQueueThunk`/`OSWakeupThreadThunk`; their placement at this unit's tail is a GUESS.
 * RESIDUALS. every body is unwritten.
 * SHAPES. none yet.
 */
