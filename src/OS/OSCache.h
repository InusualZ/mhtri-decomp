/*
 * OS/OSCache.h - declarations of the symbols owned by `OS/OSCache.c` that other units call or read.
 */
#ifndef OS_OSCACHE_H
#define OS_OSCACHE_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x804CC640 - writes a cached range back to main memory and waits. */
void DCFlushRange(void* start, u32 size); /* untyped: byte range */
/* 0x804CC6D0 - stores a range out of the data cache without waiting. */
void DCStoreRangeNoSync(void* start, u32 size); /* untyped: byte range */

/* 0x804CC670 - write a cached range back to main memory. */
void DCStoreRange(void* start, u32 size); /* untyped: byte range */

/* 0x804CC640 - writes a cached range back to main memory and waits. */
void DCFlushRange(void* start, u32 size); /* untyped: byte range */

/* 0x804CC730 - invalidates a range of the instruction cache. */
void ICInvalidateRange(void* start, u32 size); /* untyped: byte range */

#ifdef __cplusplus
}
#endif

#endif
