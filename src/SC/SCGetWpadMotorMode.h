/* SC/SCGetWpadMotorMode.h - the Wii remote settings and Bluetooth sensitivity accessors `SC/sc.cpp` owns
 *   (docs/plan.md 6.5 rule 2, leaf header). */
#ifndef MHTRI_SC_SCGETWPADMOTORMODE_H
#define MHTRI_SC_SCGETWPADMOTORMODE_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x804DCD50 - the stored rumble setting (1 = on). */
u8 SCGetWpadMotorMode(void);

/* 0x804DCDB0 - stores the rumble setting. */
s32 SCSetWpadMotorMode(s32 mode);

/* 0x804DCDC0 - the stored sensor bar position (1 = above the screen). */
u8 SCGetWpadSensorBarPosition(void);

/* 0x804DCE20 - the stored speaker volume. */
u8 SCGetWpadSpeakerVolume(void);

/* 0x804DCE80 - stores the speaker volume. */
s32 SCSetWpadSpeakerVolume(u8 volume);

/* 0x804DCCE0 - the stored Bluetooth pointer sensitivity (1..5). */
u8 SCGetBtDpdSensibility(void);

/* 0x804DCCB0 - stores the Bluetooth device table; non-zero on success. */
s32 SCSetBtDeviceInfoArray(const void* array); /* untyped: the caller-owned device table */

/* 0x804DC480 - writes the pending configuration back; `callback` gets the result (0 = ok). */
void SCFlushAsync(void (*callback)(s32 result));

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_SC_SCGETWPADMOTORMODE_H */
