/* MEM/MEMAllocFromAllocator.h - `MEMAllocFromAllocator` and `MEMFreeToAllocator`, which `MEM/mem_allocator.c` owns
 *   (docs/plan.md 6.5 rule 2, leaf header; callers: the g3d object heap helpers). */
#ifndef MHTRI_MEM_MEMALLOCFROMALLOCATOR_H
#define MHTRI_MEM_MEMALLOCFROMALLOCATOR_H

#include "types.h"

struct MEMAllocator;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x804C2480 - allocates `size` bytes through the allocator. */
void* MEMAllocFromAllocator(struct MEMAllocator* pAllocator, u32 size); /* untyped: byte range */
/* 0x804C2490 - returns a block to the allocator. */
void MEMFreeToAllocator(struct MEMAllocator* pAllocator, void* pBlock); /* untyped: byte range */

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MEM_MEMALLOCFROMALLOCATOR_H */
