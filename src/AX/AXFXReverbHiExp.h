/*
 * AX/AXFXReverbHiExp.h - entry points of the AXFXReverbHiExp unit (0x80474F30..0x80475E30) that the AXFXReverbHi wrappers call.
 */
#ifndef AX_AXFXREVERBHIEXP_H
#define AX_AXFXREVERBHIEXP_H

#include "AX/AXFXReverbHi.h"

BOOL AXFXReverbHiExpInit(AXFXReverbHi* reverb);
void DVDCancel(AXFXReverbHi* reverb);
BOOL AXFXReverbHiExpSettings(AXFXReverbHi* reverb);
void AXFXReverbHiExpCallback(AXFXBuffer* buffer, AXFXReverbHi* reverb);
BOOL AXFXReverbHiExpAllocLines(AXFXReverbHi* reverb);
void AXFXReverbHiExpClearLines(AXFXReverbHi* reverb);
void AXFXReverbHiExpFreeLines(AXFXReverbHi* reverb);
BOOL AXFXReverbHiExpApplySettings(AXFXReverbHi* reverb);

#endif
