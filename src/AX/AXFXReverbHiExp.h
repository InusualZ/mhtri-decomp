/*
 * AX/AXFXReverbHiExp.h - entry points of the AXFXReverbHiExp unit (0x80474F30..0x80475E30) that the AXFXReverbHi wrappers call.
 */
#ifndef AX_AXFXREVERBHIEXP_H
#define AX_AXFXREVERBHIEXP_H

#include "AX/AXFXReverbHi.h"

BOOL AXFXReverbHiExpInit(AXFXReverbHi* reverb);
void DVDCancel(AXFXReverbHi* reverb);
BOOL AXFXReverbHiExpSettings(AXFXReverbHi* reverb);
void AXFXReverbHiExpCallback(AXFXBuffer* buffer, AXFXReverbHi* reverb, s32* out0, s32* out1);
BOOL fn_80475730(AXFXReverbHi* reverb);
void fn_804758B0(AXFXReverbHi* reverb);
void fn_804759E0(AXFXReverbHi* reverb);
BOOL fn_80475B00(AXFXReverbHi* reverb);

#endif
