/* BTE/bta_dm_main.h - the device manager control blocks and state machine drivers `BTE/bta_dm_main.c` owns
 *   (docs/plan.md 6.5 rule 2). */
#ifndef MHTRI_BTE_BTA_DM_MAIN_H
#define MHTRI_BTE_BTA_DM_MAIN_H

#include "BTE/bta_dm_int.h"

#ifdef __cplusplus
extern "C" {
#endif

/* .bss 0x807392B8 / 0x80739334 - the device search and device manager control blocks. */
extern tBTA_DM_SEARCH_CB bta_dm_search_cb;
extern tBTA_DM_CB bta_dm_cb;

/* 0x8047DAFC / 0x8047DB94 - run the actions the state tables give for the event of `p_msg`; return TRUE. */
u8 bta_dm_sm_execute(BT_HDR* p_msg);
u8 bta_dm_search_sm_execute(BT_HDR* p_msg);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_BTE_BTA_DM_MAIN_H */
