/* WUD/WUDHidHost.cpp - the HID host side of the Wii remote device manager: the BTA HID host event callback and the BTE
 *   callouts that deliver HID data and log open/close.
 * RANGE. .text 0x80500130-0x80500720 (5 functions); .data 0x8062ED98-0x8062F030 (the BTA_HH_* event strings, the jump
 *   table 0x8062EFB0 and the callouts' strings).  Seam with WUD/wud.cpp at 0x80500130 / 0x8062ED98: under `-str reuse`
 *   "Nintendo RVL-CNT" is emitted twice (0x8062DE58 read by `WUDiInsertDeviceNode`, 0x8062EE08 read by
 *   `WUDiHidHostEventCallback`); this unit's strings start 8-aligned after WUD.c's last string "Not Found WBC"; and
 *   `WUDiHidHostEventCallback` calls the 8-to-20-byte WUD.c functions `WUDIsLinkedWBC`, `WUDiGetStoredDeviceInfo`,
 *   `_WUDSetDevAddr`, `_WUDSetQueuedSize` and `_WUDSetNotAckedSize` out of line.  Seam with WUD/debug_msg.cpp: the SDK
 *   file list, codegen-neutral.  No .bss/.sbss: the unit reads `wudCB` (WUD/wud.h) and calls WUD.c functions only.
 * FLAGS. `cflags_base` (-O4,p, 16-byte function alignment).
 * NAMES. `bta_hh_co_data`, `bta_hh_co_open`, `bta_hh_co_close` are the map's (BTE callout names).  GUESS: the unit name
 *   (SDK spelling), `WUDiHidHostEventCallback` (the BTA_HH_* event strings) and `bta_dm_co_get_compress_memory` (the BTE
 *   callout `bta_dm_l2cap_server_compress_cback` calls).
 * RESIDUALS. none in .text or .data (flipcheck READY).
 * SHAPES. the BTA_HH event payloads are file-local types (gki_time.h only forward-declares `union BtaHhEventData`); the
 *   buffer status loop indexes `entries[i]` (a walking entry pointer swaps the counter and pointer registers).
 */

#include "types.h"
#include "WUD/WUDHidHost.h"
#include "WUD/wud.h"
#include "WUD/debug_msg.h"
#include "OS/OSError.h"
#include "MSL_C/alloc.h"

/* size: 0x08 - the open, add-device and remove-device event payload: the device and the result. */
typedef struct BtaHhConn {
    /* +0x00 */ u8 bdAddr[6];
    /* +0x06 */ u8 status;
    /* +0x07 */ u8 handle;
} BtaHhConn; /* size: 0x08 */

/* size: 0x02 - the close event payload. */
typedef struct BtaHhCbData {
    /* +0x00 */ u8 status;
    /* +0x01 */ u8 handle;
} BtaHhCbData; /* size: 0x02 */

/* size: 0x06 - one device's send buffer counts in the buffer status event. */
typedef struct BtaHhBufferEntry {
    /* +0x00 */ u8 handle;
    /* +0x01 */ u8 pad_0x01;
    /* +0x02 */ u16 queued;
    /* +0x04 */ u16 notAcked;
} BtaHhBufferEntry; /* size: 0x06 */

/* size: 0x0C - the buffer status event payload with one entry; `count` entries follow `count`. */
typedef struct BtaHhBufferStatus {
    /* +0x00 */ u16 head;
    /* +0x02 */ u16 tail;
    /* +0x04 */ u16 count;
    /* +0x06 */ BtaHhBufferEntry entries[1];
} BtaHhBufferStatus; /* size: 0x0C */

/* size: 0x0C - the payload of a BTA HID host event, by event. */
union BtaHhEventData {
    /* +0x00 */ BtaHhConn conn;
    /* +0x00 */ BtaHhCbData devStatus;
    /* +0x00 */ BtaHhBufferStatus buffer;
};

/* Tracks the HID host events: enables the profile, records an opened or closed link in the device lists and the
 * per-handle tables, and copies the send buffer counts. */
void WUDiHidHostEventCallback(u8 event, union BtaHhEventData* data)
{
    WUDCB* cb = &wudCB;
    WUDDevice* device;
    WUDDevice* stored;
    s32 i;

    switch (event) {
    case 0:
        WUDiDebugPrint("BTA_HH_ENABLE_EVT\n");
        cb->enabledProfiles |= 1;
        break;
    case 1:
        WUDiDebugPrint("BTA_HH_DISABLE_EVT\n");
        break;
    case 2:
        WUDiDebugPrint("BTA_HH_OPEN_EVT\n");
        WUDiDebugPrint("handle: %d, addr: %02x:%02x:%02x:%02x:%02x:%02x\n", data->conn.handle, data->conn.bdAddr[0],
                       data->conn.bdAddr[1], data->conn.bdAddr[2], data->conn.bdAddr[3], data->conn.bdAddr[4],
                       data->conn.bdAddr[5]);
        if (data->conn.status == 0) {
            device = WUDiGetStoredDeviceInfo();
            if (memcmp(device->bdAddr, data->conn.bdAddr, 6) != 0) {
                device = WUDiGetDevInfo(data->conn.bdAddr);
            }
            switch (device->linkState) {
            case 12:
                cb->syncState = 18;
                break;
            case 2:
                cb->syncState = 23;
                break;
            }
            device->linkState = 8;
            device->handle = data->conn.handle;
            cb->wbcLinkNumber++;
            device = WUDiGetDevInfo(data->conn.bdAddr);
            if (device == NULL) {
                device = WUDiGetStoredDeviceInfo();
            }
            _WUDSetDevAddr(data->conn.handle, device->bdAddr);
            _WUDSetQueuedSize(data->conn.handle, 0);
            _WUDSetNotAckedSize(data->conn.handle, 0);
            if (device->type == 3 || device->type == 1) {
                WUDiMoveWbcToHead(device);
            } else if (!WUDIsLinkedWBC() || (WUDIsLinkedWBC() && memcmp(device->name, "Nintendo RVL-CNT", 16) == 0)) {
                WUDiMoveDeviceToHead(device);
            }
            WUDSetSniffMode(device->bdAddr, 8);
            if (cb->hidConnCallback != NULL) {
                cb->hidConnCallback(device, 1);
            }
        } else {
            WUDiDebugPrint("error code: %d\n", data->conn.status);
            if (cb->syncState != 0) {
                stored = WUDiGetStoredDeviceInfo();
                if (memcmp(data->conn.bdAddr, stored->bdAddr, 6) == 0 && stored->linkState == 2) {
                    if (WUDiGetDevInfo(data->conn.bdAddr) != NULL && data->conn.status == 12) {
                        WUDiRemoveDevice(data->conn.bdAddr);
                        cb->linkNumber--;
                    }
                    cb->syncState = 0xFF;
                }
            } else if (WUDiGetDevInfo(data->conn.bdAddr) != NULL && data->conn.status == 12) {
                device = WUDiGetDevInfo(data->conn.bdAddr);
                if (device != NULL) {
                    if (device->type == 3 || device->type == 1) {
                        WUDiMoveWbcToTail(device);
                    } else {
                        WUDiMoveDeviceToTail(device);
                    }
                }
                WUDiRemoveDevice(data->conn.bdAddr);
                cb->linkNumber--;
            }
        }
        break;
    case 3:
        WUDiDebugPrint("BTA_HH_CLOSE_EVT\n");
        cb->wbcLinkNumber--;
        WUDiDebugPrint("device handle : %d   status = %d\n", data->devStatus.handle, data->devStatus.status);
        device = WUDiGetDevInfo(_WUDGetDevAddrUnchecked(data->devStatus.handle));
        if (device != NULL) {
            if (device->type == 3 || device->type == 1) {
                WUDiInsertWbcNode(device);
            } else if (!WUDIsLinkedWBC() || (WUDIsLinkedWBC() && memcmp(device->name, "Nintendo RVL-CNT", 16) == 0)) {
                WUDiInsertDeviceNode(device);
            }
        }
        _WUDSetDevAddr(data->devStatus.handle, NULL);
        _WUDSetQueuedSize(data->devStatus.handle, 0);
        _WUDSetNotAckedSize(data->devStatus.handle, 0);
        if (cb->hidConnCallback != NULL) {
            cb->hidConnCallback(device, 0);
        }
        break;
    case 11:
        WUDiDebugPrint("BTA_HH_ADD_DEV_EVT\n");
        WUDiDebugPrint("result: %d, handle: %d, addr: %02x:%02x:%02x:%02x:%02x:%02x\n", data->conn.status,
                       data->conn.handle, data->conn.bdAddr[0], data->conn.bdAddr[1], data->conn.bdAddr[2],
                       data->conn.bdAddr[3], data->conn.bdAddr[4], data->conn.bdAddr[5]);
        device = WUDiGetDevInfo(data->conn.bdAddr);
        device->handle = data->conn.handle;
        _WUDSetDevAddr(data->conn.handle, device->bdAddr);
        _WUDSetQueuedSize(data->conn.handle, 0);
        _WUDSetNotAckedSize(data->conn.handle, 0);
        break;
    case 12:
        WUDiDebugPrint("BTA_HH_RMV_DEV_EVT\n");
        WUDiDebugPrint("result: %d, handle: %d, addr: %02x:%02x:%02x:%02x:%02x:%02x\n", data->conn.status,
                       data->conn.handle, data->conn.bdAddr[0], data->conn.bdAddr[1], data->conn.bdAddr[2],
                       data->conn.bdAddr[3], data->conn.bdAddr[4], data->conn.bdAddr[5]);
        break;
    case 15:
        cb->bufferHead = data->buffer.head;
        cb->bufferTail = data->buffer.tail;
        if (cb->linkNumber < data->buffer.count) {
            OSReport("WARNING: link num count is modified.\n");
            cb->linkNumber = data->buffer.count;
        }
        for (i = 0; i < data->buffer.count; i++) {
            if (data->buffer.entries[i].handle < 16) {
                _WUDSetQueuedSize(data->buffer.entries[i].handle, data->buffer.entries[i].queued);
                _WUDSetNotAckedSize(data->buffer.entries[i].handle, data->buffer.entries[i].notAcked);
            }
        }
        break;
    case 5:
        WUDiDebugPrint("BTA_HH_SET_RPT_EVT\n");
        break;
    case 4:
        WUDiDebugPrint("BTA_HH_GET_RPT_EVT\n");
        break;
    case 7:
        WUDiDebugPrint("BTA_HH_SET_PROTO_EVT\n");
        break;
    case 6:
        WUDiDebugPrint("BTA_HH_GET_PROTO_EVT\n");
        break;
    case 9:
        WUDiDebugPrint("BTA_HH_SET_IDLE_EVT\n");
        break;
    case 8:
        WUDiDebugPrint("BTA_HH_GET_IDLE_EVT\n");
        break;
    case 10:
        WUDiDebugPrint("BTA_HH_GET_DCSP_EVT\n");
        break;
    case 13:
        WUDiDebugPrint("BTA_HH_VS_UNPLUG_EVT\n");
        break;
    }
}

/* Passes a HID data report to the receive callback when the application id is the Wii remote's; logs any other id. */
void bta_hh_co_data(u8 handle, u8* report, u16 length, u8 mode, u8 subClass, u8 appId)
{
    WUDCB* cb = &wudCB;

    if (appId == 3) {
        if (cb->hidRecvCallback != NULL) {
            cb->hidRecvCallback(handle, report, length);
        }
    } else {
        WUDiDebugPrint("Invalid app_id [%d]\n", appId);
    }
}

/* Logs a HID host open. */
void bta_hh_co_open(void)
{
    WUDiDebugPrint("bta_hh_co_open()\n");
}

/* Logs a HID host close. */
void bta_hh_co_close(void)
{
    WUDiDebugPrint("bta_hh_co_close()\n");
}

/* Returns zero. */
s32 bta_dm_co_get_compress_memory(void)
{
    return 0;
}
