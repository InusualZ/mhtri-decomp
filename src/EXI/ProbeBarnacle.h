/*
 * Declarations for the symbols `src/EXI/ProbeBarnacle.c` owns that other units use (docs/plan.md 6.5, rule 2).
 */
#ifndef MHTRI_EXI_PROBEBARNACLE_H
#define MHTRI_EXI_PROBEBARNACLE_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x804B5AE0 - points GX vertex attribute `attr` at an array of `stride`-byte entries. */
void GXSetArray(u32 attr, const void* pBase, u8 stride); /* untyped: byte range */

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EXI_PROBEBARNACLE_H */
