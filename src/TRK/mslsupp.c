/*
 * TRK/mslsupp.c - the MSL console file hooks over TRK: `__read_file`, `__write_file`, `__TRK_write_console`, the
 *    console read and the file-access helper.
 *
 * RANGE. .text 0x8046BA60..0x8046BBEC (5 functions in the map, 0x18C B).
 * FLAGS. the `OS` lib's `cflags_os` (the group of the unit it was cut from; unmeasured until bodies exist).
 * NAMES. file name GUESS (MetroTRK `mslsupp`).
 * EVIDENCE. dump names; every function calls `GetUseSerialIO` / `TRKAccessFile`; the stream table of
 *    `ansi_files.c` stores the first one as the console read callback.
 * RESIDUALS. COARSE.
 * SHAPES. none yet: no body is written, so nothing is measured; the map rows of the range are the work list
 *    (`ledger.py unit TRK/mslsupp.c`).
 */
