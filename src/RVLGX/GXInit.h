/*
 * RVLGX/GXInit.h - the GX library's global state record and the pointer every GX unit reads it through.
 */
#ifndef MHTRI_RVLGX_GXINIT_H
#define MHTRI_RVLGX_GXINIT_H

#include "types.h"

/* A scissor corner register: the BP word `0x20`/`0x21` carries the x and y of one corner. size: 0x4 */
typedef union GXScissorReg {
    u32 word; /* +0x0 */
    struct {
        /* +0x0, bits 31..23 */ u32 pad_0: 9;
        /* +0x0, bits 22..12 */ u32 x: 11;
        /* +0x0, bit 11 */ u32 pad_1: 1;
        /* +0x0, bits 10..0 */ u32 y: 11;
    } f; /* +0x0 */
} GXScissorReg;

/* A vertex-matrix index word: the position-matrix id sits in the low six bits. size: 0x4 */
typedef union GXMatrixIndexReg {
    u32 word; /* +0x0 */
    struct {
        /* +0x0, bits 31..6 */ u32 pad_0: 26;
        /* +0x0, bits 5..0 */ u32 posMtx: 6;
    } f; /* +0x0 */
} GXMatrixIndexReg;

/* The BP color-mode register (blend, logic, dither and write masks). size: 0x4 */
typedef union GXColorModeReg {
    u32 word; /* +0x0 */
    struct {
        /* +0x0, bits 31..24 */ u32 opcode: 8;
        /* +0x0, bits 23..16 */ u32 pad_0: 8;
        /* +0x0, bits 15..12 */ u32 logicOp: 4;
        /* +0x0, bit 11 */ u32 subtract: 1;
        /* +0x0, bits 10..8 */ u32 srcFactor: 3;
        /* +0x0, bits 7..5 */ u32 dstFactor: 3;
        /* +0x0, bit 4 */ u32 alphaUpdate: 1;
        /* +0x0, bit 3 */ u32 colorUpdate: 1;
        /* +0x0, bit 2 */ u32 dither: 1;
        /* +0x0, bit 1 */ u32 logicEnable: 1;
        /* +0x0, bit 0 */ u32 blendEnable: 1;
    } f; /* +0x0 */
} GXColorModeReg;

/* The BP destination-alpha register. size: 0x4 */
typedef union GXDstAlphaReg {
    u32 word; /* +0x0 */
    struct {
        /* +0x0, bits 31..24 */ u32 opcode: 8;
        /* +0x0, bits 23..11 */ u32 pad_0: 13;
        /* +0x0, bits 10..9 */ u32 format: 2;
        /* +0x0, bit 8 */ u32 enable: 1;
        /* +0x0, bits 7..0 */ u32 alpha: 8;
    } f; /* +0x0 */
} GXDstAlphaReg;

/* The BP depth-mode register. size: 0x4 */
typedef union GXZModeReg {
    u32 word; /* +0x0 */
    struct {
        /* +0x0, bits 31..5 */ u32 pad_0: 27;
        /* +0x0, bit 4 */ u32 update: 1;
        /* +0x0, bits 3..1 */ u32 func: 3;
        /* +0x0, bit 0 */ u32 enable: 1;
    } f; /* +0x0 */
} GXZModeReg;

/* The BP pixel-engine control register. size: 0x4 */
typedef union GXPixelCtrlReg {
    u32 word; /* +0x0 */
    struct {
        /* +0x0, bits 31..7 */ u32 pad_0: 25;
        /* +0x0, bit 6 */ u32 zCompLoc: 1;
        /* +0x0, bits 5..3 */ u32 zFormat: 3;
        /* +0x0, bits 2..0 */ u32 pixelFormat: 3;
    } f; /* +0x0 */
} GXPixelCtrlReg;

/* The BP field-mode register. size: 0x4 */
typedef union GXFieldModeReg {
    u32 word; /* +0x0 */
    struct {
        /* +0x0, bits 31..23 */ u32 pad_0: 9;
        /* +0x0, bit 22 */ u32 fieldMode: 1;
        /* +0x0, bits 21..0 */ u32 pad_1: 22;
    } f; /* +0x0 */
} GXFieldModeReg;

/* A BP fog range-adjust table word: two 12-bit entries. size: 0x4 */
typedef union GXFogAdjReg {
    u32 word; /* +0x0 */
    struct {
        /* +0x0, bits 31..24 */ u32 opcode: 8;
        /* +0x0, bits 23..12 */ u32 high: 12;
        /* +0x0, bits 11..0 */ u32 low: 12;
    } f; /* +0x0 */
} GXFogAdjReg;

/* The BP fog range-adjust center register. size: 0x4 */
typedef union GXFogAdjCenterReg {
    u32 word; /* +0x0 */
    struct {
        /* +0x0, bits 31..24 */ u32 opcode: 8;
        /* +0x0, bits 23..11 */ u32 pad_0: 13;
        /* +0x0, bit 10 */ u32 enable: 1;
        /* +0x0, bits 9..0 */ u32 center: 10;
    } f; /* +0x0 */
} GXFogAdjCenterReg;

/* The BP field-mask register. size: 0x4 */
typedef union GXFieldMaskReg {
    u32 word; /* +0x0 */
    struct {
        /* +0x0, bits 31..24 */ u32 opcode: 8;
        /* +0x0, bits 23..2 */ u32 pad_0: 22;
        /* +0x0, bit 1 */ u32 odd: 1;
        /* +0x0, bit 0 */ u32 even: 1;
    } f; /* +0x0 */
} GXFieldMaskReg;

/* A TEV order register (BP 0x28..0x2F): the texture map, texture coordinate, enable and rasterized colour channel
 * of an even and an odd TEV stage. size: 0x4 */
typedef union GXTevOrderReg {
    u32 word; /* +0x0 */
    struct {
        /* +0x0, bits 31..24 */ u32 opcode: 8;
        /* +0x0, bits 23..22 */ u32 pad_0: 2;
        /* +0x0, bits 21..19 */ u32 color1: 3;
        /* +0x0, bit 18 */ u32 enable1: 1;
        /* +0x0, bits 17..15 */ u32 coord1: 3;
        /* +0x0, bits 14..12 */ u32 map1: 3;
        /* +0x0, bits 11..10 */ u32 pad_1: 2;
        /* +0x0, bits 9..7 */ u32 color0: 3;
        /* +0x0, bit 6 */ u32 enable0: 1;
        /* +0x0, bits 5..3 */ u32 coord0: 3;
        /* +0x0, bits 2..0 */ u32 map0: 3;
    } f; /* +0x0 */
} GXTevOrderReg;

/* The indirect texture order register (BP 0x27): the texture map and coordinate of the four indirect stages.
 * size: 0x4 */
typedef union GXIndTexOrderReg {
    u32 word; /* +0x0 */
    struct {
        /* +0x0, bits 31..24 */ u32 opcode: 8;
        /* +0x0, bits 23..21 */ u32 coord3: 3;
        /* +0x0, bits 20..18 */ u32 map3: 3;
        /* +0x0, bits 17..15 */ u32 coord2: 3;
        /* +0x0, bits 14..12 */ u32 map2: 3;
        /* +0x0, bits 11..9 */ u32 coord1: 3;
        /* +0x0, bits 8..6 */ u32 map1: 3;
        /* +0x0, bits 5..3 */ u32 coord0: 3;
        /* +0x0, bits 2..0 */ u32 map0: 3;
    } f; /* +0x0 */
} GXIndTexOrderReg;

/* The indirect texture mask register (BP 0x0F): one bit per texture map an indirect stage reads. size: 0x4 */
typedef union GXIndTexMaskReg {
    u32 word; /* +0x0 */
    struct {
        /* +0x0, bits 31..24 */ u32 opcode: 8;
        /* +0x0, bits 23..8 */ u32 pad_0: 16;
        /* +0x0, bits 7..0 */ u32 mask: 8;
    } f; /* +0x0 */
} GXIndTexMaskReg;

/* An indirect texture coordinate scale register (BP 0x25/0x26): the s and t scale of two indirect stages.
 * size: 0x4 */
typedef union GXIndTexScaleReg {
    u32 word; /* +0x0 */
    struct {
        /* +0x0, bits 31..24 */ u32 opcode: 8;
        /* +0x0, bits 23..16 */ u32 pad_0: 8;
        /* +0x0, bits 15..12 */ u32 t1: 4;
        /* +0x0, bits 11..8 */ u32 s1: 4;
        /* +0x0, bits 7..4 */ u32 t0: 4;
        /* +0x0, bits 3..0 */ u32 s0: 4;
    } f; /* +0x0 */
} GXIndTexScaleReg;

/* A TEV colour combiner register (BP 0xC0 + 2 * stage): inputs a..d, bias, subtract, clamp, scale and destination.
 * size: 0x4 */
typedef union GXTevColorEnvReg {
    u32 word; /* +0x0 */
    struct {
        /* +0x0, bits 31..24 */ u32 opcode: 8;
        /* +0x0, bits 23..22 */ u32 dest: 2;
        /* +0x0, bits 21..20 */ u32 scale: 2;
        /* +0x0, bit 19 */ u32 clamp: 1;
        /* +0x0, bit 18 */ u32 sub: 1;
        /* +0x0, bits 17..16 */ u32 bias: 2;
        /* +0x0, bits 15..12 */ u32 a: 4;
        /* +0x0, bits 11..8 */ u32 b: 4;
        /* +0x0, bits 7..4 */ u32 c: 4;
        /* +0x0, bits 3..0 */ u32 d: 4;
    } f; /* +0x0 */
} GXTevColorEnvReg;

/* A TEV alpha combiner register (BP 0xC1 + 2 * stage): the colour combiner's top bits, three-bit inputs a..d and the
 * texture and rasterized swap-table selectors. size: 0x4 */
typedef union GXTevAlphaEnvReg {
    u32 word; /* +0x0 */
    struct {
        /* +0x0, bits 31..24 */ u32 opcode: 8;
        /* +0x0, bits 23..22 */ u32 dest: 2;
        /* +0x0, bits 21..20 */ u32 scale: 2;
        /* +0x0, bit 19 */ u32 clamp: 1;
        /* +0x0, bit 18 */ u32 sub: 1;
        /* +0x0, bits 17..16 */ u32 bias: 2;
        /* +0x0, bits 15..13 */ u32 a: 3;
        /* +0x0, bits 12..10 */ u32 b: 3;
        /* +0x0, bits 9..7 */ u32 c: 3;
        /* +0x0, bits 6..4 */ u32 d: 3;
        /* +0x0, bits 3..2 */ u32 tswap: 2;
        /* +0x0, bits 1..0 */ u32 rswap: 2;
    } f; /* +0x0 */
} GXTevAlphaEnvReg;

/* A TEV constant-selection register (BP 0xF6..0xFD): the konst colour and alpha selectors of an even and an odd stage
 * and two channels of one swap-mode table entry. size: 0x4 */
typedef union GXTevKSelReg {
    u32 word; /* +0x0 */
    struct {
        /* +0x0, bits 31..24 */ u32 opcode: 8;
        /* +0x0, bits 23..19 */ u32 kasel1: 5;
        /* +0x0, bits 18..14 */ u32 kcsel1: 5;
        /* +0x0, bits 13..9 */ u32 kasel0: 5;
        /* +0x0, bits 8..4 */ u32 kcsel0: 5;
        /* +0x0, bits 3..2 */ u32 swapGA: 2;
        /* +0x0, bits 1..0 */ u32 swapRB: 2;
    } f; /* +0x0 */
} GXTevKSelReg;

/* The BP generation-mode register (BP 0x00): the indirect stage count and the TEV stage count less one. size: 0x4 */
typedef union GXGenModeReg {
    u32 word; /* +0x0 */
    struct {
        /* +0x0, bits 31..24 */ u32 opcode: 8;
        /* +0x0, bits 23..19 */ u32 pad_0: 5;
        /* +0x0, bits 18..16 */ u32 numIndStages: 3;
        /* +0x0, bits 15..14 */ u32 pad_1: 2;
        /* +0x0, bits 13..10 */ u32 numTevStagesMinus1: 4;
        /* +0x0, bits 9..0 */ u32 pad_2: 10;
    } f; /* +0x0 */
} GXGenModeReg;

/* The GX state record the library keeps in `.bss` (the 0x600-byte object at 0x80746CA0). Only the words the written GX
 * units use are named; the rest is `pad_0x..` and keeps its offset. size: 0x600 */
typedef struct GXData {
    /* +0x000 */ u8 pad_0x000[0x2];
    /* +0x002 */ u16 xfWritePending; /* 0 after a BP write, 1 after a CP/XF write */
    /* +0x004 */ u8 pad_0x004[0x78];
    /* +0x07C */ GXFieldModeReg fieldMode;
    /* +0x080 */ GXMatrixIndexReg matrixIndexLo; /* the first four vertex-matrix index words (current position matrix id in bits 0..5) */
    /* +0x084 */ u32 matrixIndexHi;
    /* +0x088 */ u8 pad_0x088[0xC0];
    /* +0x148 */ GXScissorReg scissorTopLeft;
    /* +0x14C */ GXScissorReg scissorBottomRight;
    /* +0x150 */ GXTevOrderReg tref[8];
    /* +0x170 */ GXIndTexOrderReg iref;
    /* +0x174 */ GXIndTexMaskReg bpMask;
    /* +0x178 */ GXIndTexScaleReg indTexScale0;
    /* +0x17C */ GXIndTexScaleReg indTexScale1;
    /* +0x180 */ GXTevColorEnvReg tevc[16];
    /* +0x1C0 */ GXTevAlphaEnvReg teva[16];
    /* +0x200 */ GXTevKSelReg tevKsel[8];
    /* +0x220 */ GXColorModeReg colorMode;
    /* +0x224 */ GXDstAlphaReg dstAlpha;
    /* +0x228 */ GXZModeReg zMode;
    /* +0x22C */ GXPixelCtrlReg pixelCtrl;
    /* +0x230 */ u8 pad_0x230[0x24];
    /* +0x254 */ GXGenModeReg genMode;
    /* +0x258 */ u8 pad_0x258[0x2D0];
    /* +0x528 */ u32 projectionType; /* 0 perspective, 1 orthographic */
    /* +0x52C */ f32 projection[6];
    /* +0x544 */ f32 viewportLeft;
    /* +0x548 */ f32 viewportTop;
    /* +0x54C */ f32 viewportWidth;
    /* +0x550 */ f32 viewportHeight;
    /* +0x554 */ f32 viewportNearZ;
    /* +0x558 */ f32 viewportFarZ;
    /* +0x55C */ f32 viewportZOffset;
    /* +0x560 */ f32 viewportZScale;
    /* +0x564 */ u8 pad_0x564[0x40];
    /* +0x5A4 */ u32 texmapId[16]; /* the texture map each TEV stage reads */
    /* +0x5E4 */ u8 pad_0x5E4[0x4];
    /* +0x5E8 */ u32 tevTcEnab; /* one bit per TEV stage with a texture coordinate enabled */
    /* +0x5EC */ u8 pad_0x5EC[0x10];
    /* +0x5FC */ u32 dirtyState;
} GXData;

#ifdef __cplusplus
extern "C" {
#endif

/* The pointer to the state record (`.sdata2` 0x8079D0F0, defined by `GXInit.c`). */
extern GXData* const __GXData;

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_RVLGX_GXINIT_H */
