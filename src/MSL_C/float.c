/*
 * MSL_C/float.c - the C library float constants (data only): `__float_nan` (0x80793CF0), `__float_huge`
 *    (0x80793CF4), `__double_huge` (0x80793CF8) and their neighbours.
 *
 * RANGE. .sdata 0x80793CF0..0x80793D00.
 * FLAGS. the `Runtime.PPCEABI.H` lib's `cflags_ppceabi` (the group of the unit it was cut from; unmeasured until
 *    bodies exist).
 * NAMES. file name GUESS (MSL `float.c`).
 * EVIDENCE. `.sdata` 0x80793CF0..0x80793D00 is read by the scanner, the number conversion, the string-to-float
 *    unit and the fdlibm units (sharing across units means a data-only TU); it follows the string unit's statics
 *    and precedes the AI library's `.sdata` in link order.
 * RESIDUALS. no bodies; no `.text`.
 * SHAPES. none yet: no body is written, so nothing is measured; the map rows of the range are the work list
 *    (`ledger.py unit MSL_C/float.c`).
 */
