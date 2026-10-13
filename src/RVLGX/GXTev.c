/*
 * RVLGX/GXTev.c - the SDK GX TEV stage setup.
 *
 * RANGE. `.text` 0x804B9360-0x804B9A30; `.data` 0x8061AC20-0x8061AC98 (16 functions / 0x678 B).
 *   - `.data` 0x8061AC20 (the `GXSetTevOp` mode tables) and 0x8061AC70 (the `GXSetTevOrder` channel table) are read
 *     only here
 * FLAGS. the `OS` lib block of `configure.py` (`Wii/1.3`) with `cflags_base` (-O4,p, default alignment 16).
 * NAMES. `TEVCOpTableST0`/`TEVCOpTableST1`/`TEVAOpTableST0`/`TEVAOpTableST1` (the four 0x14-byte `GXSetTevOp` mode
 *   tables at 0x8061AC20) and `c2r` (the 0x28-byte `GXSetTevOrder` channel-to-rasterizer table at 0x8061AC70) are GUESS
 *   names after the SDK's own spellings; the split follows the four 5-word runs. The local register unions
 *   (`GXTevColorRegA`/`B`, `GXTevColorS10RegA`/`B`, `GXAlphaCompareReg`, `GXZTexBiasReg`, `GXZTexModeReg`) and
 *   `GXColorWord`/`GXColorS10Words` are named from the BP register they build and the word they read.
 * RESIDUALS. 6 of 16 functions match (496 of 1656 B); `.data` 120 of 120 B. The other ten wait on one declaration
 *   outside this unit: `RVLGX/GXInit.h` declares `GXData* __GXData`, and the SDK's pointer is `GXData* const` (it lives
 *   in `.sdata2`). With the const declaration all 16 functions match and the flip links a byte-identical DOL; without
 *   it MWCC cannot move or repeat the pointer load across the FIFO's volatile stores. flipcheck: `.text` bytes (that
 *   declaration) and row 36 for `TEVCOpTableST1`/`TEVAOpTableST0`/`TEVAOpTableST1`, a false positive - the flipped
 *   link keeps them (they are reached through the `.data` section symbol). relocdiff: `GXSetTevOp`'s table base is
 *   `.data`+0 where the target names `TEVCOpTableST0`, the same address.
 * SHAPES. `GXSetTevColorOp`/`GXSetTevAlphaOp`/`GXSetNumTevStages` insert with `GX_SET_REG` (`__rlwimi`) into a scalar
 *   local: a union local is promoted late and colours after the address temporaries. `GXSetTevOrder` stores its
 *   copy back after each field and sends the word from memory. The colour setters read the colour as words.
 */

#include "RVLGX/GXTev.h"
#include "RVLGX/GXInit.h"

/* Inserts `value` into bits `first`..`last` of `word` (bit 0 is the most significant), as one `rlwimi`. */
#define GX_SET_REG(word, value, first, last) ((word) = __rlwimi((word), (value), 31 - (last), (first), (last)))

/* The TEV colour register's red/alpha word (BP 0xE0 + 2 * id). size: 0x4 */
typedef union GXTevColorRegA {
    u32 word; /* +0x0 */
    struct GXTevColorRegAFields { /* size: 0x4 */
        /* +0x0, bits 31..24 */ u32 opcode: 8;
        /* +0x0, bits 23..20 */ u32 konst: 4;
        /* +0x0, bits 19..12 */ u32 a: 8;
        /* +0x0, bits 11..8 */ u32 pad_0: 4;
        /* +0x0, bits 7..0 */ u32 r: 8;
    } f; /* +0x0 */
} GXTevColorRegA;

/* The TEV colour register's blue/green word (BP 0xE1 + 2 * id). size: 0x4 */
typedef union GXTevColorRegB {
    u32 word; /* +0x0 */
    struct GXTevColorRegBFields { /* size: 0x4 */
        /* +0x0, bits 31..24 */ u32 opcode: 8;
        /* +0x0, bits 23..20 */ u32 konst: 4;
        /* +0x0, bits 19..12 */ u32 g: 8;
        /* +0x0, bits 11..8 */ u32 pad_0: 4;
        /* +0x0, bits 7..0 */ u32 b: 8;
    } f; /* +0x0 */
} GXTevColorRegB;

/* The signed 11-bit form of the TEV colour register's red/alpha word. size: 0x4 */
typedef union GXTevColorS10RegA {
    u32 word; /* +0x0 */
    struct GXTevColorS10RegAFields { /* size: 0x4 */
        /* +0x0, bits 31..24 */ u32 opcode: 8;
        /* +0x0, bit 23 */ u32 pad_0: 1;
        /* +0x0, bits 22..12 */ u32 a: 11;
        /* +0x0, bit 11 */ u32 pad_1: 1;
        /* +0x0, bits 10..0 */ u32 r: 11;
    } f; /* +0x0 */
} GXTevColorS10RegA;

/* The signed 11-bit form of the TEV colour register's blue/green word. size: 0x4 */
typedef union GXTevColorS10RegB {
    u32 word; /* +0x0 */
    struct GXTevColorS10RegBFields { /* size: 0x4 */
        /* +0x0, bits 31..24 */ u32 opcode: 8;
        /* +0x0, bit 23 */ u32 pad_0: 1;
        /* +0x0, bits 22..12 */ u32 g: 11;
        /* +0x0, bit 11 */ u32 pad_1: 1;
        /* +0x0, bits 10..0 */ u32 b: 11;
    } f; /* +0x0 */
} GXTevColorS10RegB;

/* The BP alpha-compare register (BP 0xF3). size: 0x4 */
typedef union GXAlphaCompareReg {
    u32 word; /* +0x0 */
    struct GXAlphaCompareRegFields { /* size: 0x4 */
        /* +0x0, bits 31..24 */ u32 opcode: 8;
        /* +0x0, bits 23..22 */ u32 op: 2;
        /* +0x0, bits 21..19 */ u32 comp1: 3;
        /* +0x0, bits 18..16 */ u32 comp0: 3;
        /* +0x0, bits 15..8 */ u32 ref1: 8;
        /* +0x0, bits 7..0 */ u32 ref0: 8;
    } f; /* +0x0 */
} GXAlphaCompareReg;

/* The BP z-texture bias register (BP 0xF4). size: 0x4 */
typedef union GXZTexBiasReg {
    u32 word; /* +0x0 */
    struct GXZTexBiasRegFields { /* size: 0x4 */
        /* +0x0, bits 31..24 */ u32 opcode: 8;
        /* +0x0, bits 23..0 */ u32 bias: 24;
    } f; /* +0x0 */
} GXZTexBiasReg;

/* The BP z-texture mode register (BP 0xF5). size: 0x4 */
typedef union GXZTexModeReg {
    u32 word; /* +0x0 */
    struct GXZTexModeRegFields { /* size: 0x4 */
        /* +0x0, bits 31..24 */ u32 opcode: 8;
        /* +0x0, bits 23..4 */ u32 pad_0: 20;
        /* +0x0, bits 3..2 */ u32 op: 2;
        /* +0x0, bits 1..0 */ u32 type: 2;
    } f; /* +0x0 */
} GXZTexModeReg;

/* A `GXColor` read as one word: red in bits 31..24, green 23..16, blue 15..8, alpha 7..0. size: 0x4 */
typedef union GXColorWord {
    GXColor color; /* +0x0 */
    u32 word;      /* +0x0 */
} GXColorWord;

/* A `GXColorS10` read as two words: red and green, then blue and alpha. size: 0x8 */
typedef union GXColorS10Words {
    GXColorS10 color; /* +0x0 */
    struct GXColorS10WordPair { /* size: 0x8 */
        /* +0x0 */ u32 rg;
        /* +0x4 */ u32 ba;
    } w; /* +0x0 */
} GXColorS10Words;

/* The colour and alpha combiner words of the five `GXSetTevOp` modes, for stage 0 and for the later stages. */
u32 TEVCOpTableST0[5] = { 0xC008F8AF, 0xC008A89F, 0xC008AC8F, 0xC008FFF8, 0xC008FFFA };
u32 TEVCOpTableST1[5] = { 0xC008F80F, 0xC008089F, 0xC0080C8F, 0xC008FFF8, 0xC008FFF0 };
u32 TEVAOpTableST0[5] = { 0xC108F2F0, 0xC108FFD0, 0xC108F2F0, 0xC108FFC0, 0xC108FFD0 };
u32 TEVAOpTableST1[5] = { 0xC108F070, 0xC108FF80, 0xC108F070, 0xC108FFC0, 0xC108FF80 };

/* The rasterized colour selector of each colour channel id. */
s32 c2r[10] = { 0, 1, 0, 1, 0, 1, 7, 5, 6, 0 };

/* 0x804B9360 (0x94): Loads one of the five preset combiner setups into a stage. */
void GXSetTevOp(s32 stage, s32 mode)
{
    GXData* gx;
    u32* ctmp;
    u32* atmp;
    u32 reg;

    if (stage == 0) {
        ctmp = TEVCOpTableST0 + mode;
        atmp = TEVAOpTableST0 + mode;
    } else {
        ctmp = TEVCOpTableST1 + mode;
        atmp = TEVAOpTableST1 + mode;
    }

    gx = __GXData;
    reg = gx->tevc[stage].word;
    reg = (*ctmp & ~0xFF000000) | (reg & 0xFF000000);
    GXWGFifo.u8 = 0x61;
    GXWGFifo.u32 = reg;
    gx->tevc[stage].word = reg;

    reg = gx->teva[stage].word;
    reg = (*atmp & ~0xFF00000F) | (reg & 0xFF00000F);
    GXWGFifo.u8 = 0x61;
    GXWGFifo.u32 = reg;
    gx->teva[stage].word = reg;
    gx->xfWritePending = 0;
}

/* 0x804B9400 (0x40): Sets the four colour combiner inputs of a stage. */
void GXSetTevColorIn(u32 stage, u32 a, u32 b, u32 c, u32 d)
{
    GXData* gx = __GXData;
    GXTevColorEnvReg reg = gx->tevc[stage];

    reg.f.a = a;
    reg.f.b = b;
    reg.f.c = c;
    reg.f.d = d;
    GXWGFifo.u8 = 0x61;
    GXWGFifo.u32 = reg.word;
    gx->tevc[stage] = reg;
    gx->xfWritePending = 0;
}

/* 0x804B9440 (0x40): Sets the four alpha combiner inputs of a stage. */
void GXSetTevAlphaIn(u32 stage, u32 a, u32 b, u32 c, u32 d)
{
    GXData* gx = __GXData;
    GXTevAlphaEnvReg reg = gx->teva[stage];

    reg.f.a = a;
    reg.f.b = b;
    reg.f.c = c;
    reg.f.d = d;
    GXWGFifo.u8 = 0x61;
    GXWGFifo.u32 = reg.word;
    gx->teva[stage] = reg;
    gx->xfWritePending = 0;
}

/* 0x804B9480 (0x58): Sets the colour combiner operation, bias, scale, clamp and output register of a stage. */
void GXSetTevColorOp(u32 stage, s32 op, u32 bias, u32 scale, u8 clamp, u32 out_reg)
{
    GXTevColorEnvReg* p;
    u32 reg;

    p = &__GXData->tevc[stage];
    reg = p->word;
    GX_SET_REG(reg, op, 13, 13);
    if (op <= 1) {
        GX_SET_REG(reg, scale, 10, 11);
        GX_SET_REG(reg, bias, 14, 15);
    } else {
        GX_SET_REG(reg, (u32)op >> 1, 10, 11);
        GX_SET_REG(reg, 3, 14, 15);
    }
    GX_SET_REG(reg, clamp, 12, 12);
    GX_SET_REG(reg, out_reg, 8, 9);
    GXWGFifo.u8 = 0x61;
    GXWGFifo.u32 = reg;
    p->word = reg;
    __GXData->xfWritePending = 0;
}

/* 0x804B94E0 (0x58): Sets the alpha combiner operation, bias, scale, clamp and output register of a stage. */
void GXSetTevAlphaOp(u32 stage, s32 op, u32 bias, u32 scale, u8 clamp, u32 out_reg)
{
    GXTevAlphaEnvReg* p;
    u32 reg;

    p = &__GXData->teva[stage];
    reg = p->word;
    GX_SET_REG(reg, op, 13, 13);
    if (op <= 1) {
        GX_SET_REG(reg, scale, 10, 11);
        GX_SET_REG(reg, bias, 14, 15);
    } else {
        GX_SET_REG(reg, (u32)op >> 1, 10, 11);
        GX_SET_REG(reg, 3, 14, 15);
    }
    GX_SET_REG(reg, clamp, 12, 12);
    GX_SET_REG(reg, out_reg, 8, 9);
    GXWGFifo.u8 = 0x61;
    GXWGFifo.u32 = reg;
    p->word = reg;
    __GXData->xfWritePending = 0;
}

/* 0x804B9540 (0x60): Loads an 8-bit colour into a TEV colour register. */
void GXSetTevColor(u32 reg, GXColor color)
{
    GXColorWord rgba;
    GXTevColorRegA ra;
    GXTevColorRegB bg;

    rgba.color = color;
    ra.word = (0xE0 + reg * 2) << 24;
    ra.f.r = rgba.word >> 24;
    ra.f.a = rgba.word & 0xFF;

    bg.word = (0xE1 + reg * 2) << 24;
    bg.f.b = (rgba.word >> 8) & 0xFF;
    bg.f.g = (rgba.word >> 16) & 0xFF;

    GXWGFifo.u8 = 0x61;
    GXWGFifo.u32 = ra.word;
    GXWGFifo.u8 = 0x61;
    GXWGFifo.u32 = bg.word;
    GXWGFifo.u8 = 0x61;
    GXWGFifo.u32 = bg.word;
    GXWGFifo.u8 = 0x61;
    GXWGFifo.u32 = bg.word;
    __GXData->xfWritePending = 0;
}

/* 0x804B95A0 (0x64): Loads a signed 10-bit colour into a TEV colour register. */
void GXSetTevColorS10(s32 reg, GXColorS10 color)
{
    u32 rg = ((const GXColorS10Words*)&color)->w.rg;
    u32 ba = ((const GXColorS10Words*)&color)->w.ba;
    GXTevColorS10RegA ra;
    GXTevColorS10RegB bg;

    ra.word = (0xE0 + reg * 2) << 24;
    ra.f.r = (rg >> 16) & 0x7FF;
    ra.f.a = ba & 0x7FF;

    bg.word = (0xE1 + reg * 2) << 24;
    bg.f.b = (ba >> 16) & 0x7FF;
    bg.f.g = rg & 0x7FF;

    GXWGFifo.u8 = 0x61;
    GXWGFifo.u32 = ra.word;
    GXWGFifo.u8 = 0x61;
    GXWGFifo.u32 = bg.word;
    GXWGFifo.u8 = 0x61;
    GXWGFifo.u32 = bg.word;
    GXWGFifo.u8 = 0x61;
    GXWGFifo.u32 = bg.word;
    __GXData->xfWritePending = 0;
}

/* 0x804B9610 (0x5C): Loads a constant colour into a TEV konst register. */
void GXSetTevKColor(u32 id, GXColor color)
{
    GXColorWord rgba;
    GXTevColorRegA ra;
    GXTevColorRegB bg;

    rgba.color = color;
    ra.word = (0xE0 + id * 2) << 24;
    ra.f.r = rgba.word >> 24;
    ra.f.a = rgba.word & 0xFF;
    ra.f.konst = 8;

    bg.word = (0xE1 + id * 2) << 24;
    bg.f.b = (rgba.word >> 8) & 0xFF;
    bg.f.g = (rgba.word >> 16) & 0xFF;
    bg.f.konst = 8;

    GXWGFifo.u8 = 0x61;
    GXWGFifo.u32 = ra.word;
    GXWGFifo.u8 = 0x61;
    GXWGFifo.u32 = bg.word;
    __GXData->xfWritePending = 0;
}

/* 0x804B9670 (0x50): Selects the konst colour a stage reads. */
void GXSetTevKColorSel(u32 stage, u32 sel)
{
    GXData* gx = __GXData;
    GXTevKSelReg* reg = &gx->tevKsel[stage >> 1];

    if (stage & 1) {
        reg->f.kcsel1 = sel;
    } else {
        reg->f.kcsel0 = sel;
    }
    GXWGFifo.u8 = 0x61;
    GXWGFifo.u32 = reg->word;
    gx->xfWritePending = 0;
}

/* 0x804B96C0 (0x50): Selects the konst alpha a stage reads. */
void GXSetTevKAlphaSel(u32 stage, u32 sel)
{
    GXData* gx = __GXData;
    GXTevKSelReg* reg = &gx->tevKsel[stage >> 1];

    if (stage & 1) {
        reg->f.kasel1 = sel;
    } else {
        reg->f.kasel0 = sel;
    }
    GXWGFifo.u8 = 0x61;
    GXWGFifo.u32 = reg->word;
    gx->xfWritePending = 0;
}

/* 0x804B9710 (0x3C): Selects the rasterized and texture swap tables of a stage. */
void GXSetTevSwapMode(u32 stage, u32 ras_sel, u32 tex_sel)
{
    GXData* gx = __GXData;
    GXTevAlphaEnvReg* reg = &gx->teva[stage];

    reg->f.rswap = ras_sel;
    reg->f.tswap = tex_sel;
    GXWGFifo.u8 = 0x61;
    GXWGFifo.u32 = reg->word;
    gx->xfWritePending = 0;
}

/* 0x804B9750 (0x7C): Sets the four channel selectors of one swap-mode table. */
void GXSetTevSwapModeTable(u32 table, u32 red, u32 green, u32 blue, u32 alpha)
{
    GXTevKSelReg* reg;

    reg = &__GXData->tevKsel[table << 1];
    reg->f.swapRB = red;
    reg->f.swapGA = green;
    GXWGFifo.u8 = 0x61;
    GXWGFifo.u32 = reg->word;

    reg = &__GXData->tevKsel[(table << 1) + 1];
    reg->f.swapRB = blue;
    reg->f.swapGA = alpha;
    GXWGFifo.u8 = 0x61;
    GXWGFifo.u32 = reg->word;
    __GXData->xfWritePending = 0;
}

/* 0x804B97D0 (0x38): Sets the two alpha compare functions, their references and the logic combining them. */
void GXSetAlphaCompare(u32 comp0, u8 ref0, u32 op, u32 comp1, u8 ref1)
{
    GXData* gx = __GXData;
    GXAlphaCompareReg reg;

    reg.word = 0xF3000000;
    reg.f.ref0 = ref0;
    reg.f.ref1 = ref1;
    reg.f.comp0 = comp0;
    reg.f.comp1 = comp1;
    reg.f.op = op;
    GXWGFifo.u8 = 0x61;
    GXWGFifo.u32 = reg.word;
    gx->xfWritePending = 0;
}

/* 0x804B9810 (0x84): Sets the z-texture operation, format and bias. */
void GXSetZTexture(u32 op, s32 fmt, u32 bias)
{
    GXZTexBiasReg zenv0;
    GXZTexModeReg zenv1;
    u32 type;

    zenv0.word = 0;
    zenv0.f.bias = bias;
    zenv0.f.opcode = 0xF4;

    zenv1.word = 0;
    switch (fmt) {
    case 0x11:
        type = 0;
        break;
    case 0x13:
        type = 1;
        break;
    case 0x16:
        type = 2;
        break;
    default:
        type = 2;
        break;
    }
    zenv1.f.type = type;
    zenv1.f.op = op;
    zenv1.f.opcode = 0xF5;

    GXWGFifo.u8 = 0x61;
    GXWGFifo.u32 = zenv0.word;
    GXWGFifo.u8 = 0x61;
    GXWGFifo.u32 = zenv1.word;
    __GXData->xfWritePending = 0;
}

/* 0x804B98A0 (0x15C): Sets the texture coordinate, texture map and colour channel a stage reads. */
void GXSetTevOrder(s32 stage, s32 coord, s32 map, s32 color)
{
    GXTevOrderReg* ptref;
    GXTevOrderReg reg;
    u32 tmap;
    u32 tcoord;

    ptref = &__GXData->tref[stage / 2];
    __GXData->texmapId[stage] = map;

    tmap = map & ~0x100;
    tmap = (tmap >= 8) ? 0 : tmap;

    if (coord >= 8) {
        tcoord = 0;
        __GXData->tevTcEnab &= ~(1 << stage);
    } else {
        tcoord = coord;
        __GXData->tevTcEnab |= (1 << stage);
    }

    if (stage & 1) {
        reg = *ptref;
        reg.f.map1 = tmap;
        reg.f.coord1 = tcoord;
        *ptref = reg;
        reg.f.color1 = (color == 0xFF) ? 7 : c2r[color];
        *ptref = reg;
        reg.f.enable1 = (map != 0xFF && !(map & 0x100));
        *ptref = reg;
    } else {
        reg = *ptref;
        reg.f.map0 = tmap;
        reg.f.coord0 = tcoord;
        *ptref = reg;
        reg.f.color0 = (color == 0xFF) ? 7 : c2r[color];
        *ptref = reg;
        reg.f.enable0 = (map != 0xFF && !(map & 0x100));
        *ptref = reg;
    }

    GXWGFifo.u8 = 0x61;
    GXWGFifo.u32 = ptref->word;
    __GXData->xfWritePending = 0;
    __GXData->dirtyState |= 1;
}

/* 0x804B9A00 (0x24): Sets the number of active TEV stages. */
void GXSetNumTevStages(u8 nStages)
{
    GX_SET_REG(__GXData->genMode.word, nStages - 1, 18, 21);
    __GXData->dirtyState |= 4;
}
