/* WUD/wud.h - the entry points of `WUD/wud.cpp` that the Wii remote driver (`WPAD/wpad.cpp`) calls.
 *   NAMES are the `WUD...()` log strings of the unit where it prints one, otherwise GUESSes from the caller. */
#ifndef WUD_WUD_H
#define WUD_WUD_H

#include "types.h"
#include "OS/OSAlarm.h"
#include "OS/OSAlarm.h"

/* size: 0x60 - one paired device record: its Bluetooth address and link state. */
typedef struct WUDDevice {
    /* +0x00 */ u8 name[0x40];
    /* +0x40 */ u8 bdAddr[6];
    /* +0x46 */ u8 linkKey[0x10];
    /* +0x56 */ u8 handle;
    /* +0x57 */ u8 subClass;
    /* +0x58 */ u8 appId;
    /* +0x59 */ u8 linkState;
    /* +0x5A */ u8 pad_0x5A;
    /* +0x5B */ u8 type;
    /* +0x5C */ u8 pad_0x5C[2];
    /* +0x5E */ u16 attrMask;
} WUDDevice; /* size: 0x60 */

/* size: 0x0C - one entry of a recently-used device list: the record and its neighbours. */
typedef struct WUDDeviceNode {
    /* +0x00 */ WUDDevice* device;
    /* +0x04 */ struct WUDDeviceNode* prev;
    /* +0x08 */ struct WUDDeviceNode* next;
} WUDDeviceNode;

/* Reports the result of a pairing or clear request (0 = success). */
typedef void (*WUDResultCallback)(s32 result);

/* The pairing result callbacks are invoked with the result and the number of retries. */
typedef void (*WUDSyncCallback)(s32 result, u8 retries);

#ifdef __cplusplus
extern "C" {
#endif

/* 0x804FCDD0 - starts the Bluetooth stack; non-zero when it came up. */
s32 WUDInit(void);
/* 0x804FCEF0 - hands the stack its allocate and free callbacks. */
void WUDRegisterAllocator(void* (*alloc)(u32 size), s32 (*dealloc)(void* block)); /* untyped: the caller-owned block */
/* 0x804FCF40 - shuts the stack down (`flag` selects the soft path). */
void WUDShutdown(s32 flag);
/* 0x804FD160 - the stack state (0 = off, 1/2 = starting, 4 = stopping, 3 = running). */
s32 WUDGetStatus(void);
/* 0x804FD1A0 - the number of free send buffers' status byte. */
u8 WUDGetBufferStatus(void);
/* 0x804FD260 / 0x804FD2C0 / 0x804FD320 - sync and clear result callbacks. */
WUDResultCallback WUDSetSyncDeviceCallback(WUDResultCallback callback);
WUDResultCallback WUDSetSyncSimpleCallback(WUDResultCallback callback);
WUDResultCallback WUDSetClearDeviceCallback(WUDResultCallback callback);
/* 0x804FD4D0 / 0x804FD4F0 / 0x804FD5A0 / 0x804FD630 - start and stop device pairing. */
void WUDStartSyncDevice(void);
void WUDStartFastSyncSimple(void);
s32 WUDStopSyncSimple(void);
s32 WUDStartClearDevice(void);
/* 0x804FD510 - cancels a running pairing search. */
s32 WUDStopSyncDevice(void);
/* 0x804FD750 - masks Bluetooth channels (AFH). */
s32 WUDSetDisableChannel(s32 channel);
/* 0x804FD840 / 0x804FD8A0 - install the HID receive and connection callbacks; return the old one. */
typedef void (*WUDHidRecvCallback)(u8 handle, u8* report, u16 length);
WUDHidRecvCallback WUDSetHidRecvCallback(WUDHidRecvCallback callback);
typedef void (*WUDHidConnCallback)(u8 handle, s32 event);
WUDHidConnCallback WUDSetHidConnCallback(WUDHidConnCallback callback);
/* 0x804FD900 - makes the host visible and connectable. */
void WUDSetVisibility(u8 discoverable, u8 connectable);
/* 0x804FEE50 - non-zero while a pairing search is running. */
s32 WUDIsSyncing(void);
/* 0x804FFE10 .. 0x804FFF30 - per-device lookups. */
u8* _WUDGetDevAddr(u8 handle);
u16 _WUDGetQueuedSize(s8 handle);
u16 _WUDGetNotAckedSize(s8 handle);
u8 _WUDGetLinkNumber(void);
/* 0x804FFF80 - stores `bdAddr` as the history entry of channel `chan`. */
void WUDSetDeviceHistory(s32 chan, const u8* bdAddr);
/* 0x80500000 - whether channel `chan` already stores `bdAddr`. */
s32 WUDIsHistoryAddr(s32 chan, const u8* bdAddr);
/* 0x804F9B30 - whether the balance board is linked. */
s32 WUDIsLinkedWBC(void);
/* 0x80500720 - takes a log line and discards it. */
void WUDiDebugPrint(const char* format, ...);
/* 0x804F9B40 / 0x804F9B90 - the stack's memory callbacks. */
void* App_MEMalloc(u32 size); /* untyped: caller-owned block */
void App_MEMfree(void* block); /* untyped: the caller-owned block */
/* 0x804F9BE0 - the device table flush callback of the pairing state machine; 0x804FB260 / 0x804FBB60 / 0x804FC900 are the clear, bring-up and shutdown ones. */
void SyncFlushCallback(s32 result);
void DeleteFlushCallback(s32 result);
void InitFlushCallback(s32 result);
void ShutdownFlushCallback(s32 result);
/* 0x804FA3F0 .. 0x804FA580 - the NAND open, seek, write and close callbacks of the stored-device file. */
void WUDiNandOpenCallback(s32 result);
void WUDiNandSeekCallback(s32 result);
void WUDiNandWriteCallback(s32 result);
void WUDiNandCloseCallback(s32 result);
/* 0x804FB240 / 0x804FB8C0 / 0x804FBB40 / 0x804FC8E0 / 0x804FCA10 - the alarm handlers that switch to the state machine fibers; the fibers are 0x804FA750 / 0x804FB570 / 0x804FB9F0 / 0x804FC7C0 / 0x804FC950. */
void WUDiSyncAlarmHandler(OSAlarm* alarm, OSContext* context);
void WUDiClearAlarmHandler(OSAlarm* alarm, OSContext* context);
void WUDiInitFlushAlarmHandler(OSAlarm* alarm, OSContext* context);
void WUDiInitAlarmHandler(OSAlarm* alarm, OSContext* context);
void WUDiShutdownAlarmHandler(OSAlarm* alarm, OSContext* context);
void WUDiSyncFiber(void);
void WUDiClearFiber(void);
void WUDiInitFlushFiber(void);
void WUDiInitFiber(void);
void WUDiShutdownFiber(void);
/* 0x804FB2C0 - drops the link of every connected device. */
s32 WUDiTerminateDevice(void);
/* 0x804FC1F0 / 0x804FC270 - the firmware patch sequence's step callbacks. */
void WUDiPatchCallback(s32 result);
void WUDiPatchStepDone(void);
/* 0x804FD990 .. 0x804FDF00 - the firmware patch download: record and code write callbacks, the patch removal and peek-poke callbacks and the start. */
void WUDiPatchRecordCallback(s32 result);
void WUDiPatchWriteCallback(s32 result);
void RemovePatchCallback(s32 result);
void SuperPeekPokeCallback(s32 result);
void __wudAppendRuntimePatch(void);
/* 0x804FF680 / 0x804FFCE0 - the stack's vendor event and power mode callbacks. */
void WUDiVendorEventCallback(u8 length, u8* data);
void __wudPowerMangeEventStackCallback(const u8* bdAddr, u32 status, u16 value, u8 hciStatus);
/* 0x804FD960 / 0x804FE000 - the firmware download's completion callback and the stack setup it starts. */
void WUDiFirmwareDoneCallback(void);
void __wudInitSub(void);
/* 0x804FE170 / 0x804FE2D0 - adds or removes a device in the stack's device database and the HID host. */
void WUDiAddDevice(const u8* bdAddr);
void WUDiRemoveDevice(const u8* bdAddr);
/* 0x804FA5E0 - finishes a pairing search and reports it; 0x804FF470 - the stack's device search event callback. */
s32 WUDiSyncDone(void);
void WUDiSearchCallback(s32 event, const u8* data);
/* 0x804F9C60 - stops advertising and picks the next pairing step. */
s32 WUDiSyncCheckSearch(void);
/* 0x804F9DC0 - opens the HID link of the device a pairing inquiry found; returns the next sync step. */
s32 WUDiSyncCheckInquiry(void);
/* 0x804FE450 - finds the stored record of the device at `bdAddr`, or NULL. */
WUDDevice* WUDiGetDevInfo(const u8* bdAddr);
/* 0x804FE530 / 0x804FE650 / 0x804FE8E0 / 0x804FEA00 - move a device's list entry to the head or the tail of the balance board list or the remote list. */
void WUDiMoveWbcToHead(WUDDevice* device);
void WUDiMoveWbcToTail(WUDDevice* device);
void WUDiMoveDeviceToHead(WUDDevice* device);
void WUDiMoveDeviceToTail(WUDDevice* device);
/* 0x804FEED0 - the stack shutdown callback. */
void CleanupCallback(s32 result);
/* 0x804FD380 - starts the pairing state machine. */
s32 WUDiStartSync(s8 simple, s8 searchMode, s8 aux, s32 fast);
/* 0x804FD1F0 - requests sniff mode with `interval` for the device at `bdAddr`. */
void WUDSetSniffMode(const u8* bdAddr, s16 interval);
/* 0x804FF930 - the stack's device status callback. */
void WUDDeviceStatusCallback(u32 event);
/* 0x804FFF70 - the stored-device work area. */
WUDDevice* WUDiGetStoredDeviceInfo(void);
/* 0x80500060 - writes the device table back to the console settings when it changed. */
void WUDiFlushDeviceInfo(void);
/* 0x805000B0 .. 0x80500110 - the per-device table setters and the unchecked getter. */
void _WUDSetDevAddr(u8 handle, u8* addr);
u8* _WUDGetDevAddrUnchecked(u8 handle);
void _WUDSetQueuedSize(u8 handle, u16 size);
void _WUDSetNotAckedSize(u8 handle, u16 size);
/* 0x805006F0 / 0x80500700 / 0x80500710 - HID host open and close logging, and a constant zero. */
void bta_hh_co_data(u8 handle, u8* report, u16 length, u8 mode, u8 subClass, u8 appId);
void bta_hh_co_open(void);
void bta_hh_co_close(void);
s32 WUDiReturnZero(void);

#ifdef __cplusplus
}
#endif

#endif /* WUD_WUD_H */
