/*
 * IPC/memory.h - declarations of the symbols owned by `IPC/memory.c` that other units call (docs/plan.md 6.5 rule 2).
 */
#ifndef MHTRI_IPC_MEMORY_H
#define MHTRI_IPC_MEMORY_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

s32 iosCreateHeap(void* base, u32 size);
void* iosAllocAligned(s32 handle, u32 size, u32 align);
void iosFree(s32 handle, void* ptr);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_IPC_MEMORY_H */
