/*
 * AX/AXAlloc.h - declarations of the symbols owned by `AX/AXAlloc.c` that other units call.
 */
#ifndef AX_AXALLOC_H
#define AX_AXALLOC_H

#include "types.h"

#include "AX/AXVPB.h"

#ifdef __cplusplus
extern "C" {
#endif

AXVPB* __AXGetStackHead(u32 priority);
void __AXServiceCallbackStack(void);
void __AXAllocInit(void);
void __AXAllocQuit(void);
void __AXPushFreeStack(AXVPB* vpb);
void __AXPushCallbackStack(AXVPB* vpb);
void __AXRemoveFromStack(AXVPB* vpb);
void AXFreeVoice(AXVPB* vpb);
AXVPB* AXAcquireVoice(u32 priority, AXVPBCallback callback, u32 userContext);
void AXSetVoicePriority(AXVPB* vpb, u32 priority);

#ifdef __cplusplus
}
#endif

#endif
