/*
 * OS/OS.c - the OS core: FPR init, console type and arena clearing, `OSInit`, the exception vector and table, version
 *    registration and the app identity.
 * RANGE. .text 0x804C9F00-0x804CB440 (20 functions); .data 0x8061BC20-0x8061C0C8; .bss 0x8074D280-0x8074D2F0; .sdata
 *    0x80793F50-0x80793F80; .sbss 0x807952C8-0x80795310; .sdata2 0x8079D2D0-0x8079D2D8.  Cut from the OS core
 *    band 0x804C1760-0x804D9B4C.  Evidence: the build string "<< RVL_SDK - OS ... (0x4302_145) >>" opens its
 *    .data (0x8061BC20, read by `ReportOSInfo`, `OSInit`, `OSExceptionInit`); it owns
 *    `__OSInIPL`/`OSExceptionTable`/`BootInfo`/`ZeroPS`/`ZeroF` (.sbss 0x807952C8..0x80795310) and `__OSVersion`;
 *    `__OSInitAlarm` (0x804CB440) is the first function that reads `AlarmQueue`.
 * FLAGS. `cflags_os` (configure.py), the flags of the unit it was cut from; unchanged by the cut and not measured
 *    with bodies yet.
 * NAMES. `OSInit`, `OSExceptionInit`, `ReportOSInfo`, `__OSPSInit`, `OSRegisterVersion`, ... are the map's names; no
 *    `__FILE__` string for OS.c is in the image (the anchors the band names are OSReset.c, OSStateTM.c,
 *    OSPlayTime.c, OSLaunch.c).
 * RESIDUALS. every body is unwritten.
 * SHAPES. none yet.
 */
