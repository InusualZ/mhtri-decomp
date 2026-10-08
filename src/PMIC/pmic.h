/*
 * PMIC/pmic.h - declarations of the public entry points owned by `PMIC/pmic.c` that other units call: the USB
 *   microphone device bring-up and its request API.  The names are GUESSES (the `PMIC/pmic.c` header).
 */
#ifndef MHTRI_PMIC_PMIC_H
#define MHTRI_PMIC_PMIC_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Reports a request's result to its caller with the caller's argument. */
typedef void (*PMICUserCallback)(s32 result, s32 arg);

/* 0x80522E00 - brings the library up once over the caller's work area; returns 0, or a negative PMIC error. */
s32 PMICInit(u8* work, s32 size);

/* 0x80523110 - tears the library down when no transfer is in flight. */
s32 PMICEnd(void);

/* 0x80523200 - asks the USB stack to report the device when it is plugged in. */
s32 PMICStartSearch(void);

/* 0x805232D0 / 0x80523400 - starts the firmware upload / asks a running device to stop; `callback` gets the outcome. */
s32 PMICStartAsync(PMICUserCallback callback, s32 arg);
s32 PMICStopAsync(PMICUserCallback callback, s32 arg);

/* 0x80523490 - stops the device and waits until it has stopped. */
s32 PMICStop(void);

/* 0x80523510 / 0x80523630 - queue the recording start / stop once the device runs. */
s32 PMICStartRecordingAsync(PMICUserCallback callback, s32 arg);
s32 PMICStopRecordingAsync(PMICUserCallback callback, s32 arg);

/* 0x80523850 - returns the library state. */
s32 PMICGetState(void);

/* 0x80526180 / 0x80526270 - copies captured samples out / appends playback samples; returns the count or -1. */
s32 PMICRead(s16* dst, s32 count);
s32 PMICWrite(s16* src, s32 count);

/* 0x805267B0 - queues the command that selects one of four gain steps. */
s32 PMICSetGain(u16 unit, PMICUserCallback callback, s32 arg);

/* 0x80526890 - queues the command that reads one device parameter into `reply`. */
s32 PMICGetParam(u32 index, u32* reply, PMICUserCallback callback, s32 arg);

/* 0x80526980 - runs the two-channel level follower over `count` sample pairs; `silent` reports a gated block. */
s32 PMICFilterSamples(s16* left, s16* right, s32 count, s32 reset, s32* silent);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PMIC_PMIC_H */
