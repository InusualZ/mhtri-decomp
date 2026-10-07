/*
 * OS/OSPlayTime.c - the OS play-time limit: the expired flag, the alarm, `__OSGetPlayTime` and `__OSInitPlayTime`.
 * RANGE. .text 0x804D6A10-0x804D71F0 (8 functions); .data 0x806297D8-0x80629818; .bss 0x8074E380-0x8074E3B0; .sdata
 *    0x80793FD0-0x80793FD8; .sbss 0x807953F0-0x80795408; .sdata2 0x8079D320-0x8079D330.  Cut from the OS core
 *    band 0x804C1760-0x804D9B4C.  Evidence: the `__FILE__` anchor "OSPlayTime.c" at .data 0x806297EC and the
 *    expired-flag path (0x806297D8) open its strings; `__OSExpireAlarm` (.bss 0x8074E380) and the `__OSExpire*`
 *    words (.sbss 0x807953F0..0x80795408) are read only here; `fn_804D71F0` is the argument-packing helper that
 *    heads the next unit (it has a twin at 0x804CDD70).
 * FLAGS. `cflags_os` (configure.py), the flags of the unit it was cut from; unchanged by the cut and not measured
 *    with bodies yet.
 * NAMES. `OSPlayTimeIsLimited`, `__OSWriteExpiredFlag`, `__OSGetPlayTime`, `__OSInitPlayTime` are the map's names.
 * RESIDUALS. every body is unwritten.
 * SHAPES. none yet.
 */
