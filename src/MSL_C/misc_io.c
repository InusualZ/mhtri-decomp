/*
 * MSL_C/misc_io.c - float classification (`__fpclassifyf`, `__signbitd`, `__fpclassifyd`) and `__stdio_atexit`.
 *
 * RANGE. .text 0x8045B9D8..0x8045BADC (4 functions in the map, 0x104 B).
 * FLAGS. the `Runtime.PPCEABI.H` lib's `cflags_ppceabi` (the group of the unit it was cut from; unmeasured until
 *    bodies exist).
 * NAMES. file name GUESS; the unit merges two small groups that may be two files.
 * EVIDENCE. family and adjacency only: the classifiers are called by the number-conversion unit, `s_ldexp` and
 *    `g3d/g3d_anmchr.cpp`, `__stdio_atexit` stores the stdio exit hook (`.sbss` 0x80794E1C, defined by the
 *    abort/exit unit).
 * RESIDUALS. COARSE: no data separates the classifiers from `__stdio_atexit`.
 * SHAPES. none yet: no body is written, so nothing is measured; the map rows of the range are the work list
 *    (`ledger.py unit MSL_C/misc_io.c`).
 */
