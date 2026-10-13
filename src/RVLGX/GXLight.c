/*
 * RVLGX/GXLight.c - the SDK GX lighting (light objects, channel colours and controls).
 *
 * RANGE. `.text` 0x804B77E0-0x804B7F60; `.sdata2` 0x8079D128-0x8079D160 (17 functions / 0x728 B).
 *   - `.sdata2` 0x8079D128..0x8079D160 (the spot, distance-attenuation and specular constants, first-use order) is read
 *     only by this run, which repeats `1.0f` at 0x8079D140 (a second pool entry for the value `GXInit` keeps at
 *     0x8079D104); the last 4 bytes are alignment padding before `GXTexture`'s pool
 * FLAGS. the `OS` lib block of `configure.py` (`Wii/1.3`) with `cflags_base` (-O4,p) plus `-fp_contract off` on this
 *   object: `GXInitSpecularDir` and `GXInitLightSpot` are `fmuls`+`fadds`/`fsubs` with no `fmadds` in the target (with
 *   contraction on they fuse to 0x104/0x198 B against 0x10C/0x19C); no other row changes with the flag.
 * NAMES. the SDK's names; `GXLightObjInt` (the SDK's internal view of the opaque `GXLightObj`) and the static
 *   `PushLight` (inlined, no symbol) are the SDK's; the channel/spot/attenuation enumerators are the SDK's values.
 * RESIDUALS. none in `.text` or `.sdata2`; the 0xC-byte `.text` tail is link padding.
 *   - flipcheck refuses `.sdata2` with the poolseams fold note: a tool artefact (the 4 B tail is accepted as fill, then
 *     `pool_group_problems` re-tests the unpadded size); the pool bytes are identical to the target
 * SHAPES. `GXInitLightSpot` computes one reciprocal `d` per case and multiplies (the target has one `fdivs` per case
 *   and no `-1.0f` pool entry), takes `a0` of `GX_SP_RING1` from `a2 * cr`, and declares `cr` first (a later
 *   declaration swaps `f5`/`f6`); `GXSetChanCtrl` clears `diff_fn` with an `if` (a ternary becomes a branchless
 *   `cntlzw` mask); `PushLight` is one `asm volatile` block whose FIFO pointer MWCC folds into the `stw` operands
 *   (`-0x8000(r4)`) while the `psq_st`s take the materialised address; the colour setters re-read `__GXData` in
 *   every case and once more for the shared tail, as the target does.
 */

#include "RVLGX/GXLight.h"
#include "RVLGX/GXInit.h"
#include "MSL/s_cos.h"
#include "MSL/sqrt.h"
#include "gx.h"

/* The SDK's channel, spot, distance-attenuation, diffuse and attenuation function values this file tests. */
enum {
    GX_COLOR0 = 0,
    GX_COLOR1 = 1,
    GX_ALPHA0 = 2,
    GX_ALPHA1 = 3,
    GX_COLOR0A0 = 4,
    GX_COLOR1A1 = 5
};
enum {
    GX_SP_OFF = 0,
    GX_SP_FLAT = 1,
    GX_SP_COS = 2,
    GX_SP_COS2 = 3,
    GX_SP_SHARP = 4,
    GX_SP_RING1 = 5,
    GX_SP_RING2 = 6
};
enum {
    GX_DA_OFF = 0,
    GX_DA_GENTLE = 1,
    GX_DA_MEDIUM = 2,
    GX_DA_STEEP = 3
};
enum {
    GX_DF_NONE = 0
};
enum {
    GX_AF_SPEC = 0,
    GX_AF_SPOT = 1,
    GX_AF_NONE = 2
};

/* The light-object layout the SDK keeps behind the opaque `GXLightObj`. size: 0x40 */
typedef struct GXLightObjInt {
    /* +0x00 */ u32 reserved[3];
    /* +0x0C */ GXColorWord color;
    /* +0x10 */ f32 a[3]; /* angular (spot) attenuation coefficients */
    /* +0x1C */ f32 k[3]; /* distance attenuation coefficients */
    /* +0x28 */ f32 lpos[3];
    /* +0x34 */ f32 ldir[3];
} GXLightObjInt;

/* Sets both the angular and the distance attenuation coefficients. */
void GXInitLightAttn(GXLightObj* lt_obj, f32 a0, f32 a1, f32 a2, f32 k0, f32 k1, f32 k2)
{
    GXLightObjInt* obj = (GXLightObjInt*)lt_obj;

    obj->a[0] = a0;
    obj->a[1] = a1;
    obj->a[2] = a2;
    obj->k[0] = k0;
    obj->k[1] = k1;
    obj->k[2] = k2;
}

/* Sets the angular attenuation coefficients. */
void GXInitLightAttnA(GXLightObj* lt_obj, f32 a0, f32 a1, f32 a2)
{
    GXLightObjInt* obj = (GXLightObjInt*)lt_obj;

    obj->a[0] = a0;
    obj->a[1] = a1;
    obj->a[2] = a2;
}

/* Sets the distance attenuation coefficients. */
void GXInitLightAttnK(GXLightObj* lt_obj, f32 k0, f32 k1, f32 k2)
{
    GXLightObjInt* obj = (GXLightObjInt*)lt_obj;

    obj->k[0] = k0;
    obj->k[1] = k1;
    obj->k[2] = k2;
}

/* Derives the angular attenuation coefficients of a spot light from its cutoff angle and spot function. */
void GXInitLightSpot(GXLightObj* lt_obj, f32 cutoff, s32 spot_func)
{
    f32 cr;
    f32 a0;
    f32 a1;
    f32 a2;
    f32 d;
    GXLightObjInt* obj = (GXLightObjInt*)lt_obj;

    if (cutoff <= 0.0f || cutoff > 90.0f) {
        spot_func = GX_SP_OFF;
    }
    cr = cos((3.1415927f * cutoff) / 180.0f);
    switch (spot_func) {
    case GX_SP_FLAT:
        a0 = -1000.0f * cr;
        a1 = 1000.0f;
        a2 = 0.0f;
        break;
    case GX_SP_COS:
        d = 1.0f / (1.0f - cr);
        a0 = -cr * d;
        a1 = d;
        a2 = 0.0f;
        break;
    case GX_SP_COS2:
        d = 1.0f / (1.0f - cr);
        a0 = 0.0f;
        a1 = -cr * d;
        a2 = d;
        break;
    case GX_SP_SHARP:
        d = 1.0f / ((1.0f - cr) * (1.0f - cr));
        a0 = d * (cr * (cr - 2.0f));
        a1 = 2.0f * d;
        a2 = -d;
        break;
    case GX_SP_RING1:
        d = 1.0f / ((1.0f - cr) * (1.0f - cr));
        a2 = -4.0f * d;
        a0 = a2 * cr;
        a1 = (4.0f * (1.0f + cr)) * d;
        break;
    case GX_SP_RING2:
        d = 1.0f / ((1.0f - cr) * (1.0f - cr));
        a0 = 1.0f - d * (2.0f * cr * cr);
        a1 = (4.0f * cr) * d;
        a2 = -2.0f * d;
        break;
    case GX_SP_OFF:
    default:
        a0 = 1.0f;
        a1 = 0.0f;
        a2 = 0.0f;
        break;
    }
    obj->a[0] = a0;
    obj->a[1] = a1;
    obj->a[2] = a2;
}

/* Derives the distance attenuation coefficients from a reference distance, its brightness and the falloff. */
void GXInitLightDistAttn(GXLightObj* lt_obj, f32 ref_dist, f32 ref_br, s32 dist_func)
{
    f32 k0;
    f32 k1;
    f32 k2;
    GXLightObjInt* obj = (GXLightObjInt*)lt_obj;

    if (ref_dist < 0.0f) {
        dist_func = GX_DA_OFF;
    }
    if (ref_br <= 0.0f || ref_br >= 1.0f) {
        dist_func = GX_DA_OFF;
    }
    switch (dist_func) {
    case GX_DA_GENTLE:
        k0 = 1.0f;
        k1 = (1.0f - ref_br) / (ref_br * ref_dist);
        k2 = 0.0f;
        break;
    case GX_DA_MEDIUM:
        k0 = 1.0f;
        k1 = (0.5f * (1.0f - ref_br)) / (ref_br * ref_dist);
        k2 = (0.5f * (1.0f - ref_br)) / (ref_br * ref_dist * ref_dist);
        break;
    case GX_DA_STEEP:
        k0 = 1.0f;
        k1 = 0.0f;
        k2 = (1.0f - ref_br) / (ref_br * ref_dist * ref_dist);
        break;
    case GX_DA_OFF:
    default:
        k0 = 1.0f;
        k1 = 0.0f;
        k2 = 0.0f;
        break;
    }
    obj->k[0] = k0;
    obj->k[1] = k1;
    obj->k[2] = k2;
}

/* Sets the light position. */
void GXInitLightPos(GXLightObj* lt_obj, f32 x, f32 y, f32 z)
{
    GXLightObjInt* obj = (GXLightObjInt*)lt_obj;

    obj->lpos[0] = x;
    obj->lpos[1] = y;
    obj->lpos[2] = z;
}

/* Reads the light position back. */
void GXGetLightPos(const GXLightObj* lt_obj, f32* x, f32* y, f32* z)
{
    const GXLightObjInt* obj = (const GXLightObjInt*)lt_obj;

    *x = obj->lpos[0];
    *y = obj->lpos[1];
    *z = obj->lpos[2];
}

/* Sets the light direction; the object stores it negated. */
void GXInitLightDir(GXLightObj* lt_obj, f32 nx, f32 ny, f32 nz)
{
    GXLightObjInt* obj = (GXLightObjInt*)lt_obj;

    obj->ldir[0] = -nx;
    obj->ldir[1] = -ny;
    obj->ldir[2] = -nz;
}

/* Reads the light direction back, undoing the stored negation. */
void GXGetLightDir(const GXLightObj* lt_obj, f32* nx, f32* ny, f32* nz)
{
    const GXLightObjInt* obj = (const GXLightObjInt*)lt_obj;

    *nx = -obj->ldir[0];
    *ny = -obj->ldir[1];
    *nz = -obj->ldir[2];
}

/* Sets a specular light: the position far along the normal, the direction the normalised half-angle vector. */
void GXInitSpecularDir(GXLightObj* lt_obj, f32 nx, f32 ny, f32 nz)
{
    f32 mag;
    f32 vx;
    f32 vy;
    f32 vz;
    GXLightObjInt* obj = (GXLightObjInt*)lt_obj;

    vx = -nx;
    vy = -ny;
    vz = -nz + 1.0f;
    mag = vx * vx + vy * vy + vz * vz;
    if (mag != 0.0f) {
        mag = 1.0f / (f32)sqrt(mag);
    }
    obj->ldir[0] = vx * mag;
    obj->ldir[1] = vy * mag;
    obj->ldir[2] = vz * mag;
    obj->lpos[0] = nx * -1.0e18f;
    obj->lpos[1] = ny * -1.0e18f;
    obj->lpos[2] = nz * -1.0e18f;
}

/* Sets the light colour. */
void GXInitLightColor(GXLightObj* lt_obj, GXColor color)
{
    GXLightObjInt* obj = (GXLightObjInt*)lt_obj;

    obj->color.word = *(u32*)&color;
}

/* Reads the light colour back. */
void GXGetLightColor(const GXLightObj* lt_obj, GXColor* color)
{
    const GXLightObjInt* obj = (const GXLightObjInt*)lt_obj;

    *(u32*)color = obj->color.word;
}

/* Writes the colour, attenuation, position and direction words of a light object to the FIFO at `dest`. */
static inline void PushLight(register const GXLightObjInt* lt_obj, register volatile u32* dest)
{
    register u32 zero;
    register u32 color;
    register f32 a0_a1;
    register f32 a2_k0;
    register f32 k1_k2;
    register f32 px_py;
    register f32 pz_dx;
    register f32 dy_dz;

    asm volatile {
        lwz    color, 12(lt_obj)
        xor    zero, zero, zero
        psq_l  a0_a1, 16(lt_obj), 0, 0
        psq_l  a2_k0, 24(lt_obj), 0, 0
        psq_l  k1_k2, 32(lt_obj), 0, 0
        psq_l  px_py, 40(lt_obj), 0, 0
        psq_l  pz_dx, 48(lt_obj), 0, 0
        psq_l  dy_dz, 56(lt_obj), 0, 0
        stw    zero, 0(dest)
        stw    zero, 0(dest)
        stw    zero, 0(dest)
        stw    color, 0(dest)
        psq_st a0_a1, 0(dest), 0, 0
        psq_st a2_k0, 0(dest), 0, 0
        psq_st k1_k2, 0(dest), 0, 0
        psq_st px_py, 0(dest), 0, 0
        psq_st pz_dx, 0(dest), 0, 0
        psq_st dy_dz, 0(dest), 0, 0
    }
}

/* Loads the light object `lt_obj` into the hardware light slot selected by the `light` mask bit. */
void GXLoadLightObjImm(const GXLightObj* lt_obj, u32 light)
{
    u32 addr;
    u32 idx;

    idx = 31 - __cntlzw(light);
    idx &= 7;
    addr = idx * 16 + 0x600;
    GXWGFifo.u8 = 0x10;
    GXWGFifo.u32 = addr | 0x000F0000;
    PushLight((const GXLightObjInt*)lt_obj, &GXWGFifo.u32);
    __GXData->xfWritePending = 1;
}

/* Sets the ambient colour of a channel (colour part, alpha part or both). */
void GXSetChanAmbColor(s32 chan, GXColor amb_color)
{
    u32 reg;
    u32 colIdx;

    switch (chan) {
    case GX_COLOR0:
        reg = __GXData->ambColor[0].word;
        GX_SET_REG(reg, *(u32*)&amb_color >> 8, 0, 23);
        colIdx = 0;
        break;
    case GX_COLOR1:
        reg = __GXData->ambColor[1].word;
        GX_SET_REG(reg, *(u32*)&amb_color >> 8, 0, 23);
        colIdx = 1;
        break;
    case GX_ALPHA0:
        reg = __GXData->ambColor[0].word;
        GX_SET_REG(reg, amb_color.a, 24, 31);
        colIdx = 0;
        break;
    case GX_ALPHA1:
        reg = __GXData->ambColor[1].word;
        GX_SET_REG(reg, amb_color.a, 24, 31);
        colIdx = 1;
        break;
    case GX_COLOR0A0:
        reg = *(u32*)&amb_color;
        colIdx = 0;
        break;
    case GX_COLOR1A1:
        reg = *(u32*)&amb_color;
        colIdx = 1;
        break;
    default:
        return;
    }
    __GXData->dirtyState |= 0x100 << colIdx;
    __GXData->ambColor[colIdx].word = reg;
}

/* Sets the material colour of a channel (colour part, alpha part or both). */
void GXSetChanMatColor(s32 chan, GXColor mat_color)
{
    u32 reg;
    u32 colIdx;

    switch (chan) {
    case GX_COLOR0:
        reg = __GXData->matColor[0].word;
        GX_SET_REG(reg, *(u32*)&mat_color >> 8, 0, 23);
        colIdx = 0;
        break;
    case GX_COLOR1:
        reg = __GXData->matColor[1].word;
        GX_SET_REG(reg, *(u32*)&mat_color >> 8, 0, 23);
        colIdx = 1;
        break;
    case GX_ALPHA0:
        reg = __GXData->matColor[0].word;
        GX_SET_REG(reg, mat_color.a, 24, 31);
        colIdx = 0;
        break;
    case GX_ALPHA1:
        reg = __GXData->matColor[1].word;
        GX_SET_REG(reg, mat_color.a, 24, 31);
        colIdx = 1;
        break;
    case GX_COLOR0A0:
        reg = *(u32*)&mat_color;
        colIdx = 0;
        break;
    case GX_COLOR1A1:
        reg = *(u32*)&mat_color;
        colIdx = 1;
        break;
    default:
        return;
    }
    __GXData->dirtyState |= 0x400 << colIdx;
    __GXData->matColor[colIdx].word = reg;
}

/* Sets the number of colour channels the vertices carry. */
void GXSetNumChans(u8 nChans)
{
    GXData* gx = __GXData;

    GX_SET_REG(gx->genMode.word, nChans, 25, 27);
    gx->dirtyState |= 0x01000004;
}

/* Sets a channel's control word: enable, colour sources, light mask, diffuse and attenuation functions. */
void GXSetChanCtrl(s32 chan, u8 enable, s32 amb_src, s32 mat_src, u32 light_mask, s32 diff_fn, s32 attn_fn)
{
    u32 reg;
    u32 idx;

    idx = chan & 3;
    reg = 0;
    GX_SET_REG(reg, enable, 30, 30);
    GX_SET_REG(reg, mat_src, 31, 31);
    GX_SET_REG(reg, amb_src, 25, 25);
    if (attn_fn == GX_AF_SPEC) {
        diff_fn = GX_DF_NONE;
    }
    GX_SET_REG(reg, diff_fn, 23, 24);
    GX_SET_REG(reg, attn_fn != GX_AF_NONE, 22, 22);
    GX_SET_REG(reg, attn_fn != GX_AF_SPEC, 21, 21);
    GX_SET_REG(reg, light_mask, 26, 29);
    GX_SET_REG(reg, light_mask >> 4, 17, 20);
    __GXData->chanCtrl[idx].word = reg;
    __GXData->dirtyState |= 0x1000 << idx;
    if (chan == GX_COLOR0A0) {
        __GXData->chanCtrl[2].word = reg;
        __GXData->dirtyState |= 0x5000;
    } else if (chan == GX_COLOR1A1) {
        __GXData->chanCtrl[3].word = reg;
        __GXData->dirtyState |= 0xA000;
    }
}
