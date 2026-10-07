/*
 * OS/OSMessage.h - declarations of the symbols owned by `OS/OSMessage.c` that other units call or read.
 */
#ifndef OS_OSMESSAGE_H
#define OS_OSMESSAGE_H

#include "types.h"
#include "OS/OSThread.h"

#ifdef __cplusplus
extern "C" {
#endif

/* size: 0x20 - two thread queues, the message-array pointer, the count and the two ring indices */
typedef struct OSMessageQueue {
    /* +0x00 */ OSThreadQueue queueSend;    /* threads waiting for room */
    /* +0x08 */ OSThreadQueue queueReceive; /* threads waiting for a message */
    /* +0x10 */ void** msgArray;            /* the caller-owned ring of message words */
    /* +0x14 */ s32 msgCount;               /* ring capacity */
    /* +0x18 */ s32 firstIndex;             /* index of the oldest message */
    /* +0x1C */ s32 usedCount;              /* messages in the ring */
} OSMessageQueue; /* size: 0x20 */

/* size: 0x08 - the queued message and its priority */
typedef struct OSMessage {
    /* +0x00 */ void* msg;
    /* +0x04 */ s32 prio;
} OSMessage; /* size: 0x08 */

#define OS_MESSAGE_NOBLOCK 0
#define OS_MESSAGE_BLOCK   1

/* 0x804D1460 - initialise a message queue over a caller-owned array. */
void OSInitMessageQueue(OSMessageQueue* queue, OSMessage* msgArray, s32 msgCount);

/* 0x804D14C0 / 0x804D1590 - post to / take from a queue; the block flag is OS_MESSAGE_BLOCK or
 * OS_MESSAGE_NOBLOCK. */
/* untyped: the message word is passed through to the receiver */
BOOL OSSendMessage(OSMessageQueue* queue, void* msg, BOOL block);
BOOL OSReceiveMessage(OSMessageQueue* queue, OSMessage* buffer, BOOL block);

/* 0x804D1670 - posts a message at the front of the queue. */
/* untyped: the message word is passed through to the receiver */
BOOL OSJamMessage(OSMessageQueue* queue, void* msg, BOOL block);

#ifdef __cplusplus
}
#endif

#endif
