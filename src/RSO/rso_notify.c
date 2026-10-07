/*
 * RSO/rso_notify.c - the four RSO notify stubs `RSONotifyModuleLoaded`, `RSONotifyModuleUnloaded`,
 *    `RSONotifyPreRSOLink`, `RSONotifyPostRSOLink`.
 * RANGE. .text 0x804D9B3C-0x804D9B4C (4 functions).  Cut from the OS core band 0x804C1760-0x804D9B4C.  Evidence: four
 *    4-byte `blr` bodies at 0x804D9B3C..0x804D9B4C, ending where `RSO/runtime.c` starts.
 * FLAGS. `cflags_os` (configure.py), the flags of the unit it was cut from; unchanged by the cut and not measured
 *    with bodies yet.
 * NAMES. map names; the file name is a GUESS.
 * RESIDUALS. every body is unwritten.
 * SHAPES. none yet.
 */
