/*
 * g3d/g3d_draw.cpp - nw4r g3d's model draw: the indirect-matrix set-up per shape (PrepareIndMtxOp: the normal
 *   matrix and the light direction each normal-mapped indirect stage needs), the replacement material refill
 *   (SetupDraw1Mat1ShpSwap), the byte-code and depth-sorted draw passes over a ResMdl, g3d_draw_res_mdl_directly,
 *   and the sort that orders the depth records.
 * RANGE. .text 0x800777B0-0x80079FB4 (58 functions); extab 0x800082C8-0x800083D8, extabindex 0x80020A54-0x80020BEC,
 *   .data 0x8058E880-0x8058EAD8 (opens on "g3d_draw.cpp"; the `_ac.h` asserts of the handle members follow), .sdata
 *   0x807911F8-0x80791200 (the two sort jitter counters), .sdata2 0x80795E18-0x80795E28 (0.0, -2.0, 1.0: a 1.0
 *   pooled again after `g3d/g3d_draw1mat1shp.cpp`'s 0x80795E00, so a new TU at 0x800777B0).  The sort family
 *   0x80079604-0x80079F78 is used only by this unit's depth-sorted passes; `g3d/g3d_fog.cpp` follows ("g3d_fog.cpp",
 *   a 0.0 pooled again at 0x80795E28).
 * FLAGS. cflags_g3d (docs/g3d.md); `#pragma peephole off` (playbook idea 106) and `#pragma pool_data off` (each
 *   literal its own `lis`/`addi`).
 * NAMES. nw4r's where the map or the asserts carry them (Draw1Mat1ShpDirectly, ResByteCodeData::DRAW, MdlZ,
 *   WORKMEM_NUMMDLZ).  GUESS: PrepareIndMtxOp (0x800777B0), SetupDraw1Mat1ShpSwap (0x80077E70), DrawByteCode
 *   (0x8007868C), DrawByteCodeReplacement (0x80078A9C), DrawMdlZ (0x80078E7C), DrawMdlZReplacement (0x80079018),
 *   GatherMdlZ (0x800791D8), CompareMdlZXlu/CompareMdlZOpa (0x80079F14/0x80079F78), the light-object tests
 *   light_obj_is_specular_light/light_obj_is_point_light/light_obj_is_directional (0x80077D4C/0x80077D64/
 *   0x80077DD8), vec3_negate (0x80077DBC),
 *   mtx34_set (0x80077DF0, MTX34's twelve-float constructor), g3d_draw_res_mdl_directly (0x800793A4, nw4r's
 *   DrawResMdlDirectly; its consumers call the C name) and DrawResMdlReplacement's field names.  fn_80077E34,
 *   fn_80077E64 and fn_80078904 (ResMdl's copy constructor and ResNode::IsVisible) keep their stems because
 *   `g3d/g3d_scnmdl.cpp` calls them by those names (g3d-draw#14).
 * RESIDUALS. PrepareIndMtxOp: the model-info word is a named local (retail: a temporary), so three stack slots
 *   shift (a `const u32&` temporary does not move them).  sort: the first/last/i registers differ (r27/r28/r29
 *   for retail's r28/r29/r26); sort and __sort recompute the /12 reciprocal once more than retail (one `subi`).  The handle assignments call this TU's own copies where retail calls other units' (res_node_assign,
 *   res_mat_misc_copy_ctor): relocation names only.  The ResCommon<T>::operator= instances come out at the end of
 *   the object (retail: after each class's operator=).  Data: the literals are emitted but not yet in retail's
 *   order, and the inline copies' strings are merged; flipcheck is not ready.  STOPGAPs: g3d-draw#1, #3, #9.
 * SHAPES. `#pragma dont_inline on` reproduces retail's out-of-line accessors and compiler-generated assignments;
 *   the loop counters declared first (DrawMdlZ*, PrepareIndMtxOp) and the record pointer declared before the count
 *   (GatherMdlZ) give retail's register colouring; the specular half vector is written without a named scale and
 *   under `fp_contract off`.  `MSL/algorithm.h`'s quicksort names the second pivot's `3 * n / 4` before adding the
 *   jitter and tests `!comp(*first, *(j = last - 1))` in one expression (retail's scheduling of both).
 */

#include "types.h"
#include "mh3_pad.h"               /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "fn_8004CAD8/mtx.h"       /* MTX34_ctor (rule 2) */
#include "g3d/g3d_obj.h"
#include "g3d/g3d_anmchr.h"        /* res_node_ref / res_node_ref_nonconst / res_node_copy_ctor / res_node_assign (rule 2) */
#include "g3d/fn_8005AA28.h"       /* res_node_is_valid (rule 2) */
#include "g3d/g3d_anmclr.h"        /* res_mat_get_id (rule 2) */
#include "g3d/g3d_calcview.h"      /* mtx34_concat / mtx34_copy_ps / res_mdl_info_handle (rule 2) */
#include "g3d/g3d_calcworld.h"     /* res_mdl_get_info / res_mdl_info_num_view_mtx (rule 2) */
#include "g3d/g3d_resmat.h"
#include "g3d/g3d_resnode.h"
#include "g3d/g3d_resshp.h"
#include "g3d/g3d_state.h"         /* the G3D state accessors (rule 2) */
#include "g3d/g3d_ind_mtx_callback.h" /* G3dIndMtxOp, G3dIndMtxCallback (rule 1) */
#include "g3d/light_obj_is_enable.h" /* light_obj_is_enable, owner g3d/g3d_anmscn.cpp (rule 2) */
#include "g3d/g3d_gpu.h"           /* Mat33 (rule 1) */
#include "g3d/g3d_pointer_assert.h"
#include "g3d/g3d_light.h"         /* light_obj_get_light_pos / light_obj_get_light_dir (rule 2) */
#include "g3d/g3d_draw1mat1shp.h"  /* Draw1Mat1ShpDirectly, Draw1Mat1ShpSwap, res_mat_misc_copy_ctor (rule 2) */
#include "g3d/g3d_draw.h"
#include "OS/PPCArch.h"            /* PPCSync (rule 2) */
#include "nw4r/db_assert.h"        /* nw4r::db::Panic (rule 2) */
#include "MSL/algorithm.h"         /* std::sort */

#pragma peephole off
#pragma pool_data off
#pragma dont_inline on

extern "C" {
u32 vec3_normalize_into(nw4r::math::VEC3* pOut, const nw4r::math::VEC3* pIn);
u32 fn_8006E2A8(s32, s32);
nw4r::g3d::ResMdl fn_80094094(const nw4r::g3d::ResMat* pMat);
}



namespace nw4r {
namespace g3d {

/* One depth-sort record the sorted passes gather per visible DRAW command.  size: 0xC */
struct MdlZ {
    /* +0x0 */ f32 Z;          /* the view-space depth of the node's matrix */
    /* +0x4 */ u16 priority;   /* the command's draw priority */
    /* +0x6 */ u16 nodeID;
    /* +0x8 */ u16 matID;
    /* +0xA */ u16 shpID;
};

/* The capacity of the work-memory depth-record array. */
enum {
    WORKMEM_NUMMDLZ = 0x800
};

/* One material's display-list blocks as a replacement block carries them (the sizes the refill strides by). */
struct ReplPixDL { /* +0x0 */ u8 data[0x20]; };              /* size: 0x20 */
struct ReplTevColorDL { /* +0x0 */ u8 data[0x80]; };         /* size: 0x80 */
struct ReplIndMtxAndScaleDL { /* +0x0 */ u8 data[0x40]; };   /* size: 0x40 */
struct ReplTexCoordGenDL { /* +0x0 */ u8 data[0xA0]; };      /* size: 0xA0 */
struct ReplTevDL { /* +0x0 */ u8 data[0x200]; };             /* size: 0x200 */

/* The per-model replacement a draw can use instead of the model's own resources: a node visibility array and one
 * array per material resource kind, indexed by the material's id.  size: 0x40 */
struct DrawResMdlReplacement {
    /* +0x00 */ u32 flag;                            /* bit 0: keep the swap's vertex tables clear */
    /* +0x04 */ u8* visArray;                        /* per node: drawn when non-zero; NULL = the nodes' own flags */
    /* +0x08 */ ResTexObjData* texObjDataArray;
    /* +0x0C */ ResTlutObjData* tlutObjDataArray;
    /* +0x10 */ ResTexSrtData* texSrtDataArray;
    /* +0x14 */ ResMatChanData* chanDataArray;
    /* +0x18 */ ResGenModeData* genModeDataArray;
    /* +0x1C */ ResMatMiscData* matMiscDataArray;
    /* +0x20 */ ReplPixDL* pixDLArray;
    /* +0x24 */ ReplTevColorDL* tevColorDLArray;
    /* +0x28 */ ReplIndMtxAndScaleDL* indMtxAndScaleDLArray;
    /* +0x2C */ ReplTexCoordGenDL* texCoordGenDLArray;
    /* +0x30 */ ReplTevDL* tevDataArray;
    /* +0x34 */ ResVtxPosData** vtxPosTable;     /* per vertex-position id; NULL = the model's own */
    /* +0x38 */ ResVtxNrmData** vtxNrmTable;
    /* +0x3C */ ResVtxClrData** vtxClrTable;
};

static G3dIndMtxCallback* PrepareIndMtxOp(ResMat mat, ResNode node, ResShp shp);
static bool CompareMdlZXlu(const MdlZ& lhs, const MdlZ& rhs);
static bool CompareMdlZOpa(const MdlZ& lhs, const MdlZ& rhs);

}  // namespace g3d
}  // namespace nw4r

extern "C" {
s32 light_obj_is_specular_light(const G3dLightObj* pLObj);
s32 light_obj_is_point_light(const G3dLightObj* pLObj);
s32 light_obj_is_directional(const G3dLightObj* pLObj);
void vec3_negate(nw4r::math::VEC3* pOut, const nw4r::math::VEC3* pIn);
void fn_80077E64(s32* pDst, s32* pSrc);
void SetupDraw1Mat1ShpSwap(nw4r::g3d::Draw1Mat1ShpSwap* pSwap, nw4r::g3d::DrawResMdlReplacement* pRep, u32 idx);
u32 fn_80078608(s32* pDst, s32* pSrc);
}

namespace nw4r {
namespace g3d {

#pragma fp_contract off
/* 0x800777B0 (0x59C): resets the indirect-matrix callback and offers it, per normal-mapped indirect stage of the
 * material, the shape's normal matrix and the stage's light direction. */
static G3dIndMtxCallback* PrepareIndMtxOp(ResMat mat, ResNode node, ResShp shp) {
    if (!mat.IsValid() || !shp.IsValid()) {
        return NULL;
    }
    ResMatMisc misc(mat.GetResMatMisc());
    ResMatMiscData::IndirectMethod method[3];
    s8 lightRef[3];
    misc.GetIndirectMethod(GX_ITM_0, &method[0], &lightRef[0]);
    misc.GetIndirectMethod(GX_ITM_1, &method[1], &lightRef[1]);
    misc.GetIndirectMethod(GX_ITM_2, &method[2], &lightRef[2]);
    if (method[0] == ResMatMiscData::WARP && method[1] == ResMatMiscData::WARP && method[2] == ResMatMiscData::WARP) {
        return NULL;
    }

    bool nrmMtxReady = false;
    math::MTX34 nrmMtx;
    MTX34_ctor(&nrmMtx);
    u32 i;
    G3dIndMtxCallback* pOp = g3d_state_get_ind_mtx_hook();
    pOp->Reset();
    for (i = 0; i < 3; i++) {
        u32 id = i + 1;
        if (method[i] == ResMatMiscData::WARP) {
            continue;
        }
        if (!nrmMtxReady) {
            nrmMtxReady = true;
            if (shp.ptr()->curMtxIdx >= 0) {
                if (shp.ptr()->curMtxIdx == fn_8006FDCC(&node)) {
                    const Mat33* pNrm = g3d_state_get_nrm_mtx(shp.ptr()->curMtxIdx);
                    nrmMtx.m[0][0] = pNrm->m[0][0];
                    nrmMtx.m[0][1] = pNrm->m[0][1];
                    nrmMtx.m[0][2] = pNrm->m[0][2];
                    nrmMtx.m[1][0] = pNrm->m[1][0];
                    nrmMtx.m[1][1] = pNrm->m[1][1];
                    nrmMtx.m[1][2] = pNrm->m[1][2];
                    nrmMtx.m[2][0] = pNrm->m[2][0];
                    nrmMtx.m[2][1] = pNrm->m[2][1];
                    nrmMtx.m[2][2] = pNrm->m[2][2];
                } else {
                    ResMdl mdl(fn_80094094(&mat));
                    u32 info = res_mdl_get_info(&mdl);
                    int shpNodeID = res_mdl_info_get_node_of_pos_nrm_mtx(&info, shp.ptr()->curMtxIdx);
                    if (shpNodeID < 0) {
                        nw4r::db::Panic("g3d_draw.cpp", 0x64, "NW4R:Failed assertion shpNodeID >= 0");
                    }
                    ResNode shpNode(mdl.GetResNode(shpNodeID));
                    ResNodeData* pNodeData = res_node_ref_nonconst((ResHandle*)&node);
                    mtx34_concat(&nrmMtx, (const math::MTX34*)res_node_ref_nonconst((ResHandle*)&shpNode)->mInvModelMtx,
                                 (const math::MTX34*)pNodeData->mModelMtx);
                    const Mat33* pNrm = g3d_state_get_nrm_mtx(shp.ptr()->curMtxIdx);
                    math::MTX34 shpNrmMtx;
                    mtx34_set(&shpNrmMtx, pNrm->m[0][0], pNrm->m[0][1], pNrm->m[0][2], 0.0f, pNrm->m[1][0], pNrm->m[1][1], pNrm->m[1][2],
                              0.0f, pNrm->m[2][0], pNrm->m[2][1], pNrm->m[2][2], 0.0f);
                    mtx34_concat(&nrmMtx, &shpNrmMtx, &nrmMtx);
                }
                nrmMtx.m[0][3] = nrmMtx.m[1][3] = nrmMtx.m[2][3] = 0.0f;

                math::VEC3 axisX;
                setVec3(&axisX, nrmMtx.m[0][0], nrmMtx.m[1][0], nrmMtx.m[2][0]);
                vec3_normalize_into(&axisX, &axisX);
                nrmMtx.m[0][0] = axisX.x;
                nrmMtx.m[1][0] = axisX.y;
                nrmMtx.m[2][0] = axisX.z;
                math::VEC3 axisY;
                setVec3(&axisY, nrmMtx.m[0][1], nrmMtx.m[1][1], nrmMtx.m[2][1]);
                vec3_normalize_into(&axisY, &axisY);
                nrmMtx.m[0][1] = axisY.x;
                nrmMtx.m[1][1] = axisY.y;
                nrmMtx.m[2][1] = axisY.z;
                math::VEC3 axisZ;
                setVec3(&axisZ, nrmMtx.m[0][2], nrmMtx.m[1][2], nrmMtx.m[2][2]);
                vec3_normalize_into(&axisZ, &axisZ);
                nrmMtx.m[0][2] = axisZ.x;
                nrmMtx.m[1][2] = axisZ.y;
                nrmMtx.m[2][2] = axisZ.z;
            } else {
                mtx34_copy_ps(&nrmMtx, g3d_state_get_camera_mtx());
            }
        }

        G3dLightObj* pLObj = g3d_state_get_light_obj(g3d_state_get_light_set_entry(lightRef[i]));
        if (pLObj != NULL && light_obj_is_enable((LightObj*)pLObj)) {
            math::VEC3 lightDir;
            math::VEC3 specDir;
            math::VEC3 negDir;
            math::VEC3 negPos;
            VEC3_ctor(&lightDir);
            if (light_obj_is_directional(pLObj)) {
                light_obj_get_light_dir((LightObj*)pLObj, &lightDir);
                if (0.0f == lightDir.x && 0.0f == lightDir.y && 0.0f == lightDir.z) {
                    light_obj_get_light_pos((LightObj*)pLObj, &lightDir);
                    vec3_negate(&negDir, &lightDir);
                    copyVec3(&lightDir, &negDir);
                    vec3_normalize_into(&lightDir, &lightDir);
                }
            } else if (light_obj_is_point_light(pLObj)) {
                light_obj_get_light_pos((LightObj*)pLObj, &lightDir);
                vec3_negate(&negPos, &lightDir);
                copyVec3(&lightDir, &negPos);
                vec3_normalize_into(&lightDir, &lightDir);
            } else {
                VEC3_ctor(&specDir);
                if (!light_obj_is_specular_light(pLObj)) {
                    nw4r::db::Panic("g3d_draw.cpp", 0xAB, "NW4R:Failed assertion pLObj->IsSpecularLight()");
                }
                light_obj_get_light_dir((LightObj*)pLObj, &specDir);
                lightDir.x = -2.0f * specDir.z * specDir.x;
                lightDir.y = -2.0f * specDir.z * specDir.y;
                lightDir.z = 1.0f + -2.0f * specDir.z * specDir.z;
                vec3_normalize_into(&lightDir, &lightDir);
            }
            pOp->SetNrmMapMtx((_GXIndTexMtxID)id, &lightDir, &nrmMtx, method[i]);
        } else {
            pOp->SetNrmMapMtx((_GXIndTexMtxID)id, NULL, &nrmMtx, method[i]);
        }
    }
    return pOp;
}
#pragma fp_contract reset

}  // namespace g3d
}  // namespace nw4r

extern "C" {

/* 0x80077D4C (0x18): whether the light object is a specular light (flag bit 1). */
s32 light_obj_is_specular_light(const G3dLightObj* pLObj) {
    return (*(const u32*)pLObj & 2) != 0;
}

/* 0x80077D64 (0x58): whether the light object is neither directional nor specular. */
s32 light_obj_is_point_light(const G3dLightObj* pLObj) {
    s32 isPoint = 0;
    if (!light_obj_is_directional(pLObj) && !light_obj_is_specular_light(pLObj)) {
        isPoint = 1;
    }
    return isPoint;
}

/* 0x80077DBC (0x1C): stores the negated vector. */
void vec3_negate(nw4r::math::VEC3* pOut, const nw4r::math::VEC3* pIn) {
    setVec3(pOut, -pIn->x, -pIn->y, -pIn->z);
}

/* 0x80077DD8 (0x18): whether the light object is directional (flag bit 0). */
s32 light_obj_is_directional(const G3dLightObj* pLObj) {
    return (*(const u32*)pLObj & 1) != 0;
}

/* 0x80077DF0 (0x44): fills a matrix from twelve values, row by row. */
void mtx34_set(Mtx34* dst, f32 m00, f32 m01, f32 m02, f32 m03, f32 m10, f32 m11, f32 m12, f32 m13, f32 m20, f32 m21,
               f32 m22, f32 m23) {
    dst->m[0][0] = m00;
    dst->m[0][1] = m01;
    dst->m[0][2] = m02;
    dst->m[0][3] = m03;
    dst->m[1][0] = m10;
    dst->m[1][1] = m11;
    dst->m[1][2] = m12;
    dst->m[1][3] = m13;
    dst->m[2][0] = m20;
    dst->m[2][1] = m21;
    dst->m[2][2] = m22;
    dst->m[2][3] = m23;
}

/* 0x80077E34 (0x30): copy-constructs a model handle (this unit's copy, which `g3d/g3d_scnmdl.cpp` calls too). */
s32 fn_80077E34(s32 pOut, void* pIn) { /* untyped: opaque handle - the source ResMdl */
    fn_80077E64((s32*)pOut, (s32*)pIn);
    return pOut;
}

/* 0x80077E64 (0xC): copies the handle word. */
void fn_80077E64(s32* pDst, s32* pSrc) {
    *pDst = *pSrc;
}

/* 0x80077E70 (0x540): points the swap's handles at the replacement's blocks for material `idx` (a null handle where
 * the replacement has no array), and takes its vertex tables unless the replacement's flag says not to. */
void SetupDraw1Mat1ShpSwap(nw4r::g3d::Draw1Mat1ShpSwap* pSwap, nw4r::g3d::DrawResMdlReplacement* pRep, u32 idx) {
    using namespace nw4r::g3d;
    G3D_POINTER_ASSERT("g3d_draw.cpp", pSwap, 0xCD, "NW4R:Pointer Error\npSwap(=%p) is not valid pointer.");
    G3D_POINTER_ASSERT("g3d_draw.cpp", pRep, 0xCE, "NW4R:Pointer Error\npRep(=%p) is not valid pointer.");
    if (pRep->texObjDataArray != NULL) {
        pSwap->texObj = ResTexObj(&pRep->texObjDataArray[idx]);
    } else {
        pSwap->texObj = ResTexObj(NULL);
    }
    if (pRep->tlutObjDataArray != NULL) {
        pSwap->tlutObj = ResTlutObj(&pRep->tlutObjDataArray[idx]);
    } else {
        pSwap->tlutObj = ResTlutObj(NULL);
    }
    if (pRep->texSrtDataArray != NULL) {
        pSwap->texSrt = ResTexSrt(&pRep->texSrtDataArray[idx]);
    } else {
        pSwap->texSrt = ResTexSrt(NULL);
    }
    if (pRep->chanDataArray != NULL) {
        pSwap->chan = ResMatChan(&pRep->chanDataArray[idx]);
    } else {
        pSwap->chan = ResMatChan(NULL);
    }
    if (pRep->genModeDataArray != NULL) {
        pSwap->genMode = ResGenMode(&pRep->genModeDataArray[idx]);
    } else {
        pSwap->genMode = ResGenMode(NULL);
    }
    if (pRep->matMiscDataArray != NULL) {
        pSwap->misc = ResMatMisc(&pRep->matMiscDataArray[idx]);
    } else {
        pSwap->misc = ResMatMisc(NULL);
    }
    if (pRep->pixDLArray != NULL) {
        pSwap->pix = ResMatPix(&pRep->pixDLArray[idx]);
    } else {
        pSwap->pix = ResMatPix(NULL);
    }
    if (pRep->tevColorDLArray != NULL) {
        pSwap->tevColor = ResMatTevColor(&pRep->tevColorDLArray[idx]);
    } else {
        pSwap->tevColor = ResMatTevColor(NULL);
    }
    if (pRep->indMtxAndScaleDLArray != NULL) {
        pSwap->indMtxAndScale = ResMatIndMtxAndScale(&pRep->indMtxAndScaleDLArray[idx]);
    } else {
        pSwap->indMtxAndScale = ResMatIndMtxAndScale(NULL);
    }
    if (pRep->texCoordGenDLArray != NULL) {
        pSwap->texCoordGen = ResMatTexCoordGen(&pRep->texCoordGenDLArray[idx]);
    } else {
        pSwap->texCoordGen = ResMatTexCoordGen(NULL);
    }
    if (pRep->tevDataArray != NULL) {
        pSwap->tev = ResTev(&pRep->tevDataArray[idx]);
    } else {
        pSwap->tev = ResTev(NULL);
    }
    if (pRep->flag & 1) {
        pSwap->vtxPosTable = NULL;
        pSwap->vtxNrmTable = NULL;
        pSwap->vtxClrTable = NULL;
        return;
    }
    pSwap->vtxPosTable = pRep->vtxPosTable;
    pSwap->vtxNrmTable = pRep->vtxNrmTable;
    pSwap->vtxClrTable = pRep->vtxClrTable;
}

}

/* 0x800783B0 (0x30): copies the handle. */
nw4r::g3d::ResTev& nw4r::g3d::ResTev::operator=(const ResTev& rhs) {
    ResCommon<ResTevData>::operator=(rhs);
    return *this;
}

/* 0x800783EC (0x64): wraps `pData`, asserting its alignment. */
/* untyped: opaque handle - the block address */
nw4r::g3d::ResTev::ResTev(void* pData) : ResCommon<ResTevData>(pData) {
    if ((u32)pData & 0x1F) {
        nw4r::db::Panic("g3d_restev_ac.h", 0x25, "NW4R:Failed assertion !((u32)p & 0x1f)");
    }
}

/* 0x80078450 (0x8): stores the block address. */
NW4R_G3D_RESCOMMON_CTOR(nw4r::g3d::ResTevData)

/* 0x80078458 (0x30): copies the handle. */
nw4r::g3d::ResMatTexCoordGen& nw4r::g3d::ResMatTexCoordGen::operator=(const ResMatTexCoordGen& rhs) {
    ResCommon<ResMatTexCoordGenData>::operator=(rhs);
    return *this;
}

/* 0x80078494 (0x30): copies the handle. */
nw4r::g3d::ResMatIndMtxAndScale& nw4r::g3d::ResMatIndMtxAndScale::operator=(const ResMatIndMtxAndScale& rhs) {
    ResCommon<ResMatIndMtxAndScaleData>::operator=(rhs);
    return *this;
}

/* 0x800784D0 (0x30): copies the handle. */
nw4r::g3d::ResMatTevColor& nw4r::g3d::ResMatTevColor::operator=(const ResMatTevColor& rhs) {
    ResCommon<ResMatTevColorData>::operator=(rhs);
    return *this;
}

/* 0x8007850C (0x30): copies the handle. */
nw4r::g3d::ResMatPix& nw4r::g3d::ResMatPix::operator=(const ResMatPix& rhs) {
    ResCommon<ResMatPixData>::operator=(rhs);
    return *this;
}

/* 0x80078548 (0x30): copies the handle. */
nw4r::g3d::ResGenMode& nw4r::g3d::ResGenMode::operator=(const ResGenMode& rhs) {
    ResCommon<ResGenModeData>::operator=(rhs);
    return *this;
}

/* 0x80078584 (0x30): copies the handle. */
nw4r::g3d::ResMatChan& nw4r::g3d::ResMatChan::operator=(const ResMatChan& rhs) {
    ResCommon<ResMatChanData>::operator=(rhs);
    return *this;
}

/* 0x800785C0 (0x48): copies the handle word and the word after it. */
nw4r::g3d::ResTexSrt& nw4r::g3d::ResTexSrt::operator=(const ResTexSrt& rhs) {
    fn_80078608((s32*)this, (s32*)&rhs);
    fn_8006E2A8((s32)this + 4, (s32)&rhs + 4);
    return *this;
}

/* 0x80078608 (0xC): copies the handle word. */
extern "C" u32 fn_80078608(s32 *arg0, s32 *arg1) {
    *arg0 = *arg1;
}

/* 0x80078614 (0x30): copies the handle. */
nw4r::g3d::ResTlutObj& nw4r::g3d::ResTlutObj::operator=(const ResTlutObj& rhs) {
    ResCommon<ResTlutObjData>::operator=(rhs);
    return *this;
}

/* 0x80078650 (0x30): copies the handle. */
nw4r::g3d::ResTexObj& nw4r::g3d::ResTexObj::operator=(const ResTexObj& rhs) {
    ResCommon<ResTexObjData>::operator=(rhs);
    return *this;
}

namespace nw4r {
namespace g3d {

/* The byte-code commands the draw passes read: END closes the list, DRAW is
 * {4, matIdx(2), shpIdx(2), nodeIdx(2), priority(1)}. */
enum ResByteCodeCommand {
    RESBYTECODE_END = 1,
    RESBYTECODE_DRAW = 4
};

/* Draws a model's DRAW commands in byte-code order. */
static void DrawByteCode(ResMdl mdl, const u8* pByteCode, u32 drawMode) {
    ResMat mat(NULL);
    ResMat matPrev(NULL);
    ResShp shp(NULL);
    ResNode node(NULL);
    u32 ctrl = 1;
    if (drawMode & 8) {
        ctrl |= 8;
    }
    if (drawMode & 4) {
        ctrl |= 0x10;
    }

    while (*pByteCode != RESBYTECODE_END) {
        if (*pByteCode != RESBYTECODE_DRAW) {
            nw4r::db::Panic("g3d_draw.cpp", 0x128, "NW4R:Failed assertion c == ResByteCodeData::DRAW");
        }
        node = mdl.GetResNode((pByteCode[5] << 8) | pByteCode[6]);
        shp = mdl.GetResShp((pByteCode[3] << 8) | pByteCode[4]);
        if (fn_80078904((s32)&node) && shp.IsVisible()) {
            mat = mdl.GetResMat((pByteCode[1] << 8) | pByteCode[2]);
            Draw1Mat1ShpDirectly(mat, shp, NULL, NULL, (mat == matPrev) ? (ctrl | 0x10) : ctrl, NULL,
                                 PrepareIndMtxOp(mat, node, shp));
            matPrev = mat;
        }
        pByteCode += 8;
    }
}

/* 0x80078878 (0x50): whether both handles name the same block. */
bool ResMat::operator==(const ResMat& rhs) const {
    return ptr() == rhs.ptr();
}

}  // namespace g3d
}  // namespace nw4r

/* 0x80078904 (0x84): whether the node is drawn (this unit's copy of ResNode::IsVisible, which
 * `g3d/g3d_scnmdl.cpp` calls too). */
bool fn_80078904(s32 pNode) {
    const ResHandle* pHandle = (const ResHandle*)pNode;
    if (!res_node_is_valid(pHandle)) {
        nw4r::db::Panic("g3d_resnode_ac.h", 0xA5, "NW4R:Failed assertion IsValid()");
    }
    if (res_node_is_valid(pHandle)) {
        return (res_node_ref(pHandle)->mFlags & 0x100) != 0;
    }
    return false;
}

namespace nw4r {
namespace g3d {

/* 0x800789C4 (0x64): wraps `pData`, asserting its alignment. */
/* untyped: opaque handle - the block address */
ResShp::ResShp(void* pData) : ResCommon<ResShpData>(pData) {
    if ((u32)pData & 3) {
        nw4r::db::Panic("g3d_resshp_ac.h", 0x3A, "NW4R:Failed assertion !((u32)p & 0x3)");
    }
}

}  // namespace g3d
}  // namespace nw4r

/* 0x80078A28 (0x8): stores the block address. */
NW4R_G3D_RESCOMMON_CTOR(nw4r::g3d::ResShpData)

/* 0x80078A30 (0x64): wraps `pData`, asserting its alignment. */
/* untyped: opaque handle - the block address */
nw4r::g3d::ResMat::ResMat(void* pData) : ResCommon<ResMatData>(pData) {
    if ((u32)pData & 3) {
        nw4r::db::Panic("g3d_resmat_ac.h", 0x26D, "NW4R:Failed assertion !((u32)p & 0x3)");
    }
}

/* 0x80078A94 (0x8): stores the block address. */
NW4R_G3D_RESCOMMON_CTOR(nw4r::g3d::ResMatData)

namespace nw4r {
namespace g3d {

/* Draws a model's DRAW commands in byte-code order, with the replacement block's visibility and materials. */
static void DrawByteCodeReplacement(ResMdl mdl, const u8* pByteCode, DrawResMdlReplacement* pRep, u32 drawMode) {
    ResMat mat(NULL);
    ResMat matPrev(NULL);
    ResShp shp(NULL);
    ResNode node(NULL);
    Draw1Mat1ShpSwap swap;
    G3D_POINTER_ASSERT("g3d_draw.cpp", pRep, 0x14A, "NW4R:Pointer Error\npRep(=%p) is not valid pointer.");
    u32 ctrl = 1;
    if (drawMode & 8) {
        ctrl |= 8;
    }
    if (drawMode & 4) {
        ctrl |= 0x10;
    }

    while (*pByteCode != RESBYTECODE_END) {
        if (*pByteCode != RESBYTECODE_DRAW) {
            nw4r::db::Panic("g3d_draw.cpp", 0x154, "NW4R:Failed assertion c == ResByteCodeData::DRAW");
        }
        node = mdl.GetResNode((pByteCode[5] << 8) | pByteCode[6]);
        shp = mdl.GetResShp((pByteCode[3] << 8) | pByteCode[4]);
        bool visible;
        if (pRep->visArray != NULL) {
            visible = pRep->visArray[res_node_get_id((const ResHandle*)&node)] != 0;
        } else {
            visible = fn_80078904((s32)&node);
        }
        if (visible && shp.IsVisible()) {
            mat = mdl.GetResMat((pByteCode[1] << 8) | pByteCode[2]);
            SetupDraw1Mat1ShpSwap(&swap, pRep, res_mat_get_id((const ResHandle*)&mat));
            Draw1Mat1ShpDirectly(mat, shp, NULL, NULL, (mat == matPrev) ? (ctrl | 0x10) : ctrl, &swap,
                                 PrepareIndMtxOp(mat, node, shp));
            matPrev = mat;
        }
        pByteCode += 8;
    }
}

}  // namespace g3d
}  // namespace nw4r

/* 0x80078DC0 (0xBC): constructs every handle empty and clears the trailing words. */
nw4r::g3d::Draw1Mat1ShpSwap::Draw1Mat1ShpSwap()
    : texObj(NULL), tlutObj(NULL), genMode(NULL), tev(NULL), pix(NULL), tevColor(NULL), indMtxAndScale(NULL),
      chan(NULL), texCoordGen(NULL), misc(NULL), texSrt(NULL), vtxPosTable(NULL), vtxNrmTable(NULL),
      vtxClrTable(NULL) {}

namespace nw4r {
namespace g3d {

/* Draws gathered depth-sort records in their sorted order. */
static void DrawMdlZ(ResMdl mdl, const MdlZ* pMdlZ, u32 numMdlZ, u32 drawMode) {
    u32 i;
    ResMat mat(NULL);
    ResMat matPrev(NULL);
    ResShp shp(NULL);
    ResNode node(NULL);
    u32 ctrl = 1;
    if (drawMode & 8) {
        ctrl |= 8;
    }
    if (drawMode & 4) {
        ctrl |= 0x10;
    }

    for (i = 0; i < numMdlZ; i++) {
        shp = mdl.GetResShp((int)pMdlZ->shpID);
        if (shp.IsVisible()) {
            mat = mdl.GetResMat((int)pMdlZ->matID);
            node = mdl.GetResNode((int)pMdlZ->nodeID);
            Draw1Mat1ShpDirectly(mat, shp, NULL, NULL, (mat == matPrev) ? (ctrl | 0x10) : ctrl, NULL,
                                 PrepareIndMtxOp(mat, node, shp));
            matPrev = mat;
        }
        pMdlZ++;
    }
}

/* Draws gathered depth-sort records in their sorted order, with the replacement block's materials. */
static void DrawMdlZReplacement(ResMdl mdl, const MdlZ* pMdlZ, u32 numMdlZ, DrawResMdlReplacement* pRep,
                                u32 drawMode) {
    u32 i;
    ResMat mat(NULL);
    ResMat matPrev(NULL);
    ResShp shp(NULL);
    ResNode node(NULL);
    Draw1Mat1ShpSwap swap;
    u32 ctrl = 1;
    if (drawMode & 8) {
        ctrl |= 8;
    }
    if (drawMode & 4) {
        ctrl |= 0x10;
    }

    for (i = 0; i < numMdlZ; i++) {
        shp = mdl.GetResShp((int)pMdlZ->shpID);
        if (shp.IsVisible()) {
            mat = mdl.GetResMat((int)pMdlZ->matID);
            node = mdl.GetResNode((int)pMdlZ->nodeID);
            SetupDraw1Mat1ShpSwap(&swap, pRep, res_mat_get_id((const ResHandle*)&mat));
            Draw1Mat1ShpDirectly(mat, shp, NULL, NULL, (mat == matPrev) ? (ctrl | 0x10) : ctrl, &swap,
                                 PrepareIndMtxOp(mat, node, shp));
            matPrev = mat;
        }
        pMdlZ++;
    }
}

/* Gathers a depth-sort record for every visible DRAW command: its view depth, priority and indices. */
static MdlZ* GatherMdlZ(u32* pNumMdlZ, ResMdl mdl, const math::MTX34* pViewPosMtxArray, const u8* pByteCode,
                        DrawResMdlReplacement* pRep) {
    MdlZ* pMdlZ;
    u32 numMdlZ = 0;
    ResNode node(NULL);
    u32 info = res_mdl_info_handle(&mdl);
    u32 numViewMtx = res_mdl_info_num_view_mtx(&info);
    MdlZ* pMdlZArray = (MdlZ*)workmem_get_mdl_z_temporary();
    pMdlZ = pMdlZArray;

    while (*pByteCode != RESBYTECODE_END) {
        if (*pByteCode != RESBYTECODE_DRAW) {
            nw4r::db::Panic("g3d_draw.cpp", 0x1D7, "NW4R:Failed assertion c == ResByteCodeData::DRAW");
        }
        u16 nodeID = (pByteCode[5] << 8) | pByteCode[6];
        node = mdl.GetResNode((int)nodeID);
        bool visible;
        if (pRep == NULL || pRep->visArray == NULL) {
            visible = fn_80078904((s32)&node);
        } else {
            visible = pRep->visArray[res_node_get_id((const ResHandle*)&node)] != 0;
        }
        if (visible) {
            pMdlZ->nodeID = nodeID;
            pMdlZ->matID = (pByteCode[1] << 8) | pByteCode[2];
            pMdlZ->shpID = (pByteCode[3] << 8) | pByteCode[4];
            u32 mtxID = fn_8006FDCC(&node);
            if (mtxID < numViewMtx) {
                pMdlZ->Z = pViewPosMtxArray[mtxID].m[2][3];
            } else {
                pMdlZ->Z = 0.0f;
            }
            pMdlZ->priority = pByteCode[7];
            pMdlZ++;
            numMdlZ++;
        }
        pByteCode += 8;
    }
    if (numMdlZ >= WORKMEM_NUMMDLZ) {
        nw4r::db::Panic("g3d_draw.cpp", 0x1F3, "NW4R:Failed assertion numMdlZ < detail::workmem::WORKMEM_NUMMDLZ");
    }
    *pNumMdlZ = numMdlZ;
    return pMdlZArray;
}

}  // namespace g3d
}  // namespace nw4r

/* 0x800793A4 (0x260): draws the model's opaque and translucent byte code over the view matrices, sorted by depth or
 * in byte-code order, through the replacement block when one is given. */
/* untyped: caller-owned payload - the replacement block */
void g3d_draw_res_mdl_directly(const nw4r::g3d::ResMdl* pMdl, const nw4r::math::MTX34* pViewPosMtxArray,
                               const nw4r::math::MTX33* pViewNrmMtxArray, const nw4r::math::MTX34* pViewTexMtxArray,
                               const u8* pByteCodeOpa, const u8* pByteCodeXlu, const void* pReplacement,
                               u32 drawMode) {
    using namespace nw4r::g3d;
    DrawResMdlReplacement* pRep = (DrawResMdlReplacement*)pReplacement;
    if (pViewPosMtxArray == NULL) {
        nw4r::db::Panic("g3d_draw.cpp", 0x21E, "NW4R:Pointer must not be NULL (pViewPos)");
    }
    g3d_state_set_view_mtx_arrays((nw4r::math::MTX34*)pViewPosMtxArray, (Mat33*)pViewNrmMtxArray,
                                  (nw4r::math::MTX34*)pViewTexMtxArray);
    if (!(drawMode & 0x10)) {
        g3d_lc_queue_wait(0);
        PPCSync();
    }

    if (pByteCodeOpa != NULL) {
        if (drawMode & 1) {
            u32 numMdlZ;
            MdlZ* pMdlZ = GatherMdlZ(&numMdlZ, *pMdl, pViewPosMtxArray, pByteCodeOpa, pRep);
            std::sort(pMdlZ, pMdlZ + numMdlZ, CompareMdlZOpa);
            if (pRep != NULL) {
                DrawMdlZReplacement(*pMdl, pMdlZ, numMdlZ, pRep, drawMode);
            } else {
                DrawMdlZ(*pMdl, pMdlZ, numMdlZ, drawMode);
            }
        } else if (pRep != NULL) {
            DrawByteCodeReplacement(*pMdl, pByteCodeOpa, pRep, drawMode);
        } else {
            DrawByteCode(*pMdl, pByteCodeOpa, drawMode);
        }
    }

    if (pByteCodeXlu != NULL) {
        if (drawMode & 2) {
            u32 numMdlZ;
            MdlZ* pMdlZ = GatherMdlZ(&numMdlZ, *pMdl, pViewPosMtxArray, pByteCodeXlu, pRep);
            std::sort(pMdlZ, pMdlZ + numMdlZ, CompareMdlZXlu);
            if (pRep != NULL) {
                DrawMdlZReplacement(*pMdl, pMdlZ, numMdlZ, pRep, drawMode);
            } else {
                DrawMdlZ(*pMdl, pMdlZ, numMdlZ, drawMode);
            }
        } else if (pRep != NULL) {
            DrawByteCodeReplacement(*pMdl, pByteCodeXlu, pRep, drawMode);
        } else {
            DrawByteCode(*pMdl, pByteCodeXlu, drawMode);
        }
    }

    g3d_state_set_view_mtx_arrays(NULL, NULL, NULL);
}

namespace nw4r {
namespace g3d {

/* Orders translucent records by priority, then front to back, then by material. */
static bool CompareMdlZXlu(const MdlZ& lhs, const MdlZ& rhs) {
    if (lhs.priority < rhs.priority) {
        return true;
    }
    if (lhs.priority > rhs.priority) {
        return false;
    }
    if (lhs.Z < rhs.Z) {
        return true;
    }
    if (lhs.Z == rhs.Z && lhs.matID < rhs.matID) {
        return true;
    }
    return false;
}

/* Orders opaque records by priority, then back to front. */
static bool CompareMdlZOpa(const MdlZ& lhs, const MdlZ& rhs) {
    if (lhs.priority < rhs.priority) {
        return true;
    }
    if (lhs.priority > rhs.priority) {
        return false;
    }
    return lhs.Z > rhs.Z;
}

}  // namespace g3d
}  // namespace nw4r
