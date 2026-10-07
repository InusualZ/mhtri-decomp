/*
 * MSL_C/uart_console_io.c - the console stream callbacks: the console writer (initialises the UART once, writes
 *    through TRK's console write) and the console close stub.
 *
 * RANGE. .text 0x80463CC0..0x80463D98 (2 functions in the map, 0xD8 B); .sbss 0x80794E10..0x80794E18.
 * FLAGS. the `Runtime.PPCEABI.H` lib's `cflags_ppceabi` (the group of the unit it was cut from; unmeasured until
 *    bodies exist).
 * NAMES. file name GUESS (MSL `uart_console_io`).
 * EVIDENCE. `__files` stores both functions as the write and close callbacks of the three standard streams; the
 *    writer calls `InitializeUART`, `WriteUARTN` and `OSGetConsoleType`; its once-flag is `.sbss` 0x80794E10.
 * RESIDUALS. no bodies written.
 * SHAPES. none yet: no body is written, so nothing is measured; the map rows of the range are the work list
 *    (`ledger.py unit MSL_C/uart_console_io.c`).
 */
