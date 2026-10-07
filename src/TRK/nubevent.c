/*
 * TRK/nubevent.c - the MetroTRK notification and event queue: `TRKDoNotifyStopped`, the event queue (init, get,
 *    post, construct, destruct).
 *
 * RANGE. .text 0x8046940C..0x80469638 (6 functions in the map, 0x22C B); .data 0x8060F6D0..0x8060F6F0; .bss
 *    0x806F5568..0x806F5590.
 * FLAGS. the `OS` lib's `cflags_os`.
 * NAMES. file name GUESS (MetroTRK `nubevent`); `TRKDoNotifyStopped` may be a separate `notify` file. The queue
 *    record and its fields (`TRKEventQueue`, `count`, `head`, `next_serial`), `trk_event_queue` and
 *    `trk_event_queue_full_format` are GUESS from the indices the three queue functions use and the string they print.
 * EVIDENCE. `.bss` 0x806F5568 (the 0x28 B queue) is read only by the three queue functions and `.data` 0x8060F6D0
 *    (`Event Queue full`) only by `TRKPostEvent`.
 * RESIDUALS. none known.
 * SHAPES. a two-slot ring: the tail index is (head + count) % 2, serials start at 256.
 */
#include "TRK/nubevent.h"
#include "TRK/mem_TRK.h"
#include "TRK/msgbuf.h"
#include "TRK/support.h"
#include "TRK/targimpl.h"
#include "TRK/trk_report.h"

#define TRK_EVENT_QUEUE_SLOTS 2
#define TRK_EVENT_SERIAL_BASE 256
#define TRK_NOTIFY_STOPPED 0x90
#define TRK_ERR_EVENT_QUEUE_FULL 0x100

typedef struct TRKEventQueue {
    /* +0x00 */ s32 count;                                  /* events waiting */
    /* +0x04 */ s32 head;                                   /* slot of the oldest event */
    /* +0x08 */ TRKEvent events[TRK_EVENT_QUEUE_SLOTS];     /* the ring */
    /* +0x20 */ u32 next_serial;                            /* serial given to the next posted event */
    /* +0x24 */ u32 pad_0x24;                               /* padding to the object size */
} TRKEventQueue; /* size: 0x28 */

static TRKEventQueue trk_event_queue;

s32 TRKDoNotifyStopped(s32 command)
{
    s32 reply_id;
    s32 buffer_id;
    TRKBuffer* buffer;
    s32 err;

    err = TRK_GetFreeBuffer(&buffer_id, &buffer);
    if (err == 0) {
        if (command == TRK_NOTIFY_STOPPED) {
            TRKTargetAddStopInfo(buffer);
        } else {
            TRKTargetAddExceptionInfo(buffer);
        }
        err = TRK_RequestSend(buffer, &reply_id);
        if (err == 0) {
            TRK_ReleaseBuffer(reply_id);
        }
        TRK_ReleaseBuffer(buffer_id);
    }
    return err;
}

s32 TRKInitializeEventQueue(void)
{
    trk_event_queue.count = 0;
    trk_event_queue.head = 0;
    trk_event_queue.next_serial = TRK_EVENT_SERIAL_BASE;
    return 0;
}

s32 TRKGetNextEvent(TRKEvent* event)
{
    s32 found = 0;

    if (trk_event_queue.count > 0) {
        TRK_memcpy(event, &trk_event_queue.events[trk_event_queue.head], sizeof(TRKEvent));
        trk_event_queue.count--;
        trk_event_queue.head++;
        if (trk_event_queue.head == TRK_EVENT_QUEUE_SLOTS) {
            trk_event_queue.head = 0;
        }
        found = 1;
    }
    return found;
}

s32 TRKPostEvent(const TRKEvent* event)
{
    s32 err = 0;

    if (trk_event_queue.count == TRK_EVENT_QUEUE_SLOTS) {
        err = TRK_ERR_EVENT_QUEUE_FULL;
        TRK_REPORT("MetroTRK - Event Queue full\n");
    } else {
        s32 slot = (trk_event_queue.head + trk_event_queue.count) % TRK_EVENT_QUEUE_SLOTS;

        TRK_memcpy(&trk_event_queue.events[slot], event, sizeof(TRKEvent));
        trk_event_queue.events[slot].serial = trk_event_queue.next_serial;
        trk_event_queue.next_serial++;
        if (trk_event_queue.next_serial < TRK_EVENT_SERIAL_BASE) {
            trk_event_queue.next_serial = TRK_EVENT_SERIAL_BASE;
        }
        trk_event_queue.count++;
    }
    return err;
}

void TRKConstructEvent(TRKEvent* event, s32 type)
{
    event->type = type;
    event->serial = 0;
    event->buffer_id = -1;
}

void TRKDestructEvent(TRKEvent* event)
{
    TRK_ReleaseBuffer(event->buffer_id);
}
