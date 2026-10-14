/*
 * BTE/btm_link_key.c - the BTM page timeout and stored link key commands: BTM_WritePageTimeout, the read/write/delete
 *   stored link key requests and their completions, the returned link keys event and the device status report.
 *
 * RANGE. .text 0x80484224-0x80484828 (9 functions, 0x604 B); .data 0x806140E0-0x806141E0; .sdata
 *   0x80793D8C-0x80793DA0. Strong start: the .data starts after a 4-byte pad beyond 4-byte alignment (0x806140DC ->
 *   0x806140E0) and BTM_WritePageTimeout is its first reader, the first function after btm_devctl.c's last .data
 *   reader. The end is weak: the next .data pad seam (0x806141E0, BTM_SetDiscoverability) allows any cut in
 *   [0x8048453C, 0x8048495C]; it is placed after btm_report_device_status, before the btm_discovery_db_* family. The
 *   .sdata strings "TRUE"/"FALSE" (0x80793D8C, 0x80793D94) are read only by BTM_ReadStoredLinkKey and
 *   BTM_DeleteStoredLinkKey.
 * FLAGS. GC/3.0a5.2 with `cflags_nw4r`, as the matched BTE units (see BTE/l2c_utils.c).
 * NAMES. the map's, which are the BTE source's. The file name is a GUESS from its functions: the later BTE source keeps
 *   them in btm_devctl.c, but the .data pad seam puts them in a separate file whose name the binary does not carry.
 * RESIDUALS. the code matches; a trial flip links the .data 0x40 B
 *   short: mwld strips the string objects of the stripped BTM_WriteVoiceSettings/BTM_EnableTestMode, while retail keeps
 *   them. The objects are reached by @N relocations only (no kept function forms a section-relative string base), so
 *   nothing keeps them; the retail shape that does is unknown. BTM_WriteVoiceSettings and BTM_EnableTestMode are GUESS
 *   bodies around their trace strings.
 * SHAPES. plain C; the callback is btm_cb.p_stored_link_key_cmpl_cb (BTE/btm_main.h).
 */

#include "BTE/btm_link_key.h"
#include "BTE/btm_main.h"
#include "BTE/hcicmds.h"

/* The GKI pool the HCI commands are built in. */
#define HCI_CMD_POOL_ID 2

/* Copy `len` bytes from the stream `p` into the array `a` reversed, and back into the stream in order. */
#define REVERSE_STREAM_TO_ARRAY(a, p, len) {register int ijk; register u8* _pa = (u8*)(a) + (len) - 1; \
    for (ijk = 0; ijk < (len); ijk++) *_pa-- = *(p)++;}
#define ARRAY_TO_STREAM(p, a, len) {register int ijk; for (ijk = 0; ijk < (len); ijk++) *(p)++ = (u8)(a)[ijk];}

/* The BTM status codes these commands return. */
#define BTM_SUCCESS 0
#define BTM_BUSY 2
#define BTM_NO_RESOURCES 3

/* 0x80484224 (0xA0): Stores the page timeout `timeout` (in slots) and sends it to the controller. */
u8 BTM_WritePageTimeout(u16 timeout) {
    BT_HDR* p_buf;

    BTM_TRACE_EVENT1("BTM: BTM_WritePageTimeout: Timeout: %d.", timeout);

    btm_cb.page_timeout = (timeout * 625) / 1000;

    if ((p_buf = (BT_HDR*)GKI_getpoolbuf(HCI_CMD_POOL_ID)) != NULL) {
        btsnd_hcic_write_page_tout(p_buf, timeout);
        return BTM_SUCCESS;
    }
    return BTM_NO_RESOURCES;
}

/* Sends the voice setting `settings`; the linker strips it (no caller), only its trace string survives. */
u8 BTM_WriteVoiceSettings(u16 settings) {
    BTM_TRACE_EVENT1("BTM: BTM_WriteVoiceSettings: Settings: 0x%04x.", settings);

    if (GKI_getpoolbuf(HCI_CMD_POOL_ID) != NULL) {
        return BTM_SUCCESS;
    }
    return BTM_NO_RESOURCES;
}

/* Puts the controller in test mode; the linker strips it (no caller), only its trace string survives. */
u8 BTM_EnableTestMode(void) {
    BTM_TRACE_EVENT0("BTM: BTM_EnableTestMode");

    if (GKI_getpoolbuf(HCI_CMD_POOL_ID) != NULL) {
        return BTM_SUCCESS;
    }
    return BTM_NO_RESOURCES;
}

/* 0x804842C4 (0xD8): Reads the link key the controller stores for `bd_addr` (NULL: every key); `p_cb` gets the
 * result. */
u8 BTM_ReadStoredLinkKey(const u8* bd_addr, BtmCompleteCallback p_cb) {
    u8 local_bd_addr[6];
    u8 read_all_flag = FALSE;
    BT_HDR* p_buf;

    if (btm_cb.p_stored_link_key_cmpl_cb) {
        return BTM_BUSY;
    }

    if (!bd_addr) {
        read_all_flag = TRUE;
        bd_addr = local_bd_addr;
    }

    BTM_TRACE_EVENT1("BTM: BTM_ReadStoredLinkKey: Read_All: %s", read_all_flag ? "TRUE" : "FALSE");

    if ((p_buf = (BT_HDR*)GKI_getpoolbuf(HCI_CMD_POOL_ID)) != NULL) {
        btm_cb.p_stored_link_key_cmpl_cb = p_cb;
        btsnd_hcic_read_stored_key(p_buf, (u8*)bd_addr, read_all_flag);
        return BTM_SUCCESS;
    }
    return BTM_NO_RESOURCES;
}

/* 0x8048439C (0xD0): Writes up to 11 addresses and link keys to the controller; `p_cb` gets the result. */
u8 BTM_WriteStoredLinkKey(u8 num_keys, const u8* bd_addr, const u8* link_key, BtmCompleteCallback p_cb) {
    BT_HDR* p_buf;

    if (btm_cb.p_stored_link_key_cmpl_cb) {
        return BTM_BUSY;
    }

    BTM_TRACE_EVENT1("BTM: BTM_WriteStoredLinkKey: num_keys: %d", num_keys);

    if (num_keys > BTM_MAX_WRITE_LINK_KEYS) {
        num_keys = BTM_MAX_WRITE_LINK_KEYS;
    }

    if ((p_buf = (BT_HDR*)GKI_getpoolbuf(HCI_CMD_POOL_ID)) != NULL) {
        btm_cb.p_stored_link_key_cmpl_cb = p_cb;
        btsnd_hcic_write_stored_key(p_buf, num_keys, (u8 (*)[6])bd_addr, (u8 (*)[16])link_key);
        return BTM_SUCCESS;
    }
    return BTM_NO_RESOURCES;
}

/* 0x8048446C (0xD0): Deletes the link key the controller stores for `bd_addr` (NULL: every key); `p_cb` gets the
 * result. */
u8 BTM_DeleteStoredLinkKey(const u8* bd_addr, BtmCompleteCallback p_cb) {
    u8 local_bd_addr[6];
    u8 delete_all_flag = FALSE;

    if (btm_cb.p_stored_link_key_cmpl_cb) {
        return BTM_BUSY;
    }

    if (!bd_addr) {
        delete_all_flag = TRUE;
        bd_addr = local_bd_addr;
    }

    BTM_TRACE_EVENT1("BTM: BTM_DeleteStoredLinkKey: delete_all_flag: %s", delete_all_flag ? "TRUE" : "FALSE");

    btm_cb.p_stored_link_key_cmpl_cb = p_cb;
    if (!btsnd_hcic_delete_stored_key((u8*)bd_addr, delete_all_flag)) {
        return BTM_NO_RESOURCES;
    }
    return BTM_SUCCESS;
}

/* 0x8048453C (0x7C): Reports the stored link key read result to the waiting callback. */
void btm_read_stored_link_key_complete(u8* p) {
    BtmCompleteCallback p_cb = btm_cb.p_stored_link_key_cmpl_cb;
    tBTM_READ_STORED_LINK_KEY_COMPLETE result;

    btm_cb.p_stored_link_key_cmpl_cb = NULL;

    if (p_cb) {
        result.event = BTM_CB_EVT_READ_STORED_LINK_KEYS;

        STREAM_TO_UINT8(result.status, p);
        STREAM_TO_UINT16(result.max_keys, p);
        STREAM_TO_UINT16(result.read_keys, p);

        (*p_cb)(&result);
    }
}

/* 0x804845B8 (0x5C): Reports the stored link key write result to the waiting callback. */
void btm_write_stored_link_key_complete(u8* p) {
    BtmCompleteCallback p_cb = btm_cb.p_stored_link_key_cmpl_cb;
    tBTM_WRITE_STORED_LINK_KEY_COMPLETE result;

    btm_cb.p_stored_link_key_cmpl_cb = NULL;

    if (p_cb) {
        result.event = BTM_CB_EVT_WRITE_STORED_LINK_KEYS;

        STREAM_TO_UINT8(result.status, p);
        STREAM_TO_UINT8(result.num_keys, p);

        (*p_cb)(&result);
    }
}

/* 0x80484614 (0x68): Reports the stored link key delete result to the waiting callback. */
void btm_delete_stored_link_key_complete(u8* p) {
    BtmCompleteCallback p_cb = btm_cb.p_stored_link_key_cmpl_cb;
    tBTM_DELETE_STORED_LINK_KEY_COMPLETE result;

    btm_cb.p_stored_link_key_cmpl_cb = NULL;

    if (p_cb) {
        result.event = BTM_CB_EVT_DELETE_STORED_LINK_KEYS;

        STREAM_TO_UINT8(result.status, p);
        STREAM_TO_UINT16(result.num_keys, p);

        (*p_cb)(&result);
    }
}

/* 0x8048467C (0x18C): Reverses each returned address and link key in place and hands the event to the waiting
 * callback. */
void btm_return_link_keys_evt(tBTM_RETURN_LINK_KEYS_EVT* result) {
    u8 i;
    u8* p;
    u8* p1;
    u8 bd_addr[6];
    u8 link_key[16];
    BtmCompleteCallback p_cb = btm_cb.p_stored_link_key_cmpl_cb;

    if (p_cb) {
        p = result->key_data;
        for (i = 0; i < result->num_keys; i++) {
            p1 = p;

            REVERSE_STREAM_TO_ARRAY(bd_addr, p1, 6);
            REVERSE_STREAM_TO_ARRAY(link_key, p1, 16);

            ARRAY_TO_STREAM(p, bd_addr, 6);
            ARRAY_TO_STREAM(p, link_key, 16);
        }

        (*p_cb)(result);
    }
}

/* 0x80484808 (0x20): Reports the device status `status` to the registered status callback. */
void btm_report_device_status(u32 status) {
    if (btm_cb.p_dev_status_cb) {
        (*btm_cb.p_dev_status_cb)(status);
    }
}
