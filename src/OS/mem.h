/*
 * OS/mem.h - the Nintendo SDK memory-library records the `OS` module's low-level runtime band uses.
 *
 * The layout is read from the band's disassembly (0x804C1760..0x804C6D68): `mem_List`'s node is `{prev,next}`
 * at `object + list->offset`, the heap head stores its signature at +0x00, its start/end at +0x18/+0x1C, its
 * OSMutex at +0x20 and its attribute byte at +0x38, and an expandable heap keeps its free/used region lists
 * and the first free region's header as below.
 *
 * `MEMAllocator` is also viewed by `src/nw_resource.cpp`; this header is the single home (rule 1) and that
 * unit is filed as a shared-file request to include it and drop its local copy.
 */

#ifndef MHTRI_OS_MEM_H
#define MHTRI_OS_MEM_H

#include "types.h"

typedef struct MEMLink {
    void* prev; /* +0x00 */
    void* next; /* +0x04 */
} MEMLink; /* size: 0x08 */

typedef struct MEMList {
    void* head; /* +0x00 */
    void* tail; /* +0x04 */
    u16 count;  /* +0x08 */
    u16 offset; /* +0x0A  node displacement inside the object */
} MEMList; /* size: 0x0C */

/* The allocator's function table: `MEMAllocFromAllocator` calls entry 0, `MEMFreeToAllocator` entry 1. */
typedef struct MEMAllocatorFuncs {
    void* alloc; /* +0x00 */
    void* free;  /* +0x04 */
} MEMAllocatorFuncs; /* size: 0x08 */

typedef struct MEMAllocator {
    MEMAllocatorFuncs* funcs; /* +0x00 */
    void* heap;               /* +0x04 */
    u32 heapArg;              /* +0x08 */
    u32 unused_0x0C;          /* +0x0C */
} MEMAllocator; /* size: 0x10 */

/* A free/used region header: the pointer handed out is 16 bytes past it. */
typedef struct MEMiHeapRegion {
    u16 signature; /* +0x00 */
    u16 flags;     /* +0x02 */
    u32 size;      /* +0x04 */
    u32 prev;      /* +0x08 */
    u32 next;      /* +0x0C */
} MEMiHeapRegion; /* size: 0x10 */

/* SDK heap head (mem_Heap base). */
typedef struct MEMiHeapHead {
    u32 signature;   /* +0x00 */
    u32 unused_0x04; /* +0x04 */
    u32 unused_0x08; /* +0x08 */
    MEMList link;    /* +0x0C  child-heap list */
    void* start;     /* +0x18 */
    void* end;       /* +0x1C */
    u8 mutex[0x18];  /* +0x20  OSMutex */
    u8 attribute;    /* +0x38 */
    u8 pad_0x39[3];  /* +0x39 */
    void* freeHead;  /* +0x3C */
    void* freeTail;  /* +0x40 */
    void* usedHead;  /* +0x44 */
    void* usedTail;  /* +0x48 */
    u16 freeCount;   /* +0x4C */
    u16 freeOffset;  /* +0x4E */
} MEMiHeapHead; /* size: 0x50 */

typedef struct MEMRegion {
    u32 start; /* +0x00 */
    u32 end;   /* +0x04 */
} MEMRegion; /* size: 0x08 */

#endif /* MHTRI_OS_MEM_H */
