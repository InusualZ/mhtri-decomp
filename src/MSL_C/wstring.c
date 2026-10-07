/*
 * MSL_C/wstring.c - the wide-string routines: `wcslen`, `wcsncpy`, `wcscmp` and their siblings.
 *
 * RANGE. .text 0x80463B20..0x80463C48 (6 functions in the map, 0x128 B).
 * FLAGS. the `Runtime.PPCEABI.H` lib's `cflags_ppceabi` (the group of the unit it was cut from; unmeasured until
 *    bodies exist).
 * NAMES. file name GUESS (MSL `wstring`).
 * EVIDENCE. name family only (`wcs*`); no data.
 * RESIDUALS. COARSE: boundary by names and call-graph (the wide printf unit calls `wcslen`).
 * SHAPES. none yet: no body is written, so nothing is measured; the map rows of the range are the work list
 *    (`ledger.py unit MSL_C/wstring.c`).
 */
