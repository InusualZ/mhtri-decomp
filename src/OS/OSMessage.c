/*
 * OS/OSMessage.c - the OS message queue: init, send, receive and jam.
 * RANGE. .text 0x804D1460-0x804D1740 (4 functions).  Cut from the OS core band 0x804C1760-0x804D9B4C.  Evidence: four
 *    functions that call the thread sleep/wakeup pair (0x804D4A30, 0x804D4B20) and the interrupt pair; no data.
 * FLAGS. `cflags_base` per object (configure.py): the target's function starts are all 16-aligned.
 * NAMES. map names throughout; `OSWakeupThread` is the dump's name of 0x804D4B20.
 * RESIDUALS. the object's .text ends 12 bytes before the claimed end (alignment padding).
 * SHAPES. the queue stores message words; the header's `OSMessage` (8 bytes) is the NHTTP band's own view of an element.
 */

#include "types.h"

#include "OS/OSInterrupt.h"
#include "OS/OSMessage.h"
#include "OS/OSThread.h"

/* Initialises an empty queue over a caller-owned ring of message words. */
void OSInitMessageQueue(OSMessageQueue* mq, OSMessage* msgArray, s32 msgCount)
{
    OSInitThreadQueue(&mq->queueSend);
    OSInitThreadQueue(&mq->queueReceive);
    mq->msgArray = (void**)msgArray;
    mq->msgCount = msgCount;
    mq->firstIndex = 0;
    mq->usedCount = 0;
}

/* Appends a message, sleeping for room when `flags` asks to block; returns FALSE when it would block and must not. */
/* untyped: the message word is passed through to the receiver */
BOOL OSSendMessage(OSMessageQueue* mq, void* msg, BOOL flags)
{
    BOOL enabled;
    s32 lastIndex;

    enabled = OSDisableInterrupts();
    while (mq->msgCount <= mq->usedCount) {
        if (!(flags & OS_MESSAGE_BLOCK)) {
            OSRestoreInterrupts(enabled);
            return FALSE;
        }
        OSSleepThread(&mq->queueSend);
    }
    lastIndex = (mq->firstIndex + mq->usedCount) % mq->msgCount;
    mq->msgArray[lastIndex] = msg;
    mq->usedCount++;
    OSWakeupThread(&mq->queueReceive);
    OSRestoreInterrupts(enabled);
    return TRUE;
}

/* Takes the oldest message into `msg` (when not NULL), sleeping for one when `flags` asks to block. */
BOOL OSReceiveMessage(OSMessageQueue* mq, OSMessage* msg, BOOL flags)
{
    BOOL enabled;

    enabled = OSDisableInterrupts();
    while (mq->usedCount == 0) {
        if (!(flags & OS_MESSAGE_BLOCK)) {
            OSRestoreInterrupts(enabled);
            return FALSE;
        }
        OSSleepThread(&mq->queueReceive);
    }
    if (msg != NULL)
        *(void**)msg = mq->msgArray[mq->firstIndex];
    mq->firstIndex = (mq->firstIndex + 1) % mq->msgCount;
    mq->usedCount--;
    OSWakeupThread(&mq->queueSend);
    OSRestoreInterrupts(enabled);
    return TRUE;
}

/* Inserts a message at the front of the queue, sleeping for room when `flags` asks to block. */
/* untyped: the message word is passed through to the receiver */
BOOL OSJamMessage(OSMessageQueue* mq, void* msg, BOOL flags)
{
    BOOL enabled;

    enabled = OSDisableInterrupts();
    while (mq->msgCount <= mq->usedCount) {
        if (!(flags & OS_MESSAGE_BLOCK)) {
            OSRestoreInterrupts(enabled);
            return FALSE;
        }
        OSSleepThread(&mq->queueSend);
    }
    mq->firstIndex = (mq->firstIndex + mq->msgCount - 1) % mq->msgCount;
    mq->msgArray[mq->firstIndex] = msg;
    mq->usedCount++;
    OSWakeupThread(&mq->queueReceive);
    OSRestoreInterrupts(enabled);
    return TRUE;
}
