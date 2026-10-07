/*
 * MSL_C/ansi_files.c - the standard stream table: `__files` (stdin/stdout/stderr with their three 0x100-byte
 *    buffers) and `__close_all` / `__flush_all`.
 *
 * RANGE. .text 0x804591A8..0x804592B8 (2 functions in the map, 0x110 B); .data 0x8060E958..0x8060EA98; .bss
 *    0x806F4D00..0x806F5000.
 * FLAGS. the `Runtime.PPCEABI.H` lib's `cflags_ppceabi` (the group of the unit it was cut from; unmeasured until
 *    bodies exist).
 * NAMES. file name GUESS (MSL `ansi_files`): the function pair and the table are the stdio bookkeeping.
 * EVIDENCE. `.data` 0x8060E958 (`__files`, 0x140 B) is read by both functions; the three buffers `.bss`
 *    0x806F4D00..0x806F5000 are pointed to by the stream records (their `+0x1C` / `+0x24` words), and the
 *    records' callbacks are the console read/write/close functions of `MSL_C/uart_console_io.c` and
 *    `TRK/mslsupp.c`.
 * RESIDUALS. no bodies written; the stream records carry callback pointers into two other units (data, not claimed
 *    there).
 * SHAPES. none yet: no body is written, so nothing is measured; the map rows of the range are the work list
 *    (`ledger.py unit MSL_C/ansi_files.c`).
 */
