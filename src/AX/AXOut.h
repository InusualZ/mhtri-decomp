/*
 * AX/AXOut.h - declarations of the symbols owned by `AX/AXOut.c` that other units call.
 */
#ifndef AX_AXOUT_H
#define AX_AXOUT_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*AXUserCallback)(void);

s32 __AXOutNewFrame(void);
void __AXOutInit(u32 mode);
void __AXOutQuit(void);
AXUserCallback AXRegisterCallback(AXUserCallback callback);

#ifdef __cplusplus
}
#endif

#endif
