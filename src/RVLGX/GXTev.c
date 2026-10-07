/*
 * RVLGX/GXTev.c - the SDK GX TEV stage setup.
 *
 * RANGE. `.text` 0x804B9360-0x804B9A30; `.data` 0x8061AC20-0x8061AC98 (16 functions / 0x678 B).
 *   - `.data` 0x8061AC20 (the `GXSetTevOp` mode table) and 0x8061AC70 (the `GXSetTevOrder` map table) are read only
 *     here
 * FLAGS. the `OS` lib block of `configure.py` (`Wii/1.3`, `cflags_os`), the block of the unit this range was cut
 *   from; unmeasured until bodies exist.
 * RESIDUALS. the right edge (0x804B9A30, `GXSetFog`) is the first `.sdata2` reader of `GXPixel`; `GXSetNumTevStages`
 *   is placed here by the SDK function order. No body is written (16 functions), the largest `GXSetTevOrder`
 *   at 0x804B98A0 (0x15C B); `python tools/units/sweepcomments.py --unit RVLGX/GXTev.c` lists them.
 */
