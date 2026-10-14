/*
 * DVD/dvdfs.h - declarations of the symbols owned by `DVD/dvdfs.c` that other units read.
 */
#ifndef DVD_DVDFS_H
#define DVD_DVDFS_H

#include "types.h"
#include "OS/OSThread.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80793DD0 - non-zero once the long file name mode is enabled. */
extern BOOL __DVDLongFileNameFlag;

/* 0x80795078 - the queue the DVD threads sleep on while a command they wait for runs. */
extern OSThreadQueue __DVDThreadQueue;

/* 0x804A5AB0 - reads the file-system table pointers the boot code left in low memory. */
void __DVDFSInit(void);

#ifdef __cplusplus
}
#endif

#endif
