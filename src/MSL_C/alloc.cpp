/*
 * MSL_C/alloc.cpp - the MSL block allocator: sub-block link and merge helpers, the fixed-pool free path and the
 *    free/clear entry.
 *
 * RANGE. .text 0x80458C94..0x804591A8 (4 functions in the map, 0x514 B); .rodata 0x80572528..0x80572540; .bss
 *    0x806F4CC8..0x806F4D00; .sbss 0x80794E00..0x80794E08.
 * FLAGS. the `Runtime.PPCEABI.H` lib's `cflags_ppceabi` (the group of the unit it was cut from; unmeasured until
 *    bodies exist).
 * NAMES. file name kept from the registration (a `.cpp` name for a C file; renaming is a registration move left to
 *    the lane that writes the bodies).
 * EVIDENCE. `.rodata` 0x80572528 (fixed pool size table) is read only by the free path; `.bss` 0x806F4CC8 and
 *    `.sbss` 0x80794E00 are read only by its last function; every callee is inside the unit or `__sys_free` (the
 *    unit before it); `__close_all` (next unit) calls its free entry.
 * RESIDUALS. no bodies written.
 * SHAPES. none yet: no body is written, so nothing is measured; the map rows of the range are the work list
 *    (`ledger.py unit MSL_C/alloc.cpp`).
 */
