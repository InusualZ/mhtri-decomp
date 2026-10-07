/*
 * WPAD/wpad.h - the entry points of `WPAD/wpad.cpp` that other units call.
 */
#ifndef WPAD_WPAD_H
#define WPAD_WPAD_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

void WPADiDebugPrint(const char* format, ...);

s32 WPADiNullCallbackA(void);
s32 WPADiNullCallbackB(void);
u32 WPADiGetReserved0(void);
u32 WPADiGetReserved1(void);
u32 WPADiGetReserved2(void);
u32 WPADiGetReserved3(void);
s32 WPADiReturnZeroA(void);
s32 WPADiReturnZeroB(void);
s32 WPADiReturnZeroC(void);
s32 WBCReadDummy(void);
s32 WBCSetZEROPointDummy(void);
s32 WBCGetTGCWeightDummy(void);

#ifdef __cplusplus
}
#endif

#endif
