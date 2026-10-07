/*
 * OS/OSMessage.h - declarations of the symbols owned by `OS/OSMessage.c` that other units call or read.
 */
#ifndef OS_OSMESSAGE_H
#define OS_OSMESSAGE_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* size: 0x20 - two thread queues, the message-array pointer, the count and the two ring indices */
typedef struct OSMessageQueue {
    /* +0x00 */ u8 pad_0x00[0x20];
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
BOOL OSSendMessage(OSMessageQueue* queue, OSMessage* msg, BOOL block);
BOOL OSReceiveMessage(OSMessageQueue* queue, OSMessage* buffer, BOOL block);

#ifdef __cplusplus
}
#endif

#endif
