/* BTE/bta_hh_act.h - the BTA HID host actions `BTE/bta_hh_act.c` owns (docs/plan.md 6.5 rule 2). */
#ifndef MHTRI_BTE_BTA_HH_ACT_H
#define MHTRI_BTE_BTA_HH_ACT_H

#include "BTE/bta_hh_int.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8047E678 / 0x8047E7EC / 0x8047E8F0 - enable the HID host, disable it (closing live connections first), and finish
 * the disable. */
void bta_hh_api_enable(tBTA_HH_DATA* p_data);
void bta_hh_api_disable(void);
void bta_hh_disc_cmpl(void);

/* 0x8047EAC8 / 0x8047EC5C - start the SDP query of a device, and act on its result. */
void bta_hh_start_sdp(tBTA_HH_DEV_CB* p_cb, tBTA_HH_DATA* p_data);
void bta_hh_sdp_cmpl(tBTA_HH_DEV_CB* p_cb, tBTA_HH_DATA* p_data);

/* 0x8047EDC4 .. 0x8047F960 - the state-machine actions. */
void bta_hh_api_disc_act(tBTA_HH_DEV_CB* p_cb, tBTA_HH_DATA* p_data);
void bta_hh_open_cmpl_act(tBTA_HH_DEV_CB* p_cb, tBTA_HH_DATA* p_data);
void bta_hh_open_act(tBTA_HH_DEV_CB* p_cb, tBTA_HH_DATA* p_data);
void bta_hh_data_act(tBTA_HH_DEV_CB* p_cb, tBTA_HH_DATA* p_data);
void bta_hh_handsk_act(tBTA_HH_DEV_CB* p_cb, tBTA_HH_DATA* p_data);
void bta_hh_ctrl_dat_act(tBTA_HH_DEV_CB* p_cb, tBTA_HH_DATA* p_data);
void bta_hh_close_act(tBTA_HH_DEV_CB* p_cb, tBTA_HH_DATA* p_data);
void bta_hh_get_dscp_act(tBTA_HH_DEV_CB* p_cb, tBTA_HH_DATA* p_data);
void bta_hh_maint_dev_act(tBTA_HH_DEV_CB* p_cb, tBTA_HH_DATA* p_data);
void bta_hh_write_dev_act(tBTA_HH_DEV_CB* p_cb, tBTA_HH_DATA* p_data);

/* 0x8047F844 - reports the ACL queue state of every connected device. */
void bta_hh_get_acl_q_info(void);

/* 0x8047FB28 - the HID layer's device callback: forwards an event as a message to the BTA task. */
void bta_hh_cback(u8 dev_handle, u8 event, u32 data, BT_HDR* pdata);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_BTE_BTA_HH_ACT_H */
