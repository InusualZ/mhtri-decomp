/*
 * MIX/mix.c - the MIX audio mixer: per-voice channel levels, the volume/pan tables and the AX mix update.
 * RANGE. .text 0x804C25E0-0x804C5C10 (17 functions); .data 0x8061AE00-0x8061B9A0; .bss 0x80748BB8-0x8074CF40; .sbss
 *    0x807952A0-0x807952B8.  Cut from the OS core band 0x804C1760-0x804D9B4C; one TU: every function reads the
 *    0xBA0-byte table (.data 0x8061AE00), the channel arrays (.bss 0x80748BB8, 0x8074B5B8) or the mix state (.sbss
 *    0x807952A0..0x807952B4), and nothing outside the range reads them.
 * FLAGS. the `OS` lib block of configure.py (Wii/1.3) with per-object `cflags_base` (-O4,p -inline auto).
 *    `__MIXUpdateAuxVolumes` sits in `#pragma dont_inline on`: the target calls `__MIXGetVolume` out of line 12 times
 *    there while `MIXInitChannel` and `MIXUpdateSettings` inline it.
 * NAMES. `__MIXSetPan` is the dump's name; the file name `mix.c` is a GUESS.  GUESS (mix.h): `MIXInit`, `MIXQuit`,
 *    `MIXSetSoundMode`, `MIXGetSoundMode`, `MIXInitChannel`, `MIXReleaseChannel`, `MIXSetInput`, `MIXSetAuxA`,
 *    `MIXSetAuxB`, `MIXSetPan`, `MIXSetSPan`, `MIXSetFader`, `__MIXGetVolume`, `__MIXResetAux`, `MIXUpdateSettings`,
 *    `__MIXUpdateAuxVolumes`, the arrays `MIXChannelArray`/`MIXAuxArray`, the table `MIXTable`, the MIX_MODE_*,
 *    MIX_AUX_*, AX_MIXCTRL_* and AX_SYNC_* bit names and the MIXChannel/MIXAuxState field names (the layout is traced
 *    from the bodies: the mixer-control bit set per AXPB mix word, the sync bits OR-ed after each write).
 * RESIDUALS. `__MIXSetPan`: the target materialises the table base once and adds 0x990/0xA90/0x790 inside each branch
 *    (`addi` + `lhax`), the codegen of four file-static arrays pooled into one anonymous .data object; written as
 *    `static` arrays the function is byte-identical (relocations then name the pool, not `MIXTable`), but the map's
 *    single `MIXTable` row (0xBA0, global) stops pairing, so the source keeps the struct and its hoisted pointers.
 *    `MIXAuxStates` (.sbss 0x807952B0) is one 4-byte pointer and `MIXAuxArray` is 96 x 0x44 bytes; the word at
 *    0x807952B4 and the 8 bytes after the aux array are unreferenced alignment fill inside this unit's ranges.
 * SHAPES. the AXPB mix and remote-mix words are written through a post-incremented `u16*` (`*mix++`), which gives
 *    the target's `&mix[1]` base register; the level sums are spelled fader + aux + pan + surround, left to right;
 *    the locals of `MIXInitChannel` (`enabled` before `c`) and `MIXUpdateSettings` (`setMix` before `setInput`,
 *    initialised in the opposite order) are declared in the order that gives the target's registers.
 */

#include "types.h"
#include "AX/AX.h"
#include "AX/AXVPB.h"
#include "MIX/mix.h"
#include "OS/OSInterrupt.h"
#include "Runtime.PPCEABI.H/memset.h"

/* size: 0xBA0 - the dB-to-volume curve and the pan curves of the mixer. */
typedef struct MIXTables {
    /* +0x000 */ u16 volume[968];   /* indexed by tenths of a decibel + 904 */
    /* +0x790 */ s32 pan[128];      /* stereo pan curve */
    /* +0x990 */ s16 panFront[128]; /* surround pan curve, front pair */
    /* +0xA90 */ s16 panRear[128];  /* surround pan curve, rear pair */
    /* +0xB90 */ u32 pad_0xB90[4];
} MIXTables;

#define MIX_SOUND_MODE_MONO 0
#define MIX_SOUND_MODE_STEREO 1
#define MIX_SOUND_MODE_SURROUND 2
#define MIX_SOUND_MODE_DPL2 3
#define MIX_CHANNEL_COUNT 96
#define MIX_LEVEL_FLOOR (-960)

/* MIXChannel.mode: the option bits MIXInitChannel takes and the pending-update flags of MIXUpdateSettings. */
#define MIX_MODE_AUXA_PREFADER 0x00000001
#define MIX_MODE_AUXB_PREFADER 0x00000002
#define MIX_MODE_AUXC_PREFADER 0x00000004
#define MIX_MODE_MUTE 0x00000008
#define MIX_MODE_OPTIONS 0x0000000F
#define MIX_MODE_UPDATE_INPUT 0x10000000
#define MIX_MODE_UPDATE_INPUT1 0x20000000
#define MIX_MODE_UPDATE_MIX 0x40000000
#define MIX_MODE_UPDATE_MIX1 0x80000000

/* MIXAuxState.flags: recompute the targets, then move the volumes to them. */
#define MIX_AUX_UPDATE 0x40000000
#define MIX_AUX_UPDATE1 0x80000000

/* The AXPB mixer-control bits, the AXVPB sync bits and the samples of one audio frame. */
#define AX_MIXCTRL_L 0x00000001
#define AX_MIXCTRL_R 0x00000002
#define AX_MIXCTRL_LR_RAMP 0x00000004
#define AX_MIXCTRL_S 0x00000008
#define AX_MIXCTRL_S_RAMP 0x00000010
#define AX_MIXCTRL_AL 0x00010000
#define AX_MIXCTRL_AR 0x00020000
#define AX_MIXCTRL_ALR_RAMP 0x00040000
#define AX_MIXCTRL_AS 0x00080000
#define AX_MIXCTRL_AS_RAMP 0x00100000
#define AX_MIXCTRL_BL 0x00200000
#define AX_MIXCTRL_BR 0x00400000
#define AX_MIXCTRL_BLR_RAMP 0x00800000
#define AX_MIXCTRL_BS 0x01000000
#define AX_MIXCTRL_BS_RAMP 0x02000000
#define AX_MIXCTRL_CL 0x04000000
#define AX_MIXCTRL_CR 0x08000000
#define AX_MIXCTRL_CLR_RAMP 0x10000000
#define AX_MIXCTRL_CS 0x20000000
#define AX_MIXCTRL_CS_RAMP 0x40000000
#define AX_MIXCTRL_DPL2 0x80000000
#define AX_SYNC_MIXCTRL 0x00000002
#define AX_SYNC_MIX 0x00000010
#define AX_SYNC_VE 0x00000100
#define AX_SYNC_RMTMIXCTRL 0x01000000
#define AX_SYNC_RMTMIX 0x02000000
#define AX_FRAME_SAMPLES 96
#define AX_RMT_FRAME_SAMPLES 18

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

void __MIXUpdateAuxVolumes(u32 index, AXVPB* axvpb);

/* 0x804C25E0 (0xB4): fills `c`'s pan attenuations from the pan tables for the current sound mode. */
void __MIXSetPan(MIXChannel* c)
{
    s32 pan = c->pan;
    s32 span = c->span;
    s32 ipan = 127 - pan;
    s32 ispan = 127 - span;
    s16* front = MIXTable.panFront;
    s16* rear = MIXTable.panRear;
    s32* stereo = MIXTable.pan;

    if (MIXSoundMode == MIX_SOUND_MODE_DPL2) {
        c->l = front[pan];
        c->r = front[ipan];
        c->f = front[ispan];
        c->b = front[span];
        c->l1 = rear[ipan];
        c->r1 = rear[pan];
    } else {
        c->l = stereo[pan];
        c->r = stereo[ipan];
        c->f = stereo[ispan];
        c->b = stereo[span];
        c->l1 = 0;
        c->r1 = 0;
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

            MIXChannels[i].axvpb = NULL;
            channel = &MIXChannels[i];
            channel->mode = MIX_MODE_UPDATE_INPUT | MIX_MODE_UPDATE_MIX;
            channel->input = 0;
            channel->auxA = MIX_LEVEL_FLOOR;
            channel->auxB = MIX_LEVEL_FLOOR;
            channel->auxC = MIX_LEVEL_FLOOR;
            channel->fader = 0;
            channel->pan = 64;
            channel->span = 127;
            channel->vCS.current = 0;
            channel->vCR.current = 0;
            channel->vCL.current = 0;
            channel->vBS.current = 0;
            channel->vBR.current = 0;
            channel->vBL.current = 0;
            channel->vAS.current = 0;
            channel->vAR.current = 0;
            channel->vAL.current = 0;
            channel->vS.current = 0;
            channel->vR.current = 0;
            channel->vL.current = 0;
            channel->v.current = 0;
            __MIXSetPan(channel);
            __MIXResetAux(i);
        }
        MIXSoundMode = MIX_SOUND_MODE_STEREO;
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

/* 0x804C2840 (0x16C4): attaches the channel of `axvpb` to it with the given levels and writes its mix to the voice. */
void MIXInitChannel(AXVPB* axvpb, u32 mode, s32 input, s32 auxA, s32 auxB, s32 auxC, s32 pan, s32 span, s32 fader)
{
    BOOL enabled;
    MIXChannel* c;
    u32 mixerCtrl;
    u16* mix;

    c = &MIXChannels[axvpb->index];
    c->axvpb = axvpb;
    c->mode = mode & MIX_MODE_OPTIONS;
    c->input = input;
    c->auxA = auxA;
    c->auxB = auxB;
    c->auxC = auxC;
    c->pan = pan;
    c->span = span;
    c->fader = fader;
    __MIXSetPan(c);

    if (c->mode & MIX_MODE_MUTE) {
        c->v.current = 0;
    } else {
        c->v.current = __MIXGetVolume(input);
    }

    mixerCtrl = 0;
    switch (MIXSoundMode) {
    case MIX_SOUND_MODE_MONO:
        c->vL.current = __MIXGetVolume(c->fader + c->f);
        c->vR.current = __MIXGetVolume(c->fader + c->f);
        c->vS.current = __MIXGetVolume(c->fader + c->b - 30);
        if (c->mode & MIX_MODE_AUXA_PREFADER) {
            c->vAL.current = __MIXGetVolume(c->auxA + c->f);
            c->vAR.current = __MIXGetVolume(c->auxA + c->f);
            c->vAS.current = __MIXGetVolume(c->auxA + c->b - 30);
        } else {
            c->vAL.current = __MIXGetVolume(c->fader + c->auxA + c->f);
            c->vAR.current = __MIXGetVolume(c->fader + c->auxA + c->f);
            c->vAS.current = __MIXGetVolume(c->fader + c->auxA + c->b - 30);
        }
        if (c->mode & MIX_MODE_AUXB_PREFADER) {
            c->vBL.current = __MIXGetVolume(c->auxB + c->f);
            c->vBR.current = __MIXGetVolume(c->auxB + c->f);
            c->vBS.current = __MIXGetVolume(c->auxB + c->b - 30);
        } else {
            c->vBL.current = __MIXGetVolume(c->fader + c->auxB + c->f);
            c->vBR.current = __MIXGetVolume(c->fader + c->auxB + c->f);
            c->vBS.current = __MIXGetVolume(c->fader + c->auxB + c->b - 30);
        }
        if (c->mode & MIX_MODE_AUXC_PREFADER) {
            c->vCL.current = __MIXGetVolume(c->auxC + c->f);
            c->vCR.current = __MIXGetVolume(c->auxC + c->f);
            c->vCS.current = __MIXGetVolume(c->auxC + c->b - 30);
        } else {
            c->vCL.current = __MIXGetVolume(c->fader + c->auxC + c->f);
            c->vCR.current = __MIXGetVolume(c->fader + c->auxC + c->f);
            c->vCS.current = __MIXGetVolume(c->fader + c->auxC + c->b - 30);
        }
        break;
    case MIX_SOUND_MODE_STEREO:
    case MIX_SOUND_MODE_SURROUND:
        c->vL.current = __MIXGetVolume(c->fader + c->l + c->f);
        c->vR.current = __MIXGetVolume(c->fader + c->r + c->f);
        c->vS.current = __MIXGetVolume(c->fader + c->b - 30);
        if (c->mode & MIX_MODE_AUXA_PREFADER) {
            c->vAL.current = __MIXGetVolume(c->auxA + c->l + c->f);
            c->vAR.current = __MIXGetVolume(c->auxA + c->r + c->f);
            c->vAS.current = __MIXGetVolume(c->auxA + c->b - 30);
        } else {
            c->vAL.current = __MIXGetVolume(c->fader + c->auxA + c->l + c->f);
            c->vAR.current = __MIXGetVolume(c->fader + c->auxA + c->r + c->f);
            c->vAS.current = __MIXGetVolume(c->fader + c->auxA + c->b - 30);
        }
        if (c->mode & MIX_MODE_AUXB_PREFADER) {
            c->vBL.current = __MIXGetVolume(c->auxB + c->l + c->f);
            c->vBR.current = __MIXGetVolume(c->auxB + c->r + c->f);
            c->vBS.current = __MIXGetVolume(c->auxB + c->b - 30);
        } else {
            c->vBL.current = __MIXGetVolume(c->fader + c->auxB + c->l + c->f);
            c->vBR.current = __MIXGetVolume(c->fader + c->auxB + c->r + c->f);
            c->vBS.current = __MIXGetVolume(c->fader + c->auxB + c->b - 30);
        }
        if (c->mode & MIX_MODE_AUXC_PREFADER) {
            c->vCL.current = __MIXGetVolume(c->auxC + c->l + c->f);
            c->vCR.current = __MIXGetVolume(c->auxC + c->r + c->f);
            c->vCS.current = __MIXGetVolume(c->auxC + c->b - 30);
        } else {
            c->vCL.current = __MIXGetVolume(c->fader + c->auxC + c->l + c->f);
            c->vCR.current = __MIXGetVolume(c->fader + c->auxC + c->r + c->f);
            c->vCS.current = __MIXGetVolume(c->fader + c->auxC + c->b - 30);
        }
        break;
    case MIX_SOUND_MODE_DPL2:
        c->vL.current = __MIXGetVolume(c->fader + c->l + c->f);
        c->vR.current = __MIXGetVolume(c->fader + c->r + c->f);
        c->vS.current = __MIXGetVolume(c->fader + c->l1 + c->b);
        c->vCL.current = __MIXGetVolume(c->fader + c->r1 + c->b);
        if (c->mode & MIX_MODE_AUXA_PREFADER) {
            c->vAL.current = __MIXGetVolume(c->auxA + c->l + c->f);
            c->vAR.current = __MIXGetVolume(c->auxA + c->r + c->f);
            c->vAS.current = __MIXGetVolume(c->auxA + c->l1 + c->b);
            c->vCR.current = __MIXGetVolume(c->auxA + c->r1 + c->b);
        } else {
            c->vAL.current = __MIXGetVolume(c->fader + c->auxA + c->l + c->f);
            c->vAR.current = __MIXGetVolume(c->fader + c->auxA + c->r + c->f);
            c->vAS.current = __MIXGetVolume(c->fader + c->auxA + c->l1 + c->b);
            c->vCR.current = __MIXGetVolume(c->fader + c->auxA + c->r1 + c->b);
        }
        if (c->mode & MIX_MODE_AUXB_PREFADER) {
            c->vBL.current = __MIXGetVolume(c->auxB + c->l + c->f);
            c->vBR.current = __MIXGetVolume(c->auxB + c->r + c->f);
            c->vBS.current = __MIXGetVolume(c->auxB + c->l1 + c->b);
            c->vCS.current = __MIXGetVolume(c->auxB + c->r1 + c->b);
        } else {
            c->vBL.current = __MIXGetVolume(c->fader + c->auxB + c->l + c->f);
            c->vBR.current = __MIXGetVolume(c->fader + c->auxB + c->r + c->f);
            c->vBS.current = __MIXGetVolume(c->fader + c->auxB + c->l1 + c->b);
            c->vCS.current = __MIXGetVolume(c->fader + c->auxB + c->r1 + c->b);
        }
        mixerCtrl |= AX_MIXCTRL_DPL2;
        break;
    }

    __MIXResetAux(axvpb->index);
    enabled = OSDisableInterrupts();
    axvpb->pb.veVolume = c->v.current;
    axvpb->pb.veDelta = 0;
    mix = axvpb->pb.mix;
    if ((*mix++ = c->vL.current) != 0) {
        mixerCtrl |= AX_MIXCTRL_L;
    }
    *mix++ = 0;
    if ((*mix++ = c->vR.current) != 0) {
        mixerCtrl |= AX_MIXCTRL_R;
    }
    *mix++ = 0;
    if ((*mix++ = c->vAL.current) != 0) {
        mixerCtrl |= AX_MIXCTRL_AL;
    }
    *mix++ = 0;
    if ((*mix++ = c->vAR.current) != 0) {
        mixerCtrl |= AX_MIXCTRL_AR;
    }
    *mix++ = 0;
    if ((*mix++ = c->vBL.current) != 0) {
        mixerCtrl |= AX_MIXCTRL_BL;
    }
    *mix++ = 0;
    if ((*mix++ = c->vBR.current) != 0) {
        mixerCtrl |= AX_MIXCTRL_BR;
    }
    *mix++ = 0;
    if ((*mix++ = c->vCL.current) != 0) {
        mixerCtrl |= AX_MIXCTRL_CL;
    }
    *mix++ = 0;
    if ((*mix++ = c->vCR.current) != 0) {
        mixerCtrl |= AX_MIXCTRL_CR;
    }
    *mix++ = 0;
    if ((*mix++ = c->vS.current) != 0) {
        mixerCtrl |= AX_MIXCTRL_S;
    }
    *mix++ = 0;
    if ((*mix++ = c->vAS.current) != 0) {
        mixerCtrl |= AX_MIXCTRL_AS;
    }
    *mix++ = 0;
    if ((*mix++ = c->vBS.current) != 0) {
        mixerCtrl |= AX_MIXCTRL_BS;
    }
    *mix++ = 0;
    if ((*mix++ = c->vCS.current) != 0) {
        mixerCtrl |= AX_MIXCTRL_CS;
    }
    *mix++ = 0;
    axvpb->pb.stateWord = mixerCtrl;
    axvpb->sync |= AX_SYNC_MIXCTRL | AX_SYNC_MIX | AX_SYNC_VE;
    memset(axvpb->pb.rmtMix, 0, sizeof(axvpb->pb.rmtMix));
    axvpb->pb.rmtSelect = 0;
    axvpb->sync |= AX_SYNC_RMTMIXCTRL | AX_SYNC_RMTMIX;
    OSRestoreInterrupts(enabled);
}

/* 0x804C3F10 (0x18): frees the channel of `voice`. */
void MIXReleaseChannel(AXVPB* voice)
{
    MIXChannels[voice->index].axvpb = NULL;
}

/* 0x804C3F30 (0x24): sets the input level of the voice's channel. */
void MIXSetInput(AXVPB* voice, s32 tenthsDb)
{
    MIXChannel* channel = &MIXChannels[voice->index];

    channel->input = tenthsDb;
    channel->mode |= MIX_MODE_UPDATE_INPUT;
}

/* 0x804C3F60 (0x24): sets the auxiliary send A level of the voice's channel. */
void MIXSetAuxA(AXVPB* voice, s32 tenthsDb)
{
    MIXChannel* channel = &MIXChannels[voice->index];

    channel->auxA = tenthsDb;
    channel->mode |= MIX_MODE_UPDATE_MIX;
}

/* 0x804C3F90 (0x24): sets the auxiliary send B level of the voice's channel. */
void MIXSetAuxB(AXVPB* voice, s32 tenthsDb)
{
    MIXChannel* channel = &MIXChannels[voice->index];

    channel->auxB = tenthsDb;
    channel->mode |= MIX_MODE_UPDATE_MIX;
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
    channel->mode |= MIX_MODE_UPDATE_MIX;
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
    channel->mode |= MIX_MODE_UPDATE_MIX;
}

/* 0x804C40A0 (0x24): sets the fader level of the voice's channel. */
void MIXSetFader(AXVPB* voice, s32 tenthsDb)
{
    MIXChannel* channel = &MIXChannels[voice->index];

    channel->fader = tenthsDb;
    channel->mode |= MIX_MODE_UPDATE_MIX;
}

/* 0x804C40D0 (0x1694): moves every channel's volumes towards the levels set since the last frame and writes them to its voice. */
void MIXUpdateSettings(void)
{
    u32 i;

    if (MIXInitialized == 0) {
        return;
    }
    for (i = 0; i < MIXMaxVoices; i++) {
        BOOL setMix;
        BOOL setInput;
        MIXChannel* c;
        AXVPB* axvpb;
        u32 mixerCtrl;
        u16* mix;

        setInput = FALSE;
        setMix = FALSE;
        c = &MIXChannels[i];
        axvpb = c->axvpb;
        if (axvpb == NULL) {
            continue;
        }
        mixerCtrl = 0;
        if (c->mode & MIX_MODE_UPDATE_INPUT1) {
            c->v.current = c->v.target;
            c->mode &= ~MIX_MODE_UPDATE_INPUT1;
            setInput = TRUE;
        }
        if (c->mode & MIX_MODE_UPDATE_INPUT) {
            if (c->mode & MIX_MODE_MUTE) {
                c->v.target = 0;
            } else {
                c->v.target = __MIXGetVolume(c->input);
            }
            c->mode &= ~MIX_MODE_UPDATE_INPUT;
            c->mode |= MIX_MODE_UPDATE_INPUT1;
            setInput = TRUE;
        }
        if (c->mode & MIX_MODE_UPDATE_MIX1) {
            c->vL.current = c->vL.target;
            c->vR.current = c->vR.target;
            c->vS.current = c->vS.target;
            c->vAL.current = c->vAL.target;
            c->vAR.current = c->vAR.target;
            c->vAS.current = c->vAS.target;
            c->vBL.current = c->vBL.target;
            c->vBR.current = c->vBR.target;
            c->vBS.current = c->vBS.target;
            c->vCL.current = c->vCL.target;
            c->vCR.current = c->vCR.target;
            c->vCS.current = c->vCS.target;
            c->mode &= ~MIX_MODE_UPDATE_MIX1;
            setMix = TRUE;
        }
        if (c->mode & MIX_MODE_UPDATE_MIX) {
            switch (MIXSoundMode) {
            case MIX_SOUND_MODE_MONO:
                c->vL.target = __MIXGetVolume(c->fader + c->f);
                c->vR.target = __MIXGetVolume(c->fader + c->f);
                c->vS.target = __MIXGetVolume(c->fader + c->b - 30);
                if (c->mode & MIX_MODE_AUXA_PREFADER) {
                    c->vAL.target = __MIXGetVolume(c->auxA + c->f);
                    c->vAR.target = __MIXGetVolume(c->auxA + c->f);
                    c->vAS.target = __MIXGetVolume(c->auxA + c->b - 30);
                } else {
                    c->vAL.target = __MIXGetVolume(c->fader + c->auxA + c->f);
                    c->vAR.target = __MIXGetVolume(c->fader + c->auxA + c->f);
                    c->vAS.target = __MIXGetVolume(c->fader + c->auxA + c->b - 30);
                }
                if (c->mode & MIX_MODE_AUXB_PREFADER) {
                    c->vBL.target = __MIXGetVolume(c->auxB + c->f);
                    c->vBR.target = __MIXGetVolume(c->auxB + c->f);
                    c->vBS.target = __MIXGetVolume(c->auxB + c->b - 30);
                } else {
                    c->vBL.target = __MIXGetVolume(c->fader + c->auxB + c->f);
                    c->vBR.target = __MIXGetVolume(c->fader + c->auxB + c->f);
                    c->vBS.target = __MIXGetVolume(c->fader + c->auxB + c->b - 30);
                }
                if (c->mode & MIX_MODE_AUXC_PREFADER) {
                    c->vCL.target = __MIXGetVolume(c->auxC + c->f);
                    c->vCR.target = __MIXGetVolume(c->auxC + c->f);
                    c->vCS.target = __MIXGetVolume(c->auxC + c->b - 30);
                } else {
                    c->vCL.target = __MIXGetVolume(c->fader + c->auxC + c->f);
                    c->vCR.target = __MIXGetVolume(c->fader + c->auxC + c->f);
                    c->vCS.target = __MIXGetVolume(c->fader + c->auxC + c->b - 30);
                }
                break;
            case MIX_SOUND_MODE_STEREO:
            case MIX_SOUND_MODE_SURROUND:
                c->vL.target = __MIXGetVolume(c->fader + c->l + c->f);
                c->vR.target = __MIXGetVolume(c->fader + c->r + c->f);
                c->vS.target = __MIXGetVolume(c->fader + c->b - 30);
                if (c->mode & MIX_MODE_AUXA_PREFADER) {
                    c->vAL.target = __MIXGetVolume(c->auxA + c->l + c->f);
                    c->vAR.target = __MIXGetVolume(c->auxA + c->r + c->f);
                    c->vAS.target = __MIXGetVolume(c->auxA + c->b - 30);
                } else {
                    c->vAL.target = __MIXGetVolume(c->fader + c->auxA + c->l + c->f);
                    c->vAR.target = __MIXGetVolume(c->fader + c->auxA + c->r + c->f);
                    c->vAS.target = __MIXGetVolume(c->fader + c->auxA + c->b - 30);
                }
                if (c->mode & MIX_MODE_AUXB_PREFADER) {
                    c->vBL.target = __MIXGetVolume(c->auxB + c->l + c->f);
                    c->vBR.target = __MIXGetVolume(c->auxB + c->r + c->f);
                    c->vBS.target = __MIXGetVolume(c->auxB + c->b - 30);
                } else {
                    c->vBL.target = __MIXGetVolume(c->fader + c->auxB + c->l + c->f);
                    c->vBR.target = __MIXGetVolume(c->fader + c->auxB + c->r + c->f);
                    c->vBS.target = __MIXGetVolume(c->fader + c->auxB + c->b - 30);
                }
                if (c->mode & MIX_MODE_AUXC_PREFADER) {
                    c->vCL.target = __MIXGetVolume(c->auxC + c->l + c->f);
                    c->vCR.target = __MIXGetVolume(c->auxC + c->r + c->f);
                    c->vCS.target = __MIXGetVolume(c->auxC + c->b - 30);
                } else {
                    c->vCL.target = __MIXGetVolume(c->fader + c->auxC + c->l + c->f);
                    c->vCR.target = __MIXGetVolume(c->fader + c->auxC + c->r + c->f);
                    c->vCS.target = __MIXGetVolume(c->fader + c->auxC + c->b - 30);
                }
                break;
            case MIX_SOUND_MODE_DPL2:
                c->vL.target = __MIXGetVolume(c->fader + c->l + c->f);
                c->vR.target = __MIXGetVolume(c->fader + c->r + c->f);
                c->vS.target = __MIXGetVolume(c->fader + c->l1 + c->b);
                c->vCL.target = __MIXGetVolume(c->fader + c->r1 + c->b);
                if (c->mode & MIX_MODE_AUXA_PREFADER) {
                    c->vAL.target = __MIXGetVolume(c->auxA + c->l + c->f);
                    c->vAR.target = __MIXGetVolume(c->auxA + c->r + c->f);
                    c->vAS.target = __MIXGetVolume(c->auxA + c->l1 + c->b);
                    c->vCR.target = __MIXGetVolume(c->auxA + c->r1 + c->b);
                } else {
                    c->vAL.target = __MIXGetVolume(c->fader + c->auxA + c->l + c->f);
                    c->vAR.target = __MIXGetVolume(c->fader + c->auxA + c->r + c->f);
                    c->vAS.target = __MIXGetVolume(c->fader + c->auxA + c->l1 + c->b);
                    c->vCR.target = __MIXGetVolume(c->fader + c->auxA + c->r1 + c->b);
                }
                if (c->mode & MIX_MODE_AUXB_PREFADER) {
                    c->vBL.target = __MIXGetVolume(c->auxB + c->l + c->f);
                    c->vBR.target = __MIXGetVolume(c->auxB + c->r + c->f);
                    c->vBS.target = __MIXGetVolume(c->auxB + c->l1 + c->b);
                    c->vCS.target = __MIXGetVolume(c->auxB + c->r1 + c->b);
                } else {
                    c->vBL.target = __MIXGetVolume(c->fader + c->auxB + c->l + c->f);
                    c->vBR.target = __MIXGetVolume(c->fader + c->auxB + c->r + c->f);
                    c->vBS.target = __MIXGetVolume(c->fader + c->auxB + c->l1 + c->b);
                    c->vCS.target = __MIXGetVolume(c->fader + c->auxB + c->r1 + c->b);
                }
                mixerCtrl |= AX_MIXCTRL_DPL2;
                break;
            }
            c->mode &= ~MIX_MODE_UPDATE_MIX;
            c->mode |= MIX_MODE_UPDATE_MIX1;
            setMix = TRUE;
        }
        if (setInput) {
            axvpb->pb.veVolume = c->v.current;
            axvpb->pb.veDelta = (c->v.target - c->v.current) / AX_FRAME_SAMPLES;
            axvpb->sync |= AX_SYNC_VE;
        }
        if (setMix) {
            mix = axvpb->pb.mix;
            if ((*mix++ = c->vL.current) != 0) {
                mixerCtrl |= AX_MIXCTRL_L;
            }
            if ((*mix++ = (c->vL.target - c->vL.current) / AX_FRAME_SAMPLES) != 0) {
                mixerCtrl |= AX_MIXCTRL_LR_RAMP;
            }
            if ((*mix++ = c->vR.current) != 0) {
                mixerCtrl |= AX_MIXCTRL_R;
            }
            if ((*mix++ = (c->vR.target - c->vR.current) / AX_FRAME_SAMPLES) != 0) {
                mixerCtrl |= AX_MIXCTRL_LR_RAMP;
            }
            if ((*mix++ = c->vAL.current) != 0) {
                mixerCtrl |= AX_MIXCTRL_AL;
            }
            if ((*mix++ = (c->vAL.target - c->vAL.current) / AX_FRAME_SAMPLES) != 0) {
                mixerCtrl |= AX_MIXCTRL_ALR_RAMP;
            }
            if ((*mix++ = c->vAR.current) != 0) {
                mixerCtrl |= AX_MIXCTRL_AR;
            }
            if ((*mix++ = (c->vAR.target - c->vAR.current) / AX_FRAME_SAMPLES) != 0) {
                mixerCtrl |= AX_MIXCTRL_ALR_RAMP;
            }
            if ((*mix++ = c->vBL.current) != 0) {
                mixerCtrl |= AX_MIXCTRL_BL;
            }
            if ((*mix++ = (c->vBL.target - c->vBL.current) / AX_FRAME_SAMPLES) != 0) {
                mixerCtrl |= AX_MIXCTRL_BLR_RAMP;
            }
            if ((*mix++ = c->vBR.current) != 0) {
                mixerCtrl |= AX_MIXCTRL_BR;
            }
            if ((*mix++ = (c->vBR.target - c->vBR.current) / AX_FRAME_SAMPLES) != 0) {
                mixerCtrl |= AX_MIXCTRL_BLR_RAMP;
            }
            if ((*mix++ = c->vCL.current) != 0) {
                mixerCtrl |= AX_MIXCTRL_CL;
            }
            if ((*mix++ = (c->vCL.target - c->vCL.current) / AX_FRAME_SAMPLES) != 0) {
                mixerCtrl |= AX_MIXCTRL_CLR_RAMP;
            }
            if ((*mix++ = c->vCR.current) != 0) {
                mixerCtrl |= AX_MIXCTRL_CR;
            }
            if ((*mix++ = (c->vCR.target - c->vCR.current) / AX_FRAME_SAMPLES) != 0) {
                mixerCtrl |= AX_MIXCTRL_CLR_RAMP;
            }
            if ((*mix++ = c->vS.current) != 0) {
                mixerCtrl |= AX_MIXCTRL_S;
            }
            if ((*mix++ = (c->vS.target - c->vS.current) / AX_FRAME_SAMPLES) != 0) {
                mixerCtrl |= AX_MIXCTRL_S_RAMP;
            }
            if ((*mix++ = c->vAS.current) != 0) {
                mixerCtrl |= AX_MIXCTRL_AS;
            }
            if ((*mix++ = (c->vAS.target - c->vAS.current) / AX_FRAME_SAMPLES) != 0) {
                mixerCtrl |= AX_MIXCTRL_AS_RAMP;
            }
            if ((*mix++ = c->vBS.current) != 0) {
                mixerCtrl |= AX_MIXCTRL_BS;
            }
            if ((*mix++ = (c->vBS.target - c->vBS.current) / AX_FRAME_SAMPLES) != 0) {
                mixerCtrl |= AX_MIXCTRL_BS_RAMP;
            }
            if ((*mix++ = c->vCS.current) != 0) {
                mixerCtrl |= AX_MIXCTRL_CS;
            }
            if ((*mix++ = (c->vCS.target - c->vCS.current) / AX_FRAME_SAMPLES) != 0) {
                mixerCtrl |= AX_MIXCTRL_CS_RAMP;
            }
            axvpb->pb.stateWord = mixerCtrl;
            axvpb->sync |= AX_SYNC_MIXCTRL | AX_SYNC_MIX;
        }
        __MIXUpdateAuxVolumes(i, axvpb);
    }
}

/* 0x804C5770 (0x43C): moves the remote-speaker volumes of voice `index` towards its levels and writes them to `axvpb`. */
#pragma push
#pragma dont_inline on
void __MIXUpdateAuxVolumes(u32 index, AXVPB* axvpb)
{
    MIXAuxState* aux = &MIXAuxStates[index];
    u16 mixerCtrl;
    u16* mix;

    if ((aux->flags & (MIX_AUX_UPDATE | MIX_AUX_UPDATE1)) == 0) {
        return;
    }
    if (aux->flags & MIX_AUX_UPDATE1) {
        aux->ramp[0].current = aux->ramp[0].target;
        aux->ramp[1].current = aux->ramp[1].target;
        aux->ramp[2].current = aux->ramp[2].target;
        aux->ramp[3].current = aux->ramp[3].target;
        aux->ramp[4].current = aux->ramp[4].target;
        aux->ramp[5].current = aux->ramp[5].target;
        aux->ramp[6].current = aux->ramp[6].target;
        aux->ramp[7].current = aux->ramp[7].target;
        aux->flags &= ~MIX_AUX_UPDATE1;
    }
    if (aux->flags & MIX_AUX_UPDATE) {
        aux->ramp[0].target = __MIXGetVolume(aux->level[0]);
        aux->ramp[2].target = __MIXGetVolume(aux->level[1]);
        aux->ramp[4].target = __MIXGetVolume(aux->level[2]);
        aux->ramp[6].target = __MIXGetVolume(aux->level[3]);
        if (aux->flags & 1) {
            aux->ramp[1].target = __MIXGetVolume(aux->offset[0]);
        } else {
            aux->ramp[1].target = __MIXGetVolume(aux->level[0] + aux->offset[0]);
        }
        if (aux->flags & 2) {
            aux->ramp[3].target = __MIXGetVolume(aux->offset[1]);
        } else {
            aux->ramp[3].target = __MIXGetVolume(aux->level[1] + aux->offset[1]);
        }
        if (aux->flags & 4) {
            aux->ramp[5].target = __MIXGetVolume(aux->offset[2]);
        } else {
            aux->ramp[5].target = __MIXGetVolume(aux->level[2] + aux->offset[2]);
        }
        if (aux->flags & 8) {
            aux->ramp[7].target = __MIXGetVolume(aux->offset[3]);
        } else {
            aux->ramp[7].target = __MIXGetVolume(aux->level[3] + aux->offset[3]);
        }
        aux->flags &= ~MIX_AUX_UPDATE;
        aux->flags |= MIX_AUX_UPDATE1;
    }
    mixerCtrl = 0;
    mix = axvpb->pb.rmtMix;
    if ((*mix++ = aux->ramp[0].current) != 0) {
        mixerCtrl |= 0x1;
    }
    if ((*mix++ = (aux->ramp[0].target - aux->ramp[0].current) / AX_RMT_FRAME_SAMPLES) != 0) {
        mixerCtrl |= 0x2;
    }
    if ((*mix++ = aux->ramp[1].current) != 0) {
        mixerCtrl |= 0x4;
    }
    if ((*mix++ = (aux->ramp[1].target - aux->ramp[1].current) / AX_RMT_FRAME_SAMPLES) != 0) {
        mixerCtrl |= 0x8;
    }
    if ((*mix++ = aux->ramp[2].current) != 0) {
        mixerCtrl |= 0x10;
    }
    if ((*mix++ = (aux->ramp[2].target - aux->ramp[2].current) / AX_RMT_FRAME_SAMPLES) != 0) {
        mixerCtrl |= 0x20;
    }
    if ((*mix++ = aux->ramp[3].current) != 0) {
        mixerCtrl |= 0x40;
    }
    if ((*mix++ = (aux->ramp[3].target - aux->ramp[3].current) / AX_RMT_FRAME_SAMPLES) != 0) {
        mixerCtrl |= 0x80;
    }
    if ((*mix++ = aux->ramp[4].current) != 0) {
        mixerCtrl |= 0x100;
    }
    if ((*mix++ = (aux->ramp[4].target - aux->ramp[4].current) / AX_RMT_FRAME_SAMPLES) != 0) {
        mixerCtrl |= 0x200;
    }
    if ((*mix++ = aux->ramp[5].current) != 0) {
        mixerCtrl |= 0x400;
    }
    if ((*mix++ = (aux->ramp[5].target - aux->ramp[5].current) / AX_RMT_FRAME_SAMPLES) != 0) {
        mixerCtrl |= 0x800;
    }
    if ((*mix++ = aux->ramp[6].current) != 0) {
        mixerCtrl |= 0x1000;
    }
    if ((*mix++ = (aux->ramp[6].target - aux->ramp[6].current) / AX_RMT_FRAME_SAMPLES) != 0) {
        mixerCtrl |= 0x2000;
    }
    if ((*mix++ = aux->ramp[7].current) != 0) {
        mixerCtrl |= 0x4000;
    }
    if ((*mix++ = (aux->ramp[7].target - aux->ramp[7].current) / AX_RMT_FRAME_SAMPLES) != 0) {
        mixerCtrl |= 0x8000;
    }
    axvpb->pb.rmtSelect = mixerCtrl;
    axvpb->sync |= AX_SYNC_RMTMIXCTRL | AX_SYNC_RMTMIX;
}
#pragma pop

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
