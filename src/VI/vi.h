/*
 * VI/vi.h - the entry points of `VI/vi.cpp` that other units call.
 */
#ifndef VI_VI_H
#define VI_VI_H

#include "types.h"
#include "gx/GXRenderModeObj.h"

typedef void (*VIRetraceCallback)(u32 retraceCount);

#ifdef __cplusplus
extern "C" {
#endif

VIRetraceCallback VISetPreRetraceCallback(VIRetraceCallback callback);
VIRetraceCallback VISetPostRetraceCallback(VIRetraceCallback callback);
/* 0x804E7480 / 0x804E7F60 / 0x804E8630 - brings the video interface up / programs it for a render mode / sets the pan window. */
void VIInit(void);
void VIConfigure(const GXRenderModeObj* mode);
void VIConfigurePan(u16 xOrg, u16 yOrg, u16 width, u16 height);
void VIWaitForRetrace(void);
u32 VIGetRetraceCount(void);
u32 VIGetNextField(void);
void VIFlush(void);
u32 VIGetCurrentLine(void);
u32 VIGetTvFormat(void);
u32 VIGetScanMode(void);
u8 VIGetDTVStatus(void);
void VISetNextFrameBuffer(void* fb); /* untyped: caller-owned frame buffer */
void VISetBlack(BOOL black);
void __VIDisplayPositionToXY(u32 hcount, u32 vcount, s16* x, s16* y);
u32 VIResetDimmingCount(void);
u32 VIEnableDimming(s32 enable);
s32 VISetDimmingMode(s32 mode);
u32 __VIResetDimmingControlA(void);
u32 __VIResetDimmingControlB(void);

#ifdef __cplusplus
}
#endif

#endif
