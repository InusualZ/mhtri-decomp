/*
 * Declarations for the symbols `src/ef/system_core.cpp` owns that other units call (docs/plan.md 6.5, rule 2).
 */
#ifndef MHTRI_EF_SYSTEM_CORE_H
#define MHTRI_EF_SYSTEM_CORE_H

#include "types.h"

/* 0x800CF7A8 - a block from the work heap (C++ scope: the map row is `work_mem_alloc__FUl`). */
/* untyped: byte range - the work heap hands out raw blocks the caller types */
void* work_mem_alloc(u32 size);

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800D28FC - stores `mode + 1` as `system_w`'s display-state byte (+0xA58); the network transfer-mode switch
 * calls it with the transfer mode.  GUESS name. */
void setTransferDisplayState(u8 mode);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_SYSTEM_CORE_H */
