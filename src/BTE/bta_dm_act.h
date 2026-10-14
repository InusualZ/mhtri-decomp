/* BTE/bta_dm_act.h - the device manager actions and the service id tables `BTE/bta_dm_act.c` owns (docs/plan.md 6.5
 *   rule 2). */
#ifndef MHTRI_BTE_BTA_DM_ACT_H
#define MHTRI_BTE_BTA_DM_ACT_H

#include "BTE/bta_dm_int.h"

#ifdef __cplusplus
extern "C" {
#endif

/* .rodata 0x80573618 - the BTM security service id of each BTA service id. */
extern const u32 bta_service_id_to_btm_srv_id_lkup_tbl[BTA_MAX_SERVICE_ID + 1];

/* The device manager actions bta_dm_action lists. */
void bta_dm_enable(tBTA_DM_MSG* p_data);
void bta_dm_disable(tBTA_DM_MSG* p_data);
void bta_dm_set_dev_name(tBTA_DM_MSG* p_data);
void bta_dm_set_visibility(tBTA_DM_MSG* p_data);
void bta_dm_bond(tBTA_DM_MSG* p_data);
void bta_dm_pin_reply(tBTA_DM_MSG* p_data);
void bta_dm_auth_reply(tBTA_DM_MSG* p_data);
void bta_dm_signal_strength(tBTA_DM_MSG* p_data);
void bta_dm_acl_change(tBTA_DM_MSG* p_data);
void bta_dm_keep_acl(tBTA_DM_MSG* p_data);
void bta_dm_send_hci_reset(tBTA_DM_MSG* p_data);

/* The device search actions bta_dm_search_action lists. */
void bta_dm_search_start(tBTA_DM_MSG* p_data);
void bta_dm_search_cancel(tBTA_DM_MSG* p_data);
void bta_dm_discover(tBTA_DM_MSG* p_data);
void bta_dm_inq_cmpl(tBTA_DM_MSG* p_data);
void bta_dm_rmt_name(tBTA_DM_MSG* p_data);
void bta_dm_sdp_result(tBTA_DM_MSG* p_data);
void bta_dm_search_cmpl(tBTA_DM_MSG* p_data);
void bta_dm_free_sdp_db(tBTA_DM_MSG* p_data);
void bta_dm_disc_result(tBTA_DM_MSG* p_data);
void bta_dm_search_result(tBTA_DM_MSG* p_data);
void bta_dm_queue_search(tBTA_DM_MSG* p_data);
void bta_dm_queue_disc(tBTA_DM_MSG* p_data);
void bta_dm_search_clear_queue(tBTA_DM_MSG* p_data);
void bta_dm_search_cancel_cmpl(tBTA_DM_MSG* p_data);
void bta_dm_search_cancel_notify(tBTA_DM_MSG* p_data);
void bta_dm_search_cancel_transac_cmpl(tBTA_DM_MSG* p_data);
void bta_dm_disc_rmt_name(tBTA_DM_MSG* p_data);
void bta_dm_cancel_rmt_name(tBTA_DM_MSG* p_data);

/* 0x8047D620 - reports the device manager disabled at once (BTA_SYS calls it when a reset is pending). */
void bta_dm_immediate_disable(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_BTE_BTA_DM_ACT_H */
