/* BTE/l2c_utils.h - the L2CAP utility entry points `BTE/l2c_utils.c` owns: control block allocation, release and lookup,
 *   the peer signalling builders, configuration processing and connection creation (docs/plan.md 6.5 rule 2). */
#ifndef MHTRI_BTE_L2C_UTILS_H
#define MHTRI_BTE_L2C_UTILS_H

#include "BTE/l2c_int.h"

/* 0x804970F8 / 0x804971CC - take a free link control block for `p_bd_addr`, or release one with its channels. */
tL2C_LCB* l2cu_allocate_lcb(u8* p_bd_addr);
void l2cu_release_lcb(tL2C_LCB* p_lcb);

/* 0x804972CC / 0x80497354 - the in-use link to `p_bd_addr`, or with the HCI handle `handle`. */
tL2C_LCB* l2cu_find_lcb_by_bd_addr(u8* p_bd_addr);
tL2C_LCB* l2cu_find_lcb_by_handle(u16 handle);

/* 0x804973E4 - the role a new link asks for. */
u8 l2cu_get_conn_role(tL2C_LCB* p_this_lcb);

/* 0x804973F4 - a signalling buffer with the HCI, L2CAP and command headers of a `len`-byte command. */
BT_HDR* l2cu_build_header(u16 handle, u16 len, u8 cmd, u8 id);

/* 0x804974B8 - 0x80498454 - build and queue one signalling command to the peer. */
void l2cu_send_peer_cmd_reject(tL2C_LCB* p_lcb, u16 reason, u8 rem_id, u16 p1, u16 p2);
void l2cu_send_peer_connect_req(tL2C_CCB* p_ccb);
void l2cu_send_peer_connect_rsp(tL2C_CCB* p_ccb, u16 result, u16 status);
void l2cu_reject_connection(tL2C_LCB* p_lcb, u16 remote_cid, u8 rem_id, u16 result);
void l2cu_send_peer_config_req(tL2C_CCB* p_ccb, tL2CAP_CFG_INFO* p_cfg);
void l2cu_send_peer_config_rsp(tL2C_CCB* p_ccb, tL2CAP_CFG_INFO* p_cfg);
void l2cu_send_peer_config_rej(tL2C_CCB* p_ccb, u8* p_data, u16 data_len, u16 rej_len);
void l2cu_send_peer_disc_req(tL2C_CCB* p_ccb);
void l2cu_send_peer_disc_rsp(tL2C_LCB* p_lcb, u8 remote_id, u16 local_cid, u16 remote_cid);
void l2cu_send_peer_echo_req(tL2C_LCB* p_lcb, u8* p_data, u16 data_len);
void l2cu_send_peer_echo_rsp(tL2C_LCB* p_lcb, u8 id, u8* p_data, u16 data_len);
void l2cu_send_peer_info_rsp(tL2C_LCB* p_lcb, u8 remote_id, u16 info_type);

/* 0x804984F8 / 0x80498604 / 0x804987A8 - take a channel on `p_lcb`, release one, or find one by its local CID. */
tL2C_CCB* l2cu_allocate_ccb(tL2C_LCB* p_lcb);
void l2cu_release_ccb(tL2C_CCB* p_ccb);
tL2C_CCB* l2cu_find_ccb_by_cid(tL2C_LCB* p_lcb, u16 local_cid);

/* 0x80498804 / 0x80498850 / 0x80498860 - register a PSM, free its record, or find it. */
tL2C_RCB* l2cu_allocate_rcb(u16 psm);
void l2cu_release_rcb(tL2C_RCB* p_rcb);
tL2C_RCB* l2cu_find_rcb_by_psm(u16 psm);

/* 0x80498918 - 0x80498BB0 - apply a received or sent configuration request or response to the channel. */
u8 l2cu_process_peer_cfg_req(tL2C_CCB* p_ccb, tL2CAP_CFG_INFO* p_cfg);
void l2cu_process_peer_cfg_rsp(tL2C_CCB* p_ccb, tL2CAP_CFG_INFO* p_cfg);
void l2cu_process_our_cfg_req(tL2C_CCB* p_ccb, tL2CAP_CFG_INFO* p_cfg);
void l2cu_process_our_cfg_rsp(tL2C_CCB* p_ccb, tL2CAP_CFG_INFO* p_cfg);

/* 0x80498BF0 - drops every link after a controller reset. */
void l2cu_device_reset(void);

/* 0x80498C5C / 0x80498D74 - page the peer of `p_lcb`, switching the role of another link first when needed. */
u8 l2cu_create_conn(tL2C_LCB* p_lcb);
u8 l2cu_create_conn_after_switch(tL2C_LCB* p_lcb);

/* 0x80498E44 / 0x80498ED4 - the first link in `state`; whether a link is being torn down. */
tL2C_LCB* l2cu_find_lcb_by_state(tL2C_LINK_STATE state);
u8 l2cu_lcb_disconnecting(void);

#endif /* MHTRI_BTE_L2C_UTILS_H */
