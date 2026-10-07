/*
 * MEM/mem_allocator.c - the MEM allocator indirection: the two expandable-heap callbacks, `MEMAllocFromAllocator`,
 *    `MEMFreeToAllocator`, `MEMInitAllocatorForExpHeap`.
 * RANGE. .text 0x804C2460-0x804C24C0 (5 functions); .sdata2 0x8079D270-0x8079D278.  Cut from the OS core band
 *    0x804C1760-0x804D9B4C.  Evidence: the 8-byte callback table at .sdata2 0x8079D270 is read only by
 *    `MEMInitAllocatorForExpHeap` (0x804C24A0), whose two table entries are the 16- and 8-byte stubs at
 *    0x804C2460/0x804C2470.
 * FLAGS. `cflags_base` per object (configure.py): the target's function starts are all 16-aligned.
 * NAMES. `MEMAllocFromAllocator`, `MEMInitAllocatorForExpHeap` are the map's names; GUESS (the dump holds a junk name at the
 *    address): `MEMFreeToAllocator`, `MEMiAllocFromExpHeapCallback`, `MEMiFreeToExpHeapCallback`, `MEMiExpHeapAllocatorFuncs` (the .sdata2 table) and the file name.
 * RESIDUALS. see the measured rows of the report.
 * SHAPES. none beyond what the bodies show.
 */

#include "types.h"

#include "MEM/mem.h"
#include "MEM/mem_allocator.h"
#include "MEM/mem_expheap.h"

/* Forwards the allocator's alloc call to its expandable heap. */
/* untyped: byte range - the raw heap block */
static void* MEMiAllocFromExpHeapCallback(MEMAllocator* allocator, u32 size)
{
    return MEMAllocFromExpHeapEx(allocator->heap, size, allocator->heapArg);
}

/* Forwards the allocator's free call to its expandable heap. */
/* untyped: byte range - the raw heap block */
static void MEMiFreeToExpHeapCallback(MEMAllocator* allocator, void* block)
{
    MEMFreeToExpHeap(allocator->heap, block);
}

static const MEMAllocatorFuncs MEMiExpHeapAllocatorFuncs = {
    MEMiAllocFromExpHeapCallback,
    MEMiFreeToExpHeapCallback,
};

/* Calls the allocator's alloc function. */
void* MEMAllocFromAllocator(MEMAllocator* allocator, u32 size)
{
    return allocator->funcs->alloc(allocator, size);
}

/* Calls the allocator's free function. */
void MEMFreeToAllocator(MEMAllocator* allocator, void* block)
{
    allocator->funcs->free(allocator, block);
}

/* Points an allocator record at an expandable heap. */
void MEMInitAllocatorForExpHeap(MEMAllocator* allocator, MEMiHeapHead* heap, int align)
{
    allocator->funcs = &MEMiExpHeapAllocatorFuncs;
    allocator->heap = heap;
    allocator->heapArg = (u32)align;
    allocator->unused_0x0C = 0;
}
