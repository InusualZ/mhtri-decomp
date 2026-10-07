/*
 * g3d/g3d_scnmdl.cpp - nw4r g3d `ScnMdl` scene model: its replaced-material (`mReplacement`) buffers, the node
 *   visibility and option accessors, the constructor and destructor, and the `ScnMdl` name-record cluster.
 * RANGE. .text 0x8007C540-0x8007F0E4 (55 functions); extab, extabindex, .data 0x8058EDA0-0x8058F0A0 (opens on
 *   "g3d_scnmdl.cpp"), .sdata 0x80791208-0x80791210.  The left edge is `g3d/fn_80075DCC.cpp`'s cap, not a proven
 *   seam; tudiscover proves one TU through 0x8007EF1C, and the 0x8007EF1C-0x8007F0E4 tail is here because its head
 *   GetTypeObj reads the "ScnMdl" name record (scn_typename_ScnMdl), which only the class's own TU registers.
 * NAMES. The ScnMdl members are nw4r's (`src/nw4r/g3d/scnmdl.h`: the vtable gives the virtual order, the asserts name
 *   `mpAnmObjShp` and the `mReplacement.*Array` buffers, the map rows carry the manglings).  G3dProcCalcWorld,
 *   G3dProcCalcMat, G3dProcCalcVtx, G3dProcDrawOpa, G3dProcDrawXlu, IsVisBufferRefreshNeeded, IsVisBufferEnabled,
 *   UpdateVisBuffer, TestMatBufferFlag and GetAnmObjShp are GUESSES (the per-pass helpers G3dProc dispatches to and the
 *   flag and buffer readers); `g3d_root_model_bind` (0x8007F0CC) is a GUESS (it appends an object to the scene
 *   root through ScnGroup::Insert at the child count).
 *   res_mdl_info_ref is a GUESS (0x8007D404: the ResMdlInfo handle's block, asserting the handle).  The ScnMdlSimple
 *   getters, G3dProcUpdateFrame and IsDerivedFrom defined here are its weak copies (members of
 *   `g3d/g3d_scnmdlsmpl.h`), as is AnmObj::IsBound; ScnGroup::PopBack and ScnGroup::Empty (0x8007F05C, 0x8007F0BC)
 *   are `g3d/g3d_scnobj.h`'s.
 * RESIDUALS. Unwritten (objdiff scores them zero): fn_8007C540 (ScnMdl::Construct, 0xE4C), G3dProcCalcMat (0x590),
 *   fn_8007E01C (0x45C, the replacement-buffer initialiser Construct ends with) and fn_8007E498 (0x364, the
 *   per-material buffer refill G3dProcCalcMat calls; `sound/mhchar.cpp` calls its stem).  They need the ResMat
 *   accessors' remaining names.
 *   Partial: RemoveAnmObj(AnmObj*) (the vertex-position loop's counter and destination swap r27/r28).
 *   flipcheck: `.text` is short of the four unwritten rows; `.data` and `.sdata` are claimed and not emitted; the
 *   "ScnMdl" name record (`.rodata` 0x8056F678) has no registered owner.
 * SHAPES. The unit compiles with `#pragma peephole off` throughout (retail keeps the unfused `clrlwi` + `cmpwi`, `extsh`,
 *   `addi r0` vtable-store and `mr r3` + `lwz r12,0(r3)` virtual-call forms; playbook idea 106).
 */

#include "types.h"
#include "nw4r/g3d/res_common.h" /* ResHandle, IS_VALID_PTR (rule 1) */
#include "nw4r/g3d/scnmdl.h"      /* nw4r::g3d::ScnMdl (rule 1) */
#include "g3d/fn_80063888.h"      /* fn_800649B4 (rule 2: owner g3d/fn_80063888.cpp) */
#include "g3d/g3d_anmchr.h"       /* G3dObj::operator delete, TypeObj::GetTypeName, type_obj_set_name, TypeObj::operator==, G3dObj (rule 2: owner g3d/g3d_anmchr.cpp) */
#include "g3d/fn_800680CC.h"      /* fn_800696E4, fn_800697A4 (rule 2: owner g3d/fn_800680CC.cpp) */
#include "g3d/fn_80075DCC.h"      /* fn_8007B734..g3d_draw_res_mdl_directly (rule 2: owner g3d/fn_80075DCC.cpp) */
#include "g3d/g3d_scnobj.h"       /* nw4r::g3d::ScnObj / ScnLeaf / ScnGroup (rule 1) */
#include "g3d/g3d_anmvis.h"      /* g3d_apply_vis_anm_result/fn_8006ED84 (rule 2: owner g3d/g3d_anmvis.cpp) */
#include "g3d/g3d_calcview.h"     /* fn_8006FFBC/fn_8006FFC8 (rule 2: owner g3d/g3d_calcview.cpp) */
#include "g3d/g3d_calcvtx.h"       /* fn_8007270C (rule 2: g3d_calcvtx.cpp) */
#include "g3d/g3d_resvtx.h"
#include "g3d/g3d_resmat.h"
#include "g3d/g3d_resnode.h"
#include "g3d/g3d_scnmdlsmpl.h"   /* nw4r::g3d::ScnMdlSimple (rule 2) */
#include "unsplit/g3d.h"           /* scn_typename_ScnMdl, no registered owner (rule 2) */

#pragma peephole off

/* `ScnMdl` lives in its real namespace (rule 9's owner spelling); this unit's bodies name it short. */
using nw4r::g3d::ReplacementBlock;
using nw4r::g3d::ScnMdl;
using nw4r::g3d::ResMdl;

namespace nw4r {
namespace db {

/* `Panic(const char* pFile, int line, const char* pFmt, ...)` - the trailing `e` in the map's mangling
 * is MWCC's vararg marker.  Declared in the namespace (C++ linkage), never as the mangled spelling
 * (rule 9). */
void Panic(const char* pFile, int line, const char* pFmt, ...);

}  // namespace db
}  // namespace nw4r

/* The panic file/format strings and data objects this unit's bodies reference.  They are `extern` map
 * labels, not literals: MWCC's `-str reuse` would pool a repeated literal into one blob addressed
 * through a shared base register, while the target loads each one with its own `lis`/`addi`. */
extern const char lbl_8058EDA0[]; /* "g3d_scnmdl.cpp" */
extern const char lbl_8058EDE4[]; /* "NW4R:Failed assertion ((u32)buf & 0x1f) == 0" */
extern const char lbl_8058EE14[]; /* "NW4R:Failed assertion pos.GetSize() == ResVtxPos(rep.vtxPosTable..." */
extern const char lbl_8058EE64[]; /* "...((u32)&mReplacement.pixDLArray[i] & 0x1f) == 0" */
extern const char lbl_8058EEAC[]; /* "...((u32)&mReplacement.tevColorDLArray[i] & 0x1f) == 0" */
extern const char lbl_8058EEF8[]; /* "...((u32)&mReplacement.indMtxAndScaleDLArray[i] & 0x1f) == 0" */
extern const char lbl_8058EF48[]; /* "...((u32)&mReplacement.texCoordGenDLArray[i] & 0x1f) == 0" */
extern const char lbl_8058EF98[]; /* "...((u32)&mReplacement.tevDataArray[i] & 0x1f) == 0" */
extern const char lbl_8058EFE0[]; /* "NW4R:Failed assertion !mpAnmObjShp" */
extern const char lbl_8058F004[]; /* "NW4R:Failed assertion !GetParent()" */
extern const char lbl_8058F070[]; /* "%s::%s: Object not valid." */
extern const char lbl_8058F090[]; /* "g3d_resmdl_ac.h" */
extern const char lbl_80791208[4]; /* the `.sdata` string "ref" */

/* ------------------------------------------------------------------------------------------------ */
/* the object                                                                                        */
/* ------------------------------------------------------------------------------------------------ */

/* The resource block `fn_800730D8`/fn_800732F0/fn_800696E4 hand back: its leading word only.  The
 * layout belongs to g3d/g3d_calcvtx.cpp (rule 1), which defines the full block. */
struct ResVtxBlockHead {
    /* +0x00 */ u32 mSize;
}; /* size: 0x04 */

/* ------------------------------------------------------------------------------------------------ */
/* this unit's own bodies, in address order                                                          */
/* ------------------------------------------------------------------------------------------------ */

extern "C" {

/* -------- the two node-table helpers of `g3d/g3d_resmat.cpp`, declared here -------- */
u32 fn_8009A2F4(void* pSelf, u32 flag);  /* 0x8009A2F4 - the pix-DL replacement's teardown */
u32 fn_8009435C(void* pSelf, u32 flag);  /* 0x8009435C - the tex-color-DL replacement's teardown */
u32 fn_8009411C(void* pSelf, u32 flag);  /* 0x8009411C - the ind-mtx/scale replacement's teardown */

u32 fn_8007D38C(u32 p);
u32 fn_8007D468(const ResHandle* pSelf);
u32 fn_8007D470(u32 p);
u32* fn_8007DB3C(u32* pDst, const u32* pSrc);
void fn_8007DB6C(u32* pDst, const u32* pSrc);
u32* fn_8007DB78(u32* pDst, const u32* pSrc);
void fn_8007DBA8(u32* pDst, const u32* pSrc);
u32 fn_8007E478(ScnMdl* pSelf);
u32 fn_8007E480(ScnMdl* pSelf);
u32 fn_8007E488(ScnMdl* pSelf);
void fn_8007E490(void);
void fn_8007E494(void);

/* 0x8007D38C - the block pointer aligned up to 4 (the `buf & 0x3` shape the asserts below test). */
u32 fn_8007D38C(u32 p) {
    return (p + 3) & 0xFFFFFFFC;
}

} /* extern "C" */

/* 0x8007D398 (0x24): returns the colour block's size. */
u32 nw4r::g3d::ResVtxClr::GetSize() const {
    return ref().size;
}

/* 0x8007D3BC (0x24): returns the normal block's size. */
u32 nw4r::g3d::ResVtxNrm::GetSize() const {
    return ref().size;
}

/* 0x8007D3E0 (0x24): returns the position block's size. */
u32 nw4r::g3d::ResVtxPos::GetSize() const {
    return ref().size;
}

extern "C" {

/* 0x8007D404 - `ResCommon<ResMdl>::ref()`: the `g3d_resmdl_ac.h` inlined assert, then the handle. */
/* untyped: opaque handle - the info handle */
u32 res_mdl_info_ref(const void* pInfo) {
    ResHandle* pSelf = (ResHandle*)pInfo;

    if (fn_8006FFC8(pSelf) == 0) {
        nw4r::db::Panic(lbl_8058F090, 57, lbl_8058F070, fn_8006FFBC(), lbl_80791208);
    }
    return fn_8007D468(pSelf);
}

/* 0x8007D468 - the handle's resource pointer. */
u32 fn_8007D468(const ResHandle* pSelf) {
    return (u32)pSelf->mpData;
}

/* 0x8007D470 - the block pointer aligned up to 32 (the `buf & 0x1f` shape). */
u32 fn_8007D470(u32 p) {
    return (p + 31) & 0xFFFFFFE0;
}

} /* extern "C" */

/* 0x8007D47C (0xEC): the world pass: the posture, the node-visibility buffer refresh, the visibility animation (into
 * the buffer when the model keeps one), then the closing callback. */
void ScnMdl::G3dProcCalcWorld(u32 param, const nw4r::math::MTX34* pParent)
{
    CalcPosture(param, pParent);
    if (IsVisBufferEnabled() && IsVisBufferRefreshNeeded()) {
        UpdateVisBuffer();
    }
    if (GetAnmObjVis() != NULL) {
        if (mReplacement.mpNodeVisible != NULL) {
            nw4r::g3d::ResMdl local = GetResMdl();

            fn_8006ED84(mReplacement.mpNodeVisible, &local, (u32*)GetAnmObjVis());
            fn_8007C464(this);
        } else {
            nw4r::g3d::ResMdl local = GetResMdl();

            g3d_apply_vis_anm_result(&local, (void*)GetAnmObjVis());
        }
    }
    CheckCallback_CALC_WORLD(CALLBACK_TIMING_C, param, (void*)pParent);
}

/* 0x8007D568 (0x8): the visibility animation. */
nw4r::g3d::AnmObjVis* nw4r::g3d::ScnMdlSimple::GetAnmObjVis()
{
    return mpAnmObjVis;
}

/* 0x8007D570 (0x18): whether the node-visibility buffer needs a refresh. */
bool ScnMdl::IsVisBufferRefreshNeeded() const
{
    return (mFlags & 1) != 0;
}

/* 0x8007D588 (0x14): whether the node-visibility buffer is in use. */
bool ScnMdl::IsVisBufferEnabled() const
{
    return (mFlags & 2) == 0;
}

/* 0x8007DB2C (0x8): the material-colour animation. */
nw4r::g3d::AnmObjMatClr* nw4r::g3d::ScnMdlSimple::GetAnmObjMatClr()
{
    return mpAnmObjMatClr;
}

/* 0x8007DB34 (0x8): the texture-SRT animation. */
nw4r::g3d::AnmObjTexSrt* nw4r::g3d::ScnMdlSimple::GetAnmObjTexSrt()
{
    return mpAnmObjTexSrt;
}

extern "C" {

/* 0x8007DB3C - the word copy that hands the destination back (the `mReplacement` setter's shape). */
u32* fn_8007DB3C(u32* pDst, const u32* pSrc) {
    fn_8007DB6C(pDst, pSrc);
    return pDst;
}

/* 0x8007DB6C - a one-word copy. */
void fn_8007DB6C(u32* pDst, const u32* pSrc) {
    *pDst = *pSrc;
}

/* 0x8007DB78 - the same copy shape for the second replacement buffer. */
u32* fn_8007DB78(u32* pDst, const u32* pSrc) {
    fn_8007DBA8(pDst, pSrc);
    return pDst;
}

/* 0x8007DBA8 - a one-word copy. */
void fn_8007DBA8(u32* pDst, const u32* pSrc) {
    *pDst = *pSrc;
}

} /* extern "C" */

/* 0x8007DBB4 (0x8): the texture-pattern animation. */
nw4r::g3d::AnmObjTexPat* nw4r::g3d::ScnMdlSimple::GetAnmObjTexPat()
{
    return mpAnmObjTexPat;
}

/* 0x8007DBBC (0x20): whether material `idx`'s buffer flag word has any of `mask`'s bits. */
bool ScnMdl::TestMatBufferFlag(u32 idx, u32 mask) const
{
    return (mpDLBuffer[idx] & mask) != 0;
}

/* 0x8007DBDC (0xD4): the opaque draw pass with the model's replacement buffers. */
void ScnMdl::G3dProcDrawOpa(u32 param, const u32* pDrawMode)
{
    u32 key;

    CheckCallback_DRAW_OPA(CALLBACK_TIMING_A, param, (void*)pDrawMode);
    if (pDrawMode != 0) {
        key = *pDrawMode;
    } else {
        key = GetDrawMode();
    }
    nw4r::g3d::ResMdl handle = GetResMdl();
    g3d_draw_res_mdl_directly(&handle, GetViewPosMtxArray(), GetViewNrmMtxArray(), GetViewTexMtxArray(),
                              GetByteCodeDrawOpa(), NULL, &mReplacement, key);
    CheckCallback_DRAW_OPA(CALLBACK_TIMING_C, param, (void*)pDrawMode);
}

/* 0x8007DCB0 (0x8): the opaque draw byte code. */
const u8* nw4r::g3d::ScnMdlSimple::GetByteCodeDrawOpa()
{
    return mpByteCodeDrawOpa;
}

/* 0x8007DCB8 (0xD4): the translucent draw pass with the model's replacement buffers. */
void ScnMdl::G3dProcDrawXlu(u32 param, const u32* pDrawMode)
{
    u32 key;

    CheckCallback_DRAW_XLU(CALLBACK_TIMING_A, param, (void*)pDrawMode);
    if (pDrawMode != 0) {
        key = *pDrawMode;
    } else {
        key = GetDrawMode();
    }
    nw4r::g3d::ResMdl handle = GetResMdl();
    g3d_draw_res_mdl_directly(&handle, GetViewPosMtxArray(), GetViewNrmMtxArray(), GetViewTexMtxArray(), NULL,
                              GetByteCodeDrawXlu(), &mReplacement, key);
    CheckCallback_DRAW_XLU(CALLBACK_TIMING_C, param, (void*)pDrawMode);
}

/* 0x8007DD8C (0x8): the translucent draw byte code. */
const u8* nw4r::g3d::ScnMdlSimple::GetByteCodeDrawXlu()
{
    return mpByteCodeDrawXlu;
}

/* 0x8007DD94 (0x60): the vertex pass: blends the shape animation into the replacement vertex tables. */
/* untyped: caller-owned payload - the pass's info block */
void ScnMdl::G3dProcCalcVtx(u32 param, void* pInfo)
{
    if (GetAnmObjShp() != NULL) {
        nw4r::g3d::ResMdl local = GetResMdl();

        fn_8007270C(&local, (void*)GetAnmObjShp(), (const void**)mReplacement.mpVtxPosTable,
                    (const void**)mReplacement.mpVtxNrmTable, (const void**)mReplacement.mpVtxClrTable);
    }
}

/* 0x8007DDF4 (0x8): the shape animation. */
nw4r::g3d::AnmObjShp* ScnMdl::GetAnmObjShp()
{
    return mpAnmObjShp;
}

/* 0x8007DDFC (0x19C): runs the model's work for a pass unless the pass is disabled. */
/* untyped: caller-owned payload - the pass's info block */
void ScnMdl::G3dProc(u32 task, u32 param, void* pInfo)
{
    if (IsG3dProcDisabled(task)) {
        return;
    }
    switch (task) {
    case G3DPROC_GATHER_SCNOBJ:
        G3dProcGatherScnObj(param, static_cast<nw4r::g3d::IScnObjGather*>(pInfo));
        break;
    case G3DPROC_CALC_WORLD:
        G3dProcCalcWorld(param, static_cast<const nw4r::math::MTX34*>(pInfo));
        break;
    case G3DPROC_CALC_MAT:
        G3dProcCalcMat(param, pInfo);
        break;
    case G3DPROC_CALC_VIEW:
        G3dProcCalcView(param, static_cast<const nw4r::math::MTX34*>(pInfo));
        break;
    case G3DPROC_DRAW_OPA:
        G3dProcDrawOpa(param, static_cast<const u32*>(pInfo));
        break;
    case G3DPROC_DRAW_XLU:
        G3dProcDrawXlu(param, static_cast<const u32*>(pInfo));
        break;
    case G3DPROC_UPDATEFRAME:
        G3dProcUpdateFrame(param, pInfo);
        if (mpAnmObjShp != NULL) {
            mpAnmObjShp->UpdateFrame();
        }
        break;
    case G3DPROC_CHILD_DETACHED:
        RemoveAnmObj(static_cast<AnmObj*>(pInfo));
        break;
    case G3DPROC_CALC_VTX:
        G3dProcCalcVtx(param, pInfo);
        break;
    default:
        DefG3dProcScnLeaf(task, param, pInfo);
        break;
    }
}

/* 0x8007DF98 (0x4): the frame-update pass: advances the attached animations. */
/* untyped: caller-owned payload - the pass's info block */
void nw4r::g3d::ScnMdlSimple::G3dProcUpdateFrame(u32 param, void* pInfo)
{
    UpdateFrame();
}

/* 0x8007DF9C (0x40): the visibility-buffer option clears or sets its disable bit; the rest are ScnMdlSimple's. */
bool ScnMdl::SetScnObjOption(u32 option, u32 value)
{
    if (option == 0x30001) {
        if (value != 0) {
            mFlags &= ~2u;
        } else {
            mFlags |= 2u;
        }
    } else {
        return ScnMdlSimple::SetScnObjOption(option, value);
    }
    return true;
}

/* 0x8007DFDC (0x40): the visibility-buffer option reads its bit; the rest are ScnMdlSimple's. */
bool ScnMdl::GetScnObjOption(u32 option, u32* pValue) const
{
    u32 flag;

    if (pValue == 0) {
        return false;
    }
    if (option == 0x30001) {
        flag = mFlags & 2;
        *pValue = !flag;
    } else {
        return ScnMdlSimple::GetScnObjOption(option, pValue);
    }
    return true;
}

extern "C" {

/* 0x8007E478 - the pix-DL replacement's teardown (the shared destructor, flag 0). */
u32 fn_8007E478(ScnMdl* pSelf) {
    return fn_8009A2F4(pSelf, 0);
}

/* 0x8007E480 - the tex-color-DL replacement's teardown. */
u32 fn_8007E480(ScnMdl* pSelf) {
    return fn_8009435C(pSelf, 0);
}

/* 0x8007E488 - the indirect-matrix/scale replacement's teardown. */
u32 fn_8007E488(ScnMdl* pSelf) {
    return fn_8009411C(pSelf, 0);
}

/* 0x8007E490 - an empty body (`blr`): the range's no-op override. */
void fn_8007E490(void) {}

/* 0x8007E494 - an empty body (`blr`): the range's second no-op override. */
void fn_8007E494(void) {}

} /* extern "C" */

/* 0x8007E7FC (0xB8): writes one byte per node into the visibility buffer (1 where the node is visible) and clears
 * the refresh bit. */
void ScnMdl::UpdateVisBuffer()
{
    u32 view;
    nw4r::g3d::ResMdl handle = GetResMdl();
    s32 numNodes;
    u32 i;

    fn_80077E34((s32)&view, &handle);
    numNodes = reinterpret_cast<nw4r::g3d::ResMdl*>(&view)->GetResNodeNumEntries();
    if (mReplacement.mpNodeVisible != 0) {
        for (i = 0; i < (u32)numNodes; i++) {
            nw4r::g3d::ResNode node = reinterpret_cast<nw4r::g3d::ResMdl*>(&view)->GetResNode(i);

            if (fn_80078904((s32)&node) != 0) {
                mReplacement.mpNodeVisible[i] = 1;
            } else {
                mReplacement.mpNodeVisible[i] = 0;
            }
        }
    }
    mFlags &= ~1u;
}

/* 0x8007E8B4 (0x154): attaches a bound shape animation (given or found) to its slot and marks the vertex tables for
 * refreshing; any other animation goes to ScnMdlSimple. */
bool ScnMdl::SetAnmObj(AnmObj* pObj, AnmObjType type)
{
    if (pObj != NULL && pObj->GetParent() == NULL) {
        if (type == ANMOBJTYPE_SHP || type == ANMOBJTYPE_NOT_SPECIFIED) {
            AnmObjShp* pShp = nw4r::g3d::DynamicCast<AnmObjShp>(pObj);
            if (pShp != NULL) {
                if (!pShp->IsBound()) {
                    return false;
                }
                if (mpAnmObjShp != NULL) {
                    RemoveAnmObj(mpAnmObjShp);
                }
                if (mpAnmObjShp != NULL) {
                    nw4r::db::Panic(lbl_8058EDA0, 0x5B2, lbl_8058EFE0);
                }
                mpAnmObjShp = pShp;
                pShp->G3dProc(G3DPROC_ATTACH_PARENT, 0, this);
                mReplacement.mFlag &= ~1;
                return true;
            }
            if (type == ANMOBJTYPE_NOT_SPECIFIED) {
                return ScnMdlSimple::SetAnmObj(pObj, type);
            }
            return false;
        } else {
            return ScnMdlSimple::SetAnmObj(pObj, type);
        }
    }
    return false;
}

/* 0x8007EA08 (0x8): whether the animation is bound to a model. */
bool nw4r::g3d::AnmObj::IsBound() const
{
    return TestAnmFlag(ANMFLAG_ISBOUND);
}

/* 0x8007EA10 (0x7C): the checked cast to AnmObjShp. */
template nw4r::g3d::AnmObjShp* nw4r::g3d::DynamicCast<nw4r::g3d::AnmObjShp, nw4r::g3d::AnmObj>(
    nw4r::g3d::AnmObj* pObj);

/* 0x8007EA8C (0x24C): detaches the shape animation, refilling the replacement vertex tables from the resource (or
 * marking them for refreshing); any other animation goes to ScnMdlSimple. */
bool ScnMdl::RemoveAnmObj(AnmObj* pObj)
{
    if (pObj == NULL) {
        return false;
    }
    if (pObj == mpAnmObjShp) {
        mpAnmObjShp->G3dProc(G3DPROC_DETACH_PARENT, 0, this);
        mpAnmObjShp = NULL;
        if ((mBufferOption & 1) == 0) {
            mReplacement.mFlag |= 1;
            return true;
        }
        if (mReplacement.mpVtxPosTable != NULL) {
            u32 num = GetResMdl().GetResVtxPosNumEntries();
            for (u32 i = 0; i < num; i++) {
                nw4r::g3d::ResVtxPos pos(&GetResMdl().GetResVtxPos(i));
                void* pDst = mReplacement.mpVtxPosTable[i];
                if ((void*)pos.ptr() != pDst) {
                    pos.CopyTo(pDst);
                }
            }
        }
        if (mReplacement.mpVtxNrmTable != NULL) {
            u32 num = GetResMdl().GetResVtxNrmNumEntries();
            for (u32 i = 0; i < num; i++) {
                nw4r::g3d::ResVtxNrm nrm(&GetResMdl().GetResVtxNrm(i));
                void* pDst = mReplacement.mpVtxNrmTable[i];
                if ((void*)nrm.ptr() != pDst) {
                    nrm.CopyTo(pDst);
                }
            }
        }
        if (mReplacement.mpVtxClrTable != NULL) {
            u32 num = GetResMdl().GetResVtxClrNumEntries();
            for (u32 i = 0; i < num; i++) {
                nw4r::g3d::ResVtxClr clr(&GetResMdl().GetResVtxClr(i));
                void* pDst = mReplacement.mpVtxClrTable[i];
                if ((void*)clr.ptr() != pDst) {
                    clr.CopyTo(pDst);
                }
            }
        }
        return true;
    }
    return ScnMdlSimple::RemoveAnmObj(pObj);
}

/* 0x8007ECD8 (0x50): detaches and returns the animation in slot `type` (the shape slot here, the rest ScnMdlSimple's). */
nw4r::g3d::AnmObj* ScnMdl::RemoveAnmObj(AnmObjType type)
{
    if (type == ANMOBJTYPE_SHP) {
        AnmObj* pObj = mpAnmObjShp;

        RemoveAnmObj(pObj);
        return pObj;
    }
    return ScnMdlSimple::RemoveAnmObj(type);
}

/* 0x8007ED28 (0x18): the animation in slot `type`. */
nw4r::g3d::AnmObj* ScnMdl::GetAnmObj(AnmObjType type)
{
    if (type == ANMOBJTYPE_SHP) {
        return mpAnmObjShp;
    }
    return ScnMdlSimple::GetAnmObj(type);
}

/* 0x8007ED40 (0x18): the animation in slot `type`. */
const nw4r::g3d::AnmObj* ScnMdl::GetAnmObj(AnmObjType type) const
{
    if (type == ANMOBJTYPE_SHP) {
        return mpAnmObjShp;
    }
    return ScnMdlSimple::GetAnmObj(type);
}

/* 0x8007ED58 (0x110): constructs the model over its matrix arrays, with no shape animation, its material-buffer flags
 * and a copy of the caller's replacement record. */
ScnMdl::ScnMdl(MEMAllocator* pHeap, ResMdl mdl, nw4r::math::MTX34* pWorldMtxArray, u32* pWorldMtxAttribArray,
               nw4r::math::MTX34* pViewPosMtxArray, nw4r::math::MTX33* pViewNrmMtxArray,
               nw4r::math::MTX34* pViewTexMtxArray, int numView, int numViewMtx, const ReplacementBlock* pReplacement,
               u32* pMatBufferFlags, u32 bufferOption)
    : ScnMdlSimple(pHeap, mdl, pWorldMtxArray, pWorldMtxAttribArray, pViewPosMtxArray, pViewNrmMtxArray,
                   pViewTexMtxArray, numView, numViewMtx),
      mpAnmObjShp(NULL),
      mFlags(0),
      mpDLBuffer(pMatBufferFlags),
      mReplacement(*pReplacement),
      mBufferOption(bufferOption)
{
}

/* 0x8007EE68 (0xB4): asserts the model is detached and detaches its shape animation. */
ScnMdl::~ScnMdl()
{
    if (GetParent() != 0) {
        nw4r::db::Panic(lbl_8058EDA0, 1627, lbl_8058F004);
    }
    if (mpAnmObjShp != 0) {
        RemoveAnmObj(mpAnmObjShp);
    }
}

/* 0x8007EF1C (0x30): returns the type. */
const nw4r::g3d::G3dObj::TypeObj ScnMdl::GetTypeObj() const
{
    const u8* local;

    return *reinterpret_cast<const TypeObj*>(type_obj_set_name(&local, scn_typename_ScnMdl));
}

/* 0x8007EF4C (0x38): returns the type's name. */
const char* ScnMdl::GetTypeName() const
{
    return GetTypeObj().GetTypeName();
}

/* 0x8007EF84 (0x6C): whether the object is a ScnMdl or derives from `type`. */
bool ScnMdl::IsDerivedFrom(TypeObj type) const
{
    if (type == GetTypeObjStatic()) {
        return true;
    }
    return ScnMdlSimple::IsDerivedFrom(type);
}

/* 0x8007EFF0 (0x6C): whether the object is a ScnMdlSimple or derives from `type`. */
bool nw4r::g3d::ScnMdlSimple::IsDerivedFrom(TypeObj type) const
{
    if (type == GetTypeObjStatic()) {
        return true;
    }
    return ScnLeaf::IsDerivedFrom(type);
}

/* 0x8007F05C (0x60): removes and returns the last child; NULL when the group is empty. */
nw4r::g3d::ScnObj* nw4r::g3d::ScnGroup::PopBack()
{
    if (!Empty()) {
        return Remove(Size() - 1);
    }
    return NULL;
}

/* 0x8007F0BC (0x10): whether the group has no child. */
bool nw4r::g3d::ScnGroup::Empty() const
{
    return mNumScnObj == 0;
}

extern "C" {

/* 0x8007F0CC - appends `id` to the scene root's children (ScnGroup::Insert at the child count, through slot
 * +0x34). */
void g3d_root_model_bind(s32 root, u32 id) {
    nw4r::g3d::ScnGroup* pGroup = reinterpret_cast<nw4r::g3d::ScnGroup*>(root);

    pGroup->Insert(pGroup->mNumScnObj, reinterpret_cast<nw4r::g3d::ScnObj*>(id));
}

} /* extern "C" */
