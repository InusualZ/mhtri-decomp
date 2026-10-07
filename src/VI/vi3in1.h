/*
 * VI/vi3in1.h - the entry points and the shared state of `VI/vi3in1.cpp` that the video interface library uses.
 */
#ifndef VI_VI3IN1_H
#define VI_VI3IN1_H

#include "types.h"

/* One gamma curve: the register image `__VISetGammaCoef` sends. */
/* size: 0x22 */
struct VIGammaEntry {
    /* +0x00 */ u16 low[6];
    /* +0x0C */ u8 mid[7];
    /* +0x13 */ u8 pad_0x13;
    /* +0x14 */ u16 high[7];
};

#ifdef __cplusplus
extern "C" {
#endif

extern u32 viEncoderDirtyFlags;

void __VISetEncoderMode(u8 dtvStatus);
void __VISetFilter4EURGB60(u8 filter);
void __VISetCGMS(void);
void __VISetWSS(void);
void __VISetClosedCaption(void);
void __VISendEncoderRegs(void);
void __VISetGammaCoef(const struct VIGammaEntry* gamma);
void __VISetLinearGamma(void);
void __VISetGamma(void);
void __VISetTrapFilter(void);
void VISetTrapFilter(u8 filter);
void __VISendEncoderReg0A(void);
void __VIRequestEncoderUpdate(void);
void __VISetRGBModeImm(void);
void __VIInitEncoder(void);

#ifdef __cplusplus
}
#endif

#endif
