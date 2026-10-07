/*
 * KPR/kpr.h - declarations of the symbols owned by `KPR/kpr.c` that other units call, and the queue record.
 */
#ifndef KPR_KPR_H
#define KPR_KPR_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* size: 0x18 - the keyboard character queue: a few u16 characters, of which `ready` are complete and
 * `pending` still wait for a dead-key composition. */
typedef struct KPRQueue {
    /* +0x00 */ u16 chars[6];
    /* +0x0C */ u32 mode;    /* bit 0 Alt-code entry, bit 1 compose through the converter hook, bits 2/3 extra hook */
    /* +0x10 */ u8 ready;
    /* +0x11 */ u8 pending;
    /* +0x12 */ u8 pad_0x12[0x2];
    /* +0x14 */ u32 altCode;  /* Alt-code accumulator; bit 31 marks a leading 0 (code page 1252) */
} KPRQueue; /* size: 0x18 */

/* 0x80526F00 - copy up to `max` queued u16 characters to `outAddress` under disabled interrupts and return
 * how many the queue holds (the bytes at +0x10/+0x11 summed); `(queue, 0, 0)` only counts. */
/* untyped: opaque band object, typed by the callers' views */
u32 KPRLookAhead(void* queue, u32 outAddress, u32 max);

#ifdef __cplusplus
}
#endif

#endif
