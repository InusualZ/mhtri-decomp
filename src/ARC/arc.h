/*
 * ARC/arc.h - declarations of the symbols owned by `ARC/arc.cpp` that other units call or read.
 */
#ifndef ARC_ARC_H
#define ARC_ARC_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

extern u32 fn_8046FD80(void);

/* The DSP-ADPCM helpers the mediator's voice path uses (after `__AXGetCurrentProfile`, 0x80471960):
 *  - 0x804719A0 - the bytes `samples` PCM samples take once encoded: 8 per 14-sample frame, rounded up
 *    (0 for a negative count) - the dsptool `getBytesForAdpcmSamples` formula, hence the name (a GUESS);
 *  - 0x804719E0 - encodes `samples` 16-bit samples at `sampleRate` into `dst`, filling the 0x60-byte coefficient
 *    record `info` (with the nibble addresses of the loop, when 0 <= loopStart < loopEnd < samples) through the
 *    scratch `work`; it returns `samples` (0 for a null pointer or a bad count) after flushing the encoded
 *    bytes from the data cache.  NAME (a GUESS from the body). */
s32 getBytesForAdpcmSamples(s32 samples);
s32 encodeAdpcmSamples(const s16* src, s32 samples, s32 sampleRate, s32 loopStart, s32 loopEnd, u8* dst, u8* info,
                       u8* work);

#ifdef __cplusplus
}
#endif

#endif
