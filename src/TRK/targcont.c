/*
 * TRK/targcont.c - `TRKTargetContinue` and the extended-register block save/restore (`TRKSaveExtended1Block`,
 *    `TRKRestoreExtended1Block`).
 *
 * RANGE. .text 0x80469F10..0x8046A26C (3 functions in the map, 0x35C B).
 * FLAGS. the `OS` lib's `cflags_os` (the group of the unit it was cut from; unmeasured until bodies exist).
 * NAMES. file name GUESS; the two groups may be separate files.
 * EVIDENCE. adjacent dump names; the two block functions read the global CPU state; no data of their own.
 * RESIDUALS. COARSE.
 * SHAPES. none yet: no body is written, so nothing is measured; the map rows of the range are the work list
 *    (`ledger.py unit TRK/targcont.c`).
 */
