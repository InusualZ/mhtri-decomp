/*
 * g3d/g3d_scnmdl.cpp - nw4r g3d `ScnMdl` scene model: its replaced-material (`mReplacement`) buffers, the node
 *   visibility and option accessors, the constructor and destructor, and the `ScnMdl` name-record cluster.
 * RANGE. .text 0x8007C540-0x8007F0E4 (55 functions); extab, extabindex, .data 0x8058EDA0-0x8058F0A0 (opens on
 *   "g3d_scnmdl.cpp"), .sdata 0x80791208-0x80791210.  The left edge is `g3d/fn_80075DCC.cpp`'s cap, not a proven
 *   seam; tudiscover proves one TU through 0x8007EF1C, and the 0x8007EF1C-0x8007F0E4 tail is here because its head
 *   fn_8007EF1C reads the "ScnMdl" name record (lbl_8056F678), which only the class's own TU registers.
 * NAMES. Map stems (the dump answers `zz_` placeholders); `nw4r::g3d::ScnMdl` comes from the name record and the
 *   `mReplacement.*Array` assert texts; `g3d_root_model_bind` (0x8007F0CC) is a GUESS (it dispatches the material
 *   id through slot +0x34).
 *   res_mdl_info_ref is a GUESS (0x8007D404: the ResMdlInfo handle's block, asserting the handle).  The ScnMdlSimple
 *   getters, G3dProcUpdateFrame and IsDerivedFrom defined here are its weak copies (members of
 *   `g3d/g3d_scnmdlsmpl.h`), as is AnmObj::IsBound.
 * RESIDUALS. Unwritten (objdiff scores them zero): fn_8007C540, fn_8007D59C, fn_8007DDFC, fn_8007E01C,
 *   fn_8007E498, fn_8007E8B4, fn_8007EA8C.  Partial: res_mdl_info_ref, fn_8007D568, fn_8007DBDC, fn_8007DCB8,
 *   fn_8007E7FC, fn_8007EA10, 0x8007ECD8-0x8007EF1C (five functions, the constructor and destructor among them),
 *   fn_8007EF4C, ScnGroup::PopBack.
 *   ScnGroup::PopBack and ScnGroup::Empty (0x8007F05C, 0x8007F0BC) are nw4r's `g3d/g3d_scnobj.h` members.
 *   flipcheck: `.text` 0xADC of 0x2BA4; `.data` and `.sdata` are claimed and not emitted.
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

/* `ScnMdl` lives in its real namespace (rule 9's owner spelling); this unit's bodies name it short. */
using nw4r::g3d::ReplacementBlock;
using nw4r::g3d::ScnMdl;

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
extern const char lbl_8058F028[]; /* the ScnMdl vtable (0x48 B) */
extern const char lbl_8058F070[]; /* "%s::%s: Object not valid." */
extern const char lbl_8058F090[]; /* "g3d_resmdl_ac.h" */
extern u8 lbl_8056F678[];         /* the `.rodata` "ScnMdl" name record */
extern u32 lbl_80791208;          /* the `.sdata` word "ref" */

/* ------------------------------------------------------------------------------------------------ */
/* the object                                                                                        */
/* ------------------------------------------------------------------------------------------------ */

/* The resource block `fn_800730D8`/fn_800732F0/fn_800696E4 hand back: its leading word only.  The
 * layout belongs to g3d/g3d_calcvtx.cpp (rule 1), which defines the full block. */
struct ResVtxBlockHead {
    /* +0x00 */ u32 mSize;
}; /* size: 0x04 */

/* ------------------------------------------------------------------------------------------------ */
/* this unit's own bodies, in address order (definitions follow)                                     */
/* ------------------------------------------------------------------------------------------------ */

extern "C" {

/* -------- the two node-table helpers of `g3d/g3d_resmat.cpp`, declared here -------- */
u32 fn_8009A2F4(void* pSelf, u32 flag);  /* 0x8009A2F4 - the pix-DL replacement's teardown */
u32 fn_8009435C(void* pSelf, u32 flag);  /* 0x8009435C - the tex-color-DL replacement's teardown */
u32 fn_8009411C(void* pSelf, u32 flag);  /* 0x8009411C - the ind-mtx/scale replacement's teardown */

u32 fn_8007D38C(u32 p);
u32 fn_8007D468(const ResHandle* pSelf);
u32 fn_8007D470(u32 p);
void fn_8007D47C(ScnMdl* pSelf, u32* pArg2, u32* pArg3);
u32 fn_8007D570(ScnMdl* pSelf);
u32 fn_8007D588(ScnMdl* pSelf);
u32* fn_8007DB3C(u32* pDst, const u32* pSrc);
void fn_8007DB6C(u32* pDst, const u32* pSrc);
u32* fn_8007DB78(u32* pDst, const u32* pSrc);
void fn_8007DBA8(u32* pDst, const u32* pSrc);
u32 fn_8007DBBC(ScnMdl* pSelf, u32 idx, u32 mask);
void fn_8007DBDC(ScnMdl* pSelf, u32* pArg2, u32* pArg3);
void fn_8007DCB8(ScnMdl* pSelf, u32* pArg2, u32* pArg3);
void fn_8007DD94(ScnMdl* pSelf);
u32 fn_8007DDF4(ScnMdl* pSelf);
u32 fn_8007DF9C(ScnMdl* pSelf, u32 type, u32 on);
u32 fn_8007DFDC(ScnMdl* pSelf, u32 type, u32* pOut);
u32 fn_8007E478(ScnMdl* pSelf);
u32 fn_8007E480(ScnMdl* pSelf);
u32 fn_8007E488(ScnMdl* pSelf);
void fn_8007E490(void);
void fn_8007E494(void);
void fn_8007E7FC(ScnMdl* pSelf);
void* fn_8007EA10(ScnMdl* pSelf);
u32 fn_8007ECD8(ScnMdl* pSelf, u32 type);
u32 fn_8007ED28(ScnMdl* pSelf, u32 type);
u32 fn_8007ED40(ScnMdl* pSelf, u32 type);
u32 fn_8007ED58(ScnMdl* pSelf, void* pArg2, u32* pArg3, u32 a4, u32 a5, u32 a6, u32 a7, u32 a8,
                u32 a9, u32 a10, const ReplacementBlock* pReplacement, u32* pDLBuffer, u32 a13);
void* fn_8007EE68(ScnMdl* pSelf, s16 flag);
u32 fn_8007EF1C(void);
u32 fn_8007EF4C(G3dObj* pSelf);
u32 fn_8007EF84(void* pSelf, u32* pKey);
u32 g3d_root_model_bind(ScnMdl* pSelf, u32 id);

/* ------------------------------------------------------------------------------------------------ */
/* bodies                                                                                            */
/* ------------------------------------------------------------------------------------------------ */

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
u32 res_mdl_info_ref(ResHandle* pSelf) {
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

/* 0x8007D47C - the model's per-node visibility pass: refresh the copied material, then either write the
 * per-node byte vector (the node buffer exists) or hand the node table to the animation object. */
void fn_8007D47C(ScnMdl* pSelf, u32* pArg2, u32* pArg3) {
    reinterpret_cast<nw4r::g3d::ScnMdlSimple*>(pSelf)->CalcPosture((u32)pArg2, (const nw4r::math::MTX34*)pArg3);
    if (fn_8007D588(pSelf) != 0 && fn_8007D570(pSelf) != 0) {
        fn_8007E7FC(pSelf);
    }
    if (reinterpret_cast<nw4r::g3d::ScnMdlSimple*>(pSelf)->GetAnmObjVis() != 0) {
        if (pSelf->mReplacement.mpNodeVisible != 0) {
            nw4r::g3d::ResMdl local = reinterpret_cast<nw4r::g3d::ScnMdlSimple*>(pSelf)->GetResMdl();

            fn_8006ED84(pSelf->mReplacement.mpNodeVisible, &local, (u32*)reinterpret_cast<nw4r::g3d::ScnMdlSimple*>(pSelf)->GetAnmObjVis());
            fn_8007C464(pSelf);
        } else {
            nw4r::g3d::ResMdl local = reinterpret_cast<nw4r::g3d::ScnMdlSimple*>(pSelf)->GetResMdl();

            g3d_apply_vis_anm_result(&local, (void*)reinterpret_cast<nw4r::g3d::ScnMdlSimple*>(pSelf)->GetAnmObjVis());
        }
    }
    reinterpret_cast<nw4r::g3d::ScnObj*>(pSelf)->CheckCallback_CALC_WORLD(nw4r::g3d::ScnObj::CALLBACK_TIMING_C,
                                                                    (u32)pArg2, pArg3);
}

}   /* extern "C" */

/* 0x8007D568 (0x8): the visibility animation. */
nw4r::g3d::AnmObjVis* nw4r::g3d::ScnMdlSimple::GetAnmObjVis()
{
    return mpAnmObjVis;
}

extern "C" {

/* 0x8007D570 - `(mFlags & 1) != 0`, the visible bit. */
u32 fn_8007D570(ScnMdl* pSelf) {
    u32 flag = pSelf->mFlags & 1;

    return ((u32)(-(s32)flag) | flag) >> 31;
}

/* 0x8007D588 - `(mFlags & 2) == 0`, the shape-animation option's inverse. */
u32 fn_8007D588(ScnMdl* pSelf) {
    u32 flag = pSelf->mFlags & 2;

    return !flag;
}

}   /* extern "C" */

/* 0x8007DB2C (0x8): the material-colour animation. */
nw4r::g3d::AnmObjMatClr* nw4r::g3d::ScnMdlSimple::GetAnmObjMatClr()
{
    return mpAnmObjMatClr;
}

extern "C" {

}   /* extern "C" */

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

}   /* extern "C" */

/* 0x8007DBB4 (0x8): the texture-pattern animation. */
nw4r::g3d::AnmObjTexPat* nw4r::g3d::ScnMdlSimple::GetAnmObjTexPat()
{
    return mpAnmObjTexPat;
}

extern "C" {

/* 0x8007DBBC - `(pSelf->mpDLBuffer[idx] & mask) != 0`, the per-entry option query. */
u32 fn_8007DBBC(ScnMdl* pSelf, u32 idx, u32 mask) {
    u32 value = pSelf->mpDLBuffer[idx] & mask;

    return ((u32)(-(s32)value) | value) >> 31;
}

/* 0x8007DBDC - the `pixDL` replacement pass: mark the array active, gather the model's counts and run
 * the draw-buffer builder, then mark it done. */
void fn_8007DBDC(ScnMdl* pSelf, u32* pArg2, u32* pArg3) {
    u32 key;

    reinterpret_cast<nw4r::g3d::ScnMdlSimple*>(pSelf)->CheckCallback_DRAW_OPA(nw4r::g3d::ScnObj::CALLBACK_TIMING_A, (u32)pArg2, pArg3);
    if (pArg3 != 0) {
        key = *pArg3;
    } else {
        key = reinterpret_cast<nw4r::g3d::ScnMdlSimple*>(pSelf)->GetDrawMode();
    }
    nw4r::g3d::ResMdl handle = reinterpret_cast<nw4r::g3d::ScnMdlSimple*>(pSelf)->GetResMdl();
    g3d_draw_res_mdl_directly(&handle, reinterpret_cast<nw4r::g3d::ScnMdlSimple*>(pSelf)->GetViewPosMtxArray(), reinterpret_cast<nw4r::g3d::ScnMdlSimple*>(pSelf)->GetViewNrmMtxArray(), reinterpret_cast<nw4r::g3d::ScnMdlSimple*>(pSelf)->GetViewTexMtxArray(),
                              reinterpret_cast<nw4r::g3d::ScnMdlSimple*>(pSelf)->GetByteCodeDrawOpa(), NULL, &pSelf->mReplacement, key);
    reinterpret_cast<nw4r::g3d::ScnMdlSimple*>(pSelf)->CheckCallback_DRAW_OPA(nw4r::g3d::ScnObj::CALLBACK_TIMING_C, (u32)pArg2, pArg3);
}

}   /* extern "C" */

/* 0x8007DCB0 (0x8): the opaque draw byte code. */
const u8* nw4r::g3d::ScnMdlSimple::GetByteCodeDrawOpa()
{
    return mpByteCodeDrawOpa;
}

extern "C" {

/* 0x8007DCB8 - the `texCoordGen` replacement pass (the same shape as fn_8007DBDC, with the tex-coord
 * count in the parameter slot the other one gives to the pix-DL count). */
void fn_8007DCB8(ScnMdl* pSelf, u32* pArg2, u32* pArg3) {
    u32 key;

    reinterpret_cast<nw4r::g3d::ScnMdlSimple*>(pSelf)->CheckCallback_DRAW_XLU(nw4r::g3d::ScnObj::CALLBACK_TIMING_A, (u32)pArg2, pArg3);
    if (pArg3 != 0) {
        key = *pArg3;
    } else {
        key = reinterpret_cast<nw4r::g3d::ScnMdlSimple*>(pSelf)->GetDrawMode();
    }
    nw4r::g3d::ResMdl handle = reinterpret_cast<nw4r::g3d::ScnMdlSimple*>(pSelf)->GetResMdl();
    g3d_draw_res_mdl_directly(&handle, reinterpret_cast<nw4r::g3d::ScnMdlSimple*>(pSelf)->GetViewPosMtxArray(), reinterpret_cast<nw4r::g3d::ScnMdlSimple*>(pSelf)->GetViewNrmMtxArray(), reinterpret_cast<nw4r::g3d::ScnMdlSimple*>(pSelf)->GetViewTexMtxArray(),
                              NULL, reinterpret_cast<nw4r::g3d::ScnMdlSimple*>(pSelf)->GetByteCodeDrawXlu(), &pSelf->mReplacement, key);
    reinterpret_cast<nw4r::g3d::ScnMdlSimple*>(pSelf)->CheckCallback_DRAW_XLU(nw4r::g3d::ScnObj::CALLBACK_TIMING_C, (u32)pArg2, pArg3);
}

}   /* extern "C" */

/* 0x8007DD8C (0x8): the translucent draw byte code. */
const u8* nw4r::g3d::ScnMdlSimple::GetByteCodeDrawXlu()
{
    return mpByteCodeDrawXlu;
}

extern "C" {

/* 0x8007DD94 - the shape-blend driver hand-over: the model handle, the shape-animation object and the
 * replacement record's three tables. */
void fn_8007DD94(ScnMdl* pSelf) {
    if (fn_8007DDF4(pSelf) != 0) {
        nw4r::g3d::ResMdl local = reinterpret_cast<nw4r::g3d::ScnMdlSimple*>(pSelf)->GetResMdl();

        fn_8007270C(&local, (void*)fn_8007DDF4(pSelf), (const void**)pSelf->mReplacement.mpVtxPosTable,
                    (const void**)pSelf->mReplacement.mpClrTable,
                    (const void**)pSelf->mReplacement.mpTexTable);
    }
}

/* 0x8007DDF4 - the third replacement count. */
u32 fn_8007DDF4(ScnMdl* pSelf) {
    return pSelf->mpAnmObjShp;
}

}   /* extern "C" */

/* 0x8007DF98 (0x4): the frame-update pass: advances the attached animations. */
/* untyped: caller-owned payload - the pass's info block */
void nw4r::g3d::ScnMdlSimple::G3dProcUpdateFrame(u32 param, void* pInfo)
{
    UpdateFrame();
}

extern "C" {

/* 0x8007DF9C - the option setter for the one option this unit owns (bit 1 of mFlags, inverted); every
 * other option tail-calls the shared setter. */
u32 fn_8007DF9C(ScnMdl* pSelf, u32 type, u32 on) {
    if (type == 0x30001) {
        if (on != 0) {
            pSelf->mFlags &= ~2u;
        } else {
            pSelf->mFlags |= 2u;
        }
    } else {
        return reinterpret_cast<nw4r::g3d::ScnMdlSimple*>(pSelf)->nw4r::g3d::ScnMdlSimple::SetScnObjOption(type, on);
    }
    return 1;
}

/* 0x8007DFDC - the matching option query. */
u32 fn_8007DFDC(ScnMdl* pSelf, u32 type, u32* pOut) {
    u32 flag;

    if (pOut == 0) {
        return 0;
    }
    if (type == 0x30001) {
        flag = pSelf->mFlags & 2;
        *pOut = !flag;
    } else {
        return reinterpret_cast<nw4r::g3d::ScnMdlSimple*>(pSelf)->nw4r::g3d::ScnMdlSimple::GetScnObjOption(type, pOut);
    }
    return 1;
}

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

/* 0x8007E7FC - write one byte per node into the visible array (1 where the node is visible) and clear
 * the visible bit, which forces the next query to re-read the node table. */
void fn_8007E7FC(ScnMdl* pSelf) {
    nw4r::g3d::ResMdl handle = reinterpret_cast<nw4r::g3d::ScnMdlSimple*>(pSelf)->GetResMdl();
    u32 view;
    s32 numNodes;
    u32 i;

    fn_80077E34((s32)&view, &handle);
    numNodes = reinterpret_cast<nw4r::g3d::ResMdl*>(&view)->GetResNodeNumEntries();
    if (pSelf->mReplacement.mpNodeVisible != 0) {
        for (i = 0; i < (u32)numNodes; i++) {
            nw4r::g3d::ResNode node = reinterpret_cast<nw4r::g3d::ResMdl*>(&view)->GetResNode(i);

            if (fn_80078904((s32)&node) != 0) {
                pSelf->mReplacement.mpNodeVisible[i] = 1;
            } else {
                pSelf->mReplacement.mpNodeVisible[i] = 0;
            }
        }
    }
    pSelf->mFlags &= ~1u;
}

}   /* extern "C" */

/* 0x8007EA08 (0x8): whether the animation is bound to a model. */
bool nw4r::g3d::AnmObj::IsBound() const
{
    return TestAnmFlag(ANMFLAG_ISBOUND);
}

extern "C" {

/* 0x8007EA10 - `DynamicCast`-shaped: resolve the caller's name record and hand the object back only
 * when the object's own type query accepts it. */
void* fn_8007EA10(ScnMdl* pSelf) {
    u32 ok = 0;

    if (pSelf != 0) {
        u32 local = fn_800697A4();

        if (pSelf->mpfn_0x08(&local) != 0) {
            ok = 1;
        }
    }
    return ok != 0 ? pSelf : 0;
}

/* 0x8007ECD8 - the option accessor that reads the shape-animation object instead of a flag. */
u32 fn_8007ECD8(ScnMdl* pSelf, u32 type) {
    if (type == 5) {
        u32 value = pSelf->mpAnmObjShp;

        pSelf->mpfn_0x38(value);
        return value;
    }
    return (u32)reinterpret_cast<nw4r::g3d::ScnMdlSimple*>(pSelf)->nw4r::g3d::ScnMdlSimple::RemoveAnmObj((nw4r::g3d::ScnMdlSimple::AnmObjType)type);
}

/* 0x8007ED28 - the `type == 5` fast path of the second option accessor. */
u32 fn_8007ED28(ScnMdl* pSelf, u32 type) {
    if (type == 5) {
        return pSelf->mpAnmObjShp;
    }
    return (u32)reinterpret_cast<nw4r::g3d::ScnMdlSimple*>(pSelf)->nw4r::g3d::ScnMdlSimple::GetAnmObj((nw4r::g3d::ScnMdlSimple::AnmObjType)type);
}

/* 0x8007ED40 - the same fast path for the setter side. */
u32 fn_8007ED40(ScnMdl* pSelf, u32 type) {
    if (type == 5) {
        return pSelf->mpAnmObjShp;
    }
    return (u32)reinterpret_cast<const nw4r::g3d::ScnMdlSimple*>(pSelf)->nw4r::g3d::ScnMdlSimple::GetAnmObj(
        (nw4r::g3d::ScnMdlSimple::AnmObjType)type);
}

/* 0x8007ED58 - the ScnMdl constructor: the base constructor with the caller's two-word record, the vtable, the two
 * cleared members, the DL buffer pointer, the copied 0x40-byte replacement record and the trailing argument. */
u32 fn_8007ED58(ScnMdl* pSelf, void* pArg2, u32* pArg3, u32 a4, u32 a5, u32 a6, u32 a7, u32 a8,
                u32 a9, u32 a10, const ReplacementBlock* pReplacement, u32* pDLBuffer, u32 a13) {
    u32 args[3];

    args[2] = *pArg3;
    args[0] = a9;
    args[1] = a10;
    fn_80080C60(pSelf, pArg2, &args[2]);
    *(const char**)pSelf = lbl_8058F028;
    pSelf->mpAnmObjShp = 0;
    pSelf->mFlags = 0;
    pSelf->mpDLBuffer = pDLBuffer;
    pSelf->mReplacement = *pReplacement;
    pSelf->field_0x184 = a13;
    return (u32)pSelf;
}

/* 0x8007EE68 - the ScnMdl deleting destructor: install the vtable, assert the parent is gone, release
 * the shape animation object, run the base teardown and, for a positive flag, free the object. */
void* fn_8007EE68(ScnMdl* pSelf, s16 flag) {
    if (pSelf != 0) {
        *(const char**)pSelf = lbl_8058F028;
        if (reinterpret_cast<nw4r::g3d::G3dObj*>(pSelf)->GetParent() != 0) {
            nw4r::db::Panic(lbl_8058EDA0, 1627, lbl_8058F004);
        }
        if (pSelf->mpAnmObjShp != 0) {
            pSelf->mpfn_0x38(pSelf->mpAnmObjShp);
        }
        dtor_80080F7C(pSelf, 0);
        if ((s16)flag > 0) {
            nw4r::g3d::G3dObj::operator delete(pSelf);
        }
    }
    return pSelf;
}

/* 0x8007EF1C - the ScnMdl name record (`g3d_scnmdl.cpp`'s own type registration). */
u32 fn_8007EF1C(void) {
    const u8* local;

    return (u32)*type_obj_set_name(&local, lbl_8056F678);
}

/* 0x8007EF4C - the vtable-dispatch wrapper: run the object's slot +0x14 and read the word back
 * through the `TypeObj::GetTypeName` helper. */
u32 fn_8007EF4C(G3dObj* pSelf) {
    u32 tmp = pSelf->vt->method_0x14(pSelf);

    return (u32)reinterpret_cast<const nw4r::g3d::G3dObj::TypeObj*>(&tmp)->GetTypeName();
}

/* 0x8007EF84 - one step of the name-record chain: resolve the ScnMdl record, compare the caller's key
 * against it and, on a miss, run the next step's insertion with a copy of the key word. */
u32 fn_8007EF84(void* pSelf, u32* pKey) {
    u32 res = fn_8007B764(pSelf);

    if ((*reinterpret_cast<const nw4r::g3d::G3dObj::TypeObj*>(pKey) == *reinterpret_cast<const nw4r::g3d::G3dObj::TypeObj*>(&res))) {
        return 1;
    }
    {
        u32 local = *pKey;

        return reinterpret_cast<nw4r::g3d::ScnMdlSimple*>(pSelf)->nw4r::g3d::ScnMdlSimple::IsDerivedFrom(
            *reinterpret_cast<const nw4r::g3d::G3dObj::TypeObj*>(&local));
    }
}

}   /* extern "C" */

/* 0x8007EFF0 (0x6C): whether the object is a ScnMdlSimple or derives from `type`. */
bool nw4r::g3d::ScnMdlSimple::IsDerivedFrom(TypeObj type) const
{
    if (type == GetTypeObjStatic()) {
        return true;
    }
    return ScnLeaf::IsDerivedFrom(type);
}

extern "C" {

/* 0x8007F05C - drop the last material of the copied-material list (a no-op when the resource handle is
 * empty). */
}   /* extern "C": the scene object members below have C++ linkage */

/* 0x8007F05C (0x60): removes and returns the last child; NULL when the group is empty. */
nw4r::g3d::ScnObj* nw4r::g3d::ScnGroup::PopBack()
{
    if (!Empty()) {
        return Remove(Size() - 1);
    }
    return NULL;
}

extern "C" {


/* 0x8007F0BC - `mResMdl == 0`: the resource handle is empty. */
}   /* extern "C": the scene object members below have C++ linkage */

/* 0x8007F0BC (0x10): whether the group has no child. */
bool nw4r::g3d::ScnGroup::Empty() const
{
    return mNumScnObj == 0;
}

extern "C" {


/* 0x8007F0CC - dispatch the material id through slot +0x34 (the `CopiedMatAccess` constructor's
 * handle hand-over). */
u32 g3d_root_model_bind(ScnMdl* pSelf, u32 id) {
    return pSelf->mpfn_0x34(pSelf->mResMdl, id);
}

} /* extern "C" */
