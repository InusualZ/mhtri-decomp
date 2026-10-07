/*
 * MSL_C/extras.c - the MSL `extras` string routines: `stricmp`, `strupr` and a third case-insensitive helper.
 *
 * RANGE. .text 0x804681CC..0x80468358 (3 functions in the map, 0x18C B).
 * FLAGS. the `OS` lib's `cflags_os` (the group of the unit it was cut from; unmeasured until bodies exist).
 * NAMES. file name GUESS (MSL `extras`).
 * EVIDENCE. family only; all three read the locale class table `.data` 0x8060EDB0; no data of their own; the TRK
 *    access stubs follow.
 * RESIDUALS. COARSE.
 * SHAPES. none yet: no body is written, so nothing is measured; the map rows of the range are the work list
 *    (`ledger.py unit MSL_C/extras.c`).
 */
