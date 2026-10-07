/*
 * TRK/nubinit.c - the MetroTRK nub initialisation: `TRKInitializeNub`, `TRKTerminateNub`, `TRKNubWelcome`,
 *    `TRKInitializeEndian`.
 *
 * RANGE. .text 0x80469638..0x80469788 (4 functions in the map, 0x150 B); .data 0x8060F6F0..0x8060F710; .sbss
 *    0x80794E58..0x80794E68.
 * FLAGS. the `OS` lib's `cflags_os` (the group of the unit it was cut from; unmeasured until bodies exist).
 * NAMES. file name GUESS (MetroTRK `nubinit`).
 * EVIDENCE. `.data` 0x8060F6F0 (`MetroTRK for Revolution v0.4`) is read only by `TRKNubWelcome`;
 *    `TRKInitializeEndian` writes `.sbss` 0x80794E58 and `TRKInitializeNub` touches 0x80794E60.
 * RESIDUALS. no bodies written.
 * SHAPES. none yet: no body is written, so nothing is measured; the map rows of the range are the work list
 *    (`ledger.py unit TRK/nubinit.c`).
 */
