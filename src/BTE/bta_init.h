/* BTE/bta_init.h - the stack start and stop entry points `BTE/bta_init.c` owns (docs/plan.md 6.5 rule 2). */
#ifndef MHTRI_BTE_BTA_INIT_H
#define MHTRI_BTE_BTA_INIT_H

#include "types.h"

/* The completion callback of `BTA_CleanUp`. */
typedef void (*BtaCleanUpCallback)(s32 result);

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8047A630 - initialises GKI, the stack and the HCI transport, starts the BTU task's periodic alarm and waits until
 * the controller is up; returns 0. */
u8 BTA_Init(void);

/* 0x8047A710 - stores `callback` and closes the transport; the callback reports when the stack is down. */
void BTA_CleanUp(BtaCleanUpCallback callback);

/* 0x8047A750 - reports the close of the USB device (`status` negative on failure) to the BTA_CleanUp callback. */
void bta_usb_close_evt(s8 status);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_BTE_BTA_INIT_H */
