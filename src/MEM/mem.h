/*
 * MEM/mem.h - the Nintendo SDK memory-library (MEM) records: the intrusive list, the heap head, the allocator.
 *
 * The layout is read from the library's disassembly (0x804C1760..0x804C25E0): `mem_List`'s node is `{prev,next}`
 * at `object + list->offset`, the heap head stores its signature at +0x00, its start/end at +0x18/+0x1C, its
 * OSMutex at +0x20 and its attribute byte at +0x38, and an expandable heap keeps its free/used region lists
 * and the first free region's header as below.
 *
 * `MEMAllocator` is also viewed by `src/nw_resource.cpp`; this header is the single home (rule 1) and that
 * unit is filed as a shared-file request to include it and drop its local copy.
 */

#ifndef MHTRI_MEM_MEM_H
#define MHTRI_MEM_MEM_H

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

struct MEMAllocator;

/* The allocator's function table: `MEMAllocFromAllocator` calls `alloc`, `MEMFreeToAllocator` calls `free`. */
typedef struct MEMAllocatorFuncs {
    void* (*alloc)(struct MEMAllocator* allocator, u32 size);  /* +0x00 */ /* untyped: raw heap block */
    void (*free)(struct MEMAllocator* allocator, void* block); /* +0x04 */ /* untyped: raw heap block */
} MEMAllocatorFuncs; /* size: 0x08 */

typedef struct MEMAllocator {
    const MEMAllocatorFuncs* funcs; /* +0x00 */
    struct MEMiHeapHead* heap;      /* +0x04 */
    u32 heapArg;                    /* +0x08 */
    u32 unused_0x0C;                /* +0x0C */
} MEMAllocator; /* size: 0x10 */

/* A free/used region header: the pointer handed out is 16 bytes past it. */
typedef struct MEMiHeapRegion {
    u16 signature;                 /* +0x00  'RF' free, 'UD' used */
    union {
        u16 value;                 /* +0x02 */
        struct {
            u16 allocDirection : 1; /* +0x02  bit 15: 0 from the region start, 1 from its end */
            u16 alignment : 7;      /* +0x02  bits 8..14: padding bytes in front of the header */
            u16 groupID : 8;        /* +0x02  bits 0..7 */
        } bits; /* size: 0x02 */
    } attribute;                   /* +0x02 */
    u32 size;                      /* +0x04  bytes after the header */
    struct MEMiHeapRegion* prev;   /* +0x08 */
    struct MEMiHeapRegion* next;   /* +0x0C */
} MEMiHeapRegion; /* size: 0x10 */

/* Head and tail of a region list (the free list or the used list). */
typedef struct MEMiRegionList {
    MEMiHeapRegion* head; /* +0x00 */
    MEMiHeapRegion* tail; /* +0x04 */
} MEMiRegionList; /* size: 0x08 */

/* Heap attribute word: the option flags occupy its low byte. */
typedef union MEMiHeapAttribute {
    u32 value; /* +0x00 */
    struct {
        u32 reserved : 24; /* +0x38 */
        u32 optFlag : 8;   /* +0x38 */
    } fields; /* size: 0x04 */
} MEMiHeapAttribute; /* size: 0x04 */

/* SDK heap head (mem_Heap base). */
typedef struct MEMiHeapHead {
    u32 signature;   /* +0x00 */
    MEMLink link;    /* +0x04  node in the parent heap's (or the root) list */
    MEMList children; /* +0x0C  child-heap list; nodes at +0x04 */
    void* start;     /* +0x18 */
    void* end;       /* +0x1C */
    u8 mutex[0x18];  /* +0x20  OSMutex */
    MEMiHeapAttribute attribute; /* +0x38 */
} MEMiHeapHead; /* size: 0x3C */

/* The expandable heap's own data, directly after its MEMiHeapHead. */
typedef struct MEMiExpHeapHead {
    MEMiRegionList freeList; /* +0x00 */
    MEMiRegionList usedList; /* +0x08 */
    u16 groupID;             /* +0x10 */
    union {
        u16 value;                /* +0x12 */
        struct {
            u16 reserved : 14;    /* +0x12  bits 2..15 */
            u16 reuseMargins : 1; /* +0x12  bit 1: alignment margins become free regions */
            u16 bestFit : 1;      /* +0x12  bit 0: best-fit instead of first-fit search */
        } bits; /* size: 0x02 */
    } features;                   /* +0x12 */
} MEMiExpHeapHead; /* size: 0x14 */

typedef struct MEMiExpHeap {
    MEMiHeapHead head;       /* +0x00 */
    MEMiExpHeapHead exp;     /* +0x3C */
} MEMiExpHeap; /* size: 0x50 */

typedef struct MEMRegion {
    u32 start; /* +0x00 */
    u32 end;   /* +0x04 */
} MEMRegion; /* size: 0x08 */

#endif /* MHTRI_MEM_MEM_H */
