/*
 * TRK/msghndlr.c - the MetroTRK command handlers: connect, disconnect, reset, override, read/write memory and
 *    registers, continue, step, stop, set option.
 *
 * RANGE. .text 0x8046AADC..0x8046BA60 (13 functions in the map, 0xF84 B); .data 0x8060F7A8..0x8060F810; .sbss
 *    0x80794E70..0x80794E78.
 * FLAGS. the `OS` lib's `cflags_os` (the group of the unit it was cut from; unmeasured until bodies exist).
 * NAMES. file name GUESS (MetroTRK `msghndlr`).
 * EVIDENCE. jump tables `.data` 0x8060F7A8 / 0x8060F7C4, the option string 0x8060F7E0 and `.sbss` 0x80794E70 /
 *    0x80794E74 are read only by this `TRKDo*` run.
 * RESIDUALS. no bodies written.
 * SHAPES. none yet: no body is written, so nothing is measured; the map rows of the range are the work list
 *    (`ledger.py unit TRK/msghndlr.c`).
 */
