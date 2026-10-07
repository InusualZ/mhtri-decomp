/*
 * MSL_C/abort_exit.c - the abort path and the runtime-constraint handler word.
 *
 * RANGE. .text 0x80463D98..0x80463DE4 (2 functions in the map, 0x4C B); .sbss 0x80794E18..0x80794E28.
 * FLAGS. the `Runtime.PPCEABI.H` lib's `cflags_ppceabi` (the group of the unit it was cut from; unmeasured until
 *    bodies exist).
 * NAMES. file name GUESS (MSL `abort_exit`); the second function may belong to a separate file.
 * EVIDENCE. `.sbss` 0x80794E18 (abort-in-progress flag, written by the first function), 0x80794E1C
 *    (`__stdio_exit`, stored by `__stdio_atexit`) and 0x80794E20 (constraint handler, read by the second
 *    function) are consecutive and follow the console unit's flag.
 * RESIDUALS. COARSE: the constraint handler may be a separate TU.
 * SHAPES. none yet: no body is written, so nothing is measured; the map rows of the range are the work list
 *    (`ledger.py unit MSL_C/abort_exit.c`).
 */
