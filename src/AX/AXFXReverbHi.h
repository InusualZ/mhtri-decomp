/*
 * AX/AXFXReverbHi.h - the AXFX reverb body shared by the AXFXReverbHi, AXFXReverbHiExp and AXFXReverbStd units.
 */
#ifndef AX_AXFXREVERBHI_H
#define AX_AXFXREVERBHI_H

#include "types.h"

typedef struct AXFXBuffer {
    /* +0x00 */ s32* out0;
    /* +0x04 */ s32* out1;
    /* +0x08 */ s32* out2;
} AXFXBuffer; /* size: 0x0C */

/*
 * One AXFX reverb body (0x3E8 bytes).  The delay-line descriptor occupies 0x00-0x10B, the live
 * (class A) parameters 0x10C-0x147, and the class B parameters overlay 0x140-0x18C - the "Exp" effect
 * is the same struct read at a +0x30 shift, which is why the two Init/Settings/Callback families write
 * overlapping offsets and why only one class is ever live in a unit.
 */
typedef struct AXFXReverbHi {
    /* +0x000 */ void* delay0[3];        /* per-channel buffers, size0 each */
    /* +0x00C */ u32 idx0;
    /* +0x010 */ u32 idx1;
    /* +0x014 */ u32 idx2;
    /* +0x018 */ u32 length0;
    /* +0x01C */ u32 size0;
    /* +0x020 */ f32 coef0;
    /* +0x024 */ f32 coef1;
    /* +0x028 */ f32 coef2;
    /* +0x02C */ void* delay1[3];        /* per-channel buffers, size1 each */
    /* +0x038 */ u32 idx3;
    /* +0x03C */ u32 length1;
    /* +0x040 */ u32 size1;
    /* +0x044 */ void* delay2[3][3];     /* 3 channels x 3 lines, size2[] */
    /* +0x068 */ u32 filter_idx[3];
    /* +0x074 */ u32 filter_len[3];
    /* +0x080 */ u32 size2[3];
    /* +0x08C */ f32 filter_gain[3];
    /* +0x098 */ void* delay3[3][2];     /* 3 channels x 2 lines, size3[] */
    /* +0x0B0 */ u32 idx7;
    /* +0x0B4 */ u32 idx8;
    /* +0x0B8 */ u32 length5;
    /* +0x0BC */ u32 length6;
    /* +0x0C0 */ u32 size3[2];
    /* +0x0C8 */ void* delay4[3];        /* per-channel buffers, size4[] */
    /* +0x0D4 */ u32 idx9[3];
    /* +0x0E0 */ u32 length7[3];
    /* +0x0EC */ u32 size4[3];
    /* +0x0F8 */ f32 coef6;
    /* +0x0FC */ f32 coef7[3];
    /* +0x108 */ f32 coef10;
    /* +0x10C */ u32 flags;
    /* +0x110 */ u32 mode;
    /* +0x114 */ f32 pre_delay;
    /* +0x118 */ f32 pre_delay2;
    /* +0x11C */ u32 filter_mode;
    /* +0x120 */ f32 time;
    /* +0x124 */ f32 damping;
    /* +0x128 */ f32 coloration;
    /* +0x12C */ f32 crosstalk;
    /* +0x130 */ f32 out_gain;
    /* +0x134 */ f32 dry_gain;
    /* +0x138 */ AXFXBuffer* early_buf;
    /* +0x13C */ AXFXBuffer* fused_buf;
    union {
        struct {
            /* +0x140 */ f32 mix;
            /* +0x144 */ f32 early_gain;
            /* +0x148 */ f32 in_damping;
            /* +0x14C */ f32 in_mix;
            /* +0x150 */ f32 in_time;
            /* +0x154 */ f32 in_coloration;
            /* +0x158 */ f32 in_pre_delay;
            /* +0x15C */ f32 in_crosstalk;
            /* +0x160 */ f32 in_160;
            /* +0x164 */ f32 in_164;
            /* +0x168 */ void* in_168;
            /* +0x16C */ void* in_16c;
            /* +0x170 */ f32 in_170;
            /* +0x174 */ f32 in_174;
        } a; /* size: 0x34 */
        struct {
            /* +0x140 */ u32 exp_mode;
            /* +0x144 */ f32 exp_pre_delay;
            /* +0x148 */ f32 exp_pre_delay2;
            /* +0x14C */ u32 exp_filter_mode;
            /* +0x150 */ f32 exp_time;
            /* +0x154 */ f32 exp_damping;
            /* +0x158 */ f32 exp_coloration;
            /* +0x15C */ f32 exp_crosstalk;
            /* +0x160 */ f32 exp_out_gain;
            /* +0x164 */ f32 exp_dry_gain;
            /* +0x168 */ void* exp_early_buf;
            /* +0x16C */ void* exp_fused_buf;
            /* +0x170 */ f32 exp_mix;
            /* +0x174 */ f32 exp_early_gain;
        } b; /* size: 0x34 */
    } params;
    /* +0x178 */ f32 exp_in_damping;
    /* +0x17C */ f32 exp_in_mix;
    /* +0x180 */ f32 exp_in_time;
    /* +0x184 */ f32 exp_in_coloration;
    /* +0x188 */ f32 exp_in_pre_delay;
    /* +0x18C */ f32 exp_in_crosstalk;
    /* +0x190 */ u8 pad_190[0x258];
} AXFXReverbHi; /* size: 0x3E8 */

#endif
