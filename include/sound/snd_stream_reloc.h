/* sound/snd_stream_reloc.h - the declarations `sound/snd_stream_reloc.cpp` owns that other units call (docs/plan.md 6.5 rule 2).  The signatures are the ones the callers use;
 * a function the unit has not written yet is declared from its callers. */
#ifndef MHTRI_SOUND_SND_STREAM_RELOC_H
#define MHTRI_SOUND_SND_STREAM_RELOC_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif
s16 fn_800E852C(void);
s16 fn_800E8594(u32 idx);
u32 snd_handle_get(void);
s32 fn_800E885C(void);
/* 0x800E89E0 - records whether the network transfer mode is 1 (GUESS name: the network transfer-mode switch
 * is the caller). */
void setStreamTransferMode(u32 mode);
/* 0x800E8D40 - resets the reverb work area (its 0x8000-byte buffer at 0x90003F60 and the cursor words). */
void clearReverbWorkArea(void);
#ifdef __cplusplus
}
#endif

#endif
