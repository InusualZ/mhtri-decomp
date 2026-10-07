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

/* The GX state record the library keeps in `.bss` (the 0x600-byte object at 0x80746CA0). Only the words the written GX
 * units use are named; the rest is `pad_0x..` and keeps its offset. size: 0x600 */
typedef struct GXData {
    /* +0x000 */ u8 pad_0x000[0x2];
    /* +0x002 */ u16 xfWritePending; /* 0 after a BP write, 1 after a CP/XF write */
    /* +0x004 */ u8 pad_0x004[0x7C];
    /* +0x080 */ GXMatrixIndexReg matrixIndexLo; /* the first four vertex-matrix index words (current position matrix id in bits 0..5) */
    /* +0x084 */ u32 matrixIndexHi;
    /* +0x088 */ u8 pad_0x088[0xC0];
    /* +0x148 */ GXScissorReg scissorTopLeft;
    /* +0x14C */ GXScissorReg scissorBottomRight;
    /* +0x150 */ u8 pad_0x150[0x3D8];
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
