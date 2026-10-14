/* BTE/btm_sec.h - the security manager entry points `BTE/btm_sec.c` owns that other units call and the completion
 *   callback type they take (docs/plan.md 6.5 rule 2). */
#ifndef MHTRI_BTE_BTM_SEC_H
#define MHTRI_BTE_BTM_SEC_H

#include "types.h"

/* The service records and the remote name and link key notification callbacks the control block holds. */
#define BTM_SEC_MAX_SERVICE_RECORDS 24
#define BTM_SEC_MAX_CALLBACKS 2

/* size: 0x3C - a security service record: the protocol and channels it covers, the PSM, the security it needs and
 * the originating and terminating service names. */
typedef struct tBTM_SEC_SERV_REC {
    /* +0x00 */ u32 mx_proto_id;
    /* +0x04 */ u32 orig_mx_chan_id;
    /* +0x08 */ u32 term_mx_chan_id;
    /* +0x0C */ u16 psm;
    /* +0x0E */ u8 security_flags;
    /* +0x0F */ u8 service_id;
    /* +0x10 */ char orig_service_name[0x16];
    /* +0x26 */ char term_service_name[0x16];
} tBTM_SEC_SERV_REC; /* size: 0x3C */

/* The application's security callbacks (BTE `tBTM_APPL_INFO`). */
typedef u8 (tBTM_AUTHORIZE_CALLBACK)(u8* bd_addr, u8* dev_class, u8* bd_name, char* service_name, u8 service_id,
                                     u8 is_originator);
typedef u8 (tBTM_PIN_CALLBACK)(u8* bd_addr, u8* dev_class, u8* bd_name);
typedef u8 (tBTM_LINK_KEY_CALLBACK)(u8* bd_addr, u8* dev_class, u8* bd_name, u8* key, u8 key_type);
typedef u8 (tBTM_LINK_KEY_REQ_CALLBACK)(u8* bd_addr, u8* key);
typedef u8 (tBTM_AUTH_COMPLETE_CALLBACK)(u8* bd_addr, u8* dev_class, u8* bd_name, int result);
typedef void (tBTM_ABORT_CALLBACK)(u8* bd_addr, u8* dev_class, u8* bd_name);
typedef void (tBTM_RMT_NAME_CALLBACK)(u8* bd_addr, u8* dev_class, u8* bd_name);
typedef void (tBTM_MKEY_CALLBACK)(u8* bd_addr, u8 status, u8 key_flag);

/* size: 0x18 - the application's security callbacks. */
typedef struct tBTM_APPL_INFO {
    /* +0x00 */ tBTM_AUTHORIZE_CALLBACK* p_authorize_callback;
    /* +0x04 */ tBTM_PIN_CALLBACK* p_pin_callback;
    /* +0x08 */ tBTM_LINK_KEY_CALLBACK* p_link_key_callback;
    /* +0x0C */ tBTM_LINK_KEY_REQ_CALLBACK* p_link_key_req_callback;
    /* +0x10 */ tBTM_AUTH_COMPLETE_CALLBACK* p_auth_complete_callback;
    /* +0x14 */ tBTM_ABORT_CALLBACK* p_abort_callback;
} tBTM_APPL_INFO; /* size: 0x18 */

#ifdef __cplusplus
extern "C" {
#endif

/* The security check completion callback `btm_sec_mx_access_request` calls with the caller's reference. */
/* untyped: a caller-owned payload - the reference the caller passed in */
typedef void (tBTM_SEC_CALLBACK)(u8* bd_addr, void* p_ref_data, u8 result);

/* 0x80488DE0 - checks the security of a multiplexed channel; calls `p_callback` when the check completes. */
/* untyped: a caller-owned payload - the reference handed back to the callback */
void btm_sec_mx_access_request(u8* bd_addr, u16 psm, u8 is_originator, u32 mx_proto_id, u32 mx_chan_id,
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

/* 0x804891A8 - sets the security mode and clears the pending PIN request. */
void btm_sec_init(u8 sec_mode);

/* 0x804892B8 - the remote name of `p_bd_addr` (NULL: the pending request failed) arrived with `status`. */
void btm_sec_rmt_name_request_complete(u8* p_bd_addr, u8* p_bd_name, u8 status);

/* 0x80487F1C / 0x80487F94 / 0x80487FE4 - register the application's security callbacks; add or remove a remote name
 * notification callback. */
u8 BTM_SecRegister(tBTM_APPL_INFO* p_cb_info);
u8 BTM_SecAddRmtNameNotifyCallback(tBTM_RMT_NAME_CALLBACK* p_callback);
u8 BTM_SecDeleteRmtNameNotifyCallback(tBTM_RMT_NAME_CALLBACK* p_callback);

/* Dead-stripped in retail: set the security mode; encrypt the link to `bd_addr`. */
void BTM_SetSecurityMode(u8 sec_mode);
/* untyped: a caller-owned payload - the reference handed back to the callback */
u8 BTM_SetEncryption(u8* bd_addr, tBTM_SEC_CALLBACK* p_callback, void* p_ref_data);

/* 0x80488038 / 0x80488310 / 0x804883D4 / 0x804885B0 - the PIN type, the PIN reply, the authorization answer and
 * bonding. */
void BTM_SetPinType(u8 pin_type, u8* pin_code, u8 pin_code_len);
void BTM_PINCodeReply(u8* bd_addr, u8 res, u8 pin_len, u8* p_pin, u32* trusted_mask);
void BTM_DeviceAuthorized(u8* bd_addr, u8 res, u32* trusted_mask);
u8 BTM_SecBond(u8* bd_addr, u8 pin_len, u8* p_pin, u32* trusted_mask);

/* 0x8048907C / 0x804891C4 - an incoming connection request; re-enable link security after a controller reset. */
void btm_sec_conn_req(u8* bda, u8* dc);
void btm_sec_dev_reset(void);

/* 0x804895A0 / 0x804898E4 / 0x804899E0 - the authentication, master key and encryption change events. */
void btm_sec_auth_complete(u16 handle, u8 status);
void btm_sec_mkey_comp_event(u16 handle, u8 status, u8 key_flg);
void btm_sec_encrypt_change(u16 handle, u8 status, u8 encr_enable);

/* 0x80489B5C / 0x8048A098 - an ACL link connected or disconnected. */
void btm_sec_connected(u8* bda, u16 handle, u8 status, u8 enc_mode);
void btm_sec_disconnected(u16 handle, u8 reason);

/* 0x8048A1D4 / 0x8048A380 / 0x8048A524 - the link key notification, link key request and PIN request events. */
void btm_sec_link_key_notification(u8* p_bda, u8* p_link_key, u8 key_type);
void btm_sec_link_key_request(u8* p_bda);
void btm_sec_pin_code_request(u8* p_bda);

/* 0x8048A880 / 0x8048AEAC - record a link's clock offset; the trusted-service bitmap of `bd_addr`. */
void btm_sec_update_clock_offset(u16 handle, u16 clock_offset);
u32* btm_read_trusted_mask(u8* bd_addr);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_BTE_BTM_SEC_H */
