/* BTE/btm_sco.h - the SCO link entry points `BTE/btm_sco.c` owns that other units call, and the eSCO parameter change
 *   request they take, and the SCO records of the control block (docs/plan.md 6.5 rule 2). */
#ifndef MHTRI_BTE_BTM_SCO_H
#define MHTRI_BTE_BTM_SCO_H

#include "types.h"
#include "BTE/gki_buffer.h"

/* The SCO links the control block holds. */
#define BTM_MAX_SCO_LINKS 3

/* size: 0x10 - the eSCO link parameters. */
typedef struct tBTM_ESCO_PARAMS {
    /* +0x00 */ u32 tx_bw;
    /* +0x04 */ u32 rx_bw;
    /* +0x08 */ u16 max_latency;
    /* +0x0A */ u16 voice_contfmt;
    /* +0x0C */ u16 packet_types;
    /* +0x0E */ u8 retrans_effort;
    /* +0x0F */ u8 pad_0x0F;
} tBTM_ESCO_PARAMS; /* size: 0x10 */

/* size: 0x0E - the eSCO link's negotiated values and peer, as the connection complete event reports them. */
typedef struct tBTM_ESCO_DATA {
    /* +0x00 */ u16 rx_pkt_len;
    /* +0x02 */ u16 tx_pkt_len;
    /* +0x04 */ u8 bd_addr[6];
    /* +0x0A */ u8 link_type;
    /* +0x0B */ u8 tx_interval;
    /* +0x0C */ u8 retrans_window;
    /* +0x0D */ u8 air_mode;
} tBTM_ESCO_DATA; /* size: 0x0E */

/* size: 0x10 - an eSCO link change the eSCO callback reports. */
typedef struct tBTM_CHG_ESCO_EVT_DATA {
    /* +0x00 */ u16 sco_inx;
    /* +0x02 */ u16 rx_pkt_len;
    /* +0x04 */ u16 tx_pkt_len;
    /* +0x06 */ u8 bd_addr[6];
    /* +0x0C */ u8 hci_status;
    /* +0x0D */ u8 tx_interval;
    /* +0x0E */ u8 retrans_window;
    /* +0x0F */ u8 pad_0x0F;
} tBTM_CHG_ESCO_EVT_DATA; /* size: 0x10 */

/* size: 0x0C - an incoming (e)SCO connection request the eSCO callback reports. */
typedef struct tBTM_ESCO_CONN_REQ_EVT_DATA {
    /* +0x00 */ u16 sco_inx;
    /* +0x02 */ u8 bd_addr[6];
    /* +0x08 */ u8 dev_class[3];
    /* +0x0B */ u8 link_type;
} tBTM_ESCO_CONN_REQ_EVT_DATA; /* size: 0x0C */

/* size: 0x10 - the payload of an eSCO event. */
typedef union tBTM_ESCO_EVT_DATA {
    /* +0x00 */ tBTM_CHG_ESCO_EVT_DATA chg_evt;
    /* +0x00 */ tBTM_ESCO_CONN_REQ_EVT_DATA conn_evt;
} tBTM_ESCO_EVT_DATA; /* size: 0x10 */

/* The SCO connect/disconnect callback, the eSCO event callback and the SCO indication callback. */
typedef void (tBTM_SCO_CB)(u16 sco_inx);
typedef void (tBTM_ESCO_CBACK)(u8 event, tBTM_ESCO_EVT_DATA* p_data);
typedef void (tBTM_SCO_IND_CBACK)(u16 sco_inx);

/* size: 0x24 - the eSCO state of a SCO link: its event callback, the parameters asked for and what was negotiated. */
typedef struct tBTM_ESCO_INFO {
    /* +0x00 */ tBTM_ESCO_CBACK* p_esco_cback;
    /* +0x04 */ tBTM_ESCO_PARAMS setup;
    /* +0x14 */ tBTM_ESCO_DATA data;
    /* +0x22 */ u8 hci_status;
    /* +0x23 */ u8 pad_0x23;
} tBTM_ESCO_INFO; /* size: 0x24 */

/* size: 0x34 - a SCO link. */
typedef struct tSCO_CONN {
    /* +0x00 */ tBTM_SCO_CB* p_conn_cb;
    /* +0x04 */ tBTM_SCO_CB* p_disc_cb;
    /* +0x08 */ u16 state;
    /* +0x0A */ u16 hci_handle;
    /* +0x0C */ u8 is_orig;
    /* +0x0D */ u8 rem_bd_known;
    /* +0x0E */ u8 pad_0x0E[2];
    /* +0x10 */ tBTM_ESCO_INFO esco;
} tSCO_CONN; /* size: 0x34 */

/* size: 0xBC - the SCO links and the eSCO defaults. */
typedef struct tSCO_CB {
    /* +0x00 */ tBTM_SCO_IND_CBACK* app_sco_ind_cb;
    /* +0x04 */ tSCO_CONN sco_db[BTM_MAX_SCO_LINKS];
    /* +0xA0 */ u8 unused_0xA0[6];
    /* +0xA6 */ u16 sco_disc_reason;
    /* +0xA8 */ tBTM_ESCO_PARAMS def_esco_parms;
    /* +0xB8 */ u8 esco_supported;
    /* +0xB9 */ u8 desired_sco_mode;
    /* +0xBA */ u8 pad_0xBA[2];
} tSCO_CB; /* size: 0xBC */

/* size: 0x06 - the eSCO parameters a connected link may change. */
typedef struct tBTM_CHG_ESCO_PARAMS {
    /* +0x00 */ u16 max_latency;
    /* +0x02 */ u16 packet_types;
    /* +0x04 */ u8 retrans_effort;
    /* +0x05 */ u8 pad_0x05;
} tBTM_CHG_ESCO_PARAMS; /* size: 0x06 */

#ifdef __cplusplus
extern "C" {
#endif

/* 0x804870E4 - initialises the SCO links and the eSCO defaults. */
void btm_sco_init(void);

/* 0x80487128 - answers an incoming (e)SCO connection request on link `sco_inx`. */
void btm_esco_conn_rsp(u16 sco_inx, u8 hci_status, u8* bda, tBTM_ESCO_PARAMS* p_parms);

/* 0x80487304 / 0x80487490 / 0x80487670 - the HCI event handlers: an ACL link left park mode, a peer asks for a SCO
 * link, a SCO link connected. */
void btm_sco_chk_pend_unpark(u8 hci_status, u16 hci_handle, u8 mode);
void btm_sco_conn_req(u8* bda, u8* dev_class, u8 link_type);
void btm_sco_connected(u8 hci_status, u8* bda, u16 hci_handle, tBTM_ESCO_DATA* p_esco_data);

/* 0x80487810 - drops the SCO link `sco_inx`. */
u8 BTM_RemoveSco(u16 sco_inx);

/* 0x804878C8 / 0x80487A60 - drop the SCO links to `bda`, or forget them after its ACL link went down. */
void btm_remove_sco_links(u8* bda);
void btm_sco_acl_removed(u8* bda);

/* 0x8048794C / 0x80487B14 - the SCO link `hci_handle` disconnected; a SCO data packet arrived. */
void btm_sco_removed(u16 hci_handle, u8 reason);
void btm_route_sco_data(BT_HDR* p_msg);

/* 0x80487B18 / 0x80487CC0 - change a connected link's eSCO parameters; the controller changed them. */
u8 BTM_ChangeEScoLinkParms(u16 sco_inx, tBTM_CHG_ESCO_PARAMS* p_parms);
void btm_esco_proc_conn_chg(u8 status, u16 handle, u8 tx_interval, u8 retrans_window, u16 rx_pkt_len,
                            u16 tx_pkt_len);

/* 0x80487DC4 / 0x80487E34 / 0x80487E94 - whether the SCO link `handle` is up, how many links are busy, and whether a
 * SCO link to `remote_bda` is up. */
u8 btm_is_sco_active(u16 handle);
u8 btm_num_sco_links_active(void);
u8 btm_is_sco_active_by_bdaddr(u8* remote_bda);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_BTE_BTM_SCO_H */
