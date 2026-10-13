/*
 * RVLGX/GXAttr.c - the SDK GX vertex-attribute setup (descriptors, formats, arrays, texcoord generation).
 *
 * RANGE. `.text` 0x804B4E40-0x804B5DB0; `.data` 0x8061A7D0-0x8061A9C0; `.sdata` 0x80793E50-0x80793E60 (13 functions /
 *   0xF24 B).
 *   - six `.data` jump tables 0x8061A7D0..0x8061A9C0 are read by `GXSetVtxDesc`, `GXGetVtxDesc`, the two attribute-
 *     format setters, `GXGetVtxAttrFmt` and `GXSetTexCoordGen2`; `.sdata` 0x80793E50..0x80793E60 by
 *     `__GXCalculateVLim`
 * FLAGS. the `OS` lib block of `configure.py` (`Wii/1.3`) with `cflags_base` (-O4,p); the target has no float
 *   operation, so there is no `-fp_contract` evidence either way.
 * NAMES. SDK names throughout, including the `static inline` helpers `__GXXfVtxSpecs` and `SetVAT` (the SDK's
 *   `SETVAT`); the VLim size tables `tbl1`/`tbl2`/`tbl3` keep the SDK's own (function-local in the SDK source) names.
 * RESIDUALS. `__GXSetVCD`: the instruction sequence is the target's, the register allocation is not - the target
 *   composes the XF word as `or r5,r5,r7` (`nTex << 4` first operand, `nCols` second) into r5/r4 with the second pipe
 *   base in r6, ours `or r6,r7,r6` with the base in r4 (`global_optimizer off` makes it worse). flipcheck names
 *   that function's 15 bytes as the `.text` blocker. Jump tables are emitted as compiler-local `@NNN` while the target object names them
 *   `jumptable_8061A7D0`..`jumptable_8061A96C` (relocdiff reports only that naming). The map sizes `tbl3` 0x8 (a
 *   4-byte table plus 4 bytes of tail pad): the target `.sdata` is 0x10 while ours is 0xC; the `GXTexture` `.sdata`
 *   that follows is 8-aligned, so a flip that refuses on that size resizes `tbl3` to 0x4 in a separate map change.
 *   The right edge (0x804B5DB0, `GXSetMisc`) lies in a gap with no data evidence (`GXSetNumTexGens` is the last
 *   attribute function; naming order decides). The pool census reports the GX units as one fold candidate through a
 *   single `.sdata2` word, `__GXData` (0x8079D0F0), an SDK global defined in `RVLGX/GXInit.c` and not a pooled
 *   literal. Debt outside this unit (rule 2): local declarations of this unit's functions in `font/flfnt.cpp`
 *   (`GXSetTexCoordGen2`), `main.cpp` (`GXInvalidateVtxCache`), the `ef/ef_draw*strategy` units, `fn_80056F24.cpp`
 *   and `sound/fn_800E3CBC.cpp`.
 * SHAPES. the VLim tables are file-scope `static u8` arrays: as function-local statics MWCC emits them as weak
 *   `@LOCAL@__GXCalculateVLim__Fv@tbl1` objects instead of the target's local `tbl1`..`tbl3`. Register inserts use
 *   `GX_SET_REG` on the record's `word` (one `rlwimi` per field in the target's order); each `GXSetVtxDesc` case
 *   reads `__GXData` again, as the target reloads it. The two format setters share one `static inline` setter.
 *   `__GXCalculateVLim` and `GXSetVtxAttrFmtv` sit in a `#pragma global_optimizer off` scope: with it on, the VLim
 *   schedule loads `tbl3[position]` before `tbl3[normal]` and the format loop reuses the loop test's `list->attr`
 *   instead of reloading it (the whole file off loses five other functions). `__GXSetVCD` builds the XF word as
 *   `nCols | nTex << 4` then `|= nNrm << 2`; `GXGetVtxAttrFmt` collects the normal fraction in a local so the three
 *   arms share one `stb`.
 */

#pragma function_align 16

#include "types.h"
#include "gx.h"
#include "RVLGX/GXAttr.h"
#include "RVLGX/GXInit.h"

/* Writes `value` to the CP register `addr` through the write-gather pipe. */
#define GX_WRITE_CP_REG(addr, value) \
    (GXWGFifo.u8 = 0x08, GXWGFifo.u8 = (u8)(addr), GXWGFifo.u32 = (u32)(value))

/* Writes `value` to the XF register `addr` through the write-gather pipe. */
#define GX_WRITE_XF_REG(addr, value) \
    (GXWGFifo.u8 = 0x10, GXWGFifo.u32 = (u32)(addr), GXWGFifo.u32 = (u32)(value))

/* 0x804B4E40 (0x264): Sets how vertex attribute `attr` is sent (none, direct, 8- or 16-bit index). */
void GXSetVtxDesc(s32 attr, s32 type)
{
    switch (attr) {
    case 0: GX_SET_REG(__GXData->vcdLo.word, type, 31, 31); break;
    case 1: GX_SET_REG(__GXData->vcdLo.word, type, 30, 30); break;
    case 2: GX_SET_REG(__GXData->vcdLo.word, type, 29, 29); break;
    case 3: GX_SET_REG(__GXData->vcdLo.word, type, 28, 28); break;
    case 4: GX_SET_REG(__GXData->vcdLo.word, type, 27, 27); break;
    case 5: GX_SET_REG(__GXData->vcdLo.word, type, 26, 26); break;
    case 6: GX_SET_REG(__GXData->vcdLo.word, type, 25, 25); break;
    case 7: GX_SET_REG(__GXData->vcdLo.word, type, 24, 24); break;
    case 8: GX_SET_REG(__GXData->vcdLo.word, type, 23, 23); break;
    case 9: GX_SET_REG(__GXData->vcdLo.word, type, 21, 22); break;
    case 10:
        if (type != 0) {
            __GXData->hasNrms = 1;
            __GXData->hasBiNrms = 0;
            __GXData->nrmType = type;
        } else {
            __GXData->hasNrms = 0;
        }
        break;
    case 25:
        if (type != 0) {
            __GXData->hasBiNrms = 1;
            __GXData->hasNrms = 0;
            __GXData->nrmType = type;
        } else {
            __GXData->hasBiNrms = 0;
        }
        break;
    case 11: GX_SET_REG(__GXData->vcdLo.word, type, 17, 18); break;
    case 12: GX_SET_REG(__GXData->vcdLo.word, type, 15, 16); break;
    case 13: GX_SET_REG(__GXData->vcdHi.word, type, 30, 31); break;
    case 14: GX_SET_REG(__GXData->vcdHi.word, type, 28, 29); break;
    case 15: GX_SET_REG(__GXData->vcdHi.word, type, 26, 27); break;
    case 16: GX_SET_REG(__GXData->vcdHi.word, type, 24, 25); break;
    case 17: GX_SET_REG(__GXData->vcdHi.word, type, 22, 23); break;
    case 18: GX_SET_REG(__GXData->vcdHi.word, type, 20, 21); break;
    case 19: GX_SET_REG(__GXData->vcdHi.word, type, 18, 19); break;
    case 20: GX_SET_REG(__GXData->vcdHi.word, type, 16, 17); break;
    }
    if (__GXData->hasNrms || __GXData->hasBiNrms) {
        GX_SET_REG(__GXData->vcdLo.word, __GXData->nrmType, 19, 20);
    } else {
        GX_SET_REG(__GXData->vcdLo.word, 0, 19, 20);
    }
    __GXData->dirtyState |= 8;
}

/* Sends the XF vertex specification: the colour, normal and texture-coordinate counts the descriptor enables. */
static inline void __GXXfVtxSpecs(void)
{
    u32 nCols;
    u32 nNrm;
    u32 nTex;
    u32 reg;

    nNrm = __GXData->hasBiNrms ? 2 : (__GXData->hasNrms != 0);
    nCols = 33 - __cntlzw((__GXData->vcdLo.word >> 13) & 0xF);
    nCols /= 2;
    nTex = 33 - __cntlzw(__GXData->vcdHi.word & 0xFFFF);
    nTex /= 2;
    reg = nCols | (nTex << 4);
    reg |= nNrm << 2;
    GX_WRITE_XF_REG(0x1008, reg);
    __GXData->xfWritePending = 1;
}

/* 0x804B50B0 (0xB0): Sends the vertex descriptor to the CP and the vertex specification to the XF. */
void __GXSetVCD(void)
{
    GX_WRITE_CP_REG(0x50, __GXData->vcdLo.word);
    GX_WRITE_CP_REG(0x60, __GXData->vcdHi.word);
    __GXXfVtxSpecs();
}

static u8 tbl1[] = {0, 4, 1, 2};
static u8 tbl2[] = {0, 8, 1, 2};
static u8 tbl3[] = {0, 12, 1, 2};

#pragma push
#pragma global_optimizer off
/* 0x804B5160 (0x12C): Computes the size of one vertex in the CP's units from the descriptor and format 0. */
void __GXCalculateVLim(void)
{
    GXData* gx = __GXData;
    s32 nc;
    u32 vlm;
    u32 vl;
    u32 vh;
    u32 va;

    if (gx->vNum == 0) {
        return;
    }
    vl = gx->vcdLo.word;
    vh = gx->vcdHi.word;
    va = gx->vatA[0].word;
    nc = (va >> 9) & 1;
    vlm = vl & 1;
    vlm += (vl >> 1) & 1;
    vlm += (vl >> 2) & 1;
    vlm += (vl >> 3) & 1;
    vlm += (vl >> 4) & 1;
    vlm += (vl >> 5) & 1;
    vlm += (vl >> 6) & 1;
    vlm += (vl >> 7) & 1;
    vlm += (vl >> 8) & 1;
    vlm += tbl3[(vl >> 9) & 3];
    vlm += tbl3[(vl >> 11) & 3] * ((nc == 1) ? 3 : 1);
    vlm += tbl1[(vl >> 13) & 3];
    vlm += tbl1[(vl >> 15) & 3];
    vlm += tbl2[vh & 3];
    vlm += tbl2[(vh >> 2) & 3];
    vlm += tbl2[(vh >> 4) & 3];
    vlm += tbl2[(vh >> 6) & 3];
    vlm += tbl2[(vh >> 8) & 3];
    vlm += tbl2[(vh >> 10) & 3];
    vlm += tbl2[(vh >> 12) & 3];
    vlm += tbl2[(vh >> 14) & 3];
    gx->vLim = vlm;
}
#pragma pop

/* 0x804B5290 (0x1B4): Returns how vertex attribute `attr` is sent. */
void GXGetVtxDesc(s32 attr, s32* type)
{
    u32 cpType;

    switch (attr) {
    case 0: cpType = __GXData->vcdLo.word & 1; break;
    case 1: cpType = (__GXData->vcdLo.word >> 1) & 1; break;
    case 2: cpType = (__GXData->vcdLo.word >> 2) & 1; break;
    case 3: cpType = (__GXData->vcdLo.word >> 3) & 1; break;
    case 4: cpType = (__GXData->vcdLo.word >> 4) & 1; break;
    case 5: cpType = (__GXData->vcdLo.word >> 5) & 1; break;
    case 6: cpType = (__GXData->vcdLo.word >> 6) & 1; break;
    case 7: cpType = (__GXData->vcdLo.word >> 7) & 1; break;
    case 8: cpType = (__GXData->vcdLo.word >> 8) & 1; break;
    case 9: cpType = (__GXData->vcdLo.word >> 9) & 3; break;
    case 10: cpType = __GXData->hasNrms ? (__GXData->vcdLo.word >> 11) & 3 : 0; break;
    case 25: cpType = __GXData->hasBiNrms ? (__GXData->vcdLo.word >> 11) & 3 : 0; break;
    case 11: cpType = (__GXData->vcdLo.word >> 13) & 3; break;
    case 12: cpType = (__GXData->vcdLo.word >> 15) & 3; break;
    case 13: cpType = __GXData->vcdHi.word & 3; break;
    case 14: cpType = (__GXData->vcdHi.word >> 2) & 3; break;
    case 15: cpType = (__GXData->vcdHi.word >> 4) & 3; break;
    case 16: cpType = (__GXData->vcdHi.word >> 6) & 3; break;
    case 17: cpType = (__GXData->vcdHi.word >> 8) & 3; break;
    case 18: cpType = (__GXData->vcdHi.word >> 10) & 3; break;
    case 19: cpType = (__GXData->vcdHi.word >> 12) & 3; break;
    case 20: cpType = (__GXData->vcdHi.word >> 14) & 3; break;
    default: cpType = 0; break;
    }
    *type = cpType;
}

/* 0x804B5450 (0x34): Resets the vertex descriptor to a direct position and nothing else. */
void GXClearVtxDesc(void)
{
    GXData* gx = __GXData;

    gx->vcdLo.word = 0;
    GX_SET_REG(gx->vcdLo.word, 1, 21, 22);
    gx->vcdHi.word = 0;
    gx->hasNrms = 0;
    gx->hasBiNrms = 0;
    gx->dirtyState |= 8;
}

/* Inserts the component count, type and fraction of attribute `attr` into the format's three VAT words. */
static inline void SetVAT(GXVatAReg* va, GXVatBReg* vb, GXVatCReg* vc, s32 attr, s32 cnt, s32 type, u8 frac)
{
    switch (attr) {
    case 9:
        GX_SET_REG(va->word, cnt, 31, 31);
        GX_SET_REG(va->word, type, 28, 30);
        GX_SET_REG(va->word, frac, 23, 27);
        break;
    case 10:
    case 25:
        GX_SET_REG(va->word, type, 19, 21);
        if (cnt == 2) {
            GX_SET_REG(va->word, 1, 22, 22);
            GX_SET_REG(va->word, 1, 0, 0);
        } else {
            GX_SET_REG(va->word, cnt, 22, 22);
            GX_SET_REG(va->word, 0, 0, 0);
        }
        break;
    case 11:
        GX_SET_REG(va->word, cnt, 18, 18);
        GX_SET_REG(va->word, type, 15, 17);
        break;
    case 12:
        GX_SET_REG(va->word, cnt, 14, 14);
        GX_SET_REG(va->word, type, 11, 13);
        break;
    case 13:
        GX_SET_REG(va->word, cnt, 10, 10);
        GX_SET_REG(va->word, type, 7, 9);
        GX_SET_REG(va->word, frac, 2, 6);
        break;
    case 14:
        GX_SET_REG(vb->word, cnt, 31, 31);
        GX_SET_REG(vb->word, type, 28, 30);
        GX_SET_REG(vb->word, frac, 23, 27);
        break;
    case 15:
        GX_SET_REG(vb->word, cnt, 22, 22);
        GX_SET_REG(vb->word, type, 19, 21);
        GX_SET_REG(vb->word, frac, 14, 18);
        break;
    case 16:
        GX_SET_REG(vb->word, cnt, 13, 13);
        GX_SET_REG(vb->word, type, 10, 12);
        GX_SET_REG(vb->word, frac, 5, 9);
        break;
    case 17:
        GX_SET_REG(vb->word, cnt, 4, 4);
        GX_SET_REG(vb->word, type, 1, 3);
        GX_SET_REG(vc->word, frac, 27, 31);
        break;
    case 18:
        GX_SET_REG(vc->word, cnt, 26, 26);
        GX_SET_REG(vc->word, type, 23, 25);
        GX_SET_REG(vc->word, frac, 18, 22);
        break;
    case 19:
        GX_SET_REG(vc->word, cnt, 17, 17);
        GX_SET_REG(vc->word, type, 14, 16);
        GX_SET_REG(vc->word, frac, 9, 13);
        break;
    case 20:
        GX_SET_REG(vc->word, cnt, 8, 8);
        GX_SET_REG(vc->word, type, 5, 7);
        GX_SET_REG(vc->word, frac, 0, 4);
        break;
    }
}

/* 0x804B5490 (0x1A0): Sets the component count, type and fraction of one attribute in vertex format `vtxfmt`. */
void GXSetVtxAttrFmt(u32 vtxfmt, s32 attr, s32 cnt, s32 type, u8 frac)
{
    GXVatAReg* va;
    GXVatBReg* vb;
    GXVatCReg* vc;

    va = &__GXData->vatA[vtxfmt];
    vb = &__GXData->vatB[vtxfmt];
    vc = &__GXData->vatC[vtxfmt];
    SetVAT(va, vb, vc, attr, cnt, type, frac);
    __GXData->dirtyState |= 0x10;
    __GXData->dirtyVAT |= (u8)(1 << (u8)vtxfmt);
}

#pragma push
#pragma global_optimizer off
/* 0x804B5630 (0x1C8): Sets the format of every attribute in `list` (ended by attr 0xFF) for vertex format `vtxfmt`. */
void GXSetVtxAttrFmtv(u32 vtxfmt, const GXVtxAttrFmtList* list)
{
    GXVatAReg* va;
    GXVatBReg* vb;
    GXVatCReg* vc;

    va = &__GXData->vatA[vtxfmt];
    vb = &__GXData->vatB[vtxfmt];
    vc = &__GXData->vatC[vtxfmt];
    for (; list->attr != 0xFF; list++) {
        SetVAT(va, vb, vc, list->attr, list->cnt, list->type, list->frac);
    }
    __GXData->dirtyState |= 0x10;
    __GXData->dirtyVAT |= (u8)(1 << (u8)vtxfmt);
}
#pragma pop

/* 0x804B5800 (0x80): Sends the VAT words of every vertex format marked dirty to the CP. */
void __GXSetVAT(void)
{
    s32 i;
    u32 dirty;

    dirty = __GXData->dirtyVAT;
    i = 0;
    do {
        if (dirty & 1) {
            GX_WRITE_CP_REG(i | 0x70, __GXData->vatA[i].word);
            GX_WRITE_CP_REG(i | 0x80, __GXData->vatB[i].word);
            GX_WRITE_CP_REG(i | 0x90, __GXData->vatC[i].word);
        }
        dirty >>= 1;
        i++;
    } while (dirty != 0);
    GXWGFifo.u8 = 0;
    __GXData->dirtyVAT = 0;
}

/* 0x804B5880 (0x25C): Returns the component count, type and fraction of one attribute in vertex format `vtxfmt`. */
void GXGetVtxAttrFmt(u32 vtxfmt, s32 attr, s32* cnt, s32* type, u8* frac)
{
    GXVatAReg* va;
    GXVatBReg* vb;
    GXVatCReg* vc;
    u8 nrmFrac;

    va = &__GXData->vatA[vtxfmt];
    vb = &__GXData->vatB[vtxfmt];
    vc = &__GXData->vatC[vtxfmt];
    switch (attr) {
    case 9:
        *cnt = va->word & 1;
        *type = (va->word >> 1) & 7;
        *frac = (va->word >> 4) & 0x1F;
        return;
    case 10:
    case 25:
        *cnt = (va->word >> 9) & 1;
        if (*cnt == 1 && (va->word >> 31)) {
            *cnt = 2;
        }
        *type = (va->word >> 10) & 7;
        switch (*type) {
        case 1: nrmFrac = 6; break;
        case 3: nrmFrac = 14; break;
        default: nrmFrac = 0; break;
        }
        *frac = nrmFrac;
        return;
    case 11:
        *cnt = (va->word >> 13) & 1;
        *type = (va->word >> 14) & 7;
        *frac = 0;
        return;
    case 12:
        *cnt = (va->word >> 17) & 1;
        *type = (va->word >> 18) & 7;
        *frac = 0;
        return;
    case 13:
        *cnt = (va->word >> 21) & 1;
        *type = (va->word >> 22) & 7;
        *frac = (va->word >> 25) & 0x1F;
        return;
    case 14:
        *cnt = vb->word & 1;
        *type = (vb->word >> 1) & 7;
        *frac = (vb->word >> 4) & 0x1F;
        return;
    case 15:
        *cnt = (vb->word >> 9) & 1;
        *type = (vb->word >> 10) & 7;
        *frac = (vb->word >> 13) & 0x1F;
        return;
    case 16:
        *cnt = (vb->word >> 18) & 1;
        *type = (vb->word >> 19) & 7;
        *frac = (vb->word >> 22) & 0x1F;
        return;
    case 17:
        *cnt = (vb->word >> 27) & 1;
        *type = (vb->word >> 28) & 7;
        *frac = vc->word & 0x1F;
        return;
    case 18:
        *cnt = (vc->word >> 5) & 1;
        *type = (vc->word >> 6) & 7;
        *frac = (vc->word >> 9) & 0x1F;
        return;
    case 19:
        *cnt = (vc->word >> 14) & 1;
        *type = (vc->word >> 15) & 7;
        *frac = (vc->word >> 18) & 0x1F;
        return;
    case 20:
        *cnt = (vc->word >> 23) & 1;
        *type = (vc->word >> 24) & 7;
        *frac = vc->word >> 27;
        return;
    default:
        *cnt = 1;
        *type = 0;
        *frac = 0;
        return;
    }
}

/* 0x804B5AE0 (0x40): Points vertex attribute `attr` at an array of `stride`-byte entries. */
void GXSetArray(s32 attr, const void* pBase, u8 stride) /* untyped: byte range */
{
    s32 cpAttr;

    if (attr == 25) {
        attr = 10;
    }
    cpAttr = attr - 9;
    GX_WRITE_CP_REG(cpAttr | 0xA0, (u32)pBase & 0x3FFFFFFF);
    GX_WRITE_CP_REG(cpAttr | 0xB0, stride);
}

/* 0x804B5B20 (0x10): Tells the hardware the vertex cache's contents are stale (a write-gather-pipe command). */
void GXInvalidateVtxCache(void)
{
    GXWGFifo.u8 = 0x48;
}

/* 0x804B5B30 (0x244): Sets how texture coordinate `dst_coord` is generated and which matrices transform it. */
void GXSetTexCoordGen2(s32 dst_coord, s32 func, s32 src_param, u32 mtx, u8 normalize, u32 pt_texmtx)
{
    u32 reg;
    u32 row;
    u32 form;

    reg = 0;
    form = 0;
    row = 5;
    switch (src_param) {
    case 0: row = 0; form = 1; break;
    case 1: row = 1; form = 1; break;
    case 2: row = 3; form = 1; break;
    case 3: row = 4; form = 1; break;
    case 19: row = 2; break;
    case 20: row = 2; break;
    case 4: row = 5; break;
    case 5: row = 6; break;
    case 6: row = 7; break;
    case 7: row = 8; break;
    case 8: row = 9; break;
    case 9: row = 10; break;
    case 10: row = 11; break;
    case 11: row = 12; break;
    default: break;
    }

    switch (func) {
    case 1:
        GX_SET_REG(reg, 0, 30, 30);
        GX_SET_REG(reg, form, 29, 29);
        GX_SET_REG(reg, 0, 25, 27);
        GX_SET_REG(reg, row, 20, 24);
        break;
    case 0:
        GX_SET_REG(reg, 1, 30, 30);
        GX_SET_REG(reg, form, 29, 29);
        GX_SET_REG(reg, 0, 25, 27);
        GX_SET_REG(reg, row, 20, 24);
        break;
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
        GX_SET_REG(reg, 0, 30, 30);
        GX_SET_REG(reg, form, 29, 29);
        GX_SET_REG(reg, 1, 25, 27);
        GX_SET_REG(reg, row, 20, 24);
        GX_SET_REG(reg, src_param - 12, 17, 19);
        GX_SET_REG(reg, func - 2, 14, 16);
        break;
    case 10:
        GX_SET_REG(reg, 0, 30, 30);
        GX_SET_REG(reg, form, 29, 29);
        if (src_param == 19) {
            GX_SET_REG(reg, 2, 25, 27);
        } else {
            GX_SET_REG(reg, 3, 25, 27);
        }
        GX_SET_REG(reg, 2, 20, 24);
        break;
    default:
        break;
    }

    __GXData->texGenCtrl[dst_coord].word = reg;
    __GXData->dirtyState |= 0x10000 << dst_coord;
    reg = 0;
    GX_SET_REG(reg, pt_texmtx - 64, 26, 31);
    GX_SET_REG(reg, normalize, 23, 23);
    __GXData->postTexGen[dst_coord].word = reg;

    switch (dst_coord) {
    case 0: GX_SET_REG(__GXData->matrixIndexLo.word, mtx, 20, 25); break;
    case 1: GX_SET_REG(__GXData->matrixIndexLo.word, mtx, 14, 19); break;
    case 2: GX_SET_REG(__GXData->matrixIndexLo.word, mtx, 8, 13); break;
    case 3: GX_SET_REG(__GXData->matrixIndexLo.word, mtx, 2, 7); break;
    case 4: GX_SET_REG(__GXData->matrixIndexHi.word, mtx, 26, 31); break;
    case 5: GX_SET_REG(__GXData->matrixIndexHi.word, mtx, 20, 25); break;
    case 6: GX_SET_REG(__GXData->matrixIndexHi.word, mtx, 14, 19); break;
    default: GX_SET_REG(__GXData->matrixIndexHi.word, mtx, 8, 13); break;
    }
    __GXData->dirtyState |= 0x04000000;
}

/* 0x804B5D80 (0x24): Sets the number of texture coordinates the XF generates. */
void GXSetNumTexGens(u8 nTexGens)
{
    GXData* gx = __GXData;

    GX_SET_REG(gx->genMode.word, nTexGens, 28, 31);
    gx->dirtyState |= 0x02000004;
}
