/*
 * TRK/main_TRK.c - the MetroTRK entry: `TRK_main` and `TRKNubMainLoop`.
 *
 * RANGE. .text 0x804688A0..0x804689C8 (2 functions in the map, 0x128 B); .sbss 0x80794E40..0x80794E48.
 * FLAGS. the `OS` lib's `cflags_os` (the group of the unit it was cut from; unmeasured until bodies exist).
 * NAMES. file name GUESS (MetroTRK `main_TRK`).
 * EVIDENCE. `.sbss` 0x80794E40 (`TRK_mainError`) is read only by `TRK_main`; the loop reads `gTRKInputPendingPtr`.
 * RESIDUALS. COARSE: dump names only.
 * SHAPES. none yet: no body is written, so nothing is measured; the map rows of the range are the work list
 *    (`ledger.py unit TRK/main_TRK.c`).
 */
