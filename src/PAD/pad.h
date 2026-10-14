/*
 * PAD/pad.h - declarations of the symbols owned by `PAD/pad.c` that other units call or read.
 */
#ifndef PAD_PAD_H
#define PAD_PAD_H

#include "types.h"
#include "PAD/PADClamp.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x804D8B20 (0x108): queues the controllers in `mask` for a reset; always TRUE. */
BOOL PADReset(u32 mask);

/* 0x804D8C30 (0x10C): queues the controllers in `mask` for a reset with recalibration; always TRUE. */
BOOL PADRecalibrate(u32 mask);

/* 0x804D8D40 (0x158): initialises the PAD library once and resets all four controllers. */
BOOL PADInit(void);

/* 0x804D8EA0 (0x4C0): reads the four controllers into `status`; returns the channel bits of rumble-capable pads. */
u32 PADRead(PADStatus* status);

/* 0x804D9AE0 (0x5C): turns controller recalibration off (returns the previous setting) or restores it. */
BOOL __PADDisableRecalibration(BOOL disable);

/* 0x80795418 - the pad specification word the boot information block supplies. */
extern s32 __PADSpec;

#ifdef __cplusplus
}
#endif

#endif
