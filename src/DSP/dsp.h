/*
 * DSP/dsp.h - the DSP interface types and the declarations of the symbols owned by `DSP/dsp.c` that other units call.
 */
#ifndef DSP_DSP_H
#define DSP_DSP_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*DSPTaskCallback)(void);

/* size: 0x60; the DSP task descriptor, only the fields the AX output stage sets are named. */
typedef struct DSPTask {
    /* +0x00 */ u32 state;
    /* +0x04 */ u32 priority;
    /* +0x08 */ u32 flags;
    /* +0x0C */ void* iramMmemAddr; /* untyped: a DSP program image in main memory */
    /* +0x10 */ u32 iramLength;
    /* +0x14 */ u32 iramAddr;
    /* +0x18 */ void* dramMmemAddr; /* untyped: the task's DRAM image in main memory */
    /* +0x1C */ u32 dramLength;
    /* +0x20 */ u32 dramAddr;
    /* +0x24 */ u16 initVector;
    /* +0x26 */ u16 resumeVector;
    /* +0x28 */ DSPTaskCallback initCallback;
    /* +0x2C */ DSPTaskCallback resumeCallback;
    /* +0x30 */ DSPTaskCallback doneCallback;
    /* +0x34 */ DSPTaskCallback requestCallback;
    /* +0x38 */ u8 pad_0x38[0x28];
} DSPTask;

/* 0x804A4F20 */
u32 DSPCheckMailToDSP(void);
/* 0x804A4F60 */
void DSPSendMailToDSP(u32 mail);
/* 0x804A4F80 */
void DSPInit(void);
/* 0x804A5040 - returns whether DSPInit has run. */
s32 DSPCheckInit(void);
/* 0x804A5050 - queues a task and returns it. */
DSPTask* DSPAddTask(DSPTask* task);
/* 0x804A50C0 - flags a task for cancellation and returns it. */
DSPTask* DSPCancelTask(DSPTask* task);
/* 0x804A5100 */
void DSPAssertTask(DSPTask* task);

#ifdef __cplusplus
}
#endif

#endif
