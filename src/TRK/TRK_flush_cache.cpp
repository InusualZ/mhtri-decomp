/*
 * TRK/TRK_flush_cache.cpp - the MetroTRK data/instruction cache flush loop (`TRK_flush_cache`).
 *
 * RANGE. .text 0x80468868..0x804688A0 (1 functions in the map, 0x38 B).
 * FLAGS. the `OS` lib's `cflags_os` (the group of the unit it was cut from; unmeasured until bodies exist).
 * NAMES. file name kept from the registration.
 * EVIDENCE. the dump names the single function; it is called by `TRKTargetAccessMemory`, `TRKPPCAccessSpecialReg`,
 *    `TRKTargetSupportRequest`.
 * RESIDUALS. no body written.
 * SHAPES. none yet: no body is written, so nothing is measured; the map rows of the range are the work list
 *    (`ledger.py unit TRK/TRK_flush_cache.cpp`).
 */
