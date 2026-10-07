/*
 * RVLGX/GXLight.c - the SDK GX lighting (light objects, channel colours and controls).
 *
 * RANGE. `.text` 0x804B77E0-0x804B7F60; `.sdata2` 0x8079D128-0x8079D160 (17 functions / 0x728 B).
 *   - `.sdata2` 0x8079D128..0x8079D160 (the light-direction and attenuation constants) is read only by this run,
 *     which also repeats `1.0f` at 0x8079D140 (a second pool entry for the same value than `GXInit`'s 0x8079D104)
 * FLAGS. the `OS` lib block of `configure.py` (`Wii/1.3`, `cflags_os`), the block of the unit this range was cut
 *   from; unmeasured until bodies exist.
 * RESIDUALS. No body is written (17 functions), the largest `fn_804B7820` at 0x804B7820 (0x19C B); `python
 *   tools/units/sweepcomments.py --unit RVLGX/GXLight.c` lists them.
 */
