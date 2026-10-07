/*
 * OS/OSMutex.c - the OS mutex: init, lock, unlock, `__OSUnlockAllMutex` and the two thread-queue branch thunks that
 *    end it.
 * RANGE. .text 0x804D1EE0-0x804D2160 (6 functions).  Cut from the OS core band 0x804C1760-0x804D9B4C.  Evidence: four
 *    functions that call the thread layer; the two 4-byte `b OSInitThreadQueue` / `b OSWakeupThread` thunks at
 *    0x804D2140/0x804D2150 follow them before `__OSReboot`.
 * FLAGS. `cflags_base` per object (configure.py): the target's function starts are all 16-aligned.
 * NAMES. map names throughout; the two thunks keep the map's GUESS names
 *    `OSInitThreadQueueThunk`/`OSWakeupThreadThunk`; their placement at this unit's tail is a GUESS.
 * RESIDUALS. the object's .text ends 12 bytes before the claimed end (alignment padding).
 * SHAPES. the public entries keep the `void*` mutex parameter the other bands declare; the bodies view it as an OSMutex.
 */

#include "types.h"

#define OS_MUTEX_TYPED_API

#include "OS/OSInterrupt.h"
#include "OS/OSMutex.h"
#include "OS/OSThread.h"

/* Unlinks `mutex` from the owner's mutex queue (the list the thread keeps of mutexes it holds). */
#define UNLINK_MUTEX(thread, mutex)                       \
    do {                                                  \
        OSMutex* next = (mutex)->link.next;               \
        OSMutex* prev = (mutex)->link.prev;               \
        if (next == NULL)                                 \
            (thread)->queueMutex.tail = prev;             \
        else                                              \
            next->link.prev = prev;                       \
        if (prev == NULL)                                 \
            (thread)->queueMutex.head = next;             \
        else                                              \
            prev->link.next = next;                       \
    } while (0)

/* Initialises an unlocked mutex. */
void OSInitMutex(OSMutex* m)
{

    OSInitThreadQueue(&m->queue);
    m->thread = NULL;
    m->count = 0;
}

/* Locks a mutex for the calling thread, sleeping while another thread holds it. */
void OSLockMutex(OSMutex* m)
{
    BOOL enabled = OSDisableInterrupts();
    OSThread* currentThread = OSGetCurrentThread();
    OSThread* ownerThread;

    while (TRUE) {
        ownerThread = m->thread;
        if (ownerThread == NULL) {
            OSMutex* tail;

            m->thread = currentThread;
            m->count++;
            tail = currentThread->queueMutex.tail;
            if (tail == NULL)
                currentThread->queueMutex.head = m;
            else
                tail->link.next = m;
            m->link.prev = tail;
            m->link.next = NULL;
            currentThread->queueMutex.tail = m;
            break;
        } else if (ownerThread == currentThread) {
            m->count++;
            break;
        } else {
            currentThread->mutex = m;
            __OSPromoteThread(m->thread, currentThread->priority);
            OSSleepThread(&m->queue);
            currentThread->mutex = NULL;
        }
    }
    OSRestoreInterrupts(enabled);
}

/* Unlocks a mutex held by the calling thread and wakes its waiters once the lock count drops to zero. */
void OSUnlockMutex(OSMutex* m)
{
    BOOL enabled = OSDisableInterrupts();
    OSThread* currentThread = OSGetCurrentThread();

    if (m->thread == currentThread) {
        m->count--;
        if (m->count == 0) {
            UNLINK_MUTEX(currentThread, m);
            m->thread = NULL;
            if (currentThread->priority < currentThread->base)
                currentThread->priority = __OSGetEffectivePriority(currentThread);
            OSWakeupThread(&m->queue);
        }
    }
    OSRestoreInterrupts(enabled);
}

/* Releases every mutex a terminating thread still holds. */
void __OSUnlockAllMutex(OSThread* thread)
{
    OSMutex* m;
    OSMutex* next;

    while (thread->queueMutex.head != NULL) {
        m = thread->queueMutex.head;
        next = m->link.next;
        if (next == NULL)
            thread->queueMutex.tail = NULL;
        else
            next->link.prev = NULL;
        thread->queueMutex.head = next;
        m->count = 0;
        m->thread = NULL;
        OSWakeupThread(&m->queue);
    }
}

/* Branches to `OSInitThreadQueue`. */
void OSInitThreadQueueThunk(OSThreadQueue* queue)
{
    OSInitThreadQueue(queue);
}

/* Branches to `OSWakeupThread`. */
void OSWakeupThreadThunk(OSThreadQueue* queue)
{
    OSWakeupThread(queue);
}
