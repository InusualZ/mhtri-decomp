/* BTE/hidh_conn.h - the HID host connection layer `BTE/hidh_conn.c` owns: its L2CAP registration and callbacks, the
 *   security check completions, data send and connection origination (docs/plan.md 6.5 rule 2). */
#ifndef MHTRI_BTE_HIDH_CONN_H
#define MHTRI_BTE_HIDH_CONN_H

#include "BTE/hid_int.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80490E54 / 0x80492E68 - register the two HID PSMs with L2CAP (returns a HID result), or deregister them. */
u8 hidh_conn_reg(void);
void hidh_conn_dereg(void);

/* 0x80490FAC - closes both channels of the device `dhandle`; returns a HID result. */
u8 hidh_conn_disconnect(u8 dhandle);

/* 0x80491050 / 0x80491524 - the security check completions of an incoming and an outgoing control channel. */
/* untyped: a caller-owned payload - the device the security check was started for */
void hidh_sec_check_complete_term(u8* bd_addr, void* p_ref_data, u8 res);
/* untyped: a caller-owned payload - the device the security check was started for */
void hidh_sec_check_complete_orig(u8* bd_addr, void* p_ref_data, u8 res);

/* 0x804913D8 - retries the connection of the device whose handle the timer carries. */
void hidh_proc_repage_timeout(TIMER_LIST_ENT* p_tle);

/* 0x80491180 - 0x80492738 - the L2CAP callbacks of the two HID PSMs. */
void hidh_l2cif_connect_ind(u8* bd_addr, u16 l2cap_cid, u16 psm, u8 l2cap_id);
void hidh_l2cif_connect_cfm(u16 l2cap_cid, u16 result);
void hidh_l2cif_config_ind(u16 l2cap_cid, tL2CAP_CFG_INFO* p_cfg);
void hidh_l2cif_config_cfm(u16 l2cap_cid, tL2CAP_CFG_INFO* p_cfg);
void hidh_l2cif_disconnect_ind(u16 l2cap_cid, u8 ack_needed);
void hidh_l2cif_disconnect_cfm(u16 l2cap_cid, u16 result);
void hidh_l2cif_cong_ind(u16 l2cap_cid, u8 congested);
void hidh_l2cif_data_ind(u16 l2cap_cid, BT_HDR* p_msg);

/* 0x80492AD0 - sends a HID transaction to the device `dhandle`, segmenting `buf` to the channel MTU; returns a HID
 * result. */
u8 hidh_conn_snd_data(u8 dhandle, u8 trans_type, u8 param, u16 data, u8 report_id, BT_HDR* buf);

/* 0x80492D6C - opens the control channel to the device `dhandle`; returns a HID result. */
u8 hidh_conn_initiate(u8 dhandle);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_BTE_HIDH_CONN_H */
