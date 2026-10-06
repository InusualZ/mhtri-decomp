/* g3d/g3d_resmat - nw4r g3d material resources (texture/TLUT objects, texture SRT, gen mode, misc, fur, pixel, tev
 *   colour, indirect matrix and channel display lists, texture-palette binding) and the `ResMdl` model accessors.
 * RANGE. .text 0x800947A4-0x80098D5C (140 functions); extab, extabindex, .data 0x80590D78-0x80591280 (the asserts'
 *   strings), .sdata2 0x80795F10-0x80795F48.
 * FLAGS. cflags_g3d (docs/g3d.md).
 * NAMES. The classes and members are nw4r's (`g3d_resmat.h`, `g3d_resmdl.h`); ResMatFur::GetLayerRatio and
 *   ResMat::GetResMatDLData are GUESSES (the fur layer ratio's two curves; the 0x180-byte block ResMat::Init
 *   stores).
 * RESIDUALS. the ResMdl accessors from 0x80097D40 are `g3d_resmdl.cpp`'s code (a seam candidate: their strings
 *   follow the material's and cite only `g3d_resvtx_ac.h`).
 *   ResTexPlttInfo::Bind: retail copy-constructs the two names and the found texture/palette out of line.
 *   ResMdl::Bind: saves r25 where retail saves from r26 (`_savegpr_25`/`_restgpr_25`).
 *   relocdiff --callees: the asserts pass literal strings where retail names this unit's `.data` labels (the
 *   TLUT range assertion `resmat_tlut_id_range_assert_msg` in the five ResTlutObj accessors); ResMatMisc::
 *   GetIndirectMethod calls `ptr() const` where retail calls the non-const copy; ResMatTevColor::CallDisplayList
 *   and the three ResMatIndMtxAndScale matrix accessors reach `ref()` through a different copy than retail;
 *   ResTexPlttInfo::Bind copy-constructs the names and the found handles inline (retail calls fn_80062D58,
 *   fn_80069C14 and fn_80069BD8).
 *   flipcheck: `.data` 0x4A8 of 0x508 and `.sdata2` 0x2C of 0x38 (the accessor copies' strings and pool sit
 *   elsewhere), `.sdata` 0x4 emitted and not claimed.
 * SHAPES. every accessor copy another unit keeps is called through its class (g3d/g3d_rescommon.h); the copies
 *   this TU keeps are defined here in retail's order.
 */

#include "types.h"
#include "gx.h"
#include "nw4r/math.h"
#include "nw4r/db_assert.h"
#include "g3d/g3d_rescommon.h"
#include "g3d/g3d_resmat.h"
#include "g3d/g3d_resvtx.h"
#include "g3d/g3d_resnode.h"
#include "g3d/g3d_resshp.h"
#include "g3d/g3d_resfile.h"
#include "g3d/g3d_resanmtexsrt.h"
#include "g3d/g3d_cpu.h"
#include "g3d/g3d_calcview.h"
#include "g3d/fn_80075DCC.h"
#include "fn_8004CAD8.h"
#include "fn_80047398.h"
#include "RVLGX/GXTexture_tail.h"

#pragma pool_data off

#define RESMAT_FILE "g3d_resmat.cpp"
#define RESMAT_AC_FILE "g3d_resmat_ac.h"
#define RESMAT_ASSERT(cond, line, text) \
    (void)((cond) || (nw4r::db::Panic(RESMAT_FILE, line, "NW4R:Failed assertion " text), 0))

namespace nw4r {
namespace g3d {

/* 0x800947A4 (0xC4): returns texture map `id`'s object, or NULL. */
const GXTexObj* ResTexObj::GetTexObj(_GXTexMapID id) const {
    RESMAT_ASSERT(IsValid(), 384, "IsValid()");
    RESMAT_ASSERT(id >= GX_TEXMAP0 && id <= GX_TEXMAP7, 385, "id >= GX_TEXMAP0 && id <= GX_TEXMAP7");
    if (IsValid() && id >= 0 && id <= 7) {
        return &ptr()->texObj[id];
    }
    return NULL;
}

NW4R_G3D_RESOURCE_PTR_CONST(ResTexObj)

/* 0x80094870 (0xC4): returns texture map `id`'s object, or NULL. */
GXTexObj* ResTexObj::GetTexObj(_GXTexMapID id) {
    RESMAT_ASSERT(IsValid(), 400, "IsValid()");
    RESMAT_ASSERT(id >= GX_TEXMAP0 && id <= GX_TEXMAP7, 401, "id >= GX_TEXMAP0 && id <= GX_TEXMAP7");
    if (IsValid() && id >= 0 && id <= 7) {
        return &ptr()->texObj[id];
    }
    return NULL;
}

NW4R_G3D_RESOURCE_PTR(ResTexObj)

/* 0x8009493C (0xD4): tells whether texture map `id`'s object is in use. */
bool ResTexObj::IsValidTexObj(_GXTexMapID id) const {
    RESMAT_ASSERT(IsValid(), 416, "IsValid()");
    RESMAT_ASSERT(id >= GX_TEXMAP0 && id <= GX_TEXMAP7, 417, "id >= GX_TEXMAP0 && id <= GX_TEXMAP7");
    if (IsValid() && id >= 0 && id <= 7) {
        return (ptr()->flagUsedTexMapID & (1 << id)) != 0;
    }
    return false;
}

/* 0x80094A10 (0xC4): marks texture map `id`'s object as in use. */
void ResTexObj::Validate(_GXTexMapID id) {
    RESMAT_ASSERT(IsValid(), 432, "IsValid()");
    RESMAT_ASSERT(id >= GX_TEXMAP0 && id <= GX_TEXMAP7, 433, "id >= GX_TEXMAP0 && id <= GX_TEXMAP7");
    if (IsValid() && id >= 0 && id <= 7) {
        ptr()->flagUsedTexMapID |= 1 << id;
    }
}

/* 0x80094AD4 (0xC8): marks texture map `id`'s object as unused. */
void ResTexObj::Invalidate(_GXTexMapID id) {
    RESMAT_ASSERT(IsValid(), 444, "IsValid()");
    RESMAT_ASSERT(id >= GX_TEXMAP0 && id <= GX_TEXMAP7, 445, "id >= GX_TEXMAP0 && id <= GX_TEXMAP7");
    if (IsValid() && id >= 0 && id <= 7) {
        ptr()->flagUsedTexMapID &= ~(1 << id);
    }
}

/* 0x80094B9C (0x11C): copies the in-use texture objects to `pDst` and returns a handle on the copy. */
/* untyped: byte range */
ResTexObj ResTexObj::CopyTo(void* pDst) const {
    RESMAT_ASSERT(IsValid(), 457, "IsValid()");
    RESMAT_ASSERT(!((u32)pDst & 0x3), 458, "!((u32)p & 0x3)");
    const ResTexObjData* pSrc = ptr();
    ResTexObjData* pData = static_cast<ResTexObjData*>(pDst);
    u32 flag = pSrc->flagUsedTexMapID;
    pData->flagUsedTexMapID = flag;
    RESMAT_ASSERT(!(flag & 0xffffff00), 467, "!(flag & 0xffffff00)");
    const GXTexObj* pSrcObj = &pSrc->texObj[0];
    GXTexObj* pDstObj = &pData->texObj[0];
    for (; flag != 0; flag >>= 1, pSrcObj++, pDstObj++) {
        if (flag & 1) {
            detail::Copy32ByteBlocks(pDstObj, pSrcObj, sizeof(GXTexObj));
        }
    }
    return ResTexObj(pDst);
}

/* 0x80094CB8 (0xC4): returns TLUT `id`'s object, or NULL. */
const GXTlutObj* ResTlutObj::GetTlut(_GXTlut id) const {
    RESMAT_ASSERT(IsValid(), 490, "IsValid()");
    RESMAT_ASSERT(id >= GX_TLUT0 && id <= GX_TLUT7, 491, "id >= GX_TLUT0 && id <= GX_TLUT7");
    if (IsValid() && id >= 0 && id <= 7) {
        return &ptr()->tlutObj[id];
    }
    return NULL;
}

NW4R_G3D_RESOURCE_PTR_CONST(ResTlutObj)

/* 0x80094D84 (0xC4): returns TLUT `id`'s object, or NULL. */
GXTlutObj* ResTlutObj::GetTlut(_GXTlut id) {
    RESMAT_ASSERT(IsValid(), 506, "IsValid()");
    RESMAT_ASSERT(id >= GX_TLUT0 && id <= GX_TLUT7, 507, "id >= GX_TLUT0 && id <= GX_TLUT7");
    if (IsValid() && id >= 0 && id <= 7) {
        return &ptr()->tlutObj[id];
    }
    return NULL;
}

NW4R_G3D_RESOURCE_PTR(ResTlutObj)

/* 0x80094E50 (0xD4): tells whether TLUT `id`'s object is in use. */
bool ResTlutObj::IsValidTlut(_GXTlut id) const {
    RESMAT_ASSERT(IsValid(), 522, "IsValid()");
    RESMAT_ASSERT(id >= GX_TLUT0 && id <= GX_TLUT7, 523, "id >= GX_TLUT0 && id <= GX_TLUT7");
    if (IsValid() && id >= 0 && id <= 7) {
        return (ptr()->flagUsedTlutID & (1 << id)) != 0;
    }
    return false;
}

/* 0x80094F24 (0xC4): marks TLUT `id`'s object as in use. */
void ResTlutObj::Validate(_GXTlut id) {
    RESMAT_ASSERT(IsValid(), 538, "IsValid()");
    RESMAT_ASSERT(id >= GX_TLUT0 && id <= GX_TLUT7, 539, "id >= GX_TLUT0 && id <= GX_TLUT7");
    if (IsValid() && id >= 0 && id <= 7) {
        ptr()->flagUsedTlutID |= 1 << id;
    }
}

/* 0x80094FE8 (0xC8): marks TLUT `id`'s object as unused. */
void ResTlutObj::Invalidate(_GXTlut id) {
    RESMAT_ASSERT(IsValid(), 550, "IsValid()");
    RESMAT_ASSERT(id >= GX_TLUT0 && id <= GX_TLUT7, 551, "id >= GX_TLUT0 && id <= GX_TLUT7");
    if (IsValid() && id >= 0 && id <= 7) {
        ptr()->flagUsedTlutID &= ~(1 << id);
    }
}

/* 0x800950B0 (0x108): copies the in-use TLUT objects (in 32-byte blocks) to `pDst`, returning a handle on it. */
/* untyped: byte range */
ResTlutObj ResTlutObj::CopyTo(void* pDst) const {
    RESMAT_ASSERT(!((u32)pDst & 0x3), 562, "!((u32)p & 0x3)");
    const ResTlutObjData& r = ref();
    ResTlutObjData* pData = static_cast<ResTlutObjData*>(pDst);
    u32 flag = r.flagUsedTlutID;
    pData->flagUsedTlutID = flag;
    RESMAT_ASSERT(!(flag & 0xffffff00), 570, "!(flag & 0xffffff00)");
    if (flag != 0) {
        if (!(flag & 0xfc)) {
            detail::Copy32ByteBlocks(&pData->tlutObj[0], &r.tlutObj[0], 32);
        } else if (!(flag & 0xe0)) {
            detail::Copy32ByteBlocks(&pData->tlutObj[0], &r.tlutObj[0], 64);
        } else {
            detail::Copy32ByteBlocks(&pData->tlutObj[0], &r.tlutObj[0], 96);
        }
    }
    return ResTlutObj(pDst);
}

NW4R_G3D_RESOURCE_REF_CONST(ResTlutObj, RESMAT_AC_FILE, 74)
NW4R_G3D_RESOURCE_CLASS_NAME(ResTlutObj)

/* 0x80095228 (0x11C): copies the transforms and effects of the coordinates in use to `pDst`. */
/* untyped: byte range */
ResTexSrt ResTexSrt::CopyTo(void* pDst) const {
    RESMAT_ASSERT(!((u32)pDst & 0x3), 602, "!((u32)p & 0x3)");
    const ResTexSrtData& r = ref();
    ResTexSrtData* pData = static_cast<ResTexSrtData*>(pDst);
    u32 flag = r.flagTexSrt;
    pData->flagTexSrt = flag;
    pData->texMtxMode = r.texMtxMode;
    const TexSrt* pSrcSrt = &r.texSrt[0];
    const TexMtxEffect* pSrcEffect = &r.effect[0];
    TexSrt* pDstSrt = &pData->texSrt[0];
    TexMtxEffect* pDstEffect = &pData->effect[0];
    for (; flag != 0; flag >>= 4, pSrcSrt++, pSrcEffect++, pDstSrt++, pDstEffect++) {
        if (flag & 0xf) {
            *pDstSrt = *pSrcSrt;
            pDstEffect->ref_camera = pSrcEffect->ref_camera;
            pDstEffect->ref_light = pSrcEffect->ref_light;
            pDstEffect->map_mode = pSrcEffect->map_mode;
            pDstEffect->misc_flag = pSrcEffect->misc_flag;
            mtx34_copy_ps(&pDstEffect->effectMtx, &pSrcEffect->effectMtx);
        }
    }
    return ResTexSrt(pDst);
}

/* 0x80095344 (0x8C): sets coordinate `id`'s effect matrix (the identity when `pMtx` is NULL). */
bool ResTexSrt::SetEffectMtx(u32 id, const math::MTX34* pMtx) {
    if (id < 8) {
        TexMtxEffect* pEffect = &ref().effect[id];
        if (pMtx != NULL) {
            mtx34_copy_ps(&pEffect->effectMtx, pMtx);
            pEffect->misc_flag &= ~1;
        } else {
            mtx34_identity(&pEffect->effectMtx);
            pEffect->misc_flag |= 1;
        }
        return true;
    }
    return false;
}

/* 0x800953D0 (0x68): copies coordinate `id`'s effect matrix out. */
bool ResTexSrt::GetEffectMtx(u32 id, math::MTX34* pMtx) const {
    if (pMtx != NULL && id < 8) {
        mtx34_copy_ps(pMtx, &ref().effect[id].effectMtx);
        return true;
    }
    return false;
}

/* 0x80095438 (0xC4): sets coordinate `id`'s mapping mode and the camera and light it follows. */
bool ResTexSrt::SetMapMode(u32 id, u32 mapMode, int refCamera, int refLight) {
    if (id < 8 && mapMode < 256) {
        TexMtxEffect* pEffect = &ref().effect[id];
        pEffect->map_mode = mapMode;
        bool validCamera = (u32)refCamera <= 31;
        pEffect->ref_camera = validCamera ? (s8)refCamera : (s8)-1;
        bool validLight = (u32)refLight <= 127;
        pEffect->ref_light = validLight ? (s8)refLight : (s8)-1;
        return true;
    }
    return false;
}

/* 0x800954FC (0xA4): reads coordinate `id`'s mapping mode and the camera and light it follows. */
bool ResTexSrt::GetMapMode(u32 id, u32* pMapMode, int* pRefCamera, int* pRefLight) const {
    if (id < 8) {
        const TexMtxEffect* pEffect = &ref().effect[id];
        if (pMapMode != NULL) {
            *pMapMode = pEffect->map_mode;
        }
        if (pRefCamera != NULL) {
            *pRefCamera = pEffect->ref_camera;
        }
        if (pRefLight != NULL) {
            *pRefLight = pEffect->ref_light;
        }
        return true;
    }
    return false;
}

NW4R_G3D_RESOURCE_PTR(ResGenMode)

/* 0x800955A8 (0x78): sets the number of tev stages. */
void ResGenMode::GXSetNumTevStages(u8 nStages) {
    RESMAT_ASSERT(IsValid(), 742, "IsValid()");
    if (IsValid()) {
        ptr()->nTevs = nStages;
    }
}

/* Sets the cull mode. */
void ResGenMode::GXSetCullMode(_GXCullMode cullMode) {
    RESMAT_ASSERT(IsValid(), 758, "IsValid()");
    if (IsValid()) {
        ptr()->cullMode = cullMode;
    }
}

/* 0x80095698 (0x80): copies the block to `pDst` and returns a handle on the copy. */
/* untyped: byte range */
ResGenMode ResGenMode::CopyTo(void* pDst) const {
    RESMAT_ASSERT(!((u32)pDst & 0x3), 768, "!((u32)p & 0x3)");
    *static_cast<ResGenModeData*>(pDst) = ref();
    return ResGenMode(pDst);
}

/* 0x80095718 (0x2C): copies the counts and the cull mode. */
ResGenModeData& ResGenModeData::operator=(const ResGenModeData& rhs) {
    nTexGens = rhs.nTexGens;
    nChans = rhs.nChans;
    nTevs = rhs.nTevs;
    nInds = rhs.nInds;
    cullMode = rhs.cullMode;
    return *this;
}

NW4R_G3D_RESOURCE_REF_CONST(ResGenMode, RESMAT_AC_FILE, 175)
NW4R_G3D_RESOURCE_CLASS_NAME(ResGenMode)
NW4R_G3D_RESOURCE_PTR(ResMatMisc)

/* 0x800957BC (0x74): returns the z-compare location, false when the handle is NULL. */
u8 ResMatMisc::GXGetZCompLoc() const {
    RESMAT_ASSERT(IsValid(), 795, "IsValid()");
    if (IsValid()) {
        return ptr()->zCompLoc;
    }
    return 0;
}

NW4R_G3D_RESOURCE_PTR_CONST(ResMatMisc)

/* 0x80095838 (0x90): sets the light set the material uses (-1 when `idx` is out of range). */
void ResMatMisc::SetLightSetIdx(int idx) {
    RESMAT_ASSERT(IsValid(), 810, "IsValid()");
    if (IsValid()) {
        ResMatMiscData* p = ptr();
        if ((u32)idx > 127) {
            p->light_set_idx = -1;
        } else {
            p->light_set_idx = idx;
        }
    }
}

/* 0x800958C8 (0x78): returns the light set the material uses, -1 for none. */
int ResMatMisc::GetLightSetIdx() const {
    RESMAT_ASSERT(IsValid(), 825, "IsValid()");
    if (IsValid()) {
        return ptr()->light_set_idx;
    }
    return -1;
}

/* 0x80095940 (0x78): returns the fog the material uses, -1 for none. */
int ResMatMisc::GetFogIdx() const {
    RESMAT_ASSERT(IsValid(), 853, "IsValid()");
    if (IsValid()) {
        return ptr()->fog_idx;
    }
    return -1;
}

/* 0x800959B8 (0x124): sets indirect matrix `id`'s method and the light a normal map follows. */
void ResMatMisc::SetIndirectMethod(_GXIndTexMtxID id, ResMatMiscData::IndirectMethod method, s8 normalMapRef) {
    RESMAT_ASSERT(IsValid(), 875, "IsValid()");
    RESMAT_ASSERT(id == GX_ITM_0 || id == GX_ITM_1 || id == GX_ITM_2, 879, "id == GX_ITM_0 || id == GX_ITM_1 || id == GX_ITM_2");
    RESMAT_ASSERT(method >= 0 && method < ResMatMiscData::NUM_OF_INDIRECT_METHOD, 882,
                  "method >= 0 && method < ResMatMiscData::NUM_OF_INDIRECT_METHOD");
    if (IsValid() && (id == GX_ITM_0 || id == GX_ITM_1 || id == GX_ITM_2) && method >= 0 && method < ResMatMiscData::NUM_OF_INDIRECT_METHOD) {
        ResMatMiscData* p = ptr();
        p->indirect_method[id - 1] = method;
        if ((s8)normalMapRef < 0) {
            p->normal_map_ref[id - 1] = -1;
        } else {
            p->normal_map_ref[id - 1] = normalMapRef;
        }
    }
}

/* 0x80095ADC (0xE8): reads indirect matrix `id`'s method and the light a normal map follows. */
void ResMatMisc::GetIndirectMethod(_GXIndTexMtxID id, ResMatMiscData::IndirectMethod* pMethod, s8* pNormalMapRef) const {
    RESMAT_ASSERT(IsValid(), 903, "IsValid()");
    RESMAT_ASSERT(id == GX_ITM_0 || id == GX_ITM_1 || id == GX_ITM_2, 907, "id == GX_ITM_0 || id == GX_ITM_1 || id == GX_ITM_2");
    if (IsValid() && (id == GX_ITM_0 || id == GX_ITM_1 || id == GX_ITM_2)) {
        const ResMatMiscData* p = ptr();
        if (pMethod != NULL) {
            *pMethod = static_cast<ResMatMiscData::IndirectMethod>(p->indirect_method[id - 1]);
        }
        if (pNormalMapRef != NULL) {
            *pNormalMapRef = p->normal_map_ref[id - 1];
        }
    }
}

/* 0x80095BC4 (0x80): copies the block to `pDst` and returns a handle on the copy. */
/* untyped: byte range */
ResMatMisc ResMatMisc::CopyTo(void* pDst) const {
    RESMAT_ASSERT(!((u32)pDst & 0x3), 924, "!((u32)p & 0x3)");
    *static_cast<ResMatMiscData*>(pDst) = ref();
    return ResMatMisc(pDst);
}

/* 0x80095C44 (0x34): copies the four bytes and the two indirect tables. */
ResMatMiscData& ResMatMiscData::operator=(const ResMatMiscData& rhs) {
    zCompLoc = rhs.zCompLoc;
    light_set_idx = rhs.light_set_idx;
    fog_idx = rhs.fog_idx;
    pad_0x3 = rhs.pad_0x3;
    indirect_method_word = rhs.indirect_method_word;
    normal_map_ref_word = rhs.normal_map_ref_word;
    return *this;
}

NW4R_G3D_RESOURCE_REF_CONST(ResMatMisc, RESMAT_AC_FILE, 243)
NW4R_G3D_RESOURCE_CLASS_NAME(ResMatMisc)
NW4R_G3D_RESOURCE_REF_CONST(ResMatFur, RESMAT_AC_FILE, 287)
NW4R_G3D_RESOURCE_PTR_CONST(ResMatFur)

/* 0x80095D54 (0x114): returns fur layer `layer`'s ratio: linear in the layer, or a 0.6 power of it. */
f32 ResMatFur::GetLayerRatio(u32 layer) const {
    RESMAT_ASSERT(IsValid(), 1081, "IsValid()");
    if (IsValid()) {
        u32 numLayer = ref().numLayer;
        switch (ref().layerRatioMode) {
        case 0:
            return (f32)(layer + 1) / (f32)numLayer;
        case 1:
            return math::FPow((f32)(layer + 1) / (f32)numLayer, 0.6f);
        default:
            return 0.0f;
        }
    }
    return 0.0f;
}

/* 0x80095E68 (0xC8): reads the alpha compare state from the display list; false when it carries none. */
bool ResMatPix::GXGetAlphaCompare(_GXCompare* pComp0, u8* pRef0, _GXAlphaOp* pOp, _GXCompare* pComp1, u8* pRef1) const {
    const u8* pDL = reinterpret_cast<const u8*>(&ref());
    if (pDL[0] == 0) {
        return false;
    }
    u32 reg;
    detail::ResReadBPCmd(pDL, &reg);
    if (pRef0 != NULL) {
        *pRef0 = reg & 0xff;
    }
    if (pRef1 != NULL) {
        *pRef1 = (reg >> 8) & 0xff;
    }
    if (pComp0 != NULL) {
        *pComp0 = static_cast<_GXCompare>((reg >> 16) & 7);
    }
    if (pComp1 != NULL) {
        *pComp1 = static_cast<_GXCompare>((reg >> 19) & 7);
    }
    if (pOp != NULL) {
        *pOp = static_cast<_GXAlphaOp>((reg >> 22) & 3);
    }
    return true;
}

/* Writes the alpha compare state into the display list. */
void ResMatPix::GXSetAlphaCompare(_GXCompare comp0, u8 ref0, _GXAlphaOp op, _GXCompare comp1, u8 ref1) {
    u8* pDL = reinterpret_cast<u8*>(&ref());
    detail::ResWriteBPCmd(pDL, (comp0 << 16 | ref0 | ref1 << 8) | (op << 22 | comp1 << 19 | 0xf3000000));
}

/* 0x80095FA4 (0xF8): reads the blend state from the display list; false when it carries none. */
bool ResMatPix::GXGetBlendMode(_GXBlendMode* pType, _GXBlendFactor* pSrcFactor, _GXBlendFactor* pDstFactor,
                               _GXLogicOp* pOp) const {
    const u8* pDL = reinterpret_cast<const u8*>(&ref());
    if (pDL[10] == 0) {
        return false;
    }
    u32 reg;
    detail::ResReadBPCmd(pDL + 15, &reg);
    u32 type;
    if ((reg >> 1) & 1) {
        type = 2; /* GX_BM_LOGIC */
    } else if ((reg >> 11) & 1) {
        type = 3; /* GX_BM_SUBTRACT */
    } else {
        type = (reg & 1) != 0;
    }
    if (pType != NULL) {
        *pType = static_cast<_GXBlendMode>(type);
    }
    if (pSrcFactor != NULL) {
        *pSrcFactor = static_cast<_GXBlendFactor>((reg >> 8) & 7);
    }
    if (pDstFactor != NULL) {
        *pDstFactor = static_cast<_GXBlendFactor>((reg >> 5) & 7);
    }
    if (pOp != NULL) {
        *pOp = static_cast<_GXLogicOp>((reg >> 12) & 0xf);
    }
    return true;
}

/* Writes the blend state into the display list. */
void ResMatPix::GXSetBlendMode(_GXBlendMode type, _GXBlendFactor srcFactor, _GXBlendFactor dstFactor,
                               _GXLogicOp op) {
    u8* pDL = reinterpret_cast<u8*>(&ref()) + 10;
    detail::ResWriteSSMask(pDL, 0xffe3);
    detail::ResWriteBPCmd(pDL + 5, op << 12 | (type == 3) << 11 | srcFactor << 8 | dstFactor << 5 |
                                       (type == 2) << 1 | (type == 1 || type == 3) | 0x41000000);
}

/* 0x8009627C (0x20): packs four components into a colour. */
static GXColor MakeColor(u8 r, u8 g, u8 b, u8 a) {
    GXColor color;
    color.r = r;
    color.g = g;
    color.b = b;
    color.a = a;
    return color;
}

/* 0x80096154 (0x128): reads tev colour register `id` (1..3) from the display list. */
bool ResMatTevColor::GXGetTevColor(_GXTevRegID id, GXColor* pColor) const {
    if (pColor == NULL) {
        nw4r::db::Panic(RESMAT_FILE, 1468, "NW4R:Pointer must not be NULL (color)");
    }
    bool inRange = id >= 1 && id <= 3;
    if (!inRange) {
        nw4r::db::Panic(RESMAT_FILE, 1469, "id is out of bounds(%d)\n%d <= id <= %d not satisfied.", id, 1, 3);
    }
    const u8* pDL = reinterpret_cast<const u8*>(&ref()) + (id - 1) * 20;
    if (pDL[0] == 0) {
        return false;
    }
    u32 ra;
    u32 bg;
    detail::ResReadBPCmd(pDL, &ra);
    detail::ResReadBPCmd(pDL + 5, &bg);
    GXColor color = MakeColor(ra & 0xfff, (bg >> 12) & 0x7ff, bg & 0xfff, (ra >> 12) & 0x7ff);
    color_rgba_copy(reinterpret_cast<u8*>(pColor), reinterpret_cast<u8*>(&color));
    return true;
}

/* Writes tev colour register `id` (1..3) into the display list. */
void ResMatTevColor::GXSetTevColor(_GXTevRegID id, GXColor color) {
    bool inRange = id >= 1 && id <= 3;
    if (!inRange) {
        nw4r::db::Panic(RESMAT_FILE, 1504, "id is out of bounds(%d)\n%d <= id <= %d not satisfied.", id, 1, 3);
    }
    u8* pDL = reinterpret_cast<u8*>(&ref()) + (id - 1) * 20;
    u32 bg = color.b | color.g << 12 | (id * 2 + 0xe1) << 24;
    detail::ResWriteBPCmd(pDL, color.r | (id * 2 + 0xe0) << 24 | color.a << 12);
    detail::ResWriteBPCmd(pDL + 5, bg);
    detail::ResWriteBPCmd(pDL + 10, bg);
    detail::ResWriteBPCmd(pDL + 15, bg);
}

}  // namespace g3d
}  // namespace nw4r

/* 0x8009639C (0x24): copies the four signed components. */
GXColorS10& GXColorS10::operator=(const GXColorS10& rhs) {
    r = rhs.r;
    g = rhs.g;
    b = rhs.b;
    a = rhs.a;
    return *this;
}

namespace nw4r {
namespace g3d {

/* 0x800963C0 (0x124): reads tev constant colour `id` (0..3) from the display list. */
bool ResMatTevColor::GXGetTevKColor(_GXTevKColorID id, GXColor* pColor) const {
    if (pColor == NULL) {
        nw4r::db::Panic(RESMAT_FILE, 1620, "NW4R:Pointer must not be NULL (color)");
    }
    bool inRange = id >= 0 && id <= 3;
    if (!inRange) {
        nw4r::db::Panic(RESMAT_FILE, 1621, "id is out of bounds(%d)\n%d <= id <= %d not satisfied.", id, 0, 3);
    }
    const u8* pDL = reinterpret_cast<const u8*>(&ref()) + id * 10 + 64;
    if (pDL[0] == 0) {
        return false;
    }
    u32 ra;
    u32 bg;
    detail::ResReadBPCmd(pDL, &ra);
    detail::ResReadBPCmd(pDL + 5, &bg);
    GXColor color = MakeColor(ra & 0xfff, (bg >> 12) & 0x7ff, bg & 0xfff, (ra >> 12) & 0x7ff);
    color_rgba_copy(reinterpret_cast<u8*>(pColor), reinterpret_cast<u8*>(&color));
    return true;
}

/* Writes tev constant colour `id` (0..3) into the display list. */
void ResMatTevColor::GXSetTevKColor(_GXTevKColorID id, GXColor color) {
    bool inRange = id >= 0 && id <= 3;
    if (!inRange) {
        nw4r::db::Panic(RESMAT_FILE, 1656, "id is out of bounds(%d)\n%d <= id <= %d not satisfied.", id, 0, 3);
    }
    u8* pDL = reinterpret_cast<u8*>(&ref()) + id * 10 + 64;
    u32 bg = color.g << 12 | (color.b | 0x800000) | (id * 2 + 0xe1) << 24;
    detail::ResWriteBPCmd(pDL, color.a << 12 | (color.r | 0x800000) | (id * 2 + 0xe0) << 24);
    detail::ResWriteBPCmd(pDL + 5, bg);
}

/* 0x800965D0 (0x54): calls the tev colour display list, through the GX FIFO directly unless `bSync`. */
void ResMatIndMtxAndScale::CallDisplayList(u8 indNum, bool bSync) const {
    if (indNum) {
        const void* pDL = &ref();
        if (bSync) {
            GXCallDisplayList(pDL, 64);
        } else {
            GXFastCallDisplayList(pDL, 64);
        }
    }
}

/* 0x80096624 (0x344): reads indirect matrix `id` (1..3) from the display list, the scale folded in. */
bool ResMatIndMtxAndScale::GXGetIndTexMtx(_GXIndTexMtxID id, math::MTX34* pMtx) const {
    const u8* pDL;
    switch (id) {
    case 1:
        pDL = reinterpret_cast<const u8*>(&ref()) + 10;
        break;
    case 2:
        pDL = reinterpret_cast<const u8*>(&ref()) + 32;
        break;
    case 3:
        pDL = reinterpret_cast<const u8*>(&ref()) + 47;
        break;
    default:
        nw4r::db::Panic(RESMAT_FILE, 1814, "NW4R:Fatal Error\nGXGetIndTexMtx: Invalid matrix id");
        return false;
    }
    if (pDL[0] == 0) {
        return false;
    }
    u32 reg0;
    u32 reg1;
    u32 reg2;
    detail::ResReadBPCmd(pDL, &reg0);
    detail::ResReadBPCmd(pDL + 5, &reg1);
    detail::ResReadBPCmd(pDL + 10, &reg2);
    s8 scaleExp = (s8)((((reg2 >> 22) & 3) << 4 | (((reg0 >> 22) & 3) | ((reg1 >> 22) & 3) << 2)) - 17);
    f32 scale = 1.0f;
    if (scaleExp > 0) {
        for (; scaleExp > 0; scaleExp--) {
            scale *= 2.0f;
        }
    } else if (scaleExp < 0) {
        for (; scaleExp < 0; scaleExp++) {
            scale *= 0.5f;
        }
    }
    if (pMtx != NULL) {
        pMtx->m[0][0] = (1.0f / 1024.0f) * (scale * (f32)((s32)((reg0 & 0x7ff) << 21) >> 21));
        pMtx->m[0][1] = (1.0f / 1024.0f) * (scale * (f32)((s32)((reg1 & 0x7ff) << 21) >> 21));
        pMtx->m[0][2] = (1.0f / 1024.0f) * (scale * (f32)((s32)((reg2 & 0x7ff) << 21) >> 21));
        pMtx->m[0][3] = 0.0f;
        pMtx->m[1][0] = (1.0f / 1024.0f) * (scale * (f32)((s32)(((reg0 >> 11) & 0x7ff) << 21) >> 21));
        pMtx->m[1][1] = (1.0f / 1024.0f) * (scale * (f32)((s32)(((reg1 >> 11) & 0x7ff) << 21) >> 21));
        pMtx->m[1][2] = (1.0f / 1024.0f) * (scale * (f32)((s32)(((reg2 >> 11) & 0x7ff) << 21) >> 21));
        pMtx->m[1][3] = 0.0f;
        pMtx->m[2][0] = 0.0f;
        pMtx->m[2][1] = 0.0f;
        pMtx->m[2][2] = 1.0f;
        pMtx->m[2][3] = 0.0f;
    }
    return true;
}

/* 0x80096968 (0x244): reads indirect matrix `id` (1..3) and its scale exponent from the display list. */
bool ResMatIndMtxAndScale::GXGetIndTexMtx(_GXIndTexMtxID id, math::MTX34* pMtx, s8* pScaleExp) const {
    const u8* pDL;
    switch (id) {
    case 1:
        pDL = reinterpret_cast<const u8*>(&ref()) + 10;
        break;
    case 2:
        pDL = reinterpret_cast<const u8*>(&ref()) + 32;
        break;
    case 3:
        pDL = reinterpret_cast<const u8*>(&ref()) + 47;
        break;
    default:
        nw4r::db::Panic(RESMAT_FILE, 1910, "NW4R:Fatal Error\nGXGetIndTexMtx: Invalid matrix id");
        return false;
    }
    if (pDL[0] == 0) {
        return false;
    }
    u32 reg0;
    u32 reg1;
    u32 reg2;
    detail::ResReadBPCmd(pDL, &reg0);
    detail::ResReadBPCmd(pDL + 5, &reg1);
    detail::ResReadBPCmd(pDL + 10, &reg2);
    u32 exp = ((reg2 >> 22) & 3) << 4 | (((reg0 >> 22) & 3) | ((reg1 >> 22) & 3) << 2);
    if (pScaleExp != NULL) {
        *pScaleExp = (s8)(exp - 17);
    }
    if (pMtx != NULL) {
        pMtx->m[0][0] = (1.0f / 1024.0f) * (f32)((s32)((reg0 & 0x7ff) << 21) >> 21);
        pMtx->m[0][1] = (1.0f / 1024.0f) * (f32)((s32)((reg1 & 0x7ff) << 21) >> 21);
        pMtx->m[0][2] = (1.0f / 1024.0f) * (f32)((s32)((reg2 & 0x7ff) << 21) >> 21);
        pMtx->m[0][3] = 0.0f;
        pMtx->m[1][0] = (1.0f / 1024.0f) * (f32)((s32)(((reg0 >> 11) & 0x7ff) << 21) >> 21);
        pMtx->m[1][1] = (1.0f / 1024.0f) * (f32)((s32)(((reg1 >> 11) & 0x7ff) << 21) >> 21);
        pMtx->m[1][2] = (1.0f / 1024.0f) * (f32)((s32)(((reg2 >> 11) & 0x7ff) << 21) >> 21);
        pMtx->m[1][3] = 0.0f;
        pMtx->m[2][0] = 0.0f;
        pMtx->m[2][1] = 0.0f;
        pMtx->m[2][2] = 1.0f;
        pMtx->m[2][3] = 0.0f;
    }
    return true;
}

/* Writes indirect matrix `id` (1..3) and its scale exponent into the display list. */
void ResMatIndMtxAndScale::GXSetIndTexMtx(_GXIndTexMtxID id, const math::MTX34& mtx, s8 scaleExp) {
    u8* pDL;
    u32 reg;
    switch (id) {
    case 1:
        pDL = reinterpret_cast<u8*>(&ref()) + 10;
        reg = 0;
        break;
    case 2:
        pDL = reinterpret_cast<u8*>(&ref()) + 32;
        reg = 3;
        break;
    case 3:
        pDL = reinterpret_cast<u8*>(&ref()) + 47;
        reg = 6;
        break;
    default:
        nw4r::db::Panic(RESMAT_FILE, 2109, "NW4R:Fatal Error\nGXSetIndTexMtx: Invalid matrix id");
        return;
    }
    scaleExp += 17;
    detail::ResWriteBPCmd(pDL, (reg + 6) << 24 | ((scaleExp & 3) << 22 | ((s32)(1024.0f * mtx.m[0][0]) & 0x7ff) |
                                                  ((s32)(1024.0f * mtx.m[1][0]) & 0x7ff) << 11));
    detail::ResWriteBPCmd(pDL + 5, (reg + 7) << 24 | ((scaleExp >> 2) & 3) << 22 |
                                       (((s32)(1024.0f * mtx.m[0][1]) & 0x7ff) |
                                        ((s32)(1024.0f * mtx.m[1][1]) & 0x7ff) << 11));
    detail::ResWriteBPCmd(pDL + 10, (reg + 8) << 24 | ((scaleExp >> 4) & 3) << 22 |
                                        (((s32)(1024.0f * mtx.m[0][2]) & 0x7ff) |
                                         ((s32)(1024.0f * mtx.m[1][2]) & 0x7ff) << 11));
}

/* Sets channel `chan`'s material colour (the alpha too for a colour-alpha channel). */
void ResMatChan::GXSetChanMatColor(u32 chan, GXColor color) {
    ResChanEntry* pEntry = &ref().chan[chan & 1];
    if (!(chan & 2)) {
        pEntry->matColor.r = color.r;
        pEntry->matColor.g = color.g;
        pEntry->matColor.b = color.b;
        if (chan == 4 || chan == 5) {
            pEntry->matColor.a = color.a;
        }
    } else {
        pEntry->matColor.a = color.a;
    }
}

/* Sets channel `chan`'s ambient colour (the alpha too for a colour-alpha channel). */
void ResMatChan::GXSetChanAmbColor(u32 chan, GXColor color) {
    ResChanEntry* pEntry = &ref().chan[chan & 1];
    if (!(chan & 2)) {
        pEntry->ambColor.r = color.r;
        pEntry->ambColor.g = color.g;
        pEntry->ambColor.b = color.b;
        if (chan == 4 || chan == 5) {
            pEntry->ambColor.a = color.a;
        }
    } else {
        pEntry->ambColor.a = color.a;
    }
}

/* 0x80096E8C (0x88): reads channel `chan`'s material colour. */
bool ResMatChan::GXGetChanMatColor(u32 chan, GXColor* pColor) const {
    if (pColor == NULL) {
        nw4r::db::Panic(RESMAT_FILE, 2307, "NW4R:Pointer must not be NULL (mat_color)");
    }
    color_rgba_copy(reinterpret_cast<u8*>(pColor),
                    reinterpret_cast<const u8*>(&ref().chan[chan & 1].matColor));
    return true;
}

/* 0x80096F14 (0x88): reads channel `chan`'s ambient colour. */
bool ResMatChan::GXGetChanAmbColor(u32 chan, GXColor* pColor) const {
    if (pColor == NULL) {
        nw4r::db::Panic(RESMAT_FILE, 2327, "NW4R:Pointer must not be NULL (amb_color)");
    }
    color_rgba_copy(reinterpret_cast<u8*>(pColor),
                    reinterpret_cast<const u8*>(&ref().chan[chan & 1].ambColor));
    return true;
}

/* 0x80096F9C (0x13C): decodes channel `chan`'s control word into the GXSetChanCtrl parameters. */
bool ResMatChan::GXGetChanCtrl(u32 chan, u8* pEnable, u32* pAmbSrc, u32* pMatSrc, u32* pLightMask, u32* pDiffFn,
                               u32* pAttnFn) const {
    if (chan > 3) {
        nw4r::db::Panic(RESMAT_FILE, 2362,
                        "NW4R:Failed assertion (chan == GX_COLOR0) || (chan == GX_COLOR1) || (chan == GX_ALPHA0) || "
                        "(chan == GX_ALPHA1)");
    }
    const ResChanEntry* pEntry = &ref().chan[chan & 1];
    u32 param;
    if (!(chan & 2)) {
        param = pEntry->paramChanCtrlC;
    } else {
        param = pEntry->paramChanCtrlA;
    }
    if (pEnable != NULL) {
        *pEnable = (param >> 1) & 1;
    }
    if (pAmbSrc != NULL) {
        *pAmbSrc = (param >> 6) & 1;
    }
    if (pMatSrc != NULL) {
        *pMatSrc = param & 1;
    }
    u32 maskLo = (param >> 2) & 0xf;
    u32 maskHi = (param >> 11) & 0xf;
    if (pLightMask != NULL) {
        *pLightMask = maskLo | maskHi << 4;
    }
    u32 attnFn;
    u32 diffFn;
    if (!((param >> 10) & 1)) {
        attnFn = 0;
        diffFn = 0;
    } else if (!((param >> 9) & 1)) {
        attnFn = 2;
        diffFn = (param >> 7) & 3;
    } else {
        attnFn = 1;
        diffFn = (param >> 7) & 3;
    }
    if (pDiffFn != NULL) {
        *pDiffFn = diffFn;
    }
    if (pAttnFn != NULL) {
        *pAttnFn = attnFn;
    }
    return true;
}

/* 0x800970D8 (0x80): copies the block to `pDst` and returns a handle on the copy. */
/* untyped: byte range */
ResMatChan ResMatChan::CopyTo(void* pDst) const {
    RESMAT_ASSERT(!((u32)pDst & 0x3), 2433, "!((u32)p & 0x3)");
    *static_cast<ResMatChanData*>(pDst) = ref();
    return ResMatChan(pDst);
}

/* 0x80097158 (0x54): copies both channels. */
ResMatChanData& ResMatChanData::operator=(const ResMatChanData& rhs) {
    /* The block is copied as a whole, ten words at a time in pairs. */
    struct Words {
        /* +0x00 */ u32 word[10];
    }; /* size: 0x28 */
    *reinterpret_cast<Words*>(this) = *reinterpret_cast<const Words*>(&rhs);
    return *this;
}

/* 0x800971AC (0xE0): calls the texture-coordinate display list sized for `numGens` generators. */
void ResMatTexCoordGen::CallDisplayList(u8 numGens, bool bSync) const {
    if (numGens != 0) {
        const void* pDL = &ref();
        if (bSync) {
            if (numGens < 2) {
                GXCallDisplayList(pDL, 32);
            } else if (numGens < 4) {
                GXCallDisplayList(pDL, 64);
            } else if (numGens < 8) {
                GXCallDisplayList(pDL, 128);
            } else {
                GXCallDisplayList(pDL, 160);
            }
        } else {
            if (numGens < 2) {
                GXFastCallDisplayList(pDL, 32);
            } else if (numGens < 4) {
                GXFastCallDisplayList(pDL, 64);
            } else if (numGens < 8) {
                GXFastCallDisplayList(pDL, 128);
            } else {
                GXFastCallDisplayList(pDL, 160);
            }
        }
    }
}

/* 0x8009728C (0xB0): binds every texture and palette of the material from `file`; true when all were found. */
bool ResMat::Bind(ResFile file) {
    u32 num = GetNumResTexPlttInfo();
    u32 numBound = 0;
    for (u32 i = 0; i < num; i++) {
        if (GetResTexPlttInfo(i).Bind(file, GetResTexObj(), GetResTlutObj())) {
            numBound++;
        }
    }
    return numBound == num;
}

NW4R_G3D_RESCOMMON_CTOR(ResTexPlttInfoData)

/* 0x8009733C (0xB0): returns texture-palette record `idx`. */
ResTexPlttInfo ResMat::GetResTexPlttInfo(u32 idx) {
    RESMAT_ASSERT(idx < GetNumResTexPlttInfo(), 726, "idx < GetNumResTexPlttInfo()");
    ResTexPlttInfoData* p = ofs_to_ptr<ResTexPlttInfoData>(ref().toResTexPlttInfo);
    if (p == NULL) {
        nw4r::db::Panic(RESMAT_FILE, 728, "NW4R:Pointer must not be NULL (p)");
    }
    return ResTexPlttInfo(&p[idx]);
}

NW4R_G3D_RESOURCE_CTOR_ALIGNED(ResTexPlttInfo, RESMAT_AC_FILE, 560)

/* 0x80097474 (0x24): returns the number of texture-palette records. */
u32 ResMat::GetNumResTexPlttInfo() const {
    return ref().numResTexPlttInfo;
}

/* 0x80097498 (0x8C): unbinds every texture and palette of the material. */
void ResMat::Release() {
    u32 num = GetNumResTexPlttInfo();
    for (u32 i = 0; i < num; i++) {
        GetResTexPlttInfo(i).Release(GetResTexObj(), GetResTlutObj());
    }
}

/* 0x80097524 (0x28): stores the material's display lists out of the data cache. */
void ResMat::Init() {
    DC::StoreRangeNoSync(GetResMatDLData(), 0x180);
}

/* 0x8009754C (0x3C): returns the material's tev. */
ResTev ResMat::GetResTev() {
    return ofs_to_obj<ResTev>(ref().toResTevData);
}

/* 0x800975D4 (0x3C): returns the material's fur parameters. */
ResMatFur ResMat::GetResMatFur() {
    return ofs_to_obj<ResMatFur>(ref().toResMatFurData);
}

/* 0x8009765C (0x1A8): binds `tex` to the record's texture map, initialising its texture object. */
void ResTexPlttInfo::BindTex(ResTex tex, ResTexObj texObj) {
    ResTexPlttInfoData& r = ref();
    r.pTexData = const_cast<ResTexData*>(&static_cast<const ResTex&>(tex).ref());
    bool inRange = (s32)ref().mapID >= 0 && (s32)ref().mapID <= 7;
    if (!inRange) {
        nw4r::db::Panic(RESMAT_FILE, 2959, "ref().mapID is out of bounds(%d)\n%d <= ref().mapID <= %d not satisfied.",
                        ref().mapID, 0, 7);
    }
    GXTexObj* pObj = texObj.GetTexObj(static_cast<_GXTexMapID>(r.mapID));
    void* pImage;
    u16 width;
    u16 height;
    u32 format;
    f32 minLod;
    f32 maxLod;
    u8 mipmap;
    if (IsCIFmt()) {
        tex.GetTexObjCIParam(&pImage, &width, &height, &format, &minLod, &maxLod, &mipmap);
        GXInitTexObjCI(pObj, pImage, width, height, format, r.wrap_s, r.wrap_t, mipmap, r.tlutID);
    } else {
        tex.GetTexObjParam(&pImage, &width, &height, &format, &minLod, &maxLod, &mipmap);
        GXInitTexObj(pObj, pImage, width, height, format, r.wrap_s, r.wrap_t, mipmap);
    }
    GXInitTexObjLOD(pObj, r.min_filt, r.mag_filt, minLod, maxLod, r.lod_bias, r.bias_clamp, r.do_edge_lod,
                    r.max_aniso);
    texObj.Validate(static_cast<_GXTexMapID>(r.mapID));
}

/* 0x80097804 (0x30): tells whether the record names a palette (a colour-indexed texture). */
bool ResTexPlttInfo::IsCIFmt() const {
    return ref().namePltt != 0;
}

NW4R_G3D_RESOURCE_REF_CONST(ResTexPlttInfo, RESMAT_AC_FILE, 560)
NW4R_G3D_RESOURCE_PTR_CONST(ResTexPlttInfo)
NW4R_G3D_RESOURCE_CLASS_NAME(ResTexPlttInfo)
NW4R_G3D_RESOURCE_IS_VALID(ResTexPlttInfo)
NW4R_G3D_RESOURCE_REF(ResTexPlttInfo, RESMAT_AC_FILE, 560)
NW4R_G3D_RESOURCE_PTR(ResTexPlttInfo)

/* 0x8009792C (0xD8): binds `pltt` to the record's TLUT, initialising its TLUT object. */
void ResTexPlttInfo::BindPltt(ResPltt pltt, ResTlutObj tlutObj) {
    ResTexPlttInfoData& r = ref();
    r.pPlttData = const_cast<ResPlttData*>(&static_cast<const ResPltt&>(pltt).ref());
    bool inRange = r.mapID >= 0 && r.mapID <= 7;
    if (!inRange) {
        nw4r::db::Panic(RESMAT_FILE, 3028, "r.mapID is out of bounds(%d)\n%d <= r.mapID <= %d not satisfied.",
                        r.mapID, 0, 7);
    }
    GXTlutObj* pTlut = tlutObj.GetTlut(static_cast<_GXTlut>(r.mapID));
    u16 numEntries = pltt.GetNumEntries();
    u32 format = pltt.GetFmt();
    GXInitTlutObj(pTlut, static_cast<const ResPltt&>(pltt).GetPlttData(), format, numEntries);
    tlutObj.Validate(static_cast<_GXTlut>(r.mapID));
}

/* 0x80097A04 (0x38): returns the palette's colour data, or NULL. */
/* untyped: byte range */
void* ResPltt::GetPlttData() const {
    const ResPlttData& r = ref();
    return r.toPlttData != 0 ? (u8*)&r + r.toPlttData : NULL;
}

/* 0x80097A3C (0x1AC): binds the record's texture and palette from `file`; false when one is missing. */
bool ResTexPlttInfo::Bind(ResFile file, ResTexObj texObj, ResTlutObj tlutObj) {
    bool success = true;
    ResName texName = GetTexName();
    RESMAT_ASSERT(texName.IsValid(), 3061, "texName.IsValid()");
    if (ref().pTexData == NULL) {
        ResTex tex = file.GetResTex(texName);
        if (tex.IsValid() && (!IsCIFmt() || tex.IsCIFmt())) {
            BindTex(tex, texObj);
        } else {
            success = false;
        }
    }
    if (ref().pPlttData == NULL) {
        ResName plttName = GetPlttName();
        if (plttName.IsValid()) {
            ResPltt pltt = file.GetResPltt(plttName);
            if (pltt.IsValid()) {
                BindPltt(pltt, tlutObj);
            } else {
                success = false;
            }
        }
    }
    return success;
}

/* 0x80097BE8 (0x58): returns the palette's name. */
ResName ResTexPlttInfo::GetPlttName() const {
    const ResTexPlttInfoData& r = ref();
    if (r.namePltt != 0) {
        return ResName((u8*)&r + r.namePltt - 4);
    }
    return ResName(NULL);
}

/* 0x80097C40 (0x58): returns the texture's name. */
ResName ResTexPlttInfo::GetTexName() const {
    const ResTexPlttInfoData& r = ref();
    if (r.nameTex != 0) {
        return ResName((u8*)&r + r.nameTex - 4);
    }
    return ResName(NULL);
}

/* 0x80097C98 (0xA8): unbinds the record's texture and palette. */
void ResTexPlttInfo::Release(ResTexObj texObj, ResTlutObj tlutObj) {
    ResTexPlttInfoData& r = ref();
    r.pTexData = NULL;
    r.pPlttData = NULL;
    bool inRange = r.mapID >= 0 && r.mapID <= 7;
    if (!inRange) {
        nw4r::db::Panic(RESMAT_FILE, 3116, "r.mapID is out of bounds(%d)\n%d <= r.mapID <= %d not satisfied.",
                        r.mapID, 0, 7);
    }
    texObj.Invalidate(static_cast<_GXTexMapID>(r.mapID));
    tlutObj.Invalidate(static_cast<_GXTlut>(r.mapID));
}

/* 0x80097D40 (0x58): returns byte code `pName`, or NULL. */
const u8* ResMdl::GetResByteCode(const char* pName) const {
    return static_cast<const u8*>(ofs_to_obj<ResDic>(ref().toResByteCodeDic)[pName]);
}

/* 0x80097DE4 (0x68): returns node `pName`. */
ResNode ResMdl::GetResNode(const char* pName) const {
    return ResNode(ofs_to_obj<ResDic>(ref().toResNodeDic)[pName]);
}

/* 0x80097E4C (0x64): returns node `name`. */
ResNode ResMdl::GetResNode(ResName name) const {
    return ResNode(ofs_to_obj<ResDic>(ref().toResNodeDic)[name]);
}

/* 0x80097EB0 (0x68): returns node `idx`. */
ResNode ResMdl::GetResNode(int idx) const {
    return ResNode(ofs_to_obj<ResDic>(ref().toResNodeDic)[idx]);
}

/* 0x80097F18 (0x68): returns node `idx`. */
ResNode ResMdl::GetResNode(u32 idx) const {
    return ResNode(ofs_to_obj<ResDic>(ref().toResNodeDic)[(int)idx]);
}

/* 0x80097F80 (0x48): returns the number of nodes. */
u32 ResMdl::GetResNodeNumEntries() const {
    return ofs_to_obj<ResDic>(ref().toResNodeDic).GetNumData();
}

/* 0x80097FC8 (0x68): returns vertex-position array `idx`. */
ResVtxPos ResMdl::GetResVtxPos(int idx) const {
    return ResVtxPos(ofs_to_obj<ResDic>(ref().toResVtxPosDic)[idx]);
}

/* 0x80098030 (0x68): returns vertex-position array `idx`. */
ResVtxPos ResMdl::GetResVtxPos(u32 idx) const {
    return ResVtxPos(ofs_to_obj<ResDic>(ref().toResVtxPosDic)[(int)idx]);
}

/* 0x80098098 (0x48): returns the number of vertex-position arrays. */
u32 ResMdl::GetResVtxPosNumEntries() const {
    return ofs_to_obj<ResDic>(ref().toResVtxPosDic).GetNumData();
}

/* 0x800980E0 (0x68): returns vertex-normal array `idx`. */
ResVtxNrm ResMdl::GetResVtxNrm(int idx) const {
    return ResVtxNrm(ofs_to_obj<ResDic>(ref().toResVtxNrmDic)[idx]);
}

/* 0x80098148 (0x68): returns vertex-normal array `idx`. */
ResVtxNrm ResMdl::GetResVtxNrm(u32 idx) const {
    return ResVtxNrm(ofs_to_obj<ResDic>(ref().toResVtxNrmDic)[(int)idx]);
}

/* 0x800981B0 (0x48): returns the number of vertex-normal arrays. */
u32 ResMdl::GetResVtxNrmNumEntries() const {
    return ofs_to_obj<ResDic>(ref().toResVtxNrmDic).GetNumData();
}

/* 0x800981F8 (0x68): returns vertex-colour array `idx`. */
ResVtxClr ResMdl::GetResVtxClr(int idx) const {
    return ResVtxClr(ofs_to_obj<ResDic>(ref().toResVtxClrDic)[idx]);
}

/* 0x80098260 (0x68): returns vertex-colour array `idx`. */
ResVtxClr ResMdl::GetResVtxClr(u32 idx) const {
    return ResVtxClr(ofs_to_obj<ResDic>(ref().toResVtxClrDic)[(int)idx]);
}

/* 0x800982C8 (0x48): returns the number of vertex-colour arrays. */
u32 ResMdl::GetResVtxClrNumEntries() const {
    return ofs_to_obj<ResDic>(ref().toResVtxClrDic).GetNumData();
}

NW4R_G3D_RESCOMMON_CTOR(ResVtxTexCoordData)
NW4R_G3D_RESOURCE_CTOR_ALIGNED(ResVtxTexCoord, "g3d_resvtx_ac.h", 211)

/* 0x8009837C (0x68): returns texture-coordinate array `idx`. */
ResVtxTexCoord ResMdl::GetResVtxTexCoord(int idx) const {
    return ResVtxTexCoord(ofs_to_obj<ResDic>(ref().toResVtxTexCoordDic)[idx]);
}

/* 0x800983E4 (0x68): returns texture-coordinate array `idx`. */
ResVtxTexCoord ResMdl::GetResVtxTexCoord(u32 idx) const {
    return ResVtxTexCoord(ofs_to_obj<ResDic>(ref().toResVtxTexCoordDic)[(int)idx]);
}

/* 0x8009844C (0x48): returns the number of texture-coordinate arrays. */
u32 ResMdl::GetResVtxTexCoordNumEntries() const {
    return ofs_to_obj<ResDic>(ref().toResVtxTexCoordDic).GetNumData();
}

NW4R_G3D_RESCOMMON_CTOR(ResVtxFurVecData)
NW4R_G3D_RESOURCE_CTOR_ALIGNED(ResVtxFurVec, "g3d_resvtx_ac.h", 270)

/* 0x80098500 (0x68): returns fur-direction array `idx`. */
ResVtxFurVec ResMdl::GetResVtxFurVec(u32 idx) const {
    return ResVtxFurVec(ofs_to_obj<ResDic>(ref().toResVtxFurVecDic)[(int)idx]);
}

/* 0x80098568 (0x48): returns the number of fur-direction arrays. */
u32 ResMdl::GetResVtxFurVecNumEntries() const {
    return ofs_to_obj<ResDic>(ref().toResVtxFurVecDic).GetNumData();
}

NW4R_G3D_RESCOMMON_CTOR(ResVtxFurPosData)
NW4R_G3D_RESOURCE_CTOR_ALIGNED(ResVtxFurPos, "g3d_resvtx_ac.h", 318)

/* 0x8009861C (0x68): returns fur-position array `idx`. */
ResVtxFurPos ResMdl::GetResVtxFurPos(int idx) const {
    return ResVtxFurPos(ofs_to_obj<ResDic>(ref().toResVtxFurPosDic)[idx]);
}

/* 0x80098684 (0x68): returns fur-position array `idx`. */
ResVtxFurPos ResMdl::GetResVtxFurPos(u32 idx) const {
    return ResVtxFurPos(ofs_to_obj<ResDic>(ref().toResVtxFurPosDic)[(int)idx]);
}

/* 0x800986EC (0x48): returns the number of fur-position arrays. */
u32 ResMdl::GetResVtxFurPosNumEntries() const {
    return ofs_to_obj<ResDic>(ref().toResVtxFurPosDic).GetNumData();
}

/* 0x80098734 (0x64): returns material `name`. */
ResMat ResMdl::GetResMat(ResName name) const {
    return ResMat(ofs_to_obj<ResDic>(ref().toResMatDic)[name]);
}

/* 0x80098798 (0x68): returns material `idx`. */
ResMat ResMdl::GetResMat(int idx) const {
    return ResMat(ofs_to_obj<ResDic>(ref().toResMatDic)[idx]);
}

/* 0x80098800 (0x68): returns material `idx`. */
ResMat ResMdl::GetResMat(u32 idx) const {
    return ResMat(ofs_to_obj<ResDic>(ref().toResMatDic)[(int)idx]);
}

/* 0x80098868 (0x48): returns the number of materials. */
u32 ResMdl::GetResMatNumEntries() const {
    return ofs_to_obj<ResDic>(ref().toResMatDic).GetNumData();
}

/* 0x800988B0 (0x68): returns shape `idx`. */
ResShp ResMdl::GetResShp(int idx) const {
    return ResShp(ofs_to_obj<ResDic>(ref().toResShpDic)[idx]);
}

/* 0x80098918 (0x68): returns shape `idx`. */
ResShp ResMdl::GetResShp(u32 idx) const {
    return ResShp(ofs_to_obj<ResDic>(ref().toResShpDic)[(int)idx]);
}

/* 0x80098980 (0x48): returns the number of shapes. */
u32 ResMdl::GetResShpNumEntries() const {
    return ofs_to_obj<ResDic>(ref().toResShpDic).GetNumData();
}

/* 0x800989C8 (0x98): binds every material's textures from `file`; true when all were found. */
bool ResMdl::Bind(const ResFile& file) {
    bool success = true;
    u32 num = GetResMatNumEntries();
    for (u32 i = 0; i < num; i++) {
        bool ok = false;
        if (GetResMat(i).Bind(file) && success) {
            ok = true;
        }
        success = ok;
    }
    return success;
}

/* 0x80098A60 (0x6C): unbinds every material's textures. */
void ResMdl::Release() {
    u32 num = GetResMatNumEntries();
    for (u32 i = 0; i < num; i++) {
        GetResMat(i).Release();
    }
}

/* 0x80098ACC (0x1F4): stores every material, shape and vertex array out of the data cache. */
void ResMdl::Init() {
    u32 i;
    u32 num;

    num = GetResMatNumEntries();
    for (i = 0; i < num; i++) {
        GetResMat(i).Init();
    }
    num = GetResShpNumEntries();
    for (i = 0; i < num; i++) {
        GetResShp(i).Init();
    }
    num = GetResVtxPosNumEntries();
    for (i = 0; i < num; i++) {
        GetResVtxPos(i).Init();
    }
    num = GetResVtxNrmNumEntries();
    for (i = 0; i < num; i++) {
        GetResVtxNrm(i).Init();
    }
    num = GetResVtxClrNumEntries();
    for (i = 0; i < num; i++) {
        GetResVtxClr(i).Init();
    }
    num = GetResVtxTexCoordNumEntries();
    for (i = 0; i < num; i++) {
        GetResVtxTexCoord(i).Init();
    }
    num = GetResVtxFurVecNumEntries();
    for (i = 0; i < num; i++) {
        GetResVtxFurVec(i).Init();
    }
    num = GetResVtxFurPosNumEntries();
    for (i = 0; i < num; i++) {
        GetResVtxFurPos(i).Init();
    }
}

/* 0x80098CC0 (0x8): stores the block out of the data cache without waiting. */
void ResVtxFurPos::Init() {
    DCStore(false);
}

/* 0x80098CC8 (0x8): stores the block out of the data cache without waiting. */
void ResVtxFurVec::Init() {
    DCStore(false);
}

/* 0x80098CD0 (0x8): stores the block out of the data cache without waiting. */
void ResVtxTexCoord::Init() {
    DCStore(false);
}

/* 0x80098CD8 (0x8): stores the block out of the data cache without waiting. */
void ResVtxClr::Init() {
    DCStore(false);
}

/* 0x80098CE0 (0x8): stores the block out of the data cache without waiting. */
void ResVtxNrm::Init() {
    DCStore(false);
}

/* 0x80098CE8 (0x8): stores the block out of the data cache without waiting. */
void ResVtxPos::Init() {
    DCStore(false);
}

/* 0x80098CF0 (0x6C): releases every shape's runtime state. */
void ResMdl::Terminate() {
    u32 num = GetResShpNumEntries();
    for (u32 i = 0; i < num; i++) {
        GetResShp(i).Terminate();
    }
}

}  // namespace g3d
}  // namespace nw4r
