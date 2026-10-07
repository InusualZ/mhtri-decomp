/*
 * MSL_C/strtold.c - the string-to-floating-point routine (`__strtold`: digit collection, the `INFINITY` / `NAN(`
 *    forms, the scale and round steps).
 *
 * RANGE. .text 0x8045F9E8..0x80460D0C (1 functions in the map, 0x1324 B); .rodata 0x80572B78..0x80572B88; .sdata2
 *    0x8079CA08..0x8079CA28.
 * FLAGS. the `Runtime.PPCEABI.H` lib's `cflags_ppceabi` (the group of the unit it was cut from; unmeasured until
 *    bodies exist).
 * NAMES. file name GUESS (MSL `strtold`).
 * EVIDENCE. `.sdata2` 0x8079CA08..0x8079CA28 pools `NAN(`, `)`, `0.0`, `DBL_MIN`, `DBL_MAX` and the string
 *    `INFINITY` is `.rodata` 0x80572B78; the value `0.0` repeats at 0x8079C9F8 and 0x8079CA28, so it is a TU of
 *    its own.
 * RESIDUALS. no body written.
 * SHAPES. none yet: no body is written, so nothing is measured; the map rows of the range are the work list
 *    (`ledger.py unit MSL_C/strtold.c`).
 */
