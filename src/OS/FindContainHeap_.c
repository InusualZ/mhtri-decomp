/*
 * OS/FindContainHeap_.c - the Nintendo SDK low-level runtime band at 0x804C1760..0x804C68A0.
 *
 * `.text` 0x804C1760..0x804C68A0 (57 functions).  Registered from
 * `proposal/804C1760_FindContainHeap_.c` (docs/plan.md 12); phase 4 cut the tail from 0x804C68A0 into
 * MTX/mtxvec.c, MTX/mtx44.c and MTX/vec.c (the vec cluster and the C_MTXOrtho / PSMTXMultVec stubs).
 *
 * Module `OS` is class-3 evidence: the nearest registered unit in splits.txt is `OS/OSAlarm.c`
 * (0x804CBC50, 0x4EE8 above the range end, against `Runtime.PPCEABI.H/__init_cpp_exceptions.cpp`
 * 0x69E64 below its start), and every foreign call the mem half makes is the OS library (OSInitMutex /
 * OSLockMutex / OSUnlockMutex).  The lib block extended is the existing `OS` block (cflags_os =
 * cflags_base + `-func_align 4`; the run's own `.text` is `align 2**2`, the same evidence OSAlarm.c used).
 *
 * Name.  No `__FILE__` string covers the range (the DOL's data pool holds no memory/matrix source name),
 * so class 1 fails.  The runtime dump gives *real* SDK function names (`dumpmap.py lookup 0x804C1760` ->
 * `FindContainHeap_`, `MEMiInitHeapHead`, `PSMTXIdentity`, `__MIXSetPan`, ...), so those names are used
 * as-is; where the map/dump carries only `fn_XXXXXXXX` the stem is kept (Naming note below).
 *
 * Extent / seam.  `tudiscover at` finds the mtx cluster (0x804C5C10..0x804C6898, 19 functions; owns
 * `.sdata2` 0x8079D278..0x8079D2B8) and the vec cluster (0x804C6B60..) each as *certainly one TU*, so the
 * run holds several original SDK TUs - mem (`mem_Heap` / `mem_ExpHeap` / `mem_allocator` / `mem_List`),
 * mix/AX, mtx and vec.  The seam is unproven (the discovery cut is a byte cap, not a boundary); it
 * settles as its functions match.
 *
 * Residuals (against MAIN's `auto_03_80458A60_text.o`, the retired run object that owns the range):
 *   - the heap search bodies FindContainHeap_ / MEMiInitHeapHead / fn_804C1A60 / fn_804C1BD0 /
 *     fn_804C1E00 / fn_804C1EE0 / RecycleRegion_ - reconstructed best-effort below; the triple-nested
 *     search and the free-region recycler are the hard rows and carry the residual.
 *   - fn_804C2840 (0x16C4) and fn_804C40D0 (0x1694), the two large AX/mix bodies - recorded unwritten.
 *   - the mtx cluster (PSMTXIdentity .. fn_804C6810) is built around paired-single ops (psq_st), the
 *     codegen class docs/matching.md records as not reachable from this C frontend - recorded unwritten.
 *   - the vec cluster (PSVECNormalize .. PSVECSquareDistance), the hand-written vector library - unwritten.
 *
 * Naming note: the map/dump carry only fn_XXXXXXXX for part of this range (checked with
 * `dumpmap.py lookup` over the inventory and `.pi/notes/dumpmap-join.json`): fn_804C1A60, fn_804C1BD0,
 * fn_804C1E00, fn_804C1EE0, MEMAllocFromExpHeapEx, MEMGetAllocatableSizeForExpHeapEx, fn_804C2460, fn_804C2470, fn_804C24C0, fn_804C24E0,
 * fn_804C2550, fn_804C25C0, fn_804C26A0, fn_804C26E0, fn_804C2800, fn_804C2820, fn_804C2830, fn_804C2840,
 * fn_804C3F10..fn_804C40D0, fn_804C5770, fn_804C5BB0, fn_804C5D50, fn_804C5FE0, fn_804C61E0,
 * fn_804C6290, fn_804C6430, fn_804C64E0, fn_804C6660, fn_804C6710, fn_804C6810, fn_804C6900,
 * fn_804C69A0, fn_804C6B30, fn_804C6C60.  The real names (FindContainHeap_, MEMiInitHeapHead,
 * MEMCreateExpHeapEx, MEMDestroyExpHeap, MEMFreeToExpHeap, MEMAllocFromAllocator, MEMFreeToAllocator,
 * MEMInitAllocatorForExpHeap, __MIXSetPan, PSMTX*, PSVEC*, C_MTXOrtho) are used as the map spells them.
 */

#include "types.h"

#include "OS/mem.h"
#include "NAND/nand.h"
#include "OS/FindContainHeap_.h"

extern u32 lbl_80795298;      /* 'heap list initialised' flag (.sbss) */
extern MEMList lbl_80748B90;  /* global heap list (.bss) */
extern u8 lbl_80748BA0[];     /* global heap-list OSMutex (.bss) */
extern u32 lbl_807952A0;      /* AX mix state (.sbss) */
extern u32 lbl_807952A4;      /* AX mix state (.sbss) */
extern u32 lbl_807952B0;      /* AX mix state (.sbss) */
extern int lbl_807952AC;      /* AX mixing mode (.sbss) */
extern u8 lbl_8061AE00[];     /* AX pan/mix coefficient table (.data, 0xBA0 B) */

/* mem_List. */
void fn_804C24C0(MEMList* list, u16 offset);
void fn_804C24E0(MEMList* list, void* object);
void fn_804C2550(MEMList* list, void* object);
void* fn_804C25C0(MEMList* list, void* object);

/* mem_ExpHeap. */
void MEMiInitHeapHead(MEMiHeapHead* heap, u32 signature, void* start, void* end, u16 attribute);
void* fn_804C1E00(MEMiHeapHead* heap, u32 size, u32 align);
void* fn_804C1EE0(MEMiHeapHead* heap, u32 size, u32 align);
void fn_804C1A60(MEMiHeapHead* heap);
u32 MEMGetAllocatableSizeForExpHeapEx(MEMiHeapHead* heap, s32 size);

/* ---------------------------------------------------------------------------------------------------
 * mem_List - the doubly-linked list primitives the heaps and the allocator build on.
 * --------------------------------------------------------------------------------------------------- */

/* Empty a list and record the node displacement its objects use. */
void fn_804C24C0(MEMList* list, u16 offset)
{
    list->head = NULL;
    list->tail = NULL;
    list->count = 0;
    list->offset = offset;
}

/* Append an object, linking it after the current tail (or making it head and tail of an empty list). */
void fn_804C24E0(MEMList* list, void* object)
{
    if (list->head == NULL) {
        MEMLink* link = (MEMLink*)((u8*)object + list->offset);
        link->next = NULL;
        link->prev = NULL;
        list->head = object;
        list->tail = object;
        list->count++;
    } else {
        MEMLink* link = (MEMLink*)((u8*)object + list->offset);
        link->prev = list->tail;
        link->next = NULL;
        ((MEMLink*)((u8*)list->tail + list->offset))->next = object;
        list->tail = object;
        list->count++;
    }
}

/* Unlink an object from the middle of the list and clear its node. */
void fn_804C2550(MEMList* list, void* object)
{
    MEMLink* link = (MEMLink*)((u8*)object + list->offset);

    if (link->prev == NULL)
        list->head = link->next;
    else
        ((MEMLink*)((u8*)link->prev + list->offset))->next = link->next;

    if (link->next == NULL)
        list->tail = link->prev;
    else
        ((MEMLink*)((u8*)link->next + list->offset))->prev = link->prev;

    link->prev = NULL;
    link->next = NULL;
    list->count--;
}

/* The object after `object` (NULL yields the head). */
void* fn_804C25C0(MEMList* list, void* object)
{
    if (object == NULL)
        return list->head;
    return ((MEMLink*)((u8*)object + list->offset))->next;
}

/* ---------------------------------------------------------------------------------------------------
 * mem_allocator - the indirection records.
 * --------------------------------------------------------------------------------------------------- */

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

/* ---------------------------------------------------------------------------------------------------
 * mem_ExpHeap - the expandable heap.
 * --------------------------------------------------------------------------------------------------- */

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
    fn_804C1A60((MEMiHeapHead*)heap);
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

/* ---------------------------------------------------------------------------------------------------
 * The AX mixing state accessors next to __MIXSetPan.
 * --------------------------------------------------------------------------------------------------- */

/* Clear the mixing state. */
void fn_804C2800(void)
{
    lbl_807952A0 = 0;
    lbl_807952B0 = 0;
    lbl_807952A4 = 0;
}

/* Set the mixing mode. */
void fn_804C2820(int mode)
{
    lbl_807952AC = mode;
}

/* Read the mixing mode. */
int fn_804C2830(void)
{
    return lbl_807952AC;
}

/* Clamp an AX pan/volume index into the coefficient table's u16 row. */
u16 fn_804C26A0(s32 x)
{
    if (x <= -904)
        return 0;
    if (x >= 60)
        return 0xFF64;
    return ((u16*)lbl_8061AE00)[x + 904];
}
