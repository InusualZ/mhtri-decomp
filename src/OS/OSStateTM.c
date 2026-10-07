/*
 * OS/OSStateTM.c - the OS state/STM event handling: STM init, shutdown-to-standby, hot reset and the state-event
 *    handler.
 * RANGE. .text 0x804D56B0-0x804D5E00 (12 functions); .data 0x80629518-0x806295E0; .bss 0x8074E0A0-0x8074E160; .sbss
 *    0x807953A8-0x807953D0.  Cut from the OS core band 0x804C1760-0x804D9B4C.  Evidence: the `__FILE__` anchor
 *    "OSStateTM.c" at .data 0x80629540 and the "/dev/stm/*" device strings (0x80629518); the STM buffers (.bss
 *    0x8074E0A0..0x8074E160) and the `Stm*` state (.sbss 0x807953A8..0x807953D0) are read only here.
 * FLAGS. `cflags_os` (configure.py), the flags of the unit it was cut from; unchanged by the cut and not measured
 *    with bodies yet.
 * NAMES. map names throughout.
 * RESIDUALS. every body is unwritten.
 * SHAPES. none yet.
 */
