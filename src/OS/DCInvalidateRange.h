/* OS/DCInvalidateRange.h - the data-cache range entry points `OS/OSCache.c` owns (docs/plan.md 6.5 rule 2, leaf
 *   header). */
#ifndef MHTRI_OS_DCINVALIDATERANGE_H
#define MHTRI_OS_DCINVALIDATERANGE_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x804CC610 - invalidates the data-cache lines of the range. */
void DCInvalidateRange(void* start, u32 nBytes); /* untyped: byte range */

/* 0x804CC640 - writes back and invalidates the data-cache lines of the range. */
void DCFlushRange(void* start, u32 nBytes); /* untyped: byte range */

/* 0x804CC6A0 - flushes the data-cache lines of the range without waiting. */
void DCFlushRangeNoSync(void* start, u32 nBytes); /* untyped: byte range */

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_OS_DCINVALIDATERANGE_H */
