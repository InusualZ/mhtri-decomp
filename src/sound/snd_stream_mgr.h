/* sound/snd_stream_mgr.h - the declarations `sound/snd_stream_mgr.cpp` owns that other units call (docs/plan.md 6.5
 * rule 2).  The signatures are the ones the callers use: they reach the manager as the `StreamWork` record of
 * `sound/fn_800E46E8_types.h`, which is the `StreamMgr` class the unit defines. */
#ifndef MHTRI_SOUND_SND_STREAM_MGR_H
#define MHTRI_SOUND_SND_STREAM_MGR_H

#include "types.h"

struct StreamWork;

#ifdef __cplusplus
extern "C" {
#endif
u8 stream_rand(void);
void stream_buffers_setup(StreamWork* work, u32 base, u32 size, u32 count);
void stream_play(StreamWork* work, u32 slot, u32 cmd_id, u32 paused);
void stream_play_cmd(StreamWork* work, u32 slot, u32 cmd, u32 paused);
u32 stream_status(StreamWork* work, u32 id);
extern StreamWork g_stream_mgr; /* the stream manager */
#ifdef __cplusplus
}
#endif

#endif
