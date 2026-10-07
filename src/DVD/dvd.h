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
