/*
 * BTE/wbt_ext.c - the WBT extension: WBT_ExtCreateRecord, which adds the device identification attributes to the local
 *   SDP DI record.
 *
 * RANGE. .text 0x8048C1B4-0x8048C294 (1 function, 0xE0 B); no data. Medium confidence: the only function of its family,
 *   between BTE_Init and GAP_Init, with no data on either side.
 * FLAGS. GC/3.0a5.2 with `cflags_nw4r`, the compiler of every measured BTE unit (BTE/l2c_utils.c FLAGS line).
 * NAMES. the map's.
 * RESIDUALS. none known; Wii/1.3 schedules the prologue differently.
 * SHAPES. plain C; the handle is `volatile`, which keeps the retail store that clears it before the failure return, and
 *   the record is written through a pointer kept in a saved register.
 */

#include "BTE/wbt_ext.h"
#include "BTE/sdp_db.h"
#include "BTE/sdp_int.h"
#include "Runtime.PPCEABI.H/memset.h"

/* The public browse group service class and the browse group list attribute. */
#define UUID_SERVCLASS_PUBLIC_BROWSE_GROUP 0x1002
#define ATTR_ID_BROWSE_GROUP_LIST 0x0005

/* The vendor id of the DI record (Broadcom) and its source (Bluetooth SIG). */
#define WBT_DI_VENDOR 15
#define WBT_DI_VENDOR_ID_SOURCE 1

/* The WBT extension attribute and its two-byte value. */
#define ATTR_ID_WBT_EXT 0x8001
#define WBT_EXT_VALUE_LEN 2

/* 0x8048C1B4 (0xE0): Creates the local DI record when there is none and adds the browse group and the WBT attribute. */
u8 WBT_ExtCreateRecord(void) {
    u8 ext_value[WBT_EXT_VALUE_LEN];
    volatile u32 sdp_handle = 0;
    u16 browse_uuid = UUID_SERVCLASS_PUBLIC_BROWSE_GROUP;
    tSDP_DI_GET_RECORD di_rec;
    tSDP_DI_RECORD* p_rec;

    if (SDP_GetLocalDiRecord(&di_rec, (u32*)&sdp_handle) != SDP_SUCCESS) {
        p_rec = &di_rec.rec;
        memset(p_rec, 0, sizeof(tSDP_DI_RECORD));
        p_rec->vendor = WBT_DI_VENDOR;
        p_rec->vendor_id_source = WBT_DI_VENDOR_ID_SOURCE;
        p_rec->primary_record = TRUE;

        if (SDP_SetLocalDiRecord(p_rec, (u32*)&sdp_handle) != SDP_SUCCESS) {
            sdp_handle = 0;
            return FALSE;
        }
    }

    SDP_AddUuidSequence(sdp_handle, ATTR_ID_BROWSE_GROUP_LIST, 1, &browse_uuid);

    ext_value[0] = 0;
    ext_value[1] = 1;
    SDP_AddAttribute(sdp_handle, ATTR_ID_WBT_EXT, UINT_DESC_TYPE, WBT_EXT_VALUE_LEN, ext_value);

    return TRUE;
}
