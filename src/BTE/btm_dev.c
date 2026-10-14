/*
 * BTE/btm_dev.c - the BTM security device records: add, delete and name read, record allocation and the lookups by
 *   handle, by address and the oldest record.
 *
 * RANGE. .text 0x804827D4-0x80482EA0 (7 functions, 0x6CC B); no data. Medium: the seven functions are exactly the BTE
 *   source's btm_dev.c (BTM_SecAddDevice..btm_find_oldest_dev), between btm_acl.c's btm_chg_all_acl_pkt_types and
 *   btm_devctl.c's btm_dev_init; no data on either side pins the cuts.
 * FLAGS. GC/3.0a5.2 with `cflags_nw4r`, as the matched BTE units (see BTE/l2c_utils.c); btm_find_dev inlines into
 *   its three callers here (`-ipa file`). Measured on this unit: Wii/1.3 + `cflags_os` matches 3 of 7 functions,
 *   GC/3.0a5.2 + `cflags_nw4r` all 7.
 * NAMES. the map's, which are the BTE source's.
 * RESIDUALS. none; a trial flip links byte-identically (DOL hash unchanged).
 * SHAPES. plain C; the records are btm_cb.sec_dev_rec (BTE/btm_main.h), the record type is BTE/btm_dev.h's.
 */

#include "BTE/btm_dev.h"
#include "BTE/btm_main.h"
#include "BTE/btm_inq.h"
#include "MSL_C/alloc.h"
#include "MSL_C/string.h"
#include "Runtime.PPCEABI.H/memcpy.h"
#include "Runtime.PPCEABI.H/memset.h"

/* 0x804827D4 (0x188): Adds the device at `bd_addr`, or refreshes its record, with the given class, name, features,
 * trusted services and link key. */
u8 BTM_SecAddDevice(u8* bd_addr, u8* dev_class, u8* bd_name, u8* features, u32* trusted_mask, u8* link_key) {
    tBTM_SEC_DEV_REC* p_dev_rec;

    p_dev_rec = btm_find_dev(bd_addr);
    if (!p_dev_rec) {
        p_dev_rec = btm_sec_alloc_dev(bd_addr);
        if (!p_dev_rec) {
            return FALSE;
        }
    } else {
        p_dev_rec->timestamp = btm_cb.dev_rec_count++;
    }

    if (dev_class) {
        memcpy(p_dev_rec->dev_class, dev_class, 3);
    }

    memset(p_dev_rec->sec_bd_name, 0, sizeof(p_dev_rec->sec_bd_name));

    if (bd_name && bd_name[0]) {
        p_dev_rec->sec_flags |= BTM_SEC_NAME_KNOWN;
        strncpy((char*)p_dev_rec->sec_bd_name, (char*)bd_name, BTM_MAX_REM_BD_NAME_LEN);
    }

    if (features) {
        memcpy(p_dev_rec->features, features, sizeof(p_dev_rec->features));
    } else {
        memset(p_dev_rec->features, 0, sizeof(p_dev_rec->features));
    }

    p_dev_rec->trusted_mask[0] = trusted_mask[0];
    p_dev_rec->trusted_mask[1] = trusted_mask[1];

    if (link_key) {
        p_dev_rec->sec_flags |= BTM_SEC_LINK_KEY_KNOWN;
        memcpy(p_dev_rec->link_key, link_key, sizeof(p_dev_rec->link_key));
    }

    return TRUE;
}

/* 0x8048295C (0xAC): Frees the record of the device at `bd_addr` and deletes its stored link key. */
u8 BTM_SecDeleteDevice(u8* bd_addr) {
    tBTM_SEC_DEV_REC* p_dev_rec;

    if ((p_dev_rec = btm_find_dev(bd_addr)) == NULL) {
        return FALSE;
    }

    p_dev_rec->sec_flags = 0;

    BTM_DeleteStoredLinkKey(bd_addr, NULL);

    return TRUE;
}

/* 0x80482A08 (0xA0): Returns the stored name of the device at `bd_addr`, or NULL. */
char* BTM_SecReadDevName(u8* bd_addr) {
    char* p_name = NULL;
    tBTM_SEC_DEV_REC* p_srec;

    if ((p_srec = btm_find_dev(bd_addr)) != NULL) {
        p_name = (char*)p_srec->sec_bd_name;
    }

    return p_name;
}

/* 0x80482AA8 (0x120): Takes a free record (else the oldest one), clears it and fills in what is known of `bd_addr`. */
tBTM_SEC_DEV_REC* btm_sec_alloc_dev(u8* bd_addr) {
    tBTM_SEC_DEV_REC* p_dev_rec = NULL;
    tBTM_INQ_INFO* p_inq_info;
    int i;

    for (i = 0; i < BTM_SEC_MAX_DEVICE_RECORDS; i++) {
        if (!(btm_cb.sec_dev_rec[i].sec_flags & BTM_SEC_IN_USE)) {
            p_dev_rec = &btm_cb.sec_dev_rec[i];
            break;
        }
    }

    if (!p_dev_rec) {
        p_dev_rec = btm_find_oldest_dev();
    }

    memset(p_dev_rec, 0, sizeof(tBTM_SEC_DEV_REC));

    p_dev_rec->sec_flags = BTM_SEC_IN_USE;
    p_dev_rec->link_key_type = 0;

    if ((p_inq_info = BTM_InqDbRead(bd_addr)) != NULL) {
        memcpy(p_dev_rec->dev_class, p_inq_info->dev_class, 3);
    } else {
        memcpy(p_dev_rec->dev_class, btm_cb.connecting_dc, 3);
    }

    memcpy(p_dev_rec->bd_addr, bd_addr, 6);

    p_dev_rec->hci_handle = BTM_GetHCIConnHandle(bd_addr);
    p_dev_rec->timestamp = btm_cb.dev_rec_count++;

    return p_dev_rec;
}

/* 0x80482BC8 (0xB8): Returns the record of the device on the link `handle`, or NULL. */
tBTM_SEC_DEV_REC* btm_find_dev_by_handle(u16 handle) {
    tBTM_SEC_DEV_REC* p_dev_rec = &btm_cb.sec_dev_rec[0];
    int i;

    for (i = 0; i < BTM_SEC_MAX_DEVICE_RECORDS; i++, p_dev_rec++) {
        if ((p_dev_rec->sec_flags & BTM_SEC_IN_USE) && (p_dev_rec->hci_handle == handle)) {
            return p_dev_rec;
        }
    }
    return NULL;
}

/* 0x80482C80 (0x88): Returns the record of the device at `bd_addr`, or NULL. */
tBTM_SEC_DEV_REC* btm_find_dev(u8* bd_addr) {
    tBTM_SEC_DEV_REC* p_dev_rec = &btm_cb.sec_dev_rec[0];
    int i;

    for (i = 0; i < BTM_SEC_MAX_DEVICE_RECORDS; i++, p_dev_rec++) {
        if ((p_dev_rec->sec_flags & BTM_SEC_IN_USE) && !memcmp(p_dev_rec->bd_addr, bd_addr, 6)) {
            return p_dev_rec;
        }
    }
    return NULL;
}

/* 0x80482D08 (0x198): Returns the oldest record of a device without a link key, else the oldest record. */
tBTM_SEC_DEV_REC* btm_find_oldest_dev(void) {
    tBTM_SEC_DEV_REC* p_dev_rec = &btm_cb.sec_dev_rec[0];
    tBTM_SEC_DEV_REC* p_oldest = p_dev_rec;
    u32 ot = 0xFFFFFFFF;
    int i;

    for (i = 0; i < BTM_SEC_MAX_DEVICE_RECORDS; i++, p_dev_rec++) {
        if (((p_dev_rec->sec_flags & BTM_SEC_IN_USE) == 0) || ((p_dev_rec->sec_flags & BTM_SEC_LINK_KEY_KNOWN) != 0)) {
            continue;
        }

        if (p_dev_rec->timestamp < ot) {
            p_oldest = p_dev_rec;
            ot = p_dev_rec->timestamp;
        }
    }

    if (ot != 0xFFFFFFFF) {
        return p_oldest;
    }

    p_dev_rec = &btm_cb.sec_dev_rec[0];
    for (i = 0; i < BTM_SEC_MAX_DEVICE_RECORDS; i++, p_dev_rec++) {
        if ((p_dev_rec->sec_flags & BTM_SEC_IN_USE) == 0) {
            continue;
        }

        if (p_dev_rec->timestamp < ot) {
            p_oldest = p_dev_rec;
            ot = p_dev_rec->timestamp;
        }
    }
    return p_oldest;
}
