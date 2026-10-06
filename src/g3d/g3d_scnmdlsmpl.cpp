/*
 * g3d/g3d_scnmdlsmpl.cpp - nw4r g3d `ScnMdlSimple` scene model.
 * RANGE. .text 0x8007F0E4-0x80081188 (46 functions); extab, extabindex, .data 0x8058F0A0-0x8058F3D8 (opens on
 *   "g3d_scnmdlsmpl.cpp" with the class's assert texts), .sdata 0x80791210-0x80791228.  Both edges are tudiscover's
 *   weak codegen-fingerprint cuts and `.data` fragment boundaries; the 0x800810DC-0x80081188 tail is here because
 *   GetTypeObj reads the "ScnMdlSimple" name record (scn_typename_ScnMdlSimple).  The right edge 0x80081188 is where
 *   `g3d/g3d_scnobj.cpp` opens: ScnObj::CalcWorldMtx, ScnObj::CalcViewMtx and the ScnObj constructor (it stores that
 *   unit's ScnObj vtable and reads the `.sdata2` word 0x80795E60, which is its claim).
 * NAMES. The ScnMdlSimple members are nw4r's (the asserts name `ScnMdlSimple::SetAnmObj`, `mpAnmObjChr` ..
 *   `mpAnmObjTexSrt`, `mdl.IsValid()`, the vtable gives the virtual order, the map rows carry the manglings).
 *   AABB_ctor (0x80080F44) is a GUESS in the `MTX34_ctor`/`VEC3_ctor` scheme: it runs the two corner records'
 *   constructors, the element constructor of ScnObj's bounding-box array.  scnmdlsmpl_align32 is a GUESS (the
 *   32-byte round-up the matrix-array sizes use), world_mtx_attr_ignore_anm_trans is a GUESS (bit 27 of a world-matrix
 *   attribute, set when the ignore-animation-translation option is on), CalcPosture, CalcSkinning,
 *   G3dProcGatherScnObj .. G3dProcUpdateFrame, GetDrawMode and the GetCalcWorld* getters are GUESSES (the per-pass
 *   helpers G3dProc dispatches to and the field readers), as are the callees' names in `g3d/g3d_calcworld.h`,
 *   `g3d/g3d_calcview.h`, `g3d/g3d_calcmaterial.h`, `g3d/g3d_anmvis.h`, `g3d/fn_80075DCC.h` and `g3d/g3d_scnmdl.h`
 *   (res_mdl_get_info, res_mdl_info_num_pos_nrm_mtx, res_mdl_info_num_view_mtx, res_mdl_info_ref, world_mtx_attr_*,
 *   g3d_calc_world, g3d_calc_skinning, g3d_calc_view*, g3d_lc_*, g3d_dc_invalidate_range, g3d_calc_material_directly,
 *   g3d_apply_vis_anm_result, g3d_draw_res_mdl_directly, type_obj_set_name_anmchr).
 * RESIDUALS. Unwritten: 0x80080B10-0x80080B5C (the callback-timing setter: foreign units call its stem) and
 *   0x800810DC-0x80081120 (ICalcWorldCallback's destructor: defining it would emit the interface's vtable here).
 *   Written but unpaired while `g3d/g3d_scnmdl.cpp`'s C-style ScnMdl constructor and destructor call their stems:
 *   the constructor (fn_80080C60) and the destructor (dtor_80080F7C).
 *   Partial: SetAnmObj (retail's NOT_SPECIFIED case jumps into the per-type attach blocks, a goto shape rule 8
 *   forbids; the two-switch dispatch measures 88.2 and saves r25 through `_savegpr_25`/`_restgpr_25`, per-case
 *   inline attach helpers 34.3).  The constructor copies
 *   mResMdl inline where retail calls the out-of-line ResMdl copy constructor (0x80077E34).
 *   flipcheck: `.data` and `.sdata` are claimed and not emitted; the "ScnMdlSimple" name record (`.rodata`
 *   0x8056F688) has no registered owner.
 * SHAPES. The unit compiles with `#pragma peephole off` (retail keeps every `clrlwi` + `cmpwi` pair).
 *   GetNumViewMtx and GetCalcWorldNodeID return u32: their callers use the `lhz` result unmasked.
 */

#include "types.h"
#include "g3d/g3d_scnmdlsmpl.h" /* this unit's own declarations (rule 1) */
#include "mh3_pad.h"            /* VEC3_ctor (rule 2) */
#include "unsplit/g3d.h"        /* scn_typename_ScnMdlSimple, no registered owner (rule 2) */

/* 0x80080F44 (0x38): constructs the box's two corner records. */
extern "C" nw4r::math::AABB* AABB_ctor(nw4r::math::AABB* pBox)
{
    VEC3_ctor(&pBox->min);
    VEC3_ctor(&pBox->max);
    return pBox;
}

#include "g3d/g3d_obj.h"         /* nw4r::g3d::DynamicCast, AnmObjVis/MatClr/TexPat (rule 1) */
#include "g3d/g3d_anmchr.h"      /* nw4r::g3d::AnmObjChr, type_obj_set_name_anmchr (rule 2) */
#include "g3d/g3d_anmtexsrt.h"   /* nw4r::g3d::AnmObjTexSrt (rule 2) */
#include "g3d/g3d_calcworld.h"   /* the world-matrix calculator and its attribute helpers (rule 2) */
#include "g3d/g3d_calcview.h"    /* the view-matrix calculator and the locked-cache helpers (rule 2) */
#include "g3d/g3d_calcmaterial.h" /* g3d_calc_material_directly (rule 2) */
#include "g3d/g3d_anmvis.h"      /* g3d_apply_vis_anm_result (rule 2) */
#include "g3d/fn_80075DCC.h"     /* g3d_draw_res_mdl_directly (rule 2) */
#include "g3d/g3d_scnmdl.h"      /* res_mdl_info_ref (rule 2) */
#include "nw4r/fn_805012C4.h"    /* nw4r::ut::LC (rule 2) */
#include "g3d/g3d_resvtx.h"      /* nw4r::g3d::DC::StoreRange (rule 2) */
#include "nw4r/db_assert.h"      /* nw4r::db::Panic / Warning (rule 2) */

#pragma pool_data off
#pragma peephole off

using nw4r::g3d::ScnMdlSimple;

#define SMPL_POINTER_ASSERT(ptr, line, msg)                                                    \
    {                                                                                          \
        BOOL ok1_ = TRUE, ok2_ = TRUE, ok3_ = TRUE, ok4_ = TRUE, ok5_ = TRUE, ok6_ = TRUE;      \
        u32 top_ = (u32)(ptr) & 0xFF000000u;                                                    \
        if (!(top_ == 0x80000000u) && !(((u32)(ptr) & 0xFF800000u) == 0x81000000u))             \
            ok6_ = FALSE;                                                                        \
        if (!ok6_ && !(((u32)(ptr) & 0xF8000000u) == 0x90000000u))                              \
            ok5_ = FALSE;                                                                        \
        if (!ok5_ && !(top_ == 0xC0000000u))                                                    \
            ok4_ = FALSE;                                                                        \
        if (!ok4_ && !(((u32)(ptr) & 0xFF800000u) == 0xC1000000u))                              \
            ok3_ = FALSE;                                                                        \
        if (!ok3_ && !(((u32)(ptr) & 0xF8000000u) == 0xD0000000u))                              \
            ok2_ = FALSE;                                                                        \
        if (!ok2_ && !(((u32)(ptr) & 0xFFFFC000u) == 0xE0000000u))                              \
            ok1_ = FALSE;                                                                        \
        if (!ok1_)                                                                              \
            nw4r::db::Panic("g3d_scnmdlsmpl.cpp", line, msg, (ptr));                             \
    }

/* The fields of a model's `ResMdlInfo` block the constructor and Construct read.  size: 0x40 (approximation: only
 * the read fields are named) */
struct ScnMdlResMdlInfoData {
    /* +0x00 */ u8 pad_0x00[0x20];
    /* +0x20 */ u8 needNrmMtxArray;
    /* +0x21 */ u8 needTexMtxArray;
    /* +0x22 */ u8 isValidVolume;
    /* +0x23 */ u8 pad_0x23[0x5];
    /* +0x28 */ nw4r::math::VEC3 volumeMin;
    /* +0x34 */ nw4r::math::VEC3 volumeMax;
};

extern "C" {
u32 scnmdlsmpl_align32(u32 size);
u32 world_mtx_attr_ignore_anm_trans(u32 attrib);
}

/* 0x8007F0E4 (0x32C): sizes a model for `mdl` with `numView` views, reports the size through `pSize`, and builds it in
 * one block from `pHeap` (NULL without a valid model, a heap or memory). */
ScnMdlSimple* ScnMdlSimple::Construct(MEMAllocator* pHeap, u32* pSize, ResMdl mdl, int numView)
{
    if (!mdl.IsValid()) {
        return NULL;
    }
    if (numView == 0) {
        numView = 1;
    } else if (numView > 16) {
        numView = 16;
    }
    ScnMdlSimple* pMdl = NULL;
    u32 info0 = res_mdl_get_info(&mdl);
    u32 numMtx = res_mdl_info_num_pos_nrm_mtx(&info0);
    u32 info1 = res_mdl_get_info(&mdl);
    u32 numViewMtx = res_mdl_info_num_view_mtx(&info1);
    u32 worldMtxSize = numMtx * sizeof(math::MTX34);
    u32 worldAttribSize = numMtx * sizeof(u32);
    u32 viewMtxSize = numViewMtx * sizeof(math::MTX34);
    u32 viewPosSize = numView * scnmdlsmpl_align32(viewMtxSize);
    u32 viewNrmSize = numViewMtx * sizeof(math::MTX33);
    u32 info2 = res_mdl_get_info(&mdl);
    if (((const ScnMdlResMdlInfoData*)res_mdl_info_ref(&info2))->needNrmMtxArray) {
        viewNrmSize = numView * scnmdlsmpl_align32(viewNrmSize);
    } else {
        viewNrmSize = 0;
    }
    u32 info3 = res_mdl_get_info(&mdl);
    u32 viewTexSize;
    if (((const ScnMdlResMdlInfoData*)res_mdl_info_ref(&info3))->needTexMtxArray) {
        viewTexSize = numView * scnmdlsmpl_align32(viewMtxSize);
    } else {
        viewTexSize = 0;
    }
    u32 worldMtxOffset = scnmdlsmpl_align32(sizeof(ScnMdlSimple));
    u32 worldAttribOffset = scnmdlsmpl_align32(worldMtxOffset + worldMtxSize);
    u32 viewPosOffset = scnmdlsmpl_align32(worldAttribOffset + worldAttribSize);
    u32 viewNrmOffset = scnmdlsmpl_align32(viewPosOffset + viewPosSize);
    u32 viewTexOffset = scnmdlsmpl_align32(viewNrmOffset + viewNrmSize);
    u32 size = scnmdlsmpl_align32(viewTexOffset + viewTexSize);
    if (pSize != NULL) {
        SMPL_POINTER_ASSERT(pSize, 0x57, "NW4R:Pointer Error\npSize(=%p) is not valid pointer.");
        *pSize = size;
    }
    if (pHeap != NULL) {
        u8* pBuf = (u8*)Alloc(pHeap, size);
        if ((u32)pBuf & 0x1F) {
            nw4r::db::Panic("g3d_scnmdlsmpl.cpp", 0x62, "NW4R:Failed assertion ((u32)buf & 0x1f) == 0");
        }
        if (pBuf == NULL) {
            return NULL;
        }
        pMdl = new (pBuf) ScnMdlSimple(pHeap, mdl, (math::MTX34*)(pBuf + worldMtxOffset),
                                       (u32*)(pBuf + worldAttribOffset), (math::MTX34*)(pBuf + viewPosOffset),
                                       viewNrmSize ? (math::MTX33*)(pBuf + viewNrmOffset) : NULL,
                                       viewTexSize ? (math::MTX34*)(pBuf + viewTexOffset) : NULL, numView,
                                       numViewMtx);
    }
    return pMdl;
}

extern "C" {
/* 0x8007F410 (0xC): rounds `size` up to a multiple of 32. */
u32 scnmdlsmpl_align32(u32 size)
{
    return (size + 0x1F) & ~0x1F;
}
}

/* 0x8007F41C (0x2A0): computes the object's world matrix and the model's node matrices (in the locked cache when
 * they fit), running the CALC_WORLD callback before and after the object's own matrix. */
void ScnMdlSimple::CalcPosture(u32 param, const math::MTX34* pParent)
{
    CheckCallback_CALC_WORLD(CALLBACK_TIMING_A, param, (void*)pParent);
    CalcWorldMtx(pParent, &param);
    CheckCallback_CALC_WORLD(CALLBACK_TIMING_B, param, (void*)pParent);
    u32 ignoreTrans;
    u32 info;
    ResMdl mdl = GetResMdl();
    info = res_mdl_get_info(&mdl);
    u32 numMtx = res_mdl_info_num_pos_nrm_mtx(&info);
    bool lockedCache = false;
    math::MTX34* pWorldMtx;
    if (numMtx > 8 && numMtx <= 341 && (lockedCache = nw4r::ut::LC::Lock()) == true) {
        mFlagScnMdlSimple |= 1;
        g3d_dc_invalidate_range(GetWldMtxArray(), numMtx * sizeof(math::MTX34));
        pWorldMtx = (math::MTX34*)g3d_lc_base();
    } else {
        mFlagScnMdlSimple &= ~1;
        pWorldMtx = GetWldMtxArray();
    }
    ScaleProperty scale = GetScaleProperty();
    u32 rootAttrib = world_mtx_attr_root_mtx();
    GetScnObjOption(0x20001, &ignoreTrans);
    if (scale == UNIFORM_SCALED) {
        rootAttrib = world_mtx_attr_not_scale_one(rootAttrib);
    } else if (scale == NONUNIFORM_SCALED) {
        rootAttrib = world_mtx_attr_not_scale_uniform(rootAttrib);
    }
    if (ignoreTrans) {
        rootAttrib = world_mtx_attr_ignore_anm_trans(rootAttrib);
    }
    if (GetCalcWorldCallback()) {
        FuncObjCalcWorld funcObj(GetCalcWorldCallback(), GetCalcWorldCallbackTiming(), GetCalcWorldNodeID());
        ResMdl mdlArg = GetResMdl();
        g3d_calc_world(pWorldMtx, GetWldMtxAttribArray(), GetByteCodeCalc(), GetMtxPtr(MTX_WORLD), &mdlArg,
                       GetAnmObjChr(), &funcObj, rootAttrib);
    } else {
        ResMdl mdlArg = GetResMdl();
        g3d_calc_world(pWorldMtx, GetWldMtxAttribArray(), GetByteCodeCalc(), GetMtxPtr(MTX_WORLD), &mdlArg,
                       GetAnmObjChr(), NULL, rootAttrib);
    }
    if (lockedCache) {
        nw4r::ut::LC::StoreData(GetWldMtxArray(), (void*)0xE0000000, numMtx * sizeof(math::MTX34));
        nw4r::ut::LC::Unlock();
    }
}

/* 0x8007F6BC (0x8): the character animation. */
nw4r::g3d::AnmObjChr* ScnMdlSimple::GetAnmObjChr()
{
    return mpAnmObjChr;
}

/* 0x8007F6C4 (0x8): the node-tree byte code. */
const u8* ScnMdlSimple::GetByteCodeCalc()
{
    return mpByteCodeCalc;
}

/* 0x8007F6CC (0x8): the world matrices' attribute words. */
u32* ScnMdlSimple::GetWldMtxAttribArray()
{
    return mpWorldMtxAttribArray;
}

/* 0x8007F6D4 (0x80): records the callback, its timing mask and node, asserting both fit their fields. */
#pragma peephole off
nw4r::g3d::FuncObjCalcWorld::FuncObjCalcWorld(ICalcWorldCallback* pCallback, u32 timing, u32 nodeID)
    : mpCallback(pCallback), mTiming(timing), mNodeID(nodeID)
{
    bool valid = false;
    if (timing < 0x100 && nodeID < 0x10000) {
        valid = true;
    }
    if (!valid) {
        nw4r::db::Panic("g3d_calcworld.h", 0x8C, "NW4R:Failed assertion timing < 0x100 && nodeID < 0x10000");
    }
}


/* 0x8007F754 (0x8): the node the world callback runs for. */
u32 ScnMdlSimple::GetCalcWorldNodeID()
{
    return mCalcWorldNodeID;
}

/* 0x8007F75C (0x8): the world callback's timing mask. */
u8 ScnMdlSimple::GetCalcWorldCallbackTiming()
{
    return mCalcWorldCallbackTiming;
}

/* 0x8007F764 (0x8): the world-matrix callback. */
nw4r::g3d::ICalcWorldCallback* ScnMdlSimple::GetCalcWorldCallback()
{
    return mpCalcWorldCallback;
}

extern "C" {
/* 0x8007F76C (0x8): marks a world-matrix attribute as ignoring the animation's translation. */
u32 world_mtx_attr_ignore_anm_trans(u32 attrib)
{
    return attrib | 0x08000000;
}
}

/* 0x8007F774 (0x8): the world matrices. */
nw4r::math::MTX34* ScnMdlSimple::GetWldMtxArray()
{
    return mpWorldMtxArray;
}

/* 0x8007F77C (0x124): blends the skinned matrices from the node-mix byte code (in the locked cache when the world
 * matrices were calculated there). */
void ScnMdlSimple::CalcSkinning()
{
    if (GetByteCodeMix()) {
        u32 numMtx;
        math::MTX34* pWorldMtx;
        bool lockedCache = false;
        if ((mFlagScnMdlSimple & 1) && (lockedCache = nw4r::ut::LC::Lock()) == true) {
            u32 info;
            ResMdl mdl = GetResMdl();
            info = res_mdl_get_info(&mdl);
            numMtx = res_mdl_info_num_pos_nrm_mtx(&info);
            u32 size = numMtx * sizeof(math::MTX34);
            g3d_dc_invalidate_range(GetWldMtxArray(), size);
            pWorldMtx = (math::MTX34*)g3d_lc_base();
            nw4r::ut::LC::LoadData(pWorldMtx, GetWldMtxArray(), size);
            g3d_lc_queue_wait(0);
        } else {
            pWorldMtx = GetWldMtxArray();
        }
        ResMdl mdlArg = GetResMdl();
        g3d_calc_skinning(pWorldMtx, GetWldMtxAttribArray(), &mdlArg, GetByteCodeMix());
        if (lockedCache) {
            nw4r::ut::LC::StoreData(GetWldMtxArray(), (void*)0xE0000000, numMtx * sizeof(math::MTX34));
            nw4r::ut::LC::Unlock();
        }
    }
}

/* 0x8007F8A0 (0x8): the node-mix byte code. */
const u8* ScnMdlSimple::GetByteCodeMix()
{
    return mpByteCodeMix;
}

/* 0x8007F8A8 (0x7C): hands the model to the collector with its opaque and translucent gather flags. */
void ScnMdlSimple::G3dProcGatherScnObj(u32 param, IScnObjGather* pGather)
{
    pGather->Add(this, !TestScnObjFlag(SCNOBJFLAG_NOT_GATHER_DRAW_OPA), !TestScnObjFlag(SCNOBJFLAG_NOT_GATHER_DRAW_XLU));
}

/* 0x8007F924 (0x88): the world pass: the posture, the visibility animation, then the closing callback. */
void ScnMdlSimple::G3dProcCalcWorld(u32 param, const math::MTX34* pParent)
{
    CalcPosture(param, pParent);
    if (GetAnmObjVis()) {
        ResMdl mdlArg = GetResMdl();
        g3d_apply_vis_anm_result(&mdlArg, GetAnmObjVis());
    }
    CheckCallback_CALC_WORLD(CALLBACK_TIMING_C, param, (void*)pParent);
}

/* 0x8007F9AC (0xCC): the material pass: applies the texture-pattern, texture-SRT and colour animations. */
/* untyped: caller-owned payload - the pass's info block */
void ScnMdlSimple::G3dProcCalcMat(u32 param, void* pInfo)
{
    CheckCallback_CALC_MAT(CALLBACK_TIMING_A, param, pInfo);
    if (GetAnmObjTexPat() || GetAnmObjTexSrt() || GetAnmObjMatClr()) {
        ResMdl mdlArg = GetResMdl();
        g3d_calc_material_directly(&mdlArg, GetAnmObjTexPat(), GetAnmObjTexSrt(), GetAnmObjMatClr());
    }
    CheckCallback_CALC_MAT(CALLBACK_TIMING_C, param, pInfo);
}

/* 0x8007FA78 (0x230): the view pass: advances the view slot, computes the object's view matrix and the model's view
 * matrices (by locked-cache DMA when possible). */
void ScnMdlSimple::G3dProcCalcView(u32 param, const math::MTX34* pCamera)
{
    mCurView = (mCurView + 1) % mNumView;
    CheckCallback_CALC_VIEW(CALLBACK_TIMING_A, param, (void*)pCamera);
    CalcViewMtx(pCamera);
    CheckCallback_CALC_VIEW(CALLBACK_TIMING_B, param, (void*)pCamera);
    if (nw4r::ut::LC::Lock()) {
        if (mFlagScnMdlSimple & 1) {
            nw4r::g3d::DC::StoreRange(GetWldMtxArray(), GetNumViewMtx() * sizeof(math::MTX34));
            ResMdl mdlArg = GetResMdl();
            g3d_calc_view_lc_dma(GetViewPosMtxArray(), GetViewNrmMtxArray(), GetWldMtxArray(), GetWldMtxAttribArray(),
                                 GetNumViewMtx(), pCamera, &mdlArg, GetViewTexMtxArray());
        } else {
            ResMdl mdlArg = GetResMdl();
            g3d_calc_view_lc(GetViewPosMtxArray(), GetViewNrmMtxArray(), GetWldMtxArray(), GetWldMtxAttribArray(),
                             GetNumViewMtx(), pCamera, &mdlArg, GetViewTexMtxArray());
        }
        nw4r::ut::LC::Unlock();
    } else {
        ResMdl mdlArg = GetResMdl();
        g3d_calc_view(GetViewPosMtxArray(), GetViewNrmMtxArray(), GetWldMtxArray(), GetWldMtxAttribArray(),
                      GetNumViewMtx(), pCamera, &mdlArg, GetViewTexMtxArray());
    }
    CheckCallback_CALC_VIEW(CALLBACK_TIMING_C, param, (void*)pCamera);
}

/* 0x8007FCA8 (0x8): the number of view matrices per view. */
u32 ScnMdlSimple::GetNumViewMtx() const
{
    return mNumViewMtx;
}

/* 0x8007FCB0 (0xD4): the opaque draw pass, in the caller's draw mode or the model's. */
void ScnMdlSimple::G3dProcDrawOpa(u32 param, const u32* pDrawMode)
{
    CheckCallback_DRAW_OPA(CALLBACK_TIMING_A, param, (void*)pDrawMode);
    u32 drawMode = pDrawMode != NULL ? *pDrawMode : GetDrawMode();
    ResMdl mdlArg = GetResMdl();
    g3d_draw_res_mdl_directly(&mdlArg, GetViewPosMtxArray(), GetViewNrmMtxArray(), GetViewTexMtxArray(),
                              GetByteCodeDrawOpa(), NULL, NULL, drawMode);
    CheckCallback_DRAW_OPA(CALLBACK_TIMING_C, param, (void*)pDrawMode);
}

/* 0x8007FD84 (0xD4): the translucent draw pass, in the caller's draw mode or the model's. */
void ScnMdlSimple::G3dProcDrawXlu(u32 param, const u32* pDrawMode)
{
    CheckCallback_DRAW_XLU(CALLBACK_TIMING_A, param, (void*)pDrawMode);
    u32 drawMode = pDrawMode != NULL ? *pDrawMode : GetDrawMode();
    ResMdl mdlArg = GetResMdl();
    g3d_draw_res_mdl_directly(&mdlArg, GetViewPosMtxArray(), GetViewNrmMtxArray(), GetViewTexMtxArray(), NULL,
                              GetByteCodeDrawXlu(), NULL, drawMode);
    CheckCallback_DRAW_XLU(CALLBACK_TIMING_C, param, (void*)pDrawMode);
}

/* 0x8007FE58 (0x16C): runs the model's work for a pass unless the pass is disabled. */
/* untyped: caller-owned payload - the pass's info block */
void ScnMdlSimple::G3dProc(u32 task, u32 param, void* pInfo)
{
    if (IsG3dProcDisabled(task)) {
        return;
    }
    switch (task) {
    case G3DPROC_GATHER_SCNOBJ:
        G3dProcGatherScnObj(param, static_cast<IScnObjGather*>(pInfo));
        break;
    case G3DPROC_CALC_WORLD:
        G3dProcCalcWorld(param, static_cast<const math::MTX34*>(pInfo));
        break;
    case G3DPROC_CALC_MAT:
        G3dProcCalcMat(param, pInfo);
        break;
    case G3DPROC_CALC_VIEW:
        G3dProcCalcView(param, static_cast<const math::MTX34*>(pInfo));
        break;
    case G3DPROC_DRAW_OPA:
        G3dProcDrawOpa(param, static_cast<const u32*>(pInfo));
        break;
    case G3DPROC_DRAW_XLU:
        G3dProcDrawXlu(param, static_cast<const u32*>(pInfo));
        break;
    case G3DPROC_UPDATEFRAME:
        G3dProcUpdateFrame(param, pInfo);
        break;
    case G3DPROC_CHILD_DETACHED:
        RemoveAnmObj(static_cast<AnmObj*>(pInfo));
        break;
    default:
        DefG3dProcScnLeaf(task, param, pInfo);
        break;
    }
}

/* 0x8007FFC4 (0x40): the ignore-animation-translation option sets its flag; the rest are the leaf's. */
bool ScnMdlSimple::SetScnObjOption(u32 option, u32 value)
{
    if (option == 0x20001) {
        SetScnObjFlag((ScnObjFlag)0x100, value);
    } else {
        return ScnLeaf::SetScnObjOption(option, value);
    }
    return true;
}

/* 0x80080004 (0x60): the ignore-animation-translation option reads its flag; the rest are the leaf's. */
bool ScnMdlSimple::GetScnObjOption(u32 option, u32* pValue) const
{
    if (pValue == NULL) {
        return false;
    }
    if (option == 0x20001) {
        *pValue = TestScnObjFlag((ScnObjFlag)0x100);
    } else {
        return ScnLeaf::GetScnObjOption(option, pValue);
    }
    return true;
}

/* 0x80080064 (0x47C): attaches a bound animation to the slot its type (given or found) names; false for a NULL or
 * already attached object, a type mismatch or an unbound animation. */
bool ScnMdlSimple::SetAnmObj(AnmObj* pObj, AnmObjType type)
{
    AnmObjChr* pChr;
    AnmObjVis* pVis;
    AnmObjMatClr* pMatClr;
    AnmObjTexPat* pTexPat;
    AnmObjTexSrt* pTexSrt;
    if (pObj == NULL || pObj->GetParent() != NULL) {
        return false;
    }
    switch (type) {
    case ANMOBJTYPE_NOT_SPECIFIED:
        if ((pChr = DynamicCast<AnmObjChr>(pObj)) != NULL) {
            type = ANMOBJTYPE_CHR;
        } else if ((pVis = DynamicCast<AnmObjVis>(pObj)) != NULL) {
            type = ANMOBJTYPE_VIS;
        } else if ((pMatClr = DynamicCast<AnmObjMatClr>(pObj)) != NULL) {
            type = ANMOBJTYPE_MATCLR;
        } else if ((pTexPat = DynamicCast<AnmObjTexPat>(pObj)) != NULL) {
            type = ANMOBJTYPE_TEXPAT;
        } else if ((pTexSrt = DynamicCast<AnmObjTexSrt>(pObj)) != NULL) {
            type = ANMOBJTYPE_TEXSRT;
        } else {
            return false;
        }
        break;
    case ANMOBJTYPE_CHR:
        if ((pChr = DynamicCast<AnmObjChr>(pObj)) == NULL) {
            return false;
        }
        break;
    case ANMOBJTYPE_VIS:
        if ((pVis = DynamicCast<AnmObjVis>(pObj)) == NULL) {
            return false;
        }
        break;
    case ANMOBJTYPE_MATCLR:
        if ((pMatClr = DynamicCast<AnmObjMatClr>(pObj)) == NULL) {
            return false;
        }
        break;
    case ANMOBJTYPE_TEXPAT:
        if ((pTexPat = DynamicCast<AnmObjTexPat>(pObj)) == NULL) {
            return false;
        }
        break;
    case ANMOBJTYPE_TEXSRT:
        if ((pTexSrt = DynamicCast<AnmObjTexSrt>(pObj)) == NULL) {
            return false;
        }
        break;
    default:
        return false;
    }
    switch (type) {
    case ANMOBJTYPE_CHR:
        if (!pChr->IsBound()) {
            nw4r::db::Warning("g3d_scnmdlsmpl.cpp", 0x246,
                              "ScnMdlSimple::SetAnmObj does not 'Bind' AnmObjChr now. Please 'Bind' before call "
                              "SetAnmObj.");
            return false;
        }
        if (mpAnmObjChr != NULL) {
            RemoveAnmObj(mpAnmObjChr);
        }
        if (mpAnmObjChr != NULL) {
            nw4r::db::Panic("g3d_scnmdlsmpl.cpp", 0x24D, "NW4R:Failed assertion !mpAnmObjChr");
        }
        mpAnmObjChr = pChr;
        pChr->G3dProc(G3DPROC_ATTACH_PARENT, 0, this);
        return true;
    case ANMOBJTYPE_VIS:
        if (!pVis->IsBound()) {
            nw4r::db::Warning("g3d_scnmdlsmpl.cpp", 0x259,
                              "ScnMdlSimple::SetAnmObj does not 'Bind' AnmObjVis now. Please 'Bind' before call "
                              "SetAnmObj.");
            return false;
        }
        if (mpAnmObjVis != NULL) {
            RemoveAnmObj(mpAnmObjVis);
        }
        if (mpAnmObjVis != NULL) {
            nw4r::db::Panic("g3d_scnmdlsmpl.cpp", 0x260, "NW4R:Failed assertion !mpAnmObjVis");
        }
        mpAnmObjVis = pVis;
        pVis->G3dProc(G3DPROC_ATTACH_PARENT, 0, this);
        return true;
    case ANMOBJTYPE_MATCLR:
        if (!pMatClr->IsBound()) {
            return false;
        }
        if (mpAnmObjMatClr != NULL) {
            RemoveAnmObj(mpAnmObjMatClr);
        }
        if (mpAnmObjMatClr != NULL) {
            nw4r::db::Panic("g3d_scnmdlsmpl.cpp", 0x26D, "NW4R:Failed assertion !mpAnmObjMatClr");
        }
        mpAnmObjMatClr = pMatClr;
        pMatClr->G3dProc(G3DPROC_ATTACH_PARENT, 0, this);
        return true;
    case ANMOBJTYPE_TEXPAT:
        if (!pTexPat->IsBound()) {
            return false;
        }
        if (mpAnmObjTexPat != NULL) {
            RemoveAnmObj(mpAnmObjTexPat);
        }
        if (mpAnmObjTexPat != NULL) {
            nw4r::db::Panic("g3d_scnmdlsmpl.cpp", 0x27A, "NW4R:Failed assertion !mpAnmObjTexPat");
        }
        mpAnmObjTexPat = pTexPat;
        pTexPat->G3dProc(G3DPROC_ATTACH_PARENT, 0, this);
        return true;
    default:
        if (!pTexSrt->IsBound()) {
            return false;
        }
        if (mpAnmObjTexSrt != NULL) {
            RemoveAnmObj(mpAnmObjTexSrt);
        }
        if (mpAnmObjTexSrt != NULL) {
            nw4r::db::Panic("g3d_scnmdlsmpl.cpp", 0x287, "NW4R:Failed assertion !mpAnmObjTexSrt");
        }
        mpAnmObjTexSrt = pTexSrt;
        pTexSrt->G3dProc(G3DPROC_ATTACH_PARENT, 0, this);
        return true;
    }
}

/* 0x8008074C (0x178): detaches `pObj` from whichever slot holds it; false when no slot does. */
bool ScnMdlSimple::RemoveAnmObj(AnmObj* pObj)
{
    if (pObj == NULL) {
        return false;
    }
    if (pObj == mpAnmObjChr) {
        mpAnmObjChr->G3dProc(G3DPROC_DETACH_PARENT, 0, this);
        mpAnmObjChr = NULL;
        return true;
    }
    if (pObj == mpAnmObjVis) {
        mpAnmObjVis->G3dProc(G3DPROC_DETACH_PARENT, 0, this);
        mpAnmObjVis = NULL;
        return true;
    }
    if (pObj == mpAnmObjMatClr) {
        mpAnmObjMatClr->G3dProc(G3DPROC_DETACH_PARENT, 0, this);
        mpAnmObjMatClr = NULL;
        return true;
    }
    if (pObj == mpAnmObjTexPat) {
        mpAnmObjTexPat->G3dProc(G3DPROC_DETACH_PARENT, 0, this);
        mpAnmObjTexPat = NULL;
        return true;
    }
    if (pObj == mpAnmObjTexSrt) {
        mpAnmObjTexSrt->G3dProc(G3DPROC_DETACH_PARENT, 0, this);
        mpAnmObjTexSrt = NULL;
        return true;
    }
    return false;
}

/* 0x800808C4 (0xE0): detaches and returns the animation in slot `type`. */
nw4r::g3d::AnmObj* ScnMdlSimple::RemoveAnmObj(AnmObjType type)
{
    AnmObj* pObj = NULL;
    switch (type) {
    case ANMOBJTYPE_CHR:
        pObj = mpAnmObjChr;
        RemoveAnmObj(pObj);
        break;
    case ANMOBJTYPE_VIS:
        pObj = mpAnmObjVis;
        RemoveAnmObj(pObj);
        break;
    case ANMOBJTYPE_MATCLR:
        pObj = mpAnmObjMatClr;
        RemoveAnmObj(pObj);
        break;
    case ANMOBJTYPE_TEXPAT:
        pObj = mpAnmObjTexPat;
        RemoveAnmObj(pObj);
        break;
    case ANMOBJTYPE_TEXSRT:
        pObj = mpAnmObjTexSrt;
        RemoveAnmObj(pObj);
        break;
    }
    return pObj;
}

/* 0x800809A4 (0x5C): the animation in slot `type`. */
nw4r::g3d::AnmObj* ScnMdlSimple::GetAnmObj(AnmObjType type)
{
    switch (type) {
    case ANMOBJTYPE_CHR:
        return mpAnmObjChr;
    case ANMOBJTYPE_VIS:
        return mpAnmObjVis;
    case ANMOBJTYPE_MATCLR:
        return mpAnmObjMatClr;
    case ANMOBJTYPE_TEXPAT:
        return mpAnmObjTexPat;
    case ANMOBJTYPE_TEXSRT:
        return mpAnmObjTexSrt;
    default:
        return NULL;
    }
}

/* 0x80080A00 (0x5C): the animation in slot `type`. */
const nw4r::g3d::AnmObj* ScnMdlSimple::GetAnmObj(AnmObjType type) const
{
    switch (type) {
    case ANMOBJTYPE_CHR:
        return mpAnmObjChr;
    case ANMOBJTYPE_VIS:
        return mpAnmObjVis;
    case ANMOBJTYPE_MATCLR:
        return mpAnmObjMatClr;
    case ANMOBJTYPE_TEXPAT:
        return mpAnmObjTexPat;
    case ANMOBJTYPE_TEXSRT:
        return mpAnmObjTexSrt;
    default:
        return NULL;
    }
}

/* 0x80080A5C (0xB4): advances every attached animation's frame. */
void ScnMdlSimple::UpdateFrame()
{
    if (mpAnmObjChr != NULL) {
        mpAnmObjChr->UpdateFrame();
    }
    if (mpAnmObjVis != NULL) {
        mpAnmObjVis->UpdateFrame();
    }
    if (mpAnmObjMatClr != NULL) {
        mpAnmObjMatClr->UpdateFrame();
    }
    if (mpAnmObjTexPat != NULL) {
        mpAnmObjTexPat->UpdateFrame();
    }
    if (mpAnmObjTexSrt != NULL) {
        mpAnmObjTexSrt->UpdateFrame();
    }
}

/* 0x80080B5C (0x4C): the current view's position matrices. */
nw4r::math::MTX34* ScnMdlSimple::GetViewPosMtxArray()
{
    math::MTX34* pArray = mpViewPosMtxArray;
    return (math::MTX34*)((u8*)pArray + mCurView * scnmdlsmpl_align32(mNumViewMtx * sizeof(math::MTX34)));
}

/* 0x80080BA8 (0x5C): the current view's normal matrices, NULL when the model keeps none. */
nw4r::math::MTX33* ScnMdlSimple::GetViewNrmMtxArray()
{
    math::MTX33* pArray = mpViewNrmMtxArray;
    if (pArray != NULL) {
        return (math::MTX33*)((u8*)pArray + mCurView * scnmdlsmpl_align32(mNumViewMtx * sizeof(math::MTX33)));
    }
    return NULL;
}

/* 0x80080C04 (0x5C): the current view's texture matrices, NULL when the model keeps none. */
nw4r::math::MTX34* ScnMdlSimple::GetViewTexMtxArray()
{
    math::MTX34* pArray = mpViewTexMtxArray;
    if (pArray != NULL) {
        return (math::MTX34*)((u8*)pArray + mCurView * scnmdlsmpl_align32(mNumViewMtx * sizeof(math::MTX34)));
    }
    return NULL;
}

/* 0x80080C60 (0x2D8): constructs the model over its caller-provided matrix arrays, picks up its byte codes, flushes
 * the view arrays and takes the resource's bounding box. */
ScnMdlSimple::ScnMdlSimple(MEMAllocator* pHeap, ResMdl mdl, math::MTX34* pWorldMtxArray, u32* pWorldMtxAttribArray,
                           math::MTX34* pViewPosMtxArray, math::MTX33* pViewNrmMtxArray,
                           math::MTX34* pViewTexMtxArray, int numView, int numViewMtx)
    : ScnLeaf(pHeap),
      mResMdl(mdl),
      mpWorldMtxArray(pWorldMtxArray),
      mpWorldMtxAttribArray(pWorldMtxAttribArray),
      mpViewPosMtxArray(pViewPosMtxArray),
      mpViewNrmMtxArray(pViewNrmMtxArray),
      mpViewTexMtxArray(pViewTexMtxArray),
      mNumView(numView),
      mCurView(0),
      mNumViewMtx(numViewMtx),
      mFlagScnMdlSimple(0),
      mpByteCodeCalc(mdl.GetResByteCode("NodeTree")),
      mpByteCodeMix(mdl.GetResByteCode("NodeMix")),
      mpByteCodeDrawOpa(mdl.GetResByteCode("DrawOpa")),
      mpByteCodeDrawXlu(mdl.GetResByteCode("DrawXlu")),
      mDrawMode(2),
      mpCalcWorldCallback(NULL),
      mCalcWorldCallbackTiming(0),
      mCalcWorldCallbackDeleteOption(0),
      mCalcWorldNodeID(0),
      mpAnmObjChr(NULL),
      mpAnmObjVis(NULL),
      mpAnmObjMatClr(NULL),
      mpAnmObjTexPat(NULL),
      mpAnmObjTexSrt(NULL)
{
    bool aligned = false;
    if ((u32)mpViewPosMtxArray % 32 == 0 && (u32)mpViewNrmMtxArray % 32 == 0) {
        aligned = true;
    }
    if (!aligned) {
        nw4r::db::Panic("g3d_scnmdlsmpl.cpp", 0x3F1,
                        "NW4R:Failed assertion (u32)mpViewPosMtxArray % 32 == 0 && (u32)mpViewNrmMtxArray % 32 == 0");
    }
    if (!mdl.IsValid()) {
        nw4r::db::Panic("g3d_scnmdlsmpl.cpp", 0x3F2, "NW4R:Failed assertion mdl.IsValid()");
    }
    if (mpByteCodeDrawOpa) {
        SetScnObjFlag(SCNOBJFLAG_NOT_GATHER_DRAW_OPA, false);
    } else {
        SetScnObjFlag(SCNOBJFLAG_NOT_GATHER_DRAW_OPA, true);
    }
    if (mpByteCodeDrawXlu) {
        SetScnObjFlag(SCNOBJFLAG_NOT_GATHER_DRAW_XLU, false);
    } else {
        SetScnObjFlag(SCNOBJFLAG_NOT_GATHER_DRAW_XLU, true);
    }
    if (mpViewPosMtxArray) {
        g3d_dc_invalidate_range(mpViewPosMtxArray, numView * scnmdlsmpl_align32(numViewMtx * sizeof(math::MTX34)));
    }
    if (mpViewNrmMtxArray) {
        g3d_dc_invalidate_range(mpViewNrmMtxArray, numView * scnmdlsmpl_align32(numViewMtx * sizeof(math::MTX33)));
    }
    if (mpViewTexMtxArray) {
        g3d_dc_invalidate_range(mpViewTexMtxArray, numView * scnmdlsmpl_align32(numViewMtx * sizeof(math::MTX34)));
    }
    u32 info = res_mdl_get_info(&mdl);
    if (((const ScnMdlResMdlInfoData*)res_mdl_info_ref(&info))->isValidVolume) {
        info = res_mdl_get_info(&mdl);
        const ScnMdlResMdlInfoData* pMinInfo = (const ScnMdlResMdlInfoData*)res_mdl_info_ref(&info);
        info = res_mdl_get_info(&mdl);
        const ScnMdlResMdlInfoData* pMaxInfo = (const ScnMdlResMdlInfoData*)res_mdl_info_ref(&info);
        math::AABB box;
        AABB_ctor(&box);
        box.min.x = pMinInfo->volumeMin.x;
        box.min.y = pMinInfo->volumeMin.y;
        box.min.z = pMinInfo->volumeMin.z;
        box.max.x = pMaxInfo->volumeMax.x;
        box.max.y = pMaxInfo->volumeMax.y;
        box.max.z = pMaxInfo->volumeMax.z;
        SetBoundingVolume(&box);
    }
}

/* 0x80080F38 (0xC): sets the local bounding box. */
bool nw4r::g3d::ScnObj::SetBoundingVolume(const math::AABB* pAABB)
{
    return SetBoundingVolume(BOUNDINGVOLUME_AABB_LOCAL, pAABB);
}

/* 0x80080F7C (0x160): asserts the model is detached, deletes an owned world callback and detaches every animation. */
#pragma peephole off
ScnMdlSimple::~ScnMdlSimple()
{
    if (GetParent()) {
        nw4r::db::Panic("g3d_scnmdlsmpl.cpp", 0x437, "NW4R:Failed assertion !GetParent()");
    }
    if ((int)mCalcWorldCallbackDeleteOption == 1) {
        delete mpCalcWorldCallback;
    }
    if (mpAnmObjChr != NULL) {
        RemoveAnmObj(mpAnmObjChr);
    }
    if (mpAnmObjVis != NULL) {
        RemoveAnmObj(mpAnmObjVis);
    }
    if (mpAnmObjMatClr != NULL) {
        RemoveAnmObj(mpAnmObjMatClr);
    }
    if (mpAnmObjTexPat != NULL) {
        RemoveAnmObj(mpAnmObjTexPat);
    }
    if (mpAnmObjTexSrt != NULL) {
        RemoveAnmObj(mpAnmObjTexSrt);
    }
}


/* 0x80081120 (0x30): returns the type. */
const nw4r::g3d::G3dObj::TypeObj ScnMdlSimple::GetTypeObj() const
{
    void* pName;
    return *reinterpret_cast<const TypeObj*>(type_obj_set_name_anmchr(&pName, scn_typename_ScnMdlSimple));
}

/* 0x80081150 (0x38): returns the type's name. */
const char* ScnMdlSimple::GetTypeName() const
{
    return GetTypeObj().GetTypeName();
}
