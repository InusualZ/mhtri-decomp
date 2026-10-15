/*
 * g3d/g3d_calcmaterial.cpp - nw4r g3d material-controller helpers: pointer wrappers over the global material table
 *   fn_8005A9BC returns (their inlined `g3d_resmat_ac.h` constructors assert 32-byte alignment in fn_8006F158/
 *   fn_8006F298 and 4-byte in fn_8006F374/fn_8006F41C/fn_8006F4BC), and g3d_calc_material_directly, which walks the material
 *   list and drives the per-material matrix updates.
 * RANGE. .text 0x8006EE78-0x8006F5A0 (31 functions); extab, extabindex, .data 0x8058D7E0-0x8058D938 (opens on
 *   "g3d_calcmaterial.cpp", g3d_calc_material_directly's assert).  The right seam: the two paired-single routines from
 *   0x8006F5A0 are g3d_calcview.cpp's (their only callers and their constant are calcview's).
 * NAMES. res_mat_chan_end_edit is a GUESS; res_mat_ind_mtx_copy_ctor is a GUESS;
 *   res_mat_ind_mtx_end_edit is a GUESS; res_mat_tev_color_copy_ctor is a GUESS;
 *   res_mat_tev_color_end_edit is a GUESS; res_tex_obj_end_edit is a GUESS; res_tex_srt_copy_ctor is a GUESS;
 *   res_tex_srt_end_edit is a GUESS; res_tlut_obj_end_edit is a GUESS;
 *   tex_pat_anm_result_ctor is a GUESS (the evidence follows).
 *   Map stems, except the material blocks' edit hooks: res_mat_chan_end_edit, res_tex_srt_end_edit,
 *   res_tlut_obj_end_edit, res_tex_obj_end_edit, res_mat_tev_color_end_edit and res_mat_ind_mtx_end_edit are
 *   GUESSES (C spellings of nw4r's `EndEdit`, by the ScnMdl buffer refill that calls each after the block's CopyTo).
 *   res_mat_tev_color_copy_ctor, res_mat_ind_mtx_copy_ctor, res_tex_srt_copy_ctor and tex_pat_anm_result_ctor are
 *   GUESSES (the handle copies and the TexPatAnmResult constructor the ScnMdl material pass calls).
 *   g3d_calc_material_directly is a GUESS (0x8006EE78: applies the texture and colour animations to the model's
 *   materials, called by ScnMdlSimple's material pass).
 * RESIDUALS. fn_8006F200: retail branches to a separate `li r3,0; blr` where ours folds the return into `bnelr`
 *   (tried: the inverted test and a result variable - worse or equal).
 *   flipcheck: `.text` 0x4D4 of 0x8C0; `.data`: the strings are literals, the `_ac.h` accessor strings are not the inline tail yet (retail: weak `@STRING@` copies, one per inline
 *   instance, after the TU's own strings, playbook 80).
 */
#include "types.h"
#include "g3d/g3d_resmat.h"
#include "g3d/g3d_resfile.h"       /* res_mat_*_dc_store (rule 2) */
#include "nw4r/g3d/res_common.h"   /* ResHandle (rule 1) */
#include "g3d/g3d_anmclr.h"        /* res_mat_copy_ctor, apply_clr_anm_result (rule 2) */
#include "g3d/g3d_anmtexpat.h"     /* apply_tex_pat_anm_result (rule 2) */
#include "g3d/g3d_anmtexsrt.h"     /* nw4r::g3d::AnmObjTexSrt, apply_tex_srt_anm_result (rule 2) */
#include "g3d/res_mat_chan_copy_ctor.h" /* res_mat_chan_copy_ctor (rule 2) */

#pragma pool_data off

/* The five assert wrappers below need the *un-fused* compare (retail keeps `clrlwi` + `cmpwi` and a
 * separate `add`/`blr`, where the peephole pass folds them into `clrlwi.`/`bnelr`), exactly as
 * g3d_basic.cpp found for its asserts. */
#pragma peephole off

namespace nw4r {
namespace db {
/* `Panic(const char* pFile, int line, const char* pFmt, ...)`; the map name is the C++ mangling
 * `Panic__Q24nw4r2dbFPCciPCce`, so it is called through its owner, never by the mangled spelling. */
void Panic(const char* pFile, int line, const char* pFmt, ...);
}  // namespace db
}  // namespace nw4r

extern "C" {
/* The helpers this unit calls (C linkage, plain map stems). */
/* The two handle-array element constructors tex_pat_anm_result_ctor runs. */
void res_tex_ctor(void* p, u32 flag);
void res_pltt_ctor(void* p, u32 flag);
/* The global material-table accessor the accessor chain reads at +0x3C. */

/* This unit's own bodies, in address order. */
void fn_8006F118(void* pDst, const void* pSrc);
void* fn_8006F200(void* pHandle, u32 offset);
void fn_8006F258(void* pDst, const void* pSrc);
void* res_tex_srt_copy_ctor(void* pDst, const void* pSrc);
void fn_8006F334(void* pDst, const void* pSrc);
void fn_8006F3D8(u32* pDst, u32 value);
u32* tex_pat_anm_result_ctor(u32* self);

}

/* The 32-byte-aligned resource pointer wrapper fn_8006F158 builds and the 4-byte-aligned ones
 * (fn_8006F374 / fn_8006F41C / fn_8006F4BC).  The asserts are the g3d_resmat_ac.h inlines. */

extern "C" {

/* Copies a 4-byte resource handle. */
void fn_8006F118(void* pDst, const void* pSrc) {
    *(u32*)pDst = *(u32*)pSrc;
}

/* The handle stores and copies. */
} /* extern "C" */

/* 0x8006F1BC (0x8): stores the block address. */
NW4R_G3D_RESCOMMON_CTOR(nw4r::g3d::ResMatTevColorData)

extern "C" {
} /* extern "C" */

/* 0x8006F2FC (0x8): stores the block address. */
NW4R_G3D_RESCOMMON_CTOR(nw4r::g3d::ResMatIndMtxAndScaleData)

extern "C" {
void fn_8006F334(void* pDst, const void* pSrc) {
    *(u32*)pDst = *(u32*)pSrc;
}
void fn_8006F3D8(u32* pDst, u32 value) {
    *pDst = value;
}
} /* extern "C" */

/* 0x8006F480 (0x8): stores the block address. */
NW4R_G3D_RESCOMMON_CTOR(nw4r::g3d::ResTlutObjData)

extern "C" {
} /* extern "C" */

/* 0x8006F520 (0x8): stores the block address. */
NW4R_G3D_RESCOMMON_CTOR(nw4r::g3d::ResTexObjData)

extern "C" {
void fn_8006F258(void* pDst, const void* pSrc) {
    *(u32*)pDst = *(u32*)pSrc;
}

/* The empty hooks. */
void res_mat_chan_end_edit(struct ResHandle* pSelf) {
    (void)pSelf;
}
void res_tex_srt_end_edit(struct ResHandle* pSelf) {
    (void)pSelf;
}
void res_tlut_obj_end_edit(struct ResHandle* pSelf) {
    (void)pSelf;
}
void res_tex_obj_end_edit(struct ResHandle* pSelf) {
    (void)pSelf;
}

/* Base handle + offset, with offset 0 meaning the null resource. */
void* fn_8006F200(void* pHandle, u32 offset) {
    u32 base = *(u32*)pHandle;
    if (offset == 0) {
        return 0;
    }
    return (void*)(base + offset);
}

/* Tail calls into the pool allocator/registry with a null flag. */
void res_mat_tev_color_end_edit(struct ResHandle* pSelf) {
    res_mat_tev_color_dc_store(pSelf, 0);
}
void res_mat_ind_mtx_end_edit(struct ResHandle* pSelf) {
    res_mat_ind_mtx_dc_store(pSelf, 0);
}

/* Copy-construct and return the destination. */
ResHandle* res_mat_tev_color_copy_ctor(ResHandle* pDst, const ResHandle* pSrc) {
    fn_8006F118(pDst, pSrc);
    return pDst;
}
ResHandle* res_mat_ind_mtx_copy_ctor(ResHandle* pDst, const ResHandle* pSrc) {
    fn_8006F258(pDst, pSrc);
    return pDst;
}
void* res_tex_srt_copy_ctor(void* pDst, const void* pSrc) {
    fn_8006F334(pDst, pSrc);
    return pDst;
}

/* The alignment-asserting resource pointer constructors (g3d_resmat_ac.h inlines) pass the header file
 * strings and the "!((u32)p & mask)" messages. */
} /* extern "C" */

/* 0x8006F158 (0x64): wraps `pData`, asserting its alignment. */
/* untyped: opaque handle - the block address */
nw4r::g3d::ResMatTevColor::ResMatTevColor(void* pData) : ResCommon<ResMatTevColorData>(pData) {
    if ((u32)pData & 0x1F) {
        nw4r::db::Panic("g3d_resmat_ac.h", 378, "NW4R:Failed assertion !((u32)p & 0x1f)");
    }
}

extern "C" {
} /* extern "C" */

/* 0x8006F298 (0x64): wraps `pData`, asserting its alignment. */
/* untyped: opaque handle - the block address */
nw4r::g3d::ResMatIndMtxAndScale::ResMatIndMtxAndScale(void* pData) : ResCommon<ResMatIndMtxAndScaleData>(pData) {
    if ((u32)pData & 0x1F) {
        nw4r::db::Panic("g3d_resmat_ac.h", 413, "NW4R:Failed assertion !((u32)p & 0x1f)");
    }
}

extern "C" {
} /* extern "C" */

/* 0x8006F374 (0x64): wraps `pData`, asserting its alignment. */
/* untyped: opaque handle - the block address */
nw4r::g3d::ResTexSrt::ResTexSrt(void* pData) {
    fn_8006F3D8((u32*)this, (u32)pData);
    if ((u32)pData & 0x3) {
        nw4r::db::Panic("g3d_resmat_ac.h", 107, "NW4R:Failed assertion !((u32)p & 0x3)");
    }
}

extern "C" {
} /* extern "C" */

/* 0x8006F41C (0x64): wraps `pData`, asserting its alignment. */
/* untyped: opaque handle - the block address */
nw4r::g3d::ResTlutObj::ResTlutObj(void* pData) : ResCommon<ResTlutObjData>(pData) {
    if ((u32)pData & 0x3) {
        nw4r::db::Panic("g3d_resmat_ac.h", 74, "NW4R:Failed assertion !((u32)p & 0x3)");
    }
}

extern "C" {
} /* extern "C" */

/* 0x8006F4BC (0x64): wraps `pData`, asserting its alignment. */
/* untyped: opaque handle - the block address */
nw4r::g3d::ResTexObj::ResTexObj(void* pData) : ResCommon<ResTexObjData>(pData) {
    if ((u32)pData & 0x3) {
        nw4r::db::Panic("g3d_resmat_ac.h", 40, "NW4R:Failed assertion !((u32)p & 0x3)");
    }
}

extern "C" {

/* The global material table `fn_8005A9BC` returns; only its per-frame offset at +0x3C is read here,
 * so the size is the minimum that covers it. */
typedef struct {
    /* +0x00 */ u8 pad_0x00[0x3C];
    /* +0x3C */ u32 offset_0x3C;
} G3dMaterialTable; /* size: 0x40 */

/* Returns the material table base plus the per-frame offset at +0x3C of the global fn_8005A9BC returns. */
} /* extern "C" */

/* 0x8006F1C4 (0x3C): returns the material's display-list block. */
/* untyped: byte range - the display-list block */
void* nw4r::g3d::ResMat::GetResMatDLData() {
    G3dMaterialTable* pGlobal = (G3dMaterialTable*)&ref();
    return fn_8006F200(this, pGlobal->offset_0x3C);
}

extern "C" {

/* The typed sub-resource accessors: build the assert-checked pointer and read its handle. */
} /* extern "C" */

/* 0x8006F124 (0x34): returns the material's MatTevColor block. */
nw4r::g3d::ResMatTevColor nw4r::g3d::ResMat::GetResMatTevColor() {
    return ResMatTevColor((u8*)GetResMatDLData() + 32);
}

extern "C" {
} /* extern "C" */

/* 0x8006F264 (0x34): returns the material's MatIndMtxAndScale block. */
nw4r::g3d::ResMatIndMtxAndScale nw4r::g3d::ResMat::GetResMatIndMtxAndScale() {
    return ResMatIndMtxAndScale((u8*)GetResMatDLData() + 160);
}

extern "C" {
} /* extern "C" */

/* 0x8006F340 (0x34): returns the material's TexSrt block. */
nw4r::g3d::ResTexSrt nw4r::g3d::ResMat::GetResTexSrt() {
    return ResTexSrt((u8*)&ref() + 424);
}

extern "C" {

} /* extern "C" */

/* 0x8006F3E8 (0x34): returns the material's palette-object block. */
nw4r::g3d::ResTlutObj nw4r::g3d::ResMat::GetResTlutObj() {
    return ResTlutObj((u8*)&ref() + 324);
}

/* 0x8006F488 (0x34): returns the material's texture-object block. */
nw4r::g3d::ResTexObj nw4r::g3d::ResMat::GetResTexObj() {
    return ResTexObj((u8*)&ref() + 64);
}

extern "C" {

/* Constructs the 8+8 entry handle arrays at +0x4 and +0x24 of the record. */
u32* tex_pat_anm_result_ctor(u32* self) {
    u32* p = self + 1;
    u32* mid = self + 9;
    do {
        res_tex_ctor(p, 0);
        p++;
    } while (p < mid);
    u32* end = self + 17;
    do {
        res_pltt_ctor(mid, 0);
        mid++;
    } while (mid < end);
    return self;
}

}  // extern "C"

/* 0x8006EE78 (0x264): applies the texture-pattern, texture-SRT and colour animations to the model's materials
 * directly (no material buffer): each animated material block is edited in place and its display list stored. */
extern "C" void g3d_calc_material_directly(const nw4r::g3d::ResMdl* pMdl, nw4r::g3d::AnmObjTexPat* pTexPat,
                                           nw4r::g3d::AnmObjTexSrt* pTexSrt, nw4r::g3d::AnmObjMatClr* pMatClr) {
    if (!pMdl->IsValid()) {
        nw4r::db::Panic("g3d_calcmaterial.cpp", 0x2C, "NW4R:Failed assertion mdl.IsValid()");
    }
    u32 numMat = pMdl->GetResMatNumEntries();
    for (u32 i = 0; i < numMat; i++) {
        ResHandle mat;
        res_mat_copy_ctor(&mat, (ResHandle*)&pMdl->GetResMat(i));
        if (pTexPat != NULL && pTexPat->TestExistence(i)) {
            nw4r::g3d::TexPatAnmResult result;
            tex_pat_anm_result_ctor((u32*)&result);
            const nw4r::g3d::TexPatAnmResult* pResult = pTexPat->GetResult(&result, i);
            apply_tex_pat_anm_result(*(ResHandle*)&reinterpret_cast<nw4r::g3d::ResMat&>(mat).GetResTexObj(),
                                     *(ResHandle*)&reinterpret_cast<nw4r::g3d::ResMat&>(mat).GetResTlutObj(),
                                     pResult);
            res_tex_obj_end_edit((ResHandle*)&reinterpret_cast<nw4r::g3d::ResMat&>(mat).GetResTexObj());
            res_tlut_obj_end_edit((ResHandle*)&reinterpret_cast<nw4r::g3d::ResMat&>(mat).GetResTlutObj());
        }
        if (pTexSrt != NULL && pTexSrt->TestExistence(i)) {
            ResHandle texSrt;
            ResHandle indMtx;
            nw4r::g3d::TexSrtAnmResult result;
            res_tex_srt_copy_ctor(&texSrt, (ResHandle*)&reinterpret_cast<nw4r::g3d::ResMat&>(mat).GetResTexSrt());
            res_mat_ind_mtx_copy_ctor(&indMtx,
                                      (ResHandle*)&reinterpret_cast<nw4r::g3d::ResMat&>(mat).GetResMatIndMtxAndScale());
            const nw4r::g3d::TexSrtAnmResult* pResult = pTexSrt->GetResult(&result, i);
            apply_tex_srt_anm_result(texSrt, indMtx, pResult);
            res_mat_ind_mtx_end_edit(&indMtx);
            res_tex_srt_end_edit(&texSrt);
        }
        if (pMatClr != NULL && pMatClr->TestExistence(i)) {
            ResHandle tevColor;
            ResHandle chan;
            nw4r::g3d::ClrAnmResult result;
            res_mat_tev_color_copy_ctor(&tevColor,
                                        (ResHandle*)&reinterpret_cast<nw4r::g3d::ResMat&>(mat).GetResMatTevColor());
            res_mat_chan_copy_ctor(&chan, (ResHandle*)&reinterpret_cast<nw4r::g3d::ResMat&>(mat).GetResMatChan());
            const nw4r::g3d::ClrAnmResult* pResult = pMatClr->GetResult(&result, i);
            apply_clr_anm_result(chan, tevColor, pResult);
            res_mat_chan_end_edit(&chan);
            res_mat_tev_color_end_edit(&tevColor);
        }
    }
}
