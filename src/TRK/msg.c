/*
 * TRK/msg.c - `TRK_MessageSend` (writes a framed message to the UART).
 *
 * RANGE. .text 0x8046A26C..0x8046A2D0 (1 functions in the map, 0x64 B); .data 0x8060F758..0x8060F780; .sbss
 *    0x80794E68..0x80794E70.
 * FLAGS. the `OS` lib's `cflags_os` (the group of the unit it was cut from; unmeasured until bodies exist).
 * NAMES. file name GUESS (MetroTRK `msg`).
 * EVIDENCE. `.data` 0x8060F758 (`TRK_WriteUARTN returned %ld`) and `.sbss` 0x80794E68 are read only by it.
 * RESIDUALS. no body written.
 * SHAPES. none yet: no body is written, so nothing is measured; the map rows of the range are the work list
 *    (`ledger.py unit TRK/msg.c`).
 */
