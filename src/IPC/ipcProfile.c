/*
 * IPC/ipcProfile.c - the SDK IPC profiling counters (`IPCiProf*`).
 *
 * RANGE. `.text` 0x804BD5A0-0x804BD780; `.bss` 0x80747440-0x80747540; `.sbss` 0x80795258-0x80795260 (4 functions /
 *   0x1C8 B).
 *   - `IpcFdArray` / `IpcReqPtrArray` (.bss 0x80747440..0x80747540) and `.sbss` 0x80795258..0x80795260 are read only
 *     here
 * FLAGS. the `OS` lib block of `configure.py` (`Wii/1.3`, `cflags_os`), the block of the unit this range was cut
 *   from; unmeasured until bodies exist.
 * RESIDUALS. No body is written (4 functions), the largest `IPCiProfInit` at 0x804BD5A0 (0xB8 B); `python
 *   tools/units/sweepcomments.py --unit IPC/ipcProfile.c` lists them.
 */
