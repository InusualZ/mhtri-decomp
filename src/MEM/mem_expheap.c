/*
 * MEM/mem_expheap.c - the MEM expandable heap: region search and recycling, create/destroy, allocate/free,
 *    allocatable-size query.
 * RANGE. .text 0x804C1BD0-0x804C2460 (9 functions).  Cut from the OS core band 0x804C1760-0x804D9B4C.  Evidence: its
 *    first function `fn_804C1BD0` is the region search `fn_804C1E00` and `fn_804C1EE0` call; the range ends where
 *    the allocator's two callbacks (0x804C2460/0x804C2470, address-taken by `MEMInitAllocatorForExpHeap`) begin;
 *    it reads no heap-core data.
 * FLAGS. `cflags_os` (configure.py), the flags of the unit it was cut from; the bodies written here are measured with
 *    them.
 * NAMES. `MEMCreateExpHeapEx`, `MEMDestroyExpHeap`, `MEMFreeToExpHeap` are the map's names; GUESS (the dump holds a junk name at
 *    the address): `MEMAllocFromExpHeapEx`, `MEMGetAllocatableSizeForExpHeapEx`, and the file name.
 * RESIDUALS. `fn_804C1BD0`, `fn_804C1E00`, `fn_804C1EE0`, `RecycleRegion_` and `MEMFreeToExpHeap` are unwritten; the
 *    written bodies are `MEMCreateExpHeapEx` 86 %, `MEMDestroyExpHeap` 100 %, `MEMAllocFromExpHeapEx` 97 %,
 *    `MEMGetAllocatableSizeForExpHeapEx` 72 %.
 * SHAPES. none beyond what the bodies show.
 */

#include "types.h"

#include "MEM/mem.h"
#include "MEM/mem_expheap.h"
#include "MEM/mem_heap.h"
#include "OS/OSMutex.h"

/* The region search the allocate entry point dispatches to (unwritten). */
void* fn_804C1E00(MEMiHeapHead* heap, u32 size, u32 align);
void* fn_804C1EE0(MEMiHeapHead* heap, u32 size, u32 align);

/* Create an expandable heap over [start,start+size); it needs 100 B of headroom. */
void* MEMCreateExpHeapEx(void* start, u32 size, u16 attribute)
{
    u8* end = (u8*)((u32)start + size);
    u8* base = (u8*)(((u32)start + 3) & ~3u);
    MEMiHeapHead* heap;
    u8* block;

    if (base > end)
        return NULL;
    if ((u32)(end - base) < 100)
        return NULL;

    MEMiInitHeapHead((MEMiHeapHead*)base, 0x45585048 /* 'EXPH' */, base + 0x50, end, attribute);
    heap = (MEMiHeapHead*)base;

    heap->freeCount = 0;
    heap->freeOffset = 0;

    block = (u8*)heap->start;
    {
        MEMiHeapRegion* region = (MEMiHeapRegion*)block;
        region->signature = 0x4652; /* 'RF' free region */
        region->flags = 0;
        region->size = (u32)heap->end - ((u32)region + 16);
        region->prev = 0;
        region->next = 0;
    }

    heap->freeHead = block;
    heap->freeTail = block;
    heap->usedHead = NULL;
    heap->usedTail = NULL;

    return base;
}

/* Tear a heap down and hand its address back. */
void* MEMDestroyExpHeap(void* heap)
{
    MEMiFinalizeHeap((MEMiHeapHead*)heap);
    return heap;
}

/* Allocate `size` bytes at `align` (a negative alignment means "aligned down from the region end"). */
void* MEMAllocFromExpHeapEx(MEMiHeapHead* heap, u32 size, s32 align)
{
    void* block;

    if (size == 0)
        size = 1;
    size = (size + 3) & ~3u;

    if (heap->attribute & 4)
        OSLockMutex(heap->mutex);

    if (align >= 0)
        block = fn_804C1E00(heap, size, (u32)align);
    else
        block = fn_804C1EE0(heap, size, (u32)(-align));

    if (heap->attribute & 4)
        OSUnlockMutex(heap->mutex);

    return block;
}

/* The largest allocatable region for `size` (sign selects the search side). */
u32 MEMGetAllocatableSizeForExpHeapEx(MEMiHeapHead* heap, s32 size)
{
    u32 best = 0;
    u32 bestOffset = 0xFFFFFFFF;
    MEMiHeapRegion* region;

    if (size < 0)
        size = -size;

    if (heap->attribute & 4)
        OSLockMutex(heap->mutex);

    region = (MEMiHeapRegion*)heap->freeHead;
    while (region != NULL) {
        u32 body = (u32)region + 16;
        u32 total = region->size;
        u32 aligned = ((u32)size + body - 1) & ~((u32)size - 1);
        u32 limit = total + body;

        if (aligned < limit) {
            u32 room = limit - aligned;
            u32 offset = body - aligned;

            if (best < room || (best == room && bestOffset > offset)) {
                best = room;
                bestOffset = offset;
            }
        }
        region = (MEMiHeapRegion*)region->next;
    }

    if (heap->attribute & 4)
        OSUnlockMutex(heap->mutex);

    return best;
}
