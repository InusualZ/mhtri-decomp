/*
 * MEM/mem_expheap.c - the MEM expandable heap: region search and recycling, create/destroy, allocate/free,
 *    allocatable-size query.
 * RANGE. .text 0x804C1BD0-0x804C2460 (9 functions).  Cut from the OS core band 0x804C1760-0x804D9B4C.  Evidence: its
 *    first function is the used-block carve that the two region searches call; the range ends where
 *    the allocator's two callbacks (0x804C2460/0x804C2470, address-taken by `MEMInitAllocatorForExpHeap`) begin;
 *    it reads no heap-core data.
 * FLAGS. `cflags_base` per object (configure.py): the target's function starts are all 16-aligned.
 * NAMES. `MEMCreateExpHeapEx`, `MEMDestroyExpHeap`, `MEMFreeToExpHeap`, `RecycleRegion_` are the map's names; GUESS
 *    (the dump holds junk names at the addresses): `MEMAllocFromExpHeapEx`, `MEMGetAllocatableSizeForExpHeapEx`,
 *    `AllocUsedBlockFromFreeBlock_`, `AllocFromHead_`, `AllocFromTail_`, the inlined helpers (no symbols) and the file name.
 * RESIDUALS. `AllocUsedBlockFromFreeBlock_`: the `back.start` add is scheduled after the free-region unlink loads where the
 *    target issues it first, so the volatile temporaries r5/r6 (front start, back end) are swapped and the used-list
 *    append takes r3/r4 the other way round; tried: statement order of the four range bounds (six orders), local
 *    declaration order, an append helper, a region-extent helper.  The other eight functions are byte-identical.
 * SHAPES. The RVL SDK helper structure decides the allocation: `RegionBody_`/`RegionEnd_`, `RemoveRegion_` (returns the
 *    predecessor), `InsertRegionAfter_` (returns the region), `InitRegion_`, `FillAllocMemory_`, inlined into each
 *    caller; best-fit locals initialised at their declarations; `__abs` for the allocatable-size alignment;
 *    `RecycleRegion_` compares the predecessor's end as a region pointer.  The bit-field region header (direction,
 *    alignment, group id) is stored in two halfword writes by the target.
 */

#include "types.h"

#include "MEM/mem.h"
#include "MEM/mem_expheap.h"
#include "MEM/mem_heap.h"
#include "OS/OSMutex.h"
#include "Runtime.PPCEABI.H/memset.h"

#define EXP_HEAP_ATTR_LOCK 4
#define EXP_HEAP_ATTR_ZERO_FILL 1

/* Returns the user memory after a region header. */
static inline u32 RegionBody_(MEMiHeapRegion* region)
{
    return (u32)region + 16;
}

/* Returns the end of a region's user memory. */
static inline u32 RegionEnd_(MEMiHeapRegion* region)
{
    return RegionBody_(region) + region->size;
}

/* Links `region` into `list` directly after `prev` (NULL: at the head) and returns it. */
static inline MEMiHeapRegion* InsertRegionAfter_(MEMiRegionList* list, MEMiHeapRegion* prev, MEMiHeapRegion* region)
{
    MEMiHeapRegion* next;

    region->prev = prev;
    if (prev != NULL) {
        next = prev->next;
        prev->next = region;
    } else {
        next = list->head;
        list->head = region;
    }
    region->next = next;
    if (next != NULL)
        next->prev = region;
    else
        list->tail = region;
    return region;
}

/* Unlinks `region` from `list` and returns the region that preceded it. */
static inline MEMiHeapRegion* RemoveRegion_(MEMiRegionList* list, MEMiHeapRegion* region)
{
    MEMiHeapRegion* prev = region->prev;
    MEMiHeapRegion* next = region->next;

    if (prev != NULL)
        prev->next = next;
    else
        list->head = next;
    if (next != NULL)
        next->prev = prev;
    else
        list->tail = prev;
    return prev;
}

/* Makes a free region header at `start` covering [start, end) and links it after `prev`. */
static inline void MakeFreeRegion_(MEMiRegionList* list, MEMiHeapRegion* prev, MEMiHeapRegion* start, u32 end)
{
    start->signature = 0x4652; /* 'RF' */
    start->attribute.value = 0;
    start->size = end - ((u32)start + 16);
    start->next = NULL;
    InsertRegionAfter_(list, prev, start);
}

/* Zero-fills a fresh block when the heap was created with the zero-fill attribute. */
static inline void FillAllocMemory_(MEMiHeapHead* heap, u32 address, u32 size)
{
    if (heap->attribute.value & EXP_HEAP_ATTR_ZERO_FILL)
        memset((void*)address, 0, size);
}

/* Makes a region header with `signature` at `range->start` covering the range; the header is unlinked. */
static inline MEMiHeapRegion* InitRegion_(MEMRegion* range, u16 signature)
{
    MEMiHeapRegion* region = (MEMiHeapRegion*)range->start;

    region->signature = signature;
    region->attribute.value = 0;
    region->size = range->end - RegionBody_(region);
    region->prev = NULL;
    region->next = NULL;
    return region;
}

/* Carves a used block for `size` bytes at `memory` out of the free region `block` and returns `memory`. */
/* untyped: byte range - the user memory handed out */
static void* AllocUsedBlockFromFreeBlock_(MEMiExpHeapHead* exp, MEMiHeapRegion* block, void* memory, u32 size,
                                          u16 direction)
{
    MEMRegion front;
    MEMRegion back;
    MEMiHeapRegion* prev;

    front.end = (u32)memory - 16;
    back.start = size + (u32)memory;
    front.start = (u32)block - block->attribute.bits.alignment;
    back.end = RegionEnd_(block);

    prev = RemoveRegion_(&exp->freeList, block);

    if (front.end - front.start < 20 || (direction == 0 && !exp->features.bits.reuseMargins))
        front.end = front.start;
    else
        prev = InsertRegionAfter_(&exp->freeList, prev, InitRegion_(&front, 0x4652)); /* 'RF' */

    if (back.end - back.start < 20 || (direction == 1 && !exp->features.bits.reuseMargins))
        back.start = back.end;
    else
        InsertRegionAfter_(&exp->freeList, prev, InitRegion_(&back, 0x4652)); /* 'RF' */

    FillAllocMemory_(&((MEMiHeapHead*)exp)[-1], front.end, back.start - front.end);

    {
        MEMRegion range;
        MEMiHeapRegion* used;

        range.start = (u32)memory - 16;
        range.end = back.start;
        used = InitRegion_(&range, 0x5544); /* 'UD' */
        used->attribute.bits.allocDirection = direction;
        used->attribute.bits.alignment = (u32)used - front.end;
        used->attribute.bits.groupID = exp->groupID;
        InsertRegionAfter_(&exp->usedList, exp->usedList.tail, used);
    }
    return memory;
}

/* Allocates `size` bytes aligned to `align` from the lowest end of a free region. */
static void* AllocFromHead_(MEMiHeapHead* heap, u32 size, s32 align)
{
    MEMiExpHeapHead* exp = &((MEMiExpHeap*)heap)->exp;
    const int firstFit = exp->features.bits.bestFit == 0;
    MEMiHeapRegion* region = NULL;
    MEMiHeapRegion* best = NULL;
    u32 bestSize = 0xFFFFFFFF;
    u32 bestStart = 0;

    for (region = exp->freeList.head; region != NULL; region = region->next) {
        u32 body = RegionBody_(region);
        u32 start = (align + body - 1) & ~(align - 1);
        u32 offset = start - body;

        if (region->size >= size + offset && bestSize > region->size) {
            best = region;
            bestSize = region->size;
            bestStart = start;
            if (firstFit || bestSize == size)
                break;
        }
    }

    if (best != NULL)
        return AllocUsedBlockFromFreeBlock_(exp, best, (void*)bestStart, size, 0);
    return NULL;
}

/* Allocates `size` bytes aligned to `align` from the highest end of a free region. */
static void* AllocFromTail_(MEMiHeapHead* heap, u32 size, s32 align)
{
    MEMiExpHeapHead* exp = &((MEMiExpHeap*)heap)->exp;
    const int firstFit = exp->features.bits.bestFit == 0;
    MEMiHeapRegion* region = NULL;
    MEMiHeapRegion* best = NULL;
    u32 bestSize = 0xFFFFFFFF;
    u32 bestStart = 0;

    for (region = exp->freeList.tail; region != NULL; region = region->prev) {
        u32 body = RegionBody_(region);
        u32 start = (region->size + body - size) & ~(align - 1);

        if ((s32)(start - body) >= 0 && bestSize > region->size) {
            best = region;
            bestSize = region->size;
            bestStart = start;
            if (firstFit || bestSize == size)
                break;
        }
    }

    if (best != NULL)
        return AllocUsedBlockFromFreeBlock_(exp, best, (void*)bestStart, size, 1);
    return NULL;
}

/* Returns [range.start, range.end) to the free list, merging with the free regions on both sides. */
static int RecycleRegion_(MEMiRegionList* list, MEMRegion* range)
{
    MEMiHeapRegion* before = NULL;
    MEMRegion merged = *range;

    {
        MEMiHeapRegion* region;

        for (region = list->head; region != NULL; region = region->next) {
            if ((u32)region < range->start) {
                before = region;
                continue;
            }
            if ((u32)region == range->end) {
                merged.end = RegionEnd_(region);
                RemoveRegion_(list, region);
            }
            break;
        }
    }

    if (before != NULL && (MEMiHeapRegion*)RegionEnd_(before) == (MEMiHeapRegion*)range->start) {
        merged.start = (u32)before;
        before = RemoveRegion_(list, before);
    }

    if (merged.end - merged.start < 16)
        return 0;

    MakeFreeRegion_(list, before, (MEMiHeapRegion*)merged.start, merged.end);
    return 1;
}

/* Creates an expandable heap over [start, start + size); it needs 100 bytes of headroom. */
void* MEMCreateExpHeapEx(void* start, u32 size, u16 attribute)
{
    u32 end = (size + (u32)start) & ~3u;
    u32 base = ((u32)start + 3) & ~3u;
    MEMiExpHeap* heap;
    MEMiHeapRegion* region;
    u32 regionEnd;

    if (base > end || end - base < 100)
        return NULL;

    heap = (MEMiExpHeap*)base;
    MEMiInitHeapHead(&heap->head, 0x45585048 /* 'EXPH' */, (void*)(base + sizeof(MEMiExpHeap)), (void*)end, attribute);

    heap->exp.groupID = 0;
    heap->exp.features.value = 0;

    region = (MEMiHeapRegion*)heap->head.start;
    regionEnd = (u32)heap->head.end;
    region->signature = 0x4652; /* 'RF' */
    region->attribute.value = 0;
    region->size = regionEnd - ((u32)region + 16);
    region->prev = NULL;
    region->next = NULL;

    heap->exp.freeList.head = region;
    heap->exp.freeList.tail = region;
    heap->exp.usedList.head = NULL;
    heap->exp.usedList.tail = NULL;

    return heap;
}

/* Tears a heap down and hands its address back. */
void* MEMDestroyExpHeap(void* heap)
{
    MEMiFinalizeHeap((MEMiHeapHead*)heap);
    return heap;
}

/* Allocates `size` bytes at `align` (a negative alignment allocates from the region end). */
void* MEMAllocFromExpHeapEx(MEMiHeapHead* heap, u32 size, s32 align)
{
    void* block;

    if (size == 0)
        size = 1;
    size = (size + 3) & ~3u;

    if (heap->attribute.value & EXP_HEAP_ATTR_LOCK)
        OSLockMutex(heap->mutex);

    if (align >= 0)
        block = AllocFromHead_(heap, size, align);
    else
        block = AllocFromTail_(heap, size, -align);

    if (heap->attribute.value & EXP_HEAP_ATTR_LOCK)
        OSUnlockMutex(heap->mutex);

    return block;
}

/* Returns a block to the free list. */
/* untyped: byte range - a raw heap block */
void MEMFreeToExpHeap(MEMiHeapHead* heap, void* block)
{
    MEMiHeapRegion* next;
    MEMiHeapRegion* prev;
    MEMiHeapRegion* region;
    MEMRegion range;
    MEMiExpHeapHead* exp;

    if (block == NULL)
        return;

    if (heap->attribute.value & EXP_HEAP_ATTR_LOCK)
        OSLockMutex(heap->mutex);

    region = (MEMiHeapRegion*)block - 1;
    range.start = (u32)region - region->attribute.bits.alignment;
    range.end = (u32)region + region->size + 16;

    exp = &((MEMiExpHeap*)heap)->exp;
    prev = region->prev;
    next = region->next;
    if (prev != NULL)
        prev->next = next;
    else
        exp->usedList.head = next;
    if (next != NULL)
        next->prev = prev;
    else
        exp->usedList.tail = prev;

    RecycleRegion_(&exp->freeList, &range);

    if (heap->attribute.value & EXP_HEAP_ATTR_LOCK)
        OSUnlockMutex(heap->mutex);
}

/* Returns the largest block a free region can hand out at the given alignment. */
u32 MEMGetAllocatableSizeForExpHeapEx(MEMiHeapHead* heap, s32 align)
{
    MEMiExpHeapHead* exp = &((MEMiExpHeap*)heap)->exp;
    MEMiHeapRegion* region;
    u32 best;
    u32 bestOffset;

    align = __abs(align);

    if (heap->attribute.value & EXP_HEAP_ATTR_LOCK)
        OSLockMutex(heap->mutex);

    best = 0;
    bestOffset = 0xFFFFFFFF;
    for (region = exp->freeList.head; region != NULL; region = region->next) {
        u32 start = (RegionBody_(region) + align - 1) & ~(align - 1);

        if (start < RegionEnd_(region)) {
            u32 room = RegionEnd_(region) - start;
            u32 offset = start - RegionBody_(region);

            if (best < room || (best == room && bestOffset > offset)) {
                best = room;
                bestOffset = offset;
            }
        }
    }

    if (heap->attribute.value & EXP_HEAP_ATTR_LOCK)
        OSUnlockMutex(heap->mutex);

    return best;
}
