/* BTE/port_rfc.h - the port layer's indications and confirmations `BTE/port_rfc.c` owns (docs/plan.md 6.5 rule 2). */
#ifndef MHTRI_BTE_PORT_RFC_H
#define MHTRI_BTE_PORT_RFC_H

#include "BTE/rfc_int.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8049900C - 0x8049A408 - the multiplexer and DLC events the RFCOMM state machines report to the ports. */
void PORT_StartCnf(tRFC_MCB* p_mcb, u16 result);
void PORT_StartInd(tRFC_MCB* p_mcb);
void PORT_ParNegInd(tRFC_MCB* p_mcb, u8 dlci, u16 mtu, u8 cl, u8 k);
void PORT_ParNegCnf(tRFC_MCB* p_mcb, u8 dlci, u16 mtu, u8 cl, u8 k);
void PORT_DlcEstablishInd(tRFC_MCB* p_mcb, u8 dlci, u16 mtu);
void PORT_DlcEstablishCnf(tRFC_MCB* p_mcb, u8 dlci, u16 mtu, u16 result);
void PORT_PortNegInd(tRFC_MCB* p_mcb, u8 dlci, tPORT_STATE* p_pars, u16 param_mask);
void PORT_PortNegCnf(tRFC_MCB* p_mcb, u8 dlci, tPORT_STATE* p_pars, u16 result);
void PORT_ControlInd(tRFC_MCB* p_mcb, u8 dlci, tPORT_CTRL* p_pars);
void PORT_ControlCnf(tRFC_MCB* p_mcb, u8 dlci, tPORT_CTRL* p_pars);
void PORT_LineStatusInd(tRFC_MCB* p_mcb, u8 dlci, u8 line_status);
void PORT_DlcReleaseInd(tRFC_MCB* p_mcb, u8 dlci);
void PORT_CloseInd(tRFC_MCB* p_mcb);
void Port_TimeOutCloseMux(tRFC_MCB* p_mcb);
void PORT_DataInd(tRFC_MCB* p_mcb, u8 dlci, BT_HDR* p_buf);
void PORT_FlowInd(tRFC_MCB* p_mcb, u8 dlci, u8 enable_data);
u32 port_rfc_send_tx_data(tPORT* p_port);
void port_rfc_closed(tPORT* p_port, u8 res);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_BTE_PORT_RFC_H */
