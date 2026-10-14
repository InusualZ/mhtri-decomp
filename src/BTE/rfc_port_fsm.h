/* BTE/rfc_port_fsm.h - the port state machine and the multiplexer command processing `BTE/rfc_port_fsm.c` owns
 *   (docs/plan.md 6.5 rule 2). */
#ifndef MHTRI_BTE_RFC_PORT_FSM_H
#define MHTRI_BTE_RFC_PORT_FSM_H

#include "BTE/rfc_int.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8049BF58 - runs the port state machine for `event`; `p_data` is the event's payload. */
/* untyped: a caller-owned payload - its type depends on the event */
void rfc_port_sm_execute(tPORT* p_port, u16 event, void* p_data);

/* 0x8049C90C - 0x8049D054 - the received multiplexer commands and responses. */
void rfc_process_pn(tRFC_MCB* p_mcb, u8 is_command, MX_FRAME* p_frame);
void rfc_process_rpn(tRFC_MCB* p_mcb, u8 is_command, u8 is_request, MX_FRAME* p_frame);
void rfc_process_msc(tRFC_MCB* p_mcb, u8 is_command, MX_FRAME* p_frame);
void rfc_process_rls(tRFC_MCB* p_mcb, u8 is_command, MX_FRAME* p_frame);
void rfc_process_nsc(tRFC_MCB* p_mcb, MX_FRAME* p_frame);
void rfc_process_test_rsp(tRFC_MCB* p_mcb, BT_HDR* p_buf);
void rfc_process_fcon(tRFC_MCB* p_mcb, u8 is_command);
void rfc_process_fcoff(tRFC_MCB* p_mcb, u8 is_command);
void rfc_process_l2cap_congestion(tRFC_MCB* p_mcb, u8 is_congested);

/* 0x8049D054 - copies the line parameters a port negotiation frame carries into `p_port_pars`. */
void rfc_set_port_state(tPORT_STATE* p_port_pars, MX_FRAME* p_frame);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_BTE_RFC_PORT_FSM_H */
