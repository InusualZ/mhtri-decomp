/*
 * SYN/synenv.c - a software synthesizer on top of AX, part two: the volume and modulation envelope setup (exponential
 *    timecent curves through `pow`) over the 0x200 B key-scaling table.
 *
 * RANGE. .text 0x804DFC50-0x804E01E0 (2 functions, 0x590 B); .data 0x8062A0A0-0x8062A2A0; .sdata2
 *    0x8079D360-0x8079D380.  Cut from the old SC block between `SYN/syn.c` (0x804DFC50) and `SYN/synvoice.c`
 *    (0x804E01E0).
 * FLAGS. the `OS` lib's group with `cflags_base` (configure.py `cflags=`): every function start of the band is
 *    16-aligned (`-O4,p` default alignment), not the 4-byte packing `cflags_os` gives.
 * NAMES. `SYNSetupVolumeEnvelope`, `SYNSetupModulationEnvelope`, `SYNKeyScaleTable` and the `SYNVoice`/`SYNRegion` field
 *    names are GUESSes read off the bodies; the file name `synenv.c` is a GUESS from the `pow` curves (the library name
 *    `SYN` is a GUESS too); no row was named in the map or the dump.
 * EVIDENCE. the `0x4330000080000000` constant is held at 0x8079D378 here and again at 0x8079D380 for the next unit;
 *    `.data` 0x8062A0A0 (0x200 B) is read by 0x804DFC50, 0x804DFF10 and, as an extern, by the next unit;
 *    `.sdata2` 0x8079D360..0x8079D380 are the constants of the two functions. The cut after 0x804DFF10 is
 *    placed at the first function that reads a table only the next unit's data order can own (0x804E01E0 reads
 *    0x8062A5A0, which follows 0x8062A2A0 in `.data`; the pool window is 0x804E01E0..0x804E0470).
 * RESIDUALS. `SYNSetupVolumeEnvelope` 0x804DFC50 (98.4 %) and `SYNSetupModulationEnvelope` 0x804DFF10 (98.0 %): identical
 *    instruction stream except the .sdata2 pool entries (relocation names `@NN` vs the target's labels) and, in the second,
 *    the callee-saved registers of `voice`/`peak` swapped (r31/r30 vs r30/r31); flipcheck: .sdata2 byte order.
 * SHAPES. the timecent-to-time conversion is a static inline helper expanded in place (two pow call sites per
 *    function).
 */

#include "types.h"

#include "MSL/w_math.h"
#include "SYN/syn.h"
#include "SYN/synenv.h"

#pragma fp_contract off

#define SYN_ENV_UNSET ((s32)0x80000000)

f32 SYNKeyScaleTable[128] = {
    0.0f, 0.007813f, 0.015625f, 0.023438f, 0.03125f, 0.039063f, 0.046875f, 0.054688f,
    0.0625f, 0.070313f, 0.078125f, 0.085938f, 0.09375f, 0.101563f, 0.109375f, 0.117188f,
    0.125f, 0.132813f, 0.140625f, 0.148438f, 0.15625f, 0.164063f, 0.171875f, 0.179688f,
    0.1875f, 0.195313f, 0.203125f, 0.210938f, 0.21875f, 0.226563f, 0.234375f, 0.242188f,
    0.25f, 0.257813f, 0.265625f, 0.273438f, 0.28125f, 0.289063f, 0.296875f, 0.304688f,
    0.3125f, 0.320313f, 0.328125f, 0.335938f, 0.34375f, 0.351563f, 0.359375f, 0.367188f,
    0.375f, 0.382813f, 0.390625f, 0.398438f, 0.40625f, 0.414063f, 0.421875f, 0.429688f,
    0.4375f, 0.445313f, 0.453125f, 0.460938f, 0.46875f, 0.476563f, 0.484375f, 0.492188f,
    0.5f, 0.507813f, 0.515625f, 0.523438f, 0.53125f, 0.539063f, 0.546875f, 0.554688f,
    0.5625f, 0.570313f, 0.578125f, 0.585938f, 0.59375f, 0.601563f, 0.609375f, 0.617188f,
    0.625f, 0.632813f, 0.640625f, 0.648438f, 0.65625f, 0.664063f, 0.671875f, 0.679688f,
    0.6875f, 0.695313f, 0.703125f, 0.710938f, 0.71875f, 0.726563f, 0.734375f, 0.742188f,
    0.75f, 0.757813f, 0.765625f, 0.773438f, 0.78125f, 0.789063f, 0.796875f, 0.804688f,
    0.8125f, 0.820313f, 0.828125f, 0.835938f, 0.84375f, 0.851563f, 0.859375f, 0.867188f,
    0.875f, 0.882813f, 0.890625f, 0.898438f, 0.90625f, 0.914063f, 0.921875f, 0.929688f,
    0.9375f, 0.945313f, 0.953125f, 0.960938f, 0.96875f, 0.976563f, 0.984375f, 0.992188f,
};

/* Converts an envelope time in timecents (plus its key-scaling share) to a third of a millisecond count. */
static inline s32 SYNEnvelopeTime(s32 base, s32 scale, u8 key)
{
    s32 value;

    if (base == SYN_ENV_UNSET) {
        value = 0;
    } else if (scale == SYN_ENV_UNSET) {
        value = (s32)(1000.0f * (f32)pow(2.0, (f32)base / 78643200.0f));
    } else {
        value = (s32)(1000.0f * (f32)pow(2.0, ((f32)base + (f32)scale * SYNKeyScaleTable[key]) / 78642000.0f));
    }
    return value / 3;
}

void SYNSetupVolumeEnvelope(SYNVoice* voice)
{
    s32 time;

    if (voice->region->volAttack == SYN_ENV_UNSET) {
        voice->volPhase = 1;
        voice->volLevel = 0;
        if (voice->region->volDecay == SYN_ENV_UNSET) {
            voice->volPhase = 2;
            voice->volLevel = voice->region->volSustain;
        }
    } else {
        time = SYNEnvelopeTime(voice->region->volAttack, voice->region->volAttackKeyScale, voice->velocity);
        if (time != 0) {
            voice->volAttackBase = 0;
            voice->volLevel = 0xFC400000;
            voice->volPhase = 0;
            voice->volAttackRate = 0x640000 / time;
        } else {
            voice->volAttackBase = 0;
            voice->volAttackRate = 0x640000;
            voice->volLevel = 0xFC400000;
            voice->volPhase = 0;
        }
    }
    if (voice->volPhase < 2) {
        time = SYNEnvelopeTime(voice->region->volDecay, voice->region->volDecayKeyScale, voice->key);
        if (time != 0) {
            voice->volDecayRate = 0xFC400000 / time;
        } else {
            voice->volDecayRate = 0xFC400000;
        }
    }
    voice->volSustainLevel = voice->region->volSustain;
    voice->volReleaseRate = voice->region->volRelease;
}

void SYNSetupModulationEnvelope(SYNVoice* voice)
{
    s32 time;
    s32 peak;

    voice->modLevel = 0;
    peak = voice->region->modPeak;
    voice->modPeak = peak;
    if (peak != 0) {
        if (voice->region->modAttack == SYN_ENV_UNSET) {
            voice->modPhase = 1;
            voice->modLevel = peak;
            if (voice->region->modDecay == SYN_ENV_UNSET) {
                voice->modPhase = 2;
                voice->modLevel = voice->region->modSustain;
            }
        } else {
            time = SYNEnvelopeTime(voice->region->modAttack, voice->region->modAttackKeyScale, voice->velocity);
            if (time != 0) {
                voice->modPhase = 0;
                voice->modAttackRate = peak / time;
            } else {
                voice->modAttackRate = peak;
                voice->modPhase = 0;
            }
        }
        if (voice->modPhase < 2) {
            time = SYNEnvelopeTime(voice->region->modDecay, voice->region->modDecayKeyScale, voice->key);
            if (time != 0) {
                voice->modDecayRate = voice->modPeak / time;
            } else {
                voice->modDecayRate = voice->modPeak;
            }
            voice->modDecayRate = -voice->modDecayRate;
        }
        voice->modSustainLevel = voice->region->modSustain;
        voice->modReleaseRate = voice->region->modRelease;
    }
}
