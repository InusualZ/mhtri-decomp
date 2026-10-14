/* BTE/l2c_link.h - the L2CAP link layer entry points `BTE/l2c_link.c` owns: the HCI connection events, link timeouts,
 *   the ACL send path and buffer allocation, packet-start/end checks and role changes (docs/plan.md 6.5 rule 2). */
#ifndef MHTRI_BTE_L2C_LINK_H
#define MHTRI_BTE_L2C_LINK_H

#include "BTE/l2c_int.h"

/* 0x80494F3C / 0x80495090 - an incoming ACL connection request, and the completion of a connection. */
void l2c_link_hci_conn_req(u8* bd_addr);
u8 l2c_link_hci_conn_comp(u8 status, u16 handle, u8* p_bda);

/* 0x804952C8 - the security result for the link to `p_bda`; `p_ref_data` is the reference the caller registered. */
/* untyped: an opaque handle passed through - the security manager's reference */
void l2c_link_sec_comp(u8* p_bda, void* p_ref_data, u8 status);

/* 0x8049537C / 0x80495438 - a link went down, or broke its QoS. */
u8 l2c_link_hci_disc_comp(u16 handle, u8 reason);
u8 l2c_link_hci_qos_violation(u16 handle);

/* 0x804954A8 - the link timer of `p_lcb` expired. */
void l2c_link_timeout(tL2C_LCB* p_lcb);

/* 0x80495724 - queues `p_buf` on the link and sends what the controller window allows. */
void l2c_link_check_send_pkts(tL2C_LCB* p_lcb, tL2C_CCB* p_ccb, BT_HDR* p_buf);

/* 0x804959F0 - shares the controller's ACL buffers between the active links. */
void l2c_link_adjust_allocation(void);

/* 0x80495B88 / 0x80495C6C - the controller's completed-packets event, and its buffer count. */
void l2c_link_process_num_completed_pkts(u8* p);
void l2c_link_processs_num_bufs(u16 num_lm_acl_bufs);

/* 0x80495C80 / 0x80495E50 - reassemble a received ACL packet from its segments. */
BT_HDR* l2cap_link_chk_pkt_start(BT_HDR* p_buf);
u8 l2cap_link_chk_pkt_end(void);

/* 0x80495EBC / 0x80495F3C - a role switch finished or failed. */
void l2c_link_role_changed(u8* bd_addr, u8 new_role);
void l2c_link_role_change_failed(void);

/* 0x80495FA8 - the transport sent the segments of `p_msg` only partly. */
void l2c_link_segments_xmitted(BT_HDR* p_msg);

/* 0x80496084 - the controller asked for the PIN of the link to `bd_addr`. */
void l2c_pin_code_request(u8* bd_addr);

#endif /* MHTRI_BTE_L2C_LINK_H */
