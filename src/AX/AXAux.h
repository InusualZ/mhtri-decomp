/*
 * AX/AXAux.h - declarations of the symbols owned by `AX/AXAux.c` that other units call.
 */
#ifndef AX_AXAUX_H
#define AX_AXAUX_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* untyped: caller-owned context word handed back to the callback */
typedef void (*AXAuxCallback)(s32** buffers, void* context);

void __AXAuxInit(void);
void __AXAuxQuit(void);
void __AXGetAuxAInput(u32* address);
void __AXGetAuxAOutput(u32* address);
void __AXGetAuxAInputDpl2(u32* address);
void __AXGetAuxAOutputDpl2R(u32* address);
void __AXGetAuxAOutputDpl2Ls(u32* address);
void __AXGetAuxAOutputDpl2Rs(u32* address);
void __AXGetAuxBInput(u32* address);
void __AXGetAuxBOutput(u32* address);
void __AXGetAuxBInputDpl2(u32* address);
void __AXGetAuxBOutputDpl2R(u32* address);
void __AXGetAuxBOutputDpl2Ls(u32* address);
void __AXGetAuxBOutputDpl2Rs(u32* address);
void __AXGetAuxCInput(u32* address);
void __AXGetAuxCOutput(u32* address);
void __AXProcessAux(void);
/* untyped: caller-owned context word handed back to the callback */
void AXRegisterAuxACallback(AXAuxCallback callback, void* context);
/* untyped: caller-owned context word handed back to the callback */
void AXRegisterAuxBCallback(AXAuxCallback callback, void* context);
/* untyped: caller-owned context word handed back to the callback */
void AXRegisterAuxCCallback(AXAuxCallback callback, void* context);
/* untyped: caller-owned context word handed back to the callback */
void AXGetAuxACallback(AXAuxCallback* callback, void** context);

#ifdef __cplusplus
}
#endif

#endif
