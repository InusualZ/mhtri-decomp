/* BTE/btm_devctl.h - the device control entry points `BTE/btm_devctl.c` owns that other units call (docs/plan.md 6.5
 *   rule 2). */
#ifndef MHTRI_BTE_BTM_DEVCTL_H
#define MHTRI_BTE_BTM_DEVCTL_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80483134 - whether the controller finished its reset (the device state is ready). */
u8 BTM_IsDeviceUp(void);

/* 0x80483FC4 - the controller's local feature bytes. */
u8* BTM_ReadLocalFeatures(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_BTE_BTM_DEVCTL_H */
