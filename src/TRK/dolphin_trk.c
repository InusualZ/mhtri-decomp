/*
 * TRK/dolphin_trk.c - the Dolphin/Revolution TRK start-up: `InitMetroTRK`, `InitMetroTRK_BBA`, the comm table set-
 *    up, the EXI UART wrappers and the program-end trap.
 *
 * RANGE. .text 0x80468D4C..0x8046940C (20 functions in the map, 0x6C0 B); .data 0x8060F5A8..0x8060F6D0; .bss
 *    0x806F5540..0x806F5568; .sbss 0x80794E48..0x80794E58; .sdata2 0x8079CF40..0x8079CF48.
 * FLAGS. the `OS` lib's `cflags_os` (the group of the unit it was cut from; unmeasured until bodies exist).
 * NAMES. file name GUESS (MetroTRK `dolphin_trk`).
 * EVIDENCE. `.data` 0x8060F5A8 (vector table, read by one function), the string base 0x8060F5E8 (read by
 *    `InitMetroTRKCommTable` and `TRK_board_display`), `.sbss` 0x80794E48 / 0x80794E50 and `.bss` 0x806F5540
 *    (`gDBCommTable`, read by the whole run); `.sdata2` 0x8079CF40 is read only by the program-end trap, closing
 *    the pool.
 * RESIDUALS. COARSE: the run mixes the start-up, the UART layer and the board display.
 * SHAPES. none yet: no body is written, so nothing is measured; the map rows of the range are the work list
 *    (`ledger.py unit TRK/dolphin_trk.c`).
 */
