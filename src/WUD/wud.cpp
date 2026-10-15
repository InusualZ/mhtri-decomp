/* WUD/wud.cpp - the Wii remote device manager over the Bluetooth stack: device table, pairing/sync and the stack callbacks.
 * RANGE. .text 0x804F9B30-0x80500130 (90 functions, in address order); .data 0x8062DBF0-0x8062ED98; .bss 0x8075EAA0-0x80760C48;
 *   .sdata 0x807941C8-0x807941E0; .sbss 0x80795720-0x80795758; .sdata2 0x8079D478-0x8079D480.
 *   Edges: the left edge 0x804F9B30 (`WUDIsLinkedWBC`) is the first reader of .sbss 0x80795738 and the first .data
 *   object (0x8062DBF0) is read from 0x804F9DC0; the `WUD.c` __FILE__ string (.sdata 0x807941D8) and the " %s\n" string
 *   (0x807941C8) are one copy each read from 0x804FA750 to 0x804FF980.  The right edge 0x80500130 / .data 0x8062ED98 is the
 *   seam with WUD/WUDHidHost.cpp (that header carries the evidence: the `-str reuse` duplicate "Nintendo RVL-CNT" and the
 *   out-of-line calls of this unit's small functions).
 *   Four functions the linker strips are written for their log strings, which the target's .data keeps in source order:
 *   `WUDStartSyncSpDevice` (between `WUDStartFastSyncSimple` and `WUDStopSyncDevice`), `_WUDEnableTestMode`,
 *   `_WUDStartSyncDevice` and `_WUDDeleteStoreDevice` (after `_WUDSetNotAckedSize`); their bodies are GUESSES.
 * FLAGS. `cflags_base` (-O4,p); the print helper `WUDiDebugPrint` is WUD/debug_msg.cpp's, so every log call is out of line.
 *   Scoped pragmas, each measured on its own row: `opt_strength_reduction off` keeps the `i * 12` / `i * 0x60` index
 *   arithmetic (`__wudClearControlBlock`, `WUDiInsertWbcNode`, `WUDiInsertDeviceNode`, `WUDiUpdateWiiFitSaveData`,
 *   `WUDiSyncRegisterDevice`); `opt_common_subs off` (`WUDiInsertWbcNode`, `WUDiInsertDeviceNode`, `WUDiLoadWiiFitSaveData`,
 *   `WUDiSyncRegisterDevice`); `opt_propagation off` (`WUDiSyncRegisterDevice`); `optimization_level 3` (`WUDiSyncFiber`,
 *   `WUDiLoadDeviceInfo`).
 * NAMES. the map's names (`WUDGetBufferStatus`, `WUDSetSniffMode`, `WUDSetVisibility`, `WUDiGetDevInfo`,
 *   `WUDDeviceStatusCallback`, `WUDSetDeviceHistory`, `_WUDGetDevAddr`, `_WUDGetQueuedSize`, `_WUDGetNotAckedSize`,
 *   `_WUDGetLinkNumber`, `App_MEMalloc`, `App_MEMfree`, ...);
 *   the log strings name `SyncFlushCallback`, `DeleteFlushCallback`, `InitFlushCallback`, `ShutdownFlushCallback`, `CleanupCallback`,
 *   `__wudInitSub`, `__wudAppendRuntimePatch`, `RemovePatchCallback`, `SuperPeekPokeCallback`, `WUDiRemoveDevice`,
 *   `__wudPowerMangeEventStackCallback`, `__wudClearControlBlock`, the NAND callbacks `WUDiNand{Open,Seek,Write,Close}Callback`
 *   and the stripped `WUDStartSyncSpDevice`, `_WUDEnableTestMode`, `_WUDStartSyncDevice`, `_WUDDeleteStoreDevice`.
 *   GUESS names: `WUDiGetStoredDeviceInfo`, `WUDiFlushDeviceInfo`, `_WUDSetDevAddr`, `_WUDGetDevAddrUnchecked`,
 *   `_WUDSetQueuedSize`, `_WUDSetNotAckedSize`, the alarm handlers `WUDi{Sync,Clear,InitFlush,Init,Shutdown}AlarmHandler`
 *   and their fibers `WUDi{Sync,Clear,InitFlush,Init,Shutdown}Fiber`, `WUDiPatchCallback`, `WUDiPatchStepDone`, `WUDiFirmwareDoneCallback`,
 *   `WUDiStartSync`, `WUDiAddDevice` (BTA_DmAddDevice and BTA_HhAddDev), `WUDiMove{Wbc,Device}To{Head,Tail}` (the recently-used list
 *   moves), `WUDiSyncCheckInquiry`, `WUDiSyncCheckSearch`, `WUDiSyncDone`, `WUDiSearchCallback` (the five search event strings),
 *   `WUDiPatchRecordCallback`, `WUDiPatchWriteCallback`, `WUDiVendorEventCallback`, `__wudSecurityEventStackCallback` (BTA_DM_*
 *   strings), `__wudLinkKeyEventStackCallback` (BTM_CB_EVT_* strings), `WUDiDeleteAllLinkKeys`, `WUDiUpdateWiiFitSaveData` (the
 *   checksum over the 0xA0-byte `wudWiiFitSaveData`, seeded from `wudWiiFitCrc`), `WUDiStoreDeviceInfoToSC`, `WUDiSyncRegisterDevice`,
 *   `WUDiInsertWbcNode`, `WUDiInsertDeviceNode`, `WUDiInsertDeviceNodeTail`, `WUDiLoadDeviceInfo` (rebuilds the device table from
 *   the console settings' arrays), `WUDiLoadWiiFitSaveData` (the RPHealth.dat read and checksum); the BTE entry points `BTA_Init`,
 *   `bta_sys_set_trace_level`, `BTM_SetDefaultLinkPolicy`, `BTM_SetDefaultLinkSuperTout` (16-byte setters of one u16 each, the
 *   dump name is stale) and `BTA_DisableBluetooth` (0x8047D718, after `BTA_EnableBluetooth` in the bta_dm_api order); the
 *   variables `wudInquiryResult`, `wudStackReady`, `wudTraceLevel`, `wudWbcInitCallback`, `wudInquiryRssi`, `wudInquiryCount`,
 *   `wudWbcWorkArea` (the 0x4B000-byte, 32-aligned save file buffer) and the `wudPatch*` state; the .data arrays
 *   `wudHidDescriptor`, `wudSetupCommand`, `wudPatchImage`, `wudPatchTable` and `wudSearchCancelCommand`; the `WUDCB` fields `linkKeyAddr`, `hostAddr`,
 *   `vendorEventStatus`, `syncRssiThreshold`, `enabledProfiles`, `linkKeyState`, `initFlushState`, `initState`, `shutdownState`,
 *   `wbcCount` and the `WUDDevice` fields `pinPending` and `linkKeyState` (WUD/wud.h); the .bss objects `wudBtCmpDeviceInfo`,
 *   `wudNandFileInfo`, `wudNandBlock` and the file-local types `WUDSc*`, `WUDWiiFitSaveData`, `WUDWiiFitFile`, `WUDPatchTable`,
 *   `WUDLinkKeyEvent`.
 * RESIDUALS. Every row has a body.
 *   `WUDiUpdateWiiFitSaveData`: the target folds `wudSetWiiFitSaveData`'s NULL and `&local != NULL` tests late and leaves dead
 *   `bl memcpy` calls behind unconditional branches; ours drops the NULL ones and keeps an `addic.` test of the local.
 *   `WUDiLoadWiiFitSaveData`: the step switch is a compare tree in the target and a linear compare chain here (six dense cases
 *   lower to a chain in this compiler; no source shape tried reproduces the tree).
 *   `WUDiSyncFiber`: the target materialises each case's next state as `li; b join` where ours speculates one value, and its
 *   inner switches are compare trees; the `searchTime` countdowns load with `lha` in the target (s16 fields measure lower here).
 *   `WUDiLoadDeviceInfo`: the inner search's exit test is threaded into the found/not-found jumps in the target.
 *   `__wudClearControlBlock`: the target's node cursor starts at `wudCB` with the field offset in the displacement.
 *   `WUDiSearchCallback`, `RemovePatchCallback`, `WUDiPatchWriteCallback`, `__wudAppendRuntimePatch`, `WUDiAddDevice`,
 *   `WUDiInsertWbcNode`, `WUDiInsertDeviceNode`, `WUDiDeleteAllLinkKeys`, `WUDiStoreDeviceInfoToSC`, `WUDiVendorEventCallback`,
 *   `__wudSecurityEventStackCallback`, `WUDiSyncRegisterDevice`: load scheduling and callee-saved register assignment.
 *   `WUDiMove*`, `WUDiSyncCheck*`, `WUDiSyncDone`, `WUDiInsertDeviceNodeTail`: register numbering only.
 *   .text: the object's linked functions are smaller than the 0x6600 claim by the partial rows' size gaps.
 *   .data: byte-identical to the target; dtk relocates the patch code word at 0x8062DD34 (`@etb_80010134+4`, request wud-slice1#10).
 * SHAPES. The .data head is five initialised arrays in definition order: the HID report descriptor, the controller set-up
 *   command, the patch image (address and length words, then the code), the patch records (`WUDPatchTable`, 0x5C bytes, so it
 *   stays 4-aligned where a 0x78-byte array would be 8-aligned) and the search cancel command; the patch writers index
 *   `wudPatchImage[8 + n]` and `wudPatchTable.records[n]` as the target does.  The vendor command buffers are stack arrays.
 *   The interrupt-guarded reads (`wudGetDeviceCount`, `wudGetLinkNumber`, `wudGetDeviceByIndex`, ...) and the SDK bodies the
 *   target inlines (`wudSetSniffMode`, `wudSetVisibility`, `wudStartClearDevice`, `wudGetNewDevice`, `wudCalcCrc`) are static
 *   inline helpers; `wudGetDeviceByIndex` takes a u32 index, which keeps the target's `mulli` for the balance board half.  A u32
 *   `level` holds the BOOL of `OSDisableInterrupts` (OS/OSInterrupt.h).  The security event payload is the device manager's
 *   `tBTA_DM_SEC` (BTE/bta_dm_int.h).  A case that only leaves the function is a `return` at the end of its switch, which keeps
 *   it in the compare tree (`WUDiClearFiber` case 7).
 */

#include "types.h"
#include "WUD/wud.h"
#include "WUD/WUDHidHost.h"
#include "WUD/debug_msg.h"
#include "BTE/gki_time.h"
#include "BTE/bta_dm_int.h"
#include "SC/SCGetWpadMotorMode.h"
#include "SC/sc.h"
#include "OS/OSError.h"
#include "OS/OSAlarm.h"
#include "OS/OSSetAlarm.h"
#include "OS/OSSetPeriodicAlarm.h"
#include "OS/OSSwitchFiberEx.h"
#include "OS/OS.h"
#include "OS/OSTime.h"
#include "OS/OSInterrupt.h"
#include "OS/__OSGetSystemTime.h"
#include "NAND/nand.h"
#include "NAND/NANDCore.h"
#include "SC/SCProductInfo.h"
#include "SC/SCApi.h"
#include "Runtime.PPCEABI.H/memcpy.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "MSL_C/alloc.h"

#ifdef __cplusplus
extern "C" {
#endif

/* This unit's functions WUD/wud.h does not declare (C linkage, like the rest of the unit). */
s32 WUDiSyncRegisterDevice(void);
s32 WUDiStoreDeviceInfoToSC(void);
s32 WUDiUpdateWiiFitSaveData(void);
u8 WUDiDeleteAllLinkKeys(void);
u8 WUDiLoadDeviceInfo(void);
u8 WUDiLoadWiiFitSaveData(void);
void __wudClearControlBlock(void);
void __wudSecurityEventStackCallback(u8 event, union BtaDmSecEventData* data);
void __wudLinkKeyEventStackCallback(void* result); /* untyped: the BTM command's payload, one type per command */

#ifdef __cplusplus
}
#endif

/* size: 0x108 - the last inquiry result: the device's address and its name. */
typedef struct WUDInquiryResult {
    /* +0x000 */ u8 bdAddr[6];
    /* +0x006 */ u8 name[0x40];
    /* +0x046 */ u8 pad_0x46[0xBA];
    /* +0x100 */ u32 services;
    /* +0x104 */ u8 pad_0x104[4];
} WUDInquiryResult; /* size: 0x108 */

/* size: 0x46 - one device of the console settings' device array: its address and its name. */
typedef struct WUDScDevice {
    /* +0x00 */ u8 bdAddr[6];
    /* +0x06 */ u8 name[0x40];
} WUDScDevice; /* size: 0x46 */

/* size: 0x468 - the console settings' device array (`SCSetBtDeviceInfoArray`): ten paired remotes, then the six
 * channels' history entries. */
typedef struct WUDScDeviceArray {
    /* +0x000 */ u8 count;
    /* +0x001 */ WUDScDevice devices[16];
    /* +0x461 */ u8 pad_0x461[7];
} WUDScDeviceArray; /* size: 0x468 */

/* size: 0x56 - one device of the console settings' balance board array: its address, name and link key. */
typedef struct WUDScCmpDevice {
    /* +0x00 */ u8 bdAddr[6];
    /* +0x06 */ u8 name[0x40];
    /* +0x46 */ u8 linkKey[0x10];
} WUDScCmpDevice; /* size: 0x56 */

/* size: 0x208 - the console settings' balance board array (`SCSetBtCmpDeviceInfoArray`). */
typedef struct WUDScCmpDeviceArray {
    /* +0x000 */ u8 count;
    /* +0x001 */ WUDScCmpDevice devices[6];
    /* +0x205 */ u8 pad_0x205[3];
} WUDScCmpDeviceArray; /* size: 0x208 */

/* size: 0xA0 - Wii Fit's copy of the balance board pairing: the board's address, link key and name, sealed by a checksum
 * over the first 0x80 bytes. */
typedef struct WUDWiiFitSaveData {
    /* +0x00 */ u8 bdAddr[6];
    /* +0x06 */ u8 linkKey[0x10];
    /* +0x16 */ u8 name[0x40];
    /* +0x56 */ u8 pad_0x56[0x2A];
    /* +0x80 */ u8 crc[4];
    /* +0x84 */ u8 pad_0x84[0x1C];
} WUDWiiFitSaveData; /* size: 0xA0 */

/* size: 0x4B000 - Wii Fit's save file as the work area holds it: the body, the balance board record and its checksum. */
typedef struct WUDWiiFitFile {
    /* +0x00000 */ u8 body[0x4AF18];
    /* +0x4AF18 */ u8 record[0x80];
    /* +0x4AF98 */ u8 crc[4];
    /* +0x4AF9C */ u8 pad_0x4AF9C[0x64];
} WUDWiiFitFile; /* size: 0x4B000 */

/* .bss */
WUDCB wudCB;
static WUDInquiryResult wudInquiryResult;
static WUDDevice wudStoredDeviceInfo;
static WUDScDeviceArray wudBtDeviceInfo;
static WUDWiiFitSaveData wudWiiFitSaveData;
static u8* wudDevAddrTable[0x10];
static NANDFileInfo wudNandFileInfo;
static NANDCommandBlock wudNandBlock;
static u8 wudFiberStack[0x1000] __attribute__((aligned(32)));
static WUDScCmpDeviceArray wudBtCmpDeviceInfo;
static u16 wudQueuedSize[0x10];
static u16 wudNotAckedSize[0x10];

/* .sbss */
static s32 wudInitialized;
static s32 wudStackReady;
static s32 wudStopRequested;
static u8 wudPatchRemoveParam;
static u8 wudTraceLevel;
static WUDWiiFitFile* wudWbcWorkArea;
static void (*wudWbcInitCallback)(void);
static s32 wudLinkedWbc;
static u32 wudWiiFitCrc;
static s32 wudPatchStep;
static u8 wudPatchBusy;
static u8 wudPatchRecordIndex;
static u8 wudPatchRecordCount;
static u32 wudPatchAddress;
static u32 wudPatchSent;
static u32 wudPatchLength;
static u8 wudDeviceInfoDirty;
static s8 wudSyncAux;
static u8 wudInquiryRssi;
static u8 wudInquiryCount;

/* .data */
/* The Wii remote's HID report descriptor, registered with the HID host. */
static u8 wudHidDescriptor[0xD9] = {
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
};

/* The parameters of vendor command 0xFC0A (the controller set-up before the patch download). */
static u8 wudSetupCommand[9] = {
        0x05, 0x00, 0x9A, 0x0E, 0x00, 0x01, 0x00, 0x00, 0x00,
};

/* The BCM2045 firmware patch: its address and length (little endian words), then the patch code. */
static u8 wudPatchImage[0xBC] = {
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
};

/* size: 0x5C - the patch records: the record count, then the 13-byte records of the vendor command 0xFC4F stream. */
typedef struct WUDPatchTable {
    /* +0x00 */ u8 count;
    /* +0x01 */ u8 records[7][0xD];
} WUDPatchTable; /* size: 0x5C */

/* The BCM2045 patch records. */
static WUDPatchTable wudPatchTable = {
        0x07, 0x20, 0xBC, 0x65, 0x01, 0x00, 0x84, 0x42, 0x09, 0xD2, 0x84, 0x42,
        0x09, 0xD1, 0x21, 0x84, 0x5A, 0x00, 0x00, 0x83, 0xF0, 0x74, 0xFF, 0x09,
        0x0C, 0x08, 0x43, 0x22, 0x00, 0x61, 0x00, 0x00, 0x83, 0xF0, 0x40, 0xFC,
        0x00, 0x00, 0x00, 0x00, 0x23, 0xCC, 0x9F, 0x01, 0x00, 0x6F, 0xF0, 0xE4,
        0xFC, 0x03, 0x28, 0x7D, 0xD1, 0x24, 0x3C, 0x62, 0x01, 0x00, 0x28, 0x20,
        0x00, 0xE0, 0x60, 0x8D, 0x23, 0x68, 0x25, 0x04, 0x12, 0x01, 0x00, 0x20,
        0x1C, 0x20, 0x1C, 0x24, 0xE0, 0xB0, 0x21, 0x26, 0x74, 0x2F, 0x00, 0x00,
        0x86, 0xF0, 0x18, 0xFD, 0x21, 0x4F, 0x3B, 0x60,
};

/* The vendor command 0xFC4C parameters sent when a device search is cancelled. */
static u8 wudSearchCancelCommand[0x1C] = {
    0x30, 0x36, 0x08,
};

/* The device record of table index `index`: the ten remotes, then the six balance boards; read with interrupts off. */
static inline WUDDevice* wudGetDeviceByIndex(u32 index)
{
    WUDCB* cb = &wudCB;
    WUDDevice* device;
    u32 level = OSDisableInterrupts();

    if (index <= 9) {
        device = &cb->devices[index];
    } else {
        device = &cb->wbcDevices[index - 10];
    }
    OSRestoreInterrupts(level);
    return device;
}

/* The number of paired remotes, read with interrupts off. */
static inline u8 wudGetDeviceCount(void)
{
    WUDCB* cb = &wudCB;
    u32 level = OSDisableInterrupts();
    u8 count = cb->remoteCount;

    OSRestoreInterrupts(level);
    return count;
}

/* The number of paired balance boards, read with interrupts off. */
static inline u8 wudGetWbcCount(void)
{
    WUDCB* cb = &wudCB;
    u32 level = OSDisableInterrupts();
    u8 count = cb->wbcCount;

    OSRestoreInterrupts(level);
    return count;
}

/* Returns non-zero while a pairing, clear or stack bring-up step is running. */
static inline s32 wudIsBusy(WUDCB* cb)
{
    u32 level = OSDisableInterrupts();
    if (cb->syncState == 0 && cb->clearState == 0 && cb->initFlushState == 4 && cb->initState == 6) {
        OSRestoreInterrupts(level);
        return 0;
    }
    OSRestoreInterrupts(level);
    return 1;
}

/* Requests sniff mode with `interval` (active mode when it is not positive) for the device at `bdAddr`. */
static inline void wudSetSniffMode(const u8* bdAddr, s16 interval)
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

/* Records and applies the host's discoverable and connectable flags. */
static inline void wudSetVisibility(u8 discoverable, u8 connectable)
{
    u32 level = OSDisableInterrupts();
    wudCB.discoverable = discoverable;
    wudCB.connectable = connectable;
    OSRestoreInterrupts(level);
    BTA_DmSetVisibility(discoverable, connectable);
}

/* Starts clearing the stored pairings when the stack is up and idle (the body `WUDStartClearDevice` has, for the callers
 * that inline it); returns non-zero when it started. */
static inline s32 wudStartClearDevice(void)
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

/* The number of open links, read with interrupts off. */
static inline u8 wudGetLinkNumber(void)
{
    WUDCB* cb = &wudCB;
    u32 level = OSDisableInterrupts();
    u8 count = cb->linkNumber;

    OSRestoreInterrupts(level);
    return count;
}

/* The record a new pairing goes to: the first free remote record, or the balance board list's tail record in simple
 * pairing; NULL when every remote record is in use. */
static inline WUDDevice* wudGetNewDevice(void)
{
    WUDCB* cb = &wudCB;
    WUDDevice* device;
    u32 level = OSDisableInterrupts();
    s32 i;

    if ((u8)cb->simple == 0) {
        device = NULL;
        for (i = 0; i < 10; i++) {
            if (cb->devices[i].linkState == 0) {
                WUDiDebugPrint("new entry index is %d\n", i);
                device = &cb->devices[i];
                break;
            }
        }
    } else {
        device = cb->wbcTail->device;
    }
    OSRestoreInterrupts(level);
    return device;
}

/* The paired balance board's record in the remote list, or NULL. */
static inline WUDDevice* wudFindLinkedWbc(void)
{
    WUDCB* cb = &wudCB;
    WUDDevice* wbc = NULL;
    WUDDeviceNode* node;
    u32 level = OSDisableInterrupts();

    for (node = cb->deviceHead; node != NULL; node = node->next) {
        if (memcmp(node->device->name, "Nintendo RVL-WBC", 0x10) == 0) {
            wbc = node->device;
        }
    }
    OSRestoreInterrupts(level);
    return wbc;
}

/* The Wii Fit save data checksum of `size` bytes at `data`, continued from `seed`: the sum of the halfwords in the high
 * half and the sum of their complements in the low half. */
static inline u32 wudCalcCrc(u32 seed, const u16* data, u32 size)
{
    u16 inverseSum = seed;
    u32 sum = seed >> 16;
    s32 i;

    for (i = 0; i < size / 2; i++) {
        u16 value = *data++;

        sum += value;
        inverseSum = inverseSum + ~value;
    }
    return (sum << 16) | inverseSum;
}

/* Stores the given parts of the balance board record in the Wii Fit save data and seals it with its checksum. */
static inline void wudSetWiiFitSaveData(const u8* bdAddr, const u8* linkKey, const u8* name)
{
    u32 crc;

    if (bdAddr != NULL) {
        memcpy(wudWiiFitSaveData.bdAddr, bdAddr, 6);
    }
    if (linkKey != NULL) {
        memcpy(wudWiiFitSaveData.linkKey, linkKey, 0x10);
    }
    if (name != NULL) {
        memcpy(wudWiiFitSaveData.name, name, 0x40);
    }
    crc = wudCalcCrc(wudWiiFitCrc, (u16*)&wudWiiFitSaveData, 0x80);
    memcpy(wudWiiFitSaveData.crc, &crc, 4);
}

/* The host's connectable flag, read with interrupts off. */
static inline u8 wudGetConnectable(void)
{
    WUDCB* cb = &wudCB;
    u32 level = OSDisableInterrupts();
    u8 connectable = cb->connectable;

    OSRestoreInterrupts(level);
    return connectable;
}

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

    wudSetVisibility(0, 0);
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
            WUDDevice* device = wudGetDeviceByIndex(i);

            if (device->linkState == 9) {
                wudSetSniffMode(device->bdAddr, 0);
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

/* Registers the device the pairing found: makes room in a full table (dropping a second balance board first), copies the
 * found record into a new one, adds it to the stack and the lists, and records a balance board in the history and the Wii
 * Fit save data; returns the next pairing step (7), or 0xFF when no record is free. */
#pragma push
#pragma opt_propagation off
#pragma opt_strength_reduction off
#pragma opt_common_subs off
s32 WUDiSyncRegisterDevice(void)
{
    WUDCB* cb = &wudCB;
    WUDDevice* device;
    WUDDevice* tail;

    if (wudLinkedWbc != 0 && memcmp(wudStoredDeviceInfo.name, "Nintendo RVL-CNT", 0x10) == 0) {
        if (wudFindLinkedWbc() != NULL && wudGetDeviceCount() == 10) {
            tail = cb->deviceTail->prev->device;
            WUDiMoveDeviceToTail(tail);
            WUDiRemoveDevice(tail->bdAddr);
        }
    }
    if (wudGetDeviceCount() == 10) {
        WUDiRemoveDevice(cb->deviceTail->device->bdAddr);
    }
    device = wudGetNewDevice();
    if (device == NULL) {
        return 0xFF;
    }
    if (device->linkState != 0) {
        return 0xFF;
    }
    memcpy(device, &wudStoredDeviceInfo, sizeof(WUDDevice));
    WUDiAddDevice(device->bdAddr);
    WUDiMoveDeviceToHead(device);
    if (wudLinkedWbc != 0 && memcmp(device->name, "Nintendo RVL-WBC", 0x10) == 0) {
        WUDiInsertDeviceNodeTail(device);
        memset(wudBtDeviceInfo.devices[14].bdAddr, 0, sizeof(WUDScDevice));
        memcpy(wudBtDeviceInfo.devices[14].bdAddr, device->bdAddr, 6);
        memcpy(wudBtDeviceInfo.devices[14].name, device->name, 0x40);
        memcpy(&wudBtDeviceInfo.devices[14].name[0x14], device->linkKey, 0x10);
        wudSetWiiFitSaveData(device->bdAddr, device->linkKey, device->name);
    }
    wudDevAddrTable[device->handle] = device->bdAddr;
    return 7;
}
#pragma pop

/* Whether the console settings are still busy with a read or write. */
static inline BOOL wudScIsBusy(void)
{
    return SCCheckStatus() == 1;
}

/* Copies the remote list into the console settings' device array and writes it; returns the next pairing step: 0x16 when
 * the settings are busy or refuse it, 0x64 to go on with a balance board's Wii Fit data, else 0x19. */
s32 WUDiStoreDeviceInfoToSC(void)
{
    WUDScDeviceArray* array;
    WUDDeviceNode* node;
    u8 i;
    s32 next;

    if (wudScIsBusy()) {
        return 0x16;
    }
    array = &wudBtDeviceInfo;
    memset(array->devices, 0, sizeof(WUDScDevice) * 10);
    wudBtDeviceInfo.count = wudGetDeviceCount();
    i = 0;
    for (node = wudCB.deviceHead; node != NULL; node = node->next) {
        memcpy(array->devices[i].bdAddr, node->device->bdAddr, 6);
        memcpy(array->devices[i].name, node->device->name, 0x40);
        i++;
    }
    WUDiDebugPrint("%d devices is stored into SC.\n", i);
    if (SCSetBtDeviceInfoArray(&wudBtDeviceInfo) == 0) {
        return 0x16;
    }
    next = 0x19;
    if (wudLinkedWbc != 0 && memcmp(wudStoredDeviceInfo.name, "Nintendo RVL-WBC", 0x10) == 0 &&
        SCGetProductGameRegion() == 0) {
        next = 0x64;
    }
    return next;
}

/* Handles the result of the NAND open of the stored-device file: lets the state machines go on or marks the failure. */
void WUDiNandOpenCallback(s32 result, NANDCommandBlock* block)
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
void WUDiNandSeekCallback(s32 result, NANDCommandBlock* block)
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
void WUDiNandWriteCallback(s32 result, NANDCommandBlock* block)
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
void WUDiNandCloseCallback(s32 result, NANDCommandBlock* block)
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
    WUDDevice* device;
    s32 i;

    if ((u8)cb->quickSearch != 0) {
        for (i = 0; i < 0x10; i++) {
            device = wudGetDeviceByIndex(i);
            if (device->linkState == 8) {
                wudSetSniffMode(device->bdAddr, 8);
                return 0xE;
            }
        }
    }
    OSCancelAlarm(&cb->alarm);
    if (wudStopRequested == 0) {
        wudSetVisibility(0, 1);
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

/* Whether a stored link key command is still running. */
static inline BOOL wudIsLinkKeyBusy(void)
{
    if (wudCB.linkKeyState != 0) {
        return TRUE;
    }
    return FALSE;
}

/* The number of balance board links, read with interrupts off. */
static inline u8 wudGetWbcLinkNumber(void)
{
    WUDCB* cb = &wudCB;
    u32 level = OSDisableInterrupts();
    u8 count = cb->wbcLinkNumber;

    OSRestoreInterrupts(level);
    return count;
}

/* Steps the pairing state machine: searches for a device, checks what it found against the stored records, opens its
 * link, registers it and its link key, stores the table in the console settings and the Wii Fit save file, and ends the
 * search. */
#pragma push
#pragma optimization_level 3
void WUDiSyncFiber(void)
{
    WUDCB* cb = &wudCB;
    WUDDevice* device;
    BtaDmInq inquiry;
    const char* path;
    u8 next;

    switch (cb->syncState) {
    case 1:
        cb->syncState = WUDiSyncCheckSearch();
        break;
    case 0x1D:
        if (cb->searching == 0) {
            next = 0xE;
        } else if (wudGetLinkNumber() == 4 && wudGetWbcLinkNumber() == 4) {
            next = 0xE;
        } else if ((s16)--cb->searchTime < 0) {
            next = 2;
        } else {
            next = 0x1D;
        }
        cb->syncState = next;
        break;
    case 2:
        inquiry.mode = 1;
        inquiry.maxResponses = 1;
        inquiry.filterType = 0;
        if ((u8)cb->quickSearch != 0) {
            inquiry.duration = 3;
        } else if ((u8)cb->simple == 0) {
            inquiry.duration = ((wudGetLinkNumber() == 3) ? 10 : 5) - (u8)cb->searching;
        } else {
            inquiry.duration = (wudGetLinkNumber() == 3) ? 8 : 3;
        }
        wudInquiryCount = 0;
        memset(&wudInquiryResult, 0, sizeof(wudInquiryResult));
        BTA_DmSearch(&inquiry, 0, WUDiSearchCallback);
        cb->syncState = 3;
        break;
    case 4:
        next = 1;
        if (wudInquiryCount != 0) {
            if (memcmp(wudInquiryResult.name, "Nintendo RVL-CNT", 0x10) == 0) {
                next = 5;
            }
            if (wudLinkedWbc != 0 && memcmp(wudInquiryResult.name, "Nintendo RVL-WBC", 0x10) == 0) {
                next = 5;
            }
            if ((s8)wudInquiryRssi < wudCB.syncRssiThreshold) {
                next = 1;
            }
        }
        if ((u8)wudCB.simple == 0 && next == 1) {
            next = 0x18;
            wudCB.searchTimeMax = (wudGetLinkNumber() == 3) ? 200 : 100;
            wudSetVisibility(0, 1);
        }
        cb->syncState = next;
        break;
    case 5:
        next = 0x11;
        device = WUDiGetDevInfo(wudInquiryResult.bdAddr);
        if (device != NULL) {
            memcpy(&wudStoredDeviceInfo, device, sizeof(WUDDevice));
            next = 0x10;
        }
        cb->syncState = next;
        break;
    case 0x10:
        device = &wudStoredDeviceInfo;
        device->linkState = 1;
        switch (device->type) {
        case 0:
        case 2:
        case 4:
        case 5:
            if ((u8)wudCB.simple == 0) {
                WUDiMoveDeviceToTail(device);
                WUDiRemoveDevice(device->bdAddr);
                next = 0x11;
            } else {
                device->type = 4;
                next = 0xF;
            }
            break;
        case 1:
        case 3:
            WUDiMoveWbcToTail(device);
            WUDiRemoveDevice(wudCB.wbcTail->device->bdAddr);
            next = 0x11;
            break;
        default:
            next = 0xF;
            break;
        }
        cb->syncState = next;
        break;
    case 0x11:
        device = &wudStoredDeviceInfo;
        device->linkState = 1;
        device->type = ((u8)cb->simple == 0) ? 0 : 1;
        memcpy(device->bdAddr, wudInquiryResult.bdAddr, 6);
        memcpy(device->name, wudInquiryResult.name, 0x40);
        memset(device->linkKey, 0, 0x10);
        cb->syncState = 0xF;
        break;
    case 0xF:
        cb->syncState = WUDiSyncCheckInquiry();
        break;
    case 0x12:
        next = wudStoredDeviceInfo.type;
        switch (next) {
        case 1:
            next = 0x13;
            break;
        case 0:
            next = 0x14;
            break;
        case 5:
            next = 0x15;
            break;
        case 3:
            next = 0x17;
            break;
        case 4:
            next = 7;
            break;
        case 2:
            next = 7;
            break;
        }
        cb->syncState = next;
        break;
    case 0x13:
        if (wudGetWbcCount() == 6) {
            WUDiRemoveDevice(cb->wbcTail->device->bdAddr);
        }
        device = wudGetNewDevice();
        if (device == NULL) {
            next = 0xFF;
        } else if (device->linkState != 0) {
            next = 0xFF;
        } else {
            memcpy(device, &wudStoredDeviceInfo, sizeof(WUDDevice));
            WUDiAddDevice(device->bdAddr);
            WUDiMoveWbcToHead(device);
            next = 0x17;
            wudDevAddrTable[device->handle] = device->bdAddr;
        }
        cb->syncState = next;
        break;
    case 0x14:
        cb->syncState = WUDiSyncRegisterDevice();
        break;
    case 0x15:
        WUDiMoveWbcToTail(&wudStoredDeviceInfo);
        WUDiRemoveDevice(cb->wbcTail->device->bdAddr);
        cb->syncState = WUDiSyncRegisterDevice();
        break;
    case 7:
        if (wudIsLinkKeyBusy()) {
            next = 7;
        } else if (wudLinkedWbc != 0 && memcmp(wudStoredDeviceInfo.name, "Nintendo RVL-WBC", 0x10) == 0) {
            next = 0x16;
        } else {
            device = WUDiGetDevInfo(wudStoredDeviceInfo.bdAddr);
            WUDiDebugPrint("write stored link key\n");
            WUDiDebugPrint("addr : %02x:%02x:%02x:%02x:%02x:%02x\n", device->bdAddr[0], device->bdAddr[1],
                           device->bdAddr[2], device->bdAddr[3], device->bdAddr[4], device->bdAddr[5]);
            WUDiDebugPrint("key  : %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\n",
                           device->linkKey[0], device->linkKey[1], device->linkKey[2], device->linkKey[3],
                           device->linkKey[4], device->linkKey[5], device->linkKey[6], device->linkKey[7],
                           device->linkKey[8], device->linkKey[9], device->linkKey[10], device->linkKey[11],
                           device->linkKey[12], device->linkKey[13], device->linkKey[14], device->linkKey[15]);
            cb->linkKeyState = 2;
            BTM_WriteStoredLinkKey(1, device->bdAddr, device->linkKey, __wudLinkKeyEventStackCallback);
            next = 8;
        }
        cb->syncState = next;
        break;
    case 8:
        if (!wudIsLinkKeyBusy()) {
            next = 0x16;
        } else {
            next = 8;
        }
        cb->syncState = next;
        break;
    case 0x16:
        cb->syncState = WUDiStoreDeviceInfoToSC();
        break;
    case 0x19: {
        u32 level = OSDisableInterrupts();

        cb->syncState = 0xD;
        OSRestoreInterrupts(level);
        SCFlushAsync(SyncFlushCallback);
        break;
    }
    case 0xA:
        next = 0xA;
        if (!wudIsLinkKeyBusy()) {
            next = 0x14;
            WUDiRemoveDevice(wudCB.linkKeyAddr);
        }
        cb->syncState = next;
        break;
    case 0x17:
        cb->syncRetries++;
        memset(&wudStoredDeviceInfo, 0, sizeof(WUDDevice));
        cb->syncState = ((u8)cb->simple == 0) ? 0xE : 1;
        break;
    case 0xFF:
        memset(&wudStoredDeviceInfo, 0, sizeof(WUDDevice));
        cb->syncState = ((u8)cb->simple == 0) ? 0xE : 1;
        break;
    case 0xE:
        cb->syncState = WUDiSyncDone();
        break;
    case 3:
        if (wudGetLinkNumber() == 4 && wudGetWbcLinkNumber() == 4) {
            WUDiDebugPrint("Cancel searching because 4 connections exist.\n");
            BTA_DmSearchCancel();
            next = 0x1A;
        } else {
            next = 3;
        }
        cb->syncState = next;
        break;
    case 0x18:
        next = 0x18;
        if (wudGetLinkNumber() == 4 && wudGetWbcLinkNumber() == 4) {
            next = 0xE;
        }
        if ((s16)--wudCB.searchTimeMax < 0) {
            next = 1;
        }
        cb->syncState = next;
        break;
    case 0x64:
        path = "/title/00010004/52464e4a/data/RPHealth.dat";
        if (wudPatchBusy == 0) {
            WUDiDebugPrint("WbcSaveData: %s\n", path);
            wudPatchBusy = 1;
            NANDOpenAsync(path, &wudNandFileInfo, 2, WUDiNandOpenCallback, &wudNandBlock);
        }
        break;
    case 0x65:
        if (wudPatchBusy == 0) {
            wudPatchBusy = 1;
            NANDSeekAsync(&wudNandFileInfo, 0x4AF18, 0, WUDiNandSeekCallback, &wudNandBlock);
        }
        break;
    case 0x66:
        if (wudPatchBusy == 0) {
            WUDiDebugPrint("WBC to WiiFit\n");
            WUDiDebugPrint(" %02x %02x %02x %02x %02x %02x\n", wudWiiFitSaveData.bdAddr[0], wudWiiFitSaveData.bdAddr[1],
                           wudWiiFitSaveData.bdAddr[2], wudWiiFitSaveData.bdAddr[3], wudWiiFitSaveData.bdAddr[4],
                           wudWiiFitSaveData.bdAddr[5]);
            WUDiDebugPrint(" %02x %02x %02x %02x %02x %02x %02x %02x\n", wudWiiFitSaveData.linkKey[0],
                           wudWiiFitSaveData.linkKey[1], wudWiiFitSaveData.linkKey[2], wudWiiFitSaveData.linkKey[3],
                           wudWiiFitSaveData.linkKey[4], wudWiiFitSaveData.linkKey[5], wudWiiFitSaveData.linkKey[6],
                           wudWiiFitSaveData.linkKey[7]);
            WUDiDebugPrint(" %02x %02x %02x %02x %02x %02x %02x %02x\n", wudWiiFitSaveData.linkKey[8],
                           wudWiiFitSaveData.linkKey[9], wudWiiFitSaveData.linkKey[10], wudWiiFitSaveData.linkKey[11],
                           wudWiiFitSaveData.linkKey[12], wudWiiFitSaveData.linkKey[13], wudWiiFitSaveData.linkKey[14],
                           wudWiiFitSaveData.linkKey[15]);
            WUDiDebugPrint(" %s\n", wudWiiFitSaveData.name);
            WUDiDebugPrint(" %02x%02x%02x%02x\n", wudWiiFitSaveData.crc[0], wudWiiFitSaveData.crc[1],
                           wudWiiFitSaveData.crc[2], wudWiiFitSaveData.crc[3]);
            wudPatchBusy = 1;
            NANDWriteAsync(&wudNandFileInfo, &wudWiiFitSaveData, sizeof(WUDWiiFitSaveData) - sizeof(wudWiiFitSaveData.pad_0x84),
                           WUDiNandWriteCallback, &wudNandBlock);
        }
        break;
    case 0x67:
        if (wudPatchBusy == 0) {
            wudPatchBusy = 1;
            NANDCloseAsync(&wudNandFileInfo, WUDiNandCloseCallback, &wudNandBlock);
        }
        break;
    case 6:
    case 0xD:
    case 0x1A:
        break;
    default:
        WUDiDebugPrint("WARNING: Illigal status\n");
        break;
    }
}
#pragma pop

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

/* Before a clear: removes the records still waiting for their link key and, with a balance board linked in a Japanese
 * game, wipes the link key in the Wii Fit save data; returns the next clear step (3 while links are open, 0x64 to write
 * the save data, else 5). */
#pragma push
#pragma opt_strength_reduction off
s32 WUDiUpdateWiiFitSaveData(void)
{
    WUDCB* cb = &wudCB;
    s32 i;

    if (wudGetLinkNumber() != 0) {
        return 3;
    }
    for (i = 0; i < 10; i++) {
        if (cb->devices[i].linkState == 1) {
            WUDiRemoveDevice(cb->devices[i].bdAddr);
        }
    }
    for (i = 0; i < 6; i++) {
        if (cb->wbcDevices[i].linkState == 1) {
            WUDiRemoveDevice(cb->wbcDevices[i].bdAddr);
        }
    }
    if (wudLinkedWbc != 0) {
        if (SCGetProductGameRegion() == 0) {
            u8 linkKey[0x10] = {0};

            wudSetWiiFitSaveData(NULL, linkKey, NULL);
            return 0x64;
        }
        return 5;
    }
    return 5;
}
#pragma pop

/* Steps the clear of the stored pairings: hides the host, drops the links, empties and flushes the console settings'
 * arrays, writes the wiped balance board record into Wii Fit's save file, then reports through the clear callback. */
void WUDiClearFiber(void)
{
    const char* path;
    WUDCB* cb = &wudCB;
    WUDResultCallback callback;
    s32 written;

    cb->syncState = 0;
    switch (cb->clearState) {
    case 1:
        BTA_DmSetVisibility(0, 0);
        cb->clearState = 2;
        break;
    case 2:
        cb->clearState = WUDiTerminateDevice();
        break;
    case 3:
        cb->clearState = WUDiUpdateWiiFitSaveData();
        break;
    case 5:
        if (SCCheckStatus() != 1) {
            memset(&wudBtDeviceInfo, 0, sizeof(WUDScDeviceArray) - sizeof(wudBtDeviceInfo.pad_0x461));
            memset(&wudBtCmpDeviceInfo, 0, sizeof(WUDScCmpDeviceArray) - sizeof(wudBtCmpDeviceInfo.pad_0x205));
            written = SCSetBtDeviceInfoArray(&wudBtDeviceInfo);
            if (written | SCSetBtCmpDeviceInfoArray(&wudBtCmpDeviceInfo)) {
                cb->clearState = 6;
                SCFlushAsync(DeleteFlushCallback);
            } else {
                cb->clearState = 8;
            }
        }
        break;
    case 8:
        wudSetVisibility(0, wudGetConnectable());
        OSCancelAlarm(&cb->alarm);
        callback = cb->clearCallback;
        cb->clearState = 0;
        if (callback != NULL) {
            callback(1);
        }
        cb->clearState = 0;
        break;
    case 100:
        path = "/title/00010004/52464e4a/data/RPHealth.dat";
        if (wudPatchBusy == 0) {
            WUDiDebugPrint("WbcSaveData: %s\n", path);
            wudPatchBusy = 1;
            NANDOpenAsync(path, &wudNandFileInfo, 2,
                          WUDiNandOpenCallback, &wudNandBlock);
        }
        break;
    case 101:
        if (wudPatchBusy == 0) {
            wudPatchBusy = 1;
            NANDSeekAsync(&wudNandFileInfo, 0x4AF18, 0, WUDiNandSeekCallback, &wudNandBlock);
        }
        break;
    case 102:
        if (wudPatchBusy == 0) {
            WUDiDebugPrint("WBC to WiiFit\n");
            WUDiDebugPrint(" %02x %02x %02x %02x %02x %02x\n", wudWiiFitSaveData.bdAddr[0], wudWiiFitSaveData.bdAddr[1],
                           wudWiiFitSaveData.bdAddr[2], wudWiiFitSaveData.bdAddr[3], wudWiiFitSaveData.bdAddr[4],
                           wudWiiFitSaveData.bdAddr[5]);
            WUDiDebugPrint(" %02x %02x %02x %02x %02x %02x %02x %02x\n", wudWiiFitSaveData.linkKey[0],
                           wudWiiFitSaveData.linkKey[1], wudWiiFitSaveData.linkKey[2], wudWiiFitSaveData.linkKey[3],
                           wudWiiFitSaveData.linkKey[4], wudWiiFitSaveData.linkKey[5], wudWiiFitSaveData.linkKey[6],
                           wudWiiFitSaveData.linkKey[7]);
            WUDiDebugPrint(" %02x %02x %02x %02x %02x %02x %02x %02x\n", wudWiiFitSaveData.linkKey[8],
                           wudWiiFitSaveData.linkKey[9], wudWiiFitSaveData.linkKey[10], wudWiiFitSaveData.linkKey[11],
                           wudWiiFitSaveData.linkKey[12], wudWiiFitSaveData.linkKey[13], wudWiiFitSaveData.linkKey[14],
                           wudWiiFitSaveData.linkKey[15]);
            WUDiDebugPrint(" %s\n", wudWiiFitSaveData.name);
            WUDiDebugPrint(" %02x%02x%02x%02x\n", wudWiiFitSaveData.crc[0], wudWiiFitSaveData.crc[1],
                           wudWiiFitSaveData.crc[2], wudWiiFitSaveData.crc[3]);
            wudPatchBusy = 1;
            NANDWriteAsync(&wudNandFileInfo, &wudWiiFitSaveData, sizeof(WUDWiiFitSaveData) - sizeof(wudWiiFitSaveData.pad_0x84),
                           WUDiNandWriteCallback, &wudNandBlock);
        }
        break;
    case 103:
        if (wudPatchBusy == 0) {
            wudPatchBusy = 1;
            NANDCloseAsync(&wudNandFileInfo, WUDiNandCloseCallback, &wudNandBlock);
        }
        break;
    case 7:
        return;
    }
}

/* Runs the clear fiber on the fiber stack when the clear alarm fires. */
void WUDiClearAlarmHandler(OSAlarm* alarm, OSContext* context)
{
    OSSwitchFiberEx((u32)alarm, (u32)context, 0, 0, WUDiClearFiber, wudFiberStack + sizeof(wudFiberStack));
}

/* Deletes the controller's stored link key of the next device that has one and returns 2 (wait for the callback); clears the
 * records whose key is gone; returns 3 when no key is left, 2 while a link key command runs. */
u8 WUDiDeleteAllLinkKeys(void)
{
    WUDCB* cb = &wudCB;
    WUDDevice* device;
    WUDDevice* found;
    u32 level;
    s32 i;

    if (cb->linkKeyState == 0) {
        for (i = 0; i < 0x10; i++) {
            device = wudGetDeviceByIndex(i);
            if (device->linkState != 0) {
                if (device->linkKeyState == 1) {
                    cb->linkKeyState = 3;
                    BTM_DeleteStoredLinkKey(device->bdAddr, __wudLinkKeyEventStackCallback);
                    device->linkKeyState = 0;
                    return 2;
                }
                if (device->linkKeyState != 3) {
                    found = WUDiGetDevInfo(device->bdAddr);
                    if (found != NULL) {
                        level = OSDisableInterrupts();
                        memset(found, 0, sizeof(WUDDevice));
                        OSRestoreInterrupts(level);
                    }
                }
            }
        }
        return 3;
    }
    return 2;
}

/* Steps the stack bring-up after the device table flush: reads the stored link keys, deletes them, then logs the controller's
 * version and installs the firmware patch or goes on with the stack setup. */
void WUDiInitFlushFiber(void)
{
    WUDCB* cb = &wudCB;
    BtmVersionInfo version;
    u8 next;

    switch (cb->initFlushState) {
    case 1:
        if (cb->enabledProfiles == 1) {
            cb->linkKeyState = 1;
            BTM_ReadStoredLinkKey(NULL, __wudLinkKeyEventStackCallback);
        }
        cb->initFlushState = 2;
        break;
    case 2:
        cb->initFlushState = WUDiDeleteAllLinkKeys();
        break;
    case 3:
        if (cb->linkKeyState == 0) {
            OSCancelAlarm(&cb->alarm);
            BTM_ReadLocalVersion(&version);
            WUDiDebugPrint("hci_version   : %02x\n", version.hciVersion);
            WUDiDebugPrint("hci_revision  : %04x\n", version.hciRevision);
            WUDiDebugPrint("lmp_version   : %02x\n", version.lmpVersion);
            WUDiDebugPrint("lmp_subversion: %04x\n", version.lmpSubversion);
            WUDiDebugPrint("manufacturer  : %04x\n", version.manufacturer);
            WUDiDebugPrint(" ==> 2045 firmware ver.002.003.014.%d\n", version.hciRevision & 0xFFF);
            if ((version.hciRevision & 0xFFF) == 0xA7) {
                __wudAppendRuntimePatch();
            } else {
                __wudInitSub();
            }
            next = 4;
        } else {
            next = 2;
        }
        cb->initFlushState = next;
        break;
    }
}

/* Runs the stack initialisation fiber on the fiber stack. */
void WUDiInitFlushAlarmHandler(OSAlarm* alarm, OSContext* context)
{
    OSSwitchFiberEx((u32)alarm, (u32)context, 0, 0, WUDiInitFlushFiber, wudFiberStack + sizeof(wudFiberStack));
}

/* Moves the initialisation state machine on after the device table is flushed. */
void InitFlushCallback(s32 result)
{
    WUDiDebugPrint("__wudInitFlushCallback() : %d, Init: %d\n", result, wudCB.initState);
    wudCB.initState = 5;
}

/* Whether the console settings' device array already holds the device at `bdAddr`. */
static inline BOOL wudScHasDevice(const u8* bdAddr)
{
    u8 count = wudBtDeviceInfo.count;
    s32 i;

    for (i = 0; i < count; i++) {
        if (memcmp(wudBtDeviceInfo.devices[i].bdAddr, bdAddr, 6) == 0) {
            return TRUE;
        }
    }
    return FALSE;
}

/* Logs the address and the name of a device record. */
static inline void wudPrintDevice(WUDDevice* device)
{
    WUDiDebugPrint("addr : %02x:%02x:%02x:%02x:%02x:%02x\n", device->bdAddr[0], device->bdAddr[1], device->bdAddr[2],
                   device->bdAddr[3], device->bdAddr[4], device->bdAddr[5]);
    WUDiDebugPrint("name : %s\n", device->name);
}

/* Rebuilds the device table from the console settings: the paired remotes (moving later entries into empty slots), the
 * balance board of the history and the balance board array; then writes the arrays back and flushes them. Returns 5. */
#pragma push
#pragma optimization_level 3
u8 WUDiLoadDeviceInfo(void)
{
    WUDCB* cb;
    WUDScDeviceArray* array;
    WUDDevice* device;
    u8 zero[6];
    s32 remaining;
    u8 registered;
    s32 i;
    s32 j;

    memset(&wudBtDeviceInfo, 0, sizeof(WUDScDeviceArray) - sizeof(wudBtDeviceInfo.pad_0x461));
    memset(&wudBtCmpDeviceInfo, 0, sizeof(WUDScCmpDeviceArray) - sizeof(wudBtCmpDeviceInfo.pad_0x205));
    memset(zero, 0, 6);
    SCGetBtDeviceInfoArray(&wudBtDeviceInfo);
    SCGetBtCmpDeviceInfoArray(&wudBtCmpDeviceInfo);
    array = &wudBtDeviceInfo;
    cb = &wudCB;
    cb->simple = 0;
    remaining = wudBtDeviceInfo.count;
    registered = 0;
    for (i = 0; i < 10; i++) {
        if (remaining == 0) {
            break;
        }
        if (memcmp(array->devices[i].name, "Nintendo RVL-CNT", 0x10) != 0) {
            memset(&array->devices[i], 0, sizeof(WUDScDevice));
        }
        if (memcmp(array->devices[i].bdAddr, zero, 6) == 0) {
            for (j = i + 1; j < 10; j++) {
                if (memcmp(array->devices[j].name, "Nintendo RVL-CNT", 0x10) == 0) {
                    memcpy(&array->devices[i], &array->devices[j], sizeof(WUDScDevice));
                    memset(&array->devices[j], 0, sizeof(WUDScDevice));
                    break;
                }
            }
            if (j >= 10) {
                continue;
            }
        }
        device = wudGetNewDevice();
        if (device != NULL) {
            memcpy(device->bdAddr, array->devices[i].bdAddr, 6);
            memcpy(device->name, array->devices[i].name, 0x40);
            device->linkState = 1;
            device->type = 0;
            device->linkKeyState = 2;
            device->subClass = 2;
            device->attrMask = 0x8074;
            device->appId = 3;
            wudPrintDevice(device);
            registered++;
            remaining--;
        }
    }
    if (wudLinkedWbc != 0 && memcmp(wudBtDeviceInfo.devices[14].name, "Nintendo RVL-WBC", 0x10) == 0) {
        device = wudGetNewDevice();
        if (device == NULL) {
            registered--;
            device = wudGetDeviceByIndex(9);
        }
        memcpy(device->bdAddr, wudBtDeviceInfo.devices[14].bdAddr, 6);
        memcpy(device->name, wudBtDeviceInfo.devices[14].name, 0x13);
        memcpy(device->linkKey, &wudBtDeviceInfo.devices[14].name[0x14], 0x10);
        device->linkState = 1;
        device->type = 0;
        device->subClass = 2;
        device->attrMask = 0x8074;
        device->appId = 3;
        device->linkKeyState = 3;
        wudPrintDevice(device);
        memset(&wudBtDeviceInfo.devices[registered], 0, sizeof(WUDScDevice));
        memcpy(wudBtDeviceInfo.devices[registered].bdAddr, wudBtDeviceInfo.devices[14].bdAddr, 6);
        memcpy(wudBtDeviceInfo.devices[registered].name, wudBtDeviceInfo.devices[14].name, 0x13);
        registered++;
    }
    wudBtDeviceInfo.count = registered;
    remaining = wudBtCmpDeviceInfo.count;
    cb->simple = 1;
    for (i = 5; i >= 0; i--) {
        if (remaining == 0) {
            break;
        }
        if (memcmp(wudBtCmpDeviceInfo.devices[i].bdAddr, zero, 6) != 0 &&
            !wudScHasDevice(wudBtCmpDeviceInfo.devices[i].bdAddr)) {
            device = wudGetNewDevice();
            if (device != NULL) {
                memcpy(device->bdAddr, wudBtCmpDeviceInfo.devices[i].bdAddr, 6);
                memcpy(device->name, wudBtCmpDeviceInfo.devices[i].name, 0x40);
                memcpy(device->linkKey, wudBtCmpDeviceInfo.devices[i].linkKey, 0x10);
                device->linkState = 1;
                device->type = 1;
                device->linkKeyState = 3;
                if (memcmp(device->name, "Nintendo RVL-CNT-01", 0x13) == 0) {
                    device->subClass = 2;
                    device->attrMask = 0x8074;
                    device->appId = 3;
                }
                wudPrintDevice(device);
                WUDiMoveWbcToHead(device);
                remaining--;
            }
        }
    }
    cb->simple = 0;
    cb->initState = 4;
    memset(&wudBtCmpDeviceInfo, 0, sizeof(WUDScCmpDeviceArray) - sizeof(wudBtCmpDeviceInfo.pad_0x205));
    SCSetBtDeviceInfoArray(&wudBtDeviceInfo);
    SCSetBtCmpDeviceInfoArray(&wudBtCmpDeviceInfo);
    SCFlushAsync(InitFlushCallback);
    return 5;
}
#pragma pop

/* Advances the Wii Fit save file read one step from the NAND result; a short read or an error ends it. */
void WUDiPatchCallback(s32 result, NANDCommandBlock* block)
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
void WUDiPatchStepDone(s32 result)
{
    wudPatchBusy = 0;
    wudPatchStep++;
}

/* Steps the read of Wii Fit's save file at bring-up: opens, seeks and reads it into the work area, checks the balance
 * board record's checksum and copies a valid record into the history of the console settings; returns the next
 * bring-up step (2 while it runs, 3 when it is done). */
#pragma push
#pragma opt_common_subs off
u8 WUDiLoadWiiFitSaveData(void)
{
    const char* path = "/title/00010004/52464e4a/data/RPHealth.dat";
    const char* gameCode = "RFNJ";
    u8 next = 2;
    u8 zero[0x10] = {0};
    u32 stored;
    u32 crc;
    s32 result;

    if (wudPatchBusy != 0) {
        return 2;
    }
    if (wudWbcWorkArea == NULL || ((u32)wudWbcWorkArea & 0x1F) != 0) {
        OSPanic("WUD.c", 2704, "Error: the workarea for synchronizing WBC pairing information is not allocated.\n");
    }
    switch (wudPatchStep) {
    case 0:
        result = NANDInit();
        wudPatchStep = (result == 0) ? 1 : 0xFF;
        WUDiDebugPrint("NANDInit. [%d]\n", result);
        break;
    case 1:
        if (SCGetProductGameRegion() == 0) {
            WUDiDebugPrint("WbcSaveData: %s\n", path);
            wudPatchBusy = 1;
            wudWiiFitCrc = 0;
            NANDOpenAsync(path, &wudNandFileInfo, 1, WUDiPatchCallback, &wudNandBlock);
        } else {
            wudPatchStep = 6;
        }
        break;
    case 2:
        wudPatchBusy = 1;
        NANDSeekAsync(&wudNandFileInfo, 0, 0, WUDiPatchCallback, &wudNandBlock);
        break;
    case 3:
        wudPatchBusy = 1;
        NANDReadAsync(&wudNandFileInfo, wudWbcWorkArea, sizeof(WUDWiiFitFile), WUDiPatchCallback, &wudNandBlock);
        break;
    case 4:
        memcpy(&wudWiiFitSaveData, wudWbcWorkArea->record, sizeof(wudWbcWorkArea->record));
        memcpy(&stored, wudWbcWorkArea->crc, 4);
        WUDiDebugPrint("CRC  = %d:%08x\n", stored, stored);
        wudWiiFitCrc = wudCalcCrc(0, (u16*)wudWbcWorkArea->body, sizeof(wudWbcWorkArea->body));
        crc = wudCalcCrc(wudWiiFitCrc, (u16*)wudWbcWorkArea->record, sizeof(wudWbcWorkArea->record));
        WUDiDebugPrint("CRC' = %d:%08x\n", crc, crc);
        if (stored != crc) {
            wudPatchStep = 5;
            WUDiDebugPrint("CRC Error on WiiFit SaveData.\n");
        } else if (memcmp(wudWiiFitSaveData.bdAddr, zero, 6) == 0) {
            wudPatchStep = 5;
            WUDiDebugPrint("WiiFit does not register WBC yet.\n");
        } else if (memcmp(OSGetAppGamename(), gameCode, 4) != 0 && memcmp(wudWiiFitSaveData.linkKey, zero, 0x10) == 0) {
            wudPatchStep = 5;
            WUDiDebugPrint("LinkKey saved in WiiFit SaveData is invalid.\n");
        } else {
            memset(wudWbcWorkArea, 0, sizeof(WUDWiiFitFile));
            memset(&wudBtDeviceInfo, 0, sizeof(WUDScDeviceArray) - sizeof(wudBtDeviceInfo.pad_0x461));
            SCGetBtDeviceInfoArray(&wudBtDeviceInfo);
            memcpy(wudBtDeviceInfo.devices[14].bdAddr, wudWiiFitSaveData.bdAddr, 6);
            memcpy(wudBtDeviceInfo.devices[14].name, wudWiiFitSaveData.name, 0x40);
            memcpy(&wudBtDeviceInfo.devices[14].name[0x14], wudWiiFitSaveData.linkKey, 0x10);
            WUDiDebugPrint("WBC from WiiFit\n");
            WUDiDebugPrint(" %02x %02x %02x %02x %02x %02x\n", wudWiiFitSaveData.bdAddr[0], wudWiiFitSaveData.bdAddr[1],
                           wudWiiFitSaveData.bdAddr[2], wudWiiFitSaveData.bdAddr[3], wudWiiFitSaveData.bdAddr[4],
                           wudWiiFitSaveData.bdAddr[5]);
            WUDiDebugPrint(" %02x %02x %02x %02x %02x %02x %02x %02x\n", wudWiiFitSaveData.linkKey[0],
                           wudWiiFitSaveData.linkKey[1], wudWiiFitSaveData.linkKey[2], wudWiiFitSaveData.linkKey[3],
                           wudWiiFitSaveData.linkKey[4], wudWiiFitSaveData.linkKey[5], wudWiiFitSaveData.linkKey[6],
                           wudWiiFitSaveData.linkKey[7]);
            WUDiDebugPrint(" %02x %02x %02x %02x %02x %02x %02x %02x\n", wudWiiFitSaveData.linkKey[8],
                           wudWiiFitSaveData.linkKey[9], wudWiiFitSaveData.linkKey[10], wudWiiFitSaveData.linkKey[11],
                           wudWiiFitSaveData.linkKey[12], wudWiiFitSaveData.linkKey[13], wudWiiFitSaveData.linkKey[14],
                           wudWiiFitSaveData.linkKey[15]);
            WUDiDebugPrint(" %s\n", wudWiiFitSaveData.name);
            SCSetBtDeviceInfoArray(&wudBtDeviceInfo);
            wudPatchBusy = 1;
            SCFlushAsync(WUDiPatchStepDone);
        }
        break;
    case 5:
        wudPatchBusy = 1;
        NANDCloseAsync(&wudNandFileInfo, WUDiPatchCallback, &wudNandBlock);
        break;
    default:
        next = 3;
        break;
    }
    return next;
}
#pragma pop

/* Steps the stack bring-up: waits for the console settings, clears the control block and reads the Wii Fit data or the
 * device arrays, then enables the Bluetooth stack. */
void WUDiInitFiber(void)
{
    WUDCB* cb = &wudCB;
    u8 next;

    switch (cb->initState) {
    case 1:
        next = 1;
        if ((s32)(500 - (u32)((s32)__OSGetSystemTime() - (s32)__OSStartTime) / ((OS_BUS_CLOCK / 4) / 1000)) < 0 &&
            SCCheckStatus() != 1) {
            __wudClearControlBlock();
            next = 3;
            if (wudLinkedWbc != 0 && SCGetProductGameRegion() == 0) {
                wudPatchBusy = 0;
                next = 2;
                wudPatchStep = 0;
            }
        }
        cb->initState = next;
        break;
    case 3:
        WUDiLoadDeviceInfo();
        break;
    case 5:
        OSCancelAlarm(&cb->alarm);
        cb->status = 1;
        BTA_EnableBluetooth(__wudSecurityEventStackCallback);
        cb->initState = 6;
        break;
    case 2:
        cb->initState = WUDiLoadWiiFitSaveData();
        break;
    }
}

/* Runs the stack bring-up fiber on the fiber stack. */
void WUDiInitAlarmHandler(OSAlarm* alarm, OSContext* context)
{
    OSSwitchFiberEx((u32)alarm, (u32)context, 0, 0, WUDiInitFiber, wudFiberStack + sizeof(wudFiberStack));
}

/* Moves the shutdown state machine on after the device table is flushed. */
void ShutdownFlushCallback(s32 result)
{
    WUDiDebugPrint("__wudShutdownFlushCallback() : %d, Shutdown: %d\n", result, wudCB.shutdownState);
    wudCB.shutdownState = 3;
}

/* Steps the shutdown: writes and flushes the device arrays when the stack was up, then stops the alarm and disables the
 * Bluetooth stack. */
void WUDiShutdownFiber(void)
{
    s32 written;

    switch (wudCB.shutdownState) {
    case 1:
        written = wudStackReady;
        if (SCCheckStatus() != 1) {
            written &= SCSetBtDeviceInfoArray(&wudBtDeviceInfo);
            written &= SCSetBtCmpDeviceInfoArray(&wudBtCmpDeviceInfo);
            if (written) {
                wudCB.shutdownState = 2;
                SCFlushAsync(ShutdownFlushCallback);
            } else {
                wudCB.shutdownState = 3;
            }
        }
        break;
    case 3:
        OSCancelAlarm(&wudCB.alarm);
        BTA_DisableBluetooth();
        break;
    }
}

/* Runs the shutdown fiber on the fiber stack. */
void WUDiShutdownAlarmHandler(OSAlarm* alarm, OSContext* context)
{
    OSSwitchFiberEx((u32)alarm, (u32)context, 0, 0, WUDiShutdownFiber, wudFiberStack + sizeof(wudFiberStack));
}

/* Resets the control block: the per-handle tables, both recently-used lists, the state machines and the device records. */
#pragma push
#pragma opt_strength_reduction off
void __wudClearControlBlock(void)
{
    WUDCB* cb = &wudCB;
    WUDDeviceNode* node;
    WUDDevice* device;
    u32 level;
    s32 i;

    WUDiDebugPrint("__wudClearControlBlock\n");
    for (i = 0; i < 0x10; i++) {
        wudDevAddrTable[(u8)i] = NULL;
        wudQueuedSize[(u8)i] = 0;
        wudNotAckedSize[(u8)i] = 0;
    }
    cb->wbcTail = &cb->wbcNodes[5];
    cb->wbcHead = &cb->wbcNodes[0];
    node = cb->wbcNodes;
    for (i = 0; i < 6; i++, node++) {
        node->device = &cb->wbcDevices[5 - i];
        node->prev = (i == 0) ? NULL : &cb->wbcNodes[i - 1];
        node->next = (i == 5) ? NULL : &cb->wbcNodes[i + 1];
    }
    cb->deviceTail = &cb->deviceNodes[9];
    cb->deviceHead = &cb->deviceNodes[0];
    node = cb->deviceNodes;
    device = cb->devices;
    for (i = 0; i < 10; i++, node++, device++) {
        node->device = device;
        node->prev = (i == 0) ? NULL : &cb->deviceNodes[i - 1];
        node->next = (i == 9) ? NULL : &cb->deviceNodes[i + 1];
    }
    cb->syncState = 0;
    cb->linkKeyState = 0;
    cb->clearState = 0;
    cb->initFlushState = 0;
    cb->initState = 0;
    cb->shutdownState = 0;
    cb->quickSearch = 0;
    cb->simple = 0;
    cb->searching = 1;
    cb->connectable = 0;
    cb->discoverable = 0;
    cb->status = 0;
    cb->hciHandle = 0;
    cb->syncRssiThreshold = -65;
    memset(cb->hostAddr, 0, 6);
    memset(cb->linkKeyAddr, 0, 6);
    cb->bufferHead = 0;
    cb->bufferTail = 10;
    level = OSDisableInterrupts();
    cb = &wudCB;
    memset(cb->devices, 0, sizeof(cb->devices));
    memset(cb->wbcDevices, 0, sizeof(cb->wbcDevices));
    cb->remoteCount = 0;
    cb->wbcCount = 0;
    cb->wbcLinkNumber = 0;
    cb->linkNumber = 0;
    cb->syncRetries = 0;
    OSRestoreInterrupts(level);
}
#pragma pop

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
    cb->initState = 1;
    SCInit();
    if (wudLinkedWbc != 0 && wudWbcInitCallback != NULL) {
        wudWbcInitCallback();
    }
    OSCreateAlarm(&cb->alarm);
    now = OSGetTime();
    OSSetPeriodicAlarm(&cb->alarm, now, ((OS_BUS_CLOCK / 4) / 1000) * 10, WUDiInitAlarmHandler);
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

/* Shuts the stack down: stops a running state machine, copies the remote list (and, when `saveWbc` is set, the balance
 * board list) into the console settings' arrays and starts the shutdown fiber. */
void WUDShutdown(s32 saveWbc)
{
    WUDCB* cb = &wudCB;
    WUDDeviceNode* node;
    u32 level;
    s32 i;

    WUDiDebugPrint("WUDShutdown()\n");
    level = OSDisableInterrupts();
    if (wudIsBusy(cb)) {
        OSCancelAlarm(&cb->alarm);
    }
    memset(wudBtDeviceInfo.devices, 0, sizeof(WUDScDevice) * 10);
    i = 0;
    for (node = wudCB.deviceHead; node != NULL; node = node->next) {
        memcpy(wudBtDeviceInfo.devices[i].bdAddr, node->device->bdAddr, 6);
        memcpy(wudBtDeviceInfo.devices[i].name, node->device->name, 0x40);
        i++;
    }
    wudBtDeviceInfo.count = wudGetDeviceCount();
    memset(wudBtCmpDeviceInfo.devices, 0, sizeof(wudBtCmpDeviceInfo.devices));
    if (saveWbc != 0) {
        i = 0;
        for (node = wudCB.wbcHead; node != NULL; node = node->next) {
            memcpy(wudBtCmpDeviceInfo.devices[i].bdAddr, node->device->bdAddr, 6);
            memcpy(wudBtCmpDeviceInfo.devices[i].name, node->device->name, 0x40);
            memcpy(wudBtCmpDeviceInfo.devices[i].linkKey, node->device->linkKey, 0x10);
            i++;
        }
        wudBtCmpDeviceInfo.count = wudGetWbcCount();
    } else {
        wudBtCmpDeviceInfo.count = 0;
    }
    cb->shutdownState = 1;
    OSCreateAlarm(&cb->alarm);
    OSSetPeriodicAlarm(&cb->alarm, OSGetTime(), ((OS_BUS_CLOCK / 4) / 1000) * 10, WUDiShutdownAlarmHandler);
    cb->status = 4;
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

/* Starts pairing a special device (GUESS body: only its log string is retail evidence; the linker strips it). */
s32 WUDStartSyncSpDevice(void)
{
    WUDiDebugPrint("WUDStartSyncSpDevice()\n");
    return WUDiStartSync(0, 3, 1, 0);
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
    s32 count;
    u8 records;
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
    records = count;
    batch[0] = count;
    size = records * 0xD;
    memcpy(&batch[1], wudPatchTable.records[wudPatchRecordIndex], size);
    wudPatchRecordIndex += records;
    WUDiDebugPrint("Install the patches\n");
    BTM_VendorSpecificCommand(0xFC4F, size + 1, batch, WUDiPatchRecordCallback);
}

/* Writes the next block of the patch code to the controller's RAM, then starts the record stream once the code is all sent. */
void WUDiPatchWriteCallback(s32 result)
{
    u8 batch[0x100];
    u8 command[4 + 0xFB];
    u8 chunk;
    s32 length;
    s32 i;
    u32 address;

    if (result != 0) {
        if (wudPatchLength == wudPatchSent) {
            s32 count;
            u8 records;
            s32 size;

            wudPatchRecordIndex = 0;
            wudPatchRecordCount = wudPatchTable.count;
            count = 0x13;
            if (wudPatchRecordCount - wudPatchRecordIndex < 0x13) {
                count = wudPatchRecordCount - wudPatchRecordIndex;
            }
            records = count;
            batch[0] = count;
            size = records * 0xD;
            memcpy(&batch[1], wudPatchTable.records, size);
            wudPatchRecordIndex += records;
            WUDiDebugPrint("Install the patches\n");
            BTM_VendorSpecificCommand(0xFC4F, size + 1, batch, WUDiPatchRecordCallback);
            return;
        }
        length = 0xFB;
        if (wudPatchLength - wudPatchSent < 0xFB) {
            length = wudPatchLength - wudPatchSent;
        }
        chunk = length;
        address = wudPatchAddress + wudPatchSent;
        command[0] = address;
        command[1] = address >> 8;
        command[2] = address >> 16;
        command[3] = address >> 24;
        for (i = 0; i < chunk; i++) {
            command[4 + i] = wudPatchImage[8 + wudPatchSent + i];
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
    const u8* image;
    u8 chunk;
    s32 i;

    WUDiDebugPrint("RemovePatch Callback\n");
    if (result != 0) {
        chunk = (wudPatchLength < 0xFB) ? wudPatchLength : 0xFB;
        command[0] = wudPatchAddress;
        command[1] = wudPatchAddress >> 8;
        command[2] = wudPatchAddress >> 16;
        command[3] = wudPatchAddress >> 24;
        image = wudPatchImage;
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
    wudPatchAddress = (((((wudPatchImage[3] << 8) + wudPatchImage[2]) << 8) + wudPatchImage[1]) << 8)
                      + wudPatchImage[0];
    wudPatchLength = (((((wudPatchImage[7] << 8) + wudPatchImage[6]) << 8) + wudPatchImage[5]) << 8)
                     + wudPatchImage[4];
    if (__OSInIPL != 0) {
        WUDiDebugPrint("SuperPeekPoke\n");
        BTM_VendorSpecificCommand(0xFC0A, 9, wudSetupCommand, SuperPeekPokeCallback);
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
    wudSetVisibility(0, 1);
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
        dscp.length = sizeof(wudHidDescriptor);
        dscp.descriptor = wudHidDescriptor;
        WUDiDebugPrint("BTA_HhAddDev()\n");
        BTA_HhAddDev(device->bdAddr, device->attrMask, device->subClass, device->appId, dscp);
    }
    if (device->type == 0 || device->type == 4 || device->type == 2 || device->type == 5) {
        cb->remoteCount++;
    } else {
        cb->wbcCount++;
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
            cb->wbcCount--;
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

/* Moves the balance board list entry of `device` in front of the first other entry that is not connected. */
#pragma push
#pragma opt_strength_reduction off
#pragma opt_common_subs off
void WUDiInsertWbcNode(WUDDevice* device)
{
    WUDCB* cb = &wudCB;
    u32 level = OSDisableInterrupts();
    s32 i;

    for (i = 0; i < 6; i++) {
        WUDDeviceNode* node = &cb->wbcNodes[i];

        if (memcmp(node->device->bdAddr, device->bdAddr, 6) == 0) {
            WUDDeviceNode* head = cb->wbcHead;
            WUDDeviceNode* cur;

            for (cur = head; cur != NULL; cur = cur->next) {
                WUDDevice* other = cur->device;

                if (memcmp(other->bdAddr, device->bdAddr, 6) != 0 && other->linkState <= 1) {
                    if (memcmp(head->device->bdAddr, cb->wbcNodes[i].device->bdAddr, 6) == 0) {
                        if (cur == head->next) {
                            break;
                        }
                        cb->wbcHead = cb->wbcNodes[i].next;
                    } else {
                        cb->wbcNodes[i].prev->next = cb->wbcNodes[i].next;
                    }
                    cb->wbcNodes[i].next->prev = cb->wbcNodes[i].prev;
                    if (cur != cb->wbcHead) {
                        cb->wbcNodes[i].prev = cur->prev;
                        cb->wbcNodes[i].next = cur;
                        cur->prev->next = node;
                        cur->prev = node;
                    } else {
                        cb->wbcNodes[i].prev = cur;
                        cb->wbcNodes[i].next = cur->next;
                        cur->next->prev = node;
                        cur->next = node;
                    }
                    break;
                }
            }
        }
    }
    OSRestoreInterrupts(level);
}
#pragma pop

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

/* Moves the remote list entry of `device` in front of the first other entry that is not connected (or, with a balance board
 * linked, is not a Wii remote). */
#pragma push
#pragma opt_strength_reduction off
#pragma opt_common_subs off
void WUDiInsertDeviceNode(WUDDevice* device)
{
    WUDCB* cb = &wudCB;
    u32 level = OSDisableInterrupts();
    s32 i;

    for (i = 0; i < 10; i++) {
        WUDDeviceNode* node = &cb->deviceNodes[i];

        if (memcmp(node->device->bdAddr, device->bdAddr, 6) == 0) {
            WUDDeviceNode* head = cb->deviceHead;
            WUDDeviceNode* cur;

            for (cur = head; cur != NULL; cur = cur->next) {
                WUDDevice* other = cur->device;

                if (memcmp(other->bdAddr, device->bdAddr, 6) != 0 &&
                    (other->linkState <= 1 ||
                     (memcmp(other->name, "Nintendo RVL-CNT", 0x10) != 0 && wudLinkedWbc != 0))) {
                    if (memcmp(head->device->bdAddr, cb->deviceNodes[i].device->bdAddr, 6) == 0) {
                        if (cur == head->next) {
                            break;
                        }
                        cb->deviceHead = cb->deviceNodes[i].next;
                    } else {
                        cb->deviceNodes[i].prev->next = cb->deviceNodes[i].next;
                    }
                    cb->deviceNodes[i].next->prev = cb->deviceNodes[i].prev;
                    if (cur != cb->deviceHead) {
                        cb->deviceNodes[i].prev = cur->prev;
                        cb->deviceNodes[i].next = cur;
                        cur->prev->next = node;
                        cur->prev = node;
                    } else {
                        cb->deviceNodes[i].prev = cur;
                        cb->deviceNodes[i].next = cur->next;
                        cur->next->prev = node;
                        cur->next = node;
                    }
                    break;
                }
            }
        }
    }
    OSRestoreInterrupts(level);
}
#pragma pop

/* Moves the remote list entry of `device` in front of the first other entry that has no record, or behind a connected
 * tail entry. */
void WUDiInsertDeviceNodeTail(WUDDevice* device)
{
    WUDCB* cb = &wudCB;
    u32 level = OSDisableInterrupts();
    s32 i;

    for (i = 0; i < 10; i++) {
        WUDDeviceNode* node = &cb->deviceNodes[i];
        u8* bdAddr = cb->deviceNodes[i].device->bdAddr;

        if (memcmp(bdAddr, device->bdAddr, 6) == 0) {
            WUDDeviceNode* head = cb->deviceHead;
            WUDDeviceNode* cur;

            for (cur = head; cur != NULL; cur = cur->next) {
                WUDDevice* other = cur->device;

                if (memcmp(other->bdAddr, device->bdAddr, 6) != 0 &&
                    (other->linkState == 0 || cur == cb->deviceTail)) {
                    if (memcmp(head->device->bdAddr, bdAddr, 6) == 0) {
                        if (cur == head->next) {
                            break;
                        }
                        cb->deviceHead = cb->deviceNodes[i].next;
                    } else {
                        cb->deviceNodes[i].prev->next = cb->deviceNodes[i].next;
                    }
                    cb->deviceNodes[i].next->prev = cb->deviceNodes[i].prev;
                    if (cur == cb->deviceTail && cur->device->linkState != 0) {
                        cb->deviceNodes[i].prev = cur;
                        cb->deviceNodes[i].next = NULL;
                        cur->next = node;
                        cb->deviceTail = node;
                    } else if (cur != cb->deviceHead) {
                        cb->deviceNodes[i].prev = cur->prev;
                        cb->deviceNodes[i].next = cur;
                        cur->prev->next = node;
                        cur->prev = node;
                    } else {
                        cb->deviceNodes[i].prev = cur;
                        cb->deviceNodes[i].next = cur->next;
                        cur->next->prev = node;
                        cur->next = node;
                    }
                    break;
                }
            }
        }
    }
    OSRestoreInterrupts(level);
}

/* Returns non-zero while a pairing, clear or stack bring-up step is running. */
s32 WUDIsSyncing(void)
{
    WUDCB* cb = &wudCB;
    u32 level = OSDisableInterrupts();
    if (cb->syncState == 0 && cb->clearState == 0 && cb->initFlushState == 4 && cb->initState == 6) {
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

/* Handles the device manager's security events: brings the HID host up after the stack is enabled, answers the PIN
 * request with the host address, keeps the link key of a completed authentication and counts the links. */
void __wudSecurityEventStackCallback(u8 event, union BtaDmSecEventData* data)
{
    WUDCB* cb = &wudCB;
    WUDDevice* device;
    const u8* pinAddr;
    u8 pin[6];
    u32 level;
    u32 i;

    WUDiDebugPrint("__wudCleanupStackCallback: ");
    switch (event) {
    case 0:
        memcpy(cb->hostAddr, data->enable.bd_addr, 6);
        WUDiDebugPrint("BTA_ENABLE_EVT\n");
        WUDiDebugPrint("host : %02x:%02x:%02x:%02x:%02x:%02x\n", cb->hostAddr[0], cb->hostAddr[1], cb->hostAddr[2],
                       cb->hostAddr[3], cb->hostAddr[4], cb->hostAddr[5]);
        BTA_HhEnable(0x12, WUDiHidHostEventCallback);
        cb->initFlushState = 1;
        OSCreateAlarm(&cb->alarm);
        OSSetPeriodicAlarm(&cb->alarm, OSGetTime(), ((OS_BUS_CLOCK / 4) / 1000) * 10, WUDiInitFlushAlarmHandler);
        cb->status = 2;
        break;
    case 1:
        WUDiDebugPrint("BTA_DISABLE_EVT\n");
        BTA_CleanUp(CleanupCallback);
        break;
    case 2:
        WUDiDebugPrint("BTA_DM_PIN_REQ_EVT\n");
        if ((u8)cb->simple == 0) {
            pinAddr = cb->hostAddr;
        } else {
            pinAddr = data->pin_req.bd_addr;
        }
        if (wudStoredDeviceInfo.type == 4) {
            WUDiRemoveDevice(wudStoredDeviceInfo.bdAddr);
            btm_remove_acl(wudStoredDeviceInfo.bdAddr);
        }
        wudStoredDeviceInfo.pinPending = 1;
        level = OSDisableInterrupts();
        pin[0] = pinAddr[5];
        pin[1] = pinAddr[4];
        pin[2] = pinAddr[3];
        pin[3] = pinAddr[2];
        pin[4] = pinAddr[1];
        pin[5] = pinAddr[0];
        OSRestoreInterrupts(level);
        BTA_DmPinReply(data->pin_req.bd_addr, 1, 6, pin);
        break;
    case 3:
        WUDiDebugPrint("BTA_DM_AUTH_CMPL_EVT\n");
        WUDiDebugPrint("  addr : %02x:%02x:%02x:%02x:%02x:%02x\n", data->auth_cmpl.bd_addr[0],
                       data->auth_cmpl.bd_addr[1], data->auth_cmpl.bd_addr[2], data->auth_cmpl.bd_addr[3],
                       data->auth_cmpl.bd_addr[4], data->auth_cmpl.bd_addr[5]);
        WUDiDebugPrint("  key  : %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\n",
                       data->auth_cmpl.key[0], data->auth_cmpl.key[1], data->auth_cmpl.key[2],
                       data->auth_cmpl.key[3], data->auth_cmpl.key[4], data->auth_cmpl.key[5],
                       data->auth_cmpl.key[6], data->auth_cmpl.key[7], data->auth_cmpl.key[8],
                       data->auth_cmpl.key[9], data->auth_cmpl.key[10], data->auth_cmpl.key[11],
                       data->auth_cmpl.key[12], data->auth_cmpl.key[13], data->auth_cmpl.key[14],
                       data->auth_cmpl.key[15]);
        WUDiDebugPrint("  result = %d\n", data->auth_cmpl.success);
        if (data->auth_cmpl.success != 0) {
            device = WUDiGetDevInfo(data->auth_cmpl.bd_addr);
            if (memcmp(wudStoredDeviceInfo.bdAddr, data->auth_cmpl.bd_addr, 6) == 0) {
                wudStoredDeviceInfo.linkState = 12;
                if (device == NULL) {
                    device = &wudStoredDeviceInfo;
                }
            }
            memcpy(device->linkKey, data->auth_cmpl.key, 0x10);
        }
        break;
    case 4:
        WUDiDebugPrint("BTA_DM_AUTHORIZE_EVT\n");
        break;
    case 5:
        WUDiDebugPrint("BTA_DM_LINK_UP_EVT\n");
        WUDiDebugPrint("   addr : %02x:%02x:%02x:%02x:%02x:%02x\n", data->link_up.bd_addr[0], data->link_up.bd_addr[1],
                       data->link_up.bd_addr[2], data->link_up.bd_addr[3], data->link_up.bd_addr[4], data->link_up.bd_addr[5]);
        device = WUDiGetDevInfo(data->link_up.bd_addr);
        if (device == NULL && memcmp(data->link_up.bd_addr, wudStoredDeviceInfo.bdAddr, 6) == 0) {
            device = &wudStoredDeviceInfo;
        }
        if (device == NULL || wudGetLinkNumber() == 4) {
            btm_remove_acl(data->link_up.bd_addr);
            WUDiDebugPrint("%s --> %02x:%02x:%02x:%02x:%02x:%02x\n", (device == NULL) ? "not paired" : "4 links exist",
                           data->link_up.bd_addr[0], data->link_up.bd_addr[1], data->link_up.bd_addr[2], data->link_up.bd_addr[3],
                           data->link_up.bd_addr[4], data->link_up.bd_addr[5]);
        } else {
            device->linkState = (device->linkState == 2) ? 12 : 3;
            cb->linkNumber++;
        }
        break;
    case 6:
        WUDiDebugPrint("BTA_DM_LINK_DOWN_EVT\n");
        WUDiDebugPrint("   addr : %02x:%02x:%02x:%02x:%02x:%02x\n", data->link_down.bd_addr[0], data->link_down.bd_addr[1],
                       data->link_down.bd_addr[2], data->link_down.bd_addr[3], data->link_down.bd_addr[4], data->link_down.bd_addr[5]);
        WUDiDebugPrint("result: %d\n", data->link_down.status);
        device = WUDiGetDevInfo(data->link_down.bd_addr);
        if (device != NULL) {
            device->linkState = 1;
            cb->linkNumber--;
            if (memcmp(wudStoredDeviceInfo.bdAddr, data->link_down.bd_addr, 6) == 0) {
                cb->syncState = 0xFF;
            }
            if (data->link_down.status == 0x15) {
                for (i = 0; i < 4; i++) {
                    u8* history = wudBtDeviceInfo.devices[(u8)i + 10].bdAddr;

                    if (memcmp(history, data->link_down.bd_addr, 6) == 0) {
                        memset(history, 0, sizeof(WUDScDevice));
                        wudDeviceInfoDirty = 1;
                    }
                }
            }
        } else if (memcmp(wudStoredDeviceInfo.bdAddr, data->link_down.bd_addr, 6) == 0) {
            cb->syncState = 0xFF;
            cb->linkNumber--;
        } else {
            WUDiDebugPrint("this device in not paired\n");
        }
        if (cb->linkNumber >= 250) {
            OSReport("WARNING: link num count is reset.\n");
            cb->linkNumber = 0;
        }
        break;
    case 7:
        WUDiDebugPrint("BTA_DM_SIG_STRENGTH_EVT\n");
        break;
    case 8:
        WUDiDebugPrint("BTA_DM_BUSY_LEVEL_EVT\n");
        break;
    }
}

/* Handles the stack's device search events: logs an inquiry hit, stores a discovery hit, and ends or cancels the search. */
void WUDiSearchCallback(s32 event, const u8* data)
{
    switch (event) {
    case 0: {
        u16 timeout;

        WUDiDebugPrint("INQUIRY RESULT: %02x:%02x:%02x:%02x:%02x:%02x   %02x%02x%02x   %d\n", data[0], data[1], data[2], data[3], data[4],
                       data[5], data[6], data[7], data[8], (s8)data[9]);
        wudInquiryRssi = data[9];
        if ((u8)wudCB.quickSearch == 1 || ((u8)wudCB.quickSearch == 0 && wudGetLinkNumber() < 3)) {
            timeout = 0x1900;
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
        wudCB.syncState = 4;
        return;
    case 4:
        WUDiDebugPrint("SEARCH CANCEL\n\n");
        BTM_VendorSpecificCommand(0xFC4C, 0x1C, wudSearchCancelCommand, NULL);
        wudInquiryCount = 0;
        memset(&wudInquiryResult, 0, sizeof(wudInquiryResult));
        wudCB.syncState = 4;
        return;
    default:
        WUDiDebugPrint("Warning: Search Callback returns invalid event\n");
        return;
    }
}

/* Handles the controller's vendor events: the sync button (start pairing or report it), the delete button (clear the
 * pairings or report it), the status byte of event 0x0A and the fatal module error. */
void WUDiVendorEventCallback(u8 length, u8* data)
{
    WUDCB* cb = &wudCB;
    WUDResultCallback callback;
    u32 level;
    s32 result;

    switch (data[0]) {
    case 8:
        WUDiDebugPrint("__wudProcSyncEvent()\n");
        if (!wudIsBusy(cb)) {
            level = OSDisableInterrupts();
            callback = cb->syncCallback;
            OSRestoreInterrupts(level);
            if (callback != NULL) {
                ((WUDSyncCallback)callback)(0, 0);
            } else {
                WUDiStartSync(0, 3, 0, 0);
            }
        }
        break;
    case 9:
        WUDiDebugPrint("__wudProcDeleteEvent()\n");
        level = OSDisableInterrupts();
        callback = cb->clearCallback;
        result = wudIsBusy(cb) ? -1 : 0;
        OSRestoreInterrupts(level);
        if (callback != NULL) {
            callback(result);
        } else {
            wudStartClearDevice();
        }
        break;
    case 10:
        cb->vendorEventStatus = data[1];
        break;
    case 16:
        OSPanic("WUD.c", 5342, "MODULE FATAL ERROR\n");
        break;
    }
}

/* Logs the stack's device status events and reports a fatal USB error. */
void WUDDeviceStatusCallback(u32 event)
{
    WUDiDebugPrint("__wudDeviceStatusEventStackCallback\n");
    if (event == 2) {
        OSReport("---- WARNING: USB FATAL ERROR! ----\n");
    }
}

/* size: 0x16 - one stored link key of the controller: the device and its key. */
typedef struct WUDLinkKeyEntry {
    /* +0x00 */ u8 bdAddr[6];
    /* +0x06 */ u8 linkKey[0x10];
} WUDLinkKeyEntry; /* size: 0x16 */

/* size: 0x18 - the result of a stored link key command (BTE `tBTM_*_STORED_LINK_KEY_COMPLETE`), by event; a returned keys
 * event carries `count` entries. */
typedef union WUDLinkKeyEvent {
    /* +0x00 */ u8 event;
    struct {
        /* +0x00 */ u8 event;
        /* +0x01 */ u8 count;
        /* +0x02 */ WUDLinkKeyEntry entries[1];
    } keys; /* size: 0x18 */
    struct {
        /* +0x00 */ u8 event;
        /* +0x01 */ u8 status;
        /* +0x02 */ u16 maxKeys;
        /* +0x04 */ u16 numKeys;
    } read; /* size: 0x06 */
    struct {
        /* +0x00 */ u8 event;
        /* +0x01 */ u8 status;
        /* +0x02 */ u8 numKeys;
    } write; /* size: 0x03 */
    struct {
        /* +0x00 */ u8 event;
        /* +0x01 */ u8 status;
        /* +0x02 */ u16 numKeys;
    } erase; /* size: 0x04 */
} WUDLinkKeyEvent; /* size: 0x18 */

/* Handles the results of the stored link key commands: records every returned key in its device (or in a free record),
 * and ends the read, write and delete steps. */
void __wudLinkKeyEventStackCallback(void* result) /* untyped: the BTM command's payload, one type per command */
{
    WUDLinkKeyEvent* event = (WUDLinkKeyEvent*)result;
    WUDLinkKeyEntry* entry;
    WUDCB* cb = &wudCB;
    WUDDevice* device;
    WUDDevice* free;
    s32 i;

    switch (event->event) {
    case 1:
        WUDiDebugPrint("BTM_CB_EVT_RETURN_LINK_KEYS\n");
        entry = event->keys.entries;
        for (i = 0; i < event->keys.count; i++) {
            device = WUDiGetDevInfo(entry->bdAddr);
            WUDiDebugPrint("BD_ADDR:  %02x:%02x:%02x:%02x:%02x:%02x\n", entry->bdAddr[0], entry->bdAddr[1],
                           entry->bdAddr[2], entry->bdAddr[3], entry->bdAddr[4], entry->bdAddr[5]);
            WUDiDebugPrint("LINKKEY: %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\n",
                           entry->linkKey[0], entry->linkKey[1], entry->linkKey[2], entry->linkKey[3], entry->linkKey[4],
                           entry->linkKey[5], entry->linkKey[6], entry->linkKey[7], entry->linkKey[8], entry->linkKey[9],
                           entry->linkKey[10], entry->linkKey[11], entry->linkKey[12], entry->linkKey[13],
                           entry->linkKey[14], entry->linkKey[15]);
            if (device == NULL) {
                WUDiDebugPrint("this device is not stored in NAND\n");
                free = wudGetNewDevice();
                if (free == NULL) {
                    for (i = 0; i < 6; i++) {
                        if (cb->wbcDevices[i].linkState == 0) {
                            free = &cb->wbcDevices[i];
                            break;
                        }
                    }
                }
                if (free != NULL) {
                    free->linkKeyState = 1;
                    free->linkState = 1;
                    memcpy(free->bdAddr, entry->bdAddr, 6);
                    memcpy(free->linkKey, entry->linkKey, 0x10);
                }
            } else {
                memcpy(device->bdAddr, entry->bdAddr, 6);
                memcpy(device->linkKey, entry->linkKey, 0x10);
                if (device->linkKeyState == 2) {
                    device->linkKeyState = 3;
                }
                memcpy(cb->linkKeyAddr, entry->bdAddr, 6);
                WUDiDebugPrint("   LAST: %02x:%02x:%02x:%02x:%02x:%02x\n", cb->linkKeyAddr[0], cb->linkKeyAddr[1],
                               cb->linkKeyAddr[2], cb->linkKeyAddr[3], cb->linkKeyAddr[4], cb->linkKeyAddr[5]);
            }
            entry++;
        }
        break;
    case 2:
        WUDiDebugPrint("BTM_CB_EVT_READ_STORED_LINK_KEYS\n");
        WUDiDebugPrint("  status: %d  max_keys: %d  num_keys: %d\n", event->read.status, event->read.maxKeys,
                       event->read.numKeys);
        cb->linkKeyState = 0;
        break;
    case 3:
        WUDiDebugPrint("BTM_CB_EVT_WRITE_STORED_LINK_KEYS\n");
        WUDiDebugPrint("  status: %d  num_keys: %d\n", event->write.status, event->write.numKeys);
        cb->linkKeyState = 0;
        break;
    case 4:
        WUDiDebugPrint("BTM_CB_EVT_DELETE_STORED_LINK_KEYS\n");
        WUDiDebugPrint("  status: %d  num_keys: %d\n", event->erase.status, event->erase.numKeys);
        if (event->erase.numKeys == 0) {
            WUDiDebugPrint("WARNING: no entry is deleted\n");
        }
        cb->linkKeyState = 0;
        break;
    default:
        OSPanic("WUD.c", 5498, "Unknown event\n");
        break;
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
        if (memcmp(wudStoredDeviceInfo.bdAddr, bdAddr, 6) == 0) {
            device = &wudStoredDeviceInfo;
        } else {
            WUDiDebugPrint("Unknown device is connected and changes the connection type!!!!\n");
            WUDiDebugPrint(" addr = %02x:%02x:%02x:%02x:%02x:%02x,  status = %d\n", bdAddr[0], bdAddr[1], bdAddr[2], bdAddr[3], bdAddr[4],
                           bdAddr[5], status);
            return;
        }
    }
    switch ((s32)status) {
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

/* Stores `bdAddr` (or clears the entry when NULL) as the history entry of channel `chan` and marks the table dirty. */
void WUDSetDeviceHistory(s32 chan, const u8* bdAddr)
{
    if (bdAddr == NULL) {
        memset(wudBtDeviceInfo.devices[chan + 10].bdAddr, 0, 0x46);
    } else {
        memcpy(wudBtDeviceInfo.devices[chan + 10].bdAddr, bdAddr, 6);
    }
    wudDeviceInfoDirty = 1;
}

/* Returns whether channel `chan` already stores `bdAddr`. */
s32 WUDIsHistoryAddr(s32 chan, const u8* bdAddr)
{
    if (bdAddr == NULL) {
        return 0;
    }
    return memcmp(wudBtDeviceInfo.devices[chan + 10].bdAddr, bdAddr, 6) == 0;
}

/* Writes the device table back to the console settings when it changed. */
void WUDiFlushDeviceInfo(void)
{
    if (wudDeviceInfoDirty != 0 && SCSetBtDeviceInfoArray(&wudBtDeviceInfo) != 0) {
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

/* Puts the stack into test mode (GUESS body: only its log string is retail evidence; the linker strips it). */
void _WUDEnableTestMode(void)
{
    WUDiDebugPrint("_WUDEnableTestMode\n");
}

/* Starts pairing the Wii remotes from the test tools (GUESS body: only its log string is retail evidence; the linker
 * strips it). */
s32 _WUDStartSyncDevice(void)
{
    WUDiDebugPrint("_WUDStartSyncDevice()\n");
    return WUDiStartSync(0, 3, 0, 0);
}

/* Drops the stored balance board pairing (GUESS body: only its log strings are retail evidence; the linker strips it). */
void _WUDDeleteStoreDevice(void)
{
    WUDDevice* device;

    WUDiDebugPrint("_WUDDeleteStoreDevice()\n");
    WUDiDebugPrint("dev number = %d\n", wudGetDeviceCount());
    if (WUDiGetDevInfo(wudStoredDeviceInfo.bdAddr) == NULL) {
        WUDiDebugPrint("Not Found");
    }
    device = wudFindLinkedWbc();
    if (device != NULL) {
        WUDiDebugPrint("Found the registered WBC\n");
        WUDiRemoveDevice(device->bdAddr);
        WUDiDebugPrint("Removed\n");
    } else {
        WUDiDebugPrint("Not Found WBC\n");
    }
}
