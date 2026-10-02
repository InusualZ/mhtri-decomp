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
#ifdef __cplusplus
}
#endif

#endif
