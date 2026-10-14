/*
 * RVLGX/GXTexture.c - the SDK GX texture objects and texture cache (init/load objects, regions, TMEM setup).
 *
 * RANGE. `.text` 0x804B7F60-0x804B8F00; `.data` 0x8061AAF0-0x8061AC20; `.sdata` 0x80793E60-0x80793EA8; `.sdata2`
 *   0x8079D160-0x8079D198 (29 functions / 0xEC0 B).
 *   - `.data` holds the jump tables of `__GetImageTileCount` and `GXInitTexObj`; `.sdata` the seven BP id tables
 *     and the two filter conversion tables; `.sdata2` the float pool in first-use order; all read only here
 * FLAGS. the `OS` lib block of `configure.py` (`Wii/1.3`) with `cflags_base` (-O4,p).
 * NAMES. the SDK's names; `GXTexObjInt`, `GXTlutObjInt`, `GXTexRegionInt` and `GXTlutRegionInt` are the SDK's
 *   internal views of the opaque `gx.h` records and stay file-local (no other unit reads their fields);
 *   `GX2HWFiltConv`/`HW2GXFiltConv` are file-scope statics (map scope local).
 * RESIDUALS. none in `.text`, `.data`, `.sdata` or `.sdata2`.
 * SHAPES. `__SetSURegs` is kept out of line by `#pragma dont_inline on` around it (the target calls it twice from
 *   `__GXSetSUTexRegs`); its width/height locals are `u16` and the biases come from a `? 1 : 0` ternary (a plain
 *   comparison or `u32` locals colour the registers differently). `__GXSetSUTexRegs` reads `tref` through a
 *   `ptref` computed before the odd/even test and re-reads `__GXData` for the TEV-stage loop. `GXInitTexObjCI`
 *   clears the colour-index flag with `flags = flags & ~2` (a compound `&=` truncates through `andi.`).
 *   `__GetImageTileCount` has no case 0x3D (the jump table ends at 0x3C). `GXGetTexObjLODAll` sign-extends the
 *   LOD bias as `(s8)(s16)`; `GXLoadTexObjPreLoaded` copies `tImage0`/`tMode0` as whole words.
 */

#include "RVLGX/GXTexture.h"
#include "RVLGX/GXInit.h"
#include "RVLGX/GXBump.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "gx.h"

/* The SDK's internal view of the opaque `GXTexObj`. size: 0x20 */
typedef struct GXTexObjInt {
    /* +0x00 */ GXTexMode0Reg mode0;
    /* +0x04 */ u32 mode1; /* TX_SETMODE1: min LOD in bits 7..0, max LOD in bits 15..8 */
    /* +0x08 */ GXTexImage0Reg image0;
    /* +0x0C */ u32 image3; /* TX_SETIMAGE3: the image address >> 5 in bits 23..0 */
    /* +0x10 */ void* userData;
    /* +0x14 */ u32 fmt;
    /* +0x18 */ u32 tlutName;
    /* +0x1C */ u16 loadCnt;
    /* +0x1E */ u8 loadFmt;
    /* +0x1F */ u8 flags; /* bit 0: mipmapped; bit 1: not a colour-index format */
} GXTexObjInt;

/* The SDK's internal view of the opaque `GXTlutObj`. size: 0xC */
typedef struct GXTlutObjInt {
    /* +0x0 */ u32 tlut;      /* TX_SETTLUT: format and TMEM offset */
    /* +0x4 */ u32 loadTlut0; /* BP 0x64: the table address >> 5 */
    /* +0x8 */ u16 numEntries;
    /* +0xA */ u8 pad_0xA[2];
} GXTlutObjInt;

/* The SDK's internal view of the opaque `GXTexRegion`. size: 0x10 */
typedef struct GXTexRegionInt {
    /* +0x0 */ u32 image1; /* TX_SETIMAGE1: even-LOD TMEM address and cache size */
    /* +0x4 */ u32 image2; /* TX_SETIMAGE2: odd-LOD TMEM address and cache size */
    /* +0x8 */ u8 pad_0x8[4];
    /* +0xC */ u8 is32bMipmap;
    /* +0xD */ u8 isCached;
    /* +0xE */ u8 pad_0xE[2];
} GXTexRegionInt;

/* The SDK's internal view of the opaque `GXTlutRegion`. size: 0x10 */
typedef struct GXTlutRegionInt {
    /* +0x0 */ u32 loadTlut1; /* BP 0x65: TMEM offset and entry count */
    /* +0x4 */ GXTlutObjInt tlutObj;
} GXTlutRegionInt;

u8 GXTexMode0Ids[8] = { 0x80, 0x81, 0x82, 0x83, 0xA0, 0xA1, 0xA2, 0xA3 };
u8 GXTexMode1Ids[8] = { 0x84, 0x85, 0x86, 0x87, 0xA4, 0xA5, 0xA6, 0xA7 };
u8 GXTexImage0Ids[8] = { 0x88, 0x89, 0x8A, 0x8B, 0xA8, 0xA9, 0xAA, 0xAB };
u8 GXTexImage1Ids[8] = { 0x8C, 0x8D, 0x8E, 0x8F, 0xAC, 0xAD, 0xAE, 0xAF };
u8 GXTexImage2Ids[8] = { 0x90, 0x91, 0x92, 0x93, 0xB0, 0xB1, 0xB2, 0xB3 };
u8 GXTexImage3Ids[8] = { 0x94, 0x95, 0x96, 0x97, 0xB4, 0xB5, 0xB6, 0xB7 };
u8 GXTexTlutIds[8] = { 0x98, 0x99, 0x9A, 0x9B, 0xB8, 0xB9, 0xBA, 0xBB };
static u8 GX2HWFiltConv[6] = { 0, 4, 1, 5, 2, 6 };
static u8 HW2GXFiltConv[8] = { 0, 2, 4, 0, 1, 3, 5, 0 };

/* 0x804B7F60 (0xC0): computes the row, column and total tile counts of a `wd` x `ht` image in format `fmt`. */
void __GetImageTileCount(s32 fmt, u16 wd, u16 ht, u32* rowTiles, u32* colTiles, u32* cmpTiles)
{
    u32 texRowShift;
    u32 texColShift;

    switch (fmt) {
    case 0:
    case 8:
    case 0x30:
    case 0x31:
    case 0x38:
    case 0x39:
    case 0x3A:
        texRowShift = 3;
        texColShift = 3;
        break;
    case 1:
    case 2:
    case 9:
    case 0x11:
    case 0x22:
    case 0x27:
    case 0x28:
    case 0x29:
    case 0x2A:
    case 0x3C:
        texRowShift = 3;
        texColShift = 2;
        break;
    case 3:
    case 4:
    case 5:
    case 6:
    case 10:
    case 0x13:
    case 0x14:
    case 0x16:
    case 0x20:
    case 0x21:
    case 0x2B:
    case 0x2C:
    case 0x3B:
        texRowShift = 2;
        texColShift = 2;
        break;
    default:
        texColShift = 0;
        texRowShift = 0;
        break;
    }
    if (wd == 0) {
        wd = 1;
    }
    if (ht == 0) {
        ht = 1;
    }
    *rowTiles = (wd + (1 << texRowShift) - 1) >> texRowShift;
    *colTiles = (ht + (1 << texColShift) - 1) >> texColShift;
    *cmpTiles = (fmt == 6 || fmt == 0x16) ? 2 : 1;
}

/* 0x804B8020 (0x20C): initialises a texture object for an image of the given size, format and wrap modes. */
/* untyped: byte range - the texel image the texture object points the GPU at */
void GXInitTexObj(GXTexObj* obj, void* image, u16 width, u16 height, u32 format, u32 wrap_s, u32 wrap_t, u8 mipmap)
{
    GXTexObjInt* t = (GXTexObjInt*)obj;
    u32 maxLOD;
    u8 lmax;
    u32 rowT;
    u32 colT;

    memset(t, 0, 0x20);
    GX_SET_REG(t->mode0.word, wrap_s, 30, 31);
    GX_SET_REG(t->mode0.word, wrap_t, 28, 29);
    GX_SET_REG(t->mode0.word, 1, 27, 27);
    if (mipmap != 0) {
        t->flags |= 1;
        if (format == 8 || format == 9 || format == 10) {
            GX_SET_REG(t->mode0.word, 5, 24, 26);
        } else {
            GX_SET_REG(t->mode0.word, 6, 24, 26);
        }
        if (width > height) {
            maxLOD = 31 - __cntlzw(width);
        } else {
            maxLOD = 31 - __cntlzw(height);
        }
        lmax = 16.0f * maxLOD;
        GX_SET_REG(t->mode1, lmax, 16, 23);
    } else {
        GX_SET_REG(t->mode0.word, 4, 24, 26);
    }
    t->fmt = format;
    GX_SET_REG(t->image0.word, width - 1, 22, 31);
    GX_SET_REG(t->image0.word, height - 1, 12, 21);
    GX_SET_REG(t->image0.word, format & 0xF, 8, 11);
    GX_SET_REG(t->image3, (u32)image >> 5, 8, 31);
    switch (format & 0xF) {
    case 0:
    case 8:
        t->loadFmt = 1;
        rowT = 3;
        colT = 3;
        break;
    case 1:
    case 2:
    case 9:
        t->loadFmt = 2;
        rowT = 3;
        colT = 2;
        break;
    case 3:
    case 4:
    case 5:
    case 10:
        t->loadFmt = 2;
        rowT = 2;
        colT = 2;
        break;
    case 6:
        t->loadFmt = 3;
        rowT = 2;
        colT = 2;
        break;
    case 14:
        t->loadFmt = 0;
        rowT = 3;
        colT = 3;
        break;
    default:
        t->loadFmt = 2;
        rowT = 2;
        colT = 2;
        break;
    }
    t->loadCnt = (((width + (1 << rowT) - 1) >> rowT) * ((height + (1 << colT) - 1) >> colT)) & 0x7FFF;
    t->flags |= 2;
}

/* 0x804B8230 (0x48): initialises a colour-index texture object bound to TLUT `tlut_name`. */
/* untyped: byte range - the colour-index image the texture object points the GPU at */
void GXInitTexObjCI(GXTexObj* obj, void* image, u16 width, u16 height, u32 format, u32 wrap_s, u32 wrap_t,
                    u8 mipmap, u32 tlut_name)
{
    GXTexObjInt* t = (GXTexObjInt*)obj;

    GXInitTexObj(obj, image, width, height, format, wrap_s, wrap_t, mipmap);
    t->flags = t->flags & ~2;
    t->tlutName = tlut_name;
}

/* 0x804B8280 (0x104): sets a texture object's filters, LOD range and bias, edge LOD and anisotropy. */
void GXInitTexObjLOD(GXTexObj* obj, u32 min_filt, u32 mag_filt, f32 min_lod, f32 max_lod, f32 lod_bias,
                     u8 bias_clamp, u8 do_edge_lod, u32 max_aniso)
{
    GXTexObjInt* t = (GXTexObjInt*)obj;
    u8 lbias;
    u8 lmin;
    u8 lmax;

    if (lod_bias < -4.0f) {
        lod_bias = -4.0f;
    } else if (lod_bias >= 4.0f) {
        lod_bias = 3.99f;
    }
    lbias = (u8)(32.0f * lod_bias);
    t->mode0.f.lodBias = lbias;
    t->mode0.f.magFilter = (mag_filt == 1);
    t->mode0.f.minFilter = GX2HWFiltConv[min_filt];
    t->mode0.f.diagLod = (do_edge_lod == 0);
    t->mode0.f.lodReserved = 0;
    t->mode0.f.maxAniso = max_aniso;
    t->mode0.f.biasClamp = bias_clamp;
    if (min_lod < 0.0f) {
        min_lod = 0.0f;
    } else if (min_lod > 10.0f) {
        min_lod = 10.0f;
    }
    lmin = (u8)(16.0f * min_lod);
    if (max_lod < 0.0f) {
        max_lod = 0.0f;
    } else if (max_lod > 10.0f) {
        max_lod = 10.0f;
    }
    lmax = (u8)(16.0f * max_lod);
    GX_SET_REG(t->mode1, lmin, 24, 31);
    GX_SET_REG(t->mode1, lmax, 16, 23);
}

/* 0x804B8390 (0x14): sets a texture object's s and t wrap modes. */
void GXInitTexObjWrapMode(GXTexObj* obj, u32 wrap_s, u32 wrap_t)
{
    GXTexObjInt* t = (GXTexObjInt*)obj;

    t->mode0.f.wrapS = wrap_s;
    t->mode0.f.wrapT = wrap_t;
}

/* 0x804B83B0 (0x8): binds a texture object to TLUT `tlut_name`. */
void GXInitTexObjTlut(GXTexObj* obj, u32 tlut_name)
{
    ((GXTexObjInt*)obj)->tlutName = tlut_name;
}

/* untyped: caller-owned payload - the user pointer a texture object carries */
void GXInitTexObjUserData(GXTexObj* obj, void* user_data)
{
    ((GXTexObjInt*)obj)->userData = user_data;
}

/* untyped: caller-owned payload - the user pointer a texture object carries */
void* GXGetTexObjUserData(const GXTexObj* obj)
{
    return ((const GXTexObjInt*)obj)->userData;
}

/* untyped: byte range - the texel image address the texture object holds */
void GXGetTexObjAll(const GXTexObj* obj, void** image_ptr, u16* width, u16* height, u32* format, u32* wrap_s,
                    u32* wrap_t, u8* mipmap)
{
    const GXTexObjInt* t = (const GXTexObjInt*)obj;

    *image_ptr = (void*)((t->image3 & 0x00FFFFFF) << 5);
    *width = t->image0.f.widthMinus1 + 1;
    *height = t->image0.f.heightMinus1 + 1;
    *format = t->fmt;
    *wrap_s = t->mode0.f.wrapS;
    *wrap_t = t->mode0.f.wrapT;
    *mipmap = t->flags & 1;
}

/* 0x804B8440 (0x14): returns the texture object's width. */
u16 GXGetTexObjWidth(const GXTexObj* obj)
{
    return ((const GXTexObjInt*)obj)->image0.f.widthMinus1 + 1;
}

/* 0x804B8460 (0x14): returns the texture object's height. */
u16 GXGetTexObjHeight(const GXTexObj* obj)
{
    return ((const GXTexObjInt*)obj)->image0.f.heightMinus1 + 1;
}

/* 0x804B8480 (0x8): returns the texture object's format. */
s32 GXGetTexObjFmt(const GXTexObj* obj)
{
    return ((const GXTexObjInt*)obj)->fmt;
}

/* 0x804B8490 (0xC): returns the texture object's s wrap mode. */
u32 GXGetTexObjWrapS(const GXTexObj* obj)
{
    return ((const GXTexObjInt*)obj)->mode0.f.wrapS;
}

/* 0x804B84A0 (0xC): returns the texture object's t wrap mode. */
u32 GXGetTexObjWrapT(const GXTexObj* obj)
{
    return ((const GXTexObjInt*)obj)->mode0.f.wrapT;
}

/* 0x804B84B0 (0xC): returns whether the texture object is mipmapped. */
u8 GXGetTexObjMipMap(const GXTexObj* obj)
{
    return ((const GXTexObjInt*)obj)->flags & 1;
}

/* 0x804B84C0 (0xDC): reads back a texture object's filters, LOD range and bias, edge LOD and anisotropy. */
void GXGetTexObjLODAll(const GXTexObj* obj, u32* min_filt, u32* mag_filt, f32* min_lod, f32* max_lod,
                       f32* lod_bias, u8* bias_clamp, u8* do_edge_lod, u32* max_aniso)
{
    const GXTexObjInt* t = (const GXTexObjInt*)obj;

    *min_filt = HW2GXFiltConv[t->mode0.f.minFilter];
    *mag_filt = t->mode0.f.magFilter;
    *min_lod = (u8)t->mode1 / 16.0f;
    *max_lod = (u8)(t->mode1 >> 8) / 16.0f;
    *lod_bias = (s8)(s16)t->mode0.f.lodBias / 32.0f;
    *bias_clamp = t->mode0.f.biasClamp;
    *do_edge_lod = !t->mode0.f.diagLod;
    *max_aniso = t->mode0.f.maxAniso;
}

/* 0x804B85A0 (0x8): returns the TLUT the texture object is bound to. */
u32 GXGetTexObjTlut(const GXTexObj* obj)
{
    return ((const GXTexObjInt*)obj)->tlutName;
}

/* 0x804B85B0 (0x164): loads a texture object into texture slot `id` using an already-loaded cache region. */
void GXLoadTexObjPreLoaded(const GXTexObj* obj, const GXTexRegion* region, u32 id)
{
    GXTexObjInt* t = (GXTexObjInt*)obj;
    GXTexRegionInt* r = (GXTexRegionInt*)region;
    GXTlutRegionInt* tlr;

    GX_SET_REG(t->mode0.word, GXTexMode0Ids[id], 0, 7);
    GX_SET_REG(t->mode1, GXTexMode1Ids[id], 0, 7);
    GX_SET_REG(t->image0.word, GXTexImage0Ids[id], 0, 7);
    GX_SET_REG(r->image1, GXTexImage1Ids[id], 0, 7);
    GX_SET_REG(r->image2, GXTexImage2Ids[id], 0, 7);
    GX_SET_REG(t->image3, GXTexImage3Ids[id], 0, 7);
    GXWGFifo.u8 = 0x61;
    GXWGFifo.u32 = t->mode0.word;
    GXWGFifo.u8 = 0x61;
    GXWGFifo.u32 = t->mode1;
    GXWGFifo.u8 = 0x61;
    GXWGFifo.u32 = t->image0.word;
    GXWGFifo.u8 = 0x61;
    GXWGFifo.u32 = r->image1;
    GXWGFifo.u8 = 0x61;
    GXWGFifo.u32 = r->image2;
    GXWGFifo.u8 = 0x61;
    GXWGFifo.u32 = t->image3;
    if (!(t->flags & 2)) {
        tlr = (GXTlutRegionInt*)__GXData->tlutRegionCallback(t->tlutName);
        GX_SET_REG(tlr->tlutObj.tlut, GXTexTlutIds[id], 0, 7);
        GXWGFifo.u8 = 0x61;
        GXWGFifo.u32 = tlr->tlutObj.tlut;
    }
    __GXData->tImage0[id].word = t->image0.word;
    __GXData->tMode0[id].word = t->mode0.word;
    __GXData->dirtyState |= 1;
    __GXData->xfWritePending = 0;
}

/* 0x804B8720 (0x54): loads a texture object into texture slot `id` through the texture region callback. */
void GXLoadTexObj(const GXTexObj* obj, u32 id)
{
    GXTexRegion* r = __GXData->texRegionCallback(obj, id);

    GXLoadTexObjPreLoaded(obj, r, id);
}

/* untyped: byte range - the lookup table's entries */
void GXInitTlutObj(GXTlutObj* obj, void* lut, u32 format, u16 entries)
{
    GXTlutObjInt* t = (GXTlutObjInt*)obj;

    t->tlut = 0;
    GX_SET_REG(t->tlut, format, 20, 21);
    GX_SET_REG(t->loadTlut0, (u32)lut >> 5, 8, 31);
    GX_SET_REG(t->loadTlut0, 0x64, 0, 7);
    t->numEntries = entries;
}

/* 0x804B87B0 (0x94): loads a TLUT into the TMEM region the TLUT region callback returns for `tlut_name`. */
void GXLoadTlut(const GXTlutObj* obj, u32 tlut_name)
{
    GXTlutObjInt* t = (GXTlutObjInt*)obj;
    GXTlutRegionInt* r = (GXTlutRegionInt*)__GXData->tlutRegionCallback(tlut_name);

    __GXFlushTextureState();
    GXWGFifo.u8 = 0x61;
    GXWGFifo.u32 = t->loadTlut0;
    GXWGFifo.u8 = 0x61;
    GXWGFifo.u32 = r->loadTlut1;
    __GXFlushTextureState();
    GX_SET_REG(t->tlut, r->loadTlut1, 22, 31);
    r->tlutObj = *t;
}

/* 0x804B8850 (0xA8): initialises a texture cache region from its even and odd TMEM addresses and sizes. */
void GXInitTexCacheRegion(GXTexRegion* region, u8 is_32b_mipmap, u32 tmem_even, s32 size_even, u32 tmem_odd,
                          s32 size_odd)
{
    GXTexRegionInt* t = (GXTexRegionInt*)region;
    u32 WidthExp2;

    switch (size_even) {
    case 0:
        WidthExp2 = 3;
        break;
    case 1:
        WidthExp2 = 4;
        break;
    case 2:
        WidthExp2 = 5;
        break;
    }
    t->image1 = 0;
    GX_SET_REG(t->image1, tmem_even >> 5, 17, 31);
    GX_SET_REG(t->image1, WidthExp2, 14, 16);
    GX_SET_REG(t->image1, WidthExp2, 11, 13);
    switch (size_odd) {
    case 0:
        WidthExp2 = 3;
        break;
    case 1:
        WidthExp2 = 4;
        break;
    case 2:
        WidthExp2 = 5;
        break;
    case 3:
        WidthExp2 = 0;
        break;
    }
    t->image2 = 0;
    GX_SET_REG(t->image2, tmem_odd >> 5, 17, 31);
    GX_SET_REG(t->image2, WidthExp2, 14, 16);
    GX_SET_REG(t->image2, WidthExp2, 11, 13);
    t->is32bMipmap = is_32b_mipmap;
    t->isCached = 1;
}

/* 0x804B8900 (0x20): initialises a TLUT region from its TMEM address and size. */
void GXInitTlutRegion(GXTlutRegion* region, u32 tmem_addr, u32 tlut_size)
{
    GXTlutRegionInt* t = (GXTlutRegionInt*)region;

    t->loadTlut1 = 0;
    GX_SET_REG(t->loadTlut1, (tmem_addr - 0x80000) >> 9, 22, 31);
    GX_SET_REG(t->loadTlut1, tlut_size, 11, 21);
    GX_SET_REG(t->loadTlut1, 0x65, 0, 7);
}

/* 0x804B8920 (0x48): invalidates every texture cache line. */
void GXInvalidateTexAll(void)
{
    __GXFlushTextureState();
    GXWGFifo.u8 = 0x61;
    GXWGFifo.u32 = 0x66001000;
    GXWGFifo.u8 = 0x61;
    GXWGFifo.u32 = 0x66001100;
    __GXFlushTextureState();
}

/* 0x804B8970 (0x14): installs the texture region callback and returns the previous one. */
GXTexRegionCallback GXSetTexRegionCallback(GXTexRegionCallback f)
{
    GXTexRegionCallback old = __GXData->texRegionCallback;

    __GXData->texRegionCallback = f;
    return old;
}

/* 0x804B8990 (0x14): installs the TLUT region callback and returns the previous one. */
GXTlutRegionCallback GXSetTlutRegionCallback(GXTlutRegionCallback f)
{
    GXTlutRegionCallback old = __GXData->tlutRegionCallback;

    __GXData->tlutRegionCallback = f;
    return old;
}

#pragma dont_inline on
/* 0x804B89B0 (0x90): sends the setup-unit scale registers of coordinate `tcoord` from texture map `tmap`'s size. */
static void __SetSURegs(u32 tmap, u32 tcoord)
{
    u16 w;
    u16 h;
    u8 s_bias;
    u8 t_bias;
    GXData* gx = __GXData;

    w = gx->tImage0[tmap].f.widthMinus1;
    h = gx->tImage0[tmap].f.heightMinus1;
    gx->suTs0[tcoord].f.scaleMinus1 = w;
    gx->suTs1[tcoord].f.scaleMinus1 = h;
    s_bias = (gx->tMode0[tmap].f.wrapS == 1) ? 1 : 0;
    t_bias = (gx->tMode0[tmap].f.wrapT == 1) ? 1 : 0;
    gx->suTs0[tcoord].f.rangeBias = s_bias;
    gx->suTs1[tcoord].f.rangeBias = t_bias;
    GXWGFifo.u8 = 0x61;
    GXWGFifo.u32 = gx->suTs0[tcoord].word;
    GXWGFifo.u8 = 0x61;
    GXWGFifo.u32 = gx->suTs1[tcoord].word;
    gx->xfWritePending = 0;
}

#pragma dont_inline reset
/* 0x804B8A40 (0x164): sends the setup-unit texture-coordinate scale registers of the active TEV stages. */
void __GXSetSUTexRegs(void)
{
    u32 nStages;
    u32 nIndStages;
    u32 i;
    u32 map;
    u32 tmap;
    u32 coord;
    GXTevOrderReg* ptref;
    GXData* gx = __GXData;

    if (gx->tcsManEnab != 0xFF) {
        nStages = gx->genMode.f.numTevStagesMinus1 + 1;
        nIndStages = gx->genMode.f.numIndStages;
        for (i = 0; i < nIndStages; i++) {
            switch (i) {
            case 0:
                tmap = gx->iref.f.map0;
                coord = gx->iref.f.coord0;
                break;
            case 1:
                tmap = gx->iref.f.map1;
                coord = gx->iref.f.coord1;
                break;
            case 2:
                tmap = gx->iref.f.map2;
                coord = gx->iref.f.coord2;
                break;
            case 3:
                tmap = gx->iref.f.map3;
                coord = gx->iref.f.coord3;
                break;
            }
            if (!(gx->tcsManEnab & (1 << coord))) {
                __SetSURegs(tmap, coord);
            }
        }
        for (i = 0; i < nStages; i++) {
            ptref = &__GXData->tref[i / 2];
            map = __GXData->texmapId[i];
            tmap = map & ~0x100;
            if (i & 1) {
                coord = ptref->f.coord1;
            } else {
                coord = ptref->f.coord0;
            }
            if (tmap != 0xFF && !(gx->tcsManEnab & (1 << coord)) && (__GXData->tevTcEnab & (1 << i))) {
                __SetSURegs(tmap, coord);
            }
        }
    }
}

/* 0x804B8BB0 (0x350): programs the TMEM layout of the texture cache and TLUT regions for configuration `config`. */
void __GXSetTmemConfig(u32 config)
{
    switch (config) {
    case 2:
        GXWGFifo.u8 = 0x61;
        GXWGFifo.u32 = 0x8C0D8000;
        GXWGFifo.u8 = 0x61;
        GXWGFifo.u32 = 0x900DC000;
        GXWGFifo.u8 = 0x61;
        GXWGFifo.u32 = 0x8D0D8800;
        GXWGFifo.u8 = 0x61;
        GXWGFifo.u32 = 0x910DC800;
        GXWGFifo.u8 = 0x61;
        GXWGFifo.u32 = 0x8E0D9000;
        GXWGFifo.u8 = 0x61;
        GXWGFifo.u32 = 0x920DD000;
        GXWGFifo.u8 = 0x61;
        GXWGFifo.u32 = 0x8F0D9800;
        GXWGFifo.u8 = 0x61;
        GXWGFifo.u32 = 0x930DD800;
        GXWGFifo.u8 = 0x61;
        GXWGFifo.u32 = 0xAC0DA000;
        GXWGFifo.u8 = 0x61;
        GXWGFifo.u32 = 0xB00DC400;
        GXWGFifo.u8 = 0x61;
        GXWGFifo.u32 = 0xAD0DA800;
        GXWGFifo.u8 = 0x61;
        GXWGFifo.u32 = 0xB10DCC00;
        GXWGFifo.u8 = 0x61;
        GXWGFifo.u32 = 0xAE0DB000;
        GXWGFifo.u8 = 0x61;
        GXWGFifo.u32 = 0xB20DD400;
        GXWGFifo.u8 = 0x61;
        GXWGFifo.u32 = 0xAF0DB800;
        GXWGFifo.u8 = 0x61;
        GXWGFifo.u32 = 0xB30DDC00;
        break;
    case 1:
        GXWGFifo.u8 = 0x61;
        GXWGFifo.u32 = 0x8C0D8000;
        GXWGFifo.u8 = 0x61;
        GXWGFifo.u32 = 0x900DC000;
        GXWGFifo.u8 = 0x61;
        GXWGFifo.u32 = 0x8D0D8800;
        GXWGFifo.u8 = 0x61;
        GXWGFifo.u32 = 0x910DC800;
        GXWGFifo.u8 = 0x61;
        GXWGFifo.u32 = 0x8E0D9000;
        GXWGFifo.u8 = 0x61;
        GXWGFifo.u32 = 0x920DD000;
        GXWGFifo.u8 = 0x61;
        GXWGFifo.u32 = 0x8F0D9800;
        GXWGFifo.u8 = 0x61;
        GXWGFifo.u32 = 0x930DD800;
        GXWGFifo.u8 = 0x61;
        GXWGFifo.u32 = 0xAC0DA000;
        GXWGFifo.u8 = 0x61;
        GXWGFifo.u32 = 0xB00DE000;
        GXWGFifo.u8 = 0x61;
        GXWGFifo.u32 = 0xAD0DA800;
        GXWGFifo.u8 = 0x61;
        GXWGFifo.u32 = 0xB10DE800;
        GXWGFifo.u8 = 0x61;
        GXWGFifo.u32 = 0xAE0DB000;
        GXWGFifo.u8 = 0x61;
        GXWGFifo.u32 = 0xB20DF000;
        GXWGFifo.u8 = 0x61;
        GXWGFifo.u32 = 0xAF0DB800;
        GXWGFifo.u8 = 0x61;
        GXWGFifo.u32 = 0xB30DF800;
        break;
    default:
        GXWGFifo.u8 = 0x61;
        GXWGFifo.u32 = 0x8C0D8000;
        GXWGFifo.u8 = 0x61;
        GXWGFifo.u32 = 0x900DC000;
        GXWGFifo.u8 = 0x61;
        GXWGFifo.u32 = 0x8D0D8400;
        GXWGFifo.u8 = 0x61;
        GXWGFifo.u32 = 0x910DC400;
        GXWGFifo.u8 = 0x61;
        GXWGFifo.u32 = 0x8E0D8800;
        GXWGFifo.u8 = 0x61;
        GXWGFifo.u32 = 0x920DC800;
        GXWGFifo.u8 = 0x61;
        GXWGFifo.u32 = 0x8F0D8C00;
        GXWGFifo.u8 = 0x61;
        GXWGFifo.u32 = 0x930DCC00;
        GXWGFifo.u8 = 0x61;
        GXWGFifo.u32 = 0xAC0D9000;
        GXWGFifo.u8 = 0x61;
        GXWGFifo.u32 = 0xB00DD000;
        GXWGFifo.u8 = 0x61;
        GXWGFifo.u32 = 0xAD0D9400;
        GXWGFifo.u8 = 0x61;
        GXWGFifo.u32 = 0xB10DD400;
        GXWGFifo.u8 = 0x61;
        GXWGFifo.u32 = 0xAE0D9800;
        GXWGFifo.u8 = 0x61;
        GXWGFifo.u32 = 0xB20DD800;
        GXWGFifo.u8 = 0x61;
        GXWGFifo.u32 = 0xAF0D9C00;
        GXWGFifo.u8 = 0x61;
        GXWGFifo.u32 = 0xB30DDC00;
        break;
    }
}
