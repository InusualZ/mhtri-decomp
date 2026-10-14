/* BTE/btm_sco.h - the SCO link entry points `BTE/btm_sco.c` owns that other units call (docs/plan.md 6.5 rule 2). */
#ifndef MHTRI_BTE_BTM_SCO_H
#define MHTRI_BTE_BTM_SCO_H

#include "types.h"
#include "BTE/bd.h"

/* size: 0x0E - the eSCO link parameters btm_sco_connected receives. */
typedef struct tBTM_ESCO_DATA {
    /* +0x00 */ u16 rx_pkt_len;
    /* +0x02 */ u16 tx_pkt_len;
    /* +0x04 */ u8 bd_addr[BD_ADDR_LEN];
    /* +0x0A */ u8 link_type;
    /* +0x0B */ u8 tx_interval;
    /* +0x0C */ u8 retrans_window;
    /* +0x0D */ u8 air_mode;
} tBTM_ESCO_DATA; /* size: 0x0E */

#ifdef __cplusplus
extern "C" {
#endif

/* 0x804878C8 / 0x80487A60 - drop the SCO links to `bda`, or forget them after its ACL link went down. */
void btm_remove_sco_links(u8* bda);
void btm_sco_acl_removed(u8* bda);

/* 0x80487E94 - whether a SCO link to `remote_bda` is up. */
u8 btm_is_sco_active_by_bdaddr(u8* remote_bda);

struct BT_HDR;
/* 0x80487B14 - routes the received SCO data message `p_msg` to its link. */
void btm_route_sco_data(struct BT_HDR* p_msg);

/* 0x80487670 - handles an SCO or eSCO connection complete. */
void btm_sco_connected(u8 hci_status, u8* bda, u16 hci_handle, tBTM_ESCO_DATA* p_esco_data);

/* 0x80487304 */
void btm_sco_chk_pend_unpark(u8 hci_status, u16 hci_handle, u8 mode);
/* 0x80487490 */
void btm_sco_conn_req(u8* bda, u8* dev_class, u8 link_type);
/* 0x8048794C */
u8 btm_sco_removed(u16 hci_handle, u8 reason);
/* 0x80487CC0 */
void btm_esco_proc_conn_chg(u8 status, u16 handle, u8 tx_interval, u8 retrans_window, u16 rx_pkt_len, u16 tx_pkt_len);
/* 0x80487DC4 */
u8 btm_is_sco_active(u16 handle);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_BTE_BTM_SCO_H */
