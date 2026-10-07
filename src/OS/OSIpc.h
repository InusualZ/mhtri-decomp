/*
 * OS/OSIpc.h - declarations of the symbols owned by `OS/OSIpc.c` that other units call or read.
 */
#ifndef OS_OSIPC_H
#define OS_OSIPC_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x804D5670 / 0x804D5680 - the end / start of the IPC buffer range. */
/* untyped: raw address of the IPC buffer range end */
void* __OSGetIPCBufferHi(void);
/* untyped: raw address of the IPC buffer range start */
void* __OSGetIPCBufferLo(void);

/* 0x804D5690 - latches the IPC buffer range from low memory. */
void __OSInitIPCBuffer(void);

#ifdef __cplusplus
}
#endif

#endif
