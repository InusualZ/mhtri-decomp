/*
 * OS/OSRtc.h - declarations of the symbols owned by `OS/OSRtc.c` that other units call or read.
 */
#ifndef OS_OSRTC_H
#define OS_OSRTC_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x804D2CD0 - reads the SRAM through the EXI bus and repairs its status word. */
void __OSInitSram(void);

/* 0x804D31B0 - the result of the last SRAM write-back. */
BOOL __OSSyncSram(void);

/* 0x804D31C0 - reads `length` bytes at `offset` of the EXI channel 0 boot ROM into `buffer`. */
/* untyped: the byte range of a transfer */
BOOL __OSReadROM(void* buffer, s32 length, s32 offset);

/* 0x804D32F0 / 0x804D3370 - read / write the wireless pad ID stored for `channel` in the SRAM. */
u16 OSGetWirelessID(s32 channel);
void OSSetWirelessID(s32 channel, u16 id);

/* 0x804D3410 / 0x804D3530 - read / clear the RTC flag register. */
BOOL __OSGetRTCFlags(u32* flags);
BOOL __OSClearRTCFlags(void);

#ifdef __cplusplus
}
#endif

#endif
