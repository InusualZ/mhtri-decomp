/*
 * MEM/mem_expheap.h - declarations of the symbols owned by `MEM/mem_expheap.c` that other units call or read.
 */
#ifndef MEM_MEM_EXPHEAP_H
#define MEM_MEM_EXPHEAP_H

#include "types.h"
#include "MEM/mem.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x804C2120 / 0x804C21D0 - create an expandable heap over [start, start + size) / tear it down; both hand
 * the heap's address back. */
/* untyped: byte range - the heap's backing memory in, the heap head (at its start) out */
void* MEMCreateExpHeapEx(void* start, u32 size, u16 attribute);
/* untyped: opaque handle passed through - the heap MEMCreateExpHeapEx returned */
void* MEMDestroyExpHeap(void* heap);

/* 0x804C2200 / 0x804C22B0 - allocate `size` bytes at `align` (negative: from the heap's end) / free a block. */
/* untyped: byte range - a raw heap block */
void* MEMAllocFromExpHeapEx(MEMiHeapHead* heap, u32 size, s32 align);
/* untyped: byte range - a raw heap block */
void MEMFreeToExpHeap(MEMiHeapHead* heap, void* block);

/* 0x804C2380 - the largest block the heap can hand out at the given alignment. */
u32 MEMGetAllocatableSizeForExpHeapEx(MEMiHeapHead* heap, s32 align);

#ifdef __cplusplus
}
#endif

#endif
