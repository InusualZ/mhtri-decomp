/* sound/snd_stream_mgr.h - the declarations `sound/snd_stream_mgr.cpp` owns that other units call (docs/plan.md 6.5 rule 2).  The signatures are the ones the callers use;
 * a function the unit has not written yet is declared from its callers. */
#ifndef MHTRI_SOUND_SND_STREAM_MGR_H
#define MHTRI_SOUND_SND_STREAM_MGR_H

#include "types.h"

struct StreamWork;

#ifdef __cplusplus
extern "C" {
#endif
u32 fn_800E9D00(void);
void fn_800EBAE8(StreamWork* work, u32 a, u32 b, u32 c);
void fn_800EBC24(StreamWork* work, u32 a, u32 b, u32 c);
void fn_800EBDDC(StreamWork* work, u32 a, u32 b, u32 c);
u32 fn_800ED5B4(StreamWork* work, u32 id);
extern StreamWork lbl_8069A810; /* the stream manager */
#ifdef __cplusplus
}
#endif

#endif
