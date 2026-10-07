/*
 * DVD/dvdidutils.h - declarations of the symbols owned by `DVD/dvdidutils.c` that other units call.
 */
#ifndef DVD_DVDIDUTILS_H
#define DVD_DVDIDUTILS_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The identification record at the start of a disc. */
typedef struct DVDDiskID {
    /* +0x00 */ char gameName[4];
    /* +0x04 */ char company[2];
    /* +0x06 */ u8 diskNumber;
    /* +0x07 */ u8 gameVersion;
    /* +0x08 */ u8 streaming;
    /* +0x09 */ u8 streamingBufSize;
    /* +0x0A */ u8 padding[14];
    /* +0x18 */ u32 rvlMagic;
    /* +0x1C */ u32 gcMagic;
} DVDDiskID; /* size: 0x20 */

/* Whether two disc ids name the same disc; a zero first byte or a 0xFF number is a wildcard. */
BOOL DVDCompareDiskID(const DVDDiskID* id1, const DVDDiskID* id2);

#ifdef __cplusplus
}
#endif

#endif
