/*
 * BTE/btm_sco.c - the BTM SCO/eSCO links: init, connection request/response, connected/removed handlers, link
 *   removal, eSCO parameter changes and the active-link queries.
 *
 * RANGE. .text 0x804870E4-0x80487F1C (15 functions, 0xE38 B); .rodata 0x80573850-0x80573860; .data
 *   0x80614430-0x80614730. The start is pinned by the readers (BTE/btm_pm.c's header). The end is strong: the next
 *   .data starts after a 4-byte pad beyond 4-byte alignment (0x8061472C -> 0x80614730), allowing a cut in
 *   [0x80487DC4, 0x80487F1C]; the BTE source's btm_sco.c ends with btm_is_sco_active_by_bdaddr and btm_sec.c opens
 *   with BTM_SecRegister. Every .data/.rodata object is read only by this range's functions.
 * FLAGS. GC/3.0a5.2 + `cflags_nw4r`, as the btm band (BTE/btm_pm.c's FLAGS line).
 * NAMES. the map's. GUESS (dead-stripped in retail, so no row): btm_accept_sco_link, BTM_CreateSco, BTM_SetEScoMode
 *   and BTM_ReadEScoLinkParms - their bodies are reconstructed from the trace strings the .data keeps (the
 *   "TCS accept SCO", "BTM_CreateSco", "BTM_SetEScoMode" and "BTM_ReadEScoLinkParms" messages) and the BTE source;
 *   mwld strips them again. btm_send_connect_request is the static helper BTM_CreateSco and btm_sco_chk_pend_unpark
 *   share; the compiler inlines it into both.
 * RESIDUALS. BTM_ChangeEScoLinkParms: the two string operands addressed off the pool base (+0x248, +0x28C) pair
 *   under different relocation names; the code and the .data bytes are the target's. The four GUESS bodies add .text the target does not keep (mwld strips them).
 * SHAPES. plain C; btm_sco_connected caches the link state in a local and btm_sco_removed points at the first
 *   link only after the packet type restore, as the target's register use shows.
 */

#include "BTE/btm_sco.h"
#include "BTE/btm_main.h"
#include "BTE/btm_acl.h"
#include "BTE/gki_buffer.h"
#include "BTE/gki_time.h"
#include "BTE/hcicmds.h"
#include "MSL_C/alloc.h"
#include "Runtime.PPCEABI.H/memcpy.h"
#include "Runtime.PPCEABI.H/memset.h"

/* The BTM status codes. */
#define BTM_SUCCESS 0
#define BTM_CMD_STARTED 1
#define BTM_BUSY 2
#define BTM_NO_RESOURCES 3
#define BTM_WRONG_MODE 6
#define BTM_UNKNOWN_ADDR 7

/* The SCO link states. */
#define SCO_ST_UNUSED 0
#define SCO_ST_LISTENING 1
#define SCO_ST_W4_CONN_RSP 2
#define SCO_ST_CONNECTING 3
#define SCO_ST_CONNECTED 4
#define SCO_ST_DISCONNECTING 5
#define SCO_ST_PEND_UNPARK 6

/* The link types, the packet type masks and the eSCO event codes. */
#define BTM_LINK_TYPE_SCO 0
#define BTM_LINK_TYPE_ESCO 2
#define BTM_SCO_SUPPORTED_PKTS_MASK 0x003F
#define BTM_SCO_LINK_ONLY_MASK 0x0007
#define BTM_ESCO_LINK_ONLY_MASK 0x0038
#define BTM_SCO_EXCEPTION_PKTS_MASK 0x03C0
#define BTM_SCO_PKT_TYPES_MASK_EV3 0x0008
#define BTM_ESCO_2_SCO(escotype) ((u16)(((escotype) & BTM_SCO_LINK_ONLY_MASK) << 5))
#define BTM_ESCO_CHG_EVT 1
#define BTM_ESCO_CONN_REQ_EVT 2
#define BTM_DEFAULT_SCO_MODE 2
#define BTM_INVALID_SCO_DISC_REASON 0xFFFF
#define BTM_INVALID_HCI_HANDLE 0xFFFF
#define BTM_INVALID_SCO_INDEX 0xFFFF
#define BTM_64KBITS_RATE 0x00001F40

/* The HCI version that has the eSCO exception packet types (2.0) and the HCI status codes. */
#define HCI_PROTO_VERSION_2_0 3
#define HCI_SUCCESS 0
#define HCI_ERR_HOST_REJECT_RESOURCES 0x0D
#define HCI_ERR_PEER_USER 0x13
#define HCI_ERR_LMP_ERR_TRANS_COLLISION 0x23

/* The GKI pool HCI commands are built in. */
#define HCI_CMD_POOL_ID 2

/* The trace levels and trace masks of the BTM layer. */
#define BT_TRACE_LEVEL_ERROR 1
#define BT_TRACE_LEVEL_WARNING 2
#define BT_TRACE_LEVEL_API 3
#define BT_TRACE_LEVEL_EVENT 4
#define BT_TRACE_LEVEL_DEBUG 5
#define TRACE_BTM_ERROR 0x000D0000
#define TRACE_BTM_WARNING 0x000D0001
#define TRACE_BTM_API 0x000D0002
#define TRACE_BTM_EVENT 0x000D0003
#define TRACE_BTM_DEBUG 0x000D0004


/* The eSCO parameters a new link starts with: 64 kbit/s each way, 10 ms latency, CVSD, every eSCO packet type. */
static const tBTM_ESCO_PARAMS btm_esco_defaults = {
    BTM_64KBITS_RATE, BTM_64KBITS_RATE, 0x000A, 0x0060, 0x003F, 0x01,
};

/* 0x804870E4 (0x44): Initialises the SCO links and the eSCO defaults. */
void btm_sco_init(void) {
    btm_cb.sco_cb.sco_disc_reason = BTM_INVALID_SCO_DISC_REASON;
    btm_cb.sco_cb.def_esco_parms = btm_esco_defaults;
    btm_cb.sco_cb.desired_sco_mode = BTM_DEFAULT_SCO_MODE;
}

/* 0x80487128 (0x1DC): Answers an incoming (e)SCO connection request on link `sco_inx`: rejects it with `hci_status`,
 * or accepts it with the link's (or the given) eSCO parameters. */
void btm_esco_conn_rsp(u16 sco_inx, u8 hci_status, u8* bda, tBTM_ESCO_PARAMS* p_parms) {
    tSCO_CONN* p_sco = NULL;
    tBTM_ESCO_PARAMS* p_setup;
    u16 temp_pkt_types;
    BT_HDR* p_buf;

    if ((p_buf = (BT_HDR*)GKI_getpoolbuf(HCI_CMD_POOL_ID)) == NULL) {
        if (btm_cb.trace_level >= BT_TRACE_LEVEL_ERROR) {
            LogMsg_0(TRACE_BTM_ERROR, "btm_esco_conn_rsp -> No Resources");
        }
        return;
    }

    if (sco_inx < BTM_MAX_SCO_LINKS) {
        p_sco = &btm_cb.sco_cb.sco_db[sco_inx];
    }

    if (hci_status != HCI_SUCCESS) {
        if (p_sco != NULL) {
            p_sco->state = (p_sco->state == SCO_ST_W4_CONN_RSP) ? SCO_ST_LISTENING : SCO_ST_UNUSED;
        }

        if (!btm_cb.sco_cb.esco_supported) {
            btsnd_hcic_reject_conn(p_buf, bda, hci_status);
        } else {
            btsnd_hcic_reject_esco_conn(p_buf, bda, hci_status);
        }
    } else {
        btm_chg_all_acl_pkt_types(TRUE);
        p_sco->state = SCO_ST_CONNECTING;

        if (btm_cb.sco_cb.esco_supported && p_sco->esco.data.link_type == BTM_LINK_TYPE_ESCO) {
            p_setup = &p_sco->esco.setup;
            if (p_parms != NULL) {
                *p_setup = *p_parms;
            }
            p_setup->packet_types = btm_cb.sco_cb.def_esco_parms.packet_types;

            temp_pkt_types = p_setup->packet_types & BTM_SCO_SUPPORTED_PKTS_MASK & btm_cb.btm_sco_pkt_types_supported;
            if (!(temp_pkt_types & BTM_ESCO_LINK_ONLY_MASK)) {
                temp_pkt_types |= BTM_SCO_PKT_TYPES_MASK_EV3;
            }

            if (btm_cb.local_version.hciVersion >= HCI_PROTO_VERSION_2_0) {
                temp_pkt_types |=
                    (p_setup->packet_types | btm_cb.btm_sco_pkt_types_supported) & BTM_SCO_EXCEPTION_PKTS_MASK;
            }

            btsnd_hcic_accept_esco_conn(p_buf, bda, p_setup->tx_bw, p_setup->rx_bw, p_setup->max_latency,
                                        p_setup->voice_contfmt, p_setup->retrans_effort, temp_pkt_types);
            p_setup->packet_types = temp_pkt_types;
        } else {
            btsnd_hcic_accept_conn(p_buf, bda, 0);
        }
    }
}

/* Accepts the pending SCO connection request on link `sco_inx` with the default packet types (GUESS: dead-stripped
 * in retail; its trace string survives in the .data). */
void btm_accept_sco_link(u16 sco_inx) {
    if (btm_cb.trace_level >= BT_TRACE_LEVEL_DEBUG) {
        LogMsg_1(TRACE_BTM_DEBUG, "TCS accept SCO: Packet Types 0x%04x", btm_cb.sco_cb.def_esco_parms.packet_types);
    }
    btm_esco_conn_rsp(sco_inx, HCI_SUCCESS, btm_cb.sco_cb.sco_db[sco_inx].esco.data.bd_addr, NULL);
}

/* Sends the HCI command that sets up a SCO (or eSCO) link on the ACL link `acl_handle`. */
static u8 btm_send_connect_request(u16 acl_handle, tBTM_ESCO_PARAMS* p_setup) {
    u16 temp_pkt_types;

    btm_chg_all_acl_pkt_types(TRUE);

    if (!btm_cb.sco_cb.esco_supported) {
        if (!btsnd_hcic_add_SCO_conn(acl_handle, BTM_ESCO_2_SCO(p_setup->packet_types))) {
            return BTM_NO_RESOURCES;
        }
    } else {
        temp_pkt_types = p_setup->packet_types & BTM_SCO_SUPPORTED_PKTS_MASK & btm_cb.btm_sco_pkt_types_supported;

        if (btm_cb.local_version.hciVersion >= HCI_PROTO_VERSION_2_0) {
            temp_pkt_types |=
                (p_setup->packet_types | btm_cb.btm_sco_pkt_types_supported) & BTM_SCO_EXCEPTION_PKTS_MASK;
        }

        if (btm_cb.trace_level >= BT_TRACE_LEVEL_API) {
            LogMsg_6(TRACE_BTM_API, "      txbw 0x%x, rxbw 0x%x, lat 0x%x, voice 0x%x, retrans 0x%02x, pkt 0x%04x",
                     p_setup->tx_bw, p_setup->rx_bw, p_setup->max_latency, p_setup->voice_contfmt,
                     p_setup->retrans_effort, temp_pkt_types);
        }

        if (!btsnd_hcic_setup_esco_conn(acl_handle, p_setup->tx_bw, p_setup->rx_bw, p_setup->max_latency,
                                        p_setup->voice_contfmt, p_setup->retrans_effort, temp_pkt_types)) {
            return BTM_NO_RESOURCES;
        }
        p_setup->packet_types = temp_pkt_types;
    }

    return BTM_CMD_STARTED;
}

/* Starts a SCO link to `remote_bda` (or listens for one) and returns its index in `p_sco_inx` (GUESS:
 * dead-stripped in retail; its trace string survives in the .data). */
u8 BTM_CreateSco(u8* remote_bda, u8 is_orig, u16 pkt_types, u16* p_sco_inx, tBTM_SCO_CB* p_conn_cb,
                 tBTM_SCO_CB* p_disc_cb) {
    tBTM_ESCO_PARAMS* p_setup;
    tSCO_CONN* p = &btm_cb.sco_cb.sco_db[0];
    u16 xx;
    u16 acl_handle = 0;

    *p_sco_inx = BTM_INVALID_SCO_INDEX;

    if (is_orig) {
        if (remote_bda == NULL || (acl_handle = BTM_GetHCIConnHandle(remote_bda)) == BTM_INVALID_HCI_HANDLE) {
            return BTM_UNKNOWN_ADDR;
        }
    }

    for (xx = 0; xx < BTM_MAX_SCO_LINKS; xx++, p++) {
        if (p->state == SCO_ST_UNUSED) {
            if (remote_bda != NULL) {
                memcpy(p->esco.data.bd_addr, remote_bda, 6);
                p->rem_bd_known = TRUE;
            } else {
                p->rem_bd_known = FALSE;
            }

            p_setup = &p->esco.setup;
            *p_setup = btm_cb.sco_cb.def_esco_parms;
            p_setup->packet_types = pkt_types;
            p->p_conn_cb = p_conn_cb;
            p->p_disc_cb = p_disc_cb;
            p->hci_handle = BTM_INVALID_HCI_HANDLE;
            p->is_orig = is_orig;

            if (is_orig) {
                if (btm_cb.trace_level >= BT_TRACE_LEVEL_API) {
                    LogMsg_2(TRACE_BTM_API, "BTM_CreateSco -> (e)SCO Link for ACL handle 0x%04x, Desired Type %d",
                             acl_handle, btm_cb.sco_cb.desired_sco_mode);
                }
                if (btm_send_connect_request(acl_handle, p_setup) != BTM_CMD_STARTED) {
                    return BTM_NO_RESOURCES;
                }
                p->state = SCO_ST_CONNECTING;
            } else {
                p->state = SCO_ST_LISTENING;
            }

            *p_sco_inx = xx;
            return BTM_CMD_STARTED;
        }
    }

    return BTM_NO_RESOURCES;
}

/* 0x80487304 (0x18C): Sets up the SCO links that waited for the ACL link `hci_handle` to leave park mode. */
void btm_sco_chk_pend_unpark(u8 hci_status, u16 hci_handle, u8 mode) {
    u16 xx;
    u16 acl_handle;
    tSCO_CONN* p = &btm_cb.sco_cb.sco_db[0];

    for (xx = 0; xx < BTM_MAX_SCO_LINKS; xx++, p++) {
        if (p->state == SCO_ST_PEND_UNPARK && mode == 0 && hci_status == HCI_SUCCESS &&
            (acl_handle = BTM_GetHCIConnHandle(p->esco.data.bd_addr)) == hci_handle) {
            if (btm_cb.trace_level >= BT_TRACE_LEVEL_API) {
                LogMsg_2(TRACE_BTM_API, "btm_sco_chk_pend_unpark -> (e)SCO Link for ACL handle 0x%04x, Desired Type %d",
                         acl_handle, btm_cb.sco_cb.desired_sco_mode);
            }
            if (btm_send_connect_request(acl_handle, &p->esco.setup) == BTM_CMD_STARTED) {
                p->state = SCO_ST_CONNECTING;
            }
        }
    }
}

/* 0x80487490 (0x1E0): Handles an incoming SCO connection request from `bda`: hands it to the listener or eSCO client
 * that expects it, offers it to the SCO indication callback, or rejects it. */
void btm_sco_conn_req(u8* bda, u8* dev_class, u8 link_type) {
    tSCO_CONN* p = &btm_cb.sco_cb.sco_db[0];
    u16 xx;
    tBTM_ESCO_CONN_REQ_EVT_DATA evt_data;

    for (xx = 0; xx < BTM_MAX_SCO_LINKS; xx++, p++) {
        if (((p->state == SCO_ST_LISTENING && p->rem_bd_known) || p->state == SCO_ST_CONNECTING) &&
            !memcmp(p->esco.data.bd_addr, bda, 6)) {
            p->rem_bd_known = TRUE;
            p->esco.data.link_type = link_type;
            memcpy(p->esco.data.bd_addr, bda, 6);

            if (p->esco.p_esco_cback == NULL) {
                btm_esco_conn_rsp(xx, HCI_SUCCESS, bda, NULL);
                return;
            }

            memcpy(evt_data.bd_addr, bda, 6);
            memcpy(evt_data.dev_class, dev_class, 3);
            evt_data.link_type = link_type;
            p->state = SCO_ST_W4_CONN_RSP;
            evt_data.sco_inx = xx;
            p->esco.p_esco_cback(BTM_ESCO_CONN_REQ_EVT, (tBTM_ESCO_EVT_DATA*)&evt_data);
            return;
        }
    }

    if (btm_cb.sco_cb.app_sco_ind_cb != NULL) {
        for (xx = 0, p = &btm_cb.sco_cb.sco_db[0]; xx < BTM_MAX_SCO_LINKS; xx++, p++) {
            if (p->state == SCO_ST_UNUSED) {
                p->is_orig = FALSE;
                p->state = SCO_ST_LISTENING;
                p->esco.data.link_type = link_type;
                memcpy(p->esco.data.bd_addr, bda, 6);
                p->rem_bd_known = TRUE;
                break;
            }
        }
        if (xx < BTM_MAX_SCO_LINKS) {
            btm_cb.sco_cb.app_sco_ind_cb(xx);
            return;
        }
    }

    if (btm_cb.trace_level >= BT_TRACE_LEVEL_WARNING) {
        LogMsg_0(TRACE_BTM_WARNING, "btm_sco_conn_req: No one wants this SCO connection; rejecting it");
    }
    btm_esco_conn_rsp(BTM_MAX_SCO_LINKS, HCI_ERR_HOST_REJECT_RESOURCES, bda, NULL);
}

/* 0x80487670 (0x1A0): Handles the connection complete event of a SCO link to `bda`: on success records the handle
 * and the negotiated eSCO data and calls the connect callback, on failure the disconnect callback. */
void btm_sco_connected(u8 hci_status, u8* bda, u16 hci_handle, tBTM_ESCO_DATA* p_esco_data) {
    tSCO_CONN* p = &btm_cb.sco_cb.sco_db[0];
    u16 xx;
    u16 state;
    u8 spt = FALSE;
    tBTM_CHG_ESCO_PARAMS parms;

    btm_cb.sco_cb.sco_disc_reason = hci_status;

    for (xx = 0; xx < BTM_MAX_SCO_LINKS; xx++, p++) {
        state = p->state;
        if ((state == SCO_ST_CONNECTING || state == SCO_ST_LISTENING || state == SCO_ST_W4_CONN_RSP) &&
            p->rem_bd_known && (bda == NULL || !memcmp(p->esco.data.bd_addr, bda, 6))) {
            if (hci_status != HCI_SUCCESS) {
                if (state == SCO_ST_CONNECTING) {
                    if (hci_status != HCI_ERR_LMP_ERR_TRANS_COLLISION) {
                        p->state = SCO_ST_UNUSED;
                        (*p->p_disc_cb)(xx);
                    }
                } else {
                    p->state = SCO_ST_LISTENING;
                }
                return;
            }

            if (state == SCO_ST_LISTENING) {
                spt = TRUE;
            }

            p->state = SCO_ST_CONNECTED;
            p->hci_handle = hci_handle;

            if (!btm_cb.sco_cb.esco_supported) {
                p->esco.data.link_type = BTM_LINK_TYPE_SCO;
                if (spt) {
                    parms.packet_types = p->esco.setup.packet_types;
                    parms.max_latency = p->esco.setup.max_latency;
                    parms.retrans_effort = p->esco.setup.retrans_effort;
                    BTM_ChangeEScoLinkParms(xx, &parms);
                }
            } else if (p_esco_data != NULL) {
                p->esco.data = *p_esco_data;
            }

            (*p->p_conn_cb)(xx);
            return;
        }
    }
}

/* 0x80487810 (0xB8): Drops the SCO link `sco_inx`, disconnecting it when it has an HCI handle. */
u8 BTM_RemoveSco(u16 sco_inx) {
    tSCO_CONN* p = &btm_cb.sco_cb.sco_db[sco_inx];
    u16 tempstate;

    if (sco_inx >= BTM_MAX_SCO_LINKS || p->state == SCO_ST_UNUSED) {
        return BTM_UNKNOWN_ADDR;
    }

    if (p->hci_handle == BTM_INVALID_HCI_HANDLE || p->state == SCO_ST_PEND_UNPARK) {
        p->hci_handle = BTM_INVALID_HCI_HANDLE;
        p->state = SCO_ST_UNUSED;
        p->esco.p_esco_cback = NULL;
        return BTM_SUCCESS;
    }

    tempstate = p->state;
    p->state = SCO_ST_DISCONNECTING;

    if (!btsnd_hcic_disconnect(p->hci_handle, HCI_ERR_PEER_USER)) {
        p->state = tempstate;
        return BTM_NO_RESOURCES;
    }

    return BTM_CMD_STARTED;
}

/* 0x804878C8 (0x84): Drops every SCO link to `bda`. */
void btm_remove_sco_links(u8* bda) {
    tSCO_CONN* p = &btm_cb.sco_cb.sco_db[0];
    u16 xx;

    for (xx = 0; xx < BTM_MAX_SCO_LINKS; xx++, p++) {
        if (p->rem_bd_known && !memcmp(p->esco.data.bd_addr, bda, 6)) {
            BTM_RemoveSco(xx);
        }
    }
}

/* 0x8048794C (0x114): Handles the disconnection of the SCO link `hci_handle`: restores the ACL packet types when it
 * was the last one, frees the link and calls its disconnect callback. */
void btm_sco_removed(u16 hci_handle, u8 reason) {
    tSCO_CONN* p;
    u16 xx;

    btm_cb.sco_cb.sco_disc_reason = reason;

    if (btm_num_sco_links_active() <= 1) {
        btm_chg_all_acl_pkt_types(FALSE);
    }

    p = &btm_cb.sco_cb.sco_db[0];
    for (xx = 0; xx < BTM_MAX_SCO_LINKS; xx++, p++) {
        if (p->state != SCO_ST_UNUSED && p->state != SCO_ST_LISTENING && p->hci_handle == hci_handle) {
            p->state = SCO_ST_UNUSED;
            p->hci_handle = BTM_INVALID_HCI_HANDLE;
            p->rem_bd_known = FALSE;
            p->esco.p_esco_cback = NULL;
            (*p->p_disc_cb)(xx);
            return;
        }
    }
}

/* 0x80487A60 (0xB4): Forgets the SCO links to `bda` (every link for NULL) after its ACL link went down. */
void btm_sco_acl_removed(u8* bda) {
    tSCO_CONN* p = &btm_cb.sco_cb.sco_db[0];
    u16 xx;

    for (xx = 0; xx < BTM_MAX_SCO_LINKS; xx++, p++) {
        if (p->state != SCO_ST_UNUSED) {
            if (bda == NULL || (!memcmp(p->esco.data.bd_addr, bda, 6) && p->rem_bd_known)) {
                p->state = SCO_ST_UNUSED;
                p->esco.p_esco_cback = NULL;
                (*p->p_disc_cb)(xx);
            }
        }
    }
}

/* 0x80487B14 (0x4): Drops a SCO data packet from the controller (SCO data is not routed over HCI). */
void btm_route_sco_data(BT_HDR* p_msg) {
    GKI_freebuf(p_msg);
}

/* Sets the default eSCO parameters and the link type new links ask for (GUESS: dead-stripped in retail; its trace
 * strings survive in the .data). */
u8 BTM_SetEScoMode(u8 sco_mode, tBTM_ESCO_PARAMS* p_parms) {
    tSCO_CB* p_esco = &btm_cb.sco_cb;
    tBTM_ESCO_PARAMS* p_def = &p_esco->def_esco_parms;

    if (p_esco->esco_supported) {
        if (p_parms != NULL) {
            if (sco_mode == BTM_LINK_TYPE_ESCO) {
                *p_def = *p_parms;
            } else {
                p_def->packet_types = p_parms->packet_types;
                p_def->tx_bw = BTM_64KBITS_RATE;
                p_def->rx_bw = BTM_64KBITS_RATE;
                p_def->max_latency = 0x000A;
                p_def->voice_contfmt = 0x0060;
                p_def->retrans_effort = 0;
                if (btm_cb.local_version.hciVersion >= HCI_PROTO_VERSION_2_0) {
                    p_def->packet_types |= BTM_SCO_EXCEPTION_PKTS_MASK;
                }
            }
        }
        p_esco->desired_sco_mode = sco_mode;
        if (btm_cb.trace_level >= BT_TRACE_LEVEL_API) {
            LogMsg_1(TRACE_BTM_API, "BTM_SetEScoMode -> mode %d", sco_mode);
        }
    } else {
        p_esco->desired_sco_mode = BTM_LINK_TYPE_SCO;
        p_def->packet_types &= BTM_SCO_LINK_ONLY_MASK;
        p_def->retrans_effort = 0;
        if (btm_cb.trace_level >= BT_TRACE_LEVEL_API) {
            LogMsg_0(TRACE_BTM_API, "BTM_SetEScoMode -> mode SCO (eSCO not supported)");
        }
    }

    if (btm_cb.trace_level >= BT_TRACE_LEVEL_DEBUG) {
        LogMsg_6(TRACE_BTM_DEBUG,
                 "    txbw 0x%08x, rxbw 0x%08x, max_lat 0x%04x, voice 0x%04x, pkt 0x%04x, rtx effort 0x%02x",
                 p_def->tx_bw, p_def->rx_bw, p_def->max_latency, p_def->voice_contfmt, p_def->packet_types,
                 p_def->retrans_effort);
    }

    return BTM_SUCCESS;
}

/* Copies the negotiated eSCO data of the link `sco_inx` to `p_parms` (GUESS: dead-stripped in retail; its trace
 * string survives in the .data). */
u8 BTM_ReadEScoLinkParms(u16 sco_inx, tBTM_ESCO_DATA* p_parms) {
    if (btm_cb.trace_level >= BT_TRACE_LEVEL_API) {
        LogMsg_1(TRACE_BTM_API, "BTM_ReadEScoLinkParms -> sco_inx 0x%04x", sco_inx);
    }

    if (sco_inx < BTM_MAX_SCO_LINKS && btm_cb.sco_cb.sco_db[sco_inx].state >= SCO_ST_CONNECTED) {
        *p_parms = btm_cb.sco_cb.sco_db[sco_inx].esco.data;
        return BTM_SUCCESS;
    }

    memset(p_parms, 0, sizeof(tBTM_ESCO_DATA));
    return BTM_WRONG_MODE;
}

/* 0x80487B18 (0x1A8): Changes the packet types, latency and retransmission effort of the connected link `sco_inx`. */
u8 BTM_ChangeEScoLinkParms(u16 sco_inx, tBTM_CHG_ESCO_PARAMS* p_parms) {
    tBTM_ESCO_PARAMS* p_setup;
    tSCO_CONN* p_sco;
    u16 temp_pkt_types;

    if (sco_inx >= BTM_MAX_SCO_LINKS || btm_cb.sco_cb.sco_db[sco_inx].state != SCO_ST_CONNECTED) {
        return BTM_WRONG_MODE;
    }

    p_sco = &btm_cb.sco_cb.sco_db[sco_inx];
    p_setup = &p_sco->esco.setup;

    if (p_sco->esco.data.link_type == BTM_LINK_TYPE_SCO || !btm_cb.sco_cb.esco_supported) {
        p_setup->packet_types = p_parms->packet_types & (btm_cb.btm_sco_pkt_types_supported & BTM_SCO_LINK_ONLY_MASK);

        if (btm_cb.trace_level >= BT_TRACE_LEVEL_API) {
            LogMsg_2(TRACE_BTM_API, "BTM_ChangeEScoLinkParms -> SCO Link for handle 0x%04x, pkt 0x%04x",
                     p_sco->hci_handle, p_setup->packet_types);
        }

        if (!btsnd_hcic_change_conn_type(p_sco->hci_handle, BTM_ESCO_2_SCO(p_setup->packet_types))) {
            return BTM_NO_RESOURCES;
        }
    } else {
        temp_pkt_types = p_parms->packet_types & BTM_SCO_SUPPORTED_PKTS_MASK & btm_cb.btm_sco_pkt_types_supported;

        if (btm_cb.local_version.hciVersion >= HCI_PROTO_VERSION_2_0) {
            temp_pkt_types |=
                (p_parms->packet_types | btm_cb.btm_sco_pkt_types_supported) & BTM_SCO_EXCEPTION_PKTS_MASK;
        }

        if (btm_cb.trace_level >= BT_TRACE_LEVEL_API) {
            LogMsg_1(TRACE_BTM_API, "BTM_ChangeEScoLinkParms -> eSCO Link for handle 0x%04x", p_sco->hci_handle);
        }
        if (btm_cb.trace_level >= BT_TRACE_LEVEL_API) {
            LogMsg_6(TRACE_BTM_API, "      txbw 0x%x, rxbw 0x%x, lat 0x%x, voice 0x%x, retrans 0x%02x, pkt 0x%04x",
                     p_setup->tx_bw, p_setup->rx_bw, p_parms->max_latency, p_setup->voice_contfmt,
                     p_parms->retrans_effort, temp_pkt_types);
        }

        if (!btsnd_hcic_setup_esco_conn(p_sco->hci_handle, p_setup->tx_bw, p_setup->rx_bw, p_parms->max_latency,
                                        p_setup->voice_contfmt, p_parms->retrans_effort, temp_pkt_types)) {
            return BTM_NO_RESOURCES;
        }
        p_parms->packet_types = temp_pkt_types;
    }

    return BTM_CMD_STARTED;
}

/* 0x80487CC0 (0x104): Handles an eSCO connection change event: records the new values and tells the link's eSCO
 * client. */
void btm_esco_proc_conn_chg(u8 status, u16 handle, u8 tx_interval, u8 retrans_window, u16 rx_pkt_len,
                            u16 tx_pkt_len) {
    tSCO_CONN* p = &btm_cb.sco_cb.sco_db[0];
    tBTM_CHG_ESCO_EVT_DATA data;
    u16 xx;

    if (btm_cb.trace_level >= BT_TRACE_LEVEL_EVENT) {
        LogMsg_2(TRACE_BTM_EVENT, "btm_esco_proc_conn_chg -> handle 0x%04x, status 0x%02x", handle, status);
    }

    for (xx = 0; xx < BTM_MAX_SCO_LINKS; xx++, p++) {
        if (p->state == SCO_ST_CONNECTED && handle == p->hci_handle) {
            if (p->esco.p_esco_cback != NULL) {
                memcpy(data.bd_addr, p->esco.data.bd_addr, 6);
                data.hci_status = status;
                data.sco_inx = xx;
                data.rx_pkt_len = p->esco.data.rx_pkt_len = rx_pkt_len;
                data.tx_pkt_len = p->esco.data.tx_pkt_len = tx_pkt_len;
                data.tx_interval = p->esco.data.tx_interval = tx_interval;
                data.retrans_window = p->esco.data.retrans_window = retrans_window;

                (*p->esco.p_esco_cback)(BTM_ESCO_CHG_EVT, (tBTM_ESCO_EVT_DATA*)&data);
            }
            return;
        }
    }
}

/* 0x80487DC4 (0x70): Whether the SCO link `handle` is up. */
u8 btm_is_sco_active(u16 handle) {
    u16 xx;
    tSCO_CONN* p = &btm_cb.sco_cb.sco_db[0];

    for (xx = 0; xx < BTM_MAX_SCO_LINKS; xx++, p++) {
        if (handle == p->hci_handle && p->state == SCO_ST_CONNECTED) {
            return TRUE;
        }
    }
    return FALSE;
}

/* 0x80487E34 (0x60): The number of SCO links connecting, up or disconnecting. */
u8 btm_num_sco_links_active(void) {
    int xx;
    tSCO_CONN* p = &btm_cb.sco_cb.sco_db[0];
    u8 num_scos = 0;

    for (xx = 0; xx < BTM_MAX_SCO_LINKS; xx++, p++) {
        switch (p->state) {
        case SCO_ST_W4_CONN_RSP:
        case SCO_ST_CONNECTING:
        case SCO_ST_CONNECTED:
        case SCO_ST_DISCONNECTING:
        case SCO_ST_PEND_UNPARK:
            num_scos++;
        }
    }
    return num_scos;
}

/* 0x80487E94 (0x88): Whether a SCO link to `remote_bda` is up. */
u8 btm_is_sco_active_by_bdaddr(u8* remote_bda) {
    u8 xx;
    tSCO_CONN* p = &btm_cb.sco_cb.sco_db[0];

    for (xx = 0; xx < BTM_MAX_SCO_LINKS; xx++, p++) {
        if (memcmp(p->esco.data.bd_addr, remote_bda, 6) == 0 && p->state == SCO_ST_CONNECTED) {
            return TRUE;
        }
    }
    return FALSE;
}
