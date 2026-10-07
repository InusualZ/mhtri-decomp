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
    /* +0x150 */ u8 pad_0x150[0xD0];
    /* +0x220 */ GXColorModeReg colorMode;
    /* +0x224 */ GXDstAlphaReg dstAlpha;
    /* +0x228 */ GXZModeReg zMode;
    /* +0x22C */ GXPixelCtrlReg pixelCtrl;
    /* +0x230 */ u8 pad_0x230[0x2F8];
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
    /* +0x564 */ u8 pad_0x564[0x98];
    /* +0x5FC */ u32 dirtyState;
} GXData;

#ifdef __cplusplus
extern "C" {
#endif

/* The pointer to the state record (`.sdata2` 0x8079D0F0, defined by `GXInit.c`). */
extern GXData* __GXData;

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_RVLGX_GXINIT_H */
