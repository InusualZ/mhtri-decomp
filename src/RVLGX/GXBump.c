/*
 * RVLGX/GXBump.c - the SDK GX indirect-texture (bump) setup.
 *
 * RANGE. `.text` 0x804B8F00-0x804B9360; `.sdata2` 0x8079D198-0x8079D1A0 (9 functions / 0x430 B).
 *   - `.sdata2` 0x8079D198 (the 1024.0f matrix scale, plus link padding) is read by `GXSetIndTexMtx` only
 * FLAGS. the `OS` lib block of `configure.py` (`Wii/1.3`) with `cflags_base` (-O4,p, -enum int).
 * NAMES. SDK names (Revolution SDK `GXBump.c`). `GXTevIndirectReg` and `GXIndTexMtxReg` are local bitfield views of
 *   the BP words the two setters build from zero; the words `GXData` keeps use the unions of `RVLGX/GXInit.h`.
 * SHAPES. `RVLGX/GXInit.h` declares `__GXData` `GXData* const` (it lives in `.sdata2`), so MWCC schedules its load
 *   across the FIFO stores as retail does. `GXSetIndTexMtx` converts the six matrix entries column by column
 *   (`[0][0]`, `[1][0]`, `[0][1]`, ...), the order of retail's conversion slots.
 * RESIDUALS. 8 of 9 functions match; all 9 bodies are written.
 *   - `GXSetIndTexMtx`: retail adds 0x11 to `scale_exp` with no `extsb` and inserts bits 2..3/4..5 by rotation; the
 *     s8 `+=` adds one `extsb` and two `srawi` (0x14C vs 0x140, +0xC), which moves the seven later functions by 0x10.
 *     `(scale_exp >> n) & 3` gives retail's size and its three `rlwimi`s but moves `id` off r6 and reorders the
 *     float conversions, and scores lower; a `u32`/`s32`/`u8` exponent local and per-register `ma`/`mb` locals
 *     score lower too
 *   - flipcheck: `.text` 0x464 vs the claimed 0x460 (the +0xC above net of the 0xC tail padding; most of the
 *     differing bytes are the 0x10 shift of the later functions); `.sdata2` claim is 8 B (1024.0f + link padding)
 *     while the object emits 4 B, and the pool tool lists the literal as shared with the other `RVLGX/` units
 *     (candidate fold, medium confidence)
 */

#include "RVLGX/GXBump.h"
#include "RVLGX/GXInit.h"

/* The BP TEV indirect register (BP 0x10 + stage). size: 0x4 */
typedef union GXTevIndirectReg {
    u32 word; /* +0x0 */
    struct {
        /* +0x0, bits 31..24 */ u32 opcode: 8;
        /* +0x0, bits 23..21 */ u32 pad_0: 3;
        /* +0x0, bit 20 */ u32 addPrev: 1;
        /* +0x0, bit 19 */ u32 indLod: 1;
        /* +0x0, bits 18..16 */ u32 wrapT: 3;
        /* +0x0, bits 15..13 */ u32 wrapS: 3;
        /* +0x0, bits 12..9 */ u32 matrixSel: 4;
        /* +0x0, bits 8..7 */ u32 alphaSel: 2;
        /* +0x0, bits 6..4 */ u32 biasSel: 3;
        /* +0x0, bits 3..2 */ u32 format: 2;
        /* +0x0, bits 1..0 */ u32 indStage: 2;
    } f; /* +0x0 */
} GXTevIndirectReg;

/* One row of an indirect texture matrix register (BP 0x06 + 3 * id + row): two 11-bit matrix entries and two bits
 * of the scale exponent. size: 0x4 */
typedef union GXIndTexMtxReg {
    u32 word; /* +0x0 */
    struct {
        /* +0x0, bits 31..24 */ u32 opcode: 8;
        /* +0x0, bits 23..22 */ u32 scaleExp: 2;
        /* +0x0, bits 21..11 */ u32 mb: 11;
        /* +0x0, bits 10..0 */ u32 ma: 11;
    } f; /* +0x0 */
} GXIndTexMtxReg;

#define GX_TEXMAP_NULL 0xFF
#define GX_TEXCOORD_NULL 0xFF

/* 0x804B8F00 (0x68): Sets the indirect texture operation of one TEV stage. */
void GXSetTevIndirect(u32 tev_stage, u32 ind_stage, u32 format, u32 bias_sel, u32 matrix_sel, u32 wrap_s,
                      u32 wrap_t, u8 add_prev, u8 ind_lod, u32 alpha_sel)
{
    GXData* gx;
    GXTevIndirectReg reg;

    reg.word = 0;
    reg.f.indStage = ind_stage;
    reg.f.format = format;
    reg.f.biasSel = bias_sel;
    reg.f.alphaSel = alpha_sel;
    reg.f.matrixSel = matrix_sel;
    reg.f.wrapS = wrap_s;
    reg.f.wrapT = wrap_t;
    reg.f.indLod = ind_lod;
    reg.f.addPrev = add_prev;
    reg.f.opcode = tev_stage + 0x10;
    GXWGFifo.u8 = 0x61;
    gx = __GXData;
    GXWGFifo.u32 = reg.word;
    gx->xfWritePending = 0;
}

/* 0x804B8F70 (0x140): Loads one indirect texture matrix and its scale exponent. */
void GXSetIndTexMtx(u32 mtx_id, const f32 offset[2][3], s8 scale_exp)
{
    s32 mtx[6];
    GXIndTexMtxReg reg;
    u32 id;
    GXData* gx;

    switch (mtx_id) {
    case GX_ITM_0:
    case GX_ITM_1:
    case GX_ITM_2:
        id = mtx_id - 1;
        break;
    case GX_ITM_S0:
    case GX_ITM_S1:
    case GX_ITM_S2:
        id = mtx_id - 5;
        break;
    case GX_ITM_T0:
    case GX_ITM_T1:
    case GX_ITM_T2:
        id = mtx_id - 9;
        break;
    default:
        id = 0;
        break;
    }

    mtx[0] = (s32)(1024.0f * offset[0][0]);
    mtx[3] = (s32)(1024.0f * offset[1][0]);
    mtx[1] = (s32)(1024.0f * offset[0][1]);
    mtx[4] = (s32)(1024.0f * offset[1][1]);
    mtx[2] = (s32)(1024.0f * offset[0][2]);
    mtx[5] = (s32)(1024.0f * offset[1][2]);
    scale_exp += 0x11;

    reg.word = 0;
    reg.f.ma = mtx[0];
    reg.f.mb = mtx[3];
    reg.f.scaleExp = scale_exp;
    reg.f.opcode = id * 3 + 6;
    GXWGFifo.u8 = 0x61;
    GXWGFifo.u32 = reg.word;

    reg.word = 0;
    reg.f.ma = mtx[1];
    reg.f.mb = mtx[4];
    reg.f.scaleExp = scale_exp >> 2;
    reg.f.opcode = id * 3 + 7;
    GXWGFifo.u8 = 0x61;
    GXWGFifo.u32 = reg.word;

    reg.word = 0;
    reg.f.ma = mtx[2];
    reg.f.mb = mtx[5];
    reg.f.scaleExp = scale_exp >> 4;
    reg.f.opcode = id * 3 + 8;
    GXWGFifo.u8 = 0x61;
    gx = __GXData;
    GXWGFifo.u32 = reg.word;
    gx->xfWritePending = 0;
}

/* 0x804B90B0 (0x100): Sets the texture coordinate scale of one indirect stage. */
void GXSetIndTexCoordScale(s32 ind_stage, u32 scale_s, u32 scale_t)
{
    GXData* gx;

    switch (ind_stage) {
    case 0:
        gx = __GXData;
        gx->indTexScale0.f.s0 = scale_s;
        gx->indTexScale0.f.t0 = scale_t;
        gx->indTexScale0.f.opcode = 0x25;
        GXWGFifo.u8 = 0x61;
        GXWGFifo.u32 = gx->indTexScale0.word;
        break;
    case 1:
        gx = __GXData;
        gx->indTexScale0.f.s1 = scale_s;
        gx->indTexScale0.f.t1 = scale_t;
        gx->indTexScale0.f.opcode = 0x25;
        GXWGFifo.u8 = 0x61;
        GXWGFifo.u32 = gx->indTexScale0.word;
        break;
    case 2:
        gx = __GXData;
        gx->indTexScale1.f.s0 = scale_s;
        gx->indTexScale1.f.t0 = scale_t;
        gx->indTexScale1.f.opcode = 0x26;
        GXWGFifo.u8 = 0x61;
        GXWGFifo.u32 = gx->indTexScale1.word;
        break;
    case 3:
        gx = __GXData;
        gx->indTexScale1.f.s1 = scale_s;
        gx->indTexScale1.f.t1 = scale_t;
        gx->indTexScale1.f.opcode = 0x26;
        GXWGFifo.u8 = 0x61;
        GXWGFifo.u32 = gx->indTexScale1.word;
        break;
    }
    __GXData->xfWritePending = 0;
}

/* 0x804B91B0 (0xC8): Sets the texture coordinate and map an indirect stage reads. */
void GXSetIndTexOrder(s32 ind_stage, s32 tex_coord, s32 tex_map)
{
    GXData* gx;

    if (tex_map == GX_TEXMAP_NULL) {
        tex_map = 0;
    }
    if (tex_coord == GX_TEXCOORD_NULL) {
        tex_coord = 0;
    }

    switch (ind_stage) {
    case 0:
        gx = __GXData;
        gx->iref.f.map0 = tex_map;
        gx->iref.f.coord0 = tex_coord;
        break;
    case 1:
        gx = __GXData;
        gx->iref.f.map1 = tex_map;
        gx->iref.f.coord1 = tex_coord;
        break;
    case 2:
        gx = __GXData;
        gx->iref.f.map2 = tex_map;
        gx->iref.f.coord2 = tex_coord;
        break;
    case 3:
        gx = __GXData;
        gx->iref.f.map3 = tex_map;
        gx->iref.f.coord3 = tex_coord;
        break;
    }
    GXWGFifo.u8 = 0x61;
    gx = __GXData;
    GXWGFifo.u32 = gx->iref.word;
    gx->dirtyState |= 3;
    gx->xfWritePending = 0;
}

/* 0x804B9280 (0x20): Sets the number of active indirect stages. */
void GXSetNumIndStages(u8 nIndStages)
{
    GXData* gx = __GXData;

    gx->genMode.f.numIndStages = nIndStages;
    gx->dirtyState |= 6;
}

/* 0x804B92A0 (0x48): Turns indirect texturing off for one TEV stage. */
void GXSetTevDirect(u32 tev_stage)
{
    GXSetTevIndirect(tev_stage, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

/* 0x804B92F0 (0x4): Does nothing; the BP mask is sent by the setters themselves. */
void __GXUpdateBPMask(void)
{
}

/* 0x804B9300 (0x30): Sets the indirect texture map mask and sends it. */
void __GXSetIndirectMask(u32 mask)
{
    GXData* gx = __GXData;

    gx->bpMask.f.mask = mask;
    GXWGFifo.u8 = 0x61;
    GXWGFifo.u32 = gx->bpMask.word;
    gx->xfWritePending = 0;
}

/* 0x804B9330 (0x24): Resends the indirect texture map mask. */
void __GXFlushTextureState(void)
{
    GXData* gx;

    GXWGFifo.u8 = 0x61;
    gx = __GXData;
    GXWGFifo.u32 = gx->bpMask.word;
    gx->xfWritePending = 0;
}
