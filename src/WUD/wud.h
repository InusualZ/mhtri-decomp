/* WUD/wud.h - the entry points of `WUD/wud.cpp` that the Wii remote driver (`WPAD/wpad.cpp`) calls.
 *   NAMES are the `WUD...()` log strings of the unit where it prints one, otherwise GUESSes from the caller. */
#ifndef WUD_WUD_H
#define WUD_WUD_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x804FCDD0 - starts the Bluetooth stack; non-zero when it came up. */
s32 WUDInit(void);
/* 0x804FCEF0 - hands the stack its allocate and free callbacks. */
void WUDRegisterAllocator(s32 (*alloc)(void), s32 (*dealloc)(void));
/* 0x804FCF40 - shuts the stack down (`flag` selects the soft path). */
void WUDShutdown(s32 flag);
/* 0x804FD160 - the stack state (0 = off, 1/2 = starting, 4 = stopping, 3 = running). */
s32 WUDGetStatus(void);
/* 0x804FD1A0 - the number of free send buffers' status byte. */
u8 WUDGetBufferStatus(void);
/* 0x804FD260 / 0x804FD2C0 / 0x804FD320 - sync and clear result callbacks. */
void WUDSetSyncDeviceCallback(void (*callback)(s32 result));
void WUDSetSyncSimpleCallback(void (*callback)(s32 result));
void WUDSetClearDeviceCallback(void (*callback)(s32 result));
/* 0x804FD4D0 / 0x804FD4F0 / 0x804FD5A0 / 0x804FD630 - start and stop device pairing. */
void WUDStartSyncDevice(void);
void WUDStartFastSyncSimple(void);
void WUDStopSyncSimple(void);
void WUDStartClearDevice(void);
/* 0x804FD510 - cancels a running pairing search. */
void WUDStopSyncDevice(void);
/* 0x804FD750 - masks Bluetooth channels (AFH). */
void WUDSetDisableChannel(s32 mask);
/* 0x804FD840 / 0x804FD8A0 - install the HID receive and connection callbacks; return the old one. */
typedef void (*WUDHidRecvCallback)(u8 handle, u8* report);
WUDHidRecvCallback WUDSetHidRecvCallback(WUDHidRecvCallback callback);
typedef void (*WUDHidConnCallback)(u8 handle, s32 event);
WUDHidConnCallback WUDSetHidConnCallback(WUDHidConnCallback callback);
/* 0x804FD900 - makes the host visible and connectable. */
void WUDSetVisibility(u8 discoverable, u8 connectable);
/* 0x804FEE50 - non-zero while a pairing search is running. */
s32 WUDIsSyncing(void);
/* 0x804FFE10 .. 0x804FFF30 - per-device lookups. */
u8* _WUDGetDevAddr(u8 handle);
u32 _WUDGetQueuedSize(s8 handle);
u16 _WUDGetNotAckedSize(s8 handle);
s32 _WUDGetLinkNumber(void);
/* 0x804FFF80 - stores `bdAddr` as the history entry of channel `chan`. */
void WUDSetDeviceHistory(s32 chan, const u8* bdAddr);
/* 0x80500000 - whether channel `chan` already stores `bdAddr`. */
s32 WUDIsHistoryAddr(s32 chan, const u8* bdAddr);
/* 0x804F9B30 - whether the balance board is linked. */
s32 WUDIsLinkedWBC(void);

#ifdef __cplusplus
}
#endif

#endif /* WUD_WUD_H */
