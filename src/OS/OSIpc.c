/*
 * OS/OSIpc.c - the OS IPC buffer bounds: the Hi/Lo getters and `__OSInitIPCBuffer`.
 * RANGE. .text 0x804D5670-0x804D56B0 (3 functions); .sdata 0x80793FC0-0x80793FC8; .sbss 0x807953A0-0x807953A8.  Cut
 *    from the OS core band 0x804C1760-0x804D9B4C.  Evidence: `IpcBufferHi` (.sbss 0x807953A0) and `IpcBufferLo`
 *    (.sdata 0x80793FC0) are read only by these three functions.
 * FLAGS. `cflags_os` (configure.py), the flags of the unit it was cut from; unchanged by the cut and not measured
 *    with bodies yet.
 * NAMES. map names throughout.
 * RESIDUALS. every body is unwritten.
 * SHAPES. none yet.
 */
