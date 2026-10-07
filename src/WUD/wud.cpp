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
 *   `__wudInitSub` and the NAND callbacks `WUDiNand{Open,Seek,Write,Close}Callback`.  GUESS names: `WUDiDebugPrint` (the print helper),
 *   `WUDiGetStoredDeviceInfo`, `WUDiFlushDeviceInfo`, `_WUDSetDevAddr`, `_WUDGetDevAddrUnchecked`, `_WUDSetQueuedSize`,
 *   `_WUDSetNotAckedSize`, `WUDiReturnZero`, the alarm handlers `WUDi{Sync,Clear,InitFlush,Init,Shutdown}AlarmHandler` and their
 *   fibers `WUDi{Sync,Clear,InitFlush,Init,Shutdown}Fiber` (each handler switches to the fiber on `wudFiberStack`; which state machine
 *   each pair drives is read from the callers), `WUDiPatchCallback`, `WUDiPatchStepDone`, `WUDiFirmwareDoneCallback`, `WUDiStartSync`; the other
 *   functions still carry generated names.
 * RESIDUALS. Written: 56 of 96 rows at 100 %.  Not attempted: the 0x804F9C60..0x804FA2B0 NAND and pairing helpers, the five
 *   state machine fibers (0x804FA750 `WUDiSyncFiber`, 0x804FB570, 0x804FB9F0, 0x804FC7C0, 0x804FC950), 0x804FB360, 0x804FBBB0,
 *   0x804FC290, 0x804FCA30, `WUDInit` and `WUDShutdown` (they call unnamed Bluetooth stack functions at 0x8047A630 and
 *   0x8047B128), 0x804FDA80..0x804FE2D0 (the stack setup and firmware patch calls), `__wudInitSub`, 0x804FE170,
 *   0x804FE530..0x804FECB0 (the stack event handlers), 0x804FEF10, 0x804FF470, 0x804FF680, 0x804FF980, 0x804FFCE0 and
 *   0x80500130; `wudCB` names only the fields the written bodies read.  Relocations to the .data strings and .sdata
 *   flags name the target's `lbl_` objects (the unit claims no .data); every written call target matches.  The HID host callbacks (0x80500130..0x80500720: "BTA_HH_ENABLE_EVT", `bta_hh_co_*`, jump table 0x8062EFB0) are
 *   probably a second file; no data seam separates them from the rest (one .bss object 0x8075EAA0 is read on both sides),
 *   so the unit is kept whole.
 */

#include "types.h"
#include "WUD/wud.h"
#include "BTE/gki_buffer.h"
#include "SC/SCGetWpadMotorMode.h"
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
    /* +0x012 */ u8 pad_0x012[0xD2];
    /* +0x0E4 */ WUDDevice devices[10];
    /* +0x4A4 */ WUDDevice wbcDevices[6];
    /* +0x6E4 */ u8 pad_0x6E4;
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

/* .bss */
static WUDCB wudCB;
static u8 wudStoredDeviceInfo[0x60];
static u8 wudBtDeviceInfo[0x468];
static u8* wudDevAddrTable[0x68];
static u16 wudQueuedSize[0x10];
static u16 wudNotAckedSize[0x10];
static u8 wudFiberStack[0x1000];

/* .sbss */
static s32 wudInitialized;
static s32 wudStopRequested;
static s32 wudPatchStep;
static u8 wudPatchBusy;
static s8 wudSyncAux;
static s32 wudLinkedWbc;
static u8 wudDeviceInfoDirty;

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
u8* WUDiGetStoredDeviceInfo(void)
{
    return wudStoredDeviceInfo;
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
