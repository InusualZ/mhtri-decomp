/* sound/quest_snd.h - the types and declarations `sound/quest_snd.cpp` owns that other units use (docs/plan.md 6.5
 * rules 1-2): the wave and stream-format records its AX voice setup reads, and the voice setup itself. */
#ifndef MHTRI_SOUND_QUEST_SND_H
#define MHTRI_SOUND_QUEST_SND_H

#include "types.h"
#include "AX/AXVPB.h"

struct AxHandle;
struct AxVoice;

/* The ADPCM decoder state an AX voice's PB carries at `pb.adpcm` (the copy `snd_adpcm_copy` performs is a u16-only
 * struct copy: eight words, then four half-words). */
typedef struct SndAdpcm {
    /* +0x00 */ u16 coefs[16];
    /* +0x20 */ u16 gain;
    /* +0x22 */ u16 pred_scale;
    /* +0x24 */ u16 yn1;
    /* +0x26 */ u16 yn2;
} SndAdpcm; /* size: 0x28 */

/* A sample's ADPCM decoder start state and its loop context. */
typedef struct SndAdpcmInfo {
    /* +0x00 */ SndAdpcm adpcm;
    /* +0x28 */ AXPBAdpcmLoop loop;
} SndAdpcmInfo; /* size: 0x2E */

/* One wave `snd_voice_prepare` starts on a voice; `format` is 0/1 ADPCM, 2/3 PCM16, 4/5 PCM8 (odd = looped). */
typedef struct SndWaveEntry {
    /* +0x00 */ u32 format;
    /* +0x04 */ u32 sample_rate;
    /* +0x08 */ u32 loop_addr;
    /* +0x0C */ u32 loop_end_addr;
    /* +0x10 */ u32 end_addr;
    /* +0x14 */ u32 current_addr;
    /* +0x18 */ SndAdpcmInfo adpcm;
    /* +0x46 */ u8 pad_0x46[2];
} SndWaveEntry; /* size: 0x48 (approximate: the end pad is not read) */

/* One channel of a stream format: its loop points and its ADPCM state. */
typedef struct StreamChannel {
    /* +0x00 */ u32 loop_start;
    /* +0x04 */ u32 loop_end;
    /* +0x08 */ u8 pad_0x08[8];
    /* +0x10 */ SndAdpcmInfo adpcm;
    /* +0x3E */ u8 pad_0x3E[2];
} StreamChannel; /* size: 0x40 */

/* A stream file's format header: `codec` is 0/1 ADPCM, 2/3 PCM16, 4/5 PCM8, then one record per channel. */
struct StreamFormat {
    /* +0x00 */ s32 mono;
    /* +0x04 */ u32 codec;
    /* +0x08 */ u32 sample_rate;
    /* +0x0C */ u8 pad_0x0C[4];
    /* +0x10 */ StreamChannel channel[2];
}; /* size: 0x90 */

#ifdef __cplusplus
extern "C" {
#endif

void snd_voice_prepare(AxHandle* wave, AxVoice* voice, s16 pitch, u16 lpf_freq);
void snd_adpcm_loop_copy(AXPBAdpcmLoop* dst, AXPBAdpcmLoop* src);
void snd_adpcm_copy(SndAdpcm* dst, SndAdpcm* src);
void snd_stream_voice_prepare(StreamFormat* format, AXVPB* vpb, StreamChannel* channel, u32 ratio);

#ifdef __cplusplus
}
#endif

#endif
