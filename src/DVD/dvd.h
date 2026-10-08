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
    /* +0x08 */ u8 pad_0x08[4];
    /* +0x0C */ s32 state;                    /* 0 when the command finished */
    /* +0x10 */ u8 pad_0x10[0x20];
} DVDCommandBlock; /* size: 0x30 */

/* size: 0x20 - the disc header the boot code leaves at 0x80000000 (only the head is named). */
typedef struct DVDDiskID {
    /* +0x00 */ char gameName[4];
    /* +0x04 */ u16 makerCode;
    /* +0x06 */ u8 diskNumber;
    /* +0x07 */ u8 gameVersion;
    /* +0x08 */ u8 pad_0x08[0x18];
} DVDDiskID;

/* size: 0x20 - what the drive reports about itself (only the device code is read). */
typedef struct DVDDriveInfo {
    /* +0x00 */ u16 revisionLevel;
    /* +0x02 */ u16 deviceCode;
    /* +0x04 */ u32 releaseDate;
    /* +0x08 */ u8 pad_0x08[0x18];
} DVDDriveInfo;

/* Called when a queued drive command finishes. */
typedef void (*DVDCBCallback)(s32 result, DVDCommandBlock* block);

/* 0x804A64E0 - initialises the DVD library and the drive. */
void DVDInit(void);

/* 0x804AA560 - queues a drive inquiry that fills `info`; `callback` runs when it finishes. */
s32 DVDInquiryAsync(DVDCommandBlock* block, DVDDriveInfo* info, DVDCBCallback callback);

/* 0x804AA470 - queues an asynchronous read of `length` bytes at `offset` (in 4-byte units) into `addr`. */
/* untyped: byte range the read fills */
s32 DVDReadAbsAsyncPrio(DVDCommandBlock* block, void* addr, s32 length, s32 offset, DVDCBCallback callback, s32 prio);

/* 0x804AA640 - the state of a queued command: 0 when finished, 1 and 2 while it waits or runs, negative on failure. */
s32 DVDGetCommandBlockStatus(const DVDCommandBlock* block);

/* 0x804AA7B0 - turns the cache invalidation after reads on or off. */
void DVDSetAutoInvalidation(BOOL enable);

/* 0x804AA7C0 - resumes the drive queue. */
void DVDResume(void);

/* 0x804AAD90 - quiesces the drive and calls `callback` when it is done. */
void __DVDPrepareResetAsync(void (*callback)(void));

/* 0x807950D8 - the disc layout format: the shift that turns a byte offset into the drive's address unit. */
extern s32 __DVDLayoutFormat;

/* 0x804AAC50 - the disc header at 0x80000000. */
DVDDiskID* DVDGetCurrentDiskID(void);

/* 0x804AAD40 - starts a drive command and waits until its callback reports completion. */
void DVDResetDriveSync(void);

/* 0x804AAC70 - the drive's cover state (2 when the cover is closed with a disc). */
u32 __DVDGetCoverStatus(void);

/* 0x804AAEC0 - quiesces the drive ahead of a reset. */
void __DVDPrepareReset(void);

#ifdef __cplusplus
}
#endif

#endif /* DVD_DVD_H */
