/*
 * RVLGX/GXMisc.c - the SDK GX miscellaneous control (misc setter, draw-done, PE pokes and peeks, token/finish interrupts).
 *
 * RANGE. `.text` 0x804B5DB0-0x804B65F0; `.sbss` 0x80795210-0x80795228 (20 functions / 0x798 B).
 *   - `.sbss` 0x80795210..0x80795228 is read by `GXDrawDone`, the token/finish interrupt handlers and `__GXPEInit`
 *     only
 * FLAGS. the `OS` lib block of `configure.py` (`Wii/1.3`, `cflags_os`), the block of the unit this range was cut
 *   from; unmeasured until bodies exist.
 * RESIDUALS. the left edge (0x804B5DB0) and the right edge (0x804B65F0, `__GXSetDirtyState`) lie in gaps with no data
 *   evidence; both are placed by the SDK function order. No body is written (20 functions), the largest
 *   `GXAbortFrame` at 0x804B5FA0 (0x1B4 B); `python tools/units/sweepcomments.py --unit RVLGX/GXMisc.c` lists
 *   them.
 */
