/*
 * MSL_C/file_io.c - the stdio file layer: `abs`/`labs`, the buffer prepare/flush helpers, `__fwrite`, `fclose`,
 *    `fflush`, `_ftell`, `_fseek`.
 *
 * RANGE. .text 0x8045AB38..0x8045B3A0 (10 functions in the map, 0x868 B).
 * FLAGS. the `Runtime.PPCEABI.H` lib's `cflags_ppceabi` (the group of the unit it was cut from; unmeasured until
 *    bodies exist).
 * NAMES. COARSE: probably several MSL files (arith, buffer_io, direct_io, file_io, FILE_POS), merged because
 *    nothing in the data proves the seams; file name GUESS after the main entries.
 * EVIDENCE. contiguous run with no data of its own; `__fwrite` calls `__prep_buffer`, `__flush_buffer` and
 *    `_fseek`, `fflush` calls `__flush_all` / `__flush_buffer`, `fn_8045B1D8` is `_ftell`'s thunk (call-graph
 *    closure); the next function reads the locale character table (a different TU).
 * RESIDUALS. boundaries inside the run are unproven (no data, no pool, no string).
 * SHAPES. none yet: no body is written, so nothing is measured; the map rows of the range are the work list
 *    (`ledger.py unit MSL_C/file_io.c`).
 */
