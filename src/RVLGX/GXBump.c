/*
 * RVLGX/GXBump.c - the SDK GX indirect-texture (bump) setup.
 *
 * RANGE. `.text` 0x804B8F00-0x804B9360; `.sdata2` 0x8079D198-0x8079D1A0 (9 functions / 0x430 B).
 *   - `.sdata2` 0x8079D198 (an eight-byte constant) is read by `GXSetIndTexMtx` only; the neighbours read disjoint
 *     pools
 * FLAGS. the `OS` lib block of `configure.py` (`Wii/1.3`, `cflags_os`), the block of the unit this range was cut
 *   from; unmeasured until bodies exist.
 * RESIDUALS. No body is written (9 functions), the largest `GXSetIndTexMtx` at 0x804B8F70 (0x140 B); `python
 *   tools/units/sweepcomments.py --unit RVLGX/GXBump.c` lists them.
 */
