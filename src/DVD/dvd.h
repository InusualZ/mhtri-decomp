/*
 * DVD/dvd.h - the DVD library's command block and the entry points of `DVD/dvd.c` that other DVD units call.
 */
#ifndef DVD_DVD_H
#define DVD_DVD_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* One queued drive command.  Only the queue links are named so far; the rest is carried as padding until the
 * `DVD/dvd.c` bodies name it. */
typedef struct DVDCommandBlock {
    /* +0x00 */ struct DVDCommandBlock* next; /* waiting-queue link */
    /* +0x04 */ struct DVDCommandBlock* prev; /* waiting-queue link */
    /* +0x08 */ u8 pad_0x08[0x28];
} DVDCommandBlock; /* size: 0x30 */

/* 0x804AAC70 - the drive's cover state (2 when the cover is closed with a disc). */
u32 __DVDGetCoverStatus(void);

/* 0x804AAEC0 - quiesces the drive ahead of a reset. */
void __DVDPrepareReset(void);

#ifdef __cplusplus
}
#endif

#endif /* DVD_DVD_H */
