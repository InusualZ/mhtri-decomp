/*
 * RVLGX/GXInit.c - the SDK GX initialisation (`GXInit`, `__GXInitGX`, shutdown hook).
 *
 * RANGE. `.text` 0x804B32D0-0x804B4460; `.data` 0x8061A560-0x8061A7A0; `.bss` 0x80746C20-0x807472A0; `.sdata`
 *   0x80793E48-0x80793E50; `.sbss` 0x807951C8-0x807951F0; `.sdata2` 0x8079D0F0-0x8079D118 (6 functions / 0x116C
 *   B).
 *   - `__GXVersion` (.sdata 0x80793E48) points at the version string `<< RVL_SDK - GX ... >>` (.data 0x8061A560);
 *     `GXTexRegionAddrTable` (.data 0x8061A6D0), `GXShutdownFuncInfo`, `FifoObj` (.bss 0x80746C20) are read only by
 *     `GXInit`; the 0x600-byte `.bss` object 0x80746CA0 has no reader in the band and follows `FifoObj`
 *   - `__GXData` (.sdata2 0x8079D0F0) and the register pointers `__piReg`/`__cpReg`/`__peReg`/`__memReg` (.sbss
 *     0x807951C8..0x807951D8) open the library's data and are defined here
 *   - the first function (0x804B32D0) follows `ISFS_ShutdownAsync`; the right edge is where `GXFifo` statics begin
 *     (0x804B4460 reads 0x807951F4)
 * FLAGS. the `OS` lib block of `configure.py` (`Wii/1.3`, `cflags_os`), the block of the unit this range was cut
 *   from; unmeasured until bodies exist.
 * RESIDUALS. `__GXData` is read by every GX unit and the pool census reads it as a shared literal; it is an SDK
 *   global, and the repeated `1.0f` pool entries (0x8079D104, 0x8079D140, 0x8079D1A4, 0x8079D1F4) show four
 *   separate files. No body is written (6 functions), the largest `__GXInitGX` at 0x804B3BA0 (0x8C0 B);
 *   `python tools/units/sweepcomments.py --unit RVLGX/GXInit.c` lists them.
 */
