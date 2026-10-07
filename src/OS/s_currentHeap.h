/*
 * OS/s_currentHeap.h - the declaration of `s_currentHeap`, owned by `OS/OSAlloc.c`.
 */
#ifndef OS_S_CURRENTHEAP_H
#define OS_S_CURRENTHEAP_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80793F80 - the handle of the heap `OSAllocFromHeap` users get by default, -1 until the first heap exists. */
extern s32 s_currentHeap;

#ifdef __cplusplus
}
#endif

#endif
