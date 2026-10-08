/*
 * PAD/pad.h - declarations of the symbols owned by `PAD/pad.c` that other units call or read.
 */
#ifndef PAD_PAD_H
#define PAD_PAD_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x804D9AE0 - turns controller recalibration off (returns the previous setting) or restores it. */
BOOL __PADDisableRecalibration(BOOL disable);

/* 0x80795418 - the pad specification word the boot information block supplies. */
extern s32 __PADSpec;

#ifdef __cplusplus
}
#endif

#endif
