/* BTE/rfc_ts_frames.h - the frame builders and parsers `BTE/rfc_ts_frames.c` owns (docs/plan.md 6.5 rule 2). */
#ifndef MHTRI_BTE_RFC_TS_FRAMES_H
#define MHTRI_BTE_RFC_TS_FRAMES_H

#include "BTE/rfc_int.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8049D624 - 0x8049E0D4 - build and send one frame or multiplexer command. */
void rfc_send_sabme(tRFC_MCB* p_mcb, u8 dlci);
void rfc_send_ua(tRFC_MCB* p_mcb, u8 dlci);
void rfc_send_dm(tRFC_MCB* p_mcb, u8 dlci, u8 pf);
void rfc_send_disc(tRFC_MCB* p_mcb, u8 dlci);
void rfc_send_buf_uih(tRFC_MCB* p_mcb, u8 dlci, BT_HDR* p_buf);
void rfc_send_pn(tRFC_MCB* p_mcb, u8 dlci, u8 is_command, u16 mtu, u8 cl, u8 k);
void rfc_send_fcon(tRFC_MCB* p_mcb, u8 is_command);
void rfc_send_fcoff(tRFC_MCB* p_mcb, u8 is_command);
void rfc_send_msc(tRFC_MCB* p_mcb, u8 dlci, u8 is_command, tPORT_CTRL* p_pars);
void rfc_send_rls(tRFC_MCB* p_mcb, u8 dlci, u8 is_command, u8 status);
void rfc_send_rpn(tRFC_MCB* p_mcb, u8 dlci, u8 is_command, tPORT_STATE* p_pars, u16 mask);
void rfc_send_test(tRFC_MCB* p_mcb, u8 is_command, BT_HDR* p_buf);
void rfc_send_credit(tRFC_MCB* p_mcb, u8 dlci, u8 credit);

/* 0x8049E0D4 - decodes a received frame's header into `p_frame`; returns its RFC_EVENT_* (RFC_EVENT_BAD_FRAME when
 * it is malformed). */
u8 rfc_parse_data(tRFC_MCB* p_mcb, MX_FRAME* p_frame, BT_HDR* p_buf);

/* 0x8049E648 - decodes and dispatches a multiplexer command received on DLCI 0; consumes `p_buf`. */
void rfc_process_mx_message(tRFC_MCB* p_mcb, BT_HDR* p_buf);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_BTE_RFC_TS_FRAMES_H */
