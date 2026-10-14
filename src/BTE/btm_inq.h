/* BTE/btm_inq.h - the inquiry database entry points `BTE/btm_inq.c` owns that other units call, and the inquiry
 *   records of the control block (docs/plan.md 6.5 rule 2). */
#ifndef MHTRI_BTE_BTM_INQ_H
#define MHTRI_BTE_BTM_INQ_H

#include "types.h"
#include "BTE/gki_time.h"

/* The entries the inquiry database holds. */
#define BTM_INQ_DB_SIZE 12


/* size: 0x12 - an inquiry result as the database keeps it: the clock offset, the peer, its class and paging modes,
 * the signal strength and whether the application already knows the peer's name. */
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
    /* +0x11 */ u8 pad_0x11;
} tBTM_INQ_INFO; /* size: 0x12 */

/* A 3-byte inquiry access code (LAP). */
typedef u8 LAP[3];

/* size: 0x06 - an inquiry event filter condition: a device address, or a class of device and its mask. */
typedef union tBTM_INQ_FILT_COND {
    /* +0x00 */ u8 bdaddr_cond[6];
    /* size: 0x06 */
    struct {
        /* +0x00 */ u8 dev_class[3];
        /* +0x03 */ u8 dev_class_mask[3];
    } cod_cond;
} tBTM_INQ_FILT_COND; /* size: 0x06 */

/* size: 0x0C - a peer seen during the current inquiry (the duplicate filter's entry). */
typedef struct tINQ_BDADDR {
    /* +0x00 */ u32 inq_count;
    /* +0x04 */ u8 bd_addr[6];
    /* +0x0A */ u8 pad_0x0A[2];
} tINQ_BDADDR; /* size: 0x0C */

/* size: 0x1C - an inquiry database entry: when the peer answered, in which inquiry, and its results. */
typedef struct tINQ_DB_ENT {
    /* +0x00 */ u32 time_of_resp;
    /* +0x04 */ u32 inq_count;
    /* +0x08 */ tBTM_INQ_INFO inq_info;
    /* +0x1A */ u8 in_use;
    /* +0x1B */ u8 pad_0x1B;
} tINQ_DB_ENT; /* size: 0x1C */

/* size: 0x0A - the inquiry the application started: mode, duration, response cap and the event filter. */
typedef struct tBTM_INQ_PARMS {
    /* +0x00 */ u8 mode;
    /* +0x01 */ u8 duration;
    /* +0x02 */ u8 max_resps;
    /* +0x03 */ u8 filter_cond_type;
    /* +0x04 */ tBTM_INQ_FILT_COND filter_cond;
} tBTM_INQ_PARMS; /* size: 0x0A */

/* size: 0x02 - the inquiry completion result. */
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

/* The end-of-inquiry and remote name callbacks. */
typedef void (*tBTM_INQ_CMPL_CB)(tBTM_INQUIRY_CMPL* p_cmpl);
typedef void (*tBTM_NAME_CMPL_CB)(tBTM_REMOTE_DEV_NAME* p_name);

/* The inquiry result, inquiry database change and connection filter callbacks. */
typedef void (tBTM_INQ_RESULTS_CB)(tBTM_INQ_INFO* p_inq_results);
typedef void (tBTM_INQ_DB_CHANGE_CB)(tBTM_INQ_INFO* p_entry, u8 is_new);
typedef u8 (tBTM_FILTER_CB)(u8* bd_addr, u8* dev_class);

/* size: 0x1D4 - the inquiry, scan mode and remote name state. */
typedef struct tBTM_INQUIRY_VAR_ST {
    /* +0x000 */ BtmCompleteCallback p_remname_cmpl_cb;
    /* +0x004 */ TIMER_LIST_ENT rmt_name_timer_ent;
    /* +0x01C */ u16 discoverable_mode;
    /* +0x01E */ u16 connectable_mode;
    /* +0x020 */ u16 page_scan_window;
    /* +0x022 */ u16 page_scan_period;
    /* +0x024 */ u16 inq_scan_window;
    /* +0x026 */ u16 inq_scan_period;
    /* +0x028 */ u16 inq_scan_type;
    /* +0x02A */ u16 page_scan_type;
    /* +0x02C */ u8 remname_bda[6];
    /* +0x032 */ u8 remname_active;
    /* +0x033 */ u8 pad_0x033;
    /* +0x034 */ BtmCompleteCallback p_inq_cmpl_cb;
    /* +0x038 */ tBTM_INQ_RESULTS_CB* p_inq_results_cb;
    /* +0x03C */ BtmCompleteCallback p_inqfilter_cmpl_cb;
    /* +0x040 */ tBTM_INQ_DB_CHANGE_CB* p_inq_change_cb;
    /* +0x044 */ u32 inq_counter;
    /* +0x048 */ TIMER_LIST_ENT inq_timer_ent;
    /* +0x060 */ tINQ_BDADDR* p_bd_db;
    /* +0x064 */ u16 num_bd_entries;
    /* +0x066 */ u16 max_bd_entries;
    /* +0x068 */ tINQ_DB_ENT inq_db[BTM_INQ_DB_SIZE];
    /* +0x1B8 */ tBTM_INQ_PARMS inqparms;
    /* +0x1C2 */ tBTM_INQUIRY_CMPL inq_cmpl_info;
    /* +0x1C4 */ u16 per_min_delay;
    /* +0x1C6 */ u16 per_max_delay;
    /* +0x1C8 */ u8 inqfilt_active;
    /* +0x1C9 */ u8 pending_filt_complete_event;
    /* +0x1CA */ u8 unused_0x1CA;
    /* +0x1CB */ u8 state;
    /* +0x1CC */ u8 inq_active;
    /* +0x1CD */ u8 pad_0x1CD[3];
    /* +0x1D0 */ tBTM_FILTER_CB* p_inq_filter_cb;
} tBTM_INQUIRY_VAR_ST; /* size: 0x1D4 */

#ifdef __cplusplus
extern "C" {
#endif

/* .sdata2 0x8079D0B8 / 0x8079D0BC - the general and limited inquiry access codes. */
extern const LAP general_inq_lap;
extern const LAP limited_inq_lap;

/* 0x8048495C / 0x80484BE0 / 0x80484C90 / 0x80484D40 / 0x80484DD8 - the scan settings: discoverability, the inquiry
 * and page scan types, the inquiry result mode and connectability. */
u8 BTM_SetDiscoverability(u16 inq_mode, u16 window, u16 interval);
u8 BTM_SetInquiryScanType(u16 scan_type);
u8 BTM_SetPageScanType(u16 scan_type);
u8 BTM_SetInquiryMode(u8 mode);
u8 BTM_SetConnectability(u16 page_mode, u16 window, u16 interval);

/* 0x80484F60 / 0x80484F70 / 0x80485078 - the inquiry activity flags, cancel and start an inquiry. */
u16 BTM_IsInquiryActive(void);
u8 BTM_CancelInquiry(void);
u8 BTM_StartInquiry(tBTM_INQ_PARMS* p_inqparms, tBTM_INQ_RESULTS_CB* p_results_cb, BtmCompleteCallback p_cmpl_cb);

/* 0x8048522C / 0x80485314 - ask for, or cancel, the name of a remote device. */
u8 BTM_ReadRemoteDeviceName(u8* remote_bda, tBTM_NAME_CMPL_CB p_cb);
u8 BTM_CancelRemoteDeviceName(void);

/* 0x8048538C / 0x8048544C / 0x80485504 / 0x80485640 - read, walk and clear the inquiry database. */
tBTM_INQ_INFO* BTM_InqDbRead(u8* p_bda);
tBTM_INQ_INFO* BTM_InqDbFirst(void);
tBTM_INQ_INFO* BTM_InqDbNext(tBTM_INQ_INFO* p_cur);
u8 BTM_ClearInqDb(u8* p_bda);

/* 0x80485710 / 0x804858A8 - reset the inquiry state after a controller reset; initialise the database. */
void btm_inq_db_reset(void);
void btm_inq_db_init(void);

/* 0x804858AC / 0x80485988 - the duplicate filter of the current inquiry; a new database entry. */
u8 btm_inq_find_bdaddr(u8* p_bda);
tINQ_DB_ENT* btm_inq_db_new(u8* p_bda);

/* 0x80485A8C / 0x80485B70 - set the inquiry event filter; its completion event. */
u8 btm_set_inq_event_filter(u8 filter_cond_type, tBTM_INQ_FILT_COND* p_filt_cond);
void btm_event_filter_complete(u8* p);

/* 0x80485D90 / 0x80486008 - the inquiry result and inquiry complete events. */
void btm_process_inq_results(u8* p, u8 inq_res_mode);
void btm_process_inq_complete(u8 status);

/* 0x8048610C / 0x8048625C / 0x804863B4 - start a remote name request; its result; its timeout. */
u8 btm_initiate_rem_name(u8* remote_bda, tBTM_INQ_INFO* p_cur, u8 origin, u32 timeout, BtmCompleteCallback p_cb);
void btm_process_remote_name(u8* bda, u8* bdn, u16 evt_len, u8 hci_status);
void btm_inq_rmt_name_failed(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_BTE_BTM_INQ_H */
