/*
 * DSPADPCM/dspadpcm.c - the DSP-ADPCM sample encoder: the encoded size formula, the encoder entry, the LPC
 *    coefficient search (autocorrelation, Levinson-Durbin, a covariance solve and a codebook split) and the
 *    per-frame quantiser.
 *
 * RANGE. .text 0x804719A0-0x80474CB0 (11 functions, 0x3310 B); .sdata2 0x8079CF68-0x8079CFF0.  Cut from the old
 *    ARC/AX block at 0x804719A0 (after `__AXGetCurrentProfile`); the right edge is the start of
 *    `AX/AXFXReverbHi.c` (0x80474CB0).
 * FLAGS. the `OS` lib's group with `cflags_base` (configure.py `cflags=`): every function start of the band is
 *    16-aligned through `#pragma function_align 16` (redundant: the object is byte-identical without it under
 *    `cflags_base`), not the 4-byte packing `cflags_os` gives; `#pragma fp_contract off`
 *    (retail keeps `fmul`+`fsub`: 0 fused of 320 fp ops); `__fabs`/`__abs` are the compiler intrinsics.
 * NAMES. GUESS: getBytesForAdpcmSamples, encodeAdpcmSamples (map rows named from the dsptool formula and the
 *    body), encodeAdpcmFrames, encodeAdpcmFrame, computeAdpcmCoefs, refineAdpcmCodebook, levinsonDurbin2,
 *    adpcmReflection2, adpcmCovariance2, adpcmLuDecompose2, adpcmLuSolve2 (from what each body computes);
 *    the library name `DSPADPCM` and the file name `dspadpcm.c` are GUESSES.
 * RESIDUALS. encodeAdpcmFrame: retail reloads the coefficient pair inside the 14-sample loops and indexes
 *    `pcm[i + 1]`/`out[i + 1]` as `(i + 1) * 2` (ours hoists the pair and strength-reduces to pointers), and
 *    spills the best-vector index to 0xD8(r1) (frame 0x130 against our 0x120).  refineAdpcmCodebook: register
 *    numbering and an 8-byte shorter body (retail saves r18..r31, ours r19..r31; the step-up reuses one
 *    `&a[1]` base).
 *    computeAdpcmCoefs: the codebook split indexes `(3 * i + 1) * 8` per element where ours strength-reduces to
 *    pointer offsets, the final coefficient loop keeps `coefs + 4 * i` in a scratch register, and the prologue
 *    copies `work` before `src`.  adpcmLuDecompose2: the row swap indexes each element (`(3 * m + y) * 8`) where
 *    ours strength-reduces to `lfdux`.  encodeAdpcmSamples: the `/ 14` quotients of the nibble counts are
 *    scheduled in the other order, and the two loop nibble addresses' register allocation.
 *    .text 0x3290 against the claimed 0x3310: encodeAdpcmFrame -0x44, computeAdpcmCoefs -0x1C, refineAdpcmCodebook
 *    -0x8; the in-object 16-byte alignment gaps shrink by 0x18.
 * SHAPES. the 3x3 matrices are flat `f64` arrays used 1-based (row and column 0 unused) with stride 3, walked by
 *    explicit running indices (`row`, `k`, `idx`) where retail's code keeps integer index registers; the order-2
 *    step-down in adpcmReflection2 is the generic loop `for (i = 1; i > 0; i--)`, which keeps retail's counted
 *    loops; adpcmCovariance2 takes a plain `s16*` (a `const` pointer lets the loads move above the stores).
 */

#pragma function_align 16
#pragma fp_contract off

#include "types.h"
#include "DSPADPCM/dspadpcm.h"
#include "OS/OSCache.h"

/* The 0x60-byte record the encoder fills for the sample header. */
typedef struct DspAdpcmInfo {
    /* +0x00 */ s32 sample_count;
    /* +0x04 */ s32 nibble_count;
    /* +0x08 */ s32 sample_rate;
    /* +0x0C */ s16 loop_flag;
    /* +0x0E */ s16 format;
    /* +0x10 */ s32 loop_start;
    /* +0x14 */ s32 loop_end;
    /* +0x18 */ s32 current_address;
    /* +0x1C */ s16 coef[8][2];
    /* +0x3C */ s16 gain;
    /* +0x3E */ s16 initial_scale;
    /* +0x40 */ s16 history1;
    /* +0x42 */ s16 history2;
    /* +0x44 */ s16 loop_scale;
    /* +0x46 */ u16 loop_history1;
    /* +0x48 */ u16 loop_history2;
    /* +0x4A */ u8 pad_0x4A[0x16];
} DspAdpcmInfo; /* size: 0x60 */

/* The predictor state at the loop start, captured while its frame is encoded. */
typedef struct AdpcmLoopState {
    /* +0x00 */ u8 scale;
    /* +0x02 */ s16 history1;
    /* +0x04 */ s16 history2;
} AdpcmLoopState; /* size: 0x6 */

/* Keeps both order-2 reflection coefficients strictly inside (-1, 1). */
static inline void clampReflections(f64* k)
{
    s32 i;

    for (i = 1; i <= 2; i++) {
        if (k[i] >= 1.0) {
            k[i] = 0.9999999999;
        }
        if (k[i] <= -1.0) {
            k[i] = -0.9999999999;
        }
    }
}

/* Steps up from the reflection coefficients `k` to the predictor `a`. */
static inline void predictorFromReflections(f64* a, f64* k, s32 order)
{
    s32 i;
    s32 j;

    a[0] = 1.0;
    for (i = 1; i <= order; i++) {
        a[i] = k[i];
        for (j = 1; j < i; j++) {
            a[j] += a[i] * a[i - j];
        }
    }
}

/* Stores the negated autocorrelation (lags 0..2) of the 14 samples at `pcm`, reaching back into the previous frame. */
static inline void autocorrelate(f64* out, s16* pcm)
{
    s32 i;
    s32 x;

    for (i = 0; i <= 2; i++) {
        out[i] = 0.0;
        for (x = 0; x < 14; x++) {
            out[i] -= pcm[x] * pcm[x - i];
        }
    }
}

/* Computes the autocorrelation (lags 0..2) of the order-2 predictor `src` by the step-down recursion. */
static inline void autocorrFromPredictor(f64* src, f64* dst)
{
    f64 mtx[3][3];
    s32 i;

    mtx[2][0] = 1.0;
    for (i = 1; i <= 2; i++) {
        mtx[2][i] = -src[i];
    }
    mtx[1][1] = (mtx[2][1] + mtx[2][2] * mtx[2][1]) / (1.0 - mtx[2][2] * mtx[2][2]);
    dst[0] = 1.0;
    dst[1] = mtx[1][1] * dst[0];
    dst[2] = mtx[2][1] * dst[1] + mtx[2][2] * dst[0];
}

s32 encodeAdpcmFrames(DspAdpcmInfo* info, s32 vecCount, const s16* src, s32 samples, s16 looped, s32 loopStart,
                      u8* dst, AdpcmLoopState* loopState);
void encodeAdpcmFrame(s16 (*coef)[2], s32 vecCount, s16* pcm, s32 loopIdx, u8* dst, AdpcmLoopState* loopState);
s32 computeAdpcmCoefs(const s16* src, s32 samples, DspAdpcmInfo* info, f64* work);
void refineAdpcmCodebook(f64* codebook, s32 count, f64* frames, s32 frameCount);
void levinsonDurbin2(f64* r, f64* k, f64* a);
s32 adpcmReflection2(f64* a, f64* k);
void adpcmCovariance2(s16* x, s32 n, f64* mat);
s32 adpcmLuDecompose2(f64* a, s32* index);
void adpcmLuSolve2(f64* a, s32* index, f64* b);

/* 0x804719A0 - the encoded size of `samples` PCM samples: 8 bytes per 14-sample frame, rounded up. */
s32 getBytesForAdpcmSamples(s32 samples)
{
    if (samples < 0) {
        return 0;
    }
    return (samples + 13) / 14 * 8;
}

/* The nibble address of sample `sample`: 16 nibbles per 14-sample frame, past the 2-nibble frame header. */
static inline u32 nibbleAddress(u32 sample)
{
    u32 frames = sample / 14;
    u32 rest = sample % 14;
    u32 nibble = frames * 16 + 2;

    nibble += rest;
    return nibble;
}

/* Encodes `samples` 16-bit samples into `dst` and fills the sample header record `info`. */
s32 encodeAdpcmSamples(const s16* src, s32 samples, s32 sampleRate, s32 loopStart, s32 loopEnd, u8* dst, u8* info,
                       u8* work)
{
    DspAdpcmInfo* header;
    AdpcmLoopState loopState;
    s16 looped;
    s32 loopStartNibble;
    s32 loopEndNibble;
    s32 nibbles;
    s32 vecCount;

    if (src == 0 || dst == 0 || info == 0 || work == 0) {
        return 0;
    }
    if (samples < 0 || sampleRate <= 0) {
        return 0;
    }
    looped = 0;
    if (loopStart >= 0 && loopEnd > 0 && loopEnd > loopStart && loopEnd < samples) {
        looped = 1;
        loopStartNibble = nibbleAddress(loopStart);
        loopEndNibble = nibbleAddress(loopEnd);
    }
    header = (DspAdpcmInfo*)info;
    vecCount = computeAdpcmCoefs(src, samples, header, (f64*)work);
    encodeAdpcmFrames(header, vecCount, src, samples, looped, loopStart, dst, &loopState);
    nibbles = samples / 14 * 16;
    if (samples % 14 != 0) {
        nibbles = nibbles + samples % 14 + 2;
    }
    header->sample_count = samples;
    header->nibble_count = nibbles;
    header->sample_rate = sampleRate;
    header->format = 0;
    header->gain = 0;
    header->initial_scale = dst[0];
    header->history1 = 0;
    header->history2 = 0;
    header->loop_flag = looped;
    if (looped == 0) {
        header->loop_start = 2;
        header->loop_end = nibbles - 1;
        header->current_address = 2;
        header->loop_scale = 0;
        header->loop_history1 = 0;
        header->loop_history2 = 0;
    } else {
        header->loop_start = loopStartNibble;
        header->loop_end = loopEndNibble;
        header->current_address = 2;
        header->loop_scale = loopState.scale;
        header->loop_history1 = loopState.history1;
        header->loop_history2 = loopState.history2;
    }
    DCFlushRange(dst, nibbles / 14 * 8);
    return samples;
}

/* Cuts the sample stream into 14-sample frames and runs the frame quantiser over each. */
s32 encodeAdpcmFrames(DspAdpcmInfo* info, s32 vecCount, const s16* src, s32 samples, s16 looped, s32 loopStart,
                      u8* dst, AdpcmLoopState* loopState)
{
    s32 pos;
    s16 (*coef)[2] = info->coef;
    s16 pcm[16];
    s32 n;
    s32 i;

    pcm[14] = 0;
    pcm[15] = 0;
    pos = 0;
    while (samples > 0) {
        pcm[0] = pcm[14];
        pcm[1] = pcm[15];
        n = samples < 14 ? samples : 14;
        samples -= n;
        for (i = 0; i < n; i++) {
            pcm[2 + i] = *src++;
        }
        for (; i < 14; i++) {
            pcm[2 + i] = 0;
        }
        if (looped != 0 && loopStart < 14) {
            encodeAdpcmFrame(coef, vecCount, pcm, loopStart, dst, loopState);
            looped = 0;
        } else {
            encodeAdpcmFrame(coef, vecCount, pcm, -1, dst, 0);
        }
        dst += 8;
        pos += 14;
        if (looped != 0) {
            loopStart -= 14;
        }
    }
    return pos;
}

/* Quantises one frame against every coefficient pair and keeps the encoding with the least squared error. */
void encodeAdpcmFrame(s16 (*coef)[2], s32 vecCount, s16* pcm, s32 loopIdx, u8* dst, AdpcmLoopState* loopState)
{
    s16 out[16];
    s16 bestOut[16];
    s32 nib[14];
    s32 bestNib[14];
    s32 bestScale;
    s32 bestVec;
    f64 minErr;
    s32 v;
    s32 i;

    bestScale = 0;
    bestVec = 0;
    out[0] = pcm[0];
    out[1] = pcm[1];
    minErr = 1e30;
    for (v = 0; v < vecCount; v++) {
        s32 maxRes = 0;
        s32 scale;
        s32 over;
        f64 err;
        s32 limit;

        for (i = 0; i < 14; i++) {
            s32 r = ((pcm[i + 2] << 11) - (pcm[i + 1] * coef[v][0] + pcm[i] * coef[v][1])) / 2048;

            if (r > 32767) {
                r = 32767;
            } else if (r < -32768) {
                r = -32768;
            }
            if (__abs(r) > __abs(maxRes)) {
                maxRes = r;
            }
        }
        scale = 0;
        for (limit = 0; limit < 13; limit++) {
            if ((u32)(maxRes + 8) <= 15) {
                break;
            }
            scale++;
            maxRes /= 2;
        }
        scale -= 2;
        if (scale < -1) {
            scale = -1;
        }
        do {
            s32 factor;

            scale++;
            factor = 1 << scale;
            over = 0;
            err = 0.0;
            for (i = 0; i < 14; i++) {
                s32 sample = pcm[i + 2];
                s32 pred = out[i + 1] * coef[v][0] + out[i] * coef[v][1];
                s32 diff = (sample << 11) - pred;
                f64 q = (f64)diff / (f64)(factor << 11);
                s32 q4;
                s32 recon;
                s32 e;

                q4 = (s32)(q - 0.4999999f);
                if (diff > 0) {
                    q4 = (s32)(0.4999999f + q);
                }
                if (q4 < -8) {
                    if (over < -8 - q4) {
                        over = -8 - q4;
                    }
                    q4 = -8;
                }
                if (q4 > 7) {
                    if (over < q4 - 7) {
                        over = q4 - 7;
                    }
                    q4 = 7;
                }
                nib[i] = q4;
                recon = (pred + ((q4 * factor) << 11) + 0x400) >> 11;
                if (recon > 32767) {
                    recon = 32767;
                } else if (recon < -32768) {
                    recon = -32768;
                }
                out[i + 2] = recon;
                e = sample - recon;
                err += (f64)(e * e);
            }
            limit = over + 8;
            while (limit > 256) {
                scale++;
                limit >>= 1;
                if (scale >= 12) {
                    scale = 11;
                    break;
                }
            }
        } while (scale < 12 && over > 1);
        if (err < minErr) {
            bestScale = scale;
            minErr = err;
            for (i = 0; i < 14; i++) {
                bestNib[i] = nib[i];
            }
            for (i = 0; i < 16; i++) {
                bestOut[i] = out[i];
            }
            bestVec = v;
        }
    }
    pcm[14] = bestOut[14];
    pcm[15] = bestOut[15];
    dst[0] = (bestVec << 4) | (bestScale & 0xF);
    for (i = 0; i < 7; i++) {
        dst[1 + i] = (bestNib[2 * i] << 4) | (bestNib[2 * i + 1] & 0xF);
    }
    if (loopState != 0) {
        loopState->scale = dst[0];
        loopState->history1 = bestOut[loopIdx + 1];
        loopState->history2 = bestOut[loopIdx];
    }
}

/* Finds the 8 coefficient pairs of the sample and writes them, scaled by 2048, into the header; returns 8. */
s32 computeAdpcmCoefs(const s16* src, s32 samples, DspAdpcmInfo* info, f64* work)
{
    s16 (*coefs)[2] = info->coef;
    s16 buf[28];
    f64 r[3];
    f64 refl[3];
    f64 mat[9];
    s32 index[3];
    f64 book[24];
    f64* frame;
    s32 frames;
    s32 i;
    s32 j;
    s32 n;
    s32 round;
    f64* split;
    f64 step[3];

    for (i = 0; i < 28; i++) {
        buf[i] = 0;
    }
    frame = work;
    frames = 0;
    while (samples > 0) {
        for (i = 0; i < 14; i++) {
            buf[i] = buf[i + 14];
        }
        n = samples < 14 ? samples : 14;
        samples -= n;
        for (i = 0; i < n; i++) {
            buf[14 + i] = *src++;
        }
        for (; i < 14; i++) {
            buf[14 + i] = 0;
        }
        autocorrelate(r, &buf[14]);
        if (__fabs(r[0]) > 10.0f) {
            adpcmCovariance2(&buf[14], 14, mat);
            if (adpcmLuDecompose2(mat, index) == 0) {
                adpcmLuSolve2(mat, index, r);
                r[0] = 1.0;
                if (adpcmReflection2(r, refl) == 0) {
                    frame[0] = 1.0;
                    clampReflections(refl);
                    predictorFromReflections(frame, refl, 2);
                    frames++;
                    frame += 3;
                }
            }
        }
    }
    r[0] = 1.0;
    r[1] = 0.0;
    r[2] = 0.0;
    for (i = 0; i < frames; i++) {
        autocorrFromPredictor(&work[3 * i], book);
        for (j = 1; j <= 2; j++) {
            r[j] += book[j];
        }
    }
    r[1] /= (f64)frames;
    r[2] /= (f64)frames;
    levinsonDurbin2(r, refl, book);
    clampReflections(refl);
    predictorFromReflections(book, refl, 2);
    for (round = 0; round < 3;) {
        step[0] = 0.0;
        step[1] = -1.0;
        step[2] = 0.0;
        split = &book[3 * (1 << round)];
        for (i = 0; i < (1 << round); i++) {
            for (j = 0; j <= 2; j++) {
                split[3 * i + j] = book[3 * i + j] + 0.01f * step[j];
            }
        }
        round++;
        refineAdpcmCodebook(book, 1 << round, work, frames);
    }
    for (i = 0; i < (1 << round); i++) {
        f64 v = 2048.0 * (-1.0 * book[3 * i + 1]);

        if (v > 0.0) {
            if (v > 32767.0) {
                coefs[i][0] = 0x7FFF;
            } else {
                coefs[i][0] = (s16)(0.5 + v);
            }
        } else if (v < -32768.0) {
            coefs[i][0] = -0x8000;
        } else {
            coefs[i][0] = (s16)(v - 0.5);
        }
        v = 2048.0 * (-1.0 * book[3 * i + 2]);
        if (v > 0.0) {
            if (v > 32767.0) {
                coefs[i][1] = 0x7FFF;
            } else {
                coefs[i][1] = (s16)(0.5 + v);
            }
        } else if (v < -32768.0) {
            coefs[i][1] = -0x8000;
        } else {
            coefs[i][1] = (s16)(v - 0.5);
        }
    }
    return 1 << round;
}

/* Returns the weighted distance between the codebook predictor `pred` and the frame predictor `frame`. */
static inline f64 predictorDistance(f64* pred, f64* frame)
{
    f64 ac[3];
    f64 v1;
    f64 v2;
    f64 v3;

    autocorrFromPredictor(frame, ac);
    v1 = pred[0] * pred[0] + pred[1] * pred[1] + pred[2] * pred[2];
    v2 = pred[0] * pred[1] + pred[1] * pred[2];
    v3 = pred[0] * pred[2];
    return ac[0] * v1 + 2.0 * ac[1] * v2 + 2.0 * ac[2] * v3;
}

/* Runs two nearest-codebook / recentre passes over the frame vectors. */
void refineAdpcmCodebook(f64* codebook, s32 count, f64* frames, s32 frameCount)
{
    s32 members[8];
    f64 ac[3];
    f64 acc[8][3];
    s32 pass;
    s32 f;
    s32 c;
    s32 j;

    for (pass = 0; pass < 2; pass++) {
        for (c = 0; c < count; c++) {
            members[c] = 0;
            for (j = 0; j <= 2; j++) {
                acc[c][j] = 0.0;
            }
        }
        for (f = 0; f < frameCount; f++) {
            s32 bestIdx = 0;
            f64 best = 1e30;

            for (c = 0; c < count; c++) {
                f64 d = predictorDistance(&codebook[3 * c], &frames[3 * f]);

                if (d < best) {
                    best = d;
                    bestIdx = c;
                }
            }
            members[bestIdx]++;
            autocorrFromPredictor(&frames[3 * f], ac);
            for (j = 0; j <= 2; j++) {
                acc[bestIdx][j] += ac[j];
            }
        }
        for (c = 0; c < count; c++) {
            if (members[c] > 0) {
                for (j = 0; j <= 2; j++) {
                    acc[c][j] /= (f64)members[c];
                }
            }
        }
        for (c = 0; c < count; c++) {
            levinsonDurbin2(acc[c], ac, &codebook[3 * c]);
            clampReflections(ac);
            predictorFromReflections(&codebook[3 * c], ac, 2);
        }
    }
}

/* Solves the order-2 normal equations of the autocorrelation vector `r` for the predictor `a`. */
void levinsonDurbin2(f64* r, f64* k, f64* a)
{
    f64 div = r[0];
    s32 i;
    s32 j;

    a[0] = 1.0;
    for (i = 1; i <= 2; i++) {
        f64 sum = 0.0;

        for (j = 1; j < i; j++) {
            sum += a[j] * r[i - j];
        }
        if (div > 0.0) {
            a[i] = -(sum + r[i]) / div;
        } else {
            a[i] = 0.0;
        }
        k[i] = a[i];
        for (j = 1; j < i; j++) {
            a[j] += a[i] * a[i - j];
        }
        div *= 1.0 - a[i] * a[i];
    }
}

/* Converts the order-2 predictor `a` to reflection coefficients `k`; returns nonzero when unstable. */
s32 adpcmReflection2(f64* a, f64* k)
{
    f64 tmp[3];
    f64 den;
    s32 i;
    s32 j;
    s32 ret = 0;

    for (i = 1; i > 0; i--) {
        k[i + 1] = a[i + 1];
        den = 1.0 - k[i + 1] * k[i + 1];
        if (0.0 == den) {
            return 1;
        }
        for (j = 0; j <= i; j++) {
            tmp[j] = (a[j] - k[i + 1] * a[i + 1 - j]) / den;
        }
        for (j = 0; j <= i; j++) {
            a[j] = tmp[j];
        }
        k[i] = tmp[i];
        if (__fabs(k[i]) > 1.0) {
            ret = 1;
        }
    }
    return ret;
}

/* Accumulates the order-2 covariance matrix of `n` samples of `x`. */
void adpcmCovariance2(s16* x, s32 n, f64* mat)
{
    s32 i;
    s32 j;
    s32 t;
    s32 idx = 3;

    for (i = 1; i <= 2; i++) {
        idx++;
        for (j = 1; j <= 2; j++) {
            mat[idx] = 0.0;
            for (t = 0; t < n; t++) {
                mat[idx] += x[t - i] * x[t - j];
            }
            idx++;
        }
    }
}

/* LU-decomposes the order-2 matrix in place; returns nonzero when singular. */
s32 adpcmLuDecompose2(f64* a, s32* index)
{
    f64 recips[3];
    f64 val;
    f64 tmp;
    f64 min;
    f64 max;
    s32 i;
    s32 x;
    s32 y;
    s32 maxIndex;
    s32 k;
    s32 row;

    i = 3;
    for (x = 1; x <= 2; x++) {
        val = 0.0;
        i++;
        for (y = 1; y <= 2; y++) {
            tmp = __fabs(a[i]);
            if (tmp > val) {
                val = tmp;
            }
            i++;
        }
        if (0.0 == val) {
            return 1;
        }
        recips[x] = 1.0 / val;
    }
    for (i = 1; i <= 2; i++) {
        row = 3;
        for (x = 1; x < i; x++) {
            tmp = a[row + i];
            k = i + 3;
            for (y = 1; y < x; y++) {
                tmp -= a[row + y] * a[k];
                k += 3;
            }
            a[row + i] = tmp;
            row += 3;
        }
        val = 0.0;
        for (x = i; x <= 2; x++) {
            tmp = a[row + i];
            k = i + 3;
            for (y = 1; y < i; y++) {
                tmp -= a[row + y] * a[k];
                k += 3;
            }
            a[row + i] = tmp;
            if (recips[x] * __fabs(tmp) >= val) {
                val = recips[x] * __fabs(tmp);
                maxIndex = x;
            }
            row += 3;
        }
        if (i != maxIndex) {
            for (y = 1; y <= 2; y++) {
                tmp = a[maxIndex * 3 + y];
                a[maxIndex * 3 + y] = a[i * 3 + y];
                a[i * 3 + y] = tmp;
            }
            recips[maxIndex] = recips[i];
        }
        index[i] = maxIndex;
        if (0.0 == a[i * 3 + i]) {
            return 1;
        }
        if (i != 2) {
            tmp = 1.0 / a[i * 3 + i];
            k = (i + 1) * 3 + i;
            for (x = i + 1; x <= 2; x++) {
                a[k] *= tmp;
                k += 3;
            }
        }
    }
    min = 1e10;
    max = 0.0;
    for (i = 1; i <= 2; i++) {
        tmp = __fabs(a[i * 3 + i]);
        if (tmp < min) {
            min = tmp;
        }
        if (tmp > max) {
            max = tmp;
        }
    }
    return min / max < 1e-10;
}

/* Solves the LU-decomposed order-2 system for the right-hand side `b` in place. */
void adpcmLuSolve2(f64* a, s32* index, f64* b)
{
    s32 i;
    s32 j;
    s32 k;
    s32 ii = 0;
    s32 ip;
    f64 sum;

    k = 3;
    for (i = 1; i <= 2; i++) {
        ip = index[i];
        sum = b[ip];
        b[ip] = b[i];
        if (ii != 0) {
            for (j = ii; j <= i - 1; j++) {
                sum -= a[k + j] * b[j];
            }
        } else if (sum) {
            ii = i;
        }
        b[i] = sum;
        k += 3;
    }
    k = 6;
    for (i = 2; i >= 1; i--) {
        sum = b[i];
        for (j = i + 1; j <= 2; j++) {
            sum -= a[k + j] * b[j];
        }
        b[i] = sum / a[k + i];
        k -= 3;
    }
}
