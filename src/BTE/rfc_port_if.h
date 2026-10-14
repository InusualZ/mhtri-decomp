/* BTE/rfc_port_if.h - the RFCOMM requests and responses `BTE/rfc_port_if.c` owns (docs/plan.md 6.5 rule 2). */
#ifndef MHTRI_BTE_RFC_PORT_IF_H
#define MHTRI_BTE_RFC_PORT_IF_H

#include "BTE/rfc_int.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8049D0F8 - 0x8049D624 - the requests and responses the port layer sends into the state machines. */
void RFCOMM_StartRsp(tRFC_MCB* p_mcb, u16 result);
void RFCOMM_DlcEstablishReq(tRFC_MCB* p_mcb, u8 dlci, u16 mtu);
void RFCOMM_DlcEstablishRsp(tRFC_MCB* p_mcb, u8 dlci, u16 mtu, u16 result);
void RFCOMM_ParNegReq(tRFC_MCB* p_mcb, u8 dlci, u16 mtu);
void RFCOMM_ParNegRsp(tRFC_MCB* p_mcb, u8 dlci, u16 mtu, u8 cl, u8 k);
void RFCOMM_PortNegReq(tRFC_MCB* p_mcb, u8 dlci, tPORT_STATE* p_pars);
void RFCOMM_PortNegRsp(tRFC_MCB* p_mcb, u8 dlci, tPORT_STATE* p_pars, u16 param_mask);
void RFCOMM_ControlReq(tRFC_MCB* p_mcb, u8 dlci, tPORT_CTRL* p_pars);
void RFCOMM_FlowReq(tRFC_MCB* p_mcb, u8 dlci, u8 enable);
void RFCOMM_LineStatusReq(tRFC_MCB* p_mcb, u8 dlci, u8 status);
void RFCOMM_DlcReleaseReq(tRFC_MCB* p_mcb, u8 dlci);
void RFCOMM_DataReq(tRFC_MCB* p_mcb, u8 dlci, BT_HDR* p_buf);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_BTE_RFC_PORT_IF_H */
