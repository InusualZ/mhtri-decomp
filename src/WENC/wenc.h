/*
 * WENC/wenc.h - the speaker sample encoder of `WENC/wenc.cpp` and its state record.
 */
#ifndef WENC_WENC_H
#define WENC_WENC_H

#include "types.h"

/* size: 0x18 - the encoder state carried from one call to the next: the predictor and the quantiser step. */
typedef struct WENCInfo {
    /* +0x00 */ s32 predictor;
    /* +0x04 */ s32 step;
    /* +0x08 */ s32 delta;
    /* +0x0C */ s32 remainder;
    /* +0x10 */ s32 half;
    /* +0x14 */ s32 quarter;
} WENCInfo; /* size: 0x18 */

#ifdef __cplusplus
extern "C" {
#endif

/* 0x804EB22C - encodes `sampleCount` 16-bit samples as 4-bit codes (two per byte) into `out`; bit 0 of `flags` keeps `info`'s state. */
s32 WENCGetEncodeData(WENCInfo* info, u32 flags, const s16* samples, s32 sampleCount, u8* out);

#ifdef __cplusplus
}
#endif

#endif /* WENC_WENC_H */
