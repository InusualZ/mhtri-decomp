/*
 * TRK/TRKAccessFile.c - the MetroTRK host-file stubs (`TRKAccessFile` and its three 8-byte siblings, each `li
 *    r3,0; blr`).
 *
 * RANGE. .text 0x80468358..0x80468378 (4 functions in the map, 0x20 B).
 * FLAGS. the `OS` lib's `cflags_os` (the group of the unit it was cut from; unmeasured until bodies exist).
 * NAMES. file name is the dump's `TRKAccessFile`.
 * EVIDENCE. the dump names `TRKAccessFile`; the console and file callbacks of `TRK/mslsupp.c` call it.
 * RESIDUALS. COARSE: the three siblings have no names.
 * SHAPES. none yet: no body is written, so nothing is measured; the map rows of the range are the work list
 *    (`ledger.py unit TRK/TRKAccessFile.c`).
 */
