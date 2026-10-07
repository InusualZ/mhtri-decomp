/*
 * OS/OSThread.c - the OS thread scheduler: thread init/create/exit/cancel/join/resume/suspend/sleep, run-queue
 *    handling and `OSSleepTicks`.
 * RANGE. .text 0x804D36D0-0x804D4D50 (23 functions); .bss 0x8074D698-0x8074E0A0; .sdata 0x80793FB8-0x80793FC0; .sbss
 *    0x80795390-0x807953A0.  Cut from the OS core band 0x804C1760-0x804D9B4C.  Evidence: the default thread, the
 *    run queue and the idle context/thread (.bss 0x8074D698..0x8074E0A0), `Reschedule`/`RunQueueHint`/`RunQueueBits`
 *    (.sbss 0x80795390..) and `SwitchThreadCallback` (.sdata 0x80793FB8) are read only by these functions.
 * FLAGS. `cflags_base` per object (configure.py).
 * NAMES. map names (`OSInitThreadQueue`, `OSCreateThread`, `OSSleepThread`, ...); GUESS: `RunQueue` (32 priority queues),
 *    `IdleContext` and `IdleThread` (the 0x5F0 bytes the map had as one object) and `SleepAlarmHandler` (the alarm
 *    callback `OSSleepTicks` arms); `__OSFpscrEnableBits` (0x80793F90, owned by OSError) is named for its use as the FPSCR
 *    enable mask of a new context.
 * RESIDUALS. `OSWakeupThread` (98.5 %): the woken thread and the run-queue tail temporary swap r7/r8; no declaration order fixed it.
 *    The seam: `SwitchThreadCallback`'s initial value is `DBClose` (0x804D36C0, the 4-byte `blr` the map puts in
 *    OSSync); it is this unit's default switch callback, so the OSThread range probably starts at 0x804D36C0.
 * SHAPES. `UnsetRun` needs `#pragma dont_inline` (the target calls it); the scheduler words are `volatile` (the target re-reads
 *    them after storing); `UpdatePriority` spells the effective-priority loop itself. The list operations are macros expanded in place (`AddTail`/`AddPrio`/`RemoveItem`/`DequeueHead`); the effective
 *    priority loop and the priority-inheritance walk are inline helpers.
 */

#define OS_THREAD_TYPED_API

#include "types.h"

#include "OS/OSAlarm.h"
#include "OS/OSContext.h"
#include "OS/OSError.h"
#include "OS/OSInterrupt.h"
#include "OS/OSMutex.h"
#include "OS/OSSetAlarm.h"
#include "OS/OSSync.h"
#include "OS/OSThread.h"

/* The low-memory words the scheduler shares with the context code: the running thread and the list of every thread. */
#define OS_CURRENT_THREAD (*(OSThread**)0x800000E4)
#define OS_ACTIVE_THREAD_QUEUE (*(OSThreadQueue*)0x800000DC)
#define OS_FPU_CONTEXT (*(OSContext**)0x800000D8)

#define OS_THREAD_STACK_MAGIC 0xDEADBABE
#define OS_PRIORITY_MIN 0
#define OS_PRIORITY_MAX 31
#define OS_PRIORITY_IDLE 32

/* Linker-script addresses of the boot stack. */
extern u8 _stack_addr[];
extern u8 _stack_end[];

typedef void (*OSSwitchThreadCallback)(OSThread* from, OSThread* to);

static OSThread DefaultThread;
static OSThreadQueue RunQueue[OS_PRIORITY_MAX + 1];
static OSContext IdleContext;
__declspec(export) OSThread IdleThread;
static OSSwitchThreadCallback SwitchThreadCallback = (OSSwitchThreadCallback)DBClose;

static volatile u32 RunQueueBits;
static volatile BOOL RunQueueHint;
static volatile s32 Reschedule;

/* Appends `thread` to `queue` through the list node `link`. */
#define AddTail(queue, thread, link)                                                                                   \
    do {                                                                                                               \
        OSThread* prev = (queue)->tail;                                                                                \
        if (prev == NULL) {                                                                                            \
            (queue)->head = (thread);                                                                                  \
        } else {                                                                                                       \
            prev->link.next = (thread);                                                                                \
        }                                                                                                              \
        (thread)->link.prev = prev;                                                                                    \
        (thread)->link.next = NULL;                                                                                    \
        (queue)->tail = (thread);                                                                                      \
    } while (0)

/* Inserts `thread` into `queue` ahead of the first thread of lower urgency. */
#define AddPrio(queue, thread, link)                                                                                   \
    do {                                                                                                               \
        OSThread* prev;                                                                                                \
        OSThread* next;                                                                                                \
        for (next = (queue)->head; next && next->priority <= (thread)->priority; next = next->link.next) {            \
        }                                                                                                              \
        if (next == NULL) {                                                                                            \
            AddTail(queue, thread, link);                                                                              \
        } else {                                                                                                       \
            (thread)->link.next = next;                                                                                \
            prev = next->link.prev;                                                                                    \
            next->link.prev = (thread);                                                                                \
            (thread)->link.prev = prev;                                                                                \
            if (prev == NULL) {                                                                                        \
                (queue)->head = (thread);                                                                              \
            } else {                                                                                                   \
                prev->link.next = (thread);                                                                            \
            }                                                                                                          \
        }                                                                                                              \
    } while (0)

/* Unlinks `thread` from `queue`. */
#define RemoveItem(queue, thread, link)                                                                                \
    do {                                                                                                               \
        OSThread* next = (thread)->link.next;                                                                          \
        OSThread* prev = (thread)->link.prev;                                                                          \
        if (next == NULL) {                                                                                            \
            (queue)->tail = prev;                                                                                      \
        } else {                                                                                                       \
            next->link.prev = prev;                                                                                    \
        }                                                                                                              \
        if (prev == NULL) {                                                                                            \
            (queue)->head = next;                                                                                      \
        } else {                                                                                                       \
            prev->link.next = next;                                                                                    \
        }                                                                                                              \
    } while (0)

/* Takes the first thread of `queue` into `thread`. */
#define DequeueHead(queue, thread, link)                                                                               \
    do {                                                                                                               \
        OSThread* next;                                                                                                \
        (thread) = (queue)->head;                                                                                      \
        next = (thread)->link.next;                                                                                    \
        if (next == NULL) {                                                                                            \
            (queue)->tail = NULL;                                                                                      \
        } else {                                                                                                       \
            next->link.prev = NULL;                                                                                    \
        }                                                                                                              \
        (queue)->head = next;                                                                                          \
    } while (0)

/* Puts `thread` on the run queue of its priority. */
#define SetRun(thread)                                                                                                 \
    do {                                                                                                               \
        (thread)->queue = &RunQueue[(thread)->priority];                                                               \
        AddTail((thread)->queue, thread, link);                                                                        \
        RunQueueBits |= 1u << (OS_PRIORITY_MAX - (thread)->priority);                                                  \
        RunQueueHint = TRUE;                                                                                           \
    } while (0)

/* Reschedules when a thread became runnable. */
#define RescheduleIfHinted()                                                                                           \
    do {                                                                                                               \
        if (RunQueueHint) {                                                                                            \
            SelectThread(FALSE);                                                                                       \
        }                                                                                                              \
    } while (0)

static OSThread* SelectThread(BOOL yield);

/* Turns the boot context into the default thread, clears the run queues and starts the scheduler. */
void __OSThreadInit(void)
{
    OSThread* thread = &DefaultThread;
    s32 priority;
    u32* clear;
    u32* stackEnd;
    u32* sp;

    thread->state = OS_THREAD_STATE_RUNNING;
    thread->attr = OS_THREAD_ATTR_DETACH;
    thread->priority = thread->base = 16;
    thread->suspend = 0;
    thread->val = (void*)0xFFFFFFFF;
    thread->mutex = NULL;
    thread->queueJoin.head = thread->queueJoin.tail = NULL;
    thread->queueMutex.head = thread->queueMutex.tail = NULL;

    OS_FPU_CONTEXT = &thread->context;
    OSClearContext(&thread->context);
    OSSetCurrentContext(&thread->context);

    thread->stackBase = _stack_addr;
    thread->stackEnd = _stack_end;
    *(u32*)thread->stackEnd = OS_THREAD_STACK_MAGIC;

    SwitchThreadCallback(OS_CURRENT_THREAD, thread);
    OS_CURRENT_THREAD = thread;

    sp = (u32*)OSGetStackPointer();
    stackEnd = (u32*)OS_CURRENT_THREAD->stackEnd;
    for (clear = stackEnd + 1; clear < sp; clear++) {
        *clear = 0;
    }

    RunQueueBits = 0;
    RunQueueHint = FALSE;
    for (priority = OS_PRIORITY_MIN; priority <= OS_PRIORITY_MAX; priority++) {
        RunQueue[priority].head = RunQueue[priority].tail = NULL;
    }

    OS_ACTIVE_THREAD_QUEUE.head = OS_ACTIVE_THREAD_QUEUE.tail = NULL;
    AddTail(&OS_ACTIVE_THREAD_QUEUE, thread, linkActive);

    OSClearContext(&IdleContext);
    Reschedule = 0;
}

/* Initialises a thread queue to empty. */
void OSInitThreadQueue(OSThreadQueue* queue)
{
    queue->head = queue->tail = NULL;
}

/* Returns the running thread. */
OSThread* OSGetCurrentThread(void)
{
    return OS_CURRENT_THREAD;
}

/* Returns whether `thread` has exited or was never created. */
BOOL OSIsThreadTerminated(OSThread* thread)
{
    return (thread->state == OS_THREAD_STATE_MORIBUND || thread->state == 0) ? TRUE : FALSE;
}

/* Returns whether `thread` is on the active-thread list. */
static inline BOOL IsThreadActive(OSThread* thread)
{
    OSThread* active;

    if (thread->state == 0) {
        return FALSE;
    }
    for (active = OS_ACTIVE_THREAD_QUEUE.head; active != NULL; active = active->linkActive.next) {
        if (thread == active) {
            return TRUE;
        }
    }
    return FALSE;
}

/* Stops thread switching and returns the previous nesting count. */
s32 OSDisableScheduler(void)
{
    BOOL enabled = OSDisableInterrupts();
    s32 count = Reschedule++;
    OSRestoreInterrupts(enabled);
    return count;
}

/* Allows thread switching again and returns the previous nesting count. */
s32 OSEnableScheduler(void)
{
    BOOL enabled = OSDisableInterrupts();
    s32 count = Reschedule--;
    OSRestoreInterrupts(enabled);
    return count;
}

/* Takes a ready thread off its run queue. */
#pragma dont_inline on
static void UnsetRun(OSThread* thread)
{
    OSThreadQueue* queue = thread->queue;

    RemoveItem(queue, thread, link);
    if (queue->head == NULL) {
        RunQueueBits &= ~(1u << (OS_PRIORITY_MAX - thread->priority));
    }
    thread->queue = NULL;
}
#pragma dont_inline reset

/* Returns the highest priority among the thread's base priority and the first waiter of each mutex it owns. */
s32 __OSGetEffectivePriority(OSThread* thread)
{
    s32 priority = thread->base;
    OSMutex* mutex;
    OSThread* waiter;

    for (mutex = thread->queueMutex.head; mutex != NULL; mutex = mutex->link.next) {
        waiter = mutex->queue.head;
        if (waiter != NULL && waiter->priority < priority) {
            priority = waiter->priority;
        }
    }
    return priority;
}

/* Moves `thread` to `priority` in whatever queue it sits on and returns the owner of the mutex it waits for. */
static OSThread* SetEffectivePriority(OSThread* thread, s32 priority)
{
    switch (thread->state) {
    case OS_THREAD_STATE_READY:
        UnsetRun(thread);
        thread->priority = priority;
        SetRun(thread);
        break;
    case OS_THREAD_STATE_WAITING:
        RemoveItem(thread->queue, thread, link);
        thread->priority = priority;
        AddPrio(thread->queue, thread, link);
        if (thread->mutex) {
            return thread->mutex->thread;
        }
        break;
    case OS_THREAD_STATE_RUNNING:
        RunQueueHint = TRUE;
        thread->priority = priority;
        break;
    }
    return NULL;
}

/* Re-derives the priority of `thread` and of each mutex owner it is blocked behind. */
static inline void UpdatePriority(OSThread* thread)
{
    s32 priority;
    OSMutex* mutex;

    do {
        if (thread->suspend > 0) {
            break;
        }
        priority = thread->base;
        for (mutex = thread->queueMutex.head; mutex != NULL; mutex = mutex->link.next) {
            if (mutex->queue.head != NULL && mutex->queue.head->priority < priority) {
                priority = mutex->queue.head->priority;
            }
        }
        if (thread->priority == priority) {
            break;
        }
        thread = SetEffectivePriority(thread, priority);
    } while (thread);
}

/* Raises `thread` and the owners it waits behind to at least `priority`. */
void __OSPromoteThread(OSThread* thread, s32 priority)
{
    do {
        if (thread->suspend > 0 || thread->priority <= priority) {
            break;
        }
        thread = SetEffectivePriority(thread, priority);
    } while (thread);
}

/* Switches to the best runnable thread, idling when none is; returns NULL when no switch happened. */
static OSThread* SelectThread(BOOL yield)
{
    OSContext* currentContext;
    OSThread* currentThread;
    OSThread* nextThread;
    s32 priority;
    OSThreadQueue* queue;

    if (Reschedule > 0) {
        return NULL;
    }
    currentContext = OSGetCurrentContext();
    currentThread = OS_CURRENT_THREAD;
    if (currentContext != &currentThread->context) {
        return NULL;
    }
    if (currentThread) {
        if (currentThread->state == OS_THREAD_STATE_RUNNING) {
            if (!yield) {
                priority = __cntlzw(RunQueueBits);
                if (currentThread->priority <= priority) {
                    return NULL;
                }
            }
            currentThread->state = OS_THREAD_STATE_READY;
            SetRun(currentThread);
        }
        if (!(currentThread->context.state & 2) && OSSaveContext(&currentThread->context)) {
            return NULL;
        }
    }
    if (RunQueueBits == 0) {
        SwitchThreadCallback(OS_CURRENT_THREAD, NULL);
        OS_CURRENT_THREAD = NULL;
        OSSetCurrentContext(&IdleContext);
        do {
            OSEnableInterrupts();
            while (RunQueueBits == 0) {
            }
            OSDisableInterrupts();
        } while (RunQueueBits == 0);
        OSClearContext(&IdleContext);
    }
    RunQueueHint = FALSE;
    priority = __cntlzw(RunQueueBits);
    queue = &RunQueue[priority];
    DequeueHead(queue, nextThread, link);
    if (queue->head == NULL) {
        RunQueueBits &= ~(1u << (OS_PRIORITY_MAX - priority));
    }
    nextThread->queue = NULL;
    nextThread->state = OS_THREAD_STATE_RUNNING;
    SwitchThreadCallback(OS_CURRENT_THREAD, nextThread);
    OS_CURRENT_THREAD = nextThread;
    OSSetCurrentContext(&nextThread->context);
    OSLoadContext(&nextThread->context);
    return nextThread;
}

/* Switches threads when a more urgent one became runnable. */
void __OSReschedule(void)
{
    if (RunQueueHint) {
        SelectThread(FALSE);
    }
}

/* Lets every ready thread of the same priority run first. */
void OSYieldThread(void)
{
    BOOL enabled = OSDisableInterrupts();

    SelectThread(TRUE);
    OSRestoreInterrupts(enabled);
}

/* Builds a suspended thread on `stack` that runs `entry(param)` and registers it as active; fails on a bad priority. */
/* untyped: caller-owned payload and a raw stack range */
BOOL OSCreateThread(OSThread* thread, OSThreadEntry entry, void* param, void* stack, u32 stackSize, s32 priority,
                    u16 attr)
{
    u32 i;
    BOOL enabled;
    u32* top;

    if (!(OS_PRIORITY_MIN <= priority && priority <= OS_PRIORITY_MAX)) {
        return FALSE;
    }

    thread->state = OS_THREAD_STATE_READY;
    thread->attr = attr & OS_THREAD_ATTR_DETACH;
    thread->priority = thread->base = priority;
    thread->suspend = 1;
    thread->val = (void*)0xFFFFFFFF;
    thread->mutex = NULL;
    thread->queueJoin.head = thread->queueJoin.tail = NULL;
    thread->queueMutex.head = thread->queueMutex.tail = NULL;

    top = (u32*)((u32)stack & ~7);
    top[-2] = 0;
    top[-1] = 0;
    OSInitContext(&thread->context, (u32)entry, (u32)top - 8);
    thread->context.lr = (u32)OSExitThread;
    thread->context.gpr[3] = (u32)param;

    thread->stackBase = stack;
    thread->stackEnd = (u8*)stack - stackSize;
    *(u32*)thread->stackEnd = OS_THREAD_STACK_MAGIC;

    thread->error = 0;
    thread->specific[0] = NULL;
    thread->specific[1] = NULL;

    enabled = OSDisableInterrupts();
    if (__OSErrorTable[16]) {
        thread->context.srr1 |= 0x900;
        thread->context.state |= 1;
        thread->context.fpscr = (__OSFpscrEnableBits & 0xF8) | 4;
        for (i = 0; i < 32; i++) {
            *(u64*)&thread->context.fpr[i] = 0xFFFFFFFFFFFFFFFF;
            *(u64*)&thread->context.psf[i] = 0xFFFFFFFFFFFFFFFF;
        }
    }
    AddTail(&OS_ACTIVE_THREAD_QUEUE, thread, linkActive);
    OSRestoreInterrupts(enabled);
    return TRUE;
}

/* Ends the calling thread: releases its mutexes, wakes its joiners and switches away. */
/* untyped: caller-owned payload */
void OSExitThread(void* exitValue)
{
    OSThread* currentThread;
    BOOL enabled;

    enabled = OSDisableInterrupts();
    currentThread = OS_CURRENT_THREAD;
    OSClearContext(&currentThread->context);
    if (currentThread->attr & OS_THREAD_ATTR_DETACH) {
        RemoveItem(&OS_ACTIVE_THREAD_QUEUE, currentThread, linkActive);
        currentThread->state = 0;
    } else {
        currentThread->state = OS_THREAD_STATE_MORIBUND;
        currentThread->val = exitValue;
    }
    __OSUnlockAllMutex(currentThread);
    OSWakeupThread(&currentThread->queueJoin);
    RunQueueHint = TRUE;
    RescheduleIfHinted();
    OSRestoreInterrupts(enabled);
}

/* Terminates `thread` in whatever state it is in, wakes its joiners and cleans up its mutexes. */
void OSCancelThread(OSThread* thread)
{
    BOOL enabled = OSDisableInterrupts();

    __OSCancelInternalAlarms((u32)thread);
    switch (thread->state) {
    case OS_THREAD_STATE_READY:
        if (thread->suspend <= 0) {
            UnsetRun(thread);
        }
        break;
    case OS_THREAD_STATE_RUNNING:
        RunQueueHint = TRUE;
        break;
    case OS_THREAD_STATE_WAITING:
        RemoveItem(thread->queue, thread, link);
        thread->queue = NULL;
        if (thread->suspend <= 0 && thread->mutex) {
            UpdatePriority(thread->mutex->thread);
        }
        break;
    default:
        OSRestoreInterrupts(enabled);
        return;
    }
    OSClearContext(&thread->context);
    if (thread->attr & OS_THREAD_ATTR_DETACH) {
        RemoveItem(&OS_ACTIVE_THREAD_QUEUE, thread, linkActive);
        thread->state = 0;
    } else {
        thread->state = OS_THREAD_STATE_MORIBUND;
    }
    __OSUnlockAllMutex(thread);
    OSWakeupThread(&thread->queueJoin);
    RescheduleIfHinted();
    OSRestoreInterrupts(enabled);
}

/* Waits for `thread` to exit and fetches its exit value; fails when it is detached or no longer exists. */
BOOL OSJoinThread(OSThread* thread, s32* exitValue)
{
    BOOL enabled = OSDisableInterrupts();

    if (!(thread->attr & OS_THREAD_ATTR_DETACH) && thread->state != OS_THREAD_STATE_MORIBUND
        && thread->queueJoin.head == NULL) {
        OSSleepThread(&thread->queueJoin);
        if (!IsThreadActive(thread)) {
            OSRestoreInterrupts(enabled);
            return FALSE;
        }
    }
    if (thread->state == OS_THREAD_STATE_MORIBUND) {
        if (exitValue) {
            *exitValue = (s32)thread->val;
        }
        RemoveItem(&OS_ACTIVE_THREAD_QUEUE, thread, linkActive);
        thread->state = 0;
        OSRestoreInterrupts(enabled);
        return TRUE;
    }
    OSRestoreInterrupts(enabled);
    return FALSE;
}

/* Lowers the suspend count; when it reaches zero the thread becomes runnable again. */
s32 OSResumeThread(OSThread* thread)
{
    BOOL enabled = OSDisableInterrupts();
    s32 suspendCount = thread->suspend;

    thread->suspend--;
    if (thread->suspend < 0) {
        thread->suspend = 0;
    } else if (thread->suspend == 0) {
        switch (thread->state) {
        case OS_THREAD_STATE_READY:
            thread->priority = __OSGetEffectivePriority(thread);
            SetRun(thread);
            break;
        case OS_THREAD_STATE_WAITING:
            RemoveItem(thread->queue, thread, link);
            thread->priority = __OSGetEffectivePriority(thread);
            AddPrio(thread->queue, thread, link);
            if (thread->mutex) {
                UpdatePriority(thread->mutex->thread);
            }
            break;
        }
        RescheduleIfHinted();
    }
    OSRestoreInterrupts(enabled);
    return suspendCount;
}

/* Raises the suspend count; the first suspension takes the thread out of scheduling. */
s32 OSSuspendThread(OSThread* thread)
{
    BOOL enabled = OSDisableInterrupts();
    s32 suspendCount = thread->suspend;

    thread->suspend++;
    if (suspendCount == 0) {
        switch (thread->state) {
        case OS_THREAD_STATE_RUNNING:
            RunQueueHint = TRUE;
            thread->state = OS_THREAD_STATE_READY;
            break;
        case OS_THREAD_STATE_READY:
            UnsetRun(thread);
            break;
        case OS_THREAD_STATE_WAITING:
            RemoveItem(thread->queue, thread, link);
            thread->priority = OS_PRIORITY_IDLE;
            AddTail(thread->queue, thread, link);
            if (thread->mutex) {
                UpdatePriority(thread->mutex->thread);
            }
            break;
        }
        RescheduleIfHinted();
    }
    OSRestoreInterrupts(enabled);
    return suspendCount;
}

/* Puts the calling thread to sleep on `queue`, ordered by priority, and switches away. */
void OSSleepThread(OSThreadQueue* queue)
{
    OSThread* currentThread;
    BOOL enabled = OSDisableInterrupts();

    currentThread = OS_CURRENT_THREAD;
    currentThread->state = OS_THREAD_STATE_WAITING;
    currentThread->queue = queue;
    AddPrio(queue, currentThread, link);
    RunQueueHint = TRUE;
    RescheduleIfHinted();
    OSRestoreInterrupts(enabled);
}

/* Makes every thread sleeping on `queue` runnable. */
void OSWakeupThread(OSThreadQueue* queue)
{
    OSThread* thread;
    OSThread* next;
    BOOL enabled;

    enabled = OSDisableInterrupts();
    while (queue->head != NULL) {
        thread = queue->head;
        next = thread->link.next;
        if (next == NULL) {
            queue->tail = NULL;
        } else {
            next->link.prev = NULL;
        }
        queue->head = next;
        thread->state = OS_THREAD_STATE_READY;
        if (thread->suspend <= 0) {
            SetRun(thread);
        }
    }
    RescheduleIfHinted();
    OSRestoreInterrupts(enabled);
}

/* The alarm callback of `OSSleepTicks`: resumes the sleeping thread when it is still alive. */
static void SleepAlarmHandler(OSAlarm* alarm, OSContext* context)
{
    if (IsThreadActive((OSThread*)OSGetAlarmUserData(alarm))) {
        OSResumeThread((OSThread*)OSGetAlarmUserData(alarm));
    }
}

/* Suspends the calling thread for `ticks` time-base ticks. */
void OSSleepTicks(u64 ticks)
{
    OSThread* currentThread;
    BOOL enabled;
    OSAlarm alarm;

    enabled = OSDisableInterrupts();
    currentThread = OS_CURRENT_THREAD;

    if (currentThread == NULL) {
        OSRestoreInterrupts(enabled);
        return;
    }
    OSCreateAlarm(&alarm);
    cPhs_Set(&alarm, currentThread);
    OSSetAlarm(&alarm, ticks, SleepAlarmHandler);
    OSSuspendThread(currentThread);
    OSCancelAlarm(&alarm);
    OSRestoreInterrupts(enabled);
}
