/*
 * TRK/nubevent.c - the MetroTRK notification and event queue: `TRKDoNotifyStopped`, the event queue (init, get,
 *    post, construct, destruct).
 *
 * RANGE. .text 0x8046940C..0x80469638 (6 functions in the map, 0x22C B); .data 0x8060F6D0..0x8060F6F0; .bss
 *    0x806F5568..0x806F5590.
 * FLAGS. the `OS` lib's `cflags_os` (the group of the unit it was cut from; unmeasured until bodies exist).
 * NAMES. file name GUESS (MetroTRK `nubevent`); `TRKDoNotifyStopped` may be a separate `notify` file.
 * EVIDENCE. `.bss` 0x806F5568 (the 0x28 B queue) is read only by the three queue functions and `.data` 0x8060F6D0
 *    (`Event Queue full`) only by `TRKPostEvent`.
 * RESIDUALS. COARSE.
 * SHAPES. none yet: no body is written, so nothing is measured; the map rows of the range are the work list
 *    (`ledger.py unit TRK/nubevent.c`).
 */
