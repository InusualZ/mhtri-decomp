/*
 * BTE/bta_dm_main.c - the BTA device manager state machines: bta_dm_sm_execute and bta_dm_search_sm_execute with
 *   their action and state tables.
 *
 * RANGE. .text 0x8047DAFC-0x8047DC5C (2 functions, 0x160 B); .rodata 0x80573690-0x805737B8; .bss
 *   0x807392B8-0x80739438. Medium: the two state-machine drivers between the API run and bta_dm_init_pm. The .rodata
 *   is read only by them: the action table bta_dm_action, the state table bta_dm_st_table, the search action table,
 *   the four search state tables and their index bta_dm_search_st_tbl. The .bss is GUESS: bta_dm_search_cb
 *   (0x807392B8, 0x7C B) and bta_dm_cb (0x80739334, 0x104 B) are Broadcom's bta_dm_main.c globals; the readers alone
 *   allow them in BTE/bta_dm_act.c too, with the same layout.
 * FLAGS. GC/3.0a5.2 + `cflags_nw4r`; BTE/bta_dm_act.c's header has the compiler evidence.
 * NAMES. Broadcom's bta_dm_main.c names.
 * RESIDUALS. none known.
 * SHAPES. plain C; each state table is a u8 [event][3] array (two actions and the next search state).
 */

#include "BTE/bta_dm_main.h"
#include "BTE/bta_dm_act.h"
#include "BTE/bta_dm_pm.h"

/* The number of actions per state table row, the row width and the column of the next search state. */
#define BTA_DM_ACTIONS 2
#define BTA_DM_NUM_COLS 3
#define BTA_DM_SEARCH_NEXT_STATE 2

/* The action indexes of bta_dm_action; BTA_DM_IGNORE ends a row. */
enum {
    BTA_DM_API_ENABLE,
    BTA_DM_API_DISABLE,
    BTA_DM_SET_DEV_NAME,
    BTA_DM_SET_VISIBILITY,
    BTA_DM_API_BOND,
    BTA_DM_PIN_REPLY,
    BTA_DM_AUTH_REPLY,
    BTA_DM_SIGNAL_STRENGTH,
    BTA_DM_ACL_CHANGE,
    BTA_DM_PM_BTM_STATUS,
    BTA_DM_PM_TIMER,
    BTA_DM_KEEP_ACL,
    BTA_DM_SEND_HCI_RESET,
    BTA_DM_NUM_ACTIONS
};
#define BTA_DM_IGNORE BTA_DM_NUM_ACTIONS

/* The action indexes of bta_dm_search_action; BTA_DM_SEARCH_IGNORE ends a row. */
enum {
    BTA_DM_API_SEARCH,
    BTA_DM_API_SEARCH_CANCEL,
    BTA_DM_API_DISCOVER,
    BTA_DM_INQUIRY_CMPL,
    BTA_DM_REMT_NAME,
    BTA_DM_SDP_RESULT,
    BTA_DM_SEARCH_CMPL,
    BTA_DM_FREE_SDP_DB,
    BTA_DM_DISC_RESULT,
    BTA_DM_SEARCH_RESULT,
    BTA_DM_QUEUE_SEARCH,
    BTA_DM_QUEUE_DISC,
    BTA_DM_SEARCH_CLEAR_QUEUE,
    BTA_DM_SEARCH_CANCEL_CMPL,
    BTA_DM_SEARCH_CANCEL_NOTIFY,
    BTA_DM_SEARCH_CANCEL_TRANSAC_CMPL,
    BTA_DM_DISC_RMT_NAME,
    BTA_DM_CANCEL_RMT_NAME,
    BTA_DM_SEARCH_NUM_ACTIONS
};
#define BTA_DM_SEARCH_IGNORE BTA_DM_SEARCH_NUM_ACTIONS

/* The device search states. */
enum {
    BTA_DM_SEARCH_IDLE,
    BTA_DM_SEARCH_ACTIVE,
    BTA_DM_SEARCH_CANCELLING,
    BTA_DM_DISCOVER_ACTIVE
};

tBTA_DM_SEARCH_CB bta_dm_search_cb;
tBTA_DM_CB bta_dm_cb;

/* The device manager actions, by action index. */
const tBTA_DM_ACTION bta_dm_action[BTA_DM_NUM_ACTIONS] = {
    bta_dm_enable,
    bta_dm_disable,
    bta_dm_set_dev_name,
    bta_dm_set_visibility,
    bta_dm_bond,
    bta_dm_pin_reply,
    bta_dm_auth_reply,
    bta_dm_signal_strength,
    bta_dm_acl_change,
    bta_dm_pm_btm_status,
    bta_dm_pm_timer,
    bta_dm_keep_acl,
    bta_dm_send_hci_reset,
};

/* The device manager state table: the actions of each event (the third column is unused). */
const u8 bta_dm_st_table[][BTA_DM_NUM_COLS] = {
    {BTA_DM_API_ENABLE, BTA_DM_IGNORE, 0},
    {BTA_DM_API_DISABLE, BTA_DM_IGNORE, 0},
    {BTA_DM_SET_DEV_NAME, BTA_DM_IGNORE, 0},
    {BTA_DM_SET_VISIBILITY, BTA_DM_IGNORE, 0},
    {BTA_DM_SIGNAL_STRENGTH, BTA_DM_IGNORE, 0},
    {BTA_DM_ACL_CHANGE, BTA_DM_IGNORE, 0},
    {BTA_DM_API_BOND, BTA_DM_IGNORE, 0},
    {BTA_DM_PIN_REPLY, BTA_DM_IGNORE, 0},
    {BTA_DM_AUTH_REPLY, BTA_DM_IGNORE, 0},
    {BTA_DM_PM_BTM_STATUS, BTA_DM_IGNORE, 0},
    {BTA_DM_PM_TIMER, BTA_DM_IGNORE, 0},
    {BTA_DM_KEEP_ACL, BTA_DM_IGNORE, 0},
    {BTA_DM_SEND_HCI_RESET, BTA_DM_IGNORE, 0},
};

/* The device search actions, by action index. */
const tBTA_DM_ACTION bta_dm_search_action[BTA_DM_SEARCH_NUM_ACTIONS] = {
    bta_dm_search_start,
    bta_dm_search_cancel,
    bta_dm_discover,
    bta_dm_inq_cmpl,
    bta_dm_rmt_name,
    bta_dm_sdp_result,
    bta_dm_search_cmpl,
    bta_dm_free_sdp_db,
    bta_dm_disc_result,
    bta_dm_search_result,
    bta_dm_queue_search,
    bta_dm_queue_disc,
    bta_dm_search_clear_queue,
    bta_dm_search_cancel_cmpl,
    bta_dm_search_cancel_notify,
    bta_dm_search_cancel_transac_cmpl,
    bta_dm_disc_rmt_name,
    bta_dm_cancel_rmt_name,
};

/* The idle search state: the actions of each search event and the next state. */
const u8 bta_dm_search_idle_st_table[][BTA_DM_NUM_COLS] = {
    {BTA_DM_API_SEARCH, BTA_DM_SEARCH_IGNORE, BTA_DM_SEARCH_ACTIVE},
    {BTA_DM_SEARCH_CANCEL_NOTIFY, BTA_DM_SEARCH_IGNORE, BTA_DM_SEARCH_IDLE},
    {BTA_DM_API_DISCOVER, BTA_DM_SEARCH_IGNORE, BTA_DM_DISCOVER_ACTIVE},
    {BTA_DM_SEARCH_IGNORE, BTA_DM_SEARCH_IGNORE, BTA_DM_SEARCH_IDLE},
    {BTA_DM_SEARCH_IGNORE, BTA_DM_SEARCH_IGNORE, BTA_DM_SEARCH_IDLE},
    {BTA_DM_SEARCH_IGNORE, BTA_DM_SEARCH_IGNORE, BTA_DM_SEARCH_IDLE},
    {BTA_DM_FREE_SDP_DB, BTA_DM_SEARCH_IGNORE, BTA_DM_SEARCH_IDLE},
    {BTA_DM_SEARCH_IGNORE, BTA_DM_SEARCH_IGNORE, BTA_DM_SEARCH_IDLE},
    {BTA_DM_SEARCH_IGNORE, BTA_DM_SEARCH_IGNORE, BTA_DM_SEARCH_IDLE},
};

/* The active search state. */
const u8 bta_dm_search_search_active_st_table[][BTA_DM_NUM_COLS] = {
    {BTA_DM_SEARCH_IGNORE, BTA_DM_SEARCH_IGNORE, BTA_DM_SEARCH_ACTIVE},
    {BTA_DM_API_SEARCH_CANCEL, BTA_DM_SEARCH_IGNORE, BTA_DM_SEARCH_CANCELLING},
    {BTA_DM_SEARCH_IGNORE, BTA_DM_SEARCH_IGNORE, BTA_DM_SEARCH_ACTIVE},
    {BTA_DM_INQUIRY_CMPL, BTA_DM_SEARCH_IGNORE, BTA_DM_SEARCH_ACTIVE},
    {BTA_DM_REMT_NAME, BTA_DM_SEARCH_IGNORE, BTA_DM_SEARCH_ACTIVE},
    {BTA_DM_CANCEL_RMT_NAME, BTA_DM_SEARCH_IGNORE, BTA_DM_SEARCH_ACTIVE},
    {BTA_DM_SDP_RESULT, BTA_DM_SEARCH_IGNORE, BTA_DM_SEARCH_ACTIVE},
    {BTA_DM_SEARCH_CMPL, BTA_DM_SEARCH_IGNORE, BTA_DM_SEARCH_IDLE},
    {BTA_DM_SEARCH_RESULT, BTA_DM_SEARCH_IGNORE, BTA_DM_SEARCH_ACTIVE},
};

/* The cancelling search state. */
const u8 bta_dm_search_search_cancelling_st_table[][BTA_DM_NUM_COLS] = {
    {BTA_DM_QUEUE_SEARCH, BTA_DM_SEARCH_IGNORE, BTA_DM_SEARCH_CANCELLING},
    {BTA_DM_SEARCH_CLEAR_QUEUE, BTA_DM_SEARCH_CANCEL_NOTIFY, BTA_DM_SEARCH_CANCELLING},
    {BTA_DM_QUEUE_DISC, BTA_DM_SEARCH_IGNORE, BTA_DM_SEARCH_CANCELLING},
    {BTA_DM_SEARCH_CANCEL_CMPL, BTA_DM_SEARCH_IGNORE, BTA_DM_SEARCH_IDLE},
    {BTA_DM_SEARCH_CANCEL_TRANSAC_CMPL, BTA_DM_SEARCH_CANCEL_CMPL, BTA_DM_SEARCH_IDLE},
    {BTA_DM_SEARCH_IGNORE, BTA_DM_SEARCH_IGNORE, BTA_DM_SEARCH_CANCELLING},
    {BTA_DM_SEARCH_CANCEL_TRANSAC_CMPL, BTA_DM_SEARCH_CANCEL_CMPL, BTA_DM_SEARCH_IDLE},
    {BTA_DM_SEARCH_CANCEL_CMPL, BTA_DM_SEARCH_IGNORE, BTA_DM_SEARCH_IDLE},
    {BTA_DM_SEARCH_CANCEL_CMPL, BTA_DM_SEARCH_IGNORE, BTA_DM_SEARCH_IDLE},
};

/* The active discovery state. */
const u8 bta_dm_search_disc_active_st_table[][BTA_DM_NUM_COLS] = {
    {BTA_DM_SEARCH_IGNORE, BTA_DM_SEARCH_IGNORE, BTA_DM_DISCOVER_ACTIVE},
    {BTA_DM_SEARCH_CANCEL_NOTIFY, BTA_DM_SEARCH_IGNORE, BTA_DM_DISCOVER_ACTIVE},
    {BTA_DM_SEARCH_IGNORE, BTA_DM_SEARCH_IGNORE, BTA_DM_DISCOVER_ACTIVE},
    {BTA_DM_SEARCH_IGNORE, BTA_DM_SEARCH_IGNORE, BTA_DM_DISCOVER_ACTIVE},
    {BTA_DM_DISC_RMT_NAME, BTA_DM_SEARCH_IGNORE, BTA_DM_DISCOVER_ACTIVE},
    {BTA_DM_CANCEL_RMT_NAME, BTA_DM_SEARCH_IGNORE, BTA_DM_DISCOVER_ACTIVE},
    {BTA_DM_SDP_RESULT, BTA_DM_SEARCH_IGNORE, BTA_DM_DISCOVER_ACTIVE},
    {BTA_DM_SEARCH_IGNORE, BTA_DM_SEARCH_IGNORE, BTA_DM_SEARCH_IDLE},
    {BTA_DM_DISC_RESULT, BTA_DM_SEARCH_IGNORE, BTA_DM_DISCOVER_ACTIVE},
};

/* The search state tables, by search state. */
const u8 (*const bta_dm_search_st_tbl[])[BTA_DM_NUM_COLS] = {
    bta_dm_search_idle_st_table,
    bta_dm_search_search_active_st_table,
    bta_dm_search_search_cancelling_st_table,
    bta_dm_search_disc_active_st_table,
};

/* 0x8047DAFC (0x98): Runs the device manager actions the state table gives for the event of `p_msg`. */
u8 bta_dm_sm_execute(BT_HDR* p_msg) {
    const u8(*state_table)[BTA_DM_NUM_COLS] = bta_dm_st_table;
    u8 action;
    int i;

    for (i = 0; i < BTA_DM_ACTIONS; i++) {
        if ((action = state_table[p_msg->event & 0x00FF][i]) != BTA_DM_IGNORE) {
            (*bta_dm_action[action])((tBTA_DM_MSG*)p_msg);
        } else {
            break;
        }
    }
    return TRUE;
}

/* 0x8047DB94 (0xC8): Moves the search to the next state and runs the search actions of the event of `p_msg`. */
u8 bta_dm_search_sm_execute(BT_HDR* p_msg) {
    const u8(*state_table)[BTA_DM_NUM_COLS];
    u8 action;
    int i;

    state_table = bta_dm_search_st_tbl[bta_dm_search_cb.state];
    bta_dm_search_cb.state = state_table[p_msg->event & 0x00FF][BTA_DM_SEARCH_NEXT_STATE];

    for (i = 0; i < BTA_DM_ACTIONS; i++) {
        if ((action = state_table[p_msg->event & 0x00FF][i]) != BTA_DM_SEARCH_IGNORE) {
            (*bta_dm_search_action[action])((tBTA_DM_MSG*)p_msg);
        } else {
            break;
        }
    }
    return TRUE;
}
