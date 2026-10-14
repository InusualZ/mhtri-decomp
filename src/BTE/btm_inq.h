/* BTE/btm_inq.h - the inquiry database entry points `BTE/btm_inq.c` owns that other units call, and the head of an
 *   entry (docs/plan.md 6.5 rule 2). */
#ifndef MHTRI_BTE_BTM_INQ_H
#define MHTRI_BTE_BTM_INQ_H

#include "types.h"
#include "BTE/gki_time.h"

/* size: 0x14 - the head of an inquiry database entry: the inquiry response and whether the application knows the
 * name (approximation: the entry continues past the name flag). */
typedef struct tBTM_INQ_INFO {
    /* +0x00 */ u16 clock_offset;
    /* +0x02 */ u8 remote_bd_addr[6];
    /* +0x08 */ u8 dev_class[3];
    /* +0x0B */ u8 page_scan_rep_mode;
    /* +0x0C */ u8 page_scan_per_mode;
    /* +0x0D */ u8 page_scan_mode;
    /* +0x0E */ s8 rssi;
    /* +0x0F */ u8 pad_0x0F;
    /* +0x10 */ u8 appl_knows_rem_name;
    /* +0x11 */ u8 unused_0x11[3];
} tBTM_INQ_INFO; /* size: 0x14 - approximation: the head only */

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8048538C - the inquiry database entry of the device at `p_bda`, or NULL. */
tBTM_INQ_INFO* BTM_InqDbRead(u8* p_bda);

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

/* size: 0x02 - the inquiry completion: status and the number of responses. */
typedef struct tBTM_INQUIRY_CMPL {
    /* +0x00 */ u8 status;
    /* +0x01 */ u8 num_resp;
} tBTM_INQUIRY_CMPL; /* size: 0x02 */

/* size: 0xFC - the result of a remote name request. */
typedef struct tBTM_REMOTE_DEV_NAME {
    /* +0x00 */ u16 status;
    /* +0x02 */ u16 length;
    /* +0x04 */ u8 remote_bd_name[248];
} tBTM_REMOTE_DEV_NAME; /* size: 0xFC */

/* The inquiry callbacks: a response (the head of its database entry), the end of the inquiry, a name read. */
typedef void (*tBTM_INQ_RESULTS_CB)(tBTM_INQ_INFO* p_inq_results);
typedef void (*tBTM_INQ_CMPL_CB)(tBTM_INQUIRY_CMPL* p_cmpl);
typedef void (*tBTM_NAME_CMPL_CB)(tBTM_REMOTE_DEV_NAME* p_name);

/* 0x8048495C / 0x80484DD8 - set the discoverable and connectable modes. */
u8 BTM_SetDiscoverability(u16 inq_mode, u16 window, u16 interval);
u8 BTM_SetConnectability(u16 page_mode, u16 window, u16 interval);

/* 0x80484F60 / 0x80484F70 / 0x80485078 - whether an inquiry runs, cancel it, start one with the parameters
 * `p_inqparms` (BtaDmInq's layout). */
u16 BTM_IsInquiryActive(void);
u8 BTM_CancelInquiry(void);
u8 BTM_StartInquiry(struct BtaDmInq* p_inqparms, tBTM_INQ_RESULTS_CB p_results_cb, tBTM_INQ_CMPL_CB p_cmpl_cb);

/* 0x8048522C / 0x80485314 - read the name of the device at `remote_bda`, or cancel that. */
u8 BTM_ReadRemoteDeviceName(u8* remote_bda, tBTM_NAME_CMPL_CB p_cb);
u8 BTM_CancelRemoteDeviceName(void);

/* 0x8048544C / 0x80485504 / 0x80485640 - walk the inquiry database; clear it (`p_bda` NULL: every entry). */
tBTM_INQ_INFO* BTM_InqDbFirst(void);
tBTM_INQ_INFO* BTM_InqDbNext(tBTM_INQ_INFO* p_cur);
u8 BTM_ClearInqDb(u8* p_bda);

/* 0x80484BE0 / 0x80484C90 / 0x80484D40 - set the inquiry scan type, the page scan type and the inquiry mode. */
u8 BTM_SetInquiryScanType(u16 scan_type);
u8 BTM_SetPageScanType(u16 scan_type);
u8 BTM_SetInquiryMode(u8 mode);

/* 0x80485710 - clears the inquiry database. */
void btm_inq_db_reset(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_BTE_BTM_INQ_H */
