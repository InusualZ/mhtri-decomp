/*
 * DSP/dsp_task.h - the declarations of the symbols owned by `DSP/dsp_task.c`: the DSP interrupt handler, the task
 *   queue calls and the task queue globals.
 */
#ifndef DSP_DSP_TASK_H
#define DSP_DSP_TASK_H

#include "types.h"
#include "DSP/dsp.h"
#include "OS/OSContext.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The task the DSP is running. */
extern DSPTask* __DSP_curr_task;
/* The head of the task queue (highest priority). */
extern DSPTask* __DSP_first_task;
/* The tail of the task queue. */
extern DSPTask* __DSP_last_task;
/* Cleared by DSPInit; read by nothing. */
extern DSPTask* __DSP_tmp_task;
/* The task DSPAssertTask asked to run next. */
extern DSPTask* __DSP_rude_task;
/* Set while __DSP_rude_task waits for the next task switch. */
extern BOOL __DSP_rude_task_pending;

/* 0x804A5210 (0x42C): answers the DSP's mails: runs the task callbacks and switches tasks. */
void __DSPHandler(s16 interrupt, OSContext* context);
/* 0x804A5640 (0x1A4): saves the current task's DRAM image and starts or resumes the next one. */
void __DSP_exec_task(DSPTask* curr, DSPTask* next);
/* 0x804A57F0 (0x18C): boots the DSP with a task's IRAM image. */
void __DSP_boot_task(DSPTask* task);
/* 0x804A5980 (0xA0): inserts a task into the queue by priority. */
void __DSP_insert_task(DSPTask* task);
/* 0x804A5A20 (0x88): unlinks a finished task from the queue. */
void __DSP_remove_task(DSPTask* task);

#ifdef __cplusplus
}
#endif

#endif
