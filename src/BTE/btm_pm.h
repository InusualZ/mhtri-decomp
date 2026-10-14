/* BTE/btm_pm.h - the power mode entry points `BTE/btm_pm.c` owns that other units call (docs/plan.md 6.5 rule 2). */
#ifndef MHTRI_BTE_BTM_PM_H
#define MHTRI_BTE_BTM_PM_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x804866DC - writes the power mode of the link to `remote_bda` to `p_mode`; returns the BTM status. */
u8 BTM_ReadPowerMode(u8* remote_bda, u8* p_mode);

/* 0x80486E0C */
void btm_pm_proc_cmd_status(u8 status);
/* 0x80486EBC */
void btm_pm_proc_mode_change(u8 hci_status, u16 hci_handle, u8 mode, u16 interval);

/* 0x80486794 / 0x8048680C - reset the power manager, and set up the power state of the ACL entry `ind`. */
void btm_pm_reset(void);
void btm_pm_sm_alloc(u8 ind);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_BTE_BTM_PM_H */
