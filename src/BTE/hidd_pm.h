/* BTE/hidd_pm.h - the HID device power management entry points `BTE/hidd_pm.c` owns (docs/plan.md 6.5 rule 2). */
#ifndef MHTRI_BTE_HIDD_PM_H
#define MHTRI_BTE_HIDD_PM_H

#include "BTE/hid_int.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8048FD58 - requests the power mode `p_req` for the link to the host; returns whether the request stands. */
u8 hidd_pm_set_now(BtmPmPwrMode* p_req);

/* 0x8048FF30 - takes the link's new power mode and moves on towards the final one. */
void hidd_pm_proc_mode_change(u8 hci_status, u8 mode, u16 interval);

/* 0x80490088 - switches the link to the idle power mode once the inactivity timer runs out. */
void hidd_pm_inact_timeout(TIMER_LIST_ENT* p_tle);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_BTE_HIDD_PM_H */
