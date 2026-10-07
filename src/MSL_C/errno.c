/*
 * MSL_C/errno.c - the C library's `errno` word (data only, no bodies).
 *
 * RANGE. .sbss 0x80794E08..0x80794E10.
 * FLAGS. the `Runtime.PPCEABI.H` lib's `cflags_ppceabi` (the group of the unit it was cut from; unmeasured until
 *    bodies exist).
 * NAMES. file name GUESS (MSL `errno.c`).
 * EVIDENCE. `.sbss` 0x80794E08 (8 B) is read or written by `_ftell`, `_fseek`, `strtol`, `atoi`, the fdlibm units
 *    and the sqrt wrapper: shared by many units, so it belongs to a data-only TU; its place in the `.sbss` order
 *    is between the allocator's word (0x80794E00) and the console unit's (0x80794E10).
 * RESIDUALS. no bodies; no `.text`.
 * SHAPES. none yet: no body is written, so nothing is measured; the map rows of the range are the work list
 *    (`ledger.py unit MSL_C/errno.c`).
 */
