/* sound/stream_mgr_get.h - leaf: the stream-manager accessors `sound/fn_800E46E8.cpp` defines (docs/plan.md 6.5
 * rule 2), with the signatures `sound/snd_stream_mgr.cpp` calls them through. */
#ifndef MHTRI_SOUND_STREAM_MGR_GET_H
#define MHTRI_SOUND_STREAM_MGR_GET_H

#include "types.h"

#ifdef __cplusplus
class StreamMgr;

extern "C" {
#endif

StreamMgr* stream_mgr_get(void);
u32 stream_bus_dirty_ck(u8 slot);
f32 stream_bus_volume_get(u8 slot);
s16 stream_master_volume_get(void);
s16 stream_bank_volume_get(u32 idx);

#ifdef __cplusplus
}
#endif

#endif
