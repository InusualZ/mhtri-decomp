/*
 * MSL_C/mem.c - the MSL memory routines: `memmove`, `memchr`, `__memrchr`, `memcmp` and the `__copy_longs_*`
 *    helpers `memmove` calls.
 *
 * RANGE. .text 0x8045B598..0x8045B9D8 (8 functions in the map, 0x440 B).
 * FLAGS. the `Runtime.PPCEABI.H` lib's `cflags_ppceabi` (the group of the unit it was cut from; unmeasured until
 *    bodies exist).
 * NAMES. file name GUESS (MSL `mem`); `memcpy` and `memset` are registered separately
 *    (`Runtime.PPCEABI.H/memcpy.c`, `memset.c`).
 * EVIDENCE. call-graph closure: `memmove` calls the four `__copy_longs` helpers, nothing else calls them; no data.
 * RESIDUALS. COARSE: MSL may split `mem` and `mem_funcs`; no evidence to separate them.
 * SHAPES. none yet: no body is written, so nothing is measured; the map rows of the range are the work list
 *    (`ledger.py unit MSL_C/mem.c`).
 */
