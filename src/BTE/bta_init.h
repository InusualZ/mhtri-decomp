/* BTE/bta_init.h - the stack start and stop entry points `BTE/bta_init.c` owns (docs/plan.md 6.5 rule 2). */
#ifndef MHTRI_BTE_BTA_INIT_H
#define MHTRI_BTE_BTA_INIT_H

#include "types.h"

/* The completion callback of `BTA_CleanUp`. */
typedef void (*BtaCleanUpCallback)(s32 result);

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8047A630 - initialises GKI, the stack and the HCI transport and starts the BTU task's periodic alarm. */
void BTA_Init(void);

/* 0x8047A710 - stores `callback` and closes the transport; the callback reports when the stack is down. */
void BTA_CleanUp(BtaCleanUpCallback callback);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_BTE_BTA_INIT_H */
