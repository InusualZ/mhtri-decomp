/*
 * MEM/mem_allocator.h - declarations of the symbols owned by `MEM/mem_allocator.c` that other units call or read.
 */
#ifndef MEM_MEM_ALLOCATOR_H
#define MEM_MEM_ALLOCATOR_H

#include "types.h"
#include "MEM/mem.h"

#ifdef __cplusplus
extern "C" {
#endif

/* untyped: opaque band object, typed by the callers' views */
void* MEMAllocFromAllocator(MEMAllocator* allocator, u32 size);

/* untyped: opaque band object, typed by the callers' views */
void MEMFreeToAllocator(MEMAllocator* allocator, void* block);

/* 0x804C24A0 - points an allocator record at an expandable heap. */
/* untyped: opaque handle passed through - the heap MEMCreateExpHeapEx returned */
void MEMInitAllocatorForExpHeap(MEMAllocator* allocator, void* heap, int align);

/* 0x804C24A0 - points an allocator record at an expandable heap. */
/* untyped: opaque handle passed through - the heap MEMCreateExpHeapEx returned */
void MEMInitAllocatorForExpHeap(MEMAllocator* allocator, void* heap, int align);

#ifdef __cplusplus
}
#endif

#endif
