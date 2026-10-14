/* BTE/btm_dev.h - the security device record `BTE/btm_dev.c` keeps in btm_cb and the device record entry points it owns
 *   that other units call (docs/plan.md 6.5 rule 2). */
#ifndef MHTRI_BTE_BTM_DEV_H
#define MHTRI_BTE_BTM_DEV_H

#include "types.h"
#include "BTE/btm_sec.h"

/* The number of security device records btm_cb keeps (the BTE source's BTM_SEC_MAX_DEVICE_RECORDS). */
#define BTM_SEC_MAX_DEVICE_RECORDS 16

/* The longest remote device name a record keeps, without its terminator (BTM_MAX_REM_BD_NAME_LEN). */
#define BTM_MAX_REM_BD_NAME_LEN 64

/* The `sec_flags` bits of a record. */
#define BTM_SEC_ENCRYPTED 0x04
#define BTM_SEC_NAME_KNOWN 0x08
#define BTM_SEC_LINK_KEY_KNOWN 0x10
#define BTM_SEC_IN_USE 0x80

struct tBTM_SEC_SERV_REC;

/* size: 0x88 - a security device record: the service being checked and its completion callback, the trusted
 * services, the link, the device's address, class, link key, name and features, and the state of its pairing
 * (layout traced from BTE/btm_dev.c, BTE/btm_acl.c, BTE/btm_sec.c and BTE/btm_pm.c). */
typedef struct tBTM_SEC_DEV_REC {
    /* +0x00 */ struct tBTM_SEC_SERV_REC* p_cur_service;
    /* +0x04 */ tBTM_SEC_CALLBACK* p_callback;
    /* +0x08 */ void* p_ref_data;
    /* +0x0C */ u32 timestamp;
    /* +0x10 */ u32 trusted_mask[2];
    /* +0x18 */ u16 hci_handle;
    /* +0x1A */ u16 clock_offset;
    /* +0x1C */ u8 bd_addr[6];
    /* +0x22 */ u8 dev_class[3];
    /* +0x25 */ u8 link_key[16];
    /* +0x35 */ u8 sec_bd_name[BTM_MAX_REM_BD_NAME_LEN + 1];
    /* +0x76 */ u8 sec_flags;
    /* +0x77 */ u8 features[8];
    /* +0x7F */ u8 sec_state;
    /* +0x80 */ u8 is_originator;
    /* +0x81 */ u8 security_required;
    /* +0x82 */ u8 link_key_not_sent;
    /* +0x83 */ u8 link_key_type;
    /* +0x84 */ u8 link_key_changed;
    /* +0x85 */ u8 acl_connected;
    /* +0x86 */ u8 pad_0x86[2];
} tBTM_SEC_DEV_REC; /* size: 0x88 */

#ifdef __cplusplus
extern "C" {
#endif

/* 0x804827D4 / 0x8048295C - add (or update) and delete the record of the device at `bd_addr`; both return whether
 * they found or made a record. `trusted_mask` is two words of trusted services. */
u8 BTM_SecAddDevice(u8* bd_addr, u8* dev_class, u8* bd_name, u8* features, u32* trusted_mask, u8* link_key);
u8 BTM_SecDeleteDevice(u8* bd_addr);

/* 0x80482A08 - the stored name of the device at `bd_addr`, or NULL. */
char* BTM_SecReadDevName(u8* bd_addr);

/* 0x80482AA8 - a cleared record for `bd_addr` (a free one, else the oldest one). */
tBTM_SEC_DEV_REC* btm_sec_alloc_dev(u8* bd_addr);

/* 0x80482BC8 / 0x80482C80 - the record of the device on the link `handle` or at `bd_addr`, or NULL. */
tBTM_SEC_DEV_REC* btm_find_dev_by_handle(u16 handle);
tBTM_SEC_DEV_REC* btm_find_dev(u8* bd_addr);

/* 0x80482D08 - the oldest record, preferring a device without a link key. */
tBTM_SEC_DEV_REC* btm_find_oldest_dev(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_BTE_BTM_DEV_H */
