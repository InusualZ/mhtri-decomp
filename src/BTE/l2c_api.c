/*
 * BTE/l2c_api.c - the L2CAP API: L2CA_Register/Deregister, connect/config/disconnect requests and responses, data
 *   write, the idle timeouts and the trace level.
 *
 * RANGE. .text 0x80492E94-0x80493A60 (13 functions, 0xBCC B); .data 0x80615AE0-0x80616270. Strong: the .data starts
 *   after a 4-byte pad beyond 4-byte alignment (0x80615ADC -> 0x80615AE0), pinning the start to [0x80492E68,
 *   0x80492E94] - L2CA_Register, after hidh_conn_dereg. It ends with an empty 4-byte function at 0x80493A5C (called by
 *   bta_dm_enable); l2c_csm.c opens with l2c_csm_execute at 0x80493A60. Medium confidence on the empty function's side:
 *   the .data allows any cut in [0x80493940, 0x80493A60], and only the BTE source order (l2c_csm.c's first function is
 *   l2c_csm_execute) puts it here.
 * FLAGS. GC/3.0a5.2 with `cflags_nw4r`, per object in configure.py; the evidence is on BTE/l2c_utils.c's FLAGS
 *   line. The function starts of this band are packed to 4 bytes.
 * NAMES. the BTE source's. GUESS: 0x80493A5C L2CA_RegisterCompression (empty; bta_dm_enable passes it
 *   bta_dm_l2cap_server_compress_cback and 0, and this unit's .data keeps the L2CA_SetCompression format).
 * RESIDUALS. L2CA_Ping, L2CA_SetAclPriority, L2CA_SetCompression, L2CA_Flush and L2CA_GetNumQueuedBufs are
 *   dead-stripped in retail - only their format strings remain in the .data, between L2CA_DataWrite's and the
 *   end - so their bodies are reconstructed from the BTE source around those strings and the link strips them;
 *   L2CA_SetCompression's body is a GUESS beyond its trace. flipcheck refuses on the object's larger .text (the
 *   stripped bodies) and on LogMsg_6, BTM_IsDeviceUp and BT_BD_ANY: those map rows are still generated (requests
 *   bte-l2cap#2/#3/#7/#8), so a flip links against undefined symbols. With those renames applied locally a trial
 *   flip links byte-identically (the stripped bodies drop out).
 * SHAPES. plain C.
 */

#include "BTE/l2c_csm.h"
#include "BTE/l2c_link.h"
#include "BTE/l2c_main.h"
#include "BTE/l2c_utils.h"
#include "MSL_C/alloc.h"

/* The PSM check of a registration: a valid PSM is odd and has bit 8 clear. */
#define L2C_INVALID_PSM(psm) (((psm) & 0x0101) != 0x0001)

/* The callback L2CA_RegisterCompression takes. */
typedef void (tL2CA_COMPRESS_CB)(void);

/* 0x80492E94 (0x19C): Registers (or re-registers) the callbacks of an L2CAP client for `psm`. */
u16 L2CA_Register(u16 psm, tL2CAP_APPL_INFO* p_cb_info) {
    tL2C_RCB* p_rcb;

    L2CAP_TRACE_API1("L2CAP - L2CA_Register() called for PSM: 0x%04x", psm);

    if (!p_cb_info->pL2CA_ConfigCfm_Cb || !p_cb_info->pL2CA_ConfigInd_Cb || !p_cb_info->pL2CA_DataInd_Cb ||
        !p_cb_info->pL2CA_DisconnectInd_Cb) {
        L2CAP_TRACE_ERROR1("L2CAP - no cb registering PSM: 0x%04x", psm);
        return FALSE;
    }

    if (L2C_INVALID_PSM(psm)) {
        L2CAP_TRACE_ERROR1("L2CAP - invalid PSM value, PSM: 0x%04x", psm);
        return FALSE;
    }

    if ((p_rcb = l2cu_find_rcb_by_psm(psm)) == NULL) {
        if ((p_rcb = l2cu_allocate_rcb(psm)) == NULL) {
            L2CAP_TRACE_WARNING1("L2CAP - no RCB available, PSM: 0x%04x", psm);
            return FALSE;
        }
    }

    p_rcb->api = *p_cb_info;

    return TRUE;
}

/* 0x80493030 (0x90): Frees the registration of `psm`. */
void L2CA_Deregister(u16 psm) {
    tL2C_RCB* p_rcb;

    L2CAP_TRACE_API1("L2CAP - L2CA_Deregister() called for PSM: 0x%04x", psm);

    if ((p_rcb = l2cu_find_rcb_by_psm(psm)) != NULL) {
        l2cu_release_rcb(p_rcb);
    } else {
        L2CAP_TRACE_WARNING1("L2CAP - PSM: 0x%04x not found for deregistration", psm);
    }
}

/* 0x804930C0 (0x234): Opens a channel to `psm` on the peer at `p_bd_addr`, paging it first; returns the local CID. */
u16 L2CA_ConnectReq(u16 psm, u8* p_bd_addr) {
    tL2C_LCB* p_lcb;
    tL2C_CCB* p_ccb;
    tL2C_RCB* p_rcb;

    L2CAP_TRACE_API1("L2CA_ConnectReq()  PSM: 0x%04x", psm);
    L2CAP_TRACE_API6("L2CA_ConnectReq()  BDA: %02x-%02x-%02x-%02x-%02x-%02x", p_bd_addr[0], p_bd_addr[1], p_bd_addr[2],
                     p_bd_addr[3], p_bd_addr[4], p_bd_addr[5]);

    if (!BTM_IsDeviceUp()) {
        L2CAP_TRACE_WARNING0("L2CAP connect req - BTU not ready");
        return 0;
    }

    if ((p_rcb = l2cu_find_rcb_by_psm(psm)) == NULL) {
        L2CAP_TRACE_WARNING1("L2CAP - no RCB for L2CA_conn_req, PSM: 0x%04x", psm);
        return 0;
    }

    if ((p_lcb = l2cu_find_lcb_by_bd_addr(p_bd_addr)) == NULL) {
        if ((p_lcb = l2cu_allocate_lcb(p_bd_addr)) == NULL) {
            L2CAP_TRACE_WARNING0("L2CAP - no LCB for L2CA_conn_req");
            return 0;
        }

        if (!l2cu_create_conn(p_lcb)) {
            return 0;
        }
    }

    if (p_lcb->link_state == LST_DISCONNECTING) {
        L2CAP_TRACE_API0("L2CAP API - L2CA_conn_req rejected - link disconnecting");
        return 0;
    }

    if ((p_ccb = l2cu_allocate_ccb(p_lcb)) == NULL) {
        L2CAP_TRACE_WARNING0("L2CAP - no CCB for L2CA_conn_req");
        return 0;
    }

    p_ccb->p_rcb = p_rcb;

    if (p_lcb->link_state == LST_CONNECTED) {
        l2c_csm_execute(p_ccb, L2CEVT_L2CA_CONNECT_REQ, NULL);
    }

    L2CAP_TRACE_API1("L2CAP - L2CA_conn_req() returned CID: 0x%04x", p_ccb->local_cid);

    return p_ccb->local_cid;
}

/* 0x804932F4 (0x190): Answers an incoming connection on `lcid` (accept, pending or refuse). */
u8 L2CA_ConnectRsp(u8* p_bd_addr, u8 id, u16 lcid, u16 result, u16 status) {
    tL2C_LCB* p_lcb;
    tL2C_CCB* p_ccb;
    tL2C_CONN_INFO conn_info;

    L2CAP_TRACE_API3("L2CA_ConnectRsp()  CID: 0x%04x  Result: %d  Status: %d", lcid, result, status);
    L2CAP_TRACE_API6("L2CA_ConnectRsp()  BDA: %02x-%02x-%02x-%02x-%02x-%02x", p_bd_addr[0], p_bd_addr[1], p_bd_addr[2],
                     p_bd_addr[3], p_bd_addr[4], p_bd_addr[5]);

    if ((p_lcb = l2cu_find_lcb_by_bd_addr(p_bd_addr)) == NULL) {
        L2CAP_TRACE_WARNING0("L2CAP - no LCB for L2CA_conn_rsp");
        return FALSE;
    }

    if ((p_ccb = l2cu_find_ccb_by_cid(p_lcb, lcid)) == NULL) {
        L2CAP_TRACE_WARNING0("L2CAP - no CCB for L2CA_conn_rsp");
        return FALSE;
    }

    if (p_ccb->remote_id != id) {
        L2CAP_TRACE_WARNING2("L2CAP - bad id in L2CA_conn_rsp. Exp: %d  Got: %d", p_ccb->remote_id, id);
        return FALSE;
    }

    if (result == L2CAP_CONN_OK) {
        l2c_csm_execute(p_ccb, L2CEVT_L2CA_CONNECT_RSP, NULL);
    } else {
        conn_info.l2cap_result = result;
        conn_info.l2cap_status = status;

        if (result == L2CAP_CONN_PENDING) {
            l2c_csm_execute(p_ccb, L2CEVT_L2CA_CONNECT_RSP, &conn_info);
        } else {
            l2c_csm_execute(p_ccb, L2CEVT_L2CA_CONNECT_RSP_NEG, &conn_info);
        }
    }

    return TRUE;
}

/* 0x80493484 (0xB8): Starts the configuration of `cid` with our options. */
u8 L2CA_ConfigReq(u16 cid, tL2CAP_CFG_INFO* p_cfg) {
    tL2C_CCB* p_ccb;

    L2CAP_TRACE_API1("L2CA_ConfigReq()  CID: 0x%04x", cid);

    if ((p_ccb = l2cu_find_ccb_by_cid(NULL, cid)) == NULL) {
        L2CAP_TRACE_WARNING1("L2CAP - no CCB for L2CA_cfg_req, CID: %d", cid);
        return FALSE;
    }

    p_cfg->fcr_present = FALSE;

    l2c_csm_execute(p_ccb, L2CEVT_L2CA_CONFIG_REQ, p_cfg);

    return TRUE;
}

/* 0x8049353C (0xD0): Answers the peer's configuration of `cid`. */
u8 L2CA_ConfigRsp(u16 cid, tL2CAP_CFG_INFO* p_cfg) {
    tL2C_CCB* p_ccb;

    L2CAP_TRACE_API2("L2CA_ConfigRsp()  CID: 0x%04x  Result: %d", cid, p_cfg->result);

    if ((p_ccb = l2cu_find_ccb_by_cid(NULL, cid)) == NULL) {
        L2CAP_TRACE_WARNING1("L2CAP - no CCB for L2CA_cfg_rsp, CID: %d", cid);
        return FALSE;
    }

    if (p_cfg->result == L2CAP_CFG_OK) {
        l2c_csm_execute(p_ccb, L2CEVT_L2CA_CONFIG_RSP, p_cfg);
    } else {
        l2c_csm_execute(p_ccb, L2CEVT_L2CA_CONFIG_RSP_NEG, p_cfg);
    }

    return TRUE;
}

/* 0x8049360C (0xA4): Starts closing `cid`. */
u8 L2CA_DisconnectReq(u16 cid) {
    tL2C_CCB* p_ccb;

    L2CAP_TRACE_API1("L2CA_DisconnectReq()  CID: 0x%04x", cid);

    if ((p_ccb = l2cu_find_ccb_by_cid(NULL, cid)) == NULL) {
        L2CAP_TRACE_WARNING1("L2CAP - no CCB for L2CA_disc_req, CID: %d", cid);
        return FALSE;
    }

    l2c_csm_execute(p_ccb, L2CEVT_L2CA_DISCONNECT_REQ, NULL);

    return TRUE;
}

/* 0x804936B0 (0xA4): Confirms the peer's disconnection of `cid`. */
u8 L2CA_DisconnectRsp(u16 cid) {
    tL2C_CCB* p_ccb;

    L2CAP_TRACE_API1("L2CA_DisconnectRsp()  CID: 0x%04x", cid);

    if ((p_ccb = l2cu_find_ccb_by_cid(NULL, cid)) == NULL) {
        L2CAP_TRACE_WARNING1("L2CAP - no CCB for L2CA_disc_rsp, CID: %d", cid);
        return FALSE;
    }

    l2c_csm_execute(p_ccb, L2CEVT_L2CA_DISCONNECT_RSP, NULL);

    return TRUE;
}

/* 0x80493754 (0x124): Sends `p_data` on `cid`; returns whether it was taken and whether the link is congested. */
u8 L2CA_DataWrite(u16 cid, BT_HDR* p_data) {
    tL2C_CCB* p_ccb;

    L2CAP_TRACE_API2("L2CA_DataWrite()  CID: 0x%04x  Len: %d", cid, p_data->len);

    if ((p_ccb = l2cu_find_ccb_by_cid(NULL, cid)) == NULL) {
        L2CAP_TRACE_WARNING1("L2CAP - no CCB for L2CA_DataWrite, CID: %d", cid);
        GKI_freebuf(p_data);
        return L2CAP_DW_FAILED;
    }

    if (p_data->len <= p_ccb->peer_mtu) {
        l2c_csm_execute(p_ccb, L2CEVT_L2CA_DATA_WRITE, p_data);

        if (p_ccb->p_lcb->link_congested) {
            return L2CAP_DW_CONGESTED;
        }
        return L2CAP_DW_SUCCESS;
    } else {
        L2CAP_TRACE_WARNING0("L2CAP - cannot send message bigger than peer's mtu size");
        GKI_freebuf(p_data);
        return L2CAP_DW_FAILED;
    }
}

/* Dead-stripped in retail: pings the peer at `p_bd_addr`, paging it first; `p_callback` gets the result. */
u8 L2CA_Ping(u8* p_bd_addr, tL2CA_ECHO_RSP_CB* p_callback) {
    tL2C_LCB* p_lcb;

    L2CAP_TRACE_API6("L2CA_Ping()  BDA: %02x-%02x-%02x-%02x-%02x-%02x", p_bd_addr[0], p_bd_addr[1], p_bd_addr[2],
                     p_bd_addr[3], p_bd_addr[4], p_bd_addr[5]);

    if (!BTM_IsDeviceUp()) {
        return FALSE;
    }

    if ((p_lcb = l2cu_find_lcb_by_bd_addr(p_bd_addr)) == NULL) {
        if ((p_lcb = l2cu_allocate_lcb(p_bd_addr)) == NULL) {
            L2CAP_TRACE_WARNING0("L2CAP - no LCB for L2CA_ping");
            return FALSE;
        }

        if (!l2cu_create_conn(p_lcb)) {
            return FALSE;
        }

        p_lcb->p_echo_rsp_cb = p_callback;
        return TRUE;
    }

    if (p_lcb->p_echo_rsp_cb != NULL) {
        L2CAP_TRACE_WARNING0("L2CAP - rejected second L2CA_ping");
        return FALSE;
    }

    if (p_lcb->link_state == LST_DISCONNECTING) {
        L2CAP_TRACE_WARNING0("L2CAP - L2CA_ping rejected - link disconnecting");
        return FALSE;
    }

    p_lcb->p_echo_rsp_cb = p_callback;

    if (p_lcb->link_state == LST_CONNECTED) {
        l2cu_send_peer_echo_req(p_lcb, NULL, 0);
        btu_start_timer(&p_lcb->timer_entry, BTU_TTYPE_L2CAP_LINK, L2CAP_ECHO_RSP_TOUT);
    }

    return TRUE;
}

/* 0x80493878 (0xC8): Sets the idle timeout of the link under `cid`, or the default of every new link. */
u8 L2CA_SetIdleTimeout(u16 cid, u16 timeout, u8 is_global) {
    tL2C_CCB* p_ccb;
    tL2C_LCB* p_lcb;

    if (is_global) {
        l2cb.idle_timeout = timeout;
    } else {
        if ((p_ccb = l2cu_find_ccb_by_cid(NULL, cid)) == NULL) {
            L2CAP_TRACE_WARNING1("L2CAP - no CCB for L2CA_SetIdleTimeout, CID: %d", cid);
            return FALSE;
        }

        p_lcb = p_ccb->p_lcb;

        if (p_lcb && p_lcb->in_use && p_lcb->link_state == LST_CONNECTED) {
            p_lcb->idle_timeout = timeout;
        } else {
            return FALSE;
        }
    }

    return TRUE;
}

/* 0x80493940 (0x100): Sets the idle timeout of the link to `bd_addr`, or of every connected link for BT_BD_ANY. */
u8 L2CA_SetIdleTimeoutByBdAddr(u8* bd_addr, u16 timeout) {
    tL2C_LCB* p_lcb;
    int xx;

    if (memcmp(BT_BD_ANY, bd_addr, 6)) {
        p_lcb = l2cu_find_lcb_by_bd_addr(bd_addr);

        if (p_lcb && p_lcb->in_use && p_lcb->link_state == LST_CONNECTED) {
            p_lcb->idle_timeout = timeout;
        } else {
            return FALSE;
        }
    } else {
        p_lcb = &l2cb.lcb_pool[0];

        for (xx = 0; xx < MAX_L2CAP_LINKS; xx++, p_lcb++) {
            if (p_lcb->in_use && p_lcb->link_state == LST_CONNECTED) {
                p_lcb->idle_timeout = timeout;
            }
        }
    }

    return TRUE;
}

/* 0x80493A40 (0x1C): Sets the L2CAP trace level (0xFF keeps it) and returns it. */
u8 L2CA_SetTraceLevel(u8 new_level) {
    if (new_level != 0xFF) {
        l2cb.l2cb_trace_level = new_level;
    }

    return l2cb.l2cb_trace_level;
}

/* Dead-stripped in retail: sets the ACL priority of the link to `bd_addr` and rebalances the buffers. */
u8 L2CA_SetAclPriority(u8* bd_addr, u8 priority) {
    tL2C_LCB* p_lcb;

    L2CAP_TRACE_API6("L2CA_SetAclPriority()  bdaddr: %02x%02x%02x%02x%02x%02x", bd_addr[0], bd_addr[1], bd_addr[2],
                     bd_addr[3], bd_addr[4], bd_addr[5]);

    if ((p_lcb = l2cu_find_lcb_by_bd_addr(bd_addr)) == NULL) {
        L2CAP_TRACE_WARNING0("L2CAP - no LCB for L2CA_SetAclPriority");
        return FALSE;
    }

    if (p_lcb->acl_priority != priority) {
        p_lcb->acl_priority = priority;
        l2c_link_adjust_allocation();
    }

    return TRUE;
}

/* Dead-stripped in retail: asks for compression on `local_cid`; this build has none. */
u8 L2CA_SetCompression(u16 local_cid, u8 direction, u8 pe_type, u8 mem_level, u8 wbits, u8 enable) {
    L2CAP_TRACE_API6("L2CA_SetCompression() local cid %d, direction %d, pe_type %d, mem_level %d, wbits %d, enable %d",
                     local_cid, direction, pe_type, mem_level, wbits, enable);

    return FALSE;
}

/* Dead-stripped in retail: drops the data queued on `cid`; returns how many buffers it freed. */
u16 L2CA_Flush(u16 cid) {
    tL2C_CCB* p_ccb;
    u16 num_flushed = 0;

    if ((p_ccb = l2cu_find_ccb_by_cid(NULL, cid)) == NULL) {
        L2CAP_TRACE_WARNING1("L2CAP - no CCB for L2CA_Flush, CID: %d", cid);
        return 0;
    }

    while (p_ccb->xmit_hold_q.count) {
        GKI_freebuf(GKI_dequeue(&p_ccb->xmit_hold_q));
        num_flushed++;
    }

    L2CAP_TRACE_API2("L2CA_Flush()  CID: 0x%04x flushed %d buffers", cid, num_flushed);

    return num_flushed;
}

/* Dead-stripped in retail: returns how many buffers wait on `lcid`. */
u16 L2CA_GetNumQueuedBufs(u16 lcid) {
    tL2C_CCB* p_ccb;

    if ((p_ccb = l2cu_find_ccb_by_cid(NULL, lcid)) == NULL) {
        L2CAP_TRACE_WARNING1("L2CA_GetNumQueuedBufs()  CID: 0x%04x  abmormally returning 0", lcid);
        return 0;
    }

    L2CAP_TRACE_API2("L2CA_GetNumQueuedBufs()  CID: 0x%04x  returning %d", lcid, p_ccb->xmit_hold_q.count);

    return p_ccb->xmit_hold_q.count;
}

/* 0x80493A5C (0x4): Takes the compression callback of a server; this build has no compression. */
void L2CA_RegisterCompression(tL2CA_COMPRESS_CB* p_cb, u8 enable) {
}
