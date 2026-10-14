/* BTE/btm_main.h - the BTM control block `BTE/btm_main.c` owns and the BTM layer's trace statements
 *   (docs/plan.md 6.5 rule 2). */
#ifndef MHTRI_BTE_BTM_MAIN_H
#define MHTRI_BTE_BTM_MAIN_H

#include "types.h"
#include "BTE/gki_time.h"
#include "BTE/bd.h"
#include "BTE/btm_acl.h"
#include "BTE/btm_dev.h"
#include "BTE/btm_devctl.h"
#include "BTE/btm_discovery.h"
#include "BTE/btm_inq.h"
#include "BTE/btm_pm.h"
#include "BTE/btm_sco.h"
#include "BTE/btm_sec.h"

/* The longest PIN code. */
#define BTM_MAX_PIN_LEN 16

/* size: 0x27C4 - the BTM control block (btm_init clears 0x27C4 bytes; the .bss row's 0x27C8 is the 8-byte alignment
 * of the next object): the local configuration, the ACL table, the power manager, the device control state (the BTE
 * source's devcb, flattened), the discovery and inquiry state, the SCO links, the security callbacks, service and
 * device records and the trace level. */
typedef struct tBTM_CB {
    /* +0x0000 */ char bd_name[BTM_MAX_LOC_BD_NAME_LEN + 1];
    /* +0x0020 */ u8 pin_type;
    /* +0x0021 */ u8 pin_code_len;
    /* +0x0022 */ u8 pin_code[16];
    /* +0x0032 */ u8 unused_0x0032[2];
    /* +0x0034 */ tACL_CONN acl_db[BTM_MAX_ACL_LINKS];
    /* +0x04A4 */ u8 unused_0x04A4[0x20];
    /* +0x04C4 */ u16 btm_def_link_policy;
    /* +0x04C6 */ u16 btm_def_link_super_tout;
    /* +0x04C8 */ tBTM_ACL_DB_CHANGE_CB* p_acl_changed_cb;
    /* +0x04CC */ tBTM_PM_MCB pm_mode_db[BTM_MAX_ACL_LINKS];
    /* +0x0554 */ tBTM_PM_RCB pm_reg_db[BTM_MAX_PM_RECORDS];
    /* +0x0564 */ u8 pm_pend_link;
    /* +0x0565 */ u8 pm_pend_id;
    /* +0x0566 */ u8 pad_0x0566[2];
    /* +0x0568 */ BtmDevStatusCallback p_dev_status_cb;
    /* +0x056C */ BtmVsEventCallback p_vend_spec_cb;
    /* +0x0570 */ BtmCompleteCallback p_stored_link_key_cmpl_cb;
    /* +0x0574 */ TIMER_LIST_ENT reset_timer;
    /* +0x058C */ BtmCompleteCallback p_reset_cmpl_cb;
    /* +0x0590 */ TIMER_LIST_ENT rln_timer;
    /* +0x05A8 */ BtmCompleteCallback p_rln_cmpl_cb;
    /* +0x05AC */ TIMER_LIST_ENT rlinkp_timer;
    /* +0x05C4 */ BtmCompleteCallback p_rlinkp_cmpl_cb;
    /* +0x05C8 */ TIMER_LIST_ENT rssi_timer;
    /* +0x05E0 */ BtmCompleteCallback p_rssi_cmpl_cb;
    /* +0x05E4 */ TIMER_LIST_ENT lnk_quality_timer;
    /* +0x05FC */ BtmCompleteCallback p_lnk_qual_cmpl_cb;
    /* +0x0600 */ TIMER_LIST_ENT qossu_timer;
    /* +0x0618 */ BtmCompleteCallback p_qossu_cmpl_cb;
    /* +0x061C */ BtmCompleteCallback p_vsc_cmpl_cb;
    /* +0x0620 */ BtmCompleteCallback p_hci_reset_cmpl_cb;
    /* +0x0624 */ tBTM_ROLE_SWITCH_CMPL switch_role_ref_data;
    /* +0x062C */ BtmCompleteCallback p_switch_role_cb;
    /* +0x0630 */ u8 local_addr[6];
    /* +0x0636 */ BtmVersionInfo local_version;
    /* +0x0640 */ u8 local_features[8];
    /* +0x0648 */ u8 dev_class[3];
    /* +0x064B */ u8 pad_0x064B;
    /* +0x064C */ u16 page_timeout;
    /* +0x064E */ u8 devcb_state;
    /* +0x064F */ u8 retry_count;
    /* +0x0650 */ u8 vsc_busy;
    /* +0x0651 */ u8 pad_0x0651[3];
    /* +0x0654 */ u16 btm_acl_pkt_types_supported;
    /* +0x0656 */ u16 btm_sco_pkt_types_supported;
    /* +0x0658 */ tBTM_DISC_VAR disc_var;
    /* +0x1678 */ tSDP_DISCOVERY_DB* p_disc_db;
    /* +0x167C */ tBTM_INQUIRY_VAR_ST btm_inq_vars;
    /* +0x1850 */ tSCO_CB sco_cb;
    /* +0x190C */ tBTM_APPL_INFO api;
    /* +0x1924 */ tBTM_LINK_KEY_CALLBACK* p_link_key_notify_cb[BTM_SEC_MAX_CALLBACKS];
    /* +0x192C */ tBTM_RMT_NAME_CALLBACK* p_rmt_name_callback[BTM_SEC_MAX_CALLBACKS];
    /* +0x1934 */ tBTM_FILTER_CB* p_conn_filter_cb;
    /* +0x1938 */ tBTM_SEC_DEV_REC* p_collided_dev_rec;
    /* +0x193C */ TIMER_LIST_ENT pin_code_req_tle;
    /* +0x1954 */ u8 pin_code_req_bda[6];
    /* +0x195A */ u8 pad_0x195A[2];
    /* +0x195C */ TIMER_LIST_ENT sec_collision_tle;
    /* +0x1974 */ u32 dev_rec_count;
    /* +0x1978 */ u8 security_mode;
    /* +0x1979 */ u8 pairing_disabled;
    /* +0x197A */ u8 connect_only_paired;
    /* +0x197B */ u8 security_mode_changed;
    /* +0x197C */ u8 pin_type_changed;
    /* +0x197D */ u8 bond_created_connection;
    /* +0x197E */ u8 bond_disc_reason;
    /* +0x197F */ u8 sec_pin_code_len;
    /* +0x1980 */ u8 sec_pin_code[BTM_MAX_PIN_LEN];
    /* +0x1990 */ tBTM_SEC_SERV_REC sec_serv_rec[BTM_SEC_MAX_SERVICE_RECORDS];
    /* +0x1F30 */ tBTM_SEC_DEV_REC sec_dev_rec[BTM_SEC_MAX_DEVICE_RECORDS];
    /* +0x27B0 */ tBTM_MKEY_CALLBACK* mkey_cback;
    /* +0x27B4 */ u8 connecting_bda[6];
    /* +0x27BA */ u8 connecting_dc[3];
    /* +0x27BD */ u8 first_disabled_channel;
    /* +0x27BE */ u8 last_disabled_channel;
    /* +0x27BF */ u8 acl_disc_reason;
    /* +0x27C0 */ u8 trace_level;
    /* +0x27C1 */ u8 pad_0x27C1[3];
} tBTM_CB; /* size: 0x27C4 */

/* The little-endian stream readers of the HCI event parsers; each advances `p`. */
#ifndef STREAM_TO_UINT8
#define STREAM_TO_UINT8(u8v, p) {u8v = (u8)(*(p)); (p) += 1;}
#define STREAM_TO_UINT16(u16v, p) {u16v = ((u16)(*(p)) + (((u16)(*((p) + 1))) << 8)); (p) += 2;}
#endif
#ifndef STREAM_TO_BDADDR
#define STREAM_TO_BDADDR(a, p) {int ijk; u8* pbda = (u8*)(a) + BD_ADDR_LEN - 1; \
    for (ijk = 0; ijk < BD_ADDR_LEN; ijk++) *pbda-- = *(p)++;}
#endif

/* The trace levels a statement needs. */
#ifndef BT_TRACE_LEVEL_ERROR
#define BT_TRACE_LEVEL_ERROR 1
#define BT_TRACE_LEVEL_WARNING 2
#define BT_TRACE_LEVEL_API 3
#define BT_TRACE_LEVEL_EVENT 4
#define BT_TRACE_LEVEL_DEBUG 5
#endif

/* The BTM layer's trace masks (layer 0x0D, then the type). */
#define TRACE_BTM_ERROR 0x000D0000
#define TRACE_BTM_WARNING 0x000D0001
#define TRACE_BTM_API 0x000D0002
#define TRACE_BTM_EVENT 0x000D0003
#define TRACE_BTM_DEBUG 0x000D0004

/* The BTM layer's trace statements: each prints only while btm_cb's trace level reaches its own. */
#define BTM_TRACE_API0(m) {if (btm_cb.trace_level >= BT_TRACE_LEVEL_API) LogMsg_0(TRACE_BTM_API, (m));}
#define BTM_TRACE_API1(m, p1) {if (btm_cb.trace_level >= BT_TRACE_LEVEL_API) \
    LogMsg_1(TRACE_BTM_API, (m), (u32)(p1));}
#define BTM_TRACE_API4(m, p1, p2, p3, p4) {if (btm_cb.trace_level >= BT_TRACE_LEVEL_API) \
    LogMsg_4(TRACE_BTM_API, (m), (u32)(p1), (u32)(p2), (u32)(p3), (u32)(p4));}
#define BTM_TRACE_API6(m, p1, p2, p3, p4, p5, p6) {if (btm_cb.trace_level >= BT_TRACE_LEVEL_API) \
    LogMsg_6(TRACE_BTM_API, (m), (u32)(p1), (u32)(p2), (u32)(p3), (u32)(p4), (u32)(p5), (u32)(p6));}
#define BTM_TRACE_EVENT0(m) {if (btm_cb.trace_level >= BT_TRACE_LEVEL_EVENT) LogMsg_0(TRACE_BTM_EVENT, (m));}
#define BTM_TRACE_EVENT1(m, p1) {if (btm_cb.trace_level >= BT_TRACE_LEVEL_EVENT) \
    LogMsg_1(TRACE_BTM_EVENT, (m), (u32)(p1));}
#define BTM_TRACE_EVENT2(m, p1, p2) {if (btm_cb.trace_level >= BT_TRACE_LEVEL_EVENT) \
    LogMsg_2(TRACE_BTM_EVENT, (m), (u32)(p1), (u32)(p2));}
#define BTM_TRACE_EVENT6(m, p1, p2, p3, p4, p5, p6) {if (btm_cb.trace_level >= BT_TRACE_LEVEL_EVENT) \
    LogMsg_6(TRACE_BTM_EVENT, (m), (u32)(p1), (u32)(p2), (u32)(p3), (u32)(p4), (u32)(p5), (u32)(p6));}
#define BTM_TRACE_DEBUG0(m) {if (btm_cb.trace_level >= BT_TRACE_LEVEL_DEBUG) LogMsg_0(TRACE_BTM_DEBUG, (m));}
#define BTM_TRACE_DEBUG1(m, p1) {if (btm_cb.trace_level >= BT_TRACE_LEVEL_DEBUG) \
    LogMsg_1(TRACE_BTM_DEBUG, (m), (u32)(p1));}
#define BTM_TRACE_DEBUG2(m, p1, p2) {if (btm_cb.trace_level >= BT_TRACE_LEVEL_DEBUG) \
    LogMsg_2(TRACE_BTM_DEBUG, (m), (u32)(p1), (u32)(p2));}

#ifdef __cplusplus
extern "C" {
#endif

/* .bss 0x80739698 - the BTM control block. */
extern tBTM_CB btm_cb;

/* 0x804863F4 - clears the control block and initialises the inquiry, ACL, security, SCO and device modules. */
void btm_init(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_BTE_BTM_MAIN_H */
