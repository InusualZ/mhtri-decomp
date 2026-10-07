/*
 * MSL_C/strtoul.c - the string-to-integer routines: `__strtoul`, `__strtoull`, `strtol`, `atoi`.
 *
 * RANGE. .text 0x80460D0C..0x80461778 (4 functions in the map, 0xA6C B).
 * FLAGS. the `Runtime.PPCEABI.H` lib's `cflags_ppceabi` (the group of the unit it was cut from; unmeasured until
 *    bodies exist).
 * NAMES. file name GUESS (MSL `strtoul`); `atoi` may be a separate file.
 * EVIDENCE. both `__strtoul` variants take the string reader of the scanner unit by pointer and read the locale
 *    class table; `strtol` / `atoi` call `__strtoul`; `errno` is the only data they write.
 * RESIDUALS. COARSE: no data separates `atoi` from `strtol`.
 * SHAPES. none yet: no body is written, so nothing is measured; the map rows of the range are the work list
 *    (`ledger.py unit MSL_C/strtoul.c`).
 */
