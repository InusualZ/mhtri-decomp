/*
 * DVD/dvdqueue.h - declarations of the symbols owned by `DVD/dvdqueue.c` that other DVD units call.
 */
#ifndef DVD_DVDQUEUE_H
#define DVD_DVDQUEUE_H

#include "types.h"
#include "DVD/dvd.h"

#ifdef __cplusplus
extern "C" {
#endif

void __DVDClearWaitingQueue(void);
BOOL __DVDPushWaitingQueue(s32 prio, DVDCommandBlock* block);
DVDCommandBlock* __DVDPopWaitingQueue(void);
BOOL __DVDCheckWaitingQueue(void);
DVDCommandBlock* __DVDGetNextWaitingQueue(void);
BOOL __DVDDequeueWaitingQueue(DVDCommandBlock* block);

#ifdef __cplusplus
}
#endif

#endif /* DVD_DVDQUEUE_H */
