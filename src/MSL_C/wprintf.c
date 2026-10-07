/*
 * MSL_C/wprintf.c - the wide-character formatted output engine: the wide format scanner, `__wpformatter`, the wide
 *    string writer, `vswprintf` and its wrappers.
 *
 * RANGE. .text 0x80461808..0x80463B20 (10 functions in the map, 0x2318 B); .rodata 0x80573188..0x80573190; .data
 *    0x8060F298..0x8060F538; .sdata2 0x8079CA28..0x8079CA30.
 * FLAGS. the `Runtime.PPCEABI.H` lib's `cflags_ppceabi` (the group of the unit it was cut from; unmeasured until
 *    bodies exist).
 * NAMES. file name GUESS (MSL `wprintf`).
 * EVIDENCE. jump tables `.data` 0x8060F298 and 0x8060F378, the record `.data` 0x8060F4C8 (read by three
 *    functions), `.rodata` 0x80573188 and the pooled `0.0` at `.sdata2` 0x8079CA28 are read only here; `0.0`
 *    repeats at 0x8079CA10, so this is a TU of its own.
 * RESIDUALS. no bodies written.
 * SHAPES. none yet: no body is written, so nothing is measured; the map rows of the range are the work list
 *    (`ledger.py unit MSL_C/wprintf.c`).
 */
