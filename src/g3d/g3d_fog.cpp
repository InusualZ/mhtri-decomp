/*
 * g3d/g3d_fog.cpp - nw4r g3d `Fog`: the handle constructor over a FogData record, Init, the copy, the parameter read,
 *   the range-adjustment set-up and SetGP (the GX fog and range-adjustment load).
 * RANGE. .text 0x80079FB4-0x8007A400 (8 functions); extab 0x800083D8-0x80008408, extabindex 0x80020BEC-0x80020C34,
 *   .data 0x8058EAD8-0x8058EB38 (opens on "g3d_fog.cpp", read from fog_init on), .sdata2 0x80795E28-0x80795E30 (a
 *   0.0 pooled again after `g3d/g3d_draw.cpp`'s 0x80795E18, so a new TU).  `g3d/g3d_draw.cpp` precedes it;
 *   `g3d/g3d_init.cpp` follows at the version-string user 0x8007A400.
 * FLAGS. cflags_g3d (docs/g3d.md); `#pragma peephole off` (playbook idea 106) except fn_8007A1B8 and SetGP, which
 *   measure better with the pass on (SetFogRangeAdjParam keeps the `clrlwi` of its width only with it off);
 *   `#pragma pool_data off`; `#pragma dont_inline on`.
 * NAMES. Fog, FogData and the members are nw4r's.  fog_ctor, fog_init and fn_8007A1B8 keep C names because
 *   `g3d/g3d_scnroot.cpp` and `stage/stg_w.cpp` call them by those names (nw4r's Fog::Fog, Fog::Init and
 *   Fog::GetFogParam).  GUESS: fog_set_data (0x80079FE4, the handle's store).
 * SHAPES. CopyTo copies the record as six doublewords (FogDataBlock, retail's `lfd`/`stfd` pairs): a FogData
 *   assignment is memberwise under this compiler (out of line under `dont_inline`), and its two handles are separate
 *   temporaries (stack slots 0xC and 0x8).
 * RESIDUALS. mtx44_ptr (MTX44's conversion to the GX matrix) is g3d/g3d_camera.cpp's kept copy, declared here.
 */

#include "types.h"
#include "gx.h"
#include "RVLGX/GXPixel.h"         /* GXSetFog / GXSetFogRangeAdj / GXInitFogAdjTable (rule 2) */
#include "nw4r/db_assert.h"        /* nw4r::db::Panic (rule 2) */
#include "fn_80047398.h"           /* color_rgba_copy (rule 2) */
#include "g3d/g3d_fog.h"
#include "g3d/light_obj_is_enable.h" /* fog_is_valid / fog_ptr, owner g3d/g3d_anmscn.cpp (rule 2) */

#pragma peephole off
#pragma pool_data off
#pragma dont_inline on

extern "C" {
f32 (*mtx44_ptr(const nw4r::math::MTX44* pMtx))[4];
void fog_set_data(nw4r::g3d::Fog* pFog, nw4r::g3d::FogData* pData);
}

extern "C" {

/* 0x80079FB4 (0x30): constructs a fog handle at `pSelf` on the record `pData` and returns `pSelf`. */
s32 fog_ctor(s32 pSelf, void* pData) { /* untyped: opaque handle - the FogData record */
    fog_set_data((nw4r::g3d::Fog*)pSelf, (nw4r::g3d::FogData*)pData);
    return pSelf;
}

/* 0x80079FE4 (0x8): stores the record. */
void fog_set_data(nw4r::g3d::Fog* pFog, nw4r::g3d::FogData* pData) {
    pFog->mpFogData = pData;
}

/* 0x80079FEC (0xC8): resets the fog: no fog, zero range, black, range adjustment off. */
void fog_init(s32 pSelf) {
    nw4r::g3d::Fog* pFog = (nw4r::g3d::Fog*)pSelf;
    if (!fog_is_valid(pFog)) {
        nw4r::db::Panic("g3d_fog.cpp", 0x20, "NW4R:Failed assertion IsValid()");
    }
    if (fog_is_valid(pFog)) {
        nw4r::g3d::FogData& data = *fog_ptr(pFog);
        data.type = 0;
        data.startz = 0.0f;
        data.endz = 0.0f;
        data.Near = 0.0f;
        data.Far = 0.0f;
        data.color.a = 0;
        data.color.b = 0;
        data.color.g = 0;
        data.color.r = 0;
        data.adjEnable = 0;
        data.pad_0x19 = 0;
        data.adjCenter = 0;
        data.adjTable.r[0] = 0;
        data.adjTable.r[1] = 0;
        data.adjTable.r[2] = 0;
        data.adjTable.r[3] = 0;
        data.adjTable.r[4] = 0;
        data.adjTable.r[5] = 0;
        data.adjTable.r[6] = 0;
        data.adjTable.r[7] = 0;
        data.adjTable.r[8] = 0;
        data.adjTable.r[9] = 0;
    }
}

}

namespace nw4r {
namespace g3d {

/* The record seen as six doublewords, the unit CopyTo copies it in. size: 0x30 */
struct FogDataBlock {
    /* +0x00 */ f64 words[6];
};

/* 0x8007A0B4 (0xFC): copies the record to `pDst` and returns a handle on the copy (a null handle when there is
 * nothing to copy). */
/* untyped: byte range - the copy's destination */
Fog Fog::CopyTo(void* pDst) const {
    if ((u32)pDst & 3) {
        nw4r::db::Panic("g3d_fog.cpp", 0x3D, "NW4R:Failed assertion !((u32)pDst & 0x3)");
    }
    if (!fog_is_valid(this)) {
        nw4r::db::Panic("g3d_fog.cpp", 0x3E, "NW4R:Failed assertion IsValid()");
    }
    if (pDst != NULL && fog_is_valid(this)) {
        Fog fog;
        const FogDataBlock* pSrc = (const FogDataBlock*)ptr();
        for (int i = 0; i < 6; i++) {
            ((FogDataBlock*)pDst)->words[i] = pSrc->words[i];
        }
        return *(Fog*)fog_ctor((s32)&fog, pDst);
    }
    Fog fog;
    return *(Fog*)fog_ctor((s32)&fog, NULL);
}

/* 0x8007A1B0 (0x8): returns the record. */
const FogData* Fog::ptr() const {
    return mpFogData;
}

}  // namespace g3d
}  // namespace nw4r

extern "C" {

#pragma peephole on
/* 0x8007A1B8 (0xF0): reads the fog's type, range, clip planes and colour into the outputs that are given. */
void fn_8007A1B8(const nw4r::g3d::Fog* pFog, u32* pType, f32* pStartz, f32* pEndz, f32* pNear, f32* pFar,
                 GXColor* pColor) {
    if (!fog_is_valid(pFog)) {
        nw4r::db::Panic("g3d_fog.cpp", 0x68, "NW4R:Failed assertion IsValid()");
    }
    if (fog_is_valid(pFog)) {
        const nw4r::g3d::FogData& data = *fog_ptr(pFog);
        if (pType != NULL) {
            *pType = data.type;
        }
        if (pStartz != NULL) {
            *pStartz = data.startz;
        }
        if (pEndz != NULL) {
            *pEndz = data.endz;
        }
        if (pNear != NULL) {
            *pNear = data.Near;
        }
        if (pFar != NULL) {
            *pFar = data.Far;
        }
        if (pColor != NULL) {
            color_rgba_copy((u8*)pColor, (const u8*)&data.color);
        }
    }
}
#pragma peephole off

}

namespace nw4r {
namespace g3d {

/* 0x8007A2A8 (0xAC): sets the range-adjustment centre and builds its table from the projection. */
void Fog::SetFogRangeAdjParam(u16 width, u16 center, const math::MTX44& projMtx) {
    if (!fog_is_valid(this)) {
        nw4r::db::Panic("g3d_fog.cpp", 0x81, "NW4R:Failed assertion IsValid()");
    }
    if (fog_is_valid(this)) {
        FogData& data = *fog_ptr(this);
        data.adjCenter = center;
        GXInitFogAdjTable(&data.adjTable, width, mtx44_ptr(&projMtx));
    }
}

#pragma peephole on
/* 0x8007A354 (0xAC): loads the fog (and its range adjustment when there is fog) into GX. */
void Fog::SetGP() const {
    if (!fog_is_valid(this)) {
        nw4r::db::Panic("g3d_fog.cpp", 0x8F, "NW4R:Failed assertion IsValid()");
    }
    if (fog_is_valid(this)) {
        const FogData& data = *ptr();
        if (data.type != 0) {
            GXSetFogRangeAdj(data.adjEnable, data.adjCenter, &data.adjTable);
        }
        GXSetFog(data.type, data.startz, data.endz, data.Near, data.Far, data.color);
    }
}
#pragma peephole off

}  // namespace g3d
}  // namespace nw4r
