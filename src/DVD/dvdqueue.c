/*
 * DVD/dvdqueue.c - the SDK DVD waiting queue (clear, push, pop, check, dequeue).
 *
 * RANGE. `.text` 0x804AB040-0x804AB2C0 (6 functions / 0x25C B); `.bss` 0x807466F0-0x80746720.
 *   - six functions whose only data is `WaitingQueue` (.bss 0x807466F0); the neighbours on both sides read disjoint
 *     data (clean seam at both edges)
 * FLAGS. the `OS` lib block of `configure.py` (`Wii/1.3`) with `cflags_base` (-O4,p, default alignment 16).
 * NAMES. the unit name is a GUESS (the queue helpers are named in the SDK queue scheme); `__DVDGetNextWaitingQueue` is
 *   a GUESS for the map's former placeholder (it returns the head of the first non-empty priority queue).
 * RESIDUALS. none recorded yet.
 */

#include "types.h"
#include "DVD/dvdqueue.h"
#include "OS/OSInterrupt.h"

typedef struct DVDWaitingQueue {
    /* +0x00 */ DVDCommandBlock* head;
    /* +0x04 */ DVDCommandBlock* tail;
} DVDWaitingQueue; /* size: 0x8 */

/* The four priority queues; each entry doubles as the sentinel of its circular list (head / tail overlay the
 * `next` / `prev` links of a command block).  The claimed `.bss` carries 0x10 more bytes that nothing in the DOL
 * reads; the array is sized to cover them because the linker drops a separate unreferenced variable. */
static DVDWaitingQueue WaitingQueue[4 + 2];

/* Empties every priority queue. */
void __DVDClearWaitingQueue(void)
{
    u32 i;
    DVDCommandBlock* q;

    for (i = 0; i < 4; i++) {
        q = (DVDCommandBlock*)&WaitingQueue[i];
        q->next = q;
        q->prev = q;
    }
}

/* Appends a command block to the queue of its priority. */
BOOL __DVDPushWaitingQueue(s32 prio, DVDCommandBlock* block)
{
    BOOL enabled;
    DVDCommandBlock* q;

    enabled = OSDisableInterrupts();
    q = (DVDCommandBlock*)&WaitingQueue[prio];
    q->prev->next = block;
    block->prev = q->prev;
    block->next = q;
    q->prev = block;
    OSRestoreInterrupts(enabled);
    return TRUE;
}

/* Removes and returns the first block of the highest-priority non-empty queue. */
DVDCommandBlock* __DVDPopWaitingQueue(void)
{
    BOOL enabled;
    u32 i;
    DVDCommandBlock* q;
    DVDCommandBlock* tmp;

    enabled = OSDisableInterrupts();
    for (i = 0; i < 4; i++) {
        if (WaitingQueue[i].head != (DVDCommandBlock*)&WaitingQueue[i]) {
            OSRestoreInterrupts(enabled);
            enabled = OSDisableInterrupts();
            q = (DVDCommandBlock*)&WaitingQueue[i];
            tmp = q->next;
            q->next = tmp->next;
            tmp->next->prev = q;
            OSRestoreInterrupts(enabled);
            tmp->next = NULL;
            tmp->prev = NULL;
            return tmp;
        }
    }
    OSRestoreInterrupts(enabled);
    return NULL;
}

/* Whether any priority queue holds a block. */
BOOL __DVDCheckWaitingQueue(void)
{
    BOOL enabled;
    u32 i;

    enabled = OSDisableInterrupts();
    for (i = 0; i < 4; i++) {
        if (WaitingQueue[i].head != (DVDCommandBlock*)&WaitingQueue[i]) {
            OSRestoreInterrupts(enabled);
            return TRUE;
        }
    }
    OSRestoreInterrupts(enabled);
    return FALSE;
}

/* Returns, without removing it, the first block of the highest-priority non-empty queue. */
DVDCommandBlock* __DVDGetNextWaitingQueue(void)
{
    BOOL enabled;
    u32 i;
    DVDCommandBlock* q;

    enabled = OSDisableInterrupts();
    for (i = 0; i < 4; i++) {
        q = WaitingQueue[i].head;
        if (q != (DVDCommandBlock*)&WaitingQueue[i]) {
            OSRestoreInterrupts(enabled);
            return q;
        }
    }
    OSRestoreInterrupts(enabled);
    return NULL;
}

/* Unlinks a queued block; fails when the block is not queued. */
BOOL __DVDDequeueWaitingQueue(DVDCommandBlock* block)
{
    BOOL enabled;
    DVDCommandBlock* prev;
    DVDCommandBlock* next;

    enabled = OSDisableInterrupts();
    prev = block->prev;
    next = block->next;
    if (prev == NULL || next == NULL) {
        OSRestoreInterrupts(enabled);
        return FALSE;
    }
    prev->next = next;
    next->prev = prev;
    OSRestoreInterrupts(enabled);
    return TRUE;
}
