/*
 * SYN/synenv.h - declarations of the symbols owned by `SYN/synenv.c` that other units call or read.
 */
#ifndef SYN_SYNENV_H
#define SYN_SYNENV_H

#include "types.h"
#include "SYN/syn.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8062A0A0 - the 128-entry key-scaling table (note / 128, six decimals); `SYN/synenv.c` defines it. */
extern f32 SYNKeyScaleTable[128];

/* 0x804DFC50 / 0x804DFF10 - fill a voice's volume and modulation envelope rates from its region.  NAMES: GUESSes. */
void SYNSetupVolumeEnvelope(SYNVoice* voice);
void SYNSetupModulationEnvelope(SYNVoice* voice);

#ifdef __cplusplus
}
#endif

#endif
