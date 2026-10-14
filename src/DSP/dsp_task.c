/*
 * DSP/dsp_task.c - the SDK DSP task manager: the DSP interrupt handler and the task queue.
 *
 * RANGE. .text 0x804A5210-0x804A5AB0 (5 functions); .data 0x80618588-0x806186C8 (the __DSP_boot_task and
 *   __DSP_add_task printf formats); .sbss 0x80795058-0x80795070 (the six task globals, in reverse declaration order).
 * FLAGS. cflags_base in the `OS` lib block of configure.py (Wii/1.3); no pragma; no float code.
 * NAMES. the SDK's names. __DSP_add_task is the SDK's name for the function whose message survives at .data
 *   0x80618698; its body is the SDK's append, a GUESS (no target bytes).
 * RESIDUALS.
 *   - __DSP_add_task is dead-stripped in the target; our object carries it (+0x58 .text with alignment), so flipcheck
 *     refuses on the .text size. A trial link with the three DSP units flipped strips it and hashes OK.
 *   - the .data tail (0x5 B) is alignment fill; the map's string lump lbl_80618588 pairs against the anonymous pool,
 *     so the __DSP_boot_task string relocations differ by name only.
 * SHAPES. __DSP_add_task is a plain static defined between __DSP_insert_task and __DSP_remove_task: its message lands
 *   after the __DSP_boot_task formats in .data. The globals are declared curr, first, last, tmp, rude_task,
 *   rude_task_pending (the .sbss is laid out in reverse). The DSPTask state/priority/flags fields are volatile
 *   (DSP/dsp.h): __DSP_remove_task's two stores stay in source order. Each mail is followed by its own wait loop.
 */
#include "types.h"
#include "DSP/dsp.h"
#include "DSP/dsp_debug.h"
#include "DSP/dsp_task.h"
#include "OS/OSContext.h"
#include "OS/OS.h"
#include "OS/OSInterrupt.h"

/* The DSP control/status register (0xCC00500A). */
#define DSP_CONTROL (*(volatile u16*)0xCC00500A)

/* The DSP-to-CPU task mails and the CPU-to-DSP replies. */
#define DSP_MAIL_INIT 0xDCD10000
#define DSP_MAIL_RESUME 0xDCD10001
#define DSP_MAIL_YIELD 0xDCD10002
#define DSP_MAIL_DONE 0xDCD10003
#define DSP_MAIL_REQUEST 0xDCD10004
#define DSP_REPLY_SWITCH 0xCDD10001
#define DSP_REPLY_STOP 0xCDD10002
#define DSP_REPLY_CONTINUE 0xCDD10003

DSPTask* __DSP_curr_task;
DSPTask* __DSP_first_task;
DSPTask* __DSP_last_task;
DSPTask* __DSP_tmp_task;
DSPTask* __DSP_rude_task;
BOOL __DSP_rude_task_pending;

/* 0x804A5210 (0x42C): answers the DSP's mails: runs the task callbacks and switches tasks. */
void __DSPHandler(s16 interrupt, OSContext* context)
{
    OSContext exceptionContext;
    u32 mail;

    DSP_CONTROL = (u16)((DSP_CONTROL & ~0x28) | 0x80);
    OSClearContext(&exceptionContext);
    OSSetCurrentContext(&exceptionContext);

    while (!DSPCheckMailFromDSP()) {
    }
    mail = DSPReadMailFromDSP();

    if ((__DSP_curr_task->flags & DSP_TASK_FLAG_CANCEL) && mail == DSP_MAIL_YIELD) {
        mail = DSP_MAIL_DONE;
    }

    switch (mail) {
    case DSP_MAIL_INIT:
        __DSP_curr_task->state = DSP_TASK_STATE_RUN;
        if (__DSP_curr_task->initCallback) {
            __DSP_curr_task->initCallback(__DSP_curr_task);
        }
        break;
    case DSP_MAIL_RESUME:
        __DSP_curr_task->state = DSP_TASK_STATE_RUN;
        if (__DSP_curr_task->resumeCallback) {
            __DSP_curr_task->resumeCallback(__DSP_curr_task);
        }
        break;
    case DSP_MAIL_YIELD:
        if (__DSP_rude_task_pending) {
            if (__DSP_curr_task == __DSP_rude_task) {
                DSPSendMailToDSP(DSP_REPLY_CONTINUE);
                while (DSPCheckMailToDSP()) {
                }
                __DSP_rude_task = NULL;
                __DSP_rude_task_pending = FALSE;
                if (__DSP_curr_task->resumeCallback) {
                    __DSP_curr_task->resumeCallback(__DSP_curr_task);
                }
            } else {
                DSPSendMailToDSP(DSP_REPLY_SWITCH);
                while (DSPCheckMailToDSP()) {
                }
                __DSP_exec_task(__DSP_curr_task, __DSP_rude_task);
                __DSP_curr_task->state = DSP_TASK_STATE_YIELD;
                __DSP_curr_task = __DSP_rude_task;
                __DSP_rude_task = NULL;
                __DSP_rude_task_pending = FALSE;
            }
            break;
        }

        if (__DSP_curr_task->next == NULL) {
            if (__DSP_curr_task == __DSP_first_task) {
                DSPSendMailToDSP(DSP_REPLY_CONTINUE);
                while (DSPCheckMailToDSP()) {
                }
                if (__DSP_curr_task->resumeCallback) {
                    __DSP_curr_task->resumeCallback(__DSP_curr_task);
                }
            } else {
                DSPSendMailToDSP(DSP_REPLY_SWITCH);
                while (DSPCheckMailToDSP()) {
                }
                __DSP_exec_task(__DSP_curr_task, __DSP_first_task);
                __DSP_curr_task->state = DSP_TASK_STATE_YIELD;
                __DSP_curr_task = __DSP_first_task;
            }
        } else {
            DSPSendMailToDSP(DSP_REPLY_SWITCH);
            while (DSPCheckMailToDSP()) {
            }
            __DSP_exec_task(__DSP_curr_task, __DSP_curr_task->next);
            __DSP_curr_task->state = DSP_TASK_STATE_YIELD;
            __DSP_curr_task = __DSP_curr_task->next;
        }
        break;
    case DSP_MAIL_DONE:
        if (__DSP_rude_task_pending) {
            if (__DSP_curr_task != __DSP_rude_task) {
                if (__DSP_curr_task->doneCallback) {
                    __DSP_curr_task->doneCallback(__DSP_curr_task);
                }
                DSPSendMailToDSP(DSP_REPLY_SWITCH);
                while (DSPCheckMailToDSP()) {
                }
                __DSP_exec_task(NULL, __DSP_rude_task);
                __DSP_remove_task(__DSP_curr_task);
                __DSP_curr_task = __DSP_rude_task;
                __DSP_rude_task = NULL;
                __DSP_rude_task_pending = FALSE;
                break;
            }
            __DSP_rude_task = NULL;
            __DSP_rude_task_pending = FALSE;
        }

        if (__DSP_curr_task->next == NULL) {
            if (__DSP_curr_task == __DSP_first_task) {
                if (__DSP_curr_task->doneCallback) {
                    __DSP_curr_task->doneCallback(__DSP_curr_task);
                }
                DSPSendMailToDSP(DSP_REPLY_STOP);
                while (DSPCheckMailToDSP()) {
                }
                __DSP_curr_task->state = DSP_TASK_STATE_DONE;
                __DSP_remove_task(__DSP_curr_task);
            } else {
                if (__DSP_curr_task->doneCallback) {
                    __DSP_curr_task->doneCallback(__DSP_curr_task);
                }
                DSPSendMailToDSP(DSP_REPLY_SWITCH);
                while (DSPCheckMailToDSP()) {
                }
                __DSP_curr_task->state = DSP_TASK_STATE_DONE;
                __DSP_exec_task(NULL, __DSP_first_task);
                __DSP_curr_task = __DSP_first_task;
                __DSP_remove_task(__DSP_last_task);
            }
        } else {
            if (__DSP_curr_task->doneCallback) {
                __DSP_curr_task->doneCallback(__DSP_curr_task);
            }
            DSPSendMailToDSP(DSP_REPLY_SWITCH);
            while (DSPCheckMailToDSP()) {
            }
            __DSP_curr_task->state = DSP_TASK_STATE_DONE;
            __DSP_exec_task(NULL, __DSP_curr_task->next);
            __DSP_curr_task = __DSP_curr_task->next;
            __DSP_remove_task(__DSP_curr_task->prev);
        }
        break;
    case DSP_MAIL_REQUEST:
        if (__DSP_curr_task->requestCallback) {
            __DSP_curr_task->requestCallback(__DSP_curr_task);
        }
        break;
    }

    OSClearContext(&exceptionContext);
    OSSetCurrentContext(context);
}

/* 0x804A5640 (0x1A4): saves the current task's DRAM image and starts or resumes the next one. */
void __DSP_exec_task(DSPTask* curr, DSPTask* next)
{
    if (curr != NULL) {
        DSPSendMailToDSP((u32)curr->dramMmemAddr);
        while (DSPCheckMailToDSP()) {
        }
        DSPSendMailToDSP(curr->dramLength);
        while (DSPCheckMailToDSP()) {
        }
        DSPSendMailToDSP(curr->dramAddr);
        while (DSPCheckMailToDSP()) {
        }
    } else {
        DSPSendMailToDSP(0);
        while (DSPCheckMailToDSP()) {
        }
        DSPSendMailToDSP(0);
        while (DSPCheckMailToDSP()) {
        }
        DSPSendMailToDSP(0);
        while (DSPCheckMailToDSP()) {
        }
    }

    DSPSendMailToDSP((u32)next->iramMmemAddr);
    while (DSPCheckMailToDSP()) {
    }
    DSPSendMailToDSP(next->iramLength);
    while (DSPCheckMailToDSP()) {
    }
    DSPSendMailToDSP(next->iramAddr);
    while (DSPCheckMailToDSP()) {
    }

    if (next->state == DSP_TASK_STATE_INIT) {
        DSPSendMailToDSP(next->initVector);
        while (DSPCheckMailToDSP()) {
        }
        DSPSendMailToDSP(0);
        while (DSPCheckMailToDSP()) {
        }
        DSPSendMailToDSP(0);
        while (DSPCheckMailToDSP()) {
        }
        DSPSendMailToDSP(0);
        while (DSPCheckMailToDSP()) {
        }
    } else {
        DSPSendMailToDSP(next->resumeVector);
        while (DSPCheckMailToDSP()) {
        }
        DSPSendMailToDSP((u32)next->dramMmemAddr);
        while (DSPCheckMailToDSP()) {
        }
        DSPSendMailToDSP(next->dramLength);
        while (DSPCheckMailToDSP()) {
        }
        DSPSendMailToDSP(next->dramAddr);
        while (DSPCheckMailToDSP()) {
        }
    }
}

/* 0x804A57F0 (0x18C): boots the DSP with a task's IRAM image. */
void __DSP_boot_task(DSPTask* task)
{
    volatile u32 mail;

    while (!DSPCheckMailFromDSP()) {
    }
    mail = DSPReadMailFromDSP();

    DSPSendMailToDSP(0x80F3A001);
    while (DSPCheckMailToDSP()) {
    }
    DSPSendMailToDSP((u32)task->iramMmemAddr);
    while (DSPCheckMailToDSP()) {
    }
    DSPSendMailToDSP(0x80F3C002);
    while (DSPCheckMailToDSP()) {
    }
    DSPSendMailToDSP(task->iramAddr & 0xFFFF);
    while (DSPCheckMailToDSP()) {
    }
    DSPSendMailToDSP(0x80F3A002);
    while (DSPCheckMailToDSP()) {
    }
    DSPSendMailToDSP(task->iramLength);
    while (DSPCheckMailToDSP()) {
    }
    DSPSendMailToDSP(0x80F3B002);
    while (DSPCheckMailToDSP()) {
    }
    DSPSendMailToDSP(0);
    while (DSPCheckMailToDSP()) {
    }
    DSPSendMailToDSP(0x80F3D001);
    while (DSPCheckMailToDSP()) {
    }
    DSPSendMailToDSP(task->initVector);
    while (DSPCheckMailToDSP()) {
    }

    __DSP_debug_printf("DSP is booting task: 0x%08X\n", task);
    __DSP_debug_printf("__DSP_boot_task()  : IRAM MMEM ADDR: 0x%08X\n", (u32)task->iramMmemAddr);
    __DSP_debug_printf("__DSP_boot_task()  : IRAM DSP ADDR : 0x%08X\n", task->iramAddr);
    __DSP_debug_printf("__DSP_boot_task()  : IRAM LENGTH   : 0x%08X\n", task->iramLength);
    __DSP_debug_printf("__DSP_boot_task()  : DRAM MMEM ADDR: 0x%08X\n", task->dramLength);
    __DSP_debug_printf("__DSP_boot_task()  : Start Vector  : 0x%08X\n", task->initVector);
}

/* 0x804A5980 (0xA0): inserts a task into the queue by priority. */
void __DSP_insert_task(DSPTask* task)
{
    DSPTask* temp;

    if (__DSP_first_task == NULL) {
        __DSP_first_task = __DSP_last_task = __DSP_curr_task = task;
        task->next = task->prev = NULL;
    } else {
        temp = __DSP_first_task;
        while (temp != NULL) {
            if (task->priority < temp->priority) {
                task->prev = temp->prev;
                temp->prev = task;
                task->next = temp;
                if (task->prev == NULL) {
                    __DSP_first_task = task;
                } else {
                    task->prev->next = task;
                }
                break;
            }
            temp = temp->next;
        }

        if (temp == NULL) {
            __DSP_last_task->next = task;
            task->next = NULL;
            task->prev = __DSP_last_task;
            __DSP_last_task = task;
        }
    }
}

/* Appends a task to the queue; no caller keeps it in the link. */
static void __DSP_add_task(DSPTask* task)
{
    if (__DSP_last_task == NULL) {
        __DSP_first_task = __DSP_last_task = __DSP_curr_task = task;
        task->next = task->prev = NULL;
    } else {
        __DSP_last_task->next = task;
        task->next = NULL;
        task->prev = __DSP_last_task;
        __DSP_last_task = task;
    }
    task->state = DSP_TASK_STATE_INIT;
    __DSP_debug_printf("__DSP_add_task() : Added task    : 0x%08X\n", task);
}

/* 0x804A5A20 (0x88): unlinks a finished task from the queue. */
void __DSP_remove_task(DSPTask* task)
{
    task->flags = DSP_TASK_FLAG_CLEARALL;
    task->state = DSP_TASK_STATE_DONE;

    if (__DSP_first_task == task) {
        if (task->next != NULL) {
            __DSP_first_task = task->next;
            task->next->prev = NULL;
        } else {
            __DSP_first_task = __DSP_last_task = __DSP_curr_task = NULL;
        }
    } else if (__DSP_last_task == task) {
        __DSP_last_task = task->prev;
        task->prev->next = NULL;
        __DSP_curr_task = __DSP_first_task;
    } else {
        __DSP_curr_task = task->next;
        task->prev->next = task->next;
        task->next->prev = task->prev;
    }
}
