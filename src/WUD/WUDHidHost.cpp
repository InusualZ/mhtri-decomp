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
 * RESIDUALS. `WUDiHidHostEventCallback` 0x80500130 (0x580) is unwritten; .data does not compare until it is (its strings and
 *   jump table).
 */

#include "types.h"
#include "WUD/WUDHidHost.h"
#include "WUD/wud.h"
#include "WUD/debug_msg.h"

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
