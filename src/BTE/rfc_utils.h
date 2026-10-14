/* BTE/rfc_utils.h - the RFCOMM utilities `BTE/rfc_utils.c` owns (docs/plan.md 6.5 rule 2). */
#ifndef MHTRI_BTE_RFC_UTILS_H
#define MHTRI_BTE_RFC_UTILS_H

#include "BTE/rfc_int.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8049EC6C / 0x8049ECA4 - the frame check sequence of `len` bytes, and whether `received_fcs` matches it. */
u8 rfc_calc_fcs(u16 len, u8* p);
u8 rfc_check_fcs(u16 len, u8* p, u8 received_fcs);

/* 0x8049ECF0 / 0x8049EE48 - allocate the multiplexer of the device at `bd_addr` (or reuse its open one), or free it. */
tRFC_MCB* rfc_alloc_multiplexer_channel(u8* bd_addr, u8 is_initiator);
void rfc_release_multiplexer_channel(tRFC_MCB* p_mcb);

/* 0x8049EED0 - 0x8049F01C - start (for `timeout` seconds) and stop the multiplexer and port timers. */
void rfc_timer_start(tRFC_MCB* p_mcb, u16 timeout);
void rfc_timer_stop(tRFC_MCB* p_mcb);
void rfc_port_timer_start(tPORT* p_port, u16 timeout);
void rfc_port_timer_stop(tPORT* p_port);

/* 0x8049F074 - closes the multiplexer once no DLC uses it. */
void rfc_check_mcb_active(tRFC_MCB* p_mcb);

/* 0x8049F138 - dispatches an expired RFCOMM timer to its state machine. */
void rfcomm_process_timeout(TIMER_LIST_ENT* p_tle);

/* 0x8049F174 - the security check callback of a port (`p_ref_data` is the port). */
/* untyped: a caller-owned payload - the reference handed to btm_sec_mx_access_request */
void rfc_sec_check_complete(u8* bd_addr, void* p_ref_data, u8 res);

/* 0x8049F1C8 - releases a port whose DLC is closed and reports it to the port layer. */
void rfc_port_closed(tPORT* p_port);

/* 0x8049F320 / 0x8049F3AC - add received credits to a port, or spend one sending. */
void rfc_inc_credit(tPORT* p_port, u8 credit);
void rfc_dec_credit(tPORT* p_port);

/* 0x8049F3E8 - queues `p_buf` (when not NULL) and sends the queued commands while L2CAP is not congested. */
void rfc_check_send_cmd(tRFC_MCB* p_mcb, BT_HDR* p_buf);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_BTE_RFC_UTILS_H */
