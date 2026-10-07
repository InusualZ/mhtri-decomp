/*
 * AX/AX.h - declarations of the symbols owned by `AX/AX.c` that other units call.
 */
#ifndef AX_AX_H
#define AX_AX_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

void AXInit(void);
void AXInitEx(u32 mode);
void AXQuit(void);
BOOL AXIsInit(void);

#ifdef __cplusplus
}
#endif

#endif
