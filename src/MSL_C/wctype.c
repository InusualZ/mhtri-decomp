/*
 * MSL_C/wctype.c - the wide-character class and case tables (data only).
 *
 * RANGE. .rodata 0x80572B88..0x80573188.
 * FLAGS. the `Runtime.PPCEABI.H` lib's `cflags_ppceabi` (the group of the unit it was cut from; unmeasured until
 *    bodies exist).
 * NAMES. file name GUESS (MSL `wctype`).
 * EVIDENCE. `.rodata` 0x80572B88..0x80573188 is three 0x200 B tables (class flags and two maps) read by nothing in
 *    the band; they sit between the string-to-float unit's string (0x80572B78) and the wide printf unit's 8-byte
 *    string (0x80573188).
 * RESIDUALS. no bodies; no `.text`.
 * SHAPES. none yet: no body is written, so nothing is measured; the map rows of the range are the work list
 *    (`ledger.py unit MSL_C/wctype.c`).
 */
