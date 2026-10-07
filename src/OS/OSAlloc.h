/*
 * OS/OSAlloc.h - declarations of the symbols owned by `OS/OSAlloc.c` that other units call or read.
 */
#ifndef OS_OSALLOC_H
#define OS_OSALLOC_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x804CBDC0 - takes `size` bytes from heap `heap`, or NULL when no free cell fits. */
/* untyped: raw heap memory */
void* OSAllocFromHeap(int heap, u32 size);

/* 0x804CBEC0 - returns the block `ptr` to heap `heap`. */
/* untyped: raw heap memory */
void OSFreeToHeap(int heap, void* ptr);

/* 0x804CBF40 - makes `heap` the current heap and returns the previous one. */
int OSSetCurrentHeap(int heap);

/* 0x804CBF50 - lays out `maxHeaps` heap descriptors at `arenaStart` and returns the first free arena address. */
/* untyped: raw arena address */
void* OSInitAlloc(void* arenaStart, void* arenaEnd, int maxHeaps);

/* 0x804CBFC0 - creates a heap over [`start`, `end`) and returns its index, or -1. */
/* untyped: raw arena address */
int OSCreateHeap(void* start, void* end);

#ifdef __cplusplus
}
#endif

#endif
