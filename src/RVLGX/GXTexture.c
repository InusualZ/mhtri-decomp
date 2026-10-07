/*
 * RVLGX/GXTexture.c - the SDK GX texture objects and texture cache (init/load objects, regions, TMEM setup).
 *
 * RANGE. `.text` 0x804B7F60-0x804B8F00; `.data` 0x8061AAF0-0x8061AC20; `.sdata` 0x80793E60-0x80793EA8; `.sdata2`
 *   0x8079D160-0x8079D198 (29 functions / 0xEC0 B).
 *   - `.data` jump tables 0x8061AAF0 (`__GetImageTileCount`) and 0x8061ABE4 (`GXInitTexObj`), `.sdata`
 *     0x80793E60..0x80793EA8 (TMEM size tables) and `.sdata2` 0x8079D160..0x8079D198 are read only here
 *   - the run opens at `__GetImageTileCount` (0x804B7F60): its jump table precedes the `GXInitTexObj` one in the same
 *     file
 * FLAGS. the `OS` lib block of `configure.py` (`Wii/1.3`, `cflags_os`), the block of the unit this range was cut
 *   from; unmeasured until bodies exist.
 * RESIDUALS. `__GetImageTileCount` was previously carried by the neighbouring unit (0x804B8020 edge); the `.data`
 *   jump-table order places it here. No body is written (29 functions), the largest `__GXSetTmemConfig` at
 *   0x804B8BB0 (0x350 B); `python tools/units/sweepcomments.py --unit RVLGX/GXTexture.c` lists them.
 */
