/*
 * TRK/msgbuf.c - the MetroTRK message buffers: pool init, get/release/reset, position and the typed append/read
 *    helpers.
 *
 * RANGE. .text 0x8046A2D0..0x8046AADC (15 functions in the map, 0x80C B); .data 0x8060F780..0x8060F7A8; .bss
 *    0x806F5590..0x806F6F38.
 * FLAGS. the `OS` lib's `cflags_os` (the group of the unit it was cut from; unmeasured until bodies exist).
 * NAMES. file name GUESS (MetroTRK `msgbuf`).
 * EVIDENCE. `.bss` 0x806F5590 (0x19A8 B pool) and `.data` 0x8060F780 (`No buffer available`) are read only here.
 * RESIDUALS. no bodies written.
 * SHAPES. none yet: no body is written, so nothing is measured; the map rows of the range are the work list
 *    (`ledger.py unit TRK/msgbuf.c`).
 */
