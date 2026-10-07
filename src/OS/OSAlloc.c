/*
 * OS/OSAlloc.c - the OS heap allocator: free-list insert, allocate/free, heap creation and the current-heap setters.
 * RANGE. .text 0x804CBD10-0x804CC030 (6 functions); .sdata 0x80793F80-0x80793F88; .sbss 0x80795318-0x80795328.  Cut
 *    from the OS core band 0x804C1760-0x804D9B4C.  Evidence: .sbss 0x80795318..0x80795328 and .sdata 0x80793F80
 *    are read only by this range; `OSFreeToHeap` calls `DLInsert` (0x804CBD10), the first function.
 * FLAGS. `cflags_base` per object (configure.py): the target's function starts are all 16-aligned.
 * NAMES. `DLInsert` and `OSFreeToHeap` are the map's names; GUESS: `OSAllocFromHeap`, `OSSetCurrentHeap`, `OSInitAlloc`,
 *    `OSCreateHeap` (the allocator's entry points by what they do and by their sibling `OSFreeToHeap`), and the statics
 *    `s_currentHeap`, `s_arenaEnd`, `s_arenaStart`, `s_heapCount`, `s_heapTable` (what each word holds).
 * RESIDUALS. `OSInitAlloc` (91.4 %): the end-address rounding is held in r0 across the stores in the target and in r4 in ours
 *    (register allocation of the three tail stores, same instruction set); the object's .text ends 16 bytes and its .sdata 4
 *    bytes before the claimed ends (alignment fill the link puts back only when the next unit is linked at the same alignment).
 * SHAPES. the heap descriptor table is 12-byte records of {size, free list, allocated list}; the cells are doubly linked
 *    with a 32-byte header.
 */

#include "types.h"

#include "OS/OSAlloc.h"
#include "OS/s_currentHeap.h"

#define OS_HEAP_HEADER_SIZE 32 /* bytes in front of every allocated block */

/* size: 0x0C - one free or allocated block header */
typedef struct OSHeapCell {
    /* +0x00 */ struct OSHeapCell* prev;
    /* +0x04 */ struct OSHeapCell* next;
    /* +0x08 */ s32 size;  /* bytes including the 32-byte header */
} OSHeapCell;

/* size: 0x0C - one heap descriptor */
typedef struct OSHeapDesc {
    /* +0x00 */ s32 size;               /* total bytes, negative when the slot is unused */
    /* +0x04 */ OSHeapCell* free;       /* ascending-address free list */
    /* +0x08 */ OSHeapCell* allocated;  /* blocks handed out */
} OSHeapDesc;

s32 s_currentHeap = -1;
static OSHeapDesc* s_heapTable;
static s32 s_heapCount;
static void* s_arenaStart;
static void* s_arenaEnd;

/* Unlinks `cell` from `list` and returns the list head. */
static inline OSHeapCell* DLExtract(OSHeapCell* list, OSHeapCell* cell)
{
    if (cell->next != NULL) {
        cell->next->prev = cell->prev;
    }
    if (cell->prev == NULL) {
        return cell->next;
    }
    cell->prev->next = cell->next;
    return list;
}

/* 0x804CBD10 (0xB0): inserts `cell` into the address-ordered list, merging with its neighbours, and returns the head. */
OSHeapCell* DLInsert(OSHeapCell* list, OSHeapCell* cell)
{
    OSHeapCell* before = NULL;
    OSHeapCell* after = list;

    while (after != NULL) {
        if (cell <= after) {
            break;
        }
        before = after;
        after = after->next;
    }

    cell->next = after;
    cell->prev = before;
    if (after != NULL) {
        after->prev = cell;
        if ((u8*)cell + cell->size == (u8*)after) {
            cell->size += after->size;
            after = after->next;
            cell->next = after;
            if (after != NULL) {
                after->prev = cell;
            }
        }
    }

    if (before != NULL) {
        before->next = cell;
        if ((u8*)before + before->size == (u8*)cell) {
            before->size += cell->size;
            before->next = after;
            if (after != NULL) {
                after->prev = before;
            }
        }
        return list;
    }
    return cell;
}

/* Takes `size` bytes from the first free cell of the heap that fits, splitting off the remainder. */
/* untyped: raw heap memory */
void* OSAllocFromHeap(int heap, u32 size)
{
    OSHeapDesc* desc = &s_heapTable[heap];
    OSHeapCell* cell;
    OSHeapCell* rest;
    u32 left;

    size = (size + OS_HEAP_HEADER_SIZE + 31) & ~31;
    for (cell = desc->free; cell != NULL; cell = cell->next) {
        if ((s32)size <= cell->size) {
            break;
        }
    }
    if (cell == NULL) {
        return NULL;
    }

    left = cell->size - size;
    if (left < 64) {
        desc->free = DLExtract(desc->free, cell);
    } else {
        cell->size = size;
        rest = (OSHeapCell*)((u8*)cell + size);
        rest->size = left;
        rest->prev = cell->prev;
        rest->next = cell->next;
        if (rest->next != NULL) {
            rest->next->prev = rest;
        }
        if (rest->prev != NULL) {
            rest->prev->next = rest;
        } else {
            desc->free = rest;
        }
    }

    cell->next = desc->allocated;
    cell->prev = NULL;
    if (cell->next != NULL) {
        cell->next->prev = cell;
    }
    desc->allocated = cell;
    return (u8*)cell + OS_HEAP_HEADER_SIZE;
}

/* 0x804CBEC0 (0x78): unlinks the block from the allocated list and merges it back into the free list. */
/* untyped: raw heap memory */
void OSFreeToHeap(int heap, void* ptr)
{
    OSHeapDesc* desc = &s_heapTable[heap];
    OSHeapCell* cell = (OSHeapCell*)((u8*)ptr - OS_HEAP_HEADER_SIZE);

    desc->allocated = DLExtract(desc->allocated, cell);
    desc->free = DLInsert(desc->free, cell);
}

#pragma peephole off
/* Makes `heap` the current heap and returns the one it replaces. */
int OSSetCurrentHeap(int heap)
{
    int old;

    old = s_currentHeap;
    s_currentHeap = heap;
    return old;
}
#pragma peephole on

/* Places the descriptor table at `arenaStart`, marks every slot unused and returns the next free arena address. */
/* untyped: raw arena address */
void* OSInitAlloc(void* arenaStart, void* arenaEnd, int maxHeaps)
{
    u32 tableSize = maxHeaps * sizeof(OSHeapDesc);
    int i;

    s_heapTable = (OSHeapDesc*)arenaStart;
    s_heapCount = maxHeaps;
    for (i = 0; i < s_heapCount; i++) {
        OSHeapDesc* desc = &s_heapTable[i];

        desc->size = -1;
        desc->free = desc->allocated = NULL;
    }
    s_currentHeap = -1;
    s_arenaEnd = (void*)((u32)arenaEnd & ~31);
    arenaStart = (void*)((tableSize + (u32)s_heapTable + 31) & ~31);
    s_arenaStart = arenaStart;
    return arenaStart;
}

/* Claims the first unused descriptor for [`start`, `end`) and returns its index, or -1. */
/* untyped: raw arena address */
int OSCreateHeap(void* start, void* end)
{
    int i;
    OSHeapDesc* desc;
    OSHeapCell* cell;

    start = (void*)(((u32)start + 31) & ~31);
    end = (void*)((u32)end & ~31);

    for (i = 0; i < s_heapCount; i++) {
        desc = &s_heapTable[i];
        if (desc->size < 0) {
            desc->size = (u8*)end - (u8*)start;
            cell = (OSHeapCell*)start;
            cell->prev = NULL;
            cell->next = NULL;
            cell->size = desc->size;
            desc->free = cell;
            desc->allocated = NULL;
            return i;
        }
    }
    return -1;
}
