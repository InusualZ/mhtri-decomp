/* BTE/bta_dm_pm.h - the device manager power management entry points and the connected services table
 *   `BTE/bta_dm_pm.c` owns (docs/plan.md 6.5 rule 2). */
#ifndef MHTRI_BTE_BTA_DM_PM_H
#define MHTRI_BTE_BTA_DM_PM_H

#include "BTE/bta_dm_int.h"

#ifdef __cplusplus
extern "C" {
#endif

/* .bss 0x80739438 - the services with an open connection the power manager tracks. */
extern tBTA_DM_CONNECTED_SRVCS bta_dm_conn_srvcs;

/* 0x8047DC5C / 0x8047DCC4 - register the power manager with BTA_SYS and BTM, or deregister it from BTM. */
void bta_dm_init_pm(void);
void bta_dm_disable_pm(void);

/* 0x8047E54C / 0x8047E66C - the actions of BTA_DM_PM_BTM_STATUS_EVT and BTA_DM_PM_TIMER_EVT. */
void bta_dm_pm_btm_status(tBTA_DM_MSG* p_data);
void bta_dm_pm_timer(tBTA_DM_MSG* p_data);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_BTE_BTA_DM_PM_H */
