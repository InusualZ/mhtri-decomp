/*
 * AI_SDK/ai.h - the audio interface (AI) library entry points, owned by `AI_SDK/ai.c`.
 */
#ifndef AI_SDK_AI_H
#define AI_SDK_AI_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*AIDMACallback)(void);

/* 0x8046D420 (0x44): installs the DMA-done callback and returns the previous one. */
AIDMACallback AIRegisterDMACallback(AIDMACallback callback);

/* 0x8046D470 (0x7C): programs the DMA source address and the length in 32-byte blocks. */
void AIInitDMA(u32 address, u32 length);

/* 0x8046D4F0 (0x14): starts the DMA transfer. */
void AIStartDMA(void);

/* 0x8046D510 (0x14): stops the DMA transfer. */
void AIStopDMA(void);

/* 0x8046D530 (0x10): returns the bytes still to transfer. */
u32 AIGetDMABytesLeft(void);

/* 0x8046D540 (0x18): returns the DMA source address programmed by AIInitDMA. */
u32 AIGetDMAStartAddr(void);

/* 0x8046D560 (0x10): returns the same count as AIGetDMABytesLeft. */
u32 __ARGetInterruptStatus(void);

/* 0x8046D570 (0x8): returns whether AIInit has run. */
s32 AICheckInit(void);

/* 0x8046D580 (0x180): initialises the audio interface; `stack` is the callback stack top. */
void AIInit(u8* stack);

/* 0x8046D820 (0x1CC): times the AI sample counter against the time base to pick the sample-rate window. */
void __AI_SRC_INIT(void);

/* 0x8046D700 (0xA8): AI interrupt handler: acknowledges the interrupt and runs the DMA callback. */
struct OSContext;
void __AIDHandler(s16 interrupt, struct OSContext* context);

/* 0x8046D7B0 (0x64): calls `callback` on the private callback stack. */
void __AICallbackStackSwitch(AIDMACallback callback);

#ifdef __cplusplus
}
#endif

#endif
