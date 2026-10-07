/*
 * KPR/kpr.h - declarations of the symbols owned by `KPR/kpr.c` that other units call.
 */
#ifndef KPR_KPR_H
#define KPR_KPR_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80526F00 - copy up to `max` queued u16 characters to `outAddress` under disabled interrupts and return
 * how many the queue holds (the bytes at +0x10/+0x11 summed); `(queue, 0, 0)` only counts. */
/* untyped: opaque band object, typed by the callers' views */
u32 KPRLookAhead(void* queue, u32 outAddress, u32 max);

#ifdef __cplusplus
}
#endif

#endif
