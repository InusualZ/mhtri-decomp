/*
 * SYN/synvoice.c - a software synthesizer on top of AX, part three: the voice parameter functions and the tables they
 *    read.
 *
 * RANGE. .text 0x804E01E0-0x804E1120 (21 functions, 0xF40 B); .data 0x8062A2A0-0x8062AB20; .sdata2
 *    0x8079D380-0x8079D3A0.  Cut from the old SC block between `SYN/synenv.c` (0x804E01E0) and `THP/THPDec.c`
 *    (0x804E1120).  COARSE: the library's source files of this half are not separated.
 * FLAGS. the `OS` lib's group with `cflags_base` (configure.py `cflags=`): every function start of the band is
 *    16-aligned (`-O4,p` default alignment), not the 4-byte packing `cflags_os` gives.
 * NAMES. the file name `synvoice.c` is a GUESS (the library name `SYN` is a GUESS too); every `SYNVoice*` function
 *    and the tables `SYNLfoSineTable`, `SYNVolumeTable`, `SYNAttackCurve`, `SYNPitchTable` are GUESSES from what the
 *    bodies do; no row is named in the map or the dump.
 * EVIDENCE. `.data` 0x8062A2A0..0x8062AB20 are its tables (0x100, 0x200, 0x190, 0x3F0 B), `.sdata2`
 *    0x8079D380..0x8079D3A0 its float constants (the first is the int-to-double constant again, see
 *    `SYN/syn.c`); the last pool value `0x4330000000000000` (0x8079D390) is held again at 0x8079D3A0 by
 *    `THP/THPDec.c`.
 * RESIDUALS. `SYNVoicePitchRatio` 0x804E0750 (66 %): the target materialises the table base at entry and three derived
 *    pointers (+0x190, +0x1C0, +0x000) before the table loads, we fold the offsets into the displacements and order the
 *    sum loads differently; `SYNVoiceSetupAdpcm` 0x804E09F0 (81.5 %), `SYNVoiceSetupPcm16` 0x804E0C10 and
 *    `SYNVoiceSetupPcm8` 0x804E0D20 (89 %): register numbering and the order of the address computations only;
 *    `SYNVoiceUpdateMix` 0x804E0650 (99.8 %): pool relocation names only.  flipcheck: .sdata2 pool order.
 * SHAPES. `#pragma dont_inline on` for the file with a window of `dont_inline off` around `SYNVoiceVolume`,
 *    `SYNVoiceChannelVolume` and `SYNVoiceUpdateMix` (the target expands the two level helpers in the mixer update and
 *    calls every other function); the DSP block words are stored as words through a macro (`SYNStoreWord`), not as
 *    halfword pairs; the four pitch tables are one struct (`SYNPitchTables`, 0x3F0 B); the sample set-up functions
 *    repeat the zero stores in both branches.
 */

#include "types.h"

#include "AX/AXAlloc.h"
#include "AX/AXVPB.h"
#include "MIX/mix.h"
#include "SYN/syn.h"
#include "SYN/synenv.h"
#include "SYN/synvoice.h"

#pragma fp_contract off
#pragma dont_inline on

f32 SYNLfoSineTable[64] = {
    0.0f, 0.09802f, 0.19509f, 0.29028f, 0.38268f, 0.4714f, 0.55557f, 0.63439f,
    0.70711f, 0.77301f, 0.83147f, 0.88192f, 0.92388f, 0.95694f, 0.98079f, 0.99518f,
    1.0f, 0.99518f, 0.98079f, 0.95694f, 0.92388f, 0.88192f, 0.83147f, 0.77301f,
    0.70711f, 0.63439f, 0.55557f, 0.4714f, 0.38268f, 0.29028f, 0.19509f, 0.09802f,
    0.0f, -0.09802f, -0.19509f, -0.29028f, -0.38268f, -0.4714f, -0.55557f, -0.63439f,
    -0.70711f, -0.77301f, -0.83147f, -0.88192f, -0.92388f, -0.95694f, -0.98079f, -0.99518f,
    -1.0f, -0.99518f, -0.98079f, -0.95694f, -0.92388f, -0.88192f, -0.83147f, -0.77301f,
    -0.70711f, -0.63439f, -0.55557f, -0.4714f, -0.38268f, -0.29028f, -0.19509f, -0.09802f,
};

s32 SYNVolumeTable[128] = {
    -62914560, -55149952, -47258631, -42642504, -39367310, -36826872,
    -34751184, -32996214, -31475990, -30135057, -28935552, -27850467,
    -26859863, -25948595, -25104893, -24319425, -23584669, -22894472,
    -22243736, -21628193, -21044231, -20488766, -19959147, -19453074,
    -18968542, -18503793, -18057274, -17627610, -17213572, -16814066,
    -16428104, -16054800, -15693348, -15343020, -15003151, -14673134,
    -14352415, -14040484, -13736873, -13441148, -13152910, -12871791,
    -12597446, -12329556, -12067826, -11811978, -11561753, -11316910,
    -11077221, -10842476, -10612472, -10387024, -10165954, -9949094,
    -9736289, -9527388, -9322252, -9120746, -8922745, -8728129,
    -8536784, -8348601, -8163479, -7981319, -7802027, -7625516,
    -7451699, -7280496, -7111830, -6945626, -6781814, -6620325,
    -6461095, -6304061, -6149164, -5996346, -5845552, -5696729,
    -5549827, -5404796, -5261590, -5120162, -4980470, -4842471,
    -4706125, -4571392, -4438236, -4306618, -4176505, -4047862,
    -3920657, -3794857, -3670432, -3547352, -3425589, -3305114,
    -3185901, -3067923, -2951155, -2835573, -2721152, -2607870,
    -2495703, -2384632, -2274633, -2165687, -2057774, -1950874,
    -1844968, -1740039, -1636067, -1533037, -1430931, -1329732,
    -1229425, -1129994, -1031424, -933700, -836808, -740733,
    -645463, -550983, -457281, -364343, -272158, -180714,
    -89998, 0,
};

s32 SYNAttackCurve[100] = {
    -62914560, -26157189, -22211529, -19903465, -18265868, -16995649,
    -15957805, -15080320, -14320208, -13649742, -13049989, -12507447,
    -12012145, -11556511, -11134660, -10741926, -10374548, -10029449,
    -9704081, -9396310, -9104329, -8826596, -8561787, -8308750,
    -8066484, -7834110, -7610850, -7396018, -7188999, -6989246,
    -6796265, -6609613, -6428887, -6253723, -6083789, -5918780,
    -5758421, -5602455, -5450650, -5302787, -5158668, -5018109,
    -4880936, -4746991, -4616126, -4488202, -4363090, -4240668,
    -4120824, -4003451, -3888449, -3775725, -3665190, -3556760,
    -3450358, -3345907, -3243339, -3142586, -3043586, -2946278,
    -2850605, -2756514, -2663953, -2572873, -2483227, -2394971,
    -2308063, -2222461, -2138128, -2055026, -1973120, -1892376,
    -1812761, -1734244, -1656795, -1580386, -1504989, -1430578,
    -1357127, -1284611, -1213008, -1142294, -1072448, -1003449,
    -935276, -867909, -801331, -735522, -670466, -606144,
    -542542, -479642, -417429, -355889, -295008, -234770,
    -175164, -116175, -57791, 0,
};

/* size: 0x3F0 - the pitch ratio tables: cents within a semitone, octaves, semitones, and semitones downwards. */
typedef struct SYNPitchTables {
    /* +0x000 */ f32 fine[100];
    /* +0x190 */ f32 octave[12];
    /* +0x1C0 */ f32 semitone[12];
    /* +0x1F0 */ f32 down[128];
} SYNPitchTables;

SYNPitchTables SYNPitchTable = {
    {
        1.0f, 1.000578f, 1.001156f, 1.001734f, 1.002313f, 1.002892f,
        1.003472f, 1.004052f, 1.004632f, 1.005212f, 1.005793f, 1.006374f,
        1.006956f, 1.007537f, 1.00812f, 1.008702f, 1.009285f, 1.009868f,
        1.010451f, 1.011035f, 1.011619f, 1.012204f, 1.012789f, 1.013374f,
        1.013959f, 1.014545f, 1.015132f, 1.015718f, 1.016305f, 1.016892f,
        1.01748f, 1.018068f, 1.018656f, 1.019244f, 1.019833f, 1.020423f,
        1.021012f, 1.021602f, 1.022192f, 1.022783f, 1.023374f, 1.023965f,
        1.024557f, 1.025149f, 1.025741f, 1.026334f, 1.026927f, 1.02752f,
        1.028114f, 1.028708f, 1.029302f, 1.029897f, 1.030492f, 1.031087f,
        1.031683f, 1.032279f, 1.032876f, 1.033472f, 1.03407f, 1.034667f,
        1.035265f, 1.035863f, 1.036462f, 1.03706f, 1.03766f, 1.038259f,
        1.038859f, 1.039459f, 1.04006f, 1.040661f, 1.041262f, 1.041864f,
        1.042466f, 1.043068f, 1.043671f, 1.044274f, 1.044877f, 1.045481f,
        1.046085f, 1.046689f, 1.047294f, 1.047899f, 1.048505f, 1.049111f,
        1.049717f, 1.050323f, 1.05093f, 1.051537f, 1.052145f, 1.052753f,
        1.053361f, 1.05397f, 1.054579f, 1.055188f, 1.055798f, 1.056408f,
        1.057018f, 1.057629f, 1.05824f, 1.058851f,
    },
    {
        1.0f, 2.0f, 4.0f, 8.0f, 16.0f, 32.0f,
        64.0f, 128.0f, 256.0f, 512.0f, 1024.0f, 2048.0f,
    },
    {
        1.0f, 1.059463f, 1.122462f, 1.189207f, 1.259921f, 1.33484f,
        1.414214f, 1.498307f, 1.587401f, 1.681793f, 1.781797f, 1.887749f,
    },
    {
        1.0f, 0.943874f, 0.890899f, 0.840896f, 0.793701f, 0.749154f,
        0.707107f, 0.66742f, 0.629961f, 0.594604f, 0.561231f, 0.529732f,
        0.5f, 0.471937f, 0.445449f, 0.420448f, 0.39685f, 0.374577f,
        0.353553f, 0.33371f, 0.31498f, 0.297302f, 0.280616f, 0.264866f,
        0.25f, 0.235969f, 0.222725f, 0.210224f, 0.198425f, 0.187288f,
        0.176777f, 0.166855f, 0.15749f, 0.148651f, 0.140308f, 0.132433f,
        0.125f, 0.117984f, 0.111362f, 0.105112f, 0.099213f, 0.093644f,
        0.088388f, 0.083427f, 0.078745f, 0.074325f, 0.070154f, 0.066216f,
        0.0625f, 0.058992f, 0.055681f, 0.052556f, 0.049606f, 0.046822f,
        0.044194f, 0.041714f, 0.039373f, 0.037163f, 0.035077f, 0.033108f,
        0.03125f, 0.029496f, 0.027841f, 0.026278f, 0.024803f, 0.023411f,
        0.022097f, 0.020857f, 0.019686f, 0.018581f, 0.017538f, 0.016554f,
        0.015625f, 0.014748f, 0.01392f, 0.013139f, 0.012402f, 0.011706f,
        0.011049f, 0.010428f, 0.009843f, 0.009291f, 0.008769f, 0.008277f,
        0.007813f, 0.007374f, 0.00696f, 0.00657f, 0.006201f, 0.005853f,
        0.005524f, 0.005214f, 0.004922f, 0.004645f, 0.004385f, 0.004139f,
        0.003906f, 0.003687f, 0.00348f, 0.003285f, 0.0031f, 0.002926f,
        0.002762f, 0.002607f, 0.002461f, 0.002323f, 0.002192f, 0.002069f,
        0.001953f, 0.001844f, 0.00174f, 0.001642f, 0.00155f, 0.001463f,
        0.001381f, 0.001304f, 0.00123f, 0.001161f, 0.001096f, 0.001035f,
        0.000977f, 0.000922f, 0.00087f, 0.000821f, 0.000775f, 0.000732f,
        0.000691f, 0.000652f,
    },
};

/* Stores a pair of halfwords as one word, the way the DSP block is written. */
#define SYNStoreWord(firstHalf, value) (*(u32*)(firstHalf) = (value))

/* The ADPCM nibble address of a sample offset: 14 samples per 16-nibble frame behind a two-nibble header. */
#define SYNAdpcmAddress(base, offset) ((base) + (offset) % 14 + (offset) / 14 * 16 + 2)

void SYNVoiceStepVolumeEnvelope(SYNVoice* voice)
{
    switch (voice->volPhase) {
    case 0:
        voice->volAttackBase += voice->volAttackRate;
        if (voice->volAttackBase >= 0x630000) {
            voice->volLevel = 0;
        } else {
            voice->volLevel = SYNAttackCurve[voice->volAttackBase >> 16];
        }
        if (voice->volLevel == 0) {
            voice->volPhase = 1;
        }
        break;
    case 1:
        voice->volLevel += voice->volDecayRate;
        if (voice->volLevel <= voice->volSustainLevel) {
            voice->volLevel = voice->volSustainLevel;
            voice->volPhase = 2;
        }
        if (voice->volLevel <= -0x2D00000) {
            voice->volPhase = 4;
            voice->synth->noteVoice[voice->channel][voice->key] = NULL;
        }
        break;
    case 3:
        if (voice->volLevel <= -0x2D00000) {
            voice->volPhase = 4;
        } else {
            voice->volLevel += voice->volReleaseRate;
        }
        break;
    }
}

void SYNVoiceStepModulationEnvelope(SYNVoice* voice)
{
    s32 peak = voice->modPeak;

    if (peak == 0) {
        return;
    }
    switch (voice->modPhase) {
    case 0:
        voice->modLevel += voice->modAttackRate;
        if (peak > 0) {
            if (voice->modLevel >= peak) {
                voice->modPeak = voice->modLevel;
                voice->modPhase = 1;
            }
        } else if (voice->modLevel <= peak) {
            voice->modPeak = voice->modLevel;
            voice->modPhase = 1;
        }
        break;
    case 1:
        voice->modLevel += voice->modDecayRate;
        if (peak > 0) {
            if (voice->modLevel <= 0) {
                voice->modLevel = 0;
                voice->modPeak = 0;
                return;
            }
            if (voice->modLevel <= voice->modSustainLevel) {
                voice->modLevel = voice->modSustainLevel;
                voice->modPhase = 2;
            }
        } else {
            if (voice->modLevel >= 0) {
                voice->modLevel = 0;
                voice->modPeak = 0;
                return;
            }
            if (voice->modLevel >= voice->modSustainLevel) {
                voice->modLevel = voice->modSustainLevel;
                voice->modPhase = 2;
            }
        }
        break;
    case 3:
        voice->modLevel += voice->modReleaseRate;
        if (peak > 0) {
            if (voice->modLevel <= 0) {
                voice->modLevel = 0;
                voice->modPeak = 0;
            }
        } else if (voice->modLevel >= 0) {
            voice->modLevel = 0;
            voice->modPeak = 0;
        }
        break;
    }
}

void SYNVoiceInitLfo(SYNVoice* voice)
{
    voice->lfoPitchOut = 0;
    voice->lfoVolumeOut = 0;
    voice->lfoPhase = 0;
    voice->lfoRate = voice->region->lfoRate;
    voice->lfoDelay = voice->region->lfoDelay;
    voice->lfoVolumeDepth = voice->region->lfoVolumeDepth;
    voice->lfoPitchDepth = voice->region->lfoPitchDepth;
    voice->lfoVolumeModDepth = voice->region->lfoVolumeModDepth;
    voice->lfoPitchModDepth = voice->region->lfoPitchModDepth;
}

void SYNVoiceStepLfo(SYNVoice* voice)
{
    f32 wheel;
    f32 wave;

    if (voice->lfoDelay != 0) {
        voice->lfoDelay--;
        return;
    }
    voice->lfoPhase += voice->lfoRate;
    wave = SYNLfoSineTable[(voice->lfoPhase >> 16) % 64];
    wheel = SYNKeyScaleTable[voice->synth->controller[voice->channel][1]];
    voice->lfoVolumeOut = (s32)(wave * ((f32)voice->lfoVolumeDepth + (f32)voice->lfoVolumeModDepth * wheel));
    voice->lfoPitchOut = (s32)(wave * ((f32)voice->lfoPitchDepth + (f32)voice->lfoPitchModDepth * wheel));
}

void SYNVoiceInitBaseVolume(SYNVoice* voice)
{
    voice->baseVolume = voice->layer->volumeOffset + SYNVolumeTable[voice->velocity];
}

void SYNVoiceInitPan(SYNVoice* voice)
{
    if (voice->channel == 9) {
        voice->pan = voice->region->drumPan;
    } else {
        voice->pan = voice->synth->controller[voice->channel][10];
    }
}

#pragma dont_inline off

s32 SYNVoiceVolume(SYNVoice* voice)
{
    s32 volume = voice->volLevel + voice->baseVolume;

    return (volume + voice->lfoVolumeOut) >> 16;
}

s32 SYNVoiceChannelVolume(SYNVoice* voice)
{
    SYNSynth* synth = voice->synth;

    return ((synth->masterVolume + synth->channelVolume[voice->channel]) + synth->expression[voice->channel]) >> 16;
}

void SYNVoiceUpdateMix(SYNVoice* voice)
{
    MIXSetInput(voice->axVoice, SYNVoiceVolume(voice));
    MIXSetAuxA(voice->axVoice, voice->synth->reverbSend[voice->channel] >> 16);
    MIXSetAuxB(voice->axVoice, voice->synth->chorusSend[voice->channel] >> 16);
    MIXSetFader(voice->axVoice, SYNVoiceChannelVolume(voice));
    if (voice->channel != 9) {
        MIXSetPan(voice->axVoice, voice->synth->controller[voice->channel][10]);
    }
    if (voice->synth->frameCallback != NULL) {
        voice->synth->frameCallback(voice->axVoice, voice->channel);
    }
}

#pragma dont_inline on

f32 SYNVoicePitchRatio(SYNVoice* voice)
{
    SYNPitchTables* table = &SYNPitchTable;
    s32 cents = (((voice->modLevel + voice->basePitch) + voice->lfoPitchOut) + voice->synth->pitchBend[voice->channel]) / 65536;
    s32 octaves;
    s32 fine;

    if (cents > 0) {
        return table->fine[cents % 100] * (table->octave[cents / 1200] * table->semitone[cents % 1200 / 100]);
    }
    if (cents < 0) {
        octaves = cents / 100;
        fine = cents % 100;
        if (fine != 0) {
            fine += 100;
            octaves -= 1;
        }
        return table->down[-octaves] * table->fine[fine];
    }
    return 1.0f;
}

void SYNVoiceInitPitch(SYNVoice* voice)
{
    voice->sampleGain = (f32)voice->sample->sampleRate / 32000.0f;
    voice->basePitch = (voice->key - voice->layer->rootKey) * 100;
    voice->basePitch = (voice->basePitch + voice->layer->fineTune) << 16;
}

void SYNVoiceStartPitch(SYNVoice* voice)
{
    u32 ratio = (u32)(65536.0f * (voice->sampleGain * SYNVoicePitchRatio(voice)));
    AXPB* pb;

    voice->axVoice->pb.srcSelect = 1;
    pb = &voice->axVoice->pb;
    pb->srcRatio[0] = ratio >> 16;
    pb->srcRatio[1] = ratio;
    pb->srcFraction = 0;
    pb->srcLast[0] = 0;
    pb->srcLast[1] = 0;
    pb->srcLast[2] = 0;
    pb->srcLast[3] = 0;
    voice->axVoice->sync &= ~0x20000;
    voice->axVoice->sync |= 0x10001;
}

void SYNVoiceUpdatePitch(SYNVoice* voice)
{
    SYNStoreWord(voice->axVoice->pb.srcRatio, (u32)(65536.0f * (voice->sampleGain * SYNVoicePitchRatio(voice))));
    voice->axVoice->sync |= 0x20000;
}

void SYNVoiceSetupAdpcm(SYNVoice* voice)
{
    SYNLayer* layer = voice->layer;
    AXPB* pb = &voice->axVoice->pb;
    SYNAdpcm* adpcm;
    u32 base;
    u32 loopAddress;
    u32 endAddress;
    s32 i;

    if (layer->loopStart + layer->loopLength != 0) {
        voice->looped = 1;
        base = voice->synth->sampleBaseAdpcm + voice->sample->dataOffset;
        adpcm = voice->adpcm;
        loopAddress = SYNAdpcmAddress(base, layer->loopStart);
        endAddress = SYNAdpcmAddress(base, layer->loopStart + layer->loopLength - 1);
        SYNStoreWord(&pb->addrFormat.first, 0x10000);
        SYNStoreWord(&pb->addrLoop.hi, loopAddress);
        SYNStoreWord(&pb->addrEnd.hi, endAddress);
        SYNStoreWord(&pb->addrCurrent.hi, base + 2);
        for (i = 0; i < 10; i++) {
            pb->adpcm[i] = adpcm->coef[i];
        }
        pb->adpcmLoop = adpcm->loop;
        voice->axVoice->sync = (voice->axVoice->sync & 0xFFFF87FF) | 0x40000 | 0x8400;
    } else {
        voice->looped = 0;
        base = voice->synth->sampleBaseAdpcm + voice->sample->dataOffset;
        adpcm = voice->adpcm;
        SYNStoreWord(&pb->addrFormat.first, 0);
        SYNStoreWord(&pb->addrLoop.hi, base);
        SYNStoreWord(&pb->addrEnd.hi, SYNAdpcmAddress(base, voice->sample->length - 1));
        SYNStoreWord(&pb->addrCurrent.hi, base + 2);
        for (i = 0; i < 10; i++) {
            pb->adpcm[i] = adpcm->coef[i];
        }
        voice->axVoice->sync = (voice->axVoice->sync & 0xFFFF87FF) | 0x8400;
    }
}

void SYNVoiceSetupPcm16(SYNVoice* voice)
{
    SYNLayer* layer = voice->layer;
    AXPB* pb = &voice->axVoice->pb;
    u32 base;
    u32 loopStart;
    u32 loopEnd;

    if (layer->loopStart + layer->loopLength != 0) {
        voice->looped = 1;
        base = voice->synth->sampleBasePcm16 + voice->sample->dataOffset;
        loopStart = base + layer->loopStart;
        loopEnd = loopStart + layer->loopLength - 1;
        SYNStoreWord(&pb->addrFormat.first, 0x1000A);
        SYNStoreWord(&pb->addrLoop.hi, loopStart);
        SYNStoreWord(&pb->addrEnd.hi, loopEnd);
        SYNStoreWord(&pb->addrCurrent.hi, base);
        SYNStoreWord(&pb->adpcm[0].first, 0);
        SYNStoreWord(&pb->adpcm[1].first, 0);
        SYNStoreWord(&pb->adpcm[2].first, 0);
        SYNStoreWord(&pb->adpcm[3].first, 0);
        SYNStoreWord(&pb->adpcm[4].first, 0);
        SYNStoreWord(&pb->adpcm[5].first, 0);
        SYNStoreWord(&pb->adpcm[6].first, 0);
        SYNStoreWord(&pb->adpcm[7].first, 0);
        SYNStoreWord(&pb->adpcm[8].first, 0x08000000);
        SYNStoreWord(&pb->adpcm[9].first, 0);
    } else {
        voice->looped = 0;
        base = voice->synth->sampleBasePcm16 + voice->sample->dataOffset;
        SYNStoreWord(&pb->addrFormat.first, 0xA);
        SYNStoreWord(&pb->addrLoop.hi, base);
        SYNStoreWord(&pb->addrEnd.hi, base + voice->sample->length - 1);
        SYNStoreWord(&pb->addrCurrent.hi, base);
        SYNStoreWord(&pb->adpcm[0].first, 0);
        SYNStoreWord(&pb->adpcm[1].first, 0);
        SYNStoreWord(&pb->adpcm[2].first, 0);
        SYNStoreWord(&pb->adpcm[3].first, 0);
        SYNStoreWord(&pb->adpcm[4].first, 0);
        SYNStoreWord(&pb->adpcm[5].first, 0);
        SYNStoreWord(&pb->adpcm[6].first, 0);
        SYNStoreWord(&pb->adpcm[7].first, 0);
        SYNStoreWord(&pb->adpcm[8].first, 0x08000000);
        SYNStoreWord(&pb->adpcm[9].first, 0);
    }
    voice->axVoice->sync = (voice->axVoice->sync & 0xFFFF87FF) | 0x8400;
}

void SYNVoiceSetupPcm8(SYNVoice* voice)
{
    SYNLayer* layer = voice->layer;
    AXPB* pb = &voice->axVoice->pb;
    u32 base;
    u32 loopStart;
    u32 loopEnd;

    if (layer->loopStart + layer->loopLength != 0) {
        voice->looped = 1;
        base = voice->synth->sampleBasePcm8 + voice->sample->dataOffset;
        loopStart = base + layer->loopStart;
        loopEnd = loopStart + layer->loopLength - 1;
        SYNStoreWord(&pb->addrFormat.first, 0x10019);
        SYNStoreWord(&pb->addrLoop.hi, loopStart);
        SYNStoreWord(&pb->addrEnd.hi, loopEnd);
        SYNStoreWord(&pb->addrCurrent.hi, base);
        SYNStoreWord(&pb->adpcm[0].first, 0);
        SYNStoreWord(&pb->adpcm[1].first, 0);
        SYNStoreWord(&pb->adpcm[2].first, 0);
        SYNStoreWord(&pb->adpcm[3].first, 0);
        SYNStoreWord(&pb->adpcm[4].first, 0);
        SYNStoreWord(&pb->adpcm[5].first, 0);
        SYNStoreWord(&pb->adpcm[6].first, 0);
        SYNStoreWord(&pb->adpcm[7].first, 0);
        SYNStoreWord(&pb->adpcm[8].first, 0x01000000);
        SYNStoreWord(&pb->adpcm[9].first, 0);
    } else {
        voice->looped = 0;
        base = voice->synth->sampleBasePcm8 + voice->sample->dataOffset;
        SYNStoreWord(&pb->addrFormat.first, 0x19);
        SYNStoreWord(&pb->addrLoop.hi, base);
        SYNStoreWord(&pb->addrEnd.hi, base + voice->sample->length - 1);
        SYNStoreWord(&pb->addrCurrent.hi, base);
        SYNStoreWord(&pb->adpcm[0].first, 0);
        SYNStoreWord(&pb->adpcm[1].first, 0);
        SYNStoreWord(&pb->adpcm[2].first, 0);
        SYNStoreWord(&pb->adpcm[3].first, 0);
        SYNStoreWord(&pb->adpcm[4].first, 0);
        SYNStoreWord(&pb->adpcm[5].first, 0);
        SYNStoreWord(&pb->adpcm[6].first, 0);
        SYNStoreWord(&pb->adpcm[7].first, 0);
        SYNStoreWord(&pb->adpcm[8].first, 0x01000000);
        SYNStoreWord(&pb->adpcm[9].first, 0);
    }
    voice->axVoice->sync = (voice->axVoice->sync & 0xFFFF87FF) | 0x8400;
}

void SYNVoiceSetupSample(SYNVoice* voice)
{
    switch (voice->sample->format) {
    case 0:
        SYNVoiceSetupAdpcm(voice);
        break;
    case 1:
        SYNVoiceSetupPcm16(voice);
        break;
    case 2:
        SYNVoiceSetupPcm8(voice);
        break;
    }
}

void SYNVoiceFreedCallback(AXVPB* axVoice)
{
    SYNSynth* synth = (SYNSynth*)axVoice->userContext;
    SYNVoice* voice = &SYNVoices[axVoice->index];

    MIXReleaseChannel(axVoice);
    if (voice->exclusiveClass != 0) {
        synth->exclusiveVoice[voice->channel][voice->exclusiveClass] = NULL;
    }
    if (synth->noteVoice[voice->channel][voice->key] == voice) {
        synth->noteVoice[voice->channel][voice->key] = NULL;
    }
    voice->synth = NULL;
    synth->voiceCount--;
}

void SYNVoiceRelease(SYNVoice* voice, u32 priority)
{
    voice->volPhase = 3;
    voice->modPhase = 3;
    AXSetVoicePriority(voice->axVoice, priority);
}

void SYNVoiceRun(u32 index)
{
    SYNSynth* synth;
    SYNVoice* voice = &SYNVoices[index];

    synth = voice->synth;

    if (synth != NULL) {
        if (voice->looped == 0 && voice->axVoice->pb.running == 0) {
            if (voice->exclusiveClass != 0) {
                synth->exclusiveVoice[voice->channel][voice->exclusiveClass] = NULL;
            }
            if (synth->noteVoice[voice->channel][voice->key] == voice) {
                synth->noteVoice[voice->channel][voice->key] = NULL;
            }
            voice->volPhase = 4;
        }
        SYNVoiceStepVolumeEnvelope(voice);
        if (voice->volPhase == 4) {
            if (voice->exclusiveClass != 0) {
                voice->synth->exclusiveVoice[voice->channel][voice->exclusiveClass] = NULL;
            }
            voice->synth = NULL;
            MIXReleaseChannel(voice->axVoice);
            AXFreeVoice(voice->axVoice);
            synth->voiceCount--;
            return;
        }
        SYNVoiceStepLfo(voice);
        SYNVoiceStepModulationEnvelope(voice);
        SYNVoiceUpdateMix(voice);
        SYNVoiceUpdatePitch(voice);
    }
}

s32 SYNVoiceBindLayer(SYNVoice* voice)
{
    SYNSynth* synth = voice->synth;
    u16 index = synth->keyMap[voice->channel]->layer[voice->key];

    if (index == 0xFFFF) {
        return 0;
    }
    voice->layer = &synth->layers[index];
    voice->region = &synth->regions[voice->layer->regionIndex];
    voice->sample = &synth->samples[voice->layer->sampleIndex];
    voice->adpcm = &synth->adpcms[voice->sample->adpcmIndex];
    return 1;
}
