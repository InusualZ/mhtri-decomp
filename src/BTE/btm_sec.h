/* BTE/btm_sec.h - the security manager entry points `BTE/btm_sec.c` owns that other units call and the completion
 *   callback type they take (docs/plan.md 6.5 rule 2). */
#ifndef MHTRI_BTE_BTM_SEC_H
#define MHTRI_BTE_BTM_SEC_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The security check completion callback `btm_sec_mx_access_request` calls with the caller's reference. */
/* untyped: a caller-owned payload - the reference the caller passed in */
typedef void (tBTM_SEC_CALLBACK)(u8* bd_addr, void* p_ref_data, u8 result);

/* 0x80488DE0 - checks the security of a multiplexed channel; calls `p_callback` when the check completes. */
/* untyped: a caller-owned payload - the reference handed back to the callback */
u8 btm_sec_mx_access_request(u8* bd_addr, u16 psm, u8 is_originator, u32 mx_proto_id, u32 mx_chan_id,
                             tBTM_SEC_CALLBACK* p_callback, void* p_ref_data);

/* 0x80489204 - abandons the pending security check of the device at `bd_addr`. */
void btm_sec_abort_access_req(u8* bd_addr);

/* .sdata2 0x8079D0D0 - the "any device" address FF:FF:FF:FF:FF:FF. */
extern const u8 BT_BD_ANY[6];

/* 0x8048897C - checks the security of an L2CAP channel on `psm`; calls `p_callback` when the check completes. */
u8 btm_sec_l2cap_access_req(u8* bd_addr, u16 psm, u16 handle, u8 is_originator, tBTM_SEC_CALLBACK* p_callback);

/* 0x80489B1C / 0x80489FDC - whether the link `handle` is bonding; drops it with `reason` (returns the BTM status). */
u8 btm_sec_is_bonding(u16 handle);
u8 btm_sec_disconnect(u16 handle, u8 reason);

/* 0x8048907C */
void btm_sec_conn_req(u8* bda, u8* dc);
/* 0x804892B8 */
void btm_sec_rmt_name_request_complete(u8* p_bd_addr, u8* p_bd_name, u8 status);
/* 0x804895A0 */
void btm_sec_auth_complete(u16 handle, u8 status);
/* 0x804898E4 */
void btm_sec_mkey_comp_event(u16 handle, u8 status, u8 key_flg);
/* 0x804899E0 */
void btm_sec_encrypt_change(u16 handle, u8 status, u8 encr_enable);
/* 0x80489B5C */
void btm_sec_connected(u8* bda, u16 handle, u8 status, u8 enc_mode);
/* 0x8048A098 */
void btm_sec_disconnected(u16 handle, u8 reason);
/* 0x8048A1D4 */
void btm_sec_link_key_notification(u8* p_bda, u8* p_link_key, u8 key_type);
/* 0x8048A380 */
void btm_sec_link_key_request(u8* p_bda);
/* 0x8048A524 */
void btm_sec_pin_code_request(u8* p_bda);
/* 0x8048A880 */
void btm_sec_update_clock_offset(u16 handle, u16 clock_offset);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_BTE_BTM_SEC_H */
