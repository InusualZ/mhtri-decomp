/*
 * TRK/TRK_flush_cache.cpp - the MetroTRK data/instruction cache flush loop (`TRK_flush_cache`).
 *
 * RANGE. .text 0x80468868..0x804688A0 (1 functions in the map, 0x38 B).
 * FLAGS. the `OS` lib's `cflags_os`; the body is `asm`.
 * NAMES. file name kept from the registration.
 * EVIDENCE. the dump names the single function; it is called by `TRKTargetAccessMemory`, `TRKPPCAccessSpecialReg`,
 *    `TRKTargetSupportRequest`.
 * RESIDUALS. none measured yet.
 * SHAPES. an 8-byte stride of dcbst/dcbf/sync/icbi over [addr, addr+len), `isync` at the end.
 */
#include "TRK/TRK_flush_cache.h"

/* untyped: cache-line range start */
asm void TRK_flush_cache(void* addr, u32 len)
{
    nofralloc
    lis r5, 0xFFFF
    ori r5, r5, 0xFFF1
    and r5, r5, r3
    subf r3, r5, r3
    add r4, r4, r3
flush_loop:
    dcbst 0, r5
    dcbf 0, r5
    sync
    icbi 0, r5
    addic r5, r5, 8
    addic. r4, r4, -8
    bge flush_loop
    isync
    blr
}
