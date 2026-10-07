/*
 * TRK/nubevent.h - the MetroTRK nub event record and queue entry points, owned by `TRK/nubevent.c`.
 */
#ifndef TRK_NUBEVENT_H
#define TRK_NUBEVENT_H

#include "types.h"

/* The kinds of event the nub main loop handles. */
typedef enum TRKEventType {
    TRK_EVENT_NONE = 0,
    TRK_EVENT_SHUTDOWN = 1,        /* leaves the main loop */
    TRK_EVENT_REQUEST = 2,         /* a debugger message waits in the buffer named by buffer_id */
    TRK_EVENT_BREAKPOINT = 3,      /* target interrupt */
    TRK_EVENT_EXCEPTION = 4,       /* target interrupt */
    TRK_EVENT_SUPPORT_REQUEST = 5  /* a file or console request from the program */
} TRKEventType;

typedef struct TRKEvent {
    /* +0x00 */ s32 type;           /* TRKEventType */
    /* +0x04 */ u32 serial;         /* sequence number stamped by TRKPostEvent */
    /* +0x08 */ s32 buffer_id;      /* message buffer owned by the event, -1 when none */
} TRKEvent; /* size: 0xC */

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8046940C (0x90): sends the stop (0x90) or exception notification to the debugger; returns 0 or an error code. */
s32 TRKDoNotifyStopped(s32 command);

/* 0x8046949C (0x24): empties the event queue; returns 0. */
s32 TRKInitializeEventQueue(void);

/* 0x804694C0 (0x8C): pops the oldest event into `event`; returns whether one was waiting. */
s32 TRKGetNextEvent(TRKEvent* event);

/* 0x8046954C (0xCC): queues a copy of `event`; returns 0 or 0x100 when the queue is full. */
s32 TRKPostEvent(const TRKEvent* event);

/* 0x80469618 (0x18): fills `event` with the given type, a zero serial and no buffer. */
void TRKConstructEvent(TRKEvent* event, s32 type);

/* 0x80469630 (0x8): releases the buffer the event owns. */
void TRKDestructEvent(TRKEvent* event);

#ifdef __cplusplus
}
#endif

#endif
