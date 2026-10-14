/* BTE/hidd_conn.h - the HID device connection entry points `BTE/hidd_conn.c` owns (docs/plan.md 6.5 rule 2). */
#ifndef MHTRI_BTE_HIDD_CONN_H
#define MHTRI_BTE_HIDD_CONN_H

#include "BTE/hid_int.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8048FBD0 - opens the control channel to the host; returns a HID result. */
u8 hidd_conn_initiate(void);

/* 0x8048FC90 - retries the connection to the host, or gives up after the retry limit. */
void hidd_proc_repage_timeout(TIMER_LIST_ENT* p_tle);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_BTE_HIDD_CONN_H */
