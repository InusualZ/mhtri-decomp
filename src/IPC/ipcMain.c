/*
 * IPC/ipcMain.c - the SDK IPC core (buffer bounds, register access, init).
 *
 * RANGE. `.text` 0x804BB220-0x804BB310; `.sbss` 0x80795228-0x80795240 (7 functions / 0xCC B).
 *   - `.sbss` 0x80795228..0x80795240 is read by `IPCInit` and the buffer-bound accessors only; the client run starts
 *     at 0x804BB310
 * FLAGS. the `OS` lib block of `configure.py` (`Wii/1.3`, `cflags_os`), the block of the unit this range was cut
 *   from; unmeasured until bodies exist.
 * RESIDUALS. No body is written (7 functions), the largest `IPCInit` at 0x804BB220 (0x4C B); `python
 *   tools/units/sweepcomments.py --unit IPC/ipcMain.c` lists them.
 */
