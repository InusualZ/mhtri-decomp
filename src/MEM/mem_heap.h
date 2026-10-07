/*
 * MEM/mem_heap.h - declarations of the symbols owned by `MEM/mem_heap.c` that other units call or read.
 */
#ifndef MEM_MEM_HEAP_H
#define MEM_MEM_HEAP_H

#include "types.h"
#include "MEM/mem.h"

/* 0x804C18A0 - initialises a heap head over [start, end): signature, child-heap list, mutex and attribute. */
/* untyped: byte range - the heap's start and end addresses */
void MEMiInitHeapHead(MEMiHeapHead* heap, u32 signature, void* start, void* end, u16 attribute);

/* 0x804C1A60 - finalises a heap head (unlinks it from its parent). */
void MEMiFinalizeHeap(MEMiHeapHead* heap);

/* 0x804C1760 - the innermost heap of `list` whose range holds `block`, or NULL. */
/* untyped: byte range - the address searched for */
MEMiHeapHead* FindContainHeap_(MEMList* list, const void* block);

#endif
