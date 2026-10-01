/*
 * OS/FindContainHeap_.h - declarations of the symbols owned by `OS/FindContainHeap_.c` that other units call or read.
 */
#ifndef OS_FINDCONTAINHEAP__H
#define OS_FINDCONTAINHEAP__H

#include "types.h"
#include "OS/mem.h"

#ifdef __cplusplus
extern "C" {
#endif

/* untyped: opaque band object, typed by the callers' views */
void* MEMAllocFromAllocator(MEMAllocator* allocator, u32 size);

/* untyped: opaque band object, typed by the callers' views */
void MEMFreeToAllocator(MEMAllocator* allocator, void* block);

#ifdef __cplusplus
}
#endif

#endif
