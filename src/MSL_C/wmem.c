/*
 * MSL_C/wmem.c - the wide-character memory routines (`wmemcpy` and two siblings).
 *
 * RANGE. .text 0x80461778..0x80461808 (3 functions in the map, 0x90 B).
 * FLAGS. the `Runtime.PPCEABI.H` lib's `cflags_ppceabi` (the group of the unit it was cut from; unmeasured until
 *    bodies exist).
 * NAMES. file name GUESS (MSL `wmem`).
 * EVIDENCE. family only: `wmemcpy` is called by the wide-string writer of the wide printf unit; no data.
 * RESIDUALS. COARSE: three small functions, boundary to the neighbours by names.
 * SHAPES. none yet: no body is written, so nothing is measured; the map rows of the range are the work list
 *    (`ledger.py unit MSL_C/wmem.c`).
 */
