/*
 * AX/AXProf.h - the AX profile record and the declarations of the symbols owned by `AX/AXProf.c`.
 */
#ifndef AX_AXPROF_H
#define AX_AXPROF_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* size: 0x38; the timestamps and voice count of one AX frame. */
typedef struct AXProfile {
    /* +0x00 */ s64 frameStart;
    /* +0x08 */ s64 auxStart;
    /* +0x10 */ s64 auxEnd;
    /* +0x18 */ s64 userCallbackStart;
    /* +0x20 */ s64 userCallbackEnd;
    /* +0x28 */ s64 frameEnd;
    /* +0x30 */ u32 numVoices;
    /* +0x34 */ u8 pad_0x34[4];
} AXProfile;

AXProfile* __AXGetCurrentProfile(void);

#ifdef __cplusplus
}
#endif

#endif
