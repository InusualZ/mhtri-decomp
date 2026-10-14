/*
 * DSP/dsp.c - the SDK DSP interface: mailbox access, initialisation and the public task calls.
 *
 * RANGE. .text 0x804A4F20-0x804A51C0 (9 functions); .data 0x80618508-0x80618588 (the version string, the DSPInit
 *   build-date format and its date/time strings); .sdata 0x80793DC8-0x80793DD0 (__DSPVersion); .sbss
 *   0x80795050-0x80795058 (__DSP_init_flag).
 *   - the handler and __DSP_exec_task/__DSP_boot_task call the mailbox leaves out of line, which one TU under
 *     `-O4,p -inline auto` inlines, and the .sbss ends with 4 unread bytes before the 8-aligned DSP/dsp_task.c .sbss:
 *     the leaves are in this TU, the task manager in another.
 * FLAGS. cflags_base in the `OS` lib block of configure.py (Wii/1.3); no pragma; no float code.
 * NAMES. the SDK's names (the map's, and the version string's library); the register macros are this file's.
 * RESIDUALS. none in .text; the .data/.sdata/.sbss tails (0x3/0x4/0x4 B) are alignment fill flipcheck accepts. The
 *   map's string lump lbl_80618508 pairs against the compiler's anonymous pool, so the DSPInit string relocations
 *   differ by name only.
 * SHAPES. the DSPTask state/priority/flags fields are volatile (DSP/dsp.h): DSPAddTask's two stores stay in source
 *   order. DSPInit prints the literal build date and time, not __DATE__/__TIME__, and clears the queue with one
 *   chained assignment (tmp, curr, last, first stored in that order). DSPAssertTask's inner disable/restore keeps its
 *   own level. The registers are absolute-address macros, one `lis` per function.
 */
#include "types.h"
#include "DSP/dsp.h"
#include "DSP/dsp_debug.h"
#include "DSP/dsp_task.h"
#include "OS/OSContext.h"
#include "OS/OS.h"
#include "OS/OSInterrupt.h"

/* The DSP interface registers (0xCC005000 block). */
#define DSP_MAILBOX_TO_HI (*(volatile u16*)0xCC005000)
#define DSP_MAILBOX_TO_LO (*(volatile u16*)0xCC005002)
#define DSP_MAILBOX_FROM_HI (*(volatile u16*)0xCC005004)
#define DSP_MAILBOX_FROM_LO (*(volatile u16*)0xCC005006)
#define DSP_CONTROL (*(volatile u16*)0xCC00500A)

/* The OS interrupt index and mask bit of the DSP interrupt. */
#define DSP_INTERRUPT 7
#define DSP_INTERRUPT_MASK 0x01000000

const char* __DSPVersion = "<< RVL_SDK - DSP \trelease build: Feb 27 2009 10:01:57 (0x4302_145) >>";

static s32 __DSP_init_flag;

/* 0x804A4F20 (0x10): returns 1 while the CPU-to-DSP mailbox holds an unread mail. */
u32 DSPCheckMailToDSP(void)
{
    return (DSP_MAILBOX_TO_HI >> 15) & 1;
}

/* 0x804A4F30 (0x10): returns 1 when the DSP-to-CPU mailbox holds a mail. */
u32 DSPCheckMailFromDSP(void)
{
    return (DSP_MAILBOX_FROM_HI >> 15) & 1;
}

/* 0x804A4F40 (0x14): returns the 32-bit mail in the DSP-to-CPU mailbox. */
u32 DSPReadMailFromDSP(void)
{
    return (DSP_MAILBOX_FROM_HI << 16) | DSP_MAILBOX_FROM_LO;
}

/* 0x804A4F60 (0x14): writes a 32-bit mail to the CPU-to-DSP mailbox. */
void DSPSendMailToDSP(u32 mail)
{
    DSP_MAILBOX_TO_HI = mail >> 16;
    DSP_MAILBOX_TO_LO = mail & 0xFFFF;
}

/* 0x804A4F80 (0xC0): installs the DSP interrupt handler and clears the task queue once. */
void DSPInit(void)
{
    BOOL level;

    __DSP_debug_printf("DSPInit(): Build Date: %s %s\n", "Feb 27 2009", "10:01:57");

    if (__DSP_init_flag == 1) {
        return;
    }

    OSRegisterVersion(__DSPVersion);
    level = OSDisableInterrupts();
    __OSSetInterruptHandler(DSP_INTERRUPT, __DSPHandler);
    __OSUnmaskInterrupts(DSP_INTERRUPT_MASK);

    DSP_CONTROL = (u16)((DSP_CONTROL & ~0xA8) | 0x800);
    DSP_CONTROL = (u16)(DSP_CONTROL & ~0xAC);

    __DSP_first_task = __DSP_last_task = __DSP_curr_task = __DSP_tmp_task = NULL;
    __DSP_init_flag = 1;
    OSRestoreInterrupts(level);
}

/* 0x804A5040 (0x8): returns whether DSPInit has run. */
s32 DSPCheckInit(void)
{
    return __DSP_init_flag;
}

/* 0x804A5050 (0x70): queues a task and returns it. */
DSPTask* DSPAddTask(DSPTask* task)
{
    BOOL level;

    level = OSDisableInterrupts();
    __DSP_insert_task(task);
    task->state = DSP_TASK_STATE_INIT;
    task->flags = DSP_TASK_FLAG_ATTACHED;
    OSRestoreInterrupts(level);

    if (task == __DSP_first_task) {
        __DSP_boot_task(task);
    }
    return task;
}

/* 0x804A50C0 (0x40): flags a task for cancellation and returns it. */
DSPTask* DSPCancelTask(DSPTask* task)
{
    BOOL level;

    level = OSDisableInterrupts();
    task->flags |= DSP_TASK_FLAG_CANCEL;
    OSRestoreInterrupts(level);
    return task;
}

/* 0x804A5100 (0xBC): makes a task preempt the running one; returns it, or NULL when it cannot. */
DSPTask* DSPAssertTask(DSPTask* task)
{
    BOOL level;

    level = OSDisableInterrupts();
    if (__DSP_curr_task == task) {
        __DSP_rude_task = task;
        __DSP_rude_task_pending = TRUE;
        OSRestoreInterrupts(level);
        return task;
    }

    if (task->priority < __DSP_curr_task->priority) {
        __DSP_rude_task = task;
        __DSP_rude_task_pending = TRUE;
        if (__DSP_curr_task->state == DSP_TASK_STATE_RUN) {
            BOOL inner = OSDisableInterrupts();
            DSP_CONTROL = (u16)((DSP_CONTROL & ~0xA8) | 0x2);
            OSRestoreInterrupts(inner);
        }
        OSRestoreInterrupts(level);
        return task;
    }

    OSRestoreInterrupts(level);
    return NULL;
}
