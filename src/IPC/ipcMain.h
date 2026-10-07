/*
 * IPC/ipcMain.h - declarations of the symbols owned by `IPC/ipcMain.c` that other units call (docs/plan.md 6.5 rule 2).
 */
#ifndef MHTRI_IPC_IPCMAIN_H
#define MHTRI_IPC_IPCMAIN_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

void IPCInit(void);
void IPCReInit(void);
u32 IPCReadReg(u32 reg);
void IPCWriteReg(u32 reg, u32 value);
u8* IPCGetBufferHi(void);
u8* IPCGetBufferLo(void);
void IPCSetBufferLo(u8* lo);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_IPC_IPCMAIN_H */
