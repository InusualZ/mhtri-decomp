/*
 * RVLGX/GXFrameBuf.c - the SDK GX frame-buffer copy (display copy source/dest/filter/gamma, copy clear, bounding box).
 *
 * RANGE. `.text` 0x804B6C00-0x804B77E0; `.data` 0x8061A9C0-0x8061AAF0; `.sdata2` 0x8079D118-0x8079D128 (15 functions
 *   / 0xB8C B).
 *   - the five render-mode objects `.data` 0x8061A9C0..0x8061AAF0 (0x3C each, 640x480 widths) are the globals of this
 *     file and are read by `__GXInitGX`; `.sdata2` 0x8079D118 / 0x8079D120 are the copy-scale constants read by
 *     `GXSetDispCopyYScale` and its helper
 * FLAGS. the `OS` lib block of `configure.py` (`Wii/1.3`, `cflags_os`), the block of the unit this range was cut
 *   from; unmeasured until bodies exist.
 * RESIDUALS. No body is written (15 functions), the largest `fn_804B6F70` at 0x804B6F70 (0x230 B); `python
 *   tools/units/sweepcomments.py --unit RVLGX/GXFrameBuf.c` lists them.
 */
