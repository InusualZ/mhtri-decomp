/*
 * SI/SIBios.h - declarations of the symbols owned by `SI/SIBios.c` that other units call or read.
 */
#ifndef SI_SIBIOS_H
#define SI_SIBIOS_H

#include "types.h"
#include "OS/OSAlarm.h"
#include "OS/OSContext.h"

/* The callback a finished SI transfer runs: the channel, its status nibble and the interrupted context. */
typedef void (*SICallback)(s32 chan, u32 status, OSContext* context);

/* The callback of an asynchronous type probe: the channel and its type word. */
typedef void (*SITypeCallback)(s32 chan, u32 type);

/* The handler of the controller-polling interrupt: the interrupt source and the interrupted context. */
typedef void (*SIPollingHandler)(s16 interrupt, OSContext* context);

#ifdef __cplusplus
extern "C" {
#endif

/* 0x804DD9C0 / 0x804DD9E0 - whether a transfer is running / whether `chan` has one running or queued. */
BOOL SIBusy(void);
BOOL SIIsChanBusy(s32 chan);

/* 0x804DDA20 - finishes the transfer in flight and returns its status nibble. */
u32 CompleteTransfer(void);

/* 0x804DDD20 - the SI interrupt: completes the transfer, starts the next queued one, runs the polling handlers. */
void SIInterruptHandler(s16 interrupt, OSContext* context);

/* 0x804DE820 - starts a queued transfer once its alarm fires. */
void AlarmHandler(OSAlarm* alarm, OSContext* context);

/* 0x804DE110 - sets the polling interrupt mask and returns the previous one. */
BOOL SIEnablePollingInterrupt(BOOL enable);

/* 0x804DE190 - removes a registered polling handler; TRUE when it was registered. */
BOOL SIUnregisterPollingHandler(SIPollingHandler handler);

/* 0x804DE280 - hooks the SI interrupt and probes the four controller ports. */
void SIInit(void);

/* 0x804DE340 - starts one transfer now; FALSE while another one is running. */
/* untyped: the byte ranges of a transfer */
BOOL __SITransfer(s32 chan, void* output, u32 outBytes, void* input, u32 inBytes, SICallback callback);

/* 0x804DE4F0 / 0x804DE570 / 0x804DE590 - the channel status nibble, the channel's command word and the polling X/Y. */
u32 SIGetStatus(s32 chan);
void SISetCommand(s32 chan, u32 command);
u32 SISetXY(u32 x, u32 y);

/* 0x804DE5F0 / 0x804DE680 - enable / disable polling for the channels named in the top byte of `poll`. */
u32 SIEnablePolling(u32 poll);
u32 SIDisablePolling(u32 poll);

/* 0x804DE6F0 - copies the polled response of `chan` into `data`; FALSE when none arrived. */
BOOL SIGetResponse(s32 chan, u32* data);

/* 0x804DE8B0 - queues a transfer that starts `delay` ticks after the channel's previous one. */
/* untyped: the byte ranges of a transfer */
BOOL SITransfer(s32 chan, void* output, u32 outBytes, void* input, u32 inBytes, SICallback callback, s64 delay);

/* 0x804DECE0 / 0x804DEEA0 - the channel's device type word, immediately / through `callback` once known. */
u32 SIGetType(s32 chan);
u32 SIGetTypeAsync(s32 chan, SITypeCallback callback);

/* 0x804DEFB0 / 0x804DF090 - selects the polling rate (0..11 ms) and reapplies the current one. */
void SISetSamplingRate(u32 msec);
void SIRefreshSamplingRate(void);

#ifdef __cplusplus
}
#endif

#endif
