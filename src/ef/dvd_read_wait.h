/* ef/dvd_read_wait.h - leaf header (docs/plan.md 6.5 rule 2): `ef/system_core.cpp`'s blocking file read, for `menu/movie.cpp`.  C linkage (the map row is a plain name). */
#ifndef MHTRI_EF_DVD_READ_WAIT_H
#define MHTRI_EF_DVD_READ_WAIT_H

#include "types.h"

struct DVDFileInfo;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800CEDD4 - reads `length` bytes at `offset` of `info` into `buffer`, waiting through drive errors; < 0 on failure. */
s32 dvd_read_wait(struct DVDFileInfo* info, void* buffer, s32 length, s32 offset); /* untyped: byte range - the read's destination */

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_DVD_READ_WAIT_H */
