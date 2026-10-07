/*
 * OS/OSFatal.c - the OS fatal-error screen: the video/GX configuration helpers, `OSFatal` and the text drawer.
 * RANGE. .text 0x804CF350-0x804CFF80 (4 functions); .bss 0x8074D360-0x8074D640; .sdata 0x80793FA8-0x80793FB0; .sdata2
 *    0x8079D2D8-0x8079D318.  Cut from the OS core band 0x804C1760-0x804D9B4C.  Evidence: the bss run
 *    0x8074D360..0x8074D640 and the 14 float constants at .sdata2 0x8079D2D8..0x8079D318 are read only by
 *    `OSFatal` (0x804CF7A0) and `fn_804CF990`; the helper at 0x804CF350 calls into the font code (0x804D09E0) and
 *    the one at 0x804CF680 into VI (0x804E7F60, 0x804E8630, 0x804E8CE0); `__OSBootDol` ends the exec unit at
 *    0x804CF350.
 * FLAGS. `cflags_os` (configure.py), the flags of the unit it was cut from; unchanged by the cut and not measured
 *    with bodies yet.
 * NAMES. `OSFatal` is the map's name; the left edge (0x804CF350) is medium evidence.
 * RESIDUALS. every body is unwritten.
 * SHAPES. none yet.
 */
