/*
 * MSL_C/ansi_fp.c - decimal/binary floating-point conversion for the formatted I/O: `__ull2dec`, `__timesdec`,
 *    `__str2dec`, `__two_exp`, `__num2dec_internal`, the `__num2dec` / `__dec2num` entries.
 *
 * RANGE. .text 0x804592B8..0x8045AB38 (10 functions in the map, 0x1880 B); .rodata 0x80572540..0x80572620; .data
 *    0x8060EA98..0x8060EC00; .sdata2 0x8079C9A8..0x8079C9E0.
 * FLAGS. the `Runtime.PPCEABI.H` lib's `cflags_ppceabi` (the group of the unit it was cut from; unmeasured until
 *    bodies exist).
 * NAMES. file name GUESS (MSL `ansi_fp`).
 * EVIDENCE. `.sdata2` 0x8079C9A8..0x8079C9E0 holds `0.0 / 1.0 / -1.0 / 5.0 / DBL_MAX / 2^52 magic`, and the value
 *    `0.0` is pooled again at 0x8079C9F8 (read by the printf unit) so the next pool is another TU; the jump table
 *    `.data` 0x8060EA98, the power table `.rodata` 0x80572540 and the double table 0x8060EBC0 are read only here
 *    (and by `__dec2num`).
 * RESIDUALS. no bodies written; `__num2dec` and `__dec2num` have no map names.
 * SHAPES. none yet: no body is written, so nothing is measured; the map rows of the range are the work list
 *    (`ledger.py unit MSL_C/ansi_fp.c`).
 */
