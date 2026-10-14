/*
 * DVD/dvd.h - the DVD library's command block and the entry points of `DVD/dvd.c` that other DVD units call.
 */
#ifndef DVD_DVD_H
#define DVD_DVD_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The identification record at the start of a disc (the boot code leaves it at 0x80000000); a zero first byte or a
 * 0xFF number is a wildcard to `DVDCompareDiskID`. */
typedef struct DVDDiskID {
    /* +0x00 */ char gameName[4];
    /* +0x04 */ union {
        char company[2]; /* compared bytewise */
        u16 makerCode;   /* read as one halfword */
    }; /* size: 0x2 */
    /* +0x06 */ u8 diskNumber;
    /* +0x07 */ u8 gameVersion;
    /* +0x08 */ u8 streaming;
    /* +0x09 */ u8 streamingBufSize;
    /* +0x0A */ u8 pad_0x0A[14];
    /* +0x18 */ u32 rvlMagic;
    /* +0x1C */ u32 gcMagic;
} DVDDiskID; /* size: 0x20 */

struct DVDCommandBlock;

/* Called when a queued drive command finishes. */
typedef void (*DVDCBCallback)(s32 result, struct DVDCommandBlock* block);

/* One queued drive command. */
typedef struct DVDCommandBlock {
    /* +0x00 */ struct DVDCommandBlock* next; /* waiting-queue link */
    /* +0x04 */ struct DVDCommandBlock* prev; /* waiting-queue link */
    /* +0x08 */ u32 command;                  /* the drive command the block carries (read, inquiry, ...) */
    /* +0x0C */ s32 state;                    /* 0 when the command finished */
    /* +0x10 */ u32 offset;                   /* disc offset of a read */
    /* +0x14 */ u32 length;                   /* byte count of a read */
    /* +0x18 */ void* addr;                   /* destination of a read */
    /* +0x1C */ u32 currTransferSize;         /* bytes of the transfer in flight */
    /* +0x20 */ u32 transferredSize;          /* bytes transferred so far */
    /* +0x24 */ DVDDiskID* id;                /* the disc id a disc-change command checks against */
    /* +0x28 */ DVDCBCallback callback;       /* runs when the command finishes */
    /* +0x2C */ void* userData;               /* the caller's own word */
} DVDCommandBlock; /* size: 0x30 */

/* size: 0x20 - what the drive reports about itself (only the device code is read). */
typedef struct DVDDriveInfo {
    /* +0x00 */ u16 revisionLevel;
    /* +0x02 */ u16 deviceCode;
    /* +0x04 */ u32 releaseDate;
    /* +0x08 */ u8 pad_0x08[0x18];
} DVDDriveInfo;

/* 0x804A64E0 - initialises the DVD library and the drive. */
void DVDInit(void);

/* 0x804AA560 - queues a drive inquiry that fills `info`; `callback` runs when it finishes. */
s32 DVDInquiryAsync(DVDCommandBlock* block, DVDDriveInfo* info, DVDCBCallback callback);

/* 0x804AA470 - queues an asynchronous read of `length` bytes at `offset` (in 4-byte units) into `addr`. */
/* untyped: byte range the read fills */
s32 DVDReadAbsAsyncPrio(DVDCommandBlock* block, void* addr, s32 length, s32 offset, DVDCBCallback callback, s32 prio);

/* 0x804AA640 - the state of a queued command: 0 when finished, 1 and 2 while it waits or runs, negative on failure. */
s32 DVDGetCommandBlockStatus(const DVDCommandBlock* block);

/* 0x804AA710 - the drive's state: 0 when idle, 1 while busy, negative on a fatal error. */
s32 DVDGetDriveStatus(void);

/* 0x804AA7B0 - turns the cache invalidation after reads on or off and returns the previous setting. */
BOOL DVDSetAutoInvalidation(BOOL enable);

/* 0x804AA7C0 - resumes the drive queue. */
void DVDResume(void);

/* 0x804AA810 - cancels `block`; `callback` runs once the cancel completes. */
BOOL DVDCancelAsync(DVDCommandBlock* block, DVDCBCallback callback);

/* 0x804AAB80 - cancels `block` and sleeps until the cancel completes. */
s32 DVDCancel(DVDCommandBlock* block);

/* 0x804AAD90 - quiesces the drive and calls `callback` when it is done. */
void __DVDPrepareResetAsync(DVDCBCallback callback);

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

/* 0x804AB020 - stops the drive motor; a stub in this build that reports success without queuing `block`. */
BOOL __DVDStopMotorAsync(DVDCommandBlock* block, DVDCBCallback callback);

/* 0x804AB030 - restarts the drive motor; a stub in this build. */
void __DVDRestartMotor(void);

/* 0x80746690 - the command block the motor-stop request runs on (DVDGetCommandBlockStatus tests it). */
extern DVDCommandBlock __DVDStopMotorCommandBlock;

#ifdef __cplusplus
}
#endif

#endif /* DVD_DVD_H */
