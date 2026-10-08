/*
 * MIX/mix.c - the MIX audio mixer: per-voice channel levels, the pan/volume coefficient tables and the AX mix update.
 * RANGE. .text 0x804C25E0-0x804C5C10 (17 functions); .data 0x8061AE00-0x8061B9A0; .bss 0x80748BB8-0x8074CF40; .sbss
 *    0x807952A0-0x807952B8.  Cut from the OS core band 0x804C1760-0x804D9B4C.  Evidence: `__MIXSetPan` is the
 *    dump's name for the first function; every function in the range reads the 0xBA0-byte coefficient table
 *    (.data 0x8061AE00), the 0x2A00/0x1988-byte channel arrays (.bss 0x80748BB8, 0x8074B5B8) or the mix state
 *    (.sbss 0x807952A0..0x807952B4), and it calls AX (0x8046E4E0, 0x80471950); nothing outside the range reads
 *    them.
 * FLAGS. `cflags_os` (configure.py), the flags of the unit it was cut from; the bodies written here are measured with
 *    them.
 * NAMES. `__MIXSetPan` is the dump's name; the file name `mix.c` is a GUESS.  GUESS (mix.h): `MIXInit`, `MIXQuit`,
 *    `MIXSetSoundMode`, `MIXGetSoundMode`, `MIXReleaseChannel`, `MIXSetInput`, `MIXSetAuxA`, `MIXSetAuxB`, `MIXSetPan`,
 *    `MIXSetSPan`, `MIXSetFader`, `__MIXGetVolume`, `__MIXResetAux`, the arrays `MIXChannelArray`/`MIXAuxArray` and the
 *    table `MIXTable` (the layout is traced from the bodies; the 128-entry rows are the pan curves).
 * RESIDUALS. not attempted: `fn_804C2840` (0x16C4 B, 9 arguments, the channel set-up), `fn_804C40D0` (0x1694 B, the update
 *    over all channels) and `fn_804C5770` (0x43C B, the ramp fill of one voice's parameter block).  The .sbss word pair at
 *    0x807952B0 and the 8 bytes after the aux array are claimed but their use is not traced.
 * SHAPES. none beyond what the bodies show.
 */

#include "types.h"
#include "AX/AX.h"
#include "AX/AXVPB.h"
#include "MIX/mix.h"

/* size: 0xBA0 - the dB-to-volume curve and the pan curves of the mixer. */
typedef struct MIXTables {
    /* +0x000 */ u16 volume[968];   /* indexed by tenths of a decibel + 904 */
    /* +0x790 */ s32 pan[128];      /* stereo pan curve */
    /* +0x990 */ s16 panFront[128]; /* surround pan curve, front pair */
    /* +0xA90 */ s16 panRear[128];  /* surround pan curve, rear pair */
    /* +0xB90 */ u32 tail[4];
} MIXTables;

#define MIX_SOUND_MODE_SURROUND 3
#define MIX_CHANNEL_COUNT 96
#define MIX_LEVEL_FLOOR (-960)
#define MIX_UPDATE_INPUT 0x10000000
#define MIX_UPDATE_LEVELS 0x40000000

MIXTables MIXTable = {
    {
        0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2,
        2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
        2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
        2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
        3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4,
        4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5,
        5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6,
        6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7,
        7, 7, 7, 7, 7, 7, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
        9, 9, 9, 9, 9, 9, 9, 9, 9, 10, 10, 10, 10, 10, 10, 10,
        10, 10, 11, 11, 11, 11, 11, 11, 11, 12, 12, 12, 12, 12, 12, 12,
        13, 13, 13, 13, 13, 13, 13, 14, 14, 14, 14, 14, 14, 15, 15, 15,
        15, 15, 16, 16, 16, 16, 16, 17, 17, 17, 17, 17, 18, 18, 18, 18,
        18, 19, 19, 19, 19, 19, 20, 20, 20, 20, 21, 21, 21, 21, 22, 22,
        22, 22, 23, 23, 23, 24, 24, 24, 24, 25, 25, 25, 26, 26, 26, 26,
        27, 27, 27, 28, 28, 28, 29, 29, 29, 30, 30, 30, 31, 31, 32, 32,
        32, 33, 33, 33, 34, 34, 35, 35, 35, 36, 36, 37, 37, 38, 38, 38,
        39, 39, 40, 40, 41, 41, 42, 42, 43, 43, 44, 44, 45, 45, 46, 46,
        47, 47, 48, 49, 49, 50, 50, 51, 51, 52, 53, 53, 54, 55, 55, 56,
        56, 57, 58, 58, 59, 60, 61, 61, 62, 63, 63, 64, 65, 66, 66, 67,
        68, 69, 70, 70, 71, 72, 73, 74, 75, 75, 76, 77, 78, 79, 80, 81,
        82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97,
        98, 100, 101, 102, 103, 104, 106, 107, 108, 109, 111, 112, 113, 114, 116, 117,
        118, 120, 121, 123, 124, 126, 127, 128, 130, 131, 133, 135, 136, 138, 139, 141,
        143, 144, 146, 148, 149, 151, 153, 155, 156, 158, 160, 162, 164, 166, 168, 170,
        171, 173, 175, 178, 180, 182, 184, 186, 188, 190, 192, 195, 197, 199, 202, 204,
        206, 209, 211, 214, 216, 219, 221, 224, 226, 229, 231, 234, 237, 240, 242, 245,
        248, 251, 254, 257, 260, 263, 266, 269, 272, 275, 278, 282, 285, 288, 292, 295,
        298, 302, 305, 309, 312, 316, 320, 323, 327, 331, 335, 339, 343, 347, 351, 355,
        359, 363, 367, 371, 376, 380, 384, 389, 393, 398, 403, 407, 412, 417, 422, 427,
        431, 436, 442, 447, 452, 457, 462, 468, 473, 479, 484, 490, 495, 501, 507, 513,
        519, 525, 531, 537, 543, 550, 556, 562, 569, 576, 582, 589, 596, 603, 610, 617,
        624, 631, 638, 646, 653, 661, 669, 676, 684, 692, 700, 708, 716, 725, 733, 742,
        750, 759, 768, 777, 786, 795, 804, 813, 823, 832, 842, 852, 861, 871, 881, 892,
        902, 912, 923, 934, 945, 955, 967, 978, 989, 1001, 1012, 1024, 1036, 1048, 1060, 1072,
        1085, 1097, 1110, 1123, 1136, 1149, 1162, 1176, 1189, 1203, 1217, 1231, 1245, 1260, 1274, 1289,
        1304, 1319, 1334, 1350, 1365, 1381, 1397, 1414, 1430, 1446, 1463, 1480, 1497, 1515, 1532, 1550,
        1568, 1586, 1604, 1623, 1642, 1661, 1680, 1700, 1719, 1739, 1759, 1780, 1800, 1821, 1842, 1864,
        1885, 1907, 1929, 1951, 1974, 1997, 2020, 2043, 2067, 2091, 2115, 2140, 2164, 2190, 2215, 2241,
        2266, 2293, 2319, 2346, 2373, 2401, 2429, 2457, 2485, 2514, 2543, 2573, 2602, 2632, 2663, 2694,
        2725, 2757, 2789, 2821, 2853, 2887, 2920, 2954, 2988, 3023, 3058, 3093, 3129, 3165, 3202, 3239,
        3276, 3314, 3353, 3391, 3431, 3470, 3511, 3551, 3592, 3634, 3676, 3719, 3762, 3805, 3849, 3894,
        3939, 3985, 4031, 4078, 4125, 4173, 4221, 4270, 4319, 4369, 4420, 4471, 4523, 4575, 4628, 4682,
        4736, 4791, 4846, 4902, 4959, 5017, 5075, 5133, 5193, 5253, 5314, 5375, 5438, 5501, 5564, 5629,
        5694, 5760, 5827, 5894, 5962, 6031, 6101, 6172, 6243, 6316, 6389, 6463, 6538, 6613, 6690, 6767,
        6846, 6925, 7005, 7086, 7168, 7251, 7335, 7420, 7506, 7593, 7681, 7770, 7860, 7951, 8043, 8136,
        8230, 8326, 8422, 8520, 8618, 8718, 8819, 8921, 9025, 9129, 9235, 9342, 9450, 9559, 9670, 9782,
        9895, 10010, 10126, 10243, 10362, 10482, 10603, 10726, 10850, 10976, 11103, 11231, 11361, 11493, 11626, 11761,
        11897, 12035, 12174, 12315, 12458, 12602, 12748, 12895, 13045, 13196, 13349, 13503, 13659, 13818, 13978, 14140,
        14303, 14469, 14636, 14806, 14977, 15151, 15326, 15504, 15683, 15865, 16049, 16234, 16422, 16613, 16805, 17000,
        17196, 17396, 17597, 17801, 18007, 18215, 18426, 18640, 18856, 19074, 19295, 19518, 19744, 19973, 20204, 20438,
        20675, 20914, 21156, 21401, 21649, 21900, 22153, 22410, 22669, 22932, 23197, 23466, 23738, 24013, 24291, 24572,
        24857, 25144, 25436, 25730, 26028, 26329, 26634, 26943, 27255, 27570, 27890, 28213, 28539, 28870, 29204, 29542,
        29884, 30230, 30580, 30934, 31293, 31655, 32022, 32392, 32767, 33147, 33531, 33919, 34312, 34709, 35111, 35518,
        35929, 36345, 36766, 37192, 37622, 38058, 38499, 38944, 39395, 39851, 40313, 40780, 41252, 41730, 42213, 42702,
        43196, 43696, 44202, 44714, 45232, 45756, 46286, 46821, 47364, 47912, 48467, 49028, 49596, 50170, 50751, 51339,
        51933, 52535, 53143, 53758, 54381, 55011, 55648, 56292, 56944, 57603, 58270, 58945, 59627, 60318, 61016, 61723,
        62438, 63161, 63892, 64632, 65380, 0, 0, 0,
    },
    {
        0, 0, -1, -1, -1, -2, -2, -2, -3, -3, -4, -4, -4, -5, -5, -5,
        -6, -6, -7, -7, -7, -8, -8, -9, -9, -10, -10, -10, -11, -11, -12, -12,
        -13, -13, -14, -14, -14, -15, -15, -16, -16, -17, -17, -18, -18, -19, -20, -20,
        -21, -21, -22, -22, -23, -23, -24, -25, -25, -26, -26, -27, -28, -28, -29, -30,
        -30, -31, -32, -33, -33, -34, -35, -36, -36, -37, -38, -39, -40, -40, -41, -42,
        -43, -44, -45, -46, -47, -48, -49, -50, -51, -52, -54, -55, -56, -57, -59, -60,
        -61, -63, -64, -66, -67, -69, -71, -72, -74, -76, -78, -80, -83, -85, -87, -90,
        -93, -96, -99, -102, -106, -110, -115, -120, -126, -133, -140, -150, -163, -180, -210, -904,
    },
    {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1,
        -1, -1, -2, -2, -2, -2, -3, -3, -3, -4, -4, -4, -5, -5, -6, -6,
        -6, -7, -7, -8, -8, -9, -9, -10, -11, -11, -12, -12, -13, -14, -14, -15,
        -16, -17, -17, -18, -19, -20, -21, -21, -22, -23, -24, -25, -26, -27, -28, -29,
        -30, -31, -32, -34, -35, -36, -37, -38, -40, -41, -42, -44, -45, -47, -48, -50,
        -52, -53, -55, -57, -58, -60, -62, -64, -66, -68, -70, -73, -75, -77, -80, -82,
        -85, -88, -90, -93, -96, -100, -103, -106, -110, -114, -118, -122, -126, -131, -136, -141,
        -146, -152, -159, -166, -173, -181, -190, -201, -212, -225, -241, -261, -286, -321, -381, -960,
    },
    {
        -61, -61, -60, -59, -59, -58, -58, -57, -56, -56, -55, -55, -54, -53, -53, -52,
        -52, -51, -50, -50, -49, -49, -48, -48, -47, -47, -46, -46, -45, -45, -44, -44,
        -43, -43, -42, -42, -41, -41, -40, -40, -39, -39, -38, -38, -38, -37, -37, -36,
        -36, -35, -35, -35, -34, -34, -33, -33, -32, -32, -32, -31, -31, -31, -30, -30,
        -29, -29, -29, -28, -28, -28, -27, -27, -27, -26, -26, -26, -25, -25, -25, -24,
        -24, -24, -23, -23, -23, -22, -22, -22, -21, -21, -21, -20, -20, -20, -20, -19,
        -19, -19, -18, -18, -18, -18, -17, -17, -17, -17, -16, -16, -16, -16, -15, -15,
        -15, -15, -14, -14, -14, -14, -13, -13, -13, -13, -13, -12, -12, -12, -12, -11,
    },
    { 0, 0, 0, 0 },
};

MIXAuxState* MIXAuxStates;
u32 MIXSoundMode;
u32 MIXMaxVoices;
u32 MIXInitialized;
MIXChannel* MIXChannels;

MIXChannel MIXChannelArray[MIX_CHANNEL_COUNT];
MIXAuxState MIXAuxArray[MIX_CHANNEL_COUNT];

/* 0x804C25E0 (0xB4): fills `channel`'s gain row from the pan tables for the current sound mode. */
void __MIXSetPan(MIXChannel* channel)
{
    s32 pan = channel->pan;
    s32 span = channel->span;
    s32 panInv = 127 - pan;
    s32 spanInv = 127 - span;
    s16* front = MIXTable.panFront;
    s16* rear = MIXTable.panRear;
    s32* stereo = MIXTable.pan;

    if (MIXSoundMode == MIX_SOUND_MODE_SURROUND) {
        channel->gain[0] = front[pan];
        channel->gain[1] = front[panInv];
        channel->gain[2] = front[spanInv];
        channel->gain[3] = front[span];
        channel->gain[4] = rear[panInv];
        channel->gain[5] = rear[pan];
    } else {
        channel->gain[0] = stereo[pan];
        channel->gain[1] = stereo[panInv];
        channel->gain[2] = stereo[spanInv];
        channel->gain[3] = stereo[span];
        channel->gain[4] = 0;
        channel->gain[5] = 0;
    }
}

/* 0x804C26A0 (0x3C): turns tenths of a decibel into the table's 16-bit linear volume. */
u16 __MIXGetVolume(s32 tenthsDb)
{
    u16* volume;
    s32 index;

    if (tenthsDb <= -904) {
        return 0;
    }
    if (tenthsDb >= 60) {
        return 0xFF64;
    }
    index = tenthsDb + 904;
    volume = MIXTable.volume;
    return volume[index];
}

/* 0x804C26E0 (0x114): sets up the channel arrays for every AX voice; does nothing before AX is initialised. */
void MIXInit(void)
{
    u32 i;

    if (AXIsInit() && MIXInitialized == 0) {
        MIXMaxVoices = AXGetMaxVoices();
        MIXChannels = MIXChannelArray;
        MIXAuxStates = MIXAuxArray;
        for (i = 0; i < MIXMaxVoices; i++) {
            MIXChannel* channel;

            MIXChannels[i].mode = 0;
            channel = &MIXChannels[i];
            channel->update = 0x50000000;
            channel->input = 0;
            channel->auxA = MIX_LEVEL_FLOOR;
            channel->auxB = MIX_LEVEL_FLOOR;
            channel->auxC = MIX_LEVEL_FLOOR;
            channel->fader = 0;
            channel->pan = 64;
            channel->span = 127;
            channel->ramp[12].current = 0;
            channel->ramp[11].current = 0;
            channel->ramp[10].current = 0;
            channel->ramp[9].current = 0;
            channel->ramp[8].current = 0;
            channel->ramp[7].current = 0;
            channel->ramp[6].current = 0;
            channel->ramp[5].current = 0;
            channel->ramp[4].current = 0;
            channel->ramp[3].current = 0;
            channel->ramp[2].current = 0;
            channel->ramp[1].current = 0;
            channel->ramp[0].current = 0;
            __MIXSetPan(channel);
            __MIXResetAux(i);
        }
        MIXSoundMode = 1;
        MIXInitialized = 1;
    }
}

/* 0x804C2800 (0x14): clears the mixer state. */
void MIXQuit(void)
{
    MIXChannels = NULL;
    MIXAuxStates = NULL;
    MIXInitialized = 0;
}

/* 0x804C2820 (0x8): sets the sound mode. */
void MIXSetSoundMode(u32 mode)
{
    MIXSoundMode = mode;
}

/* 0x804C2830 (0x8): returns the sound mode. */
u32 MIXGetSoundMode(void)
{
    return MIXSoundMode;
}

/* 0x804C3F10 (0x18): frees the channel of `voice`. */
void MIXReleaseChannel(AXVPB* voice)
{
    MIXChannels[voice->index].mode = 0;
}

/* 0x804C3F30 (0x24): sets the input level of the voice's channel. */
void MIXSetInput(AXVPB* voice, s32 tenthsDb)
{
    MIXChannel* channel = &MIXChannels[voice->index];

    channel->input = tenthsDb;
    channel->update |= MIX_UPDATE_INPUT;
}

/* 0x804C3F60 (0x24): sets the auxiliary send A level of the voice's channel. */
void MIXSetAuxA(AXVPB* voice, s32 tenthsDb)
{
    MIXChannel* channel = &MIXChannels[voice->index];

    channel->auxA = tenthsDb;
    channel->update |= MIX_UPDATE_LEVELS;
}

/* 0x804C3F90 (0x24): sets the auxiliary send B level of the voice's channel. */
void MIXSetAuxB(AXVPB* voice, s32 tenthsDb)
{
    MIXChannel* channel = &MIXChannels[voice->index];

    channel->auxB = tenthsDb;
    channel->update |= MIX_UPDATE_LEVELS;
}

/* 0x804C3FC0 (0x6C): sets the pan of the voice's channel, clamped to 0..127, and refreshes its gains. */
void MIXSetPan(AXVPB* voice, s32 pan)
{
    MIXChannel* channel = &MIXChannels[voice->index];
    s32 clamped;

    if (pan < 0) {
        clamped = 0;
    } else {
        clamped = 127;
        if (pan <= 127) {
            clamped = pan;
        }
    }
    channel->pan = clamped;
    __MIXSetPan(channel);
    channel->update |= MIX_UPDATE_LEVELS;
}

/* 0x804C4030 (0x6C): sets the surround pan of the voice's channel, clamped to 0..127, and refreshes its gains. */
void MIXSetSPan(AXVPB* voice, s32 span)
{
    MIXChannel* channel = &MIXChannels[voice->index];
    s32 clamped;

    if (span < 0) {
        clamped = 0;
    } else {
        clamped = 127;
        if (span <= 127) {
            clamped = span;
        }
    }
    channel->span = clamped;
    __MIXSetPan(channel);
    channel->update |= MIX_UPDATE_LEVELS;
}

/* 0x804C40A0 (0x24): sets the fader level of the voice's channel. */
void MIXSetFader(AXVPB* voice, s32 tenthsDb)
{
    MIXChannel* channel = &MIXChannels[voice->index];

    channel->fader = tenthsDb;
    channel->update |= MIX_UPDATE_LEVELS;
}

/* 0x804C5BB0 (0x58): resets the aux ramp state of voice `index`. */
void __MIXResetAux(u32 index)
{
    MIXAuxState* aux = &MIXAuxStates[index];

    aux->flags = 0;
    aux->level[0] = 0;
    aux->level[1] = 0;
    aux->level[2] = 0;
    aux->level[3] = 0;
    aux->offset[0] = MIX_LEVEL_FLOOR;
    aux->offset[1] = MIX_LEVEL_FLOOR;
    aux->offset[2] = MIX_LEVEL_FLOOR;
    aux->offset[3] = MIX_LEVEL_FLOOR;
    aux->ramp[7].current = 0;
    aux->ramp[6].current = 0;
    aux->ramp[5].current = 0;
    aux->ramp[4].current = 0;
    aux->ramp[3].current = 0;
    aux->ramp[2].current = 0;
    aux->ramp[1].current = 0;
    aux->ramp[0].current = 0;
}
