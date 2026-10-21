/*
 * g3d/g3d_draw1mat1shp.cpp - nw4r g3d's one-material/one-shape draw: the material state loader (SetupMaterial),
 *   Draw1Mat1ShpDirectly (the plain and the fur-layer draw of one shape) and the ResMat/ResShp/ResVtxFurPos/ResMatFur
 *   handle members it pulls in, the GD XF writer and FPow.
 * RANGE. .text 0x800761BC-0x800777B0 (55 functions); extab 0x80008210-0x800082C8, extabindex 0x80020940-0x80020A54,
 *   .rodata 0x8056F658-0x8056F668 (the fur pass's channel table), .data 0x8058E608-0x8058E880 (opens on
 *   "g3d_draw1mat1shp.cpp"; the handle members' `_ac.h` asserts follow), .sdata 0x807911E0-0x807911F8 (the
 *   "ref"/"ResShp" names the handle asserts print), .sdata2 0x80795E00-0x80795E18 (a 1.0 pooled again after
 *   `g3d/g3d_camera.cpp`'s 0x80795DD0).  SetupMaterial and the indirect-matrix operation's constructor are called
 *   only from Draw1Mat1ShpDirectly's chain, so the TU opens at 0x800761BC; `g3d/g3d_draw.cpp` opens on
 *   "g3d_draw.cpp" and a second pooled 1.0 at 0x800777B0.
 * FLAGS. cflags_g3d (docs/g3d.md); `#pragma peephole off` (playbook idea 106); `#pragma dont_inline on` (the
 *   accessors and the compiler-generated copies are called out of line, as retail does); `#pragma fp_contract off`
 *   around Draw1Mat1ShpDirectly (retail keeps `fmuls` + `fadds` for the fur colour blends).
 * NAMES. nw4r's where the map, the asserts or the SDK carry them (Draw1Mat1ShpDirectly, Draw1Mat1ShpSwap's
 *   vtx*Table fields, GDWrite_u8/u16/u32, GDSetCullMode).  GUESS: SetupMaterial (0x800761BC),
 *   g3d_ind_mtx_op_ctor (0x8007663C), g3d_gd_set_chan_mat_color (XF 0x100C plus the channel), GDWriteXFCmd,
 *   ResVtxFurPos::GetNumLayer (0x800774A4), ResMatFurData's alphaPowExp/colorPowExp.
 * RESIDUALS. Draw1Mat1ShpDirectly: the parameters' registers (r22.. for retail's r26..) and the by-value
 *   temporaries' stack slots (8 bytes lower): retail's fur position handle is a temporary copied by the copy
 *   constructor (0x38), ours a named word from res_shp_get_vtx_fur_pos (a `const u32&` does not move it).
 *   The copy constructors of the swap's handles are this TU's own (retail calls other units' kept copies): relocation names only.  The indirect-matrix
 *   callback and its operation are `g3d/g3d_state.cpp`'s types, declared here until that unit's header carries
 *   them (g3d-draw#3); STOPGAPs g3d-draw#4..#6.  Data: literals emitted, order and the inline copies' separate
 *   strings not yet retail's; flipcheck is not ready.
 * SHAPES. SetupMaterial copies the loaded ambient colour member by member into a colour declared before it and
 *   passes the channel's light flag as `(ctrl & 8) ? true : false` (evaluated first, as retail).
 *   Draw1Mat1ShpDirectly nests the draw under `shp.IsValid() && shp.IsVisible()`, declares the fur pass's arrays and
 *   colours before its handles (MWCC orders a frame by size, then declaration) and names the indirect stage id once.
 */

#include "types.h"
#include "gx.h"
#include "OS/PPCArch.h"            /* PPCSync (rule 2) */
#include "RVLGX/GXTev.h"           /* GXSetTevKColor (rule 2) */
#include "RVLGX/GXBump.h"          /* GXSetIndTexMtx (rule 2) */
#include "fn_8004CAD8/mtx.h"       /* MTX34_ctor (rule 2) */
#include "nw4r/db_assert.h"        /* nw4r::db::Panic (rule 2) */
#include "g3d/g3d_calcview.h"      /* g3d_lc_queue_wait (rule 2) */
#include "g3d/g3d_resvtx.h"
#include "g3d/g3d_resmat.h"
#include "g3d/g3d_resshp.h"
#include "g3d/g3d_state.h"         /* the material loaders (rule 2) */
#include "g3d/g3d_ind_mtx_callback.h" /* G3dIndMtxOp, G3dIndMtxCallback (rule 1) */
#include "gx/fn_8009AA78.h"       /* GDSetCullMode (rule 2) */
#include "g3d/g3d_draw1mat1shp.h"

#pragma peephole off
#pragma dont_inline on

extern "C" {
u32 g3d_state_load_tex_srt(nw4r::g3d::ResTexSrt texSrt);
f32 powf(f32 x, f32 y);

}


extern "C" {
G3dIndMtxOp* g3d_ind_mtx_op_ctor(G3dIndMtxOp* pOp);
const nw4r::g3d::ResMatFurData* res_mat_fur_ref(const nw4r::g3d::ResMatFur* pFur);
const nw4r::g3d::ResMatFurData* res_mat_fur_ptr(const nw4r::g3d::ResMatFur* pFur);
void GDWrite_u32(u32 value);
void GDWrite_u16(u16 value);
void GDWrite_u8(u8 value);
}

namespace nw4r {
namespace g3d {

/* 0x800761BC (0x480): loads the material's state (or the swap's replacements) unless the material is skipped, in
 * which case only the indirect matrices are offered to the callback. */
static void SetupMaterial(ResMat mat, u32 ctrl, Draw1Mat1ShpSwap* pSwap, G3dIndMtxCallback* pIndMtxOp,
                          bool bIgnoreMaterial) {
    if (!bIgnoreMaterial) {
        ResMatMisc misc(NULL);
        if (pSwap == NULL || !pSwap->misc.IsValid()) {
            misc = mat.GetResMatMisc();
        } else {
            misc = pSwap->misc;
        }
        g3d_state_load_fog(misc.GetFogIdx());
        g3d_state_set_mat_misc(misc);

        if (pSwap == NULL || !pSwap->tlutObj.IsValid()) {
            g3d_state_load_tlut_obj(mat.GetResTlutObj());
        } else {
            g3d_state_load_tlut_obj(pSwap->tlutObj);
        }
        if (pSwap == NULL || !pSwap->texObj.IsValid()) {
            g3d_state_load_tex_obj(mat.GetResTexObj());
        } else {
            g3d_state_load_tex_obj(pSwap->texObj);
        }
        if (pSwap == NULL || !pSwap->genMode.IsValid()) {
            g3d_state_set_gen_mode(mat.GetResGenMode());
        } else {
            g3d_state_set_gen_mode(pSwap->genMode);
        }
        if (pSwap == NULL || !pSwap->tev.IsValid()) {
            g3d_state_load_tev(mat.GetResTev());
        } else {
            g3d_state_load_tev(pSwap->tev);
        }
        if (pSwap == NULL || !pSwap->pix.IsValid()) {
            g3d_state_load_mat_pix(mat.GetResMatPix());
        } else {
            g3d_state_load_mat_pix(pSwap->pix);
        }
        if (pSwap == NULL || !pSwap->tevColor.IsValid()) {
            g3d_state_load_mat_tev_color(mat.GetResMatTevColor());
        } else {
            g3d_state_load_mat_tev_color(pSwap->tevColor);
        }
        if (pSwap == NULL || !pSwap->indMtxAndScale.IsValid()) {
            if (pIndMtxOp != NULL) {
                g3d_state_load_mat_ind_mtx(mat.GetResMatIndMtxAndScale(), pIndMtxOp);
            } else {
                g3d_state_load_mat_ind_mtx_dl(mat.GetResMatIndMtxAndScale());
            }
        } else if (pIndMtxOp != NULL) {
            g3d_state_load_mat_ind_mtx(pSwap->indMtxAndScale, pIndMtxOp);
        } else {
            g3d_state_load_mat_ind_mtx_dl(pSwap->indMtxAndScale);
        }

        u32 maskDiffColor;
        u32 maskDiffAlpha;
        u32 maskSpecColor;
        u32 maskSpecAlpha;
        GXColor amb;
        GXColor ambLoaded;
        g3d_state_load_light_set(misc.GetLightSetIdx(), &maskDiffColor, &maskDiffAlpha, &maskSpecColor,
                                 &maskSpecAlpha, &ambLoaded);
        amb.r = ambLoaded.r;
        amb.g = ambLoaded.g;
        amb.b = ambLoaded.b;
        amb.a = ambLoaded.a;
        if (pSwap == NULL || !pSwap->chan.IsValid()) {
            g3d_state_load_mat_chan(mat.GetResMatChan(), maskDiffColor, maskDiffAlpha, maskSpecColor, maskSpecAlpha,
                                    amb, (ctrl & 8) ? true : false);
        } else {
            g3d_state_load_mat_chan(pSwap->chan, maskDiffColor, maskDiffAlpha, maskSpecColor, maskSpecAlpha, amb,
                                    (ctrl & 8) ? true : false);
        }
        if (pSwap == NULL || !pSwap->texCoordGen.IsValid()) {
            g3d_state_load_tex_coord_gen(mat.GetResMatTexCoordGen());
        } else {
            g3d_state_load_tex_coord_gen(pSwap->texCoordGen);
        }
        if (pSwap == NULL || !pSwap->texSrt.IsValid()) {
            g3d_state_load_tex_srt(mat.GetResTexSrt());
        } else {
            g3d_state_load_tex_srt(pSwap->texSrt);
        }
    } else if (pIndMtxOp != NULL) {
        if (pSwap == NULL || !pSwap->indMtxAndScale.IsValid()) {
            G3dIndMtxOp op;
            g3d_ind_mtx_op_ctor(&op);
            pIndMtxOp->Exec(&op);
            g3d_ind_mtx_op_load(&op);
        } else {
            g3d_state_load_mat_ind_mtx(pSwap->indMtxAndScale, pIndMtxOp);
        }
    }
}

}  // namespace g3d
}  // namespace nw4r

/* 0x8007663C (0x60): clears the operation's flags and constructs its three matrices. */
G3dIndMtxOp* g3d_ind_mtx_op_ctor(G3dIndMtxOp* pOp) {
    pOp->flags = 0;
    nw4r::math::MTX34* pMtx = pOp->mtx;
    do {
        MTX34_ctor(pMtx);
        pMtx++;
    } while (pMtx < pOp->mtx + 3);
    return pOp;
}

/* 0x8007669C (0x34): returns the material's MatTexCoordGen block. */
nw4r::g3d::ResMatTexCoordGen nw4r::g3d::ResMat::GetResMatTexCoordGen() {
    return ResMatTexCoordGen((u8*)GetResMatDLData() + 0xE0);
}

/* 0x800766D0 (0x64): wraps `pData`, asserting its alignment. */
/* untyped: opaque handle - the block address */
nw4r::g3d::ResMatTexCoordGen::ResMatTexCoordGen(void* pData) : ResCommon<ResMatTexCoordGenData>(pData) {
    if ((u32)pData & 0x1F) {
        nw4r::db::Panic("g3d_resmat_ac.h", 0x201, "NW4R:Failed assertion !((u32)p & 0x1f)");
    }
}

/* 0x80076734 (0x8): stores the block address. */
NW4R_G3D_RESCOMMON_CTOR(nw4r::g3d::ResMatTexCoordGenData)

/* 0x8007673C (0x14): whether the handle is set. */
bool nw4r::g3d::ResMatTexCoordGen::IsValid() const {
    return mpData != NULL;
}

/* 0x80076750 (0x14): whether the handle is set. */
bool nw4r::g3d::ResMatTevColor::IsValid() const {
    return mpData != NULL;
}

/* 0x80076764 (0x30): returns the material's MatPix block. */
nw4r::g3d::ResMatPix nw4r::g3d::ResMat::GetResMatPix() {
    return ResMatPix(GetResMatDLData());
}

/* 0x80076794 (0x64): wraps `pData`, asserting its alignment. */
/* untyped: opaque handle - the block address */
nw4r::g3d::ResMatPix::ResMatPix(void* pData) : ResCommon<ResMatPixData>(pData) {
    if ((u32)pData & 0x1F) {
        nw4r::db::Panic("g3d_resmat_ac.h", 0x154, "NW4R:Failed assertion !((u32)p & 0x1f)");
    }
}

/* 0x800767F8 (0x8): stores the block address. */
NW4R_G3D_RESCOMMON_CTOR(nw4r::g3d::ResMatPixData)

/* 0x80076800 (0x14): tells whether the handle is set. */
bool nw4r::g3d::ResMatPix::IsValid() const {
    return mpData != NULL;
}

/* 0x80076814 (0x14): tells whether the handle is set. */
bool nw4r::g3d::ResTev::IsValid() const {
    return mpData != NULL;
}

/* 0x80076828 (0x34): returns the material's GenMode block. */
nw4r::g3d::ResGenMode nw4r::g3d::ResMat::GetResGenMode() {
    return ResGenMode((u8*)&ref() + 0x14);
}

/* 0x8007685C (0x64): wraps `pData`, asserting its alignment. */
/* untyped: opaque handle - the block address */
nw4r::g3d::ResGenMode::ResGenMode(void* pData) : ResCommon<ResGenModeData>(pData) {
    if ((u32)pData & 3) {
        nw4r::db::Panic("g3d_resmat_ac.h", 0xAF, "NW4R:Failed assertion !((u32)p & 0x3)");
    }
}

/* 0x800768C0 (0x8): stores the block address. */
NW4R_G3D_RESCOMMON_CTOR(nw4r::g3d::ResGenModeData)

/* 0x800768C8 (0x14): tells whether the handle is set. */
bool nw4r::g3d::ResGenMode::IsValid() const {
    return mpData != NULL;
}

/* 0x800768DC (0x14): tells whether the handle is set. */
bool nw4r::g3d::ResTexObj::IsValid() const {
    return mpData != NULL;
}

/* 0x800768F0 (0x14): tells whether the handle is set. */
bool nw4r::g3d::ResTlutObj::IsValid() const {
    return mpData != NULL;
}

/* 0x80076904 (0x30): copies the handle. */
nw4r::g3d::ResMatMisc& nw4r::g3d::ResMatMisc::operator=(const ResMatMisc& rhs) {
    ResCommon<ResMatMiscData>::operator=(rhs);
    return *this;
}

/* 0x80076940 (0x34): returns the material's MatMisc block. */
nw4r::g3d::ResMatMisc nw4r::g3d::ResMat::GetResMatMisc() {
    return ResMatMisc((u8*)&ref() + 0x1C);
}

/* 0x80076974 (0x14): tells whether the handle is set. */
bool nw4r::g3d::ResMatMisc::IsValid() const {
    return mpData != NULL;
}

/* 0x80076988 (0x64): wraps `pData`, asserting its alignment. */
/* untyped: opaque handle - the block address */
nw4r::g3d::ResMatMisc::ResMatMisc(void* pData) : ResCommon<ResMatMiscData>(pData) {
    if ((u32)pData & 3) {
        nw4r::db::Panic("g3d_resmat_ac.h", 0xF3, "NW4R:Failed assertion !((u32)p & 0x3)");
    }
}

/* 0x800769EC (0x8): stores the block address. */
NW4R_G3D_RESCOMMON_CTOR(nw4r::g3d::ResMatMiscData)

#pragma fp_contract off
/* 0x800769F4 (0x9A4): draws one shape with one material: the material state first, then either every fur layer
 * (each with its layer ratio's colours and indirect matrices) or the shape once, through the swap's vertex tables
 * when it has them. */
void nw4r::g3d::Draw1Mat1ShpDirectly(ResMat mat, ResShp shp, const math::MTX34* pViewPos, const math::MTX34* pViewNrm,
                                     u32 ctrl, Draw1Mat1ShpSwap* pSwap, G3dIndMtxCallback* pIndMtxOp) {
    if (!(ctrl & 1)) {
        g3d_lc_queue_wait(0);
        PPCSync();
    }

    bool bIgnoreMaterial = false;
    bool bFur = false;
    ResMatFur fur(NULL);
    if (mat.IsValid()) {
        if (ctrl & 0x10) {
            bIgnoreMaterial = true;
        }
        fur = mat.GetResMatFur();
        if (fur.IsValid()) {
            bFur = true;
        }
    } else {
        bIgnoreMaterial = true;
    }
    SetupMaterial(mat, ctrl, pSwap, pIndMtxOp, bIgnoreMaterial);

    if (shp.IsValid() && shp.IsVisible()) {
        if (bFur) {
            f32 alphaPowExp = res_mat_fur_ref(&fur)->alphaPowExp;
            f32 colorPowExp = res_mat_fur_ref(&fur)->colorPowExp;
            math::MTX34 indMtx[3];
            MTX34_ctor(&indMtx[0]);
            MTX34_ctor(&indMtx[1]);
            MTX34_ctor(&indMtx[2]);
            u32 matSrc[4];
            u32 attnFn[4];
            /* The channel order the colour controls are read in: colour 0, alpha 0, colour 1, alpha 1. */
            const u32 chanID[4] = {0, 2, 1, 3};
            GXColor matColor[2];
            GXColor kcolor0;
            GXColor kcolor1;
            GXColor kcolor2;
            u8 furInd[3];
            s8 indScaleExp[3];

            bool useMat = true;
            bool noSwap = pSwap == NULL;
            if (!noSwap && pSwap->tevColor.IsValid()) {
                useMat = false;
            }
            ResMatTevColor tevColor(useMat ? mat.GetResMatTevColor() : pSwap->tevColor);
            tevColor.GXGetTevKColor(GX_KCOLOR0, &kcolor0);
            tevColor.GXGetTevKColor(GX_KCOLOR1, &kcolor1);
            tevColor.GXGetTevKColor(GX_KCOLOR2, &kcolor2);
            u8 kcolor2G = kcolor2.g;

            useMat = false;
            if (noSwap || !pSwap->chan.IsValid()) {
                useMat = true;
            }
            ResMatChan chan(useMat ? mat.GetResMatChan() : pSwap->chan);
            chan.GXGetChanMatColor(0, &matColor[0]);
            chan.GXGetChanMatColor(1, &matColor[1]);
            for (u8 i = 0; i < 4; i++) {
                chan.GXGetChanCtrl(chanID[i], NULL, NULL, &matSrc[i], NULL, NULL, &attnFn[i]);
            }

            useMat = false;
            if (noSwap || !pSwap->misc.IsValid()) {
                useMat = true;
            }
            ResMatMisc misc(useMat ? mat.GetResMatMisc() : pSwap->misc);
            useMat = false;
            if (noSwap || !pSwap->indMtxAndScale.IsValid()) {
                useMat = true;
            }
            ResMatIndMtxAndScale ind(useMat ? mat.GetResMatIndMtxAndScale() : pSwap->indMtxAndScale);

            for (u32 i = 0; i < 3; i++) {
                ResMatMiscData::IndirectMethod method;
                s8 normalMapRef;
                _GXIndTexMtxID id = (_GXIndTexMtxID)(i + 1);
                misc.GetIndirectMethod(id, &method, &normalMapRef);
                if (method == ResMatMiscData::FUR) {
                    furInd[i] = 1;
                    ind.GXGetIndTexMtx(id, &indMtx[i], &indScaleExp[i]);
                } else {
                    furInd[i] = 0;
                }
            }

            g3d_state_load_shp_pre_prim(shp);
            if (ctrl & 4) {
                GDSetCullMode(GX_CULL_FRONT);
                g3d_state_invalidate(0x10);
            }

            u32 furPosData = res_shp_get_vtx_fur_pos((ResHandle*)&shp);
            ResVtxFurPos furPos(*(const ResVtxFurPos*)&furPosData);
            u32 numLayer = furPos.GetNumLayer();
            for (u16 layer = 0; layer < numLayer; layer++) {
                f32 ratio = fur.GetLayerRatio(layer);
                f32 invRatio = 1.0f - ratio;
                f32 alphaRatio = math::FPow(ratio, alphaPowExp);
                f32 colorRatio = math::FPow(ratio, colorPowExp);
                f32 kRatio = math::FPow(ratio, 4.0f);
                kcolor2.r = 255.0f * ratio;
                kcolor2.g = kcolor2G * kRatio;
                GXSetTevKColor(GX_KCOLOR2, kcolor2);

                for (u8 i = 0; i < 3; i++) {
                    if (furInd[i]) {
                        f32 m[2][3];
                        m[0][0] = ratio * indMtx[i].m[0][0];
                        m[0][1] = ratio * indMtx[i].m[0][1];
                        m[0][2] = indMtx[i].m[0][2];
                        m[1][0] = ratio * indMtx[i].m[1][0];
                        m[1][1] = ratio * indMtx[i].m[1][1];
                        m[1][2] = indMtx[i].m[1][2];
                        GXSetIndTexMtx(i + 1, m, indScaleExp[i]);
                    }
                }

                u8 furR = kcolor0.r * invRatio + kcolor1.r * ratio;
                u8 furG = kcolor0.g * invRatio + kcolor1.g * ratio;
                u8 furB = kcolor0.b * invRatio + kcolor1.b * ratio;
                u8 furA = kcolor0.a * (1.0f - alphaRatio) + kcolor1.a * alphaRatio;
                for (u8 i = 0; i < 2; i++) {
                    u8 c = i * 2;
                    if (matSrc[c] == 0 || matSrc[c + 1] == 0) {
                        GXColor color;
                        if (attnFn[c] == 0) {
                            color.r = colorRatio * matColor[i].r;
                            color.g = colorRatio * matColor[i].g;
                            color.b = colorRatio * matColor[i].b;
                        } else {
                            color.r = furR;
                            color.g = furG;
                            color.b = furB;
                        }
                        color.a = attnFn[c + 1] == 0 ? matColor[i].a : furA;
                        g3d_gd_set_chan_mat_color(chanID[c], color);
                    }
                }

                furPos.SetArray(layer);
                g3d_state_load_shp_prim(shp, pViewPos, pViewNrm);
            }
        } else {
            if (!(ctrl & 2)) {
                g3d_state_load_shp_pre_prim(shp);
                if (pSwap != NULL) {
                    if (pSwap->vtxPosTable != NULL) {
                        u32 shpPos = res_shp_get_vtx_pos((ResHandle*)&shp);
                        if (((ResVtxPos*)&shpPos)->GetID() !=
                            ResVtxPos(pSwap->vtxPosTable[shp.ref().idVtxPosition]).GetID()) {
                            nw4r::db::Panic("g3d_draw1mat1shp.cpp", 0x19D,
                                            "NW4R:Failed assertion shp.GetResVtxPos().GetID() == "
                                            "ResVtxPos(pSwap->vtxPosTable[shp.ref().idVtxPosition]).GetID()");
                        }
                        ResVtxPos(pSwap->vtxPosTable[shp.ref().idVtxPosition]).SetArray();
                    }
                    if (pSwap->vtxNrmTable != NULL) {
                        s16 idx = shp.ref().idVtxNrm;
                        if (idx >= 0) {
                            ResVtxNrm(pSwap->vtxNrmTable[idx]).SetArray();
                        }
                    }
                    if (pSwap->vtxClrTable != NULL) {
                        s16 idx = shp.ref().idVtxClr[0];
                        if (idx >= 0) {
                            ResVtxClr(pSwap->vtxClrTable[idx]).SetArray(11);
                        }
                        idx = shp.ref().idVtxClr[1];
                        if (idx >= 0) {
                            ResVtxClr(pSwap->vtxClrTable[idx]).SetArray(12);
                        }
                    }
                }
            }
            if (ctrl & 4) {
                GDSetCullMode(GX_CULL_FRONT);
                g3d_state_invalidate(0x10);
            }
            g3d_state_load_shp_prim(shp, pViewPos, pViewNrm);
        }
    }
}

#pragma fp_contract reset

/* 0x80077398 (0x64): returns the shape block, panicking on a NULL handle. */
nw4r::g3d::ResShpData& nw4r::g3d::ResShp::ref() {
    if (!IsValid()) {
        nw4r::db::Panic("g3d_resshp_ac.h", 0x3A, "%s::%s: Object not valid.", GetClassName(), "ref");
    }
    return *ptr();
}

/* 0x800773FC (0x8): returns the shape block. */
nw4r::g3d::ResShpData* nw4r::g3d::ResShp::ptr() {
    return mpData;
}

/* 0x80077404 (0x8): returns the class name. */
const char* nw4r::g3d::ResShp::GetClassName() {
    return "ResShp";
}

extern "C" {

/* 0x8007740C (0x14): writes a colour channel's material colour (XF 0x100C + chan). */
void g3d_gd_set_chan_mat_color(u32 chan, GXColor color) {
    GDWriteXFCmd((chan & 1) + 0x100C, *(const u32*)&color);
}

/* 0x80077420 (0x54): writes one XF register through the pipe. */
void GDWriteXFCmd(u16 addr, u32 value) {
    GDWrite_u8(0x10);
    GDWrite_u16(0);
    GDWrite_u16(addr);
    GDWrite_u32(value);
}

/* 0x80077474 (0xC): writes a word to the pipe. */
void GDWrite_u32(u32 value) {
    *(volatile u32*)0xCC008000 = value;
}

/* 0x80077480 (0x10): writes a halfword to the pipe. */
void GDWrite_u16(u16 value) {
    *(volatile u16*)0xCC008000 = value;
}

/* 0x80077490 (0x10): writes a byte to the pipe. */
void GDWrite_u8(u8 value) {
    *(volatile u8*)0xCC008000 = value;
}

}

/* 0x800774A0 (0x4): raises `x` to the power `y` through the C library. */
f32 nw4r::math::FPow(f32 x, f32 y) {
    return powf(x, y);
}

/* 0x800774A4 (0x24): returns the number of fur layers. */
u32 nw4r::g3d::ResVtxFurPos::GetNumLayer() const {
    return ref().numLayer;
}

/* 0x800774C8 (0x64): returns the fur-position block, panicking on a NULL handle. */
const nw4r::g3d::ResVtxFurPosData& nw4r::g3d::ResVtxFurPos::ref() const {
    if (!IsValid()) {
        nw4r::db::Panic("g3d_resvtx_ac.h", 0x13E, "%s::%s: Object not valid.", GetClassName(), "ref");
    }
    return *ptr();
}

/* 0x8007752C (0x8): returns the fur-position block. */
const nw4r::g3d::ResVtxFurPosData* nw4r::g3d::ResVtxFurPos::ptr() const {
    return mpData;
}

/* 0x80077534 (0xC): returns the class name. */
const char* nw4r::g3d::ResVtxFurPos::GetClassName() {
    return "ResVtxFurPos";
}

/* 0x80077540 (0x14): tells whether the handle is set. */
bool nw4r::g3d::ResVtxFurPos::IsValid() const {
    return mpData != NULL;
}

extern "C" {

/* 0x800775CC (0x64): returns the fur block, panicking on a NULL handle (this unit's copy of ResMatFur::ref). */
const nw4r::g3d::ResMatFurData* res_mat_fur_ref(const nw4r::g3d::ResMatFur* pFur) {
    if (!pFur->IsValid()) {
        nw4r::db::Panic("g3d_resmat_ac.h", 0x11F, "%s::%s: Object not valid.", nw4r::g3d::ResMatFur::GetClassName(),
                        "ref");
    }
    return res_mat_fur_ptr(pFur);
}

/* 0x80077630 (0x8): returns the fur block (this unit's copy of ResMatFur::ptr). */
const nw4r::g3d::ResMatFurData* res_mat_fur_ptr(const nw4r::g3d::ResMatFur* pFur) {
    return pFur->mpData;
}

}

/* 0x80077638 (0xC): returns the class name. */
const char* nw4r::g3d::ResMatFur::GetClassName() {
    return "ResMatFur";
}

/* 0x80077644 (0x30): whether the shape is drawn (its invisible flag is clear). */
bool nw4r::g3d::ResShp::IsVisible() const {
    return (ref().flag & 2) == 0;
}

/* 0x80077674 (0x64): returns the shape block, panicking on a NULL handle. */
const nw4r::g3d::ResShpData& nw4r::g3d::ResShp::ref() const {
    if (!IsValid()) {
        nw4r::db::Panic("g3d_resshp_ac.h", 0x3A, "%s::%s: Object not valid.", GetClassName(), "ref");
    }
    return *ptr();
}

/* 0x800776D8 (0x8): returns the shape block. */
const nw4r::g3d::ResShpData* nw4r::g3d::ResShp::ptr() const {
    return mpData;
}

/* 0x800776E0 (0x14): whether the handle is set. */
bool nw4r::g3d::ResShp::IsValid() const {
    return mpData != NULL;
}

/* 0x800776F4 (0x14): tells whether the handle is set. */
bool nw4r::g3d::ResMatFur::IsValid() const {
    return mpData != NULL;
}

/* 0x80077744 (0x64): wraps `pData`, asserting its alignment. */
/* untyped: opaque handle - the block address */
nw4r::g3d::ResMatFur::ResMatFur(void* pData) : ResCommon<ResMatFurData>(pData) {
    if ((u32)pData & 3) {
        nw4r::db::Panic("g3d_resmat_ac.h", 0x11F, "NW4R:Failed assertion !((u32)p & 0x3)");
    }
}

/* 0x800777A8 (0x8): stores the block address. */
NW4R_G3D_RESCOMMON_CTOR(nw4r::g3d::ResMatFurData)
