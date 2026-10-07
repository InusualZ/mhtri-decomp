/*
 * OS/OSStateTM.h - declarations of the symbols owned by `OS/OSStateTM.c` that other units call or read.
 */
#ifndef OS_OSSTATETM_H
#define OS_OSSTATETM_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The reset / power button handlers the STM event hook calls. */
typedef void (*OSResetCallback)(void);
typedef void (*OSPowerCallback)(void);

/* 0x804D56B0 / 0x804D57A0 - install the reset / power button handler (NULL restores the default); return the previous one,
 * NULL when it was the default. */
OSResetCallback OSSetResetCallback(OSResetCallback callback);
OSPowerCallback OSSetPowerCallback(OSPowerCallback callback);

/* 0x804D5890 - starts the STM (state/thermal) channel that reset and shutdown requests go through; TRUE when it is ready. */
BOOL __OSInitSTM(void);

/* 0x804D59B0 - powers the console down to standby; does not return. */
void __OSShutdownToSBY(void);

/* 0x804D5A30 - requests a hot reset; returns only on failure. */
void __OSHotReset(void);

/* 0x804D5AB0 - asks the STM to dim the video output; returns 1 when the request is in flight, 0 when one already is, an IOS error
 * otherwise. */
s32 __OSSetVIForceDimming(u32 enable, u32 step, u32 level);

/* 0x804D5BB0 - hands one idle-mode byte to the STM (ioctl 0x6002, 32-byte in/out blocks); -6 when the STM was never initialised.
 * Its only caller is `NWC24iPrepareShutdown`, which passes `SCIdleModeInfo.subIdle`. */
s32 SCSetIdleMode(u8 idleMode);

/* 0x804D5BF0 - removes the state-event handler. */
s32 __OSUnRegisterStateEvent(void);

#ifdef __cplusplus
}
#endif

#endif
