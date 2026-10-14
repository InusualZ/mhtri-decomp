/*
 * SI/SISamplingRate.h - declarations of the symbols owned by `SI/SISamplingRate.c` that other units call.
 */
#ifndef SI_SISAMPLINGRATE_H
#define SI_SISAMPLINGRATE_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x804DEFB0 / 0x804DF090 - selects the polling rate (0..11 ms) and reapplies the current one. */
void SISetSamplingRate(u32 msec);
void SIRefreshSamplingRate(void);

#ifdef __cplusplus
}
#endif

#endif
