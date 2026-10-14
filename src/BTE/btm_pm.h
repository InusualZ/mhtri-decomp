/* BTE/btm_pm.h - the power mode entry points `BTE/btm_pm.c` owns that other units call (docs/plan.md 6.5 rule 2). */
#ifndef MHTRI_BTE_BTM_PM_H
#define MHTRI_BTE_BTM_PM_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x804866DC - writes the power mode of the link to `remote_bda` to `p_mode`; returns the BTM status. */
u8 BTM_ReadPowerMode(u8* remote_bda, u8* p_mode);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_BTE_BTM_PM_H */
