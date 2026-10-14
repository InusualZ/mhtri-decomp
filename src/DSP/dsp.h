/*
 * DSP/dsp.h - the DSP interface types and the declarations of the symbols owned by `DSP/dsp.c` that other units call.
 */
#ifndef DSP_DSP_H
#define DSP_DSP_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* DSPTask.state values. */
#define DSP_TASK_STATE_INIT 0
#define DSP_TASK_STATE_RUN 1
#define DSP_TASK_STATE_YIELD 2
#define DSP_TASK_STATE_DONE 3

/* DSPTask.flags bits. */
#define DSP_TASK_FLAG_CLEARALL 0x00000000
#define DSP_TASK_FLAG_ATTACHED 0x00000001
#define DSP_TASK_FLAG_CANCEL 0x00000002

struct DSPTask;
typedef void (*DSPTaskCallback)(struct DSPTask* task);

/* size: 0x60; the DSP task descriptor: the program images the task manager uploads and the task queue links. */
typedef struct DSPTask {
    /* +0x00 */ volatile u32 state;
    /* +0x04 */ volatile u32 priority;
    /* +0x08 */ volatile u32 flags;
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
    /* +0x38 */ struct DSPTask* next;
    /* +0x3C */ struct DSPTask* prev;
    /* +0x40 */ u8 pad_0x40[0x20];
} DSPTask;

/* 0x804A4F20 (0x10): returns 1 while the CPU-to-DSP mailbox holds an unread mail. */
u32 DSPCheckMailToDSP(void);
/* 0x804A4F30 (0x10): returns 1 when the DSP-to-CPU mailbox holds a mail. */
u32 DSPCheckMailFromDSP(void);
/* 0x804A4F40 (0x14): returns the 32-bit mail in the DSP-to-CPU mailbox. */
u32 DSPReadMailFromDSP(void);
/* 0x804A4F60 (0x14): writes a 32-bit mail to the CPU-to-DSP mailbox. */
void DSPSendMailToDSP(u32 mail);
/* 0x804A4F80 (0xC0): installs the DSP interrupt handler and clears the task queue once. */
void DSPInit(void);
/* 0x804A5040 (0x8): returns whether DSPInit has run. */
s32 DSPCheckInit(void);
/* 0x804A5050 (0x70): queues a task and returns it. */
DSPTask* DSPAddTask(DSPTask* task);
/* 0x804A50C0 (0x40): flags a task for cancellation and returns it. */
DSPTask* DSPCancelTask(DSPTask* task);
/* 0x804A5100 (0xBC): makes a task preempt the running one; returns it, or NULL when it cannot. */
DSPTask* DSPAssertTask(DSPTask* task);

#ifdef __cplusplus
}
#endif

#endif
