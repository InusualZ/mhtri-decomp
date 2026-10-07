/*
 * OS/OSExec.c - the OS program launcher: argument packing, the exec parameters, `__OSLaunchNextFirmware`,
 *    `__OSLaunchMenu`, `__OSBootDolSimple`, `__OSBootDol`.
 * RANGE. .text 0x804CDD70-0x804CF350 (11 functions); .data 0x8061C850-0x8061C8C0; .bss 0x8074D340-0x8074D360; .sdata
 *    0x80793F98-0x80793FA8; .sbss 0x80795330-0x80795348.  Cut from the OS core band 0x804C1760-0x804D9B4C.
 *    Evidence: .data 0x8061C850 ("OSExec(): Failed to exec") and 0x8061C8B0 (the apploader date) are read by
 *    `__OSLaunchNextFirmware` and `__OSBootDolSimple`; the first four functions are argument-packing helpers
 *    (`strlen`/`strcpy`/`memset`) whose twin heads the OSLaunch unit at 0x804D71F0.
 * FLAGS. `cflags_os` (configure.py), the flags of the unit it was cut from; unchanged by the cut and not measured
 *    with bodies yet.
 * NAMES. `__OSLaunchNextFirmware`, `__OSLaunchMenu`, `__OSBootDolSimple`, `__OSBootDol`, `__OSGetExecParams` are the
 *    map's names; the left edge (0x804CDD70) is medium evidence (no data read by the four helpers).
 * RESIDUALS. every body is unwritten.
 * SHAPES. none yet.
 */
