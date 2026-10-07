/*
 * RVLGX/GXGeometry.c - the SDK GX geometry state (dirty-state flush, `GXBegin`, line/point/cull setters).
 *
 * RANGE. `.text` 0x804B65F0-0x804B6C00 (9 functions / 0x5D0 B).
 *   - no data of its own; every function reads only `__GXData`. The run is bounded by the `GXMisc` sbss end and the
 *     `GXFrameBuf` `.sdata2` 0x8079D118 first read
 * FLAGS. the `OS` lib block of `configure.py` (`Wii/1.3`, `cflags_os`), the block of the unit this range was cut
 *   from; unmeasured until bodies exist.
 * RESIDUALS. both edges lie in gaps with no data evidence; the range is placed by the SDK function order. No body is
 *   written (9 functions), the largest `__GXSetDirtyState` at 0x804B65F0 (0x280 B); `python
 *   tools/units/sweepcomments.py --unit RVLGX/GXGeometry.c` lists them.
 */
