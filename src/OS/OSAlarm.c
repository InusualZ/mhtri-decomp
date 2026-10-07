/*
 * OS/OSAlarm.c - the OS alarm queue: alarm create/set/cancel, the decrementer exception handler and the shutdown
 *    cancel hook.
 * RANGE. .text 0x804CB440-0x804CBD10 (13 functions); .data 0x8061C0C8-0x8061C0D8; .sbss 0x80795310-0x80795318.  Cut
 *    from the OS core band 0x804C1760-0x804D9B4C.  Evidence: `AlarmQueue` (.sbss 0x80795310, a `scope:local`
 *    static) is read by `__OSInitAlarm`, `InsertAlarm`, `OSCancelAlarm`, the decrementer callback, the shutdown
 *    hook and `__OSCancelInternalAlarms`, and by nothing outside the range; its `ShutdownFunctionInfo` (.data
 *    0x8061C0C8) is address-taken by `__OSInitAlarm`.
 * FLAGS. `cflags_base` per object (configure.py): the target's function starts are all 16-aligned.
 * NAMES. map names throughout (GUESS: `__OSInitAlarm`, which the dump has as a placeholder); `cPhs_Set` (0x804CBC50) is the map's name for a 16-byte alarm accessor (it stores the
 *    user data at +0x28 and -1 at +0x04) and is kept; GUESS: `OSAlarmShutdownFunction` (the shutdown hook that cancels every queued alarm but the DVD driver's), `InsertAlarm`/`DecrementerExceptionCallback`/`DecrementerExceptionHandler` are the map's names.
 * RESIDUALS. `InsertAlarm` (98.5 %): the saved registers are permuted (alarm r28/handler r29 in ours, r30/r31 in the target); no
 *    declaration order or temporary placement changed it.  The object's .text ends 8 bytes before the claimed end (alignment fill).
 * SHAPES. the timer reprogramming (`SetTimer`) is an inline helper expanded in four bodies; the decrementer handler is an
 *    asm function with `nofralloc` that saves the interrupted registers and branches to the callback.
 */

#include "types.h"

#include "DVD/__DVDTestAlarm.h"
#include "OS/OS.h"
#include "OS/OSAlarm.h"
#include "OS/OSContext.h"
#include "OS/OSInterrupt.h"
#include "OS/OSReset.h"
#include "OS/OSSetPeriodicAlarm.h"
#include "OS/OSSetAlarm.h"
#include "OS/OSThread.h"
#include "OS/OSTime.h"
#include "OS/PPCMtdec.h"
#include "OS/__OSGetSystemTime.h"

/* size: 0x08 - the queue of armed alarms, ordered by fire time */
typedef struct OSAlarmQueue {
    /* +0x00 */ OSAlarm* head;
    /* +0x04 */ OSAlarm* tail;
} OSAlarmQueue;

void DecrementerExceptionHandler(u8 exception, OSContext* context);
BOOL OSAlarmShutdownFunction(BOOL final, u32 event);

static OSShutdownFunctionInfo ShutdownFunctionInfo = {OSAlarmShutdownFunction, 0xFFFFFFFF, NULL, NULL};

static OSAlarmQueue AlarmQueue;

/* Reprograms the decrementer so it expires when `alarm` is due. */
static inline void SetTimer(OSAlarm* alarm)
{
    s64 delta = alarm->fire - __OSGetSystemTime();

    if (delta < 0) {
        PPCMtdec(0);
    } else if (delta < 0x80000000) {
        PPCMtdec((u32)delta);
    } else {
        PPCMtdec(0x7FFFFFFF);
    }
}

/* Hooks the decrementer exception to the alarm queue and registers the shutdown hook. */
void __OSInitAlarm(void)
{
    if (__OSGetExceptionHandler(8) != DecrementerExceptionHandler) {
        AlarmQueue.head = AlarmQueue.tail = NULL;
        __OSSetExceptionHandler(8, DecrementerExceptionHandler);
        OSRegisterShutdownFunction(&ShutdownFunctionInfo);
    }
}

/* Clears the alarm's handler and tag. */
void OSCreateAlarm(OSAlarm* alarm)
{
    alarm->handler = NULL;
    alarm->tag = 0;
}

/* Puts `alarm` into the fire-time ordered queue and reprograms the timer when it becomes the head. */
static void InsertAlarm(OSAlarm* alarm, s64 fire, OSAlarmHandler handler)
{
    OSAlarm* next;
    OSAlarm* prev;

    if (alarm->period > 0) {
        s64 time = __OSGetSystemTime();

        fire = alarm->start;
        if (alarm->start < time) {
            fire = alarm->start + alarm->period * (1 + (time - alarm->start) / alarm->period);
        }
    }

    alarm->handler = handler;
    alarm->fire = fire;

    for (next = AlarmQueue.head; next != NULL; next = next->next) {
        if (fire < next->fire) {
            alarm->prev = next->prev;
            next->prev = alarm;
            alarm->next = next;
            prev = alarm->prev;
            if (prev != NULL) {
                prev->next = alarm;
            } else {
                AlarmQueue.head = alarm;
                SetTimer(alarm);
            }
            return;
        }
    }

    alarm->next = NULL;
    prev = AlarmQueue.tail;
    AlarmQueue.tail = alarm;
    alarm->prev = prev;
    if (prev != NULL) {
        prev->next = alarm;
    } else {
        AlarmQueue.tail = alarm;
        AlarmQueue.head = alarm;
        SetTimer(alarm);
    }
}

/* Arms `alarm` to run `handler` after `tick` ticks. */
void OSSetAlarm(OSAlarm* alarm, s64 tick, OSAlarmHandler handler)
{
    BOOL enabled = OSDisableInterrupts();

    alarm->period = 0;
    InsertAlarm(alarm, __OSGetSystemTime() + tick, handler);
    OSRestoreInterrupts(enabled);
}

/* Arms `alarm` to run `handler` every `period` ticks, first at `start`. */
void OSSetPeriodicAlarm(OSAlarm* alarm, s64 start, s64 period, OSAlarmHandler handler)
{
    BOOL enabled = OSDisableInterrupts();

    alarm->period = period;
    alarm->start = __OSTimeToSystemTime(start);
    InsertAlarm(alarm, 0, handler);
    OSRestoreInterrupts(enabled);
}

/* Removes `alarm` from the queue and reprograms the timer for the new head. */
void OSCancelAlarm(OSAlarm* alarm)
{
    BOOL enabled = OSDisableInterrupts();
    OSAlarm* next;

    if (alarm->handler == NULL) {
        OSRestoreInterrupts(enabled);
        return;
    }

    next = alarm->next;
    if (next == NULL) {
        AlarmQueue.tail = alarm->prev;
    } else {
        next->prev = alarm->prev;
    }
    if (alarm->prev != NULL) {
        alarm->prev->next = next;
    } else {
        AlarmQueue.head = next;
        if (next != NULL) {
            SetTimer(next);
        }
    }
    alarm->handler = NULL;
    OSRestoreInterrupts(enabled);
}

/* Runs the due alarm's handler on a scratch context, then resumes the interrupted context. */
static void DecrementerExceptionCallback(u8 exception, OSContext* context)
{
    OSContext exceptionContext;
    OSAlarm* alarm;
    OSAlarmHandler handler;
    s64 time = __OSGetSystemTime();

    alarm = AlarmQueue.head;
    if (alarm == NULL) {
        OSLoadContext(context);
    }
    if (time < alarm->fire) {
        SetTimer(alarm);
        OSLoadContext(context);
    }

    AlarmQueue.head = alarm->next;
    if (AlarmQueue.head == NULL) {
        AlarmQueue.tail = NULL;
    } else {
        AlarmQueue.head->prev = NULL;
    }

    handler = alarm->handler;
    alarm->handler = NULL;
    if (alarm->period > 0) {
        InsertAlarm(alarm, 0, handler);
    }
    if (AlarmQueue.head != NULL) {
        SetTimer(AlarmQueue.head);
    }

    OSDisableScheduler();
    OSClearContext(&exceptionContext);
    OSSetCurrentContext(&exceptionContext);
    handler(alarm, context);
    OSClearContext(&exceptionContext);
    OSSetCurrentContext(context);
    OSEnableScheduler();
    __OSReschedule();
    OSLoadContext(context);
}

/* Saves the interrupted registers into `context` and branches to the alarm callback on a fresh frame. */
asm void DecrementerExceptionHandler(u8 exception, OSContext* context)
{
    nofralloc
    stw r0, 0(r4)
    stw r1, 4(r4)
    stw r2, 8(r4)
    stmw r6, 24(r4)
    mfspr r0, GQR1
    stw r0, 0x1A8(r4)
    mfspr r0, GQR2
    stw r0, 0x1AC(r4)
    mfspr r0, GQR3
    stw r0, 0x1B0(r4)
    mfspr r0, GQR4
    stw r0, 0x1B4(r4)
    mfspr r0, GQR5
    stw r0, 0x1B8(r4)
    mfspr r0, GQR6
    stw r0, 0x1BC(r4)
    mfspr r0, GQR7
    stw r0, 0x1C0(r4)
    stwu r1, -8(r1)
    b DecrementerExceptionCallback
}

/* Cancels every OS-owned alarm when the system shuts down (before the DVD drive is stopped). */
BOOL OSAlarmShutdownFunction(BOOL final, u32 event)
{
    OSAlarm* alarm;
    OSAlarm* next;

    if (final) {
        alarm = AlarmQueue.head;
        next = alarm != NULL ? alarm->next : NULL;
        while (alarm != NULL) {
            if (!__DVDTestAlarm(alarm)) {
                OSCancelAlarm(alarm);
            }
            alarm = next;
            next = alarm != NULL ? alarm->next : NULL;
        }
    }
    return TRUE;
}

/* Stores the alarm's user data. */
/* untyped: caller-owned payload */
void OSSetAlarmUserData(OSAlarm* alarm, void* userData)
{
    alarm->userData = userData;
}

/* Returns the alarm's user data. */
/* untyped: caller-owned payload */
void* OSGetAlarmUserData(OSAlarm* alarm)
{
    return alarm->userData;
}

/* 0x804CBC50 (0x10): stores `userData` at +0x28 and 0xFFFFFFFF at +0x04 (the tag). */
/* untyped: caller-owned payload */
void cPhs_Set(OSAlarm* alarm, void* userData)
{
    alarm->userData = userData;
    alarm->tag = -1;
}

/* Cancels every OS-owned alarm (tag 0xFFFFFFFF) whose user data is `userData`. */
void __OSCancelInternalAlarms(u32 userData)
{
    BOOL enabled = OSDisableInterrupts();
    OSAlarm* alarm = AlarmQueue.head;
    OSAlarm* next = alarm != NULL ? alarm->next : NULL;

    while (alarm != NULL) {
        if (alarm->tag == 0xFFFFFFFF && (u32)alarm->userData == userData) {
            OSCancelAlarm(alarm);
        }
        alarm = next;
        next = alarm != NULL ? alarm->next : NULL;
    }
    OSRestoreInterrupts(enabled);
}
