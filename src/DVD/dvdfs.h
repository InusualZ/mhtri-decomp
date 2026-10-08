/*
 * DVD/dvdfs.h - declarations of the symbols owned by `DVD/dvdfs.c` that other units read.
 */
#ifndef DVD_DVDFS_H
#define DVD_DVDFS_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80793DD0 - non-zero once the long file name mode is enabled. */
extern BOOL __DVDLongFileNameFlag;

#ifdef __cplusplus
}
#endif

#endif
