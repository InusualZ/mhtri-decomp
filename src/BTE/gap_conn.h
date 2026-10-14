/* BTE/gap_conn.h - the GAP connection layer `BTE/gap_conn.c` owns: its start-up and the L2CAP callbacks it registers
 *   (docs/plan.md 6.5 rule 2). */
#ifndef MHTRI_BTE_GAP_CONN_H
#define MHTRI_BTE_GAP_CONN_H

#include "BTE/gap_int.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8048C2F4 - fills the GAP connection layer's L2CAP callback set. */
void gap_conn_init(void);

/* 0x8048C364 - 0x8048CCFC - the L2CAP callbacks of the GAP connection layer. */
void gap_connect_ind(u8* bd_addr, u16 l2cap_cid, u16 psm, u8 l2cap_id);
void gap_connect_cfm(u16 l2cap_cid, u16 result);
void gap_config_ind(u16 l2cap_cid, tL2CAP_CFG_INFO* p_cfg);
void gap_config_cfm(u16 l2cap_cid, tL2CAP_CFG_INFO* p_cfg);
void gap_disconnect_ind(u16 l2cap_cid, u8 ack_needed);
void gap_data_ind(u16 l2cap_cid, BT_HDR* p_msg);
void gap_congestion_ind(u16 lcid, u8 is_congested);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_BTE_GAP_CONN_H */
