/* BTE/utl.h - the BTA utility functions `BTE/utl.c` owns (docs/plan.md 6.5 rule 2). */
#ifndef MHTRI_BTE_UTL_H
#define MHTRI_BTE_UTL_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8047B310 - frees the GKI buffer `*p` holds, if any, and clears the pointer. */
/* untyped: a caller-owned payload - the slot holds any GKI buffer */
void utl_freebuf(void** p);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_BTE_UTL_H */
