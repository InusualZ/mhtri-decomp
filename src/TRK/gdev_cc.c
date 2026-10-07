/*
 * TRK/gdev_cc.c - the MetroTRK debugger channel over the EXI/DB interface: `gdev_cc_initialize`, `shutdown`,
 *    `open`, `close`, `read`, `write`, `pre_continue`, `post_stop`, `peek`, `initinterrupts`.
 *
 * RANGE. .text 0x80468378..0x804685F0 (10 functions in the map, 0x278 B); .bss 0x806F5020..0x806F5540; .sbss
 *    0x80794E38..0x80794E40.
 * FLAGS. the `OS` lib's `cflags_os` (the group of the unit it was cut from; unmeasured until bodies exist).
 * NAMES. file name is the dump's prefix.
 * EVIDENCE. ten consecutive `gdev_cc_*` dump names; `.bss` 0x806F5020 (0x500 B) and 0x806F5520 (0x20 B, the
 *    receive circle buffer) and `.sbss` 0x80794E38 are read only here.
 * RESIDUALS. no bodies written.
 * SHAPES. none yet: no body is written, so nothing is measured; the map rows of the range are the work list
 *    (`ledger.py unit TRK/gdev_cc.c`).
 */
