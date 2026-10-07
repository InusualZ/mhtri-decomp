/*
 * RVLGX/GXPixel.c - the SDK GX pixel-engine state (fog, blend, z-mode, pixel format, display-list call).
 *
 * RANGE. `.text` 0x804B9A30-0x804BA230; `.data` 0x8061AC98-0x8061ACB8; `.sdata2` 0x8079D1A0-0x8079D1F0 (14 functions
 *   / 0x7C4 B).
 *   - `.data` 0x8061AC98 (the pixel-format table) and `.sdata2` 0x8079D1A0..0x8079D1F0 (the fog constants) are read
 *     only here
 * FLAGS. the `OS` lib block of `configure.py` (`Wii/1.3`, `cflags_os`), the block of the unit this range was cut
 *   from; unmeasured until bodies exist.
 * RESIDUALS. `GXCallDisplayList` (0x804BA1B0) has no data of its own and sits between this run and the transform
 *   functions; it is carried here by position. No body is written (14 functions), the largest `GXSetFog` at
 *   0x804B9A30 (0x22C B); `python tools/units/sweepcomments.py --unit RVLGX/GXPixel.c` lists them.
 */
