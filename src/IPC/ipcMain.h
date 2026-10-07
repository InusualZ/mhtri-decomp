/*
 * IPC/ipcMain.h - declarations of the symbols owned by `IPC/ipcMain.c` that other units call (docs/plan.md 6.5 rule 2).
 */
#ifndef MHTRI_IPC_IPCMAIN_H
#define MHTRI_IPC_IPCMAIN_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

void* IPCGetBufferHi(void);
void* IPCGetBufferLo(void);
void IPCSetBufferLo(void* lo);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_IPC_IPCMAIN_H */
