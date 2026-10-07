/*
 * MEM/mem_allocator.c - the MEM allocator indirection: the two expandable-heap callbacks, `MEMAllocFromAllocator`,
 *    `MEMFreeToAllocator`, `MEMInitAllocatorForExpHeap`.
 * RANGE. .text 0x804C2460-0x804C24C0 (5 functions); .sdata2 0x8079D270-0x8079D278.  Cut from the OS core band
 *    0x804C1760-0x804D9B4C.  Evidence: the 8-byte callback table at .sdata2 0x8079D270 is read only by
 *    `MEMInitAllocatorForExpHeap` (0x804C24A0), whose two table entries are the 16- and 8-byte stubs at
 *    0x804C2460/0x804C2470.
 * FLAGS. `cflags_os` (configure.py), the flags of the unit it was cut from; the bodies written here are measured with
 *    them.
 * NAMES. `MEMAllocFromAllocator`, `MEMInitAllocatorForExpHeap` are the map's names; GUESS (the dump holds a junk name at the
 *    address): `MEMFreeToAllocator`, and the file name.
 * RESIDUALS. `fn_804C2460` and `fn_804C2470` (the callbacks) are unwritten; `MEMInitAllocatorForExpHeap` is partial (83 %):
 *    the .sdata2 callback table `lbl_8079D270` (0x8079D270..0x8079D278) is claimed but not emitted (it is declared, never defined), so the
 *    function misses the relocation to it; `MEMAllocFromAllocator` and `MEMFreeToAllocator` match.
 * SHAPES. none beyond what the bodies show.
 */

#include "types.h"

#include "MEM/mem.h"
#include "MEM/mem_allocator.h"

/* Call the allocator's alloc function. */
void* MEMAllocFromAllocator(MEMAllocator* allocator, u32 size)
{
    return ((void* (*)(MEMAllocator*, u32))allocator->funcs->alloc)(allocator, size);
}

/* Call the allocator's free function. */
void MEMFreeToAllocator(MEMAllocator* allocator, void* block)
{
    ((void (*)(MEMAllocator*, void*))allocator->funcs->free)(allocator, block);
}

/* Point an allocator record at an expandable heap. */
void MEMInitAllocatorForExpHeap(MEMAllocator* allocator, void* heap, int align)
{
    allocator->funcs = NULL;
    allocator->heap = heap;
    allocator->heapArg = (u32)align;
    allocator->unused_0x0C = 0;
}
