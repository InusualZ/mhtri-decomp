/*
 * MSL_C/mbstring.c - multibyte / wide-character conversion: the narrow-to-wide and wide-to-narrow converters
 *    including `wcstombs`.
 *
 * RANGE. .text 0x8045B3A0..0x8045B598 (5 functions in the map, 0x1F8 B).
 * FLAGS. the `Runtime.PPCEABI.H` lib's `cflags_ppceabi` (the group of the unit it was cut from; unmeasured until
 *    bodies exist).
 * NAMES. file name GUESS (MSL `mbstring`).
 * EVIDENCE. function family only (the names `__wctomb_noconv`, `wcstombs`); the functions read the
 *    `_current_locale` record `.data` 0x8060EDB0 (locale unit) and call `strlen` / `strncpy`.
 * RESIDUALS. COARSE: the boundary to the neighbours is name-family evidence only.
 * SHAPES. none yet: no body is written, so nothing is measured; the map rows of the range are the work list
 *    (`ledger.py unit MSL_C/mbstring.c`).
 */
