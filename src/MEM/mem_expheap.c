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
 *    `AllocUsedBlockFromFreeBlock_`, `AllocFromHead_`, `AllocFromTail_`, and the file name.
 * RESIDUALS. `AllocUsedBlockFromFreeBlock_` (91 %): the used-header bit-field stores are scheduled one slot earlier than the
 *    target's and the local registers are numbered differently; `AllocFromHead_`/`AllocFromTail_`: the best-fit locals take
 *    volatile registers where the target saves them; `RecycleRegion_`: two local registers swapped;
 *    `MEMGetAllocatableSizeForExpHeapEx`: the saved registers are numbered heap/best/align where the target has
 *    align/heap/best (no declaration order found).
 * SHAPES. the bit-field region header (direction, alignment, group id) is stored in two halfword writes by the target.
 */

#include "types.h"

#include "MEM/mem.h"
#include "MEM/mem_expheap.h"
#include "MEM/mem_heap.h"
#include "OS/OSMutex.h"
#include "Runtime.PPCEABI.H/memset.h"

#define EXP_HEAP_ATTR_LOCK 4
#define EXP_HEAP_ATTR_ZERO_FILL 1

/* Links `region` into `list` directly after `prev` (NULL: at the head). */
static inline void InsertRegionAfter_(MEMiRegionList* list, MEMiHeapRegion* prev, MEMiHeapRegion* region)
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

/* Carves a used block for `size` bytes at `memory` out of the free region `block` and returns `memory`. */
/* untyped: byte range - the user memory handed out */
static void* AllocUsedBlockFromFreeBlock_(MEMiExpHeapHead* exp, MEMiHeapRegion* block, void* memory, u32 size,
                                          u16 direction)
{
    MEMiHeapRegion* prev = block->prev;
    MEMiHeapRegion* next = block->next;
    u32 allocStart = (u32)memory - 16;
    u32 allocEnd = (u32)memory + size;
    u32 freeStart = (u32)block - block->attribute.bits.alignment;
    u32 freeEnd = (u32)block + block->size + 16;
    u32 front = allocStart;
    MEMiHeapRegion* used;

    if (prev != NULL)
        prev->next = next;
    else
        exp->freeList.head = next;
    if (next != NULL)
        next->prev = prev;
    else
        exp->freeList.tail = prev;

    if (allocStart - freeStart < 20 || (direction == 0 && !exp->features.bits.reuseMargins)) {
        front = freeStart;
    } else {
        MakeFreeRegion_(&exp->freeList, prev, (MEMiHeapRegion*)freeStart, allocStart);
        prev = (MEMiHeapRegion*)freeStart;
    }

    if (freeEnd - allocEnd < 20 || (direction == 1 && !exp->features.bits.reuseMargins))
        allocEnd = freeEnd;
    else
        MakeFreeRegion_(&exp->freeList, prev, (MEMiHeapRegion*)allocEnd, freeEnd);

    if (((MEMiHeapHead*)exp)[-1].attribute.value & EXP_HEAP_ATTR_ZERO_FILL)
        memset((void*)front, 0, allocEnd - front);

    used = (MEMiHeapRegion*)allocStart;
    used->signature = 0x5544; /* 'UD' */
    used->attribute.value = 0;
    used->attribute.bits.allocDirection = direction;
    used->attribute.bits.alignment = allocStart - front;
    used->size = allocEnd - (allocStart + 16);
    used->prev = NULL;
    used->next = NULL;
    used->attribute.bits.groupID = exp->groupID;
    InsertRegionAfter_(&exp->usedList, exp->usedList.tail, used);
    return memory;
}

/* Allocates `size` bytes aligned to `align` from the lowest end of a free region. */
static void* AllocFromHead_(MEMiHeapHead* heap, u32 size, s32 align)
{
    MEMiExpHeapHead* exp = &((MEMiExpHeap*)heap)->exp;
    u32 bestStart;
    int firstFit;
    MEMiHeapRegion* region;
    MEMiHeapRegion* best;
    u32 bestSize;

    best = NULL;
    bestSize = 0xFFFFFFFF;
    bestStart = 0;
    firstFit = exp->features.bits.bestFit == 0;

    for (region = exp->freeList.head; region != NULL; region = region->next) {
        u32 body = (u32)region + 16;
        u32 start = (body + align - 1) & ~(align - 1);

        if (region->size < size + (start - body))
            continue;
        if (bestSize <= region->size)
            continue;
        best = region;
        bestSize = region->size;
        bestStart = start;
        if (firstFit || region->size == size)
            break;
    }

    if (best != NULL)
        return AllocUsedBlockFromFreeBlock_(exp, best, (void*)bestStart, size, 0);
    return NULL;
}

/* Allocates `size` bytes aligned to `align` from the highest end of a free region. */
static void* AllocFromTail_(MEMiHeapHead* heap, u32 size, s32 align)
{
    MEMiExpHeapHead* exp = &((MEMiExpHeap*)heap)->exp;
    u32 bestSize;
    u32 bestStart;
    int firstFit;
    MEMiHeapRegion* region;
    MEMiHeapRegion* best;

    best = NULL;
    bestSize = 0xFFFFFFFF;
    bestStart = 0;
    firstFit = exp->features.bits.bestFit == 0;

    for (region = exp->freeList.tail; region != NULL; region = region->prev) {
        u32 body = (u32)region + 16;
        u32 start = (region->size + body - size) & ~(align - 1);

        if ((s32)(start - body) < 0)
            continue;
        if (bestSize <= region->size)
            continue;
        best = region;
        bestSize = region->size;
        bestStart = start;
        if (firstFit || region->size == size)
            break;
    }

    if (best != NULL)
        return AllocUsedBlockFromFreeBlock_(exp, best, (void*)bestStart, size, 1);
    return NULL;
}

/* Returns [range.start, range.end) to the free list, merging with the free regions on both sides. */
static int RecycleRegion_(MEMiRegionList* list, MEMRegion* range)
{
    MEMiHeapRegion* region;
    MEMRegion merged = *range;
    MEMiHeapRegion* before = NULL;

    for (region = list->head; region != NULL; region = region->next) {
        if ((u32)region < range->start) {
            before = region;
            continue;
        }
        if ((u32)region == range->end) {
            MEMiHeapRegion* rprev;
            MEMiHeapRegion* rnext;

            merged.end = (u32)region + region->size + 16;
            rprev = region->prev;
            rnext = region->next;
            if (rprev != NULL)
                rprev->next = rnext;
            else
                list->head = rnext;
            if (rnext != NULL)
                rnext->prev = rprev;
            else
                list->tail = rprev;
        }
        break;
    }

    if (before != NULL && (u32)before + before->size + 16 == range->start) {
        MEMiHeapRegion* bprev = before->prev;
        MEMiHeapRegion* bnext = before->next;

        merged.start = (u32)before;
        if (bprev != NULL)
            bprev->next = bnext;
        else
            list->head = bnext;
        if (bnext != NULL)
            bnext->prev = bprev;
        else
            list->tail = bprev;
        before = bprev;
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
    u32 best;
    u32 bestOffset;
    MEMiHeapRegion* region;

    if (align < 0)
        align = -align;

    if (heap->attribute.value & EXP_HEAP_ATTR_LOCK)
        OSLockMutex(heap->mutex);

    best = 0;
    bestOffset = 0xFFFFFFFF;
    for (region = exp->freeList.head; region != NULL; region = region->next) {
        u32 body = (u32)region + 16;
        u32 end = region->size + body;
        u32 start = (align + body - 1) & ~(align - 1);

        if (start < end) {
            u32 room = end - start;
            u32 offset = start - body;

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
