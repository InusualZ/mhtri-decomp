/* BTE/rfc_l2cap_if.h - the L2CAP interface `BTE/rfc_l2cap_if.c` owns (docs/plan.md 6.5 rule 2). */
#ifndef MHTRI_BTE_RFC_L2CAP_IF_H
#define MHTRI_BTE_RFC_L2CAP_IF_H

#include "BTE/rfc_int.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8049A9E0 - registers RFCOMM's L2CAP callbacks for its PSM. */
void rfcomm_l2cap_if_init(void);

/* 0x8049AA5C - 0x8049B2F8 - the L2CAP callbacks of the RFCOMM PSM. */
void RFCOMM_ConnectInd(u8* bd_addr, u16 lcid, u16 psm, u8 id);
void RFCOMM_ConnectCnf(u16 lcid, u16 result);
void RFCOMM_ConfigInd(u16 lcid, tL2CAP_CFG_INFO* p_cfg);
void RFCOMM_ConfigCnf(u16 lcid, tL2CAP_CFG_INFO* p_cfg);
void RFCOMM_QoSViolationInd(u8* bd_addr);
void RFCOMM_DisconnectInd(u16 lcid, u8 is_conf_needed);
void RFCOMM_BufDataInd(u16 lcid, BT_HDR* p_buf);
void RFCOMM_CongestionStatusInd(u16 lcid, u8 is_congested);

/* 0x8049B2F8 - stores the multiplexer of the L2CAP channel `lcid`. */
void rfc_save_lcid_mcb(tRFC_MCB* p_mcb, u16 lcid);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_BTE_RFC_L2CAP_IF_H */
