/*
 * TRK/support.c - the MetroTRK host support requests: `TRK_RequestSend` and the four support-request builders
 *    `TRKTargetSupportRequest` calls.
 *
 * RANGE. .text 0x804698D0..0x80469F10 (6 functions in the map, 0x640 B); .data 0x8060F710..0x8060F758.
 * FLAGS. the `OS` lib's `cflags_os` (the group of the unit it was cut from; unmeasured until bodies exist).
 * NAMES. file name GUESS (MetroTRK `support`).
 * EVIDENCE. `.data` 0x8060F710 (`bad reply size`) is read only by `TRK_RequestSend`; the four builders are called
 *    only from `TRKTargetSupportRequest`.
 * RESIDUALS. COARSE: `TRK_RequestSend` may belong to another file.
 * SHAPES. none yet: no body is written, so nothing is measured; the map rows of the range are the work list
 *    (`ledger.py unit TRK/support.c`).
 */
