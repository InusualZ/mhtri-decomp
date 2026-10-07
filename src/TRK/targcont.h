/*
 * TRK/targcont.h - the MetroTRK "resume the target" entry, owned by `TRK/targcont.c`.
 */
#ifndef TRK_TARGCONT_H
#define TRK_TARGCONT_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80469F10 (0x34): resumes the stopped target program. */
s32 TRKTargetContinue(void);

/* 0x80469F44 (0x1C4): stores the segment, time base, HID, BAT and SPR registers into gTRKCPUState. */
void TRKSaveExtended1Block(void);

/* 0x8046A108 (0x164): writes those registers back from gTRKCPUState. */
void TRKRestoreExtended1Block(void);

#ifdef __cplusplus
}
#endif

#endif
