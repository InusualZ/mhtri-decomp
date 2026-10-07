/*
 * g3d/g3d_calcview.cpp - nw4r g3d view/billboard-matrix calculator: the per-billboard matrix builders
 *   (fn_8006F738, fn_8006F934, fn_8006FBBC) and the material/model accessor chain they drive (fn_8006FDCC ..
 *   fn_80070134).
 * RANGE. .text 0x8006F738-0x8007270C (44 functions); extab, extabindex, .rodata 0x8056F638-0x8056F658, .data
 *   0x8058D938-0x8058DD68 (opens on "g3d_calcview.cpp", then the billboard warning strings), .sdata
 *   0x807911A8-0x807911B8, .sdata2 0x80795DA8-0x80795DB0.  Both neighbours cite their own `__FILE__` strings.
 * NAMES. Map stems.
 *   GUESS (from the body and its callers): `mtx34_concat`, `mtx34_copy_ps` (0x8007100C: a paired-single 3x4 matrix
 *   copy, 32 call sites in ef/, enemy/ and g3d/).
 *   res_mdl_info_num_pos_nrm_mtx is a GUESS (0x8006FFDC: the info block's matrix count), g3d_calc_view is a GUESS,
 *   g3d_calc_view_lc is a GUESS and g3d_calc_view_lc_dma is a GUESS (the three view-matrix calculators
 *   ScnMdlSimple's view pass picks between), g3d_lc_queue_wait is a GUESS, g3d_dc_invalidate_range is a GUESS and
 *   g3d_lc_base is a GUESS (the tail calls of LCQueueWait / DCInvalidateRange and the locked cache's address).
 * RESIDUALS. Unwritten (empty stubs, 27 rows, 0x2C74 bytes; objdiff scores them near zero): fn_8006F738,
 *   Unwritten (empty stubs): g3d_calc_view, g3d_calc_view_lc, g3d_calc_view_lc_dma.
 *   fn_8006F898, fn_8006F8D4, fn_8006F934, fn_8006FB60, fn_8006FBBC, fn_8006FE40, fn_80070134, fn_80070410,
 *   fn_80070600, g3d_calc_view, fn_80071008, mtx34_copy_ps, fn_80071064, fn_800710A0, fn_800710B4, mtx34_concat,
 *   fn_80071130, fn_8007118C, g3d_calc_view_lc, fn_80071B70, fn_80071BD4, g3d_lc_queue_wait, g3d_dc_invalidate_range, g3d_lc_base,
 *   g3d_calc_view_lc_dma, fn_800726D0.
 *   Partial (7 written bodies): fn_8006F908, fn_8006FDCC, fn_8006FE7C, fn_8006FEC8, fn_8006FF50, res_mdl_info_num_pos_nrm_mtx,
 *   fn_800700C0.
 *   flipcheck: `.text` 0x258 of 0x2FD4; `.rodata`, `.data`, `.sdata` and `.sdata2` are claimed and not emitted.
 *   `mtx34_concat` and `mtx34_copy_ps` are unwritten (empty bodies).
 */
#include "types.h"
#include "g3d/g3d_resmat.h" /* nw4r::g3d::ResMdl (rule 2) */
#include "g3d/g3d_resnode.h" /* nw4r::g3d::ResNode (rule 2) */

/* The alignment-assert wrappers need the un-fused compare (retail keeps `clrlwi` + `cmpwi`), exactly
 * as g3d/g3d_basic.cpp and g3d/g3d_calcmaterial.cpp found. */
#pragma peephole off

namespace nw4r {
namespace db {
/* `Panic`/`Warning`; the map names carry the C++ mangling, so they are called through their owner. */
void Panic(const char* pFile, int line, const char* pFmt, ...);
void Warning(const char* pFile, int line, const char* pFmt, ...);
}  // namespace db
}  // namespace nw4r

/* The pooled file-name/assert strings and the two `.sdata` globals this unit reads, declared, not defined: the
 * claimed `.data`/`.sdata` are not emitted yet. */
extern const char lbl_8058D938[];
extern const char lbl_8058D94C[];
extern const char lbl_8058D984[];
extern const char lbl_8058D9C4[];
extern const char lbl_8058D9FC[];
extern const char lbl_8058DA3C[];
extern const char lbl_8058DA70[];
extern const char lbl_8058DC58[];
extern const char lbl_8058DC78[];
extern const char lbl_8058DC8C[];
extern const char lbl_8058DCA8[];
extern const char lbl_8058DCB8[];
extern const char lbl_8058DCE0[];
extern const char lbl_8058DCF0[];
extern const char lbl_8058DCFC[];
extern const char lbl_8058DD18[];
extern const char lbl_8058DD28[];
extern const char lbl_8058DD58[];
extern u32 lbl_807911A8;
extern u32 lbl_807911AC;
extern u32 lbl_807911B0;

extern "C" {

/* -------- the helpers this unit calls (still unsplit `fn_XXXXXXXX`, C linkage) -------- */
f32 sqrt_f32(f32 value);                      /* reciprocal-square-root / length helper */
u32 res_node_is_valid(void* self);
u32* res_node_ptr(void* self);
void fn_80069CF4(void* p0, void* p1);

/* -------- this unit's own bodies -------- */
void fn_8006F738(void* p0, void* p1, void* p2, void* p3, void* p4, void* p5, void* p6, u32 p7);
void fn_8006F898(void* p0, void* p1, void* p2, void* p3, void* p4);
void fn_8006F8D4(void* p0, void* p1, void* p2, void* p3);
f32 fn_8006F908(const f32* pMtx, u32 idx);
void fn_8006F934(void* p0, void* p1, void* p2, void* p3, void* p4, void* p5, void* p6, u32 p7);
void fn_8006FB60(void* p0, void* p1, void* p2, void* p3, f32 p4, f32 p5, f32 p6);
void fn_8006FBBC(void* p0, void* p1, void* p2, void* p3, void* p4, void* p5, void* p6, u32 p7);
u32 fn_8006FDCC(void* self);
void fn_8006FE40(void* self);
u32 fn_8006FE7C(void* self, u32 off);
u32 fn_8006FEC8(void* self, u32 index);
u32 fn_8006FF50(void* self);
u32 fn_8006FFB4(void* self);
const char* fn_8006FFBC(void);
u32 fn_8006FFC8(void* self);
u32 res_mdl_info_num_pos_nrm_mtx(void* self);
u32 fn_80070020(void* self);
u32* fn_80070054(u32* pDst, u32 ptr);
void fn_800700B8(u32* pDst, u32 value);
void fn_80070134(void* p0, void* p1, void* p2, void* p3, void* p4, void* p5, void* p6, u32 p7);
void fn_80070410(void* p0, void* p1, void* p2, void* p3, void* p4, void* p5, void* p6, u32 p7);
void fn_80070600(void* p0, void* p1, void* p2, void* p3, void* p4, void* p5, void* p6, u32 p7);
void g3d_calc_view(void* p0, void* p1, void* p2, void* p3, void* p4, void* p5, void* p6, u32 p7);
void fn_80071008(void* p);
void mtx34_copy_ps(Mtx34* pDst, const Mtx34* pSrc);
void fn_80071064(void* p);
void fn_800710A0(void* p);
void fn_800710B4(void* p);
void mtx34_concat(void* pDst, void* pA, void* pB);
void fn_80071130(void* p);
void fn_8007118C(void* p);
void g3d_calc_view_lc(void* p0, void* p1, void* p2, void* p3, void* p4, void* p5, void* p6, u32 p7);
void fn_80071B70(void* p);
void fn_80071BD0(void* p);
void fn_80071BD4(void* p);
void fn_80071C34(void* p);
void g3d_lc_queue_wait(void* p);
void g3d_dc_invalidate_range(void* p);
void g3d_lc_base(void* p);
void g3d_calc_view_lc_dma(void* p0, void* p1, void* p2, void* p3, void* p4, void* p5, void* p6, u32 p7);
void fn_800726D0(void* p);

/* -------- reconstructed bodies -------- */

/* 0x8006FDCC - the model's node-list accessor. */
u32 fn_8006FDCC(void* self) {
    if (!res_node_is_valid(self)) {
        nw4r::db::Panic(lbl_8058DC78, 83, lbl_8058DC58);
    }
    if (!res_node_is_valid(self)) {
        return 0;
    }
    return *(u32*)((u8*)res_node_ptr(self) + 16);
}

/* 0x8006FE7C - a checked resource pointer from a base handle and an offset (0 -> null). */
u32 fn_8006FE7C(void* self, u32 off) {
    u32 base = *(u32*)self;
    if (off == 0) {
        return (u32)nw4r::g3d::ResNode((void*)0).mpData;
    }
    return (u32)nw4r::g3d::ResNode((void*)(base + off)).mpData;
}

/* 0x8006F908 - the length of column `idx` of a 48-byte (12-float) matrix. */
f32 fn_8006F908(const f32* pMtx, u32 idx) {
    const f32* p = (const f32*)((const u8*)pMtx + idx * 4);
    f32 a = p[8] * p[8];
    f32 b = p[0] * p[0];
    f32 c = p[4] * p[4];
    return sqrt_f32(a + (b + c));
}

/* 0x8006FFB4 - dereference the handle. */
u32 fn_8006FFB4(void* self) {
    return *(u32*)self;
}

/* 0x8006FFBC - the "NodeTree" name string. */
const char* fn_8006FFBC(void) {
    return lbl_8058DCF0;
}

/* 0x8006FFC8 - is the handle non-null. */
u32 fn_8006FFC8(void* self) {
    return *(u32*)self != 0;
}

/* 0x800700B8 - store the 4-byte handle. */
void fn_800700B8(u32* pDst, u32 value) {
    *pDst = value;
}

/* 0x80070054 - the 4-byte-aligned resource pointer constructor (g3d_resmdl_ac.h). */
u32* fn_80070054(u32* pDst, u32 ptr) {
    fn_800700B8(pDst, ptr);
    if (ptr & 0x3) {
        nw4r::db::Panic(lbl_8058DCE0, 57, lbl_8058DCB8);
    }
    return pDst;
}

/* 0x80070020 - the checked pointer the model info lives at. */
u32 fn_80070020(void* self) {
    u8* p = (u8*)&reinterpret_cast<const nw4r::g3d::ResMdl*>(self)->ref();
    u32 handle;
    return *fn_80070054((u32*)&handle, (u32)(p + 76));
}

} /* extern "C" */

/* 0x800700C0 (0x64): returns the model block, panicking on a NULL handle. */
const nw4r::g3d::ResMdlData& nw4r::g3d::ResMdl::ref() const {
    if (!IsValid()) {
        nw4r::db::Panic(lbl_8058DCA8, 120, lbl_8058DC8C, GetClassName(), "ref");
    }
    return *ptr();
}

/* 0x80070124 (0x8): returns the model block. */
const nw4r::g3d::ResMdlData* nw4r::g3d::ResMdl::ptr() const {
    return mpData;
}

/* 0x8007012C (0x8): returns the class name. */
const char* nw4r::g3d::ResMdl::GetClassName() {
    return (const char*)&lbl_807911B0;
}

extern "C" {

/* -------- registration stubs (reconstruction pending) -------- */
void fn_8006F738(void* p0, void* p1, void* p2, void* p3, void* p4, void* p5, void* p6, u32 p7) {}
void fn_8006F898(void* p0, void* p1, void* p2, void* p3, void* p4) {}
void fn_8006F8D4(void* p0, void* p1, void* p2, void* p3) {}
void fn_8006F934(void* p0, void* p1, void* p2, void* p3, void* p4, void* p5, void* p6, u32 p7) {}
void fn_8006FB60(void* p0, void* p1, void* p2, void* p3, f32 p4, f32 p5, f32 p6) {}
void fn_8006FBBC(void* p0, void* p1, void* p2, void* p3, void* p4, void* p5, void* p6, u32 p7) {}
void fn_8006FE40(void* self) {}
u32 fn_8006FEC8(void* self, u32 index) {
    (void)self;
    (void)index;
    return 0;
}
u32 fn_8006FF50(void* self) {
    (void)self;
    return 0;
}
u32 res_mdl_info_num_pos_nrm_mtx(void* self) {
    (void)self;
    return 0;
}
void fn_80070134(void* p0, void* p1, void* p2, void* p3, void* p4, void* p5, void* p6, u32 p7) {}
void fn_80070410(void* p0, void* p1, void* p2, void* p3, void* p4, void* p5, void* p6, u32 p7) {}
void fn_80070600(void* p0, void* p1, void* p2, void* p3, void* p4, void* p5, void* p6, u32 p7) {}
void g3d_calc_view(void* p0, void* p1, void* p2, void* p3, void* p4, void* p5, void* p6, u32 p7) {}
void fn_80071008(void* p) {}
void mtx34_copy_ps(Mtx34* pDst, const Mtx34* pSrc) {}
void fn_80071064(void* p) {}
void fn_800710A0(void* p) {}
void fn_800710B4(void* p) {}
void mtx34_concat(void* pDst, void* pA, void* pB) {}
void fn_80071130(void* p) {}
void fn_8007118C(void* p) {}
void g3d_calc_view_lc(void* p0, void* p1, void* p2, void* p3, void* p4, void* p5, void* p6, u32 p7) {}
void fn_80071B70(void* p) {}
void fn_80071BD0(void* p) {}
void fn_80071BD4(void* p) {}
void fn_80071C34(void* p) {}
void g3d_lc_queue_wait(void* p) {}
void g3d_dc_invalidate_range(void* p) {}
void g3d_lc_base(void* p) {}
void g3d_calc_view_lc_dma(void* p0, void* p1, void* p2, void* p3, void* p4, void* p5, void* p6, u32 p7) {}
void fn_800726D0(void* p) {}

}  // extern "C"
