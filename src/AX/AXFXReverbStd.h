/*
 * AX/AXFXReverbStd.h - the four-channel AXFX reverb body and the entry points of `AX/AXFXReverbStd.cpp`
 * (0x80475E30..0x804770E0) that the AXFXReverbHi wrappers and other units call.
 */
#ifndef AX_AXFXREVERBSTD_H
#define AX_AXFXREVERBSTD_H

#include "AX/AXFXReverbHi.h"

/* The four channel input/output sample pointers the Std effect callbacks advance. */
typedef struct AXFXBufferStd {
    /* +0x00 */ s32* out[4];
} AXFXBufferStd; /* size: 0x10 */

/*
 * One four-channel reverb body (0x3E8 bytes): the delay-line descriptor 0x000-0x13B, the live parameters
 * 0x13C-0x177 and the caller-written parameters 0x178-0x18F.
 */
typedef struct AXFXReverbStd {
    /* +0x000 */ void* delay0[4];        /* per-channel buffers, size0 words each */
    /* +0x010 */ u32 idx0;
    /* +0x014 */ u32 idx1;
    /* +0x018 */ u32 idx2;
    /* +0x01C */ u32 length0;
    /* +0x020 */ u32 size0;
    /* +0x024 */ f32 coef0;
    /* +0x028 */ f32 coef1;
    /* +0x02C */ f32 coef2;
    /* +0x030 */ void* delay1[4];        /* per-channel pre-delay buffers, size1 words each */
    /* +0x040 */ u32 idx3;
    /* +0x044 */ u32 length1;
    /* +0x048 */ u32 size1;
    /* +0x04C */ void* delay2[4][3];     /* 4 channels x 3 lines, size2[] words */
    /* +0x07C */ u32 filter_idx[3];
    /* +0x088 */ u32 filter_len[3];
    /* +0x094 */ u32 size2[3];
    /* +0x0A0 */ f32 filter_gain[3];
    /* +0x0AC */ void* delay3[4][2];     /* 4 channels x 2 lines, size3[] words */
    /* +0x0CC */ u32 idx7;
    /* +0x0D0 */ u32 idx8;
    /* +0x0D4 */ u32 length5;
    /* +0x0D8 */ u32 length6;
    /* +0x0DC */ u32 size3[2];
    /* +0x0E4 */ void* delay4[4];        /* per-channel buffers, size4[] words each */
    /* +0x0F4 */ u32 idx9[4];
    /* +0x104 */ u32 length7[4];
    /* +0x114 */ u32 size4[4];
    /* +0x124 */ f32 coef6;
    /* +0x128 */ f32 coef7[4];
    /* +0x138 */ f32 coef10;
    /* +0x13C */ u32 flags;              /* bit 0: disabled, bit 1: settings applied */
    /* +0x140 */ u32 mode;
    /* +0x144 */ f32 pre_delay;
    /* +0x148 */ f32 pre_delay2;
    /* +0x14C */ u32 filter_mode;
    /* +0x150 */ f32 time;
    /* +0x154 */ f32 damping;
    /* +0x158 */ f32 coloration;
    /* +0x15C */ f32 crosstalk;
    /* +0x160 */ f32 out_gain;
    /* +0x164 */ f32 dry_gain;
    /* +0x168 */ AXFXBufferStd* early_buf;
    /* +0x16C */ AXFXBufferStd* fused_buf;
    /* +0x170 */ f32 mix;
    /* +0x174 */ f32 early_gain;
    /* +0x178 */ f32 in_damping;
    /* +0x17C */ f32 in_mix;
    /* +0x180 */ f32 in_time;
    /* +0x184 */ f32 in_coloration;
    /* +0x188 */ f32 in_pre_delay;
    /* +0x18C */ f32 in_crosstalk;
    /* +0x190 */ u8 pad_0x190[0x258];
} AXFXReverbStd; /* size: 0x3E8 */

/* The leading fields of the four small effects whose line-free helpers this unit holds: the delay-line pointers
 * and the flag word; the full bodies are larger (sizes are approximations of the part read here). */
typedef struct AXFXChorus3 {
    /* +0x00 */ void* line[3];
    /* +0x0C */ u8 pad_0x0C[0x30];
    /* +0x3C */ u32 flags;
} AXFXChorus3; /* size: 0x40 (approximation) */

typedef struct AXFXChorus4 {
    /* +0x00 */ void* line[4];
    /* +0x10 */ u8 pad_0x10[0x40];
    /* +0x50 */ u32 flags;
} AXFXChorus4; /* size: 0x54 (approximation) */

typedef struct AXFXDelay3 {
    /* +0x00 */ void* line[3];
    /* +0x0C */ u8 pad_0x0C[0x70];
    /* +0x7C */ u32 flags;
} AXFXDelay3; /* size: 0x80 (approximation) */

typedef struct AXFXDelay4 {
    /* +0x00 */ void* line[4];
    /* +0x10 */ u8 pad_0x10[0x80];
    /* +0x90 */ u32 flags;
} AXFXDelay4; /* size: 0x94 (approximation) */

/* The allocator and free hooks every AXFX effect takes its delay lines from. */
/* untyped: raw heap memory */
typedef void* (*AXFXAllocFunc)(u32 size);
/* untyped: raw heap memory */
typedef void (*AXFXFreeFunc)(void* block);

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80793D20 / 0x80793D24 - the hooks installed by AXFXSetHooks. */
extern AXFXAllocFunc AXFXAlloc;
extern AXFXFreeFunc AXFXFree;

BOOL AXFXReverbStdExpInit(AXFXReverbStd* reverb);
BOOL AXFXReverbStdExpSettings(AXFXReverbStd* reverb);
void AXFXReverbStdExpShutdown(AXFXReverbStd* reverb);
void AXFXReverbStdExpCallback(AXFXBufferStd* buffer, AXFXReverbStd* reverb);
void AXFXChorus3Shutdown(AXFXChorus3* effect);
void AXFXChorus4Shutdown(AXFXChorus4* effect);
BOOL AXFXDelay3Shutdown(AXFXDelay3* effect);
BOOL AXFXDelay4Shutdown(AXFXDelay4* effect);
/* untyped: raw heap memory */
void* AXFXDefaultAlloc(u32 size);
/* untyped: raw heap memory */
void AXFXDefaultFree(void* block);
void AXFXSetHooks(AXFXAllocFunc alloc, AXFXFreeFunc free);
void AXFXGetHooks(AXFXAllocFunc* alloc, AXFXFreeFunc* free);

#ifdef __cplusplus
}
#endif

#endif
