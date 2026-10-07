/*
 * TRK/dispatch.c - the MetroTRK command dispatcher (`TRKDispatchMessage`, one switch).
 *
 * RANGE. .text 0x80468C2C..0x80468D4C (1 functions in the map, 0x120 B); .data 0x8060F538..0x8060F5A8.
 * FLAGS. the `OS` lib's `cflags_os` (the group of the unit it was cut from; unmeasured until bodies exist).
 * NAMES. file name GUESS (MetroTRK `dispatch`).
 * EVIDENCE. `.data` 0x8060F538 (the 0x6C B jump table) is read only by it.
 * RESIDUALS. no body written.
 * SHAPES. none yet: no body is written, so nothing is measured; the map rows of the range are the work list
 *    (`ledger.py unit TRK/dispatch.c`).
 */
