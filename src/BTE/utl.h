/* BTE/utl.h - the utility entry points `BTE/utl.c` owns that other units call (docs/plan.md 6.5 rule 2). */
#ifndef MHTRI_BTE_UTL_H
#define MHTRI_BTE_UTL_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8047B310 - frees the GKI buffer `*p` points at and clears the pointer. */
/* untyped: a caller-owned payload - the address of any GKI buffer pointer */
void utl_freebuf(void** p);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_BTE_UTL_H */
