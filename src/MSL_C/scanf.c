/*
 * MSL_C/scanf.c - the formatted input engine: the format-string scanner with its jump tables, the string reader
 *    `fn_8045F2F4` and `sscanf`.
 *
 * RANGE. .text 0x8045DFC8..0x8045F4AC (4 functions in the map, 0x14E4 B); .rodata 0x80572B50..0x80572B78; .data
 *    0x8060F028..0x8060F298.
 * FLAGS. the `Runtime.PPCEABI.H` lib's `cflags_ppceabi` (the group of the unit it was cut from; unmeasured until
 *    bodies exist).
 * NAMES. file name GUESS (MSL `scanf`).
 * EVIDENCE. jump tables `.data` 0x8060F028 / 0x8060F108 / 0x8060F128 / 0x8060F148 and the table `.rodata`
 *    0x80572B50 are read only here; the string reader is also called by `strtol` / `atoi` through a function
 *    pointer.
 * RESIDUALS. no bodies written.
 * SHAPES. none yet: no body is written, so nothing is measured; the map rows of the range are the work list
 *    (`ledger.py unit MSL_C/scanf.c`).
 */
