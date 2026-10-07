/*
 * MSL_C/signal.c - `raise`: the signal handler table dispatch.
 *
 * RANGE. .text 0x8045F4AC..0x8045F554 (1 functions in the map, 0xA8 B); .bss 0x806F5000..0x806F5020.
 * FLAGS. the `Runtime.PPCEABI.H` lib's `cflags_ppceabi` (the group of the unit it was cut from; unmeasured until
 *    bodies exist).
 * NAMES. file name GUESS (MSL `signal`).
 * EVIDENCE. `.bss` 0x806F5000 (0x20 B, 7 handler slots) is read only by it, and it calls `exit` for an unhandled
 *    signal; the abort unit calls it.
 * RESIDUALS. no body written.
 * SHAPES. none yet: no body is written, so nothing is measured; the map rows of the range are the work list
 *    (`ledger.py unit MSL_C/signal.c`).
 */
