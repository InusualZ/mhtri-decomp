/*
 * AX/AXSPB.h - the AX studio fade types and the declarations of the symbols owned by `AX/AXSPB.c`.
 */
#ifndef AX_AXSPB_H
#define AX_AXSPB_H

#include "types.h"

#include "AX/AXVPB.h"

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(push, 2)

/* size: 0x6; one depop fade record: the level handed to the DSP and its per-frame step. */
typedef struct AXStudioFade {
    /* +0x0 */ s32 level;
    /* +0x4 */ s16 step;
} AXStudioFade;

/* size: 0x78; the studio depop block the DSP reads: twelve main records then eight remote records. */
typedef struct AXStudio {
    /* +0x00 */ AXStudioFade main[12];
    /* +0x48 */ AXStudioFade rmt[8];
} AXStudio;

#pragma pack(pop)

AXStudio* __AXGetStudio(void);
void __AXPrintStudio(void);
void __AXSPBInit(void);
void __AXSPBQuit(void);
void __AXDepopVoice(AXPB* pb);

#ifdef __cplusplus
}
#endif

#endif
