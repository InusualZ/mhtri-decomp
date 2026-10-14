/* BTE/bta_dm_int.h - the BTA device manager's types, events and constants the four BTE/bta_dm_*.c units share: the
 *   control blocks, the messages the API posts and the actions read, the security and search event payloads and the
 *   configuration tables. Each unit's own entry points are declared in its own header (BTE/bta_dm_act.h,
 *   BTE/bta_dm_api.h, BTE/bta_dm_main.h, BTE/bta_dm_pm.h). */
#ifndef MHTRI_BTE_BTA_DM_INT_H
#define MHTRI_BTE_BTA_DM_INT_H

#include "types.h"
#include "BTE/gki_time.h"
#include "BTE/gki_ppc.h"
#include "BTE/sdp_int.h"
#include "BTE/btm_acl.h"
#include "BTE/btm_dev.h"
#include "BTE/btm_devctl.h"
#include "BTE/btm_inq.h"
#include "BTE/btm_main.h"
#include "BTE/btm_sec.h"
#include "BTE/hcicmds.h"
#include "BTE/bd.h"
#include "BTE/bta_dm_cfg.h"
#include "BTE/bta_sys_conn.h"
#include "BTE/bta_sys_main.h"
#include "BTE/wbt_ext.h"
#include "WUD/WUDHidHost.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif

#define BD_ADDR_LEN 6
#define BD_NAME_LEN 248
#define DEV_CLASS_LEN 3
#define LINK_KEY_LEN 16

/* The BTA service ids the lookup tables are indexed by. */
#define BTA_MAX_SERVICE_ID 23

/* The trace level the BTA layers print at (the BTA system manager's) and the BTA trace masks. */
#define BT_TRACE_LEVEL_WARNING 2
#define TRACE_APPL_WARNING 0x00000501
#define TRACE_APPL_EVENT 0x00000503
#define APPL_TRACE_WARNING0(m) {if (appl_trace_level >= BT_TRACE_LEVEL_WARNING) LogMsg_0(TRACE_APPL_WARNING, (m));}
#define APPL_TRACE_EVENT0(m) {if (appl_trace_level >= BT_TRACE_LEVEL_EVENT) LogMsg_0(TRACE_APPL_EVENT, (m));}
#define APPL_TRACE_EVENT1(m, p1) {if (appl_trace_level >= BT_TRACE_LEVEL_EVENT) LogMsg_1(TRACE_APPL_EVENT, (m), (u32)(p1));}
#define APPL_TRACE_EVENT3(m, p1, p2, p3) {if (appl_trace_level >= BT_TRACE_LEVEL_EVENT) \
    LogMsg_3(TRACE_APPL_EVENT, (m), (u32)(p1), (u32)(p2), (u32)(p3));}

/* The BTM status values the callbacks return. */
#define BTM_SUCCESS 0
#define BTM_CMD_STARTED 1
#define BTM_NOT_AUTHORIZED 11

/* The device manager events (BTA_ID_DM << 8) the API posts; the low byte indexes bta_dm_st_table. */
#define BTA_DM_API_ENABLE_EVT 0x100
#define BTA_DM_API_DISABLE_EVT 0x101
#define BTA_DM_API_SET_NAME_EVT 0x102
#define BTA_DM_API_SET_VISIBILITY_EVT 0x103
#define BTA_DM_API_SIG_STRENGTH_EVT 0x104
#define BTA_DM_ACL_CHANGE_EVT 0x105
#define BTA_DM_API_BOND_EVT 0x106
#define BTA_DM_API_PIN_REPLY_EVT 0x107
#define BTA_DM_API_AUTH_REPLY_EVT 0x108
#define BTA_DM_PM_BTM_STATUS_EVT 0x109
#define BTA_DM_PM_TIMER_EVT 0x10A
#define BTA_DM_API_KEEP_ACL_EVT 0x10B
#define BTA_DM_API_SEND_HCI_RESET_EVT 0x10C

/* The device search events (BTA_ID_DM_SEARCH << 8); the low byte indexes the search state tables. */
#define BTA_DM_API_SEARCH_EVT 0x200
#define BTA_DM_API_SEARCH_CANCEL_EVT 0x201
#define BTA_DM_API_DISCOVER_EVT 0x202
#define BTA_DM_INQUIRY_CMPL_EVT 0x203
#define BTA_DM_REMT_NAME_EVT 0x204
#define BTA_DM_REMT_NAME_TOUT_EVT 0x205
#define BTA_DM_SDP_RESULT_EVT 0x206
#define BTA_DM_SEARCH_CMPL_EVT 0x207
#define BTA_DM_DISCOVERY_RESULT_EVT 0x208

/* The security events the device manager reports to its security callback. */
#define BTA_DM_ENABLE_EVT 0
#define BTA_DM_DISABLE_EVT 1
#define BTA_DM_PIN_REQ_EVT 2
#define BTA_DM_AUTH_CMPL_EVT 3
#define BTA_DM_AUTHORIZE_EVT 4
#define BTA_DM_LINK_UP_EVT 5
#define BTA_DM_LINK_DOWN_EVT 6
#define BTA_DM_SIG_STRENGTH_EVT 7

/* The search events the device manager reports to its search callback. */
#define BTA_DM_INQ_RES_EVT 0
#define BTA_DM_INQ_CMPL_EVT 1
#define BTA_DM_DISC_RES_EVT 2
#define BTA_DM_DISC_CMPL_EVT 3
#define BTA_DM_SEARCH_CANCEL_CMPL_EVT 4

/* The masks a signal strength request and report carry. */
#define BTA_SIG_STRENGTH_RSSI_MASK 1
#define BTA_SIG_STRENGTH_LINK_QUALITY_MASK 2

/* The connection states and the power modes the power manager selects. */
#define BTA_DM_CONNECTED 1
#define BTA_DM_PM_NO_ACTION 0
#define BTA_DM_PM_PARK 1
#define BTA_DM_PM_SNIFF 2
#define BTA_DM_PM_ACTIVE 4
#define BTA_DM_PM_NO_PREF 0x10
#define BTA_DM_PM_RESTART 1
#define BTA_DM_PM_NEW_REQ 0
#define BTA_DM_NUM_PM_TIMER 3
#define BTA_DM_NUM_CONN_SRVS 5
#define BTA_DM_NUM_COMPRESS_SRVS 5
#define BTA_DM_NUM_PM_EVTS 9
#define BTA_DM_MAX_PEER_DEVICES 7

/* The connection state changes BTA_SYS reports to the resource manager, compression and power manager callbacks. */
#define BTA_SYS_CONN_OPEN 0
#define BTA_SYS_CONN_CLOSE 1
#define BTA_SYS_APP_OPEN 2
#define BTA_SYS_APP_CLOSE 3
#define BTA_SYS_SCO_OPEN 4
#define BTA_SYS_SCO_CLOSE 5
#define BTA_ALL_APP_ID 0xFF

/* The host controller features the power manager tests: a version 1.1 controller of manufacturer 15 (Broadcom). */
#define HCI_PROTO_VERSION_1_2 2
#define LMP_COMPID_BROADCOM 15
#define HCI_ENABLE_SNIFF_MODE_MASK 0x000B

/* size: 0x0B - a connected peer device: its address, connection state and power mode bookkeeping. */
typedef struct tBTA_DM_PEER_DEVICE {
    /* +0x00 */ u8 peer_bdaddr[BD_ADDR_LEN];
    /* +0x06 */ u8 conn_state;
    /* +0x07 */ u8 pref_role;
    /* +0x08 */ u8 unused_0x08;
    /* +0x09 */ u8 pm_mode_attempted;
    /* +0x0A */ u8 pm_mode_failed;
} tBTA_DM_PEER_DEVICE; /* size: 0x0B */

/* size: 0x4E - the connected peer devices. */
typedef struct tBTA_DM_ACTIVE_LINK {
    /* +0x00 */ tBTA_DM_PEER_DEVICE peer_device[BTA_DM_MAX_PEER_DEVICES];
    /* +0x4D */ u8 count;
} tBTA_DM_ACTIVE_LINK; /* size: 0x4E */

/* size: 0x20 - a power manager timer and the peer it runs for. */
typedef struct tBTA_PM_TIMER {
    /* +0x00 */ TIMER_LIST_ENT timer;
    /* +0x18 */ u8 peer_bdaddr[BD_ADDR_LEN];
    /* +0x1E */ u8 in_use;
    /* +0x1F */ u8 pad_0x1F;
} tBTA_PM_TIMER; /* size: 0x20 */

/* size: 0x104 - the device manager control block (bta_dm_cb): the links, the security callback, the signal
 * strength, disable and power manager timers and the pending PIN request. */
typedef struct tBTA_DM_CB {
    /* +0x000 */ tBTA_DM_ACTIVE_LINK device_list;
    /* +0x04E */ u8 pad_0x04E[2];
    /* +0x050 */ BtaDmSecCallback p_sec_cback;
    /* +0x054 */ TIMER_LIST_ENT signal_strength_timer;
    /* +0x06C */ u8 signal_strength_mask;
    /* +0x06D */ u8 pad_0x06D[3];
    /* +0x070 */ u16 signal_strength_period;
    /* +0x072 */ u8 disabling;
    /* +0x073 */ u8 pad_0x073;
    /* +0x074 */ TIMER_LIST_ENT disable_timer;
    /* +0x08C */ u8 unused_0x08C[9];
    /* +0x095 */ u8 pm_id;
    /* +0x096 */ u8 pad_0x096[2];
    /* +0x098 */ tBTA_PM_TIMER pm_timer[BTA_DM_NUM_PM_TIMER];
    /* +0x0F8 */ u8 pin_bd_addr[BD_ADDR_LEN];
    /* +0x0FE */ u8 pin_dev_class[DEV_CLASS_LEN];
    /* +0x101 */ u8 keep_acl_on_shut_down;
    /* +0x102 */ u8 pad_0x102[2];
} tBTA_DM_CB; /* size: 0x104 */

union tBTA_DM_MSG;
union tBTA_DM_SEARCH;

/* The search event callback the device manager reports to. */
typedef void (*tBTA_DM_SEARCH_CBACK)(u8 event, union tBTA_DM_SEARCH* p_data);

/* size: 0x7C - the device search control block (bta_dm_search_cb). */
typedef struct tBTA_DM_SEARCH_CB {
    /* +0x00 */ tBTA_DM_SEARCH_CBACK p_search_cback;
    /* +0x04 */ tBTM_INQ_INFO* p_btm_inq_info;
    /* +0x08 */ u32 services;
    /* +0x0C */ u32 services_to_search;
    /* +0x10 */ u32 services_found;
    /* +0x14 */ tSDP_DISCOVERY_DB* p_sdp_db;
    /* +0x18 */ u16 state;
    /* +0x1A */ u8 peer_bdaddr[BD_ADDR_LEN];
    /* +0x20 */ char peer_name[32];
    /* +0x40 */ TIMER_LIST_ENT search_timer;
    /* +0x58 */ TIMER_LIST_ENT rnr_timer;
    /* +0x70 */ u8 service_index;
    /* +0x71 */ u8 pad_0x71[3];
    /* +0x74 */ union tBTA_DM_MSG* p_search_queue;
    /* +0x78 */ u8 wait_disc;
    /* +0x79 */ u8 sdp_results;
    /* +0x7A */ u8 pad_0x7A[2];
} tBTA_DM_SEARCH_CB; /* size: 0x7C */

/* size: 0x09 - a service that opened a connection: the peer, the BTA id, the application id and its state. */
typedef struct tBTA_DM_SRVCS {
    /* +0x00 */ u8 peer_bdaddr[BD_ADDR_LEN];
    /* +0x06 */ u8 id;
    /* +0x07 */ u8 app_id;
    /* +0x08 */ u8 state;
} tBTA_DM_SRVCS; /* size: 0x09 */

/* size: 0x2E - the services the power manager tracks (bta_dm_conn_srvcs). */
typedef struct tBTA_DM_CONNECTED_SRVCS {
    /* +0x00 */ u8 count;
    /* +0x01 */ tBTA_DM_SRVCS conn_srvc[BTA_DM_NUM_CONN_SRVS];
} tBTA_DM_CONNECTED_SRVCS; /* size: 0x2E */

/* size: 0x0C - BTA_DM_API_ENABLE_EVT: the security callback. */
typedef struct tBTA_DM_API_ENABLE {
    /* +0x00 */ BT_HDR hdr;
    /* +0x08 */ BtaDmSecCallback p_sec_cback;
} tBTA_DM_API_ENABLE; /* size: 0x0C */

/* size: 0x28 - BTA_DM_API_SET_NAME_EVT: the local device name. */
typedef struct tBTA_DM_API_SET_NAME {
    /* +0x00 */ BT_HDR hdr;
    /* +0x08 */ char name[32];
} tBTA_DM_API_SET_NAME; /* size: 0x28 */

/* size: 0x0A - BTA_DM_API_SET_VISIBILITY_EVT: the discoverable and connectable modes. */
typedef struct tBTA_DM_API_SET_VISIBILITY {
    /* +0x00 */ BT_HDR hdr;
    /* +0x08 */ u8 disc_mode;
    /* +0x09 */ u8 conn_mode;
} tBTA_DM_API_SET_VISIBILITY; /* size: 0x0A */

/* size: 0x0E - BTA_DM_API_SIG_STRENGTH_EVT: what to read, how often, and whether to start or stop. */
typedef struct tBTA_DM_API_SIG_STRENGTH {
    /* +0x00 */ BT_HDR hdr;
    /* +0x08 */ u8 mask;
    /* +0x09 */ u8 pad_0x09;
    /* +0x0A */ u16 period;
    /* +0x0C */ u8 start;
    /* +0x0D */ u8 pad_0x0D;
} tBTA_DM_API_SIG_STRENGTH; /* size: 0x0E - approximation: no poster in the build */

/* size: 0x12 - BTA_DM_ACL_CHANGE_EVT: a link came up or went down. */
typedef struct tBTA_DM_ACL_CHANGE {
    /* +0x00 */ BT_HDR hdr;
    /* +0x08 */ u8 pad_0x08[2];
    /* +0x0A */ u8 is_new;
    /* +0x0B */ u8 bd_addr[BD_ADDR_LEN];
    /* +0x11 */ u8 pad_0x11;
} tBTA_DM_ACL_CHANGE; /* size: 0x12 */

/* size: 0x20 - BTA_DM_API_BOND_EVT: the device and the PIN to bond with. */
typedef struct tBTA_DM_API_BOND {
    /* +0x00 */ BT_HDR hdr;
    /* +0x08 */ u8 bd_addr[BD_ADDR_LEN];
    /* +0x0E */ u8 pin_len;
    /* +0x0F */ u8 p_pin[16];
    /* +0x1F */ u8 pad_0x1F;
} tBTA_DM_API_BOND; /* size: 0x20 - approximation: no poster in the build */

/* size: 0x20 - BTA_DM_API_PIN_REPLY_EVT: the answer to a PIN request. */
typedef struct tBTA_DM_API_PIN_REPLY {
    /* +0x00 */ BT_HDR hdr;
    /* +0x08 */ u8 bd_addr[BD_ADDR_LEN];
    /* +0x0E */ u8 accept;
    /* +0x0F */ u8 pin_len;
    /* +0x10 */ u8 p_pin[16];
} tBTA_DM_API_PIN_REPLY; /* size: 0x20 */

/* size: 0x10 - BTA_DM_API_AUTH_REPLY_EVT: the answer to an authorization request. */
typedef struct tBTA_DM_API_AUTH_REPLY {
    /* +0x00 */ BT_HDR hdr;
    /* +0x08 */ u8 bd_addr[BD_ADDR_LEN];
    /* +0x0E */ u8 service;
    /* +0x0F */ u8 response;
} tBTA_DM_API_AUTH_REPLY; /* size: 0x10 - approximation: no poster in the build */

/* size: 0x14 - BTA_DM_PM_BTM_STATUS_EVT: a power mode change BTM reported. */
typedef struct tBTA_DM_PM_BTM_STATUS {
    /* +0x00 */ BT_HDR hdr;
    /* +0x08 */ u8 bd_addr[BD_ADDR_LEN];
    /* +0x0E */ u8 status;
    /* +0x0F */ u8 pad_0x0F;
    /* +0x10 */ u16 value;
    /* +0x12 */ u8 hci_status;
    /* +0x13 */ u8 pad_0x13;
} tBTA_DM_PM_BTM_STATUS; /* size: 0x14 */

/* size: 0x0E - BTA_DM_PM_TIMER_EVT: the peer whose power manager timer expired. */
typedef struct tBTA_DM_PM_TIMER {
    /* +0x00 */ BT_HDR hdr;
    /* +0x08 */ u8 bd_addr[BD_ADDR_LEN];
} tBTA_DM_PM_TIMER; /* size: 0x0E */

/* size: 0x1C - BTA_DM_API_SEARCH_EVT: the inquiry, the services to discover and the search callback. */
typedef struct tBTA_DM_API_SEARCH {
    /* +0x00 */ BT_HDR hdr;
    /* +0x08 */ BtaDmInq inq_params;
    /* +0x12 */ u8 pad_0x12[2];
    /* +0x14 */ u32 services;
    /* +0x18 */ tBTA_DM_SEARCH_CBACK p_cback;
} tBTA_DM_API_SEARCH; /* size: 0x1C */

/* size: 0x18 - BTA_DM_API_DISCOVER_EVT: the device, the services to discover and the search callback. */
typedef struct tBTA_DM_API_DISCOVER {
    /* +0x00 */ BT_HDR hdr;
    /* +0x08 */ u8 bd_addr[BD_ADDR_LEN];
    /* +0x0E */ u8 pad_0x0E[2];
    /* +0x10 */ u32 services;
    /* +0x14 */ tBTA_DM_SEARCH_CBACK p_cback;
} tBTA_DM_API_DISCOVER; /* size: 0x18 */

/* size: 0x0B - BTA_DM_INQ_RES_EVT: one inquiry response. */
typedef struct tBTA_DM_INQ_RES {
    /* +0x00 */ u8 bd_addr[BD_ADDR_LEN];
    /* +0x06 */ u8 dev_class[DEV_CLASS_LEN];
    /* +0x09 */ s8 rssi;
    /* +0x0A */ u8 remt_name_not_required;
} tBTA_DM_INQ_RES; /* size: 0x0B */

/* size: 0x01 - BTA_DM_INQ_CMPL_EVT: the number of responses. */
typedef struct tBTA_DM_INQ_CMPL {
    /* +0x00 */ u8 num_resps;
} tBTA_DM_INQ_CMPL; /* size: 0x01 */

/* size: 0x108 - BTA_DM_DISC_RES_EVT: a device's name and the services found on it. */
typedef struct tBTA_DM_DISC_RES {
    /* +0x000 */ u8 bd_addr[BD_ADDR_LEN];
    /* +0x006 */ char bd_name[BD_NAME_LEN];
    /* +0x0FE */ u8 pad_0x0FE[2];
    /* +0x100 */ u32 services;
    /* +0x104 */ u8 result;
    /* +0x105 */ u8 pad_0x105[3];
} tBTA_DM_DISC_RES; /* size: 0x108 */

/* size: 0x108 - the payload of a search event, by event. */
typedef union tBTA_DM_SEARCH {
    /* +0x00 */ tBTA_DM_INQ_RES inq_res;
    /* +0x00 */ tBTA_DM_INQ_CMPL inq_cmpl;
    /* +0x00 */ tBTA_DM_DISC_RES disc_res;
} tBTA_DM_SEARCH; /* size: 0x108 */

/* size: 0x0A - BTA_DM_INQUIRY_CMPL_EVT: the number of responses. */
typedef struct tBTA_DM_INQUIRY_CMPL {
    /* +0x00 */ BT_HDR hdr;
    /* +0x08 */ u8 num;
} tBTA_DM_INQUIRY_CMPL; /* size: 0x0A */

/* size: 0x0A - BTA_DM_SDP_RESULT_EVT: the SDP result code. */
typedef struct tBTA_DM_SDP_RESULT {
    /* +0x00 */ BT_HDR hdr;
    /* +0x08 */ u16 sdp_result;
} tBTA_DM_SDP_RESULT; /* size: 0x0A */

/* size: 0x110 - BTA_DM_REMT_NAME_EVT and BTA_DM_DISCOVERY_RESULT_EVT: a search result. */
typedef struct tBTA_DM_REM_NAME {
    /* +0x000 */ BT_HDR hdr;
    /* +0x008 */ tBTA_DM_SEARCH result;
} tBTA_DM_REM_NAME; /* size: 0x110 */

/* size: 0x110 - every message the device manager handles, by event. */
typedef union tBTA_DM_MSG {
    /* +0x00 */ BT_HDR hdr;
    /* +0x00 */ tBTA_DM_API_ENABLE enable;
    /* +0x00 */ tBTA_DM_API_SET_NAME set_name;
    /* +0x00 */ tBTA_DM_API_SET_VISIBILITY set_visibility;
    /* +0x00 */ tBTA_DM_API_SIG_STRENGTH sig_strength;
    /* +0x00 */ tBTA_DM_ACL_CHANGE acl_change;
    /* +0x00 */ tBTA_DM_API_BOND bond;
    /* +0x00 */ tBTA_DM_API_PIN_REPLY pin_reply;
    /* +0x00 */ tBTA_DM_API_AUTH_REPLY auth_reply;
    /* +0x00 */ tBTA_DM_PM_BTM_STATUS pm_status;
    /* +0x00 */ tBTA_DM_PM_TIMER pm_timer;
    /* +0x00 */ tBTA_DM_API_SEARCH search;
    /* +0x00 */ tBTA_DM_API_DISCOVER discover;
    /* +0x00 */ tBTA_DM_INQUIRY_CMPL inq_cmpl;
    /* +0x00 */ tBTA_DM_SDP_RESULT sdp_event;
    /* +0x00 */ tBTA_DM_REM_NAME rem_name;
    /* +0x00 */ tBTA_DM_REM_NAME disc_result;
} tBTA_DM_MSG; /* size: 0x110 */

/* size: 0x06 - BTA_DM_ENABLE_EVT and BTA_DM_LINK_UP_EVT: the device. */
typedef struct tBTA_DM_LINK_UP {
    /* +0x00 */ u8 bd_addr[BD_ADDR_LEN];
} tBTA_DM_LINK_UP; /* size: 0x06 */

/* size: 0x07 - BTA_DM_LINK_DOWN_EVT: the device and the disconnect reason. */
typedef struct tBTA_DM_LINK_DOWN {
    /* +0x00 */ u8 bd_addr[BD_ADDR_LEN];
    /* +0x06 */ u8 status;
} tBTA_DM_LINK_DOWN; /* size: 0x07 */

/* size: 0x101 - BTA_DM_PIN_REQ_EVT: the device asking for a PIN. */
typedef struct tBTA_DM_PIN_REQ {
    /* +0x000 */ u8 bd_addr[BD_ADDR_LEN];
    /* +0x006 */ char bd_name[BD_NAME_LEN];
    /* +0x0FE */ u8 dev_class[DEV_CLASS_LEN];
} tBTA_DM_PIN_REQ; /* size: 0x101 */

/* size: 0x110 - BTA_DM_AUTH_CMPL_EVT: the result of an authentication and the new link key. */
typedef struct tBTA_DM_AUTH_CMPL {
    /* +0x000 */ u8 bd_addr[BD_ADDR_LEN];
    /* +0x006 */ char bd_name[BD_NAME_LEN];
    /* +0x0FE */ u8 key_present;
    /* +0x0FF */ u8 key[LINK_KEY_LEN];
    /* +0x10F */ u8 success;
} tBTA_DM_AUTH_CMPL; /* size: 0x110 */

/* size: 0x0FF - BTA_DM_AUTHORIZE_EVT: the device and the service it asks for. */
typedef struct tBTA_DM_AUTHORIZE {
    /* +0x000 */ u8 bd_addr[BD_ADDR_LEN];
    /* +0x006 */ char bd_name[BD_NAME_LEN];
    /* +0x0FE */ u8 service;
} tBTA_DM_AUTHORIZE; /* size: 0x0FF */

/* size: 0x09 - BTA_DM_SIG_STRENGTH_EVT: the device and the value read. */
typedef struct tBTA_DM_SIG_STRENGTH {
    /* +0x00 */ u8 bd_addr[BD_ADDR_LEN];
    /* +0x06 */ u8 mask;
    /* +0x07 */ s8 rssi_value;
    /* +0x08 */ u8 link_quality_value;
} tBTA_DM_SIG_STRENGTH; /* size: 0x09 */

/* size: 0x110 - the payload of a security event, by event (tBTA_DM_SEC). */
typedef union BtaDmSecEventData {
    /* +0x00 */ tBTA_DM_LINK_UP enable;
    /* +0x00 */ tBTA_DM_PIN_REQ pin_req;
    /* +0x00 */ tBTA_DM_AUTH_CMPL auth_cmpl;
    /* +0x00 */ tBTA_DM_AUTHORIZE authorize;
    /* +0x00 */ tBTA_DM_LINK_UP link_up;
    /* +0x00 */ tBTA_DM_LINK_DOWN link_down;
    /* +0x00 */ tBTA_DM_SIG_STRENGTH sig_strength;
} tBTA_DM_SEC; /* size: 0x110 */

/* Sets the trusted-service bit of the BTM service `service` in the two-word mask `p`. */
#define BTM_SEC_SET_SERVICE(p, service)                                                                            \
    {                                                                                                              \
        u8 word = (u8)((service) / 32);                                                                            \
        (p)[word] |= 1 << ((service) - word * 32);                                                                 \
    }

/* An action of the device manager state machines. */
typedef void (*tBTA_DM_ACTION)(tBTA_DM_MSG* p_data);

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_BTE_BTA_DM_INT_H */
