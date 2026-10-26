/* sound/snd_format_voice_init.h - leaf: the stream-format helpers `sound/quest_snd.cpp` defines (docs/plan.md 6.5
 * rule 2), with the signatures `sound/snd_stream_mgr.cpp` calls them through. */
#ifndef MHTRI_SOUND_SND_FORMAT_VOICE_INIT_H
#define MHTRI_SOUND_SND_FORMAT_VOICE_INIT_H

#include "types.h"

#ifdef __cplusplus
struct StreamFormat;
struct AXVPB;

extern "C" {
#endif

void snd_format_voice_init(StreamFormat* format, AXVPB* l, AXVPB* r, s16 pitch);
u32 snd_format_bytes_to_addr(StreamFormat* format, u32 bytes);
u32 snd_format_addr_to_bytes(StreamFormat* format, u32 addr);
u32 snd_format_samples_to_bytes(StreamFormat* format, u32 samples);
u32 snd_format_samples_to_addr(StreamFormat* format, u32 samples);

#ifdef __cplusplus
}
#endif

#endif
