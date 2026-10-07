/*
 * g3d/g3d_calcmaterial.cpp - nw4r g3d material-controller helpers: pointer wrappers over the global material table
 *   fn_8005A9BC returns (their inlined `g3d_resmat_ac.h` constructors assert 32-byte alignment in fn_8006F158/
 *   fn_8006F298 and 4-byte in fn_8006F374/fn_8006F41C/fn_8006F4BC), and g3d_calc_material_directly, which walks the material
 *   list and drives the per-material matrix updates.
 * RANGE. .text 0x8006EE78-0x8006F738 (33 functions); extab, extabindex, .data 0x8058D7E0-0x8058D938 (opens on
 *   "g3d_calcmaterial.cpp", g3d_calc_material_directly's assert).  The right seam is proven: fn_8006F738 cites "g3d_calcview.cpp".
 * NAMES. mtx34_axis_in_parent is a GUESS; mtx34_up_axis_in_parent is a GUESS (the evidence follows).
 *   res_mat_chan_end_edit is a GUESS; res_mat_ind_mtx_copy_ctor is a GUESS;
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
 * RESIDUALS. The large vector/matrix bodies g3d_calc_material_directly, mtx34_axis_in_parent (0x8006F5A0, 0xC0 bytes) and mtx34_up_axis_in_parent (0x8006F660, 0xD8 bytes) are unwritten empty stubs; fn_8006F200 and
 *   tex_pat_anm_result_ctor are partial.
 *   flipcheck: `.text` 0x4D4 of 0x8C0; `.data` is claimed and not emitted.
 */
#include "types.h"
#include "g3d/g3d_resmat.h"
#include "g3d/g3d_resfile.h"       /* res_mat_*_dc_store (rule 2) */

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
void* g3d_calc_material_directly(void* pMdl, void* pMatArray, void* pTexArray, void* pClrArray);
void fn_8006F118(void* pDst, const void* pSrc);
void* fn_8006F200(void* pHandle, u32 offset);
void fn_8006F258(void* pDst, const void* pSrc);
void* res_tex_srt_copy_ctor(void* pDst, const void* pSrc);
void fn_8006F334(void* pDst, const void* pSrc);
void fn_8006F3D8(u32* pDst, u32 value);
u32* tex_pat_anm_result_ctor(u32* self);
void mtx34_axis_in_parent(f32* pDst, const f32* pMtx, const f32* pVec);
void mtx34_up_axis_in_parent(f32* pDst, const f32* pMtx, const f32* pVec);
}

/* The pooled file-name/assert strings this unit reads, declared, not defined: the claimed `.data` is not
 * emitted yet. */
extern const char lbl_8058D7E0[]; /* "g3d_calcmaterial.cpp"                            .data */
extern const char lbl_8058D7F8[]; /* "NW4R:Failed assertion mdl.IsValid()"            .data */
extern const char lbl_8058D81C[]; /* "NW4R:Failed assertion !((u32)p & 0x1f)"         .data */
extern const char lbl_8058D848[]; /* "g3d_resmat_ac.h"                                .data */
extern const char lbl_8058D858[]; /* "NW4R:Failed assertion !((u32)p & 0x1f)"         .data */
extern const char lbl_8058D880[]; /* "g3d_resmat_ac.h"                                .data */
extern const char lbl_8058D890[]; /* "NW4R:Failed assertion !((u32)p & 0x3)"          .data */
extern const char lbl_8058D8B8[]; /* "g3d_resmat_ac.h"                                .data */
extern const char lbl_8058D8C8[]; /* "NW4R:Failed assertion !((u32)p & 0x3)"          .data */
extern const char lbl_8058D8F0[]; /* "g3d_resmat_ac.h"                                .data */
extern const char lbl_8058D900[]; /* "NW4R:Failed assertion !((u32)p & 0x3)"          .data */
extern const char lbl_8058D928[]; /* "g3d_resmat_ac.h"                                .data */

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

/* The alignment-asserting resource pointer constructors (g3d_resmat_ac.h inlines).
 * `lbl_8058D848`/`lbl_8058D880`/`...` are the header file strings, `lbl_8058D81C`/`lbl_8058D858`/...
 * the "!((u32)p & mask)" messages. */
} /* extern "C" */

/* 0x8006F158 (0x64): wraps `pData`, asserting its alignment. */
/* untyped: opaque handle - the block address */
nw4r::g3d::ResMatTevColor::ResMatTevColor(void* pData) : ResCommon<ResMatTevColorData>(pData) {
    if ((u32)pData & 0x1F) {
        nw4r::db::Panic((const char*)lbl_8058D880, 378, (const char*)lbl_8058D858);
    }
}

extern "C" {
} /* extern "C" */

/* 0x8006F298 (0x64): wraps `pData`, asserting its alignment. */
/* untyped: opaque handle - the block address */
nw4r::g3d::ResMatIndMtxAndScale::ResMatIndMtxAndScale(void* pData) : ResCommon<ResMatIndMtxAndScaleData>(pData) {
    if ((u32)pData & 0x1F) {
        nw4r::db::Panic((const char*)lbl_8058D848, 413, (const char*)lbl_8058D81C);
    }
}

extern "C" {
} /* extern "C" */

/* 0x8006F374 (0x64): wraps `pData`, asserting its alignment. */
/* untyped: opaque handle - the block address */
nw4r::g3d::ResTexSrt::ResTexSrt(void* pData) {
    fn_8006F3D8((u32*)this, (u32)pData);
    if ((u32)pData & 0x3) {
        nw4r::db::Panic(lbl_8058D8B8, 107, lbl_8058D890);
    }
}

extern "C" {
} /* extern "C" */

/* 0x8006F41C (0x64): wraps `pData`, asserting its alignment. */
/* untyped: opaque handle - the block address */
nw4r::g3d::ResTlutObj::ResTlutObj(void* pData) : ResCommon<ResTlutObjData>(pData) {
    if ((u32)pData & 0x3) {
        nw4r::db::Panic((const char*)lbl_8058D8F0, 74, (const char*)lbl_8058D8C8);
    }
}

extern "C" {
} /* extern "C" */

/* 0x8006F4BC (0x64): wraps `pData`, asserting its alignment. */
/* untyped: opaque handle - the block address */
nw4r::g3d::ResTexObj::ResTexObj(void* pData) : ResCommon<ResTexObjData>(pData) {
    if ((u32)pData & 0x3) {
        nw4r::db::Panic((const char*)lbl_8058D928, 40, (const char*)lbl_8058D900);
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
    for (; p < mid; p++) {
        res_tex_ctor(p, 0);
    }
    for (; mid < self + 17; mid++) {
        res_pltt_ctor(mid, 0);
    }
    return self;
}

/* Walks the model's material list and drives the per-material matrix update (a stub). */
void* g3d_calc_material_directly(void* pMdl, void* pMatArray, void* pTexArray, void* pClrArray) {
    (void)pMdl;
    (void)pMatArray;
    (void)pTexArray;
    (void)pClrArray;
    return pMdl;
}

/* The paired-single matrix/vector builders, a compiler-cloned pair (stubs). */
void mtx34_axis_in_parent(f32* pDst, const f32* pMtx, const f32* pVec) {
    (void)pDst;
    (void)pMtx;
    (void)pVec;
}
void mtx34_up_axis_in_parent(f32* pDst, const f32* pMtx, const f32* pVec) {
    (void)pDst;
    (void)pMtx;
    (void)pVec;
}

}  // extern "C"
