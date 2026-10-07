/*
 * PAD/PADClamp.h - the controller status record and the stick clamp `PAD/PADClamp.c` owns.
 */
#ifndef PAD_PADCLAMP_H
#define PAD_PADCLAMP_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* One controller's polled state. size: 0xC */
typedef struct PADStatus {
    /* +0x00 */ u16 button;
    /* +0x02 */ s8 stickX;
    /* +0x03 */ s8 stickY;
    /* +0x04 */ s8 substickX;
    /* +0x05 */ s8 substickY;
    /* +0x06 */ u8 triggerL;
    /* +0x07 */ u8 triggerR;
    /* +0x08 */ u8 analogA;
    /* +0x09 */ u8 analogB;
    /* +0x0A */ s8 err; /* zero when the controller answered */
    /* +0x0B */ u8 pad_0x0B;
} PADStatus;

/* 0x804D81E0 - applies the dead zone and the octagon limit to the sticks and the dead zone to the triggers of four controllers. */
void PADClamp(PADStatus* status);

#ifdef __cplusplus
}
#endif

#endif
