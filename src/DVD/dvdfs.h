/*
 * DVD/dvdfs.h - declarations of the symbols owned by `DVD/dvdfs.c` that other units read.
 */
#ifndef DVD_DVDFS_H
#define DVD_DVDFS_H

#include "types.h"
#include "DVD/dvd.h"
#include "OS/OSThread.h"

#ifdef __cplusplus
extern "C" {
#endif

struct DVDFileInfo;

/* Called when a file read finishes. */
typedef void (*DVDCallback)(s32 result, struct DVDFileInfo* fileInfo);

/* An open file: its command block, where it starts on the disc and how long it is. */
typedef struct DVDFileInfo {
    /* +0x00 */ DVDCommandBlock cb;    /* the block the file's reads are queued on */
    /* +0x30 */ u32 startAddr;         /* disc address of the file, in the drive's address unit */
    /* +0x34 */ u32 length;            /* file length in bytes */
    /* +0x38 */ DVDCallback callback;  /* runs when an asynchronous read finishes */
} DVDFileInfo; /* size: 0x3C */

/* 0x80793DD0 - non-zero once the long file name mode is enabled. */
extern BOOL __DVDLongFileNameFlag;

/* 0x80795078 - the queue the DVD threads sleep on while a command they wait for runs. */
extern OSThreadQueue __DVDThreadQueue;

/* 0x804A5AB0 - reads the file-system table pointers the boot code left in low memory. */
void __DVDFSInit(void);

/* 0x804A5AE0 - the file-system entry number of a path (relative to the current directory), -1 when not found. */
s32 DVDConvertPathToEntrynum(const char* pathPtr);

/* 0x804A5DF0 - opens the file with entry number `entrynum`; FALSE for a directory or a bad number. */
BOOL DVDFastOpen(s32 entrynum, DVDFileInfo* fileInfo);

/* 0x804A5E60 - opens the file at `fileName`; FALSE (with a warning) when it does not exist. */
BOOL DVDOpen(const char* fileName, DVDFileInfo* fileInfo);

/* 0x804A6120 - closes `fileInfo`, cancelling its pending read. */
BOOL DVDClose(DVDFileInfo* fileInfo);

/* 0x804A6320 - queues a read of `length` bytes at `offset` in the file into `addr`. */
/* untyped: byte range the read fills */
BOOL DVDReadAsyncPrio(DVDFileInfo* fileInfo, void* addr, s32 length, s32 offset, DVDCallback callback, s32 prio);

#ifdef __cplusplus
}
#endif

#endif
