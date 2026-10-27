/* sound/stream_cmd_copy.h - leaf: `sound/snd_stream_mgr.cpp`'s `stream_cmd_copy` (0x800EBE60), which copies a stream command. */
#ifndef MHTRI_SOUND_STREAM_CMD_COPY_H
#define MHTRI_SOUND_STREAM_CMD_COPY_H

#include "types.h"
#include "sound/snd_stream_mgr_types.h"

#ifdef __cplusplus
extern "C" {
#endif

void stream_cmd_copy(StreamCmd* dst, StreamCmd* src);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_SOUND_STREAM_CMD_COPY_H */
