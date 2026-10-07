/*
 * OS/OSThread.c - the OS thread scheduler: thread init/create/exit/cancel/join/resume/suspend/sleep, run-queue
 *    handling and `OSSleepTicks`.
 * RANGE. .text 0x804D36D0-0x804D4D50 (23 functions); .bss 0x8074D698-0x8074E0A0; .sdata 0x80793FB8-0x80793FC0; .sbss
 *    0x80795390-0x807953A0.  Cut from the OS core band 0x804C1760-0x804D9B4C.  Evidence: the default thread, the
 *    idle thread and the run queue (.bss 0x8074D698..0x8074E0A0), `Reschedule`/`RunQueueHint`/`RunQueueBits`
 *    (.sbss 0x80795390..) and `SwitchThreadCallback` (.sdata 0x80793FB8) are read only by these functions.
 * FLAGS. `cflags_os` (configure.py), the flags of the unit it was cut from; unchanged by the cut and not measured
 *    with bodies yet.
 * NAMES. map names throughout (`OSInitThreadQueue`, `OSCreateThread`, `OSSleepThread`, ...).
 * RESIDUALS. every body is unwritten.
 * SHAPES. none yet.
 */
