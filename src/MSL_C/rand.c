/*
 * MSL_C/rand.c - `rand` and `srand` over the 8-byte seed.
 *
 * RANGE. .text 0x8045DFA0..0x8045DFC8 (2 functions in the map, 0x28 B); .sdata 0x80793CE0..0x80793CE8.
 * FLAGS. the `Runtime.PPCEABI.H` lib's `cflags_ppceabi` (the group of the unit it was cut from; unmeasured until
 *    bodies exist).
 * NAMES. file name GUESS (MSL `rand`).
 * EVIDENCE. `.sdata` 0x80793CE0 (8 B) is read only by these two functions and sits between the printf unit's
 *    `@wstringBase0` (0x80793CD8) and the string unit's `strtok` statics (0x80793CE8).
 * RESIDUALS. no bodies written.
 * SHAPES. none yet: no body is written, so nothing is measured; the map rows of the range are the work list
 *    (`ledger.py unit MSL_C/rand.c`).
 */
