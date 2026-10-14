/* BTE/btm_inq.h - the inquiry database entry points `BTE/btm_inq.c` owns that other units call, and the head of an
 *   entry (docs/plan.md 6.5 rule 2). */
#ifndef MHTRI_BTE_BTM_INQ_H
#define MHTRI_BTE_BTM_INQ_H

#include "types.h"
#include "BTE/gki_time.h"

/* size: 0x0E - the head of an inquiry database entry; only the paging fields the L2CAP layer reads are named
 * (approximation: the entry continues past the paging modes). */
typedef struct tBTM_INQ_INFO {
    /* +0x00 */ u16 clock_offset;
    /* +0x02 */ u8 remote_bd_addr[6];
    /* +0x08 */ u8 dev_class[3];
    /* +0x0B */ u8 page_scan_rep_mode;
    /* +0x0C */ u8 page_scan_per_mode;
    /* +0x0D */ u8 page_scan_mode;
} tBTM_INQ_INFO; /* size: 0x0E - approximation: the head only */

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8048538C - the inquiry database entry of the device at `p_bda`, or NULL. */
tBTM_INQ_INFO* BTM_InqDbRead(u8* p_bda);

/* 0x8048544C */
tBTM_INQ_INFO* BTM_InqDbFirst(void);
/* 0x80485504 */
tBTM_INQ_INFO* BTM_InqDbNext(tBTM_INQ_INFO* p_cur);

/* 0x8048522C - requests the name of the device at `remote_bda`; `p_cb` reports it. */
u8 BTM_ReadRemoteDeviceName(u8* remote_bda, BtmCompleteCallback p_cb);

/* 0x804863B4 - ends a remote name request that timed out. */
void btm_inq_rmt_name_failed(void);

/* 0x80485B70 */
void btm_event_filter_complete(u8* p);
/* 0x80485D90 */
void btm_process_inq_results(u8* p, u8 inq_res_mode);
/* 0x80486008 */
void btm_process_inq_complete(u8 status);
/* 0x8048625C */
void btm_process_remote_name(u8* bda, u8* bdn, u16 evt_len, u8 hci_status);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_BTE_BTM_INQ_H */
