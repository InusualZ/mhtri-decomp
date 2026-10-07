/*
 * IPC/ipcProfile.h - declarations of the symbols owned by `IPC/ipcProfile.c` that other units call.
 */
#ifndef IPC_IPCPROFILE_H
#define IPC_IPCPROFILE_H

#include "types.h"
#include "IPC/ipcclt.h"

#ifdef __cplusplus
extern "C" {
#endif

void IPCiProfInit(void);
void IPCiProfQueueReq(IPCRequest* req, s32 fd);
void IPCiProfAck(void);
void IPCiProfReply(IPCRequest* req, s32 fd);

#ifdef __cplusplus
}
#endif

#endif
