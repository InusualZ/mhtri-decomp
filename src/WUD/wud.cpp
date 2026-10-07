/* WUD/wud.cpp - the Wii remote device manager over the Bluetooth stack: device table, pairing/sync and the HID host callbacks.
 * RANGE. .text 0x804F9B30-0x80500770 (96 functions); .data 0x8062DBF0-0x8062F030; .bss 0x8075EAA0-0x80760C48;
 *   .sdata 0x807941C8-0x807941E0; .sbss 0x80795720-0x80795758; .sdata2 0x8079D478-0x8079D480.
 *   Edges: the left edge 0x804F9B30 (`WUDIsLinkedWBC`) is the first reader of .sbss 0x80795738 and the first .data
 *   object (0x8062DBF0) is read from 0x804F9DC0; the `WUD.c` __FILE__ string (.sdata 0x807941D8) and the " %s\n" string
 *   (0x807941C8) are one copy each read from 0x804FA750 to 0x804FF980; the right edge 0x80500770 is the first of
 *   three nw4r functions (`nw4r/db_console.cpp`).  The last function 0x80500720 is the print helper called from
 *   0x804F9B60 onward.
 * FLAGS. `cflags_base` (-O4,p); the print helper `WUDiDebugPrint` is defined after its callers so that the empty body is not inlined.
 * NAMES. the map's names (`WUDGetBufferStatus`, `WUDSetSniffMode`, `WUDSetVisibility`, `WUDiGetDevInfo`,
 *   `WUDDeviceStatusCallback`, `WUDSetDeviceHistory`, `_WUDGetDevAddr`, `_WUDGetQueuedSize`, `_WUDGetNotAckedSize`,
 *   `_WUDGetLinkNumber`, `App_MEMalloc`, `App_MEMfree`, `bta_hh_co_data`, `bta_hh_co_open`, `bta_hh_co_close`, ...);
 *   the log strings name `SyncFlushCallback`, `DeleteFlushCallback`, `InitFlushCallback`, `ShutdownFlushCallback`, `CleanupCallback`,
 *   `__wudInitSub`, `__wudAppendRuntimePatch`, `RemovePatchCallback`, `SuperPeekPokeCallback`, `WUDiRemoveDevice`,
 *   `__wudPowerMangeEventStackCallback` and the NAND callbacks `WUDiNand{Open,Seek,Write,Close}Callback`.  GUESS names:
 *   `WUDiDebugPrint` (the print helper), `WUDiGetStoredDeviceInfo`, `WUDiFlushDeviceInfo`, `_WUDSetDevAddr`, `_WUDGetDevAddrUnchecked`,
 *   `_WUDSetQueuedSize`, `_WUDSetNotAckedSize`, `WUDiReturnZero`, the alarm handlers `WUDi{Sync,Clear,InitFlush,Init,Shutdown}AlarmHandler`
 *   and their fibers `WUDi{Sync,Clear,InitFlush,Init,Shutdown}Fiber`, `WUDiPatchCallback`, `WUDiPatchStepDone`, `WUDiFirmwareDoneCallback`,
 *   `WUDiStartSync`, `WUDiAddDevice` (BTA_DmAddDevice and BTA_HhAddDev), `WUDiMove{Wbc,Device}To{Head,Tail}` (the recently-used list
 *   moves), `WUDiSyncCheckInquiry`, `WUDiSyncCheckSearch`, `WUDiSyncDone`, `WUDiSearchCallback` (the five search event strings),
 *   `WUDiPatchRecordCallback`, `WUDiPatchWriteCallback`, `WUDiVendorEventCallback`; the BTE entry points `BTA_Init`, `bta_sys_set_trace_level`,
 *   `BTM_SetDefaultLinkPolicy`, `BTM_SetDefaultLinkSuperTout` (16-byte setters of one u16 each, the dump name is stale), and the variables
 *   `wudInquiryResult`, `wudFirmwareData`, `wudStackReady`, `wudTraceLevel`, `wudWbcInitCallback`, `wudInquiryRssi`, `wudInquiryCount` and
 *   the `wudPatch*` state; the other functions still carry generated names.
 * RESIDUALS. Written: 76 of 96 rows have a body (58 at 100 %).  Partial rows: `WUDiSyncDone` (device pointer strength reduction and
 *   a smaller register save), `WUDiSearchCallback`, `RemovePatchCallback`, `WUDiPatchWriteCallback`, `WUDiPatchRecordCallback`
 *   (u8 clamp and unrolled copy register numbering), `WUDiMove*` (cb/device/i register order) and `WUDiAddDevice` (level/device
 *   registers); the data-relative string offsets differ until the unit's .data matches.  Not attempted (20 rows): 0x804F9F30,
 *   0x804FA2B0 (the SC device array writers), `WUDiSyncFiber` 0x804FA750, 0x804FB360, `WUDiClearFiber` 0x804FB570, 0x804FB8E0,
 *   `WUDiInitFlushFiber` 0x804FB9F0, 0x804FBBB0, 0x804FC290, `WUDiInitFiber` 0x804FC7C0, `WUDiShutdownFiber` 0x804FC950, 0x804FCA30,
 *   `WUDShutdown` 0x804FCF40 (it fills the SC arrays in .bss 0x80760A00..), 0x804FE770, 0x804FEB20, 0x804FECB0 (list inserts with
 *   shared tails), 0x804FEF10, `WUDiVendorEventCallback` 0x804FF680, 0x804FF980 and 0x80500130.  The unit's .data holds the strings and
 *   tables of those rows, the .sdata flags and the remaining .sbss/.bss objects, so its data sections do not compare yet;
 *   `wudCB` names only the fields the written bodies read.  The file order of the functions is not the address order.  The HID host
 *   callbacks (0x80500130..0x80500720: "BTA_HH_ENABLE_EVT", `bta_hh_co_*`, jump table 0x8062EFB0) are probably a second file; no data
 *   seam separates them from the rest (one .bss object 0x8075EAA0 is read on both sides), so the unit is kept whole.
 * SHAPES. `wudFirmwareData` is one 0x21C-byte initialised object: the HID report descriptor, the patch address and length words
 *   followed by the patch code, and the 13-byte record table whose first byte is the record count; the patch writers index
 *   `patchImage[8 + n]` and `patchTable[n * 0xD + 1]` as the target does.  The vendor command buffers are stack arrays.
 */

#include "types.h"
#include "WUD/wud.h"
#include "BTE/gki_buffer.h"
#include "SC/SCGetWpadMotorMode.h"
#include "SC/sc.h"
#include "OS/OSError.h"
#include "OS/OSAlarm.h"
#include "OS/OSSetAlarm.h"
#include "OS/OSSetPeriodicAlarm.h"
#include "OS/OSSwitchFiberEx.h"
#include "OS/OS.h"
#include "OS/OSTime.h"
#include "OS/OSDisableInterrupts.h"
#include "OS/OSRestoreInterrupts.h"
#include "Runtime.PPCEABI.H/memcpy.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "MSL_C/alloc.h"

/* size: 0x750 - the device manager's state: the pairing and clear state machines, the stack state and the callbacks. */
typedef struct WUDCB {
    /* +0x000 */ WUDResultCallback syncCallback;
    /* +0x004 */ WUDResultCallback simpleCallback;
    /* +0x008 */ WUDResultCallback clearCallback;
    /* +0x00C */ u8 syncState;
    /* +0x00D */ u8 clearState;
    /* +0x00E */ u8 stateE;
    /* +0x00F */ u8 stateF;
    /* +0x010 */ u8 state10;
    /* +0x011 */ u8 state11;
    /* +0x012 */ u8 remoteCount; /* GUESS: devices of the remote types in the table */
    /* +0x013 */ u8 otherCount;
    /* +0x014 */ WUDDeviceNode* wbcHead;
    /* +0x018 */ WUDDeviceNode* wbcTail;
    /* +0x01C */ WUDDeviceNode wbcNodes[6];
    /* +0x064 */ WUDDeviceNode* deviceHead;
    /* +0x068 */ WUDDeviceNode* deviceTail;
    /* +0x06C */ WUDDeviceNode deviceNodes[10];
    /* +0x0E4 */ WUDDevice devices[10];
    /* +0x4A4 */ WUDDevice wbcDevices[6];
    /* +0x6E4 */ u8 wbcLinkNumber; /* GUESS */
    /* +0x6E5 */ u8 linkNumber;
    /* +0x6E6 */ u8 syncRetries;
    /* +0x6E7 */ s8 quickSearch;
    /* +0x6E8 */ s8 searching;
    /* +0x6E9 */ s8 simple;
    /* +0x6EA */ u8 connectable;
    /* +0x6EB */ u8 discoverable;
    /* +0x6EC */ WUDHidRecvCallback hidRecvCallback;
    /* +0x6F0 */ WUDHidConnCallback hidConnCallback;
    /* +0x6F4 */ void* (*alloc)(u32 size); /* untyped: caller-owned block */
    /* +0x6F8 */ s32 (*dealloc)(void* block); /* untyped: the caller-owned block */
    /* +0x6FC */ u8 pad_0x6FC[0xC];
    /* +0x708 */ s8 status;
    /* +0x709 */ u8 pad_0x709;
    /* +0x70A */ u8 hciHandle;
    /* +0x70B */ u8 pad_0x70B[5];
    /* +0x710 */ OSAlarm alarm;
    /* +0x740 */ u8 pad_0x740[4];
    /* +0x744 */ u16 bufferHead;
    /* +0x746 */ u16 bufferTail;
    /* +0x748 */ u16 searchTime;
    /* +0x74A */ u16 searchTimeMax;
    /* +0x74C */ u8 pad_0x74C[4];
} WUDCB; /* size: 0x750 */

/* size: 0x108 - the last inquiry result: the device's address and its name. */
typedef struct WUDInquiryResult {
    /* +0x000 */ u8 bdAddr[6];
    /* +0x006 */ u8 name[0x40];
    /* +0x046 */ u8 pad_0x46[0xBA];
    /* +0x100 */ u32 services;
    /* +0x104 */ u8 pad_0x104[4];
} WUDInquiryResult; /* size: 0x108 */

/* .bss */
static WUDCB wudCB;
static WUDInquiryResult wudInquiryResult;
static WUDDevice wudStoredDeviceInfo;
static u8 wudBtDeviceInfo[0x468];
static u8* wudDevAddrTable[0x68];
static u16 wudQueuedSize[0x10];
static u16 wudNotAckedSize[0x10];
static u8 wudFiberStack[0x1000];

/* .sbss */
static s32 wudInitialized;
static s32 wudStackReady;
static s32 wudStopRequested;
static s32 wudPatchStep;
static u8 wudPatchBusy;
static u8 wudPatchRemoveParam;
static u8 wudTraceLevel;
static u8 wudInquiryRssi;
static u8 wudInquiryCount;
static u8 wudPatchRecordIndex;
static u8 wudPatchRecordCount;
static u32 wudPatchAddress;
static u32 wudPatchSent;
static u32 wudPatchLength;
static s8 wudSyncAux;
static void (*wudWbcInitCallback)(void);
static s32 wudLinkedWbc;
static u8 wudDeviceInfoDirty;

/* size: 0x21C - the Bluetooth host's HID report descriptor and the firmware patch data of the BCM2045 controller. */
typedef struct WUDFirmwareData {
    /* +0x000 */ u8 hidDescriptor[0xD9]; /* the Wii remote's HID report descriptor */
    /* +0x0D9 */ u8 pad_0xD9[3];
    /* +0x0DC */ u8 setupCommand[9]; /* vendor command 0xFC0A parameters */
    /* +0x0E5 */ u8 pad_0xE5[3];
    /* +0x0E8 */ u8 patchImage[0xBC]; /* the patch address and length (little endian words), then the patch code */
    /* +0x1A4 */ u8 patchTable[0x78]; /* the record count, then 13-byte records of the vendor command 0xFC4F stream */
} WUDFirmwareData; /* size: 0x21C */

/* .data */
static WUDFirmwareData wudFirmwareData = {
    { /* +0x000 hidDescriptor */
        0x05, 0x01, 0x09, 0x05, 0xA1, 0x01, 0x85, 0x10, 0x15, 0x00, 0x26, 0xFF,
        0x00, 0x75, 0x08, 0x95, 0x01, 0x06, 0x00, 0xFF, 0x09, 0x01, 0x91, 0x00,
        0x85, 0x11, 0x95, 0x01, 0x09, 0x01, 0x91, 0x00, 0x85, 0x12, 0x95, 0x02,
        0x09, 0x01, 0x91, 0x00, 0x85, 0x13, 0x95, 0x01, 0x09, 0x01, 0x91, 0x00,
        0x85, 0x14, 0x95, 0x01, 0x09, 0x01, 0x91, 0x00, 0x85, 0x15, 0x95, 0x01,
        0x09, 0x01, 0x91, 0x00, 0x85, 0x16, 0x95, 0x15, 0x09, 0x01, 0x91, 0x00,
        0x85, 0x17, 0x95, 0x06, 0x09, 0x01, 0x91, 0x00, 0x85, 0x18, 0x95, 0x15,
        0x09, 0x01, 0x91, 0x00, 0x85, 0x19, 0x95, 0x01, 0x09, 0x01, 0x91, 0x00,
        0x85, 0x1A, 0x95, 0x01, 0x09, 0x01, 0x91, 0x00, 0x85, 0x20, 0x95, 0x06,
        0x09, 0x01, 0x81, 0x00, 0x85, 0x21, 0x95, 0x15, 0x09, 0x01, 0x81, 0x00,
        0x85, 0x22, 0x95, 0x04, 0x09, 0x01, 0x81, 0x00, 0x85, 0x30, 0x95, 0x02,
        0x09, 0x01, 0x81, 0x00, 0x85, 0x31, 0x95, 0x05, 0x09, 0x01, 0x81, 0x00,
        0x85, 0x32, 0x95, 0x0A, 0x09, 0x01, 0x81, 0x00, 0x85, 0x33, 0x95, 0x11,
        0x09, 0x01, 0x81, 0x00, 0x85, 0x34, 0x95, 0x15, 0x09, 0x01, 0x81, 0x00,
        0x85, 0x35, 0x95, 0x15, 0x09, 0x01, 0x81, 0x00, 0x85, 0x36, 0x95, 0x15,
        0x09, 0x01, 0x81, 0x00, 0x85, 0x37, 0x95, 0x15, 0x09, 0x01, 0x81, 0x00,
        0x85, 0x3D, 0x95, 0x15, 0x09, 0x01, 0x81, 0x00, 0x85, 0x3E, 0x95, 0x15,
        0x09, 0x01, 0x81, 0x00, 0x85, 0x3F, 0x95, 0x15, 0x09, 0x01, 0x81, 0x00,
        0xC0,
    },
    { /* +0x0D9 pad_0xD9 */
        0x00, 0x00, 0x00,
    },
    { /* +0x0DC setupCommand */
        0x05, 0x00, 0x9A, 0x0E, 0x00, 0x01, 0x00, 0x00, 0x00,
    },
    { /* +0x0E5 pad_0xE5 */
        0x00, 0x00, 0x00,
    },
    { /* +0x0E8 patchImage */
        0x70, 0x99, 0x08, 0x00, 0xB4, 0x00, 0x00, 0x00, 0x88, 0x43, 0xD1, 0x07,
        0x09, 0x0C, 0x08, 0x43, 0xA0, 0x62, 0x19, 0x23, 0xDB, 0x01, 0x33, 0x80,
        0x7C, 0xF7, 0x88, 0xF8, 0x28, 0x76, 0x80, 0xF7, 0x17, 0xFF, 0x43, 0x78,
        0xEB, 0x70, 0x19, 0x23, 0xDB, 0x01, 0x33, 0x87, 0x7C, 0xF7, 0xBC, 0xFB,
        0x0B, 0x60, 0xA3, 0x7B, 0x01, 0x49, 0x0B, 0x60, 0x90, 0xF7, 0x96, 0xFB,
        0xD8, 0x1D, 0x08, 0x00, 0x00, 0xF0, 0x04, 0xF8, 0x00, 0x23, 0x79, 0xF7,
        0xE3, 0xFA, 0x00, 0x00, 0x00, 0xB5, 0x00, 0x23, 0x11, 0x49, 0x0B, 0x60,
        0x1D, 0x21, 0xC9, 0x03, 0x0B, 0x60, 0x7D, 0x20, 0x80, 0x01, 0x01, 0x38,
        0xFD, 0xD1, 0x0E, 0x4B, 0x0E, 0x4A, 0x13, 0x60, 0x47, 0x20, 0x00, 0x21,
        0x96, 0xF7, 0x96, 0xFF, 0x46, 0x20, 0x00, 0x21, 0x96, 0xF7, 0x92, 0xFF,
        0x0A, 0x4A, 0x13, 0x68, 0x0A, 0x48, 0x03, 0x40, 0x13, 0x60, 0x0A, 0x4A,
        0x13, 0x68, 0x0A, 0x48, 0x03, 0x40, 0x13, 0x60, 0x09, 0x4A, 0x13, 0x68,
        0x09, 0x48, 0x03, 0x40, 0x13, 0x60, 0x00, 0xBD, 0x24, 0x80, 0x0E, 0x00,
        0x81, 0x03, 0x0F, 0xFE, 0x5C, 0x00, 0x0F, 0x00, 0x60, 0xFC, 0x0E, 0x00,
        0xFE, 0xFF, 0x00, 0x00, 0xFC, 0xFC, 0x0E, 0x00, 0xFF, 0x9F, 0x00, 0x00,
        0x30, 0xFC, 0x0E, 0x00, 0x7F, 0xFF, 0x00, 0x00,
    },
    { /* +0x1A4 patchTable */
        0x07, 0x20, 0xBC, 0x65, 0x01, 0x00, 0x84, 0x42, 0x09, 0xD2, 0x84, 0x42,
        0x09, 0xD1, 0x21, 0x84, 0x5A, 0x00, 0x00, 0x83, 0xF0, 0x74, 0xFF, 0x09,
        0x0C, 0x08, 0x43, 0x22, 0x00, 0x61, 0x00, 0x00, 0x83, 0xF0, 0x40, 0xFC,
        0x00, 0x00, 0x00, 0x00, 0x23, 0xCC, 0x9F, 0x01, 0x00, 0x6F, 0xF0, 0xE4,
        0xFC, 0x03, 0x28, 0x7D, 0xD1, 0x24, 0x3C, 0x62, 0x01, 0x00, 0x28, 0x20,
        0x00, 0xE0, 0x60, 0x8D, 0x23, 0x68, 0x25, 0x04, 0x12, 0x01, 0x00, 0x20,
        0x1C, 0x20, 0x1C, 0x24, 0xE0, 0xB0, 0x21, 0x26, 0x74, 0x2F, 0x00, 0x00,
        0x86, 0xF0, 0x18, 0xFD, 0x21, 0x4F, 0x3B, 0x60, 0x30, 0x36, 0x08, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    },

};

/* The balance board linked flag. */
s32 WUDIsLinkedWBC(void)
{
    return wudLinkedWbc;
}

/* Hands the stack a block from the registered allocator. */
void* App_MEMalloc(u32 size) /* untyped: caller-owned block */
{
    WUDiDebugPrint("App_MEMalloc\n");
    return wudCB.alloc(size);
}

/* Gives a block back to the registered deallocator. */
void App_MEMfree(void* block) /* untyped: the caller-owned block */
{
    WUDiDebugPrint("App_MEMfree\n");
    wudCB.dealloc(block);
}

/* Moves the sync state machine on after the device table is flushed. */
void SyncFlushCallback(s32 result)
{
    WUDiDebugPrint("__wudSyncFlushCallback() : %d, Sync: %d\n", result, wudCB.syncState);
    if (wudCB.syncState != 0) {
        if (result == 0) {
            wudCB.syncState = 0x17;
            return;
        }
        wudCB.syncState = 0xFF;
    }
}

/* Stops advertising the host and returns the next pairing step: 0xE when no search runs or the links are full, 1 after
 * switching a sniffing device back to active mode, otherwise 0x1D with the search time rearmed. */
s32 WUDiSyncCheckSearch(void)
{
    WUDCB* cb = &wudCB;
    u32 level;
    u8 linkNumber;
    u8 wbcLinkNumber;

    level = OSDisableInterrupts();
    cb->discoverable = 0;
    cb->connectable = 0;
    OSRestoreInterrupts(level);
    BTA_DmSetVisibility(0, 0);
    if (cb->searching == 0) {
        return 0xE;
    }
    level = OSDisableInterrupts();
    linkNumber = cb->linkNumber;
    OSRestoreInterrupts(level);
    if (linkNumber == 4) {
        level = OSDisableInterrupts();
        wbcLinkNumber = cb->wbcLinkNumber;
        OSRestoreInterrupts(level);
        if (wbcLinkNumber == 4) {
            return 0xE;
        }
    }
    if ((u8)cb->quickSearch != 0) {
        s32 i;

        for (i = 0; i < 0x10; i++) {
            WUDDevice* device;

            level = OSDisableInterrupts();
            if ((u32)i <= 9) {
                device = (WUDDevice*)((u8*)cb->devices + i * sizeof(WUDDevice));
            } else {
                device = &cb->wbcDevices[i - 10];
            }
            OSRestoreInterrupts(level);
            if (device->linkState == 9) {
                BtmPmPwrMode mode;

                mode.mode = 0;
                mode.maxInterval = 0;
                mode.minInterval = 0;
                mode.attempt = 1;
                mode.timeout = 0;
                BTM_SetPowerMode(cb->hciHandle, device->bdAddr, &mode);
                return 1;
            }
        }
    }
    if (cb->searching > 0) {
        cb->searching--;
    }
    cb->searchTime = 0x32;
    return 0x1D;
}

/* Opens the HID connection of the stored device and returns the next pairing step for the device the inquiry found. */
s32 WUDiSyncCheckInquiry(void)
{
    s32 next = 0xFF;

    if (memcmp(wudInquiryResult.name, "Nintendo RVL-CNT", 0x10) == 0) {
        wudStoredDeviceInfo.linkState = 2;
        BTA_HhOpen(wudStoredDeviceInfo.bdAddr, 0, 0x12);
        next = 6;
    }
    if (wudLinkedWbc != 0 && memcmp(wudInquiryResult.name, "Nintendo RVL-WBC", 0x10) == 0) {
        WUDDevice* wbc = NULL;
        WUDDeviceNode* node;
        u32 level = OSDisableInterrupts();
        WUDCB* cb = &wudCB;

        for (node = cb->deviceHead; node != NULL; node = node->next) {
            WUDDevice* device = node->device;
            if (memcmp(device->name, "Nintendo RVL-WBC", 0x10) == 0) {
                wbc = device;
            }
        }
        OSRestoreInterrupts(level);
        if (wbc != NULL) {
            WUDiDebugPrint("Found the registered WBC in database\n");
            if (wbc->linkState > 1) {
                return next;
            }
            if (memcmp(wudInquiryResult.bdAddr, wbc->bdAddr, 6) != 0) {
                WUDiDebugPrint("Removed WBC data\n");
                WUDiMoveDeviceToTail(wbc);
                WUDiRemoveDevice(wbc->bdAddr);
            }
        }
        wudStoredDeviceInfo.linkState = 2;
        BTA_HhOpen(wudStoredDeviceInfo.bdAddr, 0, 0x12);
        next = 6;
    }
    return next;
}

/* Handles the result of the NAND open of the stored-device file: lets the state machines go on or marks the failure. */
void WUDiNandOpenCallback(s32 result)
{
    u8 state;

    if (wudCB.syncState != 0) {
        state = 0x19;
        if (result == 0) {
            state = 0x65;
        }
        wudCB.syncState = state;
    }
    if (wudCB.clearState != 0) {
        state = 5;
        if (result == 0) {
            state = 0x65;
        }
        wudCB.clearState = state;
    }
    wudPatchBusy = 0;
    WUDiDebugPrint("NANDOpen. [%d]\n", result);
}

/* Handles the result of the NAND seek in the stored-device file. */
void WUDiNandSeekCallback(s32 result)
{
    u8 syncNext;
    u8 clearNext;

    if (wudCB.syncState != 0) {
        syncNext = 0x19;
        if ((u32)(result - 0x40000) == 0xAF18) {
            syncNext = 0x66;
        }
        wudCB.syncState = syncNext;
    }
    if (wudCB.clearState != 0) {
        clearNext = 5;
        if ((u32)(result - 0x40000) == 0xAF18) {
            clearNext = 0x66;
        }
        wudCB.clearState = clearNext;
    }
    wudPatchBusy = 0;
    WUDiDebugPrint("NANDSeek. [%d]\n", result);
}

/* Handles the result of the NAND write of the stored-device file. */
void WUDiNandWriteCallback(s32 result)
{
    u8 state;

    if (wudCB.syncState != 0) {
        state = 0x19;
        if (result == 0x84) {
            state = 0x67;
        }
        wudCB.syncState = state;
    }
    if (wudCB.clearState != 0) {
        state = 5;
        if (result == 0x84) {
            state = 0x67;
        }
        wudCB.clearState = state;
    }
    wudPatchBusy = 0;
    WUDiDebugPrint("NANDWrite. [%d]\n", result);
}

/* Handles the result of the NAND close of the stored-device file. */
void WUDiNandCloseCallback(s32 result)
{
    if (wudCB.syncState != 0) {
        wudCB.syncState = 0x19;
    }
    if (wudCB.clearState != 0) {
        wudCB.clearState = 5;
    }
    wudPatchBusy = 0;
    WUDiDebugPrint("NANDClose. [%d]\n", result);
}

/* Finishes a pairing search: wakes a sniffing remote, otherwise stops the alarm, makes the host connectable again and reports
 * the result through the sync callback; returns 0xE while a remote is being woken, else 0. */
s32 WUDiSyncDone(void)
{
    WUDCB* cb = &wudCB;
    WUDSyncCallback callback;
    u32 level;

    if ((u8)cb->quickSearch != 0) {
        s32 i;

        for (i = 0; i < 0x10; i++) {
            WUDDevice* device;

            level = OSDisableInterrupts();
            if ((u32)i <= 9) {
                device = (WUDDevice*)((u8*)cb->devices + i * sizeof(WUDDevice));
            } else {
                device = &cb->wbcDevices[i - 10];
            }
            OSRestoreInterrupts(level);
            if (device->linkState == 8) {
                BtmPmPwrMode mode;

                mode.mode = 2;
                mode.maxInterval = 8;
                mode.minInterval = 8;
                mode.attempt = 1;
                mode.timeout = 0;
                BTM_SetPowerMode(cb->hciHandle, device->bdAddr, &mode);
                return 0xE;
            }
        }
    }
    OSCancelAlarm(&cb->alarm);
    if (wudStopRequested == 0) {
        level = OSDisableInterrupts();
        cb->discoverable = 0;
        cb->connectable = 1;
        OSRestoreInterrupts(level);
        BTA_DmSetVisibility(0, 1);
    }
    if ((u8)cb->simple == 0) {
        callback = (WUDSyncCallback)cb->syncCallback;
    } else {
        callback = (WUDSyncCallback)cb->simpleCallback;
    }
    if (callback != NULL) {
        callback(1, cb->syncRetries);
    }
    WUDiDebugPrint("Pairing Done\n");
    return 0;
}

/* Runs the pairing fiber on the fiber stack when the pairing alarm fires. */
void WUDiSyncAlarmHandler(OSAlarm* alarm, OSContext* context)
{
    OSSwitchFiberEx((u32)alarm, (u32)context, 0, 0, WUDiSyncFiber, wudFiberStack + sizeof(wudFiberStack));
}

/* Moves the clear state machine on after the device table is flushed. */
void DeleteFlushCallback(s32 result)
{
    WUDiDebugPrint("__wudDeleteFlushCallback() : %d, Delete: %d\n", result, wudCB.clearState);
    if (wudCB.clearState != 0) {
        wudCB.clearState = 8;
    }
}

/* Drops the link of every connected device and reports that the termination is done. */
s32 WUDiTerminateDevice(void)
{
    s32 i;
    WUDDevice* device;
    WUDCB* cb = &wudCB;

    device = cb->devices;
    for (i = 0; i < 10; i++) {
        if (device->linkState > 1) {
            btm_remove_acl(device->bdAddr);
        }
        device++;
    }
    device = cb->wbcDevices;
    for (i = 0; i < 6; i++) {
        if (device->linkState > 1) {
            btm_remove_acl(device->bdAddr);
        }
        device++;
    }
    return 3;
}

/* Runs the clear fiber on the fiber stack when the clear alarm fires. */
void WUDiClearAlarmHandler(OSAlarm* alarm, OSContext* context)
{
    OSSwitchFiberEx((u32)alarm, (u32)context, 0, 0, WUDiClearFiber, wudFiberStack + sizeof(wudFiberStack));
}

/* Runs the stack initialisation fiber on the fiber stack. */
void WUDiInitFlushAlarmHandler(OSAlarm* alarm, OSContext* context)
{
    OSSwitchFiberEx((u32)alarm, (u32)context, 0, 0, WUDiInitFlushFiber, wudFiberStack + sizeof(wudFiberStack));
}

/* Moves the initialisation state machine on after the device table is flushed. */
void InitFlushCallback(s32 result)
{
    WUDiDebugPrint("__wudInitFlushCallback() : %d, Init: %d\n", result, wudCB.state10);
    wudCB.state10 = 5;
}

/* Advances the firmware patch sequence one step; `result` is the vendor command's status. */
void WUDiPatchCallback(s32 result)
{
    wudPatchBusy = 0;
    switch (wudPatchStep) {
    case 1:
        wudPatchStep = (result == 0) ? wudPatchStep + 1 : 0xFF;
        return;
    case 2:
        wudPatchStep = (result == 0) ? wudPatchStep + 1 : 5;
        return;
    case 3:
        wudPatchStep = ((u32)(result - 0x40000) == 0xB000) ? wudPatchStep + 1 : 5;
        return;
    default:
        wudPatchStep = 6;
        return;
    }
}

/* Advances the firmware patch sequence by one step. */
void WUDiPatchStepDone(void)
{
    wudPatchBusy = 0;
    wudPatchStep++;
}

/* Runs the stack bring-up fiber on the fiber stack. */
void WUDiInitAlarmHandler(OSAlarm* alarm, OSContext* context)
{
    OSSwitchFiberEx((u32)alarm, (u32)context, 0, 0, WUDiInitFiber, wudFiberStack + sizeof(wudFiberStack));
}

/* Moves the shutdown state machine on after the device table is flushed. */
void ShutdownFlushCallback(s32 result)
{
    WUDiDebugPrint("__wudShutdownFlushCallback() : %d, Shutdown: %d\n", result, wudCB.state11);
    wudCB.state11 = 3;
}

/* Runs the shutdown fiber on the fiber stack. */
void WUDiShutdownAlarmHandler(OSAlarm* alarm, OSContext* context)
{
    OSSwitchFiberEx((u32)alarm, (u32)context, 0, 0, WUDiShutdownFiber, wudFiberStack + sizeof(wudFiberStack));
}

/* Brings the Bluetooth stack up and arms the periodic alarm that drives the state machines; returns 0 when it is already up. */
s32 WUDInit(void)
{
    WUDCB* cb = &wudCB;
    u64 now;

    if (wudInitialized != 0) {
        return 0;
    }
    WUDiDebugPrint("BTA_Init() is started\n");
    BTA_Init();
    bta_sys_set_trace_level(wudTraceLevel);
    L2CA_SetTraceLevel(wudTraceLevel);
    SDP_SetTraceLevel(wudTraceLevel);
    WUDiDebugPrint("BTA_Init() is done\n");
    cb->syncCallback = NULL;
    cb->simpleCallback = NULL;
    cb->clearCallback = NULL;
    cb->hidConnCallback = NULL;
    cb->hidRecvCallback = NULL;
    cb->state10 = 1;
    SCInit();
    if (wudLinkedWbc != 0 && wudWbcInitCallback != NULL) {
        wudWbcInitCallback();
    }
    OSCreateAlarm(&cb->alarm);
    now = OSGetTime();
    OSSetPeriodicAlarm(&cb->alarm, now, ((OS_BUS_CLOCK / 4) / 1000) * 10, WUDiShutdownAlarmHandler);
    wudInitialized = 1;
    wudDeviceInfoDirty = 0;
    return 1;
}

/* Stores the stack's allocate and free callbacks. */
void WUDRegisterAllocator(void* (*alloc)(u32 size), s32 (*dealloc)(void* block)) /* untyped: the caller-owned block */
{
    u32 level = OSDisableInterrupts();
    wudCB.alloc = alloc;
    wudCB.dealloc = dealloc;
    OSRestoreInterrupts(level);
}

/* Returns the stack state with interrupts off. */
s32 WUDGetStatus(void)
{
    u32 level = OSDisableInterrupts();
    s8 status = wudCB.status;
    OSRestoreInterrupts(level);
    return status;
}

/* Returns how many send buffers are in flight. */
u8 WUDGetBufferStatus(void)
{
    u32 level = OSDisableInterrupts();
    u8 count = wudCB.bufferTail - wudCB.bufferHead;
    OSRestoreInterrupts(level);
    return count;
}

/* Requests sniff mode with `interval` for the device at `bdAddr`. */
void WUDSetSniffMode(const u8* bdAddr, s16 interval)
{
    WUDCB* cb = &wudCB;
    BtmPmPwrMode mode;

    if (interval > 0) {
        mode.mode = 2;
    } else {
        mode.mode = 0;
    }
    mode.maxInterval = interval;
    mode.minInterval = interval;
    mode.attempt = 1;
    mode.timeout = 0;
    BTM_SetPowerMode(cb->hciHandle, bdAddr, &mode);
}

/* Installs the pairing result callback and returns the old one. */
WUDResultCallback WUDSetSyncDeviceCallback(WUDResultCallback callback)
{
    WUDResultCallback previous;
    u32 level;

    WUDiDebugPrint("WUDSetSyncDeviceCallback\n");
    level = OSDisableInterrupts();
    previous = wudCB.syncCallback;
    wudCB.syncCallback = callback;
    OSRestoreInterrupts(level);
    return previous;
}

/* Installs the simple pairing result callback and returns the old one. */
WUDResultCallback WUDSetSyncSimpleCallback(WUDResultCallback callback)
{
    WUDResultCallback previous;
    u32 level;

    WUDiDebugPrint("WUDSetSyncDeviceCallback\n");
    level = OSDisableInterrupts();
    previous = wudCB.simpleCallback;
    wudCB.simpleCallback = callback;
    OSRestoreInterrupts(level);
    return previous;
}

/* Installs the clear result callback and returns the old one. */
WUDResultCallback WUDSetClearDeviceCallback(WUDResultCallback callback)
{
    WUDResultCallback previous;
    u32 level;

    WUDiDebugPrint("WUDSetClearDeviceCallback\n");
    level = OSDisableInterrupts();
    previous = wudCB.clearCallback;
    wudCB.clearCallback = callback;
    OSRestoreInterrupts(level);
    return previous;
}

/* Returns non-zero while a pairing, clear or stack bring-up step is running. */
static inline s32 wudIsBusy(WUDCB* cb)
{
    u32 level = OSDisableInterrupts();
    if (cb->syncState == 0 && cb->clearState == 0 && cb->stateF == 4 && cb->state10 == 6) {
        OSRestoreInterrupts(level);
        return 0;
    }
    OSRestoreInterrupts(level);
    return 1;
}

/* Starts the pairing state machine: `simple` selects the simple pairing, `searchMode` the search kind, `fast` the quick search. */
s32 WUDiStartSync(s8 simple, s8 searchMode, s8 aux, s32 fast)
{
    s32 started = 0;
    WUDCB* cb = &wudCB;
    u32 level;
    s32 status;
    s8 quick;
    u64 now;

    level = OSDisableInterrupts();
    status = cb->status;
    OSRestoreInterrupts(level);
    if ((u32)status == 3) {
        if (wudIsBusy(cb) == 0) {
            quick = fast != 0;
            level = OSDisableInterrupts();
            wudSyncAux = aux;
            cb->syncState = 1;
            cb->searching = searchMode;
            cb->simple = simple;
            cb->quickSearch = quick;
            cb->syncRetries = 0;
            cb->searchTime = 0x32;
            cb->searchTimeMax = 0xC8;
            OSCreateAlarm(&cb->alarm);
            now = OSGetTime();
            OSSetPeriodicAlarm(&cb->alarm, now, ((OS_BUS_CLOCK / 4) / 1000) * 20, WUDiSyncAlarmHandler);
            OSRestoreInterrupts(level);
            started = 1;
        }
    }
    return started;
}

/* Starts pairing the Wii remotes. */
void WUDStartSyncDevice(void)
{
    WUDiStartSync(0, 3, 0, 0);
}

/* Starts the quick simple pairing. */
void WUDStartFastSyncSimple(void)
{
    WUDiStartSync(1, -1, 0, 1);
}

/* Cancels a running pairing search; non-zero when the stack was running. */
s32 WUDStopSyncDevice(void)
{
    u32 level;
    s32 stopped = 0;
    WUDCB* cb = &wudCB;

    wudStopRequested = 1;
    level = OSDisableInterrupts();
    if (cb->status == 3) {
        if (cb->searching != 0) {
            if (cb->syncState == 3) {
                BTA_DmSearchCancel();
            }
            cb->searching = 0;
        }
        stopped = 1;
    }
    OSRestoreInterrupts(level);
    return stopped;
}

/* Cancels a running simple pairing search; non-zero when the stack was running. */
s32 WUDStopSyncSimple(void)
{
    u32 level;
    s32 stopped = 0;
    WUDCB* cb = &wudCB;

    level = OSDisableInterrupts();

    if (cb->status == 3) {
        if (cb->searching != 0) {
            if (cb->syncState == 3) {
                BTA_DmSearchCancel();
            }
            cb->searching = 0;
        }
        stopped = 1;
    }
    OSRestoreInterrupts(level);
    return stopped;
}

/* Starts clearing the stored pairings. */
s32 WUDStartClearDevice(void)
{
    s32 started = 0;
    WUDCB* cb = &wudCB;
    u32 level;
    s32 status;
    u64 now;

    WUDiDebugPrint("WUDStartClearDevice()\n");
    level = OSDisableInterrupts();
    status = cb->status;
    OSRestoreInterrupts(level);
    if ((u32)status == 3) {
        if (wudIsBusy(cb) == 0) {
            level = OSDisableInterrupts();
            cb->clearState = 1;
            OSCreateAlarm(&cb->alarm);
            now = OSGetTime();
            OSSetPeriodicAlarm(&cb->alarm, now, ((OS_BUS_CLOCK / 4) / 1000) * 20, WUDiClearAlarmHandler);
            OSRestoreInterrupts(level);
            started = 1;
        }
    }
    return started;
}

/* Masks the Bluetooth channels around Wi-Fi channel `channel` (0 clears the mask); non-zero when the mask was applied. */
s32 WUDSetDisableChannel(s32 channel)
{
    s32 applied = 0;
    WUDCB* cb = &wudCB;
    s32 status;
    u32 level;
    s32 first;
    s32 last;

    WUDiDebugPrint("WUDSetDisableChannel()\n");
    if ((u8)channel > 13) {
        return 0;
    }
    level = OSDisableInterrupts();
    status = cb->status;
    OSRestoreInterrupts(level);
    if ((u32)status == 3) {
        if ((s8)channel == 0) {
            first = 0xFF;
            last = 0xFF;
        } else {
            s32 center = ((s8)channel + 1) * 5;
            first = center - 14;
            last = center + 14;
            if (first < 0) {
                first = 0;
            }
            if (last > 0x4E) {
                last = 0x4E;
            }
        }
        u8 afhStatus = BTM_SetAfhChannels(first, last);
        WUDiDebugPrint("BTM_SetAfhChannels() : %d\n", afhStatus);
        applied = 1;
    }
    return applied;
}

/* Logs that the firmware download finished and continues with the stack setup. */
void WUDiFirmwareDoneCallback(void)
{
    WUDiDebugPrint("Initialize the BCM2045 firmware complete.\n");
    __wudInitSub();
}

/* Sends the next batch of patch records to the controller, or resets it once the records are all sent or a command failed. */
void WUDiPatchRecordCallback(s32 result)
{
    u8 batch[0x100];
    const u8* table = wudFirmwareData.patchTable;
    s32 count;
    s32 size;

    if ((wudPatchRecordCount == wudPatchRecordIndex && result != 0) || result == 0) {
        WUDiDebugPrint("Reset again!\n");
        BTM_DeviceReset(WUDiFirmwareDoneCallback);
        return;
    }
    count = 0x13;
    if (wudPatchRecordCount - wudPatchRecordIndex < 0x13) {
        count = wudPatchRecordCount - wudPatchRecordIndex;
    }
    batch[0] = count;
    size = (u8)count * 0xD;
    memcpy(&batch[1], &table[wudPatchRecordIndex * 0xD + 1], size);
    wudPatchRecordIndex += (u8)count;
    WUDiDebugPrint("Install the patches\n");
    BTM_VendorSpecificCommand(0xFC4F, size + 1, batch, WUDiPatchRecordCallback);
}

/* Writes the next block of the patch code to the controller's RAM, then starts the record stream once the code is all sent. */
void WUDiPatchWriteCallback(s32 result)
{
    u8 batch[0x100];
    u8 command[4 + 0xFB];
    u8 chunk;
    s32 i;
    u32 address;

    if (result != 0) {
        if (wudPatchLength == wudPatchSent) {
            u8 count;
            s32 size;

            wudPatchRecordIndex = 0;
            wudPatchRecordCount = wudFirmwareData.patchTable[0];
            count = 0x13;
            if (wudFirmwareData.patchTable[0] < 0x13) {
                count = wudFirmwareData.patchTable[0];
            }
            size = count * 0xD;
            batch[0] = count;
            memcpy(&batch[1], &wudFirmwareData.patchTable[1], size);
            wudPatchRecordIndex += count;
            WUDiDebugPrint("Install the patches\n");
            BTM_VendorSpecificCommand(0xFC4F, size + 1, batch, WUDiPatchRecordCallback);
            return;
        }
        chunk = 0xFB;
        if ((u8)(wudPatchLength - wudPatchSent) < 0xFB) {
            chunk = wudPatchLength - wudPatchSent;
        }
        address = wudPatchAddress + wudPatchSent;
        command[0] = address;
        command[1] = address >> 8;
        command[2] = address >> 16;
        command[3] = address >> 24;
        for (i = 0; i < chunk; i++) {
            command[4 + i] = wudFirmwareData.patchImage[8 + wudPatchSent + i];
        }
        wudPatchSent += chunk;
        WUDiDebugPrint("Download the patch codes [%d]\n", wudPatchSent);
        BTM_VendorSpecificCommand(0xFC4C, chunk + 4, command, WUDiPatchWriteCallback);
        return;
    }
    WUDiDebugPrint("Reset again!\n");
    BTM_DeviceReset(WUDiFirmwareDoneCallback);
}

/* Starts the patch code download after the controller acknowledged the patch removal. */
void RemovePatchCallback(s32 result)
{
    u8 command[4 + 0xFB];
    const u8* image = wudFirmwareData.patchImage;
    u8 chunk;
    s32 i;

    WUDiDebugPrint("RemovePatch Callback\n");
    if (result != 0) {
        chunk = 0xFB;
        if (wudPatchLength < 0xFB) {
            chunk = wudPatchLength;
        }
        command[0] = wudPatchAddress;
        command[1] = wudPatchAddress >> 8;
        command[2] = wudPatchAddress >> 16;
        command[3] = wudPatchAddress >> 24;
        for (i = 0; i < chunk; i++) {
            command[4 + i] = image[8 + i];
        }
        wudPatchSent = chunk;
        WUDiDebugPrint("Download the patch codes [%d]\n", chunk);
        BTM_VendorSpecificCommand(0xFC4C, chunk + 4, command, WUDiPatchWriteCallback);
        return;
    }
    WUDiDebugPrint("Reset again!\n");
    BTM_DeviceReset(WUDiFirmwareDoneCallback);
}

/* Removes the previous runtime patch after the peek-poke command is acknowledged. */
void SuperPeekPokeCallback(s32 result)
{
    WUDiDebugPrint("SuperPeekPoke Callback\n");
    WUDiDebugPrint("RemovePatch\n");
    BTM_VendorSpecificCommand(0xFC4F, 1, &wudPatchRemoveParam, RemovePatchCallback);
}

/* Reads the patch address and length from the firmware data and starts the patch sequence. */
void __wudAppendRuntimePatch(void)
{
    WUDiDebugPrint("__wudAppendRuntimePatch()\n");
    wudPatchAddress = (((((wudFirmwareData.patchImage[3] << 8) + wudFirmwareData.patchImage[2]) << 8) + wudFirmwareData.patchImage[1]) << 8)
                      + wudFirmwareData.patchImage[0];
    wudPatchLength = (((((wudFirmwareData.patchImage[7] << 8) + wudFirmwareData.patchImage[6]) << 8) + wudFirmwareData.patchImage[5]) << 8)
                     + wudFirmwareData.patchImage[4];
    if (__OSInIPL != 0) {
        WUDiDebugPrint("SuperPeekPoke\n");
        BTM_VendorSpecificCommand(0xFC0A, 9, wudFirmwareData.setupCommand, SuperPeekPokeCallback);
        return;
    }
    WUDiDebugPrint("RemovePatch\n");
    BTM_VendorSpecificCommand(0xFC4F, 1, &wudPatchRemoveParam, RemovePatchCallback);
}

/* Names the host, registers the stack callbacks, re-adds the stored devices and makes the host connectable. */
void __wudInitSub(void)
{
    WUDCB* cb = &wudCB;
    char name[4] = "Wii";
    u8 deviceClass[3] = { 0x00, 0x04, 0x48 };
    s32 i;

    WUDiDebugPrint("start __wudInitSub()\n");
    BTA_DmSetDeviceName(name);
    BTM_SetDeviceClass(deviceClass);
    BTM_RegisterForVSEvents(WUDiVendorEventCallback);
    BTM_RegisterForDeviceStatusNotif(WUDDeviceStatusCallback);
    BTM_PmRegister(3, &cb->hciHandle, __wudPowerMangeEventStackCallback);
    BTM_WritePageTimeout(0x8000);
    BTM_SetDefaultLinkPolicy(5);
    BTM_SetDefaultLinkSuperTout(0xC80);
    for (i = 0; i < 10; i++) {
        if (cb->devices[i].linkState == 1) {
            WUDiAddDevice(cb->devices[i].bdAddr);
        }
    }
    for (i = 0; i < 6; i++) {
        if (cb->wbcDevices[i].linkState == 1) {
            WUDiAddDevice(cb->wbcDevices[i].bdAddr);
        }
    }
    {
        u32 level = OSDisableInterrupts();
        cb->status = 3;
        wudStackReady = 1;
        OSRestoreInterrupts(level);
    }
    {
        u32 level = OSDisableInterrupts();
        cb->discoverable = 0;
        cb->connectable = 1;
        OSRestoreInterrupts(level);
    }
    BTA_DmSetVisibility(0, 1);
}

/* Installs the HID report callback and returns the old one. */
WUDHidRecvCallback WUDSetHidRecvCallback(WUDHidRecvCallback callback)
{
    WUDHidRecvCallback previous;
    u32 level;

    WUDiDebugPrint("WUDSetHidRecvCallback()\n");
    level = OSDisableInterrupts();
    previous = wudCB.hidRecvCallback;
    wudCB.hidRecvCallback = callback;
    OSRestoreInterrupts(level);
    return previous;
}

/* Installs the HID connection callback and returns the old one. */
WUDHidConnCallback WUDSetHidConnCallback(WUDHidConnCallback callback)
{
    WUDHidConnCallback previous;
    u32 level;

    WUDiDebugPrint("WUDSetHidConnCallback()\n");
    level = OSDisableInterrupts();
    previous = wudCB.hidConnCallback;
    wudCB.hidConnCallback = callback;
    OSRestoreInterrupts(level);
    return previous;
}

/* Sets whether the host is discoverable and connectable. */
void WUDSetVisibility(u8 discoverable, u8 connectable)
{
    u32 level = OSDisableInterrupts();
    wudCB.discoverable = discoverable;
    wudCB.connectable = connectable;
    OSRestoreInterrupts(level);
    BTA_DmSetVisibility(discoverable, connectable);
}

/* Returns non-zero while a pairing, clear or stack bring-up step is running. */
s32 WUDIsSyncing(void)
{
    WUDCB* cb = &wudCB;
    u32 level = OSDisableInterrupts();
    if (cb->syncState == 0 && cb->clearState == 0 && cb->stateF == 4 && cb->state10 == 6) {
        OSRestoreInterrupts(level);
        return 0;
    }
    OSRestoreInterrupts(level);
    return 1;
}

/* Marks the stack as off once it is closed, or logs the failure. */
void CleanupCallback(s32 result)
{
    WUDCB* cb = &wudCB;

    if (result == 0) {
        wudInitialized = 0;
        cb->status = 0;
        return;
    }
    WUDiDebugPrint("WARNING: USB_CLOSE_FAILURE!\n");
}

/* Logs the stack's device status events and reports a fatal USB error. */
void WUDDeviceStatusCallback(u32 event)
{
    WUDiDebugPrint("__wudDeviceStatusEventStackCallback\n");
    if (event == 2) {
        OSReport("---- WARNING: USB FATAL ERROR! ----\n");
    }
}

/* Registers the device at `bdAddr` with the stack's device database and, for a remote or balance board, with the HID host. */
void WUDiAddDevice(const u8* bdAddr)
{
    WUDCB* cb = &wudCB;
    u32 level = OSDisableInterrupts();
    WUDDevice* device = WUDiGetDevInfo(bdAddr);
    u8 status = BTA_DmAddDevice(device->bdAddr, device->linkKey, 0, 0);

    WUDiDebugPrint("BTA_DmAddDevice(): %d\n", status);
    if (memcmp(device->name, "Nintendo RVL-CNT", 0x10) == 0 || (memcmp(device->name, "Nintendo RVL-WBC", 0x10) == 0 && wudLinkedWbc != 0)) {
        BtaHhDscpInfo dscp;
        dscp.length = sizeof(wudFirmwareData.hidDescriptor);
        dscp.descriptor = wudFirmwareData.hidDescriptor;
        WUDiDebugPrint("BTA_HhAddDev()\n");
        BTA_HhAddDev(device->bdAddr, device->attrMask, device->subClass, device->appId, dscp);
    }
    if (device->type == 0 || device->type == 4 || device->type == 2 || device->type == 5) {
        cb->remoteCount++;
    } else {
        cb->otherCount++;
    }
    OSRestoreInterrupts(level);
}

/* Removes the device at `bdAddr` from the HID host and the stack's device database and clears its record. */
void WUDiRemoveDevice(const u8* bdAddr)
{
    WUDCB* cb = &wudCB;
    WUDDevice* device;
    u32 level = OSDisableInterrupts();
    u8 status;

    WUDiDebugPrint("WUDiRemoveDevice : \n");
    device = WUDiGetDevInfo(bdAddr);
    if (device != NULL) {
        WUDiDebugPrint(" handle : %d,  addr : %02x:%02x:%02x:%02x:%02x:%02x\n", device->handle, device->bdAddr[0], device->bdAddr[1],
                       device->bdAddr[2], device->bdAddr[3], device->bdAddr[4], device->bdAddr[5]);
        WUDiDebugPrint("remove device info from database.\n");
        if (memcmp(device->name, "Nintendo RVL-CNT", 0x10) == 0 || (memcmp(device->name, "Nintendo RVL-WBC", 0x10) == 0 && wudLinkedWbc != 0)) {
            WUDiDebugPrint("BTA_HhRemoveDev()\n");
            WUDiDebugPrint(" handle : %d\n", device->handle);
            BTA_HhRemoveDev(device->handle);
        }
        status = BTA_DmRemoveDevice(device->bdAddr);
        WUDiDebugPrint("BTA_DmRemoveDevice(): %d\n", status);
        if (device->type == 0 || device->type == 2 || (u8)(device->type + 0xFC) <= 1) {
            cb->remoteCount--;
        } else {
            cb->otherCount--;
        }
        memset(device, 0, sizeof(WUDDevice));
    }
    OSRestoreInterrupts(level);
}

/* Finds the stored record of the device at `bdAddr` among the paired and the balance board devices, or NULL. */
WUDDevice* WUDiGetDevInfo(const u8* bdAddr)
{
    WUDCB* cb = &wudCB;
    WUDDevice* found = NULL;
    u32 level;
    s32 i;

    level = OSDisableInterrupts();
    for (i = 0; i < 10; i++) {
        if (memcmp(cb->devices[i].bdAddr, bdAddr, 6) == 0) {
            found = &cb->devices[i];
            break;
        }
    }
    if (found == NULL) {
        for (i = 0; i < 6; i++) {
            if (memcmp(cb->wbcDevices[i].bdAddr, bdAddr, 6) == 0) {
                found = &cb->wbcDevices[i];
                break;
            }
        }
    }
    OSRestoreInterrupts(level);
    return found;
}

/* Moves the balance board list entry of `device` to the head of the list. */
void WUDiMoveWbcToHead(WUDDevice* device)
{
    WUDCB* cb = &wudCB;
    u32 level = OSDisableInterrupts();
    s32 i;

    for (i = 0; i < 6; i++) {
        if (memcmp(cb->wbcNodes[i].device->bdAddr, device->bdAddr, 6) == 0) {
            WUDDeviceNode* node = &cb->wbcNodes[i];
            if (memcmp(cb->wbcHead->device->bdAddr, node->device->bdAddr, 6) != 0) {
                cb->wbcNodes[i].prev->next = cb->wbcNodes[i].next;
                if (memcmp(cb->wbcTail->device->bdAddr, node->device->bdAddr, 6) == 0) {
                    cb->wbcTail = cb->wbcNodes[i].prev;
                } else {
                    cb->wbcNodes[i].next->prev = cb->wbcNodes[i].prev;
                }
                cb->wbcNodes[i].next = cb->wbcHead;
                cb->wbcHead->prev = node;
                cb->wbcHead = node;
                cb->wbcNodes[i].prev = NULL;
            }
            break;
        }
    }
    OSRestoreInterrupts(level);
}

/* Moves the balance board list entry of `device` to the tail of the list. */
void WUDiMoveWbcToTail(WUDDevice* device)
{
    WUDCB* cb = &wudCB;
    u32 level = OSDisableInterrupts();
    s32 i;

    for (i = 0; i < 6; i++) {
        if (memcmp(cb->wbcNodes[i].device->bdAddr, device->bdAddr, 6) == 0) {
            WUDDeviceNode* node = &cb->wbcNodes[i];
            if (memcmp(cb->wbcTail->device->bdAddr, node->device->bdAddr, 6) != 0) {
                cb->wbcNodes[i].next->prev = cb->wbcNodes[i].prev;
                if (memcmp(cb->wbcHead->device->bdAddr, node->device->bdAddr, 6) == 0) {
                    cb->wbcHead = cb->wbcNodes[i].next;
                } else {
                    cb->wbcNodes[i].prev->next = cb->wbcNodes[i].next;
                }
                cb->wbcNodes[i].prev = cb->wbcTail;
                cb->wbcTail->next = node;
                cb->wbcTail = node;
                cb->wbcNodes[i].next = NULL;
            }
            break;
        }
    }
    OSRestoreInterrupts(level);
}

/* Moves the remote list entry of `device` to the head of the list. */
void WUDiMoveDeviceToHead(WUDDevice* device)
{
    WUDCB* cb = &wudCB;
    u32 level = OSDisableInterrupts();
    s32 i;

    for (i = 0; i < 10; i++) {
        if (memcmp(cb->deviceNodes[i].device->bdAddr, device->bdAddr, 6) == 0) {
            WUDDeviceNode* node = &cb->deviceNodes[i];
            if (memcmp(cb->deviceHead->device->bdAddr, node->device->bdAddr, 6) != 0) {
                cb->deviceNodes[i].prev->next = cb->deviceNodes[i].next;
                if (memcmp(cb->deviceTail->device->bdAddr, node->device->bdAddr, 6) == 0) {
                    cb->deviceTail = cb->deviceNodes[i].prev;
                } else {
                    cb->deviceNodes[i].next->prev = cb->deviceNodes[i].prev;
                }
                cb->deviceNodes[i].next = cb->deviceHead;
                cb->deviceHead->prev = node;
                cb->deviceHead = node;
                cb->deviceNodes[i].prev = NULL;
            }
            break;
        }
    }
    OSRestoreInterrupts(level);
}

/* Moves the remote list entry of `device` to the tail of the list. */
void WUDiMoveDeviceToTail(WUDDevice* device)
{
    WUDCB* cb = &wudCB;
    u32 level = OSDisableInterrupts();
    s32 i;

    for (i = 0; i < 10; i++) {
        if (memcmp(cb->deviceNodes[i].device->bdAddr, device->bdAddr, 6) == 0) {
            WUDDeviceNode* node = &cb->deviceNodes[i];
            if (memcmp(cb->deviceTail->device->bdAddr, node->device->bdAddr, 6) != 0) {
                cb->deviceNodes[i].next->prev = cb->deviceNodes[i].prev;
                if (memcmp(cb->deviceHead->device->bdAddr, node->device->bdAddr, 6) == 0) {
                    cb->deviceHead = cb->deviceNodes[i].next;
                } else {
                    cb->deviceNodes[i].prev->next = cb->deviceNodes[i].next;
                }
                cb->deviceNodes[i].prev = cb->deviceTail;
                cb->deviceTail->next = node;
                cb->deviceTail = node;
                cb->deviceNodes[i].next = NULL;
            }
            break;
        }
    }
    OSRestoreInterrupts(level);
}

/* Returns the Bluetooth address of the connected device `handle`, or NULL. */
u8* _WUDGetDevAddr(u8 handle)
{
    u8* addr;
    u32 level = OSDisableInterrupts();
    if (handle < 0x10) {
        addr = wudDevAddrTable[handle];
    } else {
        addr = NULL;
    }
    OSRestoreInterrupts(level);
    return addr;
}

/* Returns the bytes queued for device `handle`. */
u16 _WUDGetQueuedSize(s8 handle)
{
    u16 size;
    u32 level = OSDisableInterrupts();
    if ((u8)handle <= 0xF) {
        size = wudQueuedSize[(u8)handle];
    } else {
        size = 0;
    }
    OSRestoreInterrupts(level);
    return size;
}

/* Returns the bytes sent to device `handle` and not yet acknowledged. */
u16 _WUDGetNotAckedSize(s8 handle)
{
    u16 size;
    u32 level = OSDisableInterrupts();
    if ((u8)handle <= 0xF) {
        size = wudNotAckedSize[(u8)handle];
    } else {
        size = 0;
    }
    OSRestoreInterrupts(level);
    return size;
}

/* Returns the number of connected devices. */
u8 _WUDGetLinkNumber(void)
{
    u32 level = OSDisableInterrupts();
    u8 count = wudCB.linkNumber;
    OSRestoreInterrupts(level);
    return count;
}

/* Returns the stored-device work area. */
WUDDevice* WUDiGetStoredDeviceInfo(void)
{
    return &wudStoredDeviceInfo;
}

/* Handles the stack's device search events: logs an inquiry hit, stores a discovery hit, and ends or cancels the search. */
void WUDiSearchCallback(s32 event, const u8* data)
{
    WUDCB* cb = &wudCB;

    switch (event) {
    case 0: {
        u16 timeout;

        WUDiDebugPrint("INQUIRY RESULT: %02x:%02x:%02x:%02x:%02x:%02x   %02x%02x%02x   %d\n", data[0], data[1], data[2], data[3], data[4],
                       data[5], data[6], data[7], data[8], (s8)data[9]);
        wudInquiryRssi = data[9];
        if ((u8)cb->quickSearch == 1) {
            timeout = 0x1900;
        } else if ((s32)cb->quickSearch == 0) {
            u32 level = OSDisableInterrupts();
            u8 linkNumber = cb->linkNumber;
            OSRestoreInterrupts(level);
            if (linkNumber < 3) {
                timeout = 0x1900;
            } else {
                timeout = 0x8000;
            }
        } else {
            timeout = 0x8000;
        }
        BTM_WritePageTimeout(timeout);
        return;
    }
    case 1:
        WUDiDebugPrint("INQUIRY_COMPLETED\n");
        return;
    case 2:
        memcpy(wudInquiryResult.bdAddr, data, 6);
        memcpy(wudInquiryResult.name, data + 6, 0x40);
        wudInquiryResult.services = ((const WUDInquiryResult*)data)->services;
        wudInquiryCount++;
        WUDiDebugPrint("DISCOVER RESULT:  %02x:%02x:%02x:%02x:%02x:%02x   %s (%04x)\n", wudInquiryResult.bdAddr[0],
                       wudInquiryResult.bdAddr[1], wudInquiryResult.bdAddr[2], wudInquiryResult.bdAddr[3], wudInquiryResult.bdAddr[4],
                       wudInquiryResult.bdAddr[5], wudInquiryResult.name, wudInquiryResult.services);
        return;
    case 3:
        WUDiDebugPrint("DISCOVER COMPLETED\n");
        cb->syncState = 4;
        return;
    case 4:
        WUDiDebugPrint("SEARCH CANCEL\n\n");
        BTM_VendorSpecificCommand(0xFC4C, 0x1C, &wudFirmwareData.patchTable[0x5C], NULL);
        wudInquiryCount = 0;
        memset(&wudInquiryResult, 0, sizeof(wudInquiryResult));
        cb->syncState = 4;
        return;
    default:
        WUDiDebugPrint("Warning: Search Callback returns invalid event\n");
        return;
    }
}

/* Records the connection type the controller reports for a device (8 = active, 9 = sniff). */
void __wudPowerMangeEventStackCallback(const u8* bdAddr, u32 status, u16 value, u8 hciStatus)
{
    WUDDevice* device;

    WUDiDebugPrint("__wudPowerMangeEventStackCallback\n");
    WUDiDebugPrint("hci_status = %d\n", hciStatus);
    device = WUDiGetDevInfo(bdAddr);
    if (device == NULL) {
        device = &wudStoredDeviceInfo;
        if (memcmp(device->bdAddr, bdAddr, 6) != 0) {
            WUDiDebugPrint("Unknown device is connected and changes the connection type!!!!\n");
            WUDiDebugPrint(" addr = %02x:%02x:%02x:%02x:%02x:%02x,  status = %d\n", bdAddr[0], bdAddr[1], bdAddr[2], bdAddr[3], bdAddr[4],
                           bdAddr[5], status);
            return;
        }
    }
    switch (status) {
    case 0:
        device->linkState = 8;
        break;
    case 2:
        device->linkState = 9;
        break;
    }
    WUDiDebugPrint(" addr = %02x:%02x:%02x:%02x:%02x:%02x,  status = %d\n", device->bdAddr[0], device->bdAddr[1], device->bdAddr[2],
                   device->bdAddr[3], device->bdAddr[4], device->bdAddr[5], device->linkState);
}

/* Stores `bdAddr` (or clears the entry when NULL) as the history entry of channel `chan` and marks the table dirty. */
void WUDSetDeviceHistory(s32 chan, const u8* bdAddr)
{
    if (bdAddr == NULL) {
        memset(&wudBtDeviceInfo[(chan + 10) * 0x46 + 1], 0, 0x46);
    } else {
        memcpy(&wudBtDeviceInfo[(chan + 10) * 0x46 + 1], bdAddr, 6);
    }
    wudDeviceInfoDirty = 1;
}

/* Returns whether channel `chan` already stores `bdAddr`. */
s32 WUDIsHistoryAddr(s32 chan, const u8* bdAddr)
{
    if (bdAddr == NULL) {
        return 0;
    }
    return memcmp(&wudBtDeviceInfo[(chan + 10) * 0x46 + 1], bdAddr, 6) == 0;
}

/* Writes the device table back to the console settings when it changed. */
void WUDiFlushDeviceInfo(void)
{
    if (wudDeviceInfoDirty != 0 && SCSetBtDeviceInfoArray(wudBtDeviceInfo) != 0) {
        SCFlushAsync(NULL);
        wudDeviceInfoDirty = 0;
    }
}

/* Stores the Bluetooth address of the connected device `handle`. */
void _WUDSetDevAddr(u8 handle, u8* addr)
{
    wudDevAddrTable[handle] = addr;
}

/* Returns the Bluetooth address of device `handle` without the range check. */
u8* _WUDGetDevAddrUnchecked(u8 handle)
{
    return wudDevAddrTable[handle];
}

/* Stores the bytes queued for device `handle`. */
void _WUDSetQueuedSize(u8 handle, u16 size)
{
    wudQueuedSize[handle] = size;
}

/* Stores the bytes sent to device `handle` and not yet acknowledged. */
void _WUDSetNotAckedSize(u8 handle, u16 size)
{
    wudNotAckedSize[handle] = size;
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
s32 WUDiReturnZero(void)
{
    return 0;
}

/* Takes a log line and discards it; every WUD log call goes through here. */
void WUDiDebugPrint(const char* format, ...)
{
}
