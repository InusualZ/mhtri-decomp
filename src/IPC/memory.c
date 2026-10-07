/*
 * IPC/memory.c - the SDK IOS heap helpers (`iosCreateHeap`, `iosAllocAligned`, `iosFree`).
 *
 * RANGE. `.text` 0x804BD070-0x804BD5A0 (4 functions / 0x518 B); `.bss` 0x807473C0-0x80747440.
 *   - `.bss` 0x807473C0 (the heap table) is read only by these functions; the neighbours read disjoint data
 * FLAGS. the `OS` lib block of `configure.py` (`Wii/1.3`) with `cflags_base` (-O4,p, default alignment 16).
 * NAMES. the unit name is a GUESS (the library's file name in the SDK scheme); `__iosAlloc` is a GUESS for the map's
 *   former placeholder (the body `iosAllocAligned` forwards to); the heap and block record fields are named from
 *   their use.
 * RESIDUALS. relocdiff by name: the heap table is `heaps` here and `lbl_807473C0` in the map (name only, same address).
 *   `__iosAlloc` (96 %): the target keeps `alignedSize` in a saved register across the whole body where this
 *   source lets it die early (same instructions otherwise; declaration orders were measured).  `iosFree` (99.7 %): two
 *   operand-order swaps in the free-block merge sums.
 */

#include "types.h"
#include "IPC/memory.h"
#include "OS/OSInterrupt.h"

#define IOS_MAX_HEAPS 8
#define IOS_HEAP_ERR_INVALID (-4)
#define IOS_HEAP_ERR_NO_SLOT (-5)

/* The tag words of a heap block header. */
#define IOS_BLOCK_FREE 0xBABE0000
#define IOS_BLOCK_USED 0xBABE0001
#define IOS_BLOCK_ALIGN_PAD 0xBABE0002

/* The header in front of every block of a heap; the free blocks are chained through `next`. */
typedef struct IOSHeapBlock {
    /* +0x00 */ u32 magic;
    /* +0x04 */ u32 size; /* payload bytes */
    /* +0x08 */ struct IOSHeapBlock* prev;
    /* +0x0C */ struct IOSHeapBlock* next;
} IOSHeapBlock; /* size: 0x10 */

typedef struct IOSHeap {
    /* +0x00 */ u8* base;
    /* +0x04 */ u32 unused_0x04;
    /* +0x08 */ u32 size;
    /* +0x0C */ IOSHeapBlock* freeList;
} IOSHeap; /* size: 0x10 */

static IOSHeap heaps[IOS_MAX_HEAPS];

/* Registers a 32-byte-aligned memory range as a heap and returns its handle. */
/* untyped: the caller's memory range */
s32 iosCreateHeap(void* base, u32 size)
{
    BOOL enabled;
    s32 i;
    s32 rc = IOS_HEAP_ERR_INVALID;
    IOSHeap* heap;

    enabled = OSDisableInterrupts();
    if (!((u32)base & 0x1F)) {
        for (i = 0; i < IOS_MAX_HEAPS; i++) {
            if (heaps[i].base == NULL) {
                break;
            }
        }
        rc = i;
        if (i == IOS_MAX_HEAPS) {
            rc = IOS_HEAP_ERR_NO_SLOT;
        } else {
            heap = &heaps[i];
            heap->base = base;
            heap->size = size;
            heap->freeList = (IOSHeapBlock*)base;
            heap->freeList->magic = IOS_BLOCK_FREE;
            heap->freeList->size = size - sizeof(IOSHeapBlock);
            heap->freeList->prev = NULL;
            heap->freeList->next = NULL;
        }
    }
    OSRestoreInterrupts(enabled);
    return rc;
}

/* Carves a block of `size` bytes aligned to `align` (a power of two) out of the smallest free block that fits. */
/* untyped: a caller-owned heap block */
void* __iosAlloc(s32 handle, u32 size, u32 align)
{
    IOSHeapBlock* block;
    IOSHeap* heap;
    IOSHeapBlock* best;
    u32 used;
    IOSHeapBlock* tail;
    u32 alignedSize;
    void* result;
    u32 pad;
    BOOL enabled;

    result = NULL;
    enabled = OSDisableInterrupts();
    if (size != 0 && align != 0 && !(align & (align - 1))) {
        if (align < 0x20) {
            align = 0x20;
        }
        alignedSize = (size + 0x1F) & ~0x1F;

        if ((u32)handle > IOS_MAX_HEAPS - 1 || heaps[handle].base == NULL) {
            result = NULL;
        } else {
            heap = &heaps[handle];
            best = NULL;
            for (block = heap->freeList; block != NULL; block = block->next) {
                pad = (align - ((u32)(block + 1) & (align - 1))) & (align - 1);
                if (block->size == alignedSize && pad == 0) {
                    best = block;
                    break;
                }
                if (block->size >= alignedSize + pad && (best == NULL || block->size < best->size)) {
                    best = block;
                }
            }

            if (best != NULL) {
                pad = (align - ((u32)(best + 1) & (align - 1))) & (align - 1);
                used = alignedSize + pad;
                if (best->size > used + sizeof(IOSHeapBlock)) {
                    tail = (IOSHeapBlock*)((u8*)best + used);
                    tail[1].magic = IOS_BLOCK_FREE;
                    tail[1].size = best->size - alignedSize - pad - sizeof(IOSHeapBlock);
                    tail[1].next = best->next;
                    if (best->next != NULL) {
                        best->next->prev = &tail[1];
                    }
                    best->next = &tail[1];
                    best->size = used;
                }
                best->magic = IOS_BLOCK_USED;
                if (best->prev != NULL) {
                    best->prev->next = best->next;
                } else {
                    heap->freeList = best->next;
                }
                if (best->next != NULL) {
                    best->next->prev = best->prev;
                }
                best->next = NULL;
                best->prev = NULL;
                result = (u8*)best + sizeof(IOSHeapBlock) + pad;
                if (pad != 0) {
                    ((IOSHeapBlock*)result)[-1].magic = IOS_BLOCK_ALIGN_PAD;
                    ((IOSHeapBlock*)result)[-1].prev = best;
                }
            }
        }
    }
    OSRestoreInterrupts(enabled);
    return result;
}

/* Allocates an aligned block from a heap. */
/* untyped: a caller-owned heap block */
void* iosAllocAligned(s32 handle, u32 size, u32 align)
{
    return __iosAlloc(handle, size, align);
}

/* Returns a block to its heap and merges it with free neighbours. */
/* untyped: a caller-owned heap block */
s32 iosFree(s32 handle, void* ptr)
{
    IOSHeap* heap;
    IOSHeapBlock* block;
    IOSHeapBlock* next;
    IOSHeapBlock* nn;
    IOSHeapBlock* head;
    BOOL enabled;
    s32 rc;
    IOSHeapBlock* cursor;

    rc = IOS_HEAP_ERR_INVALID;
    enabled = OSDisableInterrupts();
    if (ptr != NULL) {
        if ((u32)handle > IOS_MAX_HEAPS - 1 || heaps[handle].base == NULL) {
            rc = IOS_HEAP_ERR_INVALID;
        } else {
            heap = &heaps[handle];
            if ((u8*)ptr >= heap->base + sizeof(IOSHeapBlock) && (u8*)ptr <= heap->base + heap->size) {
                block = (IOSHeapBlock*)ptr - 1;
                if (block->magic == IOS_BLOCK_ALIGN_PAD) {
                    block = block->prev;
                }
                if (block->magic == IOS_BLOCK_USED) {
                    block->magic = IOS_BLOCK_FREE;
                    head = heap->freeList;
                    cursor = head;
                    while (cursor != NULL) {
                        next = cursor->next;
                        if (next != NULL && next <= block) {
                            cursor = next;
                        } else {
                            break;
                        }
                    }
                    if (cursor != NULL && block > cursor) {
                        block->prev = cursor;
                        block->next = cursor->next;
                        cursor->next = block;
                        if (block->next != NULL) {
                            block->next->prev = block;
                        }
                    } else {
                        block->next = head;
                        heap->freeList = block;
                        block->prev = NULL;
                        if (block->next != NULL) {
                            block->next->prev = block;
                        }
                    }
                    if (block != NULL) {
                        next = block->next;
                        if (next == (IOSHeapBlock*)((u8*)block + block->size + sizeof(IOSHeapBlock))) {
                            nn = next->next;
                            block->next = nn;
                            if (nn != NULL) {
                                nn->prev = block;
                            }
                            block->size = next->size + block->size + sizeof(IOSHeapBlock);
                        }
                    }
                    cursor = block->prev;
                    if (cursor != NULL) {
                        next = cursor->next;
                        if (next == (IOSHeapBlock*)((u8*)cursor + cursor->size + sizeof(IOSHeapBlock))) {
                            nn = next->next;
                            cursor->next = nn;
                            if (nn != NULL) {
                                nn->prev = cursor;
                            }
                            cursor->size = next->size + cursor->size + sizeof(IOSHeapBlock);
                        }
                    }
                    rc = 0;
                }
            }
        }
    }
    OSRestoreInterrupts(enabled);
    return rc;
}
