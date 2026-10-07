/*
 * DSPADPCM/dspadpcm.c - the DSP-ADPCM sample encoder: the encoded size formula, the encoder entry, the LPC
 *    coefficient search (autocorrelation, Levinson-Durbin, a covariance solve and a codebook split) and the
 *    per-frame quantiser.
 *
 * RANGE. .text 0x804719A0-0x80474CB0 (11 functions, 0x3310 B); .sdata2 0x8079CF68-0x8079CFF0.  Cut from the old
 *    ARC/AX block at 0x804719A0 (after `__AXGetCurrentProfile`); the right edge is the start of
 *    `AX/AXFXReverbHi.c` (0x80474CB0).
 * FLAGS. the `OS` lib's group with `cflags_base` (configure.py `cflags=`): every function start of the band is
 *    16-aligned (`-O4,p` default alignment), not the 4-byte packing `cflags_os` gives; `#pragma fp_contract off`
 *    (retail keeps `fmul`+`fsub`); `fabs` is the `__fabs` intrinsic and `abs` an inline bit-twiddle (the `abs`
 *    call of the runtime is not what retail emits).
 * NAMES. `getBytesForAdpcmSamples` and `encodeAdpcmSamples` are the map's names; GUESS: encodeAdpcmFrames,
 *    encodeAdpcmFrame, computeAdpcmCoefs, refineAdpcmCodebook, levinsonDurbin2, adpcmReflection2,
 *    adpcmCovariance2, adpcmLuDecompose2, adpcmLuSolve2 (from what each body computes); the library name
 *    `DSPADPCM` and the file name `dspadpcm.c` are GUESSES.
 * RESIDUALS. every body is plain C (no asm; the `psq_st` pairs in the large prologues are the compiler's
 *    float-register saves).  adpcmReflection2 (9 %): the order-2 loops fold completely here (retail keeps the
 *    counted loops with an unroll guard on a register-held trip count of 2), so ours is 0x6C B against 0x380 B;
 *    computeAdpcmCoefs / refineAdpcmCodebook: stack-array order and the step-up loops differ (ours 0x1058 B vs
 *    0xCD8 B); adpcmCovariance2 / adpcmLuDecompose2: the flat-index running counter retail uses is not
 *    reproduced; encodeAdpcmFrame: register numbering and the f64 rounding operand order; `.sdata2` 144 of 136 B
 *    (one extra 0.0 pool entry from the `(f32)0.0` compares); `.text` pad is linker-added.
 * SHAPES. the matrices and 3-vectors are 1-based flat `f64` arrays (index 0 unused) with stride `order + 1`;
 *    the order-2 bodies are generic `static inline` helpers called with the constant 2 (retail keeps the
 *    unrolled-by-8 guard code, which only the variable-order spelling reproduces).
 */

#pragma function_align 16
#pragma fp_contract off

#include "types.h"
#include "DSPADPCM/dspadpcm.h"
#include "TRK/TRK_flush_cache.h"

extern double __fabs(double x);
#define fabs(x) __fabs(x)

/* The absolute value of a 32-bit integer. */
static inline s32 abs(s32 x)
{
    s32 m = x >> 31;

    return (x ^ m) - m;
}

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
    /* +0x1C */ s16 coef[16];
    /* +0x3C */ s16 gain;
    /* +0x3E */ s16 initial_scale;
    /* +0x40 */ s16 history1;
    /* +0x42 */ s16 history2;
    /* +0x44 */ s16 loop_scale;
    /* +0x46 */ s16 loop_history1;
    /* +0x48 */ s16 loop_history2;
    /* +0x4A */ u8 pad_0x4A[0x16];
} DspAdpcmInfo; /* size: 0x60 */

/* Keeps both order-2 reflection coefficients strictly inside (-1, 1). */
static inline void clampReflections(f64* k)
{
    if (k[1] >= 1.0) {
        k[1] = 0.9999999999;
    }
    if (k[1] <= -1.0) {
        k[1] = -0.9999999999;
    }
    if (k[2] >= 1.0) {
        k[2] = 0.9999999999;
    }
    if (k[2] <= -1.0) {
        k[2] = -0.9999999999;
    }
}

/* Step-up from the reflection coefficients `k` to the predictor `a`. */
static inline void predictorFromReflections(f64* a, f64* k, s32 order)
{
    s32 i;
    s32 j;

    a[0] = 1.0;
    for (i = 1; i <= order; i++) {
        a[i] = k[i];
        for (j = 1; j < i; j++) {
            a[j] += k[i] * a[i - j];
        }
    }
}

s32 encodeAdpcmFrames(DspAdpcmInfo* info, s32 vecCount, const s16* src, s32 samples, s16 looped, s32 loopStart,
                      u8* dst, u8* loopState);
void encodeAdpcmFrame(s16* coef, s32 vecCount, s16* pcm, s32 loopIdx, u8* dst, u8* loopState);
s32 computeAdpcmCoefs(const s16* src, s32 samples, DspAdpcmInfo* info, f64* work);
void refineAdpcmCodebook(f64* codebook, s32 count, f64* frames, s32 frameCount);
void levinsonDurbin2(f64* r, f64* k, f64* a);
s32 adpcmReflection2(f64* a, f64* k);
void adpcmCovariance2(const s16* x, s32 n, f64* mat);
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

/* Encodes `samples` 16-bit samples into `dst` and fills the sample header record `info`. */
s32 encodeAdpcmSamples(const s16* src, s32 samples, s32 sampleRate, s32 loopStart, s32 loopEnd, u8* dst, u8* info,
                       u8* work)
{
    DspAdpcmInfo* header = (DspAdpcmInfo*)info;
    u8 loopState[6];
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
        loopStartNibble = (u32)loopStart / 14 * 16 + 2 + (u32)loopStart % 14;
        loopEndNibble = (u32)loopEnd / 14 * 16 + 2 + (u32)loopEnd % 14;
    }
    vecCount = computeAdpcmCoefs(src, samples, header, (f64*)work);
    encodeAdpcmFrames(header, vecCount, src, samples, looped, loopStart, dst, loopState);
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
        header->loop_scale = loopState[0];
        header->loop_history1 = *(u16*)&loopState[2];
        header->loop_history2 = *(u16*)&loopState[4];
    }
    DCFlushRange(dst, nibbles / 14 * 8);
    return samples;
}

/* Cuts the sample stream into 14-sample frames and runs the frame quantiser over each. */
s32 encodeAdpcmFrames(DspAdpcmInfo* info, s32 vecCount, const s16* src, s32 samples, s16 looped, s32 loopStart,
                      u8* dst, u8* loopState)
{
    s16 pcm[16];
    s32 pos;
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
            encodeAdpcmFrame(info->coef, vecCount, pcm, loopStart, dst, loopState);
            looped = 0;
        } else {
            encodeAdpcmFrame(info->coef, vecCount, pcm, -1, dst, 0);
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
void encodeAdpcmFrame(s16* coef, s32 vecCount, s16* pcm, s32 loopIdx, u8* dst, u8* loopState)
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
        s16* c = &coef[v * 2];
        s32 maxRes = 0;
        s32 scale;
        s32 over;
        f64 err;
        s32 limit;

        for (i = 0; i < 14; i++) {
            s32 r = ((pcm[i + 2] << 11) - (pcm[i + 1] * c[0] + pcm[i] * c[1])) / 2048;

            if (r > 32767) {
                r = 32767;
            } else if (r < -32768) {
                r = -32768;
            }
            if (abs(r) > abs(maxRes)) {
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
                s32 pred = out[i + 1] * c[0] + out[i] * c[1];
                s32 diff = (pcm[i + 2] << 11) - pred;
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
                e = pcm[i + 2] - recon;
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
        if ((f32)err < (f32)minErr) {
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
    dst[0] = (bestScale & 0xF) | (bestVec << 4);
    for (i = 0; i < 7; i++) {
        dst[1 + i] = (bestNib[2 * i + 1] & 0xF) | ((bestNib[2 * i] << 4) & 0xF0);
    }
    if (loopState != 0) {
        loopState[0] = dst[0];
        *(s16*)&loopState[2] = bestOut[loopIdx + 1];
        *(s16*)&loopState[4] = bestOut[loopIdx];
    }
}

/* Finds the 8 coefficient pairs of the sample and writes them, scaled by 2048, into the header; returns 8. */
s32 computeAdpcmCoefs(const s16* src, s32 samples, DspAdpcmInfo* info, f64* work)
{
    s16 buf[28];
    f64 r[3];
    f64 refl[3];
    f64 mat[9];
    s32 index[3];
    f64 mean[3];
    f64 meanA[3];
    f64 book[24];
    f64* frame;
    s32 frames;
    s32 i;
    s32 j;
    s32 n;
    s32 round;

    for (i = 0; i < 28; i++) {
        buf[i] = 0;
    }
    frame = work;
    frames = 0;
    while (samples > 0) {
        s32 lag;

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
        for (lag = 0; lag < 3; lag++) {
            r[lag] = 0.0;
            for (i = 0; i < 14; i++) {
                r[lag] -= buf[14 + i] * buf[14 + i - lag];
            }
        }
        if (fabs(r[0]) > 10.0f) {
            adpcmCovariance2(&buf[14], 14, mat);
            if (adpcmLuDecompose2(mat, index) == 0) {
                adpcmLuSolve2(mat, index, r);
                r[0] = 1.0;
                if (adpcmReflection2(r, refl) == 0) {
                    clampReflections(refl);
                    predictorFromReflections(frame, refl, 2);
                    frames++;
                    frame += 3;
                }
            }
        }
    }
    mean[0] = 1.0;
    mean[1] = 0.0;
    mean[2] = 0.0;
    for (i = 0; i < frames; i++) {
        f64 a1 = -work[3 * i + 1];
        f64 a2 = -work[3 * i + 2];
        f64 rho1 = (a1 + a2 * a1) / (1.0 - a2 * a2);
        f64 rho2 = a1 * (rho1 * 1.0) + a2 * 1.0;

        mean[1] += rho1 * 1.0;
        mean[2] += rho2;
    }
    mean[1] /= (f64)frames;
    mean[2] /= (f64)frames;
    levinsonDurbin2(mean, refl, book);
    clampReflections(refl);
    predictorFromReflections(book, refl, 2);
    for (round = 0; round < 3; round++) {
        for (i = 0; i < (1 << round); i++) {
            book[3 * (1 << round) + 3 * i + 0] = book[3 * i + 0] + 0.01f * 0.0;
            book[3 * (1 << round) + 3 * i + 1] = book[3 * i + 1] + 0.01f * -1.0;
            book[3 * (1 << round) + 3 * i + 2] = book[3 * i + 2] + 0.01f * 0.0;
        }
        refineAdpcmCodebook(book, 1 << (round + 1), work, frames);
    }
    for (i = 0; i < 8; i++) {
        f64 v = 2048.0 * (-1.0 * book[3 * i + 1]);

        if (v > 0.0) {
            if (v > 32767.0) {
                info->coef[2 * i] = 0x7FFF;
            } else {
                info->coef[2 * i] = (s16)(0.5 + v);
            }
        } else if (v < -32768.0) {
            info->coef[2 * i] = -0x8000;
        } else {
            info->coef[2 * i] = (s16)(v - 0.5);
        }
        v = 2048.0 * (-1.0 * book[3 * i + 2]);
        if (v > 0.0) {
            if (v > 32767.0) {
                info->coef[2 * i + 1] = 0x7FFF;
            } else {
                info->coef[2 * i + 1] = (s16)(0.5 + v);
            }
        } else if (v < -32768.0) {
            info->coef[2 * i + 1] = -0x8000;
        } else {
            info->coef[2 * i + 1] = (s16)(v - 0.5);
        }
    }
    return 8;
}

/* Runs two nearest-codebook / recentre passes over the frame vectors. */
void refineAdpcmCodebook(f64* codebook, s32 count, f64* frames, s32 frameCount)
{
    f64 acc[24];
    s32 members[8];
    f64 k[3];
    s32 pass;
    s32 f;
    s32 c;
    s32 j;
    s32 i;

    for (pass = 0; pass < 2; pass++) {
        for (c = 0; c < count; c++) {
            acc[3 * c + 0] = 0.0;
            acc[3 * c + 1] = 0.0;
            acc[3 * c + 2] = 0.0;
            members[c] = 0;
        }
        for (f = 0; f < frameCount; f++) {
            f64 a1 = -frames[3 * f + 1];
            f64 a2 = -frames[3 * f + 2];
            f64 rho1 = (a1 + a2 * a1) / (1.0 - a2 * a2);
            f64 rho2 = a1 * (rho1 * 1.0) + a2 * 1.0;
            f64 best = 1e30;
            s32 bestIdx = 0;

            for (c = 0; c < count; c++) {
                f64 x0 = codebook[3 * c + 0];
                f64 x1 = codebook[3 * c + 1];
                f64 x2 = codebook[3 * c + 2];
                f64 d = 2.0 * rho2 * (x0 * x2) + (1.0 * (x2 * x2 + (x0 * x0 + x1 * x1)) + 2.0 * (rho1 * 1.0) * (x0 * x1 + x1 * x2));

                if ((f32)d < (f32)best) {
                    best = d;
                    bestIdx = c;
                }
            }
            members[bestIdx]++;
            acc[3 * bestIdx + 0] += 1.0;
            acc[3 * bestIdx + 1] += rho1 * 1.0;
            acc[3 * bestIdx + 2] += rho2;
        }
        for (c = 0; c < count; c++) {
            if (members[c] > 0) {
                acc[3 * c + 0] /= (f64)members[c];
                acc[3 * c + 1] /= (f64)members[c];
                acc[3 * c + 2] /= (f64)members[c];
            }
        }
        for (c = 0; c < count; c++) {
            levinsonDurbin2(&acc[3 * c], k, &codebook[3 * c]);
            clampReflections(k);
            predictorFromReflections(&codebook[3 * c], k, 2);
        }
    }
}

/* Generic order-`order` Levinson-Durbin recursion on the autocorrelation vector `r` (1-based). */
static inline void levinsonDurbin(f64* r, f64* k, f64* a, s32 order)
{
    f64 div = r[0];
    s32 i;
    s32 j;

    a[0] = 1.0;
    for (i = 1; i <= order; i++) {
        f64 sum = 0.0;

        for (j = 1; j < i; j++) {
            sum += a[j] * r[i - j];
        }
        if ((f32)div > (f32)0.0) {
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

/* Step-down from the predictor `a` to the reflection coefficients; returns nonzero when unstable. */
static inline s32 reflectionCoefs(f64* a, f64* k, s32 order)
{
    f64 tmp[8];
    f64 den;
    s32 i;

    k[order] = a[order];
    den = 1.0 - k[order] * k[order];
    if (0.0 == den) {
        return 1;
    }
    for (i = 0; i <= order - 1; i++) {
        tmp[i] = (a[i] - k[order] * a[order - i]) / den;
    }
    for (i = 0; i <= order - 1; i++) {
        a[i] = tmp[i];
    }
    k[order - 1] = tmp[order - 1];
    return fabs(k[order - 1]) > 1.0;
}

/* Accumulates the covariance matrix of `n` samples of `x`; element (i, j) lives at (order + 1) * i + j. */
static inline void covariance(const s16* x, s32 n, f64* mat, s32 order)
{
    s32 i;
    s32 j;
    s32 t;

    for (i = 1; i <= order; i++) {
        for (j = 1; j <= order; j++) {
            mat[(order + 1) * i + j] = 0.0;
            for (t = 0; t < n; t++) {
                mat[(order + 1) * i + j] += x[t - i] * x[t - j];
            }
        }
    }
}

/* Scaled partial-pivot LU decomposition in place; returns nonzero when singular or ill-conditioned. */
static inline s32 luDecompose(f64* a, s32* index, s32 order)
{
    f64 scale[8];
    s32 stride = order + 1;
    s32 i;
    s32 j;
    s32 k;
    s32 imax;
    f64 big;
    f64 sum;

    for (i = 1; i <= order; i++) {
        big = 0.0;
        for (j = 1; j <= order; j++) {
            f64 t = fabs(a[stride * i + j]);

            if (t > (f32)big) {
                big = t;
            }
        }
        if ((f32)0.0 == (f32)big) {
            return 1;
        }
        scale[i] = 1.0 / big;
    }
    for (j = 1; j <= order; j++) {
        for (i = 1; i < j; i++) {
            sum = a[stride * i + j];
            for (k = 1; k < i; k++) {
                sum -= a[stride * i + k] * a[stride * k + j];
            }
            a[stride * i + j] = sum;
        }
        big = 0.0;
        for (i = j; i <= order; i++) {
            sum = a[stride * i + j];
            for (k = 1; k < j; k++) {
                sum -= a[stride * i + k] * a[stride * k + j];
            }
            a[stride * i + j] = sum;
            if ((f32)(scale[i] * fabs(sum)) >= (f32)big) {
                big = scale[i] * fabs(sum);
                imax = i;
            }
        }
        if (j != imax) {
            for (k = 1; k <= order; k++) {
                f64 t = a[stride * imax + k];

                a[stride * imax + k] = a[stride * j + k];
                a[stride * j + k] = t;
            }
            scale[imax] = scale[j];
        }
        index[j] = imax;
        if ((f32)0.0 == (f32)a[stride * j + j]) {
            return 1;
        }
        if (j != order) {
            f64 inv = 1.0 / a[stride * j + j];

            for (i = j + 1; i <= order; i++) {
                a[stride * i + j] *= inv;
            }
        }
    }
    {
        f64 lo = 1e10;
        f64 hi = 0.0;
        f64 d;
        f32 ratio;

        for (i = 1; i <= order; i++) {
            d = fabs(a[stride * i + i]);
            if ((f32)d < (f32)lo) {
                lo = d;
            }
            if ((f32)d > (f32)hi) {
                hi = d;
            }
        }
        ratio = lo / hi;
        return ratio < 1e-10;
    }
}

/* Forward and back substitution for the decomposed matrix; `b` is overwritten with the solution. */
static inline void luSolve(f64* a, s32* index, f64* b, s32 order)
{
    s32 stride = order + 1;
    s32 i;
    s32 j;
    s32 ii = 0;
    s32 ip;
    f64 sum;

    for (i = 1; i <= order; i++) {
        ip = index[i];
        sum = b[ip];
        b[ip] = b[i];
        if (ii != 0) {
            for (j = ii; j <= i - 1; j++) {
                sum -= a[stride * i + j] * b[j];
            }
        } else if ((f32)sum != (f32)0.0) {
            ii = i;
        }
        b[i] = sum;
    }
    for (i = order; i >= 1; i--) {
        sum = b[i];
        for (j = i + 1; j <= order; j++) {
            sum -= a[stride * i + j] * b[j];
        }
        b[i] = sum / a[stride * i + i];
    }
}

/* Solves the order-2 normal equations of the autocorrelation vector `r` for the predictor `a`. */
void levinsonDurbin2(f64* r, f64* k, f64* a)
{
    levinsonDurbin(r, k, a, 2);
}

/* Converts the order-2 predictor `a` to reflection coefficients `k`; returns nonzero when unstable. */
s32 adpcmReflection2(f64* a, f64* k)
{
    return reflectionCoefs(a, k, 2);
}

/* Accumulates the order-2 covariance matrix of `n` samples of `x`. */
void adpcmCovariance2(const s16* x, s32 n, f64* mat)
{
    covariance(x, n, mat, 2);
}

/* LU-decomposes the order-2 matrix in place; returns nonzero when singular. */
s32 adpcmLuDecompose2(f64* a, s32* index)
{
    return luDecompose(a, index, 2);
}

/* Solves the LU-decomposed order-2 system for the right-hand side `b` in place. */
void adpcmLuSolve2(f64* a, s32* index, f64* b)
{
    luSolve(a, index, b, 2);
}
