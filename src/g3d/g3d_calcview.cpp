/*
 * g3d/g3d_calcview.cpp - nw4r g3d view/billboard-matrix calculator: the per-billboard matrix builders
 *   (fn_8006F738, fn_8006F934, fn_8006FBBC) and the material/model accessor chain they drive (fn_8006FDCC ..
 *   fn_80070134).
 * RANGE. .text 0x8006F738-0x8007270C (44 functions); extab, extabindex, .rodata 0x8056F638-0x8056F658, .data
 *   0x8058D938-0x8058DD68 (opens on "g3d_calcview.cpp", then the billboard warning strings), .sdata
 *   0x807911A8-0x807911B8, .sdata2 0x80795DA8-0x80795DB0.  Both neighbours cite their own `__FILE__` strings.
 * NAMES. Map stems.
 *   g3d_dc_flush_range_nosync is a GUESS; test_flag_bit29 is a GUESS; u8_cast is a GUESS;
 *   mtx34_concat_array is a GUESS; round_up_32 is a GUESS; word_forward_a is a GUESS; word_swap_a is a GUESS;
 *   word_forward_b is a GUESS; word_swap_b is a GUESS; g3d_lc_queue_drain is a GUESS;
 *   mtx34_from_rot2d_scale3 is a GUESS; mtx34_from_rot2d_scale1 is a GUESS; mtx34_from_axes_scaled is a GUESS;
 *   PSMTXConcatArray is a GUESS (the evidence follows).
 *   The names follow the bodies: the matrix builders write a 2D rotation / scaled axes, word_swap_* swap two words
 *   through an identity forwarder (the locked-cache double buffers), round_up_32 is nw4r's OSRoundUp32B,
 *   PSMTXConcatArray is the SDK's MTXConcatArray (0x804C5D50 follows PSMTXConcat).
 *   GUESS (from the body and its callers): `mtx34_concat`, `mtx34_copy_ps` (0x8007100C: a paired-single 3x4 matrix
 *   copy, 32 call sites in ef/, enemy/ and g3d/).
 *   res_mdl_info_num_pos_nrm_mtx is a GUESS (0x8006FFDC: the info block's matrix count), g3d_calc_view is a GUESS,
 *   g3d_calc_view_lc is a GUESS and g3d_calc_view_lc_dma is a GUESS (the three view-matrix calculators
 *   ScnMdlSimple's view pass picks between), g3d_lc_queue_wait is a GUESS, g3d_dc_invalidate_range is a GUESS and
 *   g3d_lc_base is a GUESS (the tail calls of LCQueueWait / DCInvalidateRange and the locked cache's address).
 * RESIDUALS. Unwritten (empty stubs, 9 rows, 0x2B3C bytes; objdiff scores them near zero): fn_8006F738, fn_8006F934,
 *   fn_8006FBBC, fn_8006FE40, fn_80070134, fn_80070410, fn_80070600, fn_80071064, and the three view passes
 *   g3d_calc_view, g3d_calc_view_lc, g3d_calc_view_lc_dma (paired-single bodies of 2-2.7 KB: not attempted).
 *   The ResMdlInfo accessors fn_8006FF50 (declared `G3DWorkObj* fn_8006FF50(void)` for g3d_calcworld.cpp's three
 *   callers, which pass r3 through), fn_8006FEC8, res_mdl_info_num_pos_nrm_mtx, fn_8006FE7C, fn_8006FDCC and
 *   fn_8006F908 are partial or stubs; their renames wait for the g3d_calcworld.cpp callers to take the handle.
 *   flipcheck: `.text` short of the claim; `.rodata`, `.data`, `.sdata` and `.sdata2` are claimed and not emitted.
 */
#include "types.h"
#include "g3d/g3d_resmat.h" /* nw4r::g3d::ResMdl (rule 2) */
#include "g3d/g3d_resnode.h" /* nw4r::g3d::ResNode (rule 2) */
#include "fn_8004CAD8.h"       /* mtx34_get_ptr, mtx34_const_ptr (rule 2) */
#include "OS/PSMTXCopy.h"      /* PSMTXCopy, PSMTXConcat, PSMTXConcatArray, owner OS/FindContainHeap_.c (rule 2) */
#include "NAND/DCInvalidateRange.h" /* DCInvalidateRange, DCFlushRangeNoSync, owner NAND/nand.c (rule 2) */
#include "NAND/LCEnable.h"     /* LCQueueLength, LCQueueWait, owner NAND/nand.c (rule 2) */
#include "NAND/OSVReport.h"    /* OSYieldThread, owner NAND/nand.c (rule 2) */

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
extern const f32 calcview_zero_f32; /* 0.0f: the unit's pooled constant (claimed, not emitted) */
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
f32 fn_8006F908(const f32* pMtx, u32 idx);
void fn_8006F934(void* p0, void* p1, void* p2, void* p3, void* p4, void* p5, void* p6, u32 p7);
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
void fn_80071064(void* p);
void g3d_calc_view_lc(void* p0, void* p1, void* p2, void* p3, void* p4, void* p5, void* p6, u32 p7);
void g3d_calc_view_lc_dma(void* p0, void* p1, void* p2, void* p3, void* p4, void* p5, void* p6, u32 p7);

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
void fn_8006F934(void* p0, void* p1, void* p2, void* p3, void* p4, void* p5, void* p6, u32 p7) {}
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
void fn_80071064(void* p) {}
void g3d_calc_view_lc(void* p0, void* p1, void* p2, void* p3, void* p4, void* p5, void* p6, u32 p7) {}
void g3d_calc_view_lc_dma(void* p0, void* p1, void* p2, void* p3, void* p4, void* p5, void* p6, u32 p7) {}

/* 0x8006F898 (0x3C): the matrix of a 2D rotation (cos, sin) with per-axis scales `sx`, `sy`, and `sz` on Z. */
asm void mtx34_from_rot2d_scale3(register Mtx34* pOut, register const f32* pCosSin, register f32 sx, register f32 sy,
                                 register f32 sz)
{
    nofralloc
    lfs f4, calcview_zero_f32(r0)
    ps_merge00 f0, sx, sy
    psq_l f1, 0(pCosSin), 0, 0
    ps_merge10 f2, f1, f1
    ps_mul f2, f2, f0
    psq_st f2, 0(pOut), 0, 0
    ps_neg f2, f1
    ps_merge01 f2, f2, f1
    ps_mul f2, f2, f0
    psq_st f2, 16(pOut), 0, 0
    stfs f4, 8(pOut)
    stfs f4, 24(pOut)
    psq_st f4, 32(pOut), 0, 0
    stfs sz, 40(pOut)
    blr
}

/* 0x8006F8D4 (0x34): the matrix of a 2D rotation (cos, sin) with a uniform scale `s`. */
asm void mtx34_from_rot2d_scale1(register Mtx34* pOut, register const f32* pCosSin, register f32 s)
{
    nofralloc
    lfs f3, calcview_zero_f32(r0)
    psq_l f0, 0(pCosSin), 0, 0
    ps_muls0 f0, f0, s
    ps_merge10 f2, f0, f0
    psq_st f2, 0(pOut), 0, 0
    ps_neg f2, f0
    ps_merge01 f2, f2, f0
    psq_st f2, 16(pOut), 0, 0
    stfs f3, 8(pOut)
    stfs f3, 24(pOut)
    psq_st f3, 32(pOut), 0, 0
    stfs s, 40(pOut)
    blr
}

/* 0x8006FB60 (0x5C): the matrix whose columns are the three vectors scaled by `sa`, `sb`, `sc`. */
asm void mtx34_from_axes_scaled(register Mtx34* pOut, register const f32* pA, register const f32* pB,
                                register const f32* pC, register f32 sa, register f32 sb, register f32 sc)
{
    nofralloc
    psq_l f0, 0(pA), 0, 0
    psq_l f4, 0(pB), 0, 0
    ps_muls0 f0, f0, sa
    ps_muls0 f4, f4, sb
    ps_merge00 f5, f0, f4
    ps_merge11 f0, f0, f4
    psq_st f5, 0(pOut), 0, 0
    psq_st f0, 16(pOut), 0, 0
    psq_l f0, 0(pC), 0, 0
    ps_muls0 f0, f0, sc
    stfs f0, 8(pOut)
    ps_merge11 f0, f0, f0
    stfs f0, 24(pOut)
    lfs f0, 8(pA)
    fmuls f0, f0, sa
    stfs f0, 32(pOut)
    lfs f0, 8(pB)
    fmuls f0, f0, sb
    stfs f0, 36(pOut)
    lfs f0, 8(pC)
    fmuls f0, f0, sc
    stfs f0, 40(pOut)
    blr
}

/* 0x80071008 (0x4): flushes the data-cache range without waiting. */
void g3d_dc_flush_range_nosync(void* pBase, u32 size) { DCFlushRangeNoSync(pBase, size); }

/* 0x8007100C (0x58): copies a 3x4 matrix. */
Mtx34* mtx34_copy_ps(Mtx34* pDst, const Mtx34* pSrc)
{
    PSMTXCopy((const Mtx34*)mtx34_const_ptr((u32)pSrc), (Mtx34*)mtx34_get_ptr(pDst));
    return pDst;
}

/* 0x800710A0 (0x14): whether bit 29 of the flag word is set. */
u32 test_flag_bit29(u32 flags) { return (flags & 0x20000000) != 0; }

/* 0x800710B4 (0x8): the low byte. */
u32 u8_cast(u32 value) { return (u8)value; }

/* 0x800710BC (0x74): concatenates two 3x4 matrices into `pDst`. */
Mtx34* mtx34_concat(Mtx34* pDst, const Mtx34* pA, const Mtx34* pB)
{
    PSMTXConcat((const Mtx34*)mtx34_const_ptr((u32)pA), (const Mtx34*)mtx34_const_ptr((u32)pB),
                (Mtx34*)mtx34_get_ptr(pDst));
    return pDst;
}

/* 0x80071130 (0x5C): concatenates `pA` with `count` matrices from `pSrc` into `pDst`. */
Mtx34* mtx34_concat_array(Mtx34* pDst, const Mtx34* pA, const Mtx34* pSrc, u32 count)
{
    PSMTXConcatArray((const Mtx34*)mtx34_const_ptr((u32)pA), pSrc, pDst, count);
    return pDst;
}

/* 0x8007118C (0xC): rounds `value` up to a multiple of 32. */
u32 round_up_32(u32 value) { return (value + 31) & ~31; }

/* 0x80071BD0 (0x4): hands the word reference back. */
u32* word_forward_a(u32* pWord) { return pWord; }

/* 0x80071B70 (0x60): swaps two words. */
void word_swap_a(u32* pA, u32* pB)
{
    u32 tmp = *word_forward_a(pA);
    *pA = *word_forward_a(pB);
    *pB = *word_forward_a(&tmp);
}

/* 0x80071C34 (0x4): hands the word reference back. */
u32* word_forward_b(u32* pWord) { return pWord; }

/* 0x80071BD4 (0x60): swaps two words. */
void word_swap_b(u32* pA, u32* pB)
{
    u32 tmp = *word_forward_b(pA);
    *pA = *word_forward_b(pB);
    *pB = *word_forward_b(&tmp);
}

/* 0x80071C38 (0x4): waits until the locked-cache queue holds at most `length` DMAs. */
void g3d_lc_queue_wait(u32 length) { LCQueueWait(length); }

/* 0x80071C3C (0x4): invalidates the data-cache range. */
void g3d_dc_invalidate_range(void* pBase, u32 size) { DCInvalidateRange(pBase, size); }

/* 0x80071C40 (0x8): the locked cache's address. */
/* untyped: byte range - the locked cache */
void* g3d_lc_base(void) { return (void*)0xE0000000; }

/* 0x800726D0 (0x3C): yields until at most `length` locked-cache DMAs are queued. */
void g3d_lc_queue_drain(u32 length)
{
    while (LCQueueLength() > length) {
        OSYieldThread();
    }
}

}  // extern "C"
