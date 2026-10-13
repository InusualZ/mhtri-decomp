/*
 * RVLGX/GXInit.h - the GX library's global state record and the pointer every GX unit reads it through.
 */
#ifndef MHTRI_RVLGX_GXINIT_H
#define MHTRI_RVLGX_GXINIT_H

#include "types.h"
#include "gx.h"

/* Inserts `value` into bits `first`..`last` of `word` (bit 0 is the most significant), as one `rlwimi`. */
#define GX_SET_REG(word, value, first, last) ((word) = __rlwimi((word), (value), 31 - (last), (first), (last)))

/* A `GXColor` read as one word: red in bits 31..24, green 23..16, blue 15..8, alpha 7..0. size: 0x4 */
typedef union GXColorWord {
    GXColor color; /* +0x0 */
    u32 word;      /* +0x0 */
} GXColorWord;

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

/* The first vertex-matrix index word: the position matrix id plus the texture matrix ids of coordinates 0..3. size: 0x4 */
typedef union GXMatrixIndexReg {
    u32 word; /* +0x0 */
    struct {
        /* +0x0, bits 31..30 */ u32 pad_0: 2;
        /* +0x0, bits 29..24 */ u32 tex3Mtx: 6;
        /* +0x0, bits 23..18 */ u32 tex2Mtx: 6;
        /* +0x0, bits 17..12 */ u32 tex1Mtx: 6;
        /* +0x0, bits 11..6 */ u32 tex0Mtx: 6;
        /* +0x0, bits 5..0 */ u32 posMtx: 6;
    } f; /* +0x0 */
} GXMatrixIndexReg;

/* The second vertex-matrix index word: the texture matrix ids of coordinates 4..7. size: 0x4 */
typedef union GXMatrixIndexHiReg {
    u32 word; /* +0x0 */
    struct {
        /* +0x0, bits 31..24 */ u32 pad_0: 8;
        /* +0x0, bits 23..18 */ u32 tex7Mtx: 6;
        /* +0x0, bits 17..12 */ u32 tex6Mtx: 6;
        /* +0x0, bits 11..6 */ u32 tex5Mtx: 6;
        /* +0x0, bits 5..0 */ u32 tex4Mtx: 6;
    } f; /* +0x0 */
} GXMatrixIndexHiReg;

/* The CP vertex descriptor, low word (CP 0x50). size: 0x4 */
typedef union GXVcdLoReg {
    u32 word; /* +0x0 */
    struct {
        /* +0x0, bits 31..17 */ u32 pad_0: 15;
        /* +0x0, bits 16..15 */ u32 color1: 2;
        /* +0x0, bits 14..13 */ u32 color0: 2;
        /* +0x0, bits 12..11 */ u32 normal: 2;
        /* +0x0, bits 10..9 */ u32 position: 2;
        /* +0x0, bit 8 */ u32 tex7MtxIdx: 1;
        /* +0x0, bit 7 */ u32 tex6MtxIdx: 1;
        /* +0x0, bit 6 */ u32 tex5MtxIdx: 1;
        /* +0x0, bit 5 */ u32 tex4MtxIdx: 1;
        /* +0x0, bit 4 */ u32 tex3MtxIdx: 1;
        /* +0x0, bit 3 */ u32 tex2MtxIdx: 1;
        /* +0x0, bit 2 */ u32 tex1MtxIdx: 1;
        /* +0x0, bit 1 */ u32 tex0MtxIdx: 1;
        /* +0x0, bit 0 */ u32 posMtxIdx: 1;
    } f; /* +0x0 */
} GXVcdLoReg;

/* The CP vertex descriptor, high word (CP 0x60): texture coordinates 0..7. size: 0x4 */
typedef union GXVcdHiReg {
    u32 word; /* +0x0 */
    struct {
        /* +0x0, bits 31..16 */ u32 pad_0: 16;
        /* +0x0, bits 15..14 */ u32 tex7: 2;
        /* +0x0, bits 13..12 */ u32 tex6: 2;
        /* +0x0, bits 11..10 */ u32 tex5: 2;
        /* +0x0, bits 9..8 */ u32 tex4: 2;
        /* +0x0, bits 7..6 */ u32 tex3: 2;
        /* +0x0, bits 5..4 */ u32 tex2: 2;
        /* +0x0, bits 3..2 */ u32 tex1: 2;
        /* +0x0, bits 1..0 */ u32 tex0: 2;
    } f; /* +0x0 */
} GXVcdHiReg;

/* A CP vertex attribute table word A (CP 0x70 + format). size: 0x4 */
typedef union GXVatAReg {
    u32 word; /* +0x0 */
    struct {
        /* +0x0, bit 31 */ u32 normalIndex3: 1;
        /* +0x0, bit 30 */ u32 byteDequant: 1;
        /* +0x0, bits 29..25 */ u32 tex0Frac: 5;
        /* +0x0, bits 24..22 */ u32 tex0Type: 3;
        /* +0x0, bit 21 */ u32 tex0Cnt: 1;
        /* +0x0, bits 20..18 */ u32 color1Type: 3;
        /* +0x0, bit 17 */ u32 color1Cnt: 1;
        /* +0x0, bits 16..14 */ u32 color0Type: 3;
        /* +0x0, bit 13 */ u32 color0Cnt: 1;
        /* +0x0, bits 12..10 */ u32 normalType: 3;
        /* +0x0, bit 9 */ u32 normalCnt: 1;
        /* +0x0, bits 8..4 */ u32 posFrac: 5;
        /* +0x0, bits 3..1 */ u32 posType: 3;
        /* +0x0, bit 0 */ u32 posCnt: 1;
    } f; /* +0x0 */
} GXVatAReg;

/* A CP vertex attribute table word B (CP 0x80 + format). size: 0x4 */
typedef union GXVatBReg {
    u32 word; /* +0x0 */
    struct {
        /* +0x0, bit 31 */ u32 vcacheEnhance: 1;
        /* +0x0, bits 30..28 */ u32 tex4Type: 3;
        /* +0x0, bit 27 */ u32 tex4Cnt: 1;
        /* +0x0, bits 26..22 */ u32 tex3Frac: 5;
        /* +0x0, bits 21..19 */ u32 tex3Type: 3;
        /* +0x0, bit 18 */ u32 tex3Cnt: 1;
        /* +0x0, bits 17..13 */ u32 tex2Frac: 5;
        /* +0x0, bits 12..10 */ u32 tex2Type: 3;
        /* +0x0, bit 9 */ u32 tex2Cnt: 1;
        /* +0x0, bits 8..4 */ u32 tex1Frac: 5;
        /* +0x0, bits 3..1 */ u32 tex1Type: 3;
        /* +0x0, bit 0 */ u32 tex1Cnt: 1;
    } f; /* +0x0 */
} GXVatBReg;

/* A CP vertex attribute table word C (CP 0x90 + format). size: 0x4 */
typedef union GXVatCReg {
    u32 word; /* +0x0 */
    struct {
        /* +0x0, bits 31..27 */ u32 tex7Frac: 5;
        /* +0x0, bits 26..24 */ u32 tex7Type: 3;
        /* +0x0, bit 23 */ u32 tex7Cnt: 1;
        /* +0x0, bits 22..18 */ u32 tex6Frac: 5;
        /* +0x0, bits 17..15 */ u32 tex6Type: 3;
        /* +0x0, bit 14 */ u32 tex6Cnt: 1;
        /* +0x0, bits 13..9 */ u32 tex5Frac: 5;
        /* +0x0, bits 8..6 */ u32 tex5Type: 3;
        /* +0x0, bit 5 */ u32 tex5Cnt: 1;
        /* +0x0, bits 4..0 */ u32 tex4Frac: 5;
    } f; /* +0x0 */
} GXVatCReg;

/* An XF texture-coordinate generation register (XF 0x1040 + coordinate). size: 0x4 */
typedef union GXTexGenCtrlReg {
    u32 word; /* +0x0 */
    struct {
        /* +0x0, bits 31..18 */ u32 pad_0: 14;
        /* +0x0, bits 17..15 */ u32 embossLight: 3;
        /* +0x0, bits 14..12 */ u32 embossSource: 3;
        /* +0x0, bits 11..7 */ u32 sourceRow: 5;
        /* +0x0, bits 6..4 */ u32 texGenType: 3;
        /* +0x0, bit 3 */ u32 pad_1: 1;
        /* +0x0, bit 2 */ u32 inputForm: 1;
        /* +0x0, bit 1 */ u32 projection: 1;
        /* +0x0, bit 0 */ u32 pad_2: 1;
    } f; /* +0x0 */
} GXTexGenCtrlReg;

/* An XF post-transform texture register (XF 0x1050 + coordinate). size: 0x4 */
typedef union GXPostTexGenReg {
    u32 word; /* +0x0 */
    struct {
        /* +0x0, bits 31..9 */ u32 pad_0: 23;
        /* +0x0, bit 8 */ u32 normalize: 1;
        /* +0x0, bits 7..6 */ u32 pad_1: 2;
        /* +0x0, bits 5..0 */ u32 postMtx: 6;
    } f; /* +0x0 */
} GXPostTexGenReg;

/* A BP setup-unit texture-coordinate scale register (BP 0x30 + 2 * coordinate). size: 0x4 */
typedef union GXSuTsReg {
    u32 word; /* +0x0 */
    struct {
        /* +0x0, bits 31..24 */ u32 opcode: 8;
        /* +0x0, bits 23..20 */ u32 pad_0: 4;
        /* +0x0, bit 19 */ u32 pointOffset: 1;
        /* +0x0, bit 18 */ u32 lineOffset: 1;
        /* +0x0, bit 17 */ u32 cylWrap: 1;
        /* +0x0, bit 16 */ u32 rangeBias: 1;
        /* +0x0, bits 15..0 */ u32 scaleMinus1: 16;
    } f; /* +0x0 */
} GXSuTsReg;

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

/* The BP line/point size register (BP 0x22). size: 0x4 */
typedef union GXLinePtSizeReg {
    u32 word; /* +0x0 */
    struct {
        /* +0x0, bits 31..24 */ u32 opcode: 8;
        /* +0x0, bit 23 */ u32 pad_0: 1;
        /* +0x0, bit 22 */ u32 halfAspect: 1;
        /* +0x0, bits 21..19 */ u32 pointTexOffset: 3;
        /* +0x0, bits 18..16 */ u32 lineTexOffset: 3;
        /* +0x0, bits 15..8 */ u32 pointSize: 8;
        /* +0x0, bits 7..0 */ u32 lineWidth: 8;
    } f; /* +0x0 */
} GXLinePtSizeReg;

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
        /* +0x0, bits 23..20 */ u32 pad_0: 4;
        /* +0x0, bit 19 */ u32 coPlanar: 1;
        /* +0x0, bits 18..16 */ u32 numIndStages: 3;
        /* +0x0, bits 15..14 */ u32 cullMode: 2;
        /* +0x0, bits 13..10 */ u32 numTevStagesMinus1: 4;
        /* +0x0, bit 9 */ u32 multisample: 1;
        /* +0x0, bits 8..7 */ u32 pad_2: 2;
        /* +0x0, bits 6..4 */ u32 numColorChans: 3;
        /* +0x0, bits 3..0 */ u32 numTexGens: 4;
    } f; /* +0x0 */
} GXGenModeReg;


/* XF channel control (0x100E..0x1011). size: 0x4 */
typedef union GXChanCtrlReg {
    u32 word; /* +0x0 */
    struct {
        /* +0x0, bits 31..15 */ u32 pad_0: 17;
        /* +0x0, bits 14..11 */ u32 lightMaskHi: 4;
        /* +0x0, bit 10 */ u32 attnSelect: 1;
        /* +0x0, bit 9 */ u32 attnEnable: 1;
        /* +0x0, bits 8..7 */ u32 diffuseFn: 2;
        /* +0x0, bit 6 */ u32 ambSrc: 1;
        /* +0x0, bits 5..2 */ u32 lightMaskLo: 4;
        /* +0x0, bit 1 */ u32 enable: 1;
        /* +0x0, bit 0 */ u32 matSrc: 1;
    } f; /* +0x0 */
} GXChanCtrlReg;

/* BP copy source corner / size. size: 0x4 */
typedef union GXCopyRectReg {
    u32 word; /* +0x0 */
    struct {
        /* +0x0, bits 31..24 */ u32 opcode: 8;
        /* +0x0, bits 23..20 */ u32 pad_0: 4;
        /* +0x0, bits 19..10 */ u32 y: 10;
        /* +0x0, bits 9..0 */ u32 x: 10;
    } f; /* +0x0 */
} GXCopyRectReg;

/* BP copy stride. size: 0x4 */
typedef union GXCopyStrideReg {
    u32 word; /* +0x0 */
    struct {
        /* +0x0, bits 31..24 */ u32 opcode: 8;
        /* +0x0, bits 23..10 */ u32 pad_0: 14;
        /* +0x0, bits 9..0 */ u32 stride: 10;
    } f; /* +0x0 */
} GXCopyStrideReg;

/* BP copy control (0x52). size: 0x4 */
typedef union GXCopyCtrlReg {
    u32 word; /* +0x0 */
    struct {
        /* +0x0, bits 31..24 */ u32 opcode: 8;
        /* +0x0, bits 23..17 */ u32 pad_0: 7;
        /* +0x0, bits 16..15 */ u32 intensityFmt: 2;
        /* +0x0, bit 14 */ u32 toXfb: 1;
        /* +0x0, bits 13..12 */ u32 frame2Field: 2;
        /* +0x0, bit 11 */ u32 clear: 1;
        /* +0x0, bit 10 */ u32 scaleY: 1;
        /* +0x0, bit 9 */ u32 mipmap: 1;
        /* +0x0, bits 8..7 */ u32 gamma: 2;
        /* +0x0, bits 6..4 */ u32 texFmt: 3;
        /* +0x0, bit 3 */ u32 texFmtH: 1;
        /* +0x0, bit 2 */ u32 pad_1: 1;
        /* +0x0, bit 1 */ u32 clampBottom: 1;
        /* +0x0, bit 0 */ u32 clampTop: 1;
    } f; /* +0x0 */
} GXCopyCtrlReg;

/* The GX state record the library keeps in `.bss` (the 0x600-byte object at 0x80746CA0). Only the words the written GX
 * units use are named; the rest is `pad_0x..` and keeps its offset. size: 0x600 */
typedef struct GXData {
    /* +0x000 */ union { /* size: 0x4 */
        /* +0x000 */ u32 vtxSentWord; /* both halves read as one word */
        /* +0x000 */ struct { /* size: 0x4 */
            /* +0x000 */ u16 vNumNot;
            /* +0x002 */ u16 xfWritePending; /* 0 after a BP write, 1 after a CP/XF write */
        };
    };
    /* +0x004 */ u16 vNum;
    /* +0x006 */ u16 vLim;
    /* +0x008 */ u8 pad_0x008[0xC];
    /* +0x014 */ GXVcdLoReg vcdLo;
    /* +0x018 */ GXVcdHiReg vcdHi;
    /* +0x01C */ GXVatAReg vatA[8];
    /* +0x03C */ GXVatBReg vatB[8];
    /* +0x05C */ GXVatCReg vatC[8];
    /* +0x07C */ GXLinePtSizeReg lpSize;
    /* +0x080 */ GXMatrixIndexReg matrixIndexLo; /* position matrix id plus the texture matrix ids of coordinates 0..3 */
    /* +0x084 */ GXMatrixIndexHiReg matrixIndexHi;
    /* +0x088 */ u8 pad_0x088[0x20];
    /* +0x0A8 */ GXColorWord ambColor[2];
    /* +0x0B0 */ GXColorWord matColor[2];
    /* +0x0B8 */ GXChanCtrlReg chanCtrl[4];
    /* +0x0C8 */ GXTexGenCtrlReg texGenCtrl[8];
    /* +0x0E8 */ GXPostTexGenReg postTexGen[8];
    /* +0x108 */ GXSuTsReg suTs0[8];
    /* +0x128 */ u8 pad_0x128[0x20];
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
    /* +0x230 */ GXCopyRectReg cpDispSrc;
    /* +0x234 */ GXCopyRectReg cpDispSize;
    /* +0x238 */ GXCopyStrideReg cpDispStride;
    /* +0x23C */ GXCopyCtrlReg cpDisp;
    /* +0x240 */ GXCopyRectReg cpTexSrc;
    /* +0x244 */ GXCopyRectReg cpTexSize;
    /* +0x248 */ GXCopyStrideReg cpTexStride;
    /* +0x24C */ GXCopyCtrlReg cpTex;
    /* +0x250 */ u8 cpTexZ;
    /* +0x251 */ u8 pad_0x251[3];
    /* +0x254 */ GXGenModeReg genMode;
    /* +0x258 */ u8 pad_0x258[0x2C8];
    /* +0x520 */ u32 nrmType;
    /* +0x524 */ u8 hasNrms;
    /* +0x525 */ u8 hasBiNrms;
    /* +0x526 */ u8 pad_0x526[2];
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
    /* +0x5EC */ u8 pad_0x5EC[0xF];
    /* +0x5FB */ u8 dirtyVAT;
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
