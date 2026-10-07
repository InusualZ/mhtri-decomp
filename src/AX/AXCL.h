/*
 * AX/AXCL.h - declarations of the symbols owned by `AX/AXCL.c` that other units call.
 */
#ifndef AX_AXCL_H
#define AX_AXCL_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80794F38 - the AX output mode word: 0 stereo, 1 surround, 2 Dolby Pro Logic II. */
extern u32 __AXOutputMode;

/* The DSP cycle costs `__AXSyncPBs` adds per voice, indexed by the voice state bits. */
extern u32 __AXCycleTableA[0x20];
extern u32 __AXCycleTableB[8];

void __AXClInit(void);
void __AXClQuit(void);
s32 __AXGetCommandListCycles(void);
u32 __AXGetCommandListAddress(void);
void __AXNextFrame(u32 surroundBuffer, u32 outputBuffer, s16** channelBuffers);

/* 0x8046FD70 - sets the output mode. */
void AXSetMode(u32 mode);
/* 0x8046FD80 - returns the output mode: 0 stereo, 1 surround, 2 Dolby Pro Logic II. */
u32 AXGetMode(void);
/* 0x8046FD90 - enables or disables the compressor. */
void AXSetCompressor(u32 compressor);
u16 AXGetAuxAReturnVolume(void);
u16 AXGetAuxBReturnVolume(void);
u16 AXGetAuxCReturnVolume(void);
void AXSetMasterVolume(u16 volume);
void AXSetAuxAReturnVolume(u16 volume);
void AXSetAuxBReturnVolume(u16 volume);
void AXSetAuxCReturnVolume(u16 volume);

#ifdef __cplusplus
}
#endif

#endif
