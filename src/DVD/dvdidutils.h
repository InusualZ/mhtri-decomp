/*
 * DVD/dvdidutils.h - declarations of the symbols owned by `DVD/dvdidutils.c` that other units call.
 */
#ifndef DVD_DVDIDUTILS_H
#define DVD_DVDIDUTILS_H

#include "types.h"
#include "DVD/dvd.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Whether two disc ids name the same disc; a zero first byte or a 0xFF number is a wildcard. */
BOOL DVDCompareDiskID(const DVDDiskID* id1, const DVDDiskID* id2);

#ifdef __cplusplus
}
#endif

#endif
