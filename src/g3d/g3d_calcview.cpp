/*
 * g3d/g3d_calcview.cpp - nw4r g3d view/billboard-matrix calculator: the per-billboard matrix builders
 *   (fn_8006F738, fn_8006F934, fn_8006FBBC) and the material/model accessor chain they drive (fn_8006FDCC ..
 *   fn_80070134).
 * RANGE. .text 0x8006F738-0x8007270C (44 functions); extab, extabindex, .rodata 0x8056F638-0x8056F658, .data
 *   0x8058D938-0x8058DD68 (opens on "g3d_calcview.cpp", then the billboard warning strings), .sdata
 *   0x807911A8-0x807911B8, .sdata2 0x80795DA8-0x80795DB0.  Both neighbours cite their own `__FILE__` strings.
 * NAMES. g3d_billboard_work_mtx is a GUESS; mtx34_inverse_affine is a GUESS; mtx34_inverse_transpose is a GUESS;
 *   g3d_billboard_func_tbl is a GUESS (the evidence follows).
 *   mtx34_calc_billboard_std is a GUESS; mtx34_calc_billboard_std_persp is a GUESS;
 *   mtx34_calc_billboard_y is a GUESS; mtx34_calc_billboard_y_persp is a GUESS;
 *   mtx34_calc_billboard_y_rot is a GUESS; mtx34_calc_billboard_y_rot_persp is a GUESS;
 *   mtx34_axis_in_parent is a GUESS; mtx34_up_axis_in_parent is a GUESS; res_mdl_info_handle is a GUESS;
 *   calcview_epsilon_f32 is a GUESS; calcview_zero_f32 is a GUESS (the evidence follows).
 *   res_mdl_info_is_valid is a GUESS; res_mdl_info_get_class_name is a GUESS; res_mdl_info_ptr is a GUESS;
 *   res_mdl_info_data is a GUESS; res_mdl_info_get_node_of_pos_nrm_mtx is a GUESS;
 *   res_node_get_parent_const is a GUESS; res_node_get_child is a GUESS (the evidence follows).
 *   Map stems.
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
 * RESIDUALS. Unwritten (empty stubs, 2 rows, 0x1460 bytes; objdiff scores them near zero): g3d_calc_view_lc and
 *   g3d_calc_view_lc_dma (the locked-cache variants of g3d_calc_view: not attempted).  g3d_calc_view is 99.8 %: the
 *   normal and texture matrix pointers of its second loop take r19/r20 swapped.
 *   Partial: fn_8006F908 (98 %), mtx34_calc_billboard_y and mtx34_calc_billboard_y_persp (99.8 %: the parent node index
 *   takes r5 where retail keeps it in r0).
 *   flipcheck: `.text` short of the claim; `.rodata`, `.data`, `.sdata` and `.sdata2` are claimed and not emitted.
 */
#include "types.h"
#include "g3d/g3d_resmat.h" /* nw4r::g3d::ResMdl (rule 2) */
#include "g3d/g3d_resnode.h" /* nw4r::g3d::ResNode (rule 2) */
#include "g3d/res_mdl_info.h" /* ResMdlInfoData (rule 1) */
#include "g3d/g3d_calcmaterial.h" /* mtx34_axis_in_parent (rule 2) */
#include "g3d/g3d_anmchr.h" /* res_node_ref, res_node_ref_nonconst, res_node_ofs_to_node (rule 2) */
#include "hud/mtx34_inverse_affine.h" /* mtx34_inverse_affine, mtx34_inverse_transpose, mtx34_to_mtx33, owner hud/pl_frame_sync.cpp (rule 2) */
#include "g3d/g3d_pointer_assert.h" /* G3D_POINTER_ASSERT (rule 1) */
#include "mh3_pad.h"          /* setVec3 (rule 2) */
#include "nw4r/fn_805012C4.h"  /* nw4r::math::MTX34Zero (rule 2) */
#include "fn_8004CAD8.h"       /* mtx34_get_ptr, mtx34_const_ptr (rule 2) */
#include "OS/PSMTXCopy.h"      /* PSMTXCopy, PSMTXConcat, PSMTXConcatArray, owner OS/FindContainHeap_.c (rule 2) */
#include "NAND/DCInvalidateRange.h" /* DCInvalidateRange, DCFlushRangeNoSync, owner NAND/nand.c (rule 2) */
#include "NAND/LCEnable.h"     /* LCQueueLength, LCQueueWait, owner NAND/nand.c (rule 2) */
#include "NAND/OSVReport.h"    /* OSYieldThread, owner NAND/nand.c (rule 2) */

/* The alignment-assert wrappers need the un-fused compare (retail keeps `clrlwi` + `cmpwi`), exactly
 * as g3d/g3d_basic.cpp and g3d/g3d_calcmaterial.cpp found. */
#pragma peephole off
#pragma pool_data off

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
extern const f32 calcview_epsilon_f32; /* 1.0e-18f */
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
f32 fn_8006F908(const f32* pMtx, u32 idx);
u32 fn_8006FDCC(void* self);
u32 res_node_get_parent_const(const ResHandle* pSelf);
const ResMdlInfoData* res_mdl_info_data(const void* pInfo);
u32 res_mdl_info_get_node_of_pos_nrm_mtx(const void* pInfo, u32 mtxID);
u32 res_mdl_info_num_pos_nrm_mtx(const void* pInfo);
u32 fn_8006FE7C(void* self, u32 off);
u32 res_mdl_info_ptr(void* self);
const char* res_mdl_info_get_class_name(void);
u32 res_mdl_info_is_valid(void* self);
u32 res_mdl_info_handle(void* self);
u32* fn_80070054(u32* pDst, u32 ptr);
void fn_800700B8(u32* pDst, u32 value);
u32 res_node_get_child(ResHandle* pSelf);
void g3d_calc_view_lc(void* p0, void* p1, void* p2, void* p3, void* p4, void* p5, void* p6, u32 p7);
void g3d_calc_view_lc_dma(void* p0, void* p1, void* p2, void* p3, void* p4, void* p5, void* p6, u32 p7);

/* -------- reconstructed bodies -------- */

/* 0x8006FDCC - the model's node-list accessor. */
u32 fn_8006FDCC(void* self) {
    if (!res_node_is_valid(self)) {
        nw4r::db::Panic(lbl_8058DC78, 83, lbl_8058DC58);
    }
    if (res_node_is_valid(self)) {
        return *(u32*)((u8*)res_node_ptr(self) + 16);
    }
    return 0;
}

/* 0x8006FE7C - a checked resource pointer from a base handle and an offset (0 -> null). */
u32 fn_8006FE7C(void* self, u32 off) {
    u32 base = *(u32*)self;
    if (off != 0) {
        return (u32)nw4r::g3d::ResNode((void*)(base + off)).mpData;
    }
    return (u32)nw4r::g3d::ResNode((void*)0).mpData;
}

/* 0x8006FE40 (0x3C): the parent node. */
u32 res_node_get_parent_const(const ResHandle* pSelf)
{
    return fn_8006FE7C((void*)pSelf, res_node_ref(pSelf)->mToParentNode);
}

/* 0x80071064 (0x3C): the first child node. */
u32 res_node_get_child(ResHandle* pSelf)
{
    return res_node_ofs_to_node(pSelf, res_node_ref_nonconst(pSelf)->mToChildNode);
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
u32 res_mdl_info_ptr(void* self) {
    return *(u32*)self;
}

/* 0x8006FFBC - the "NodeTree" name string. */
const char* res_mdl_info_get_class_name(void) {
    return lbl_8058DCF0;
}

/* 0x8006FFC8 - is the handle non-null. */
u32 res_mdl_info_is_valid(void* self) {
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
u32 res_mdl_info_handle(void* self) {
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
/* 0x8006FF50 (0x34): the info block, asserting the handle is valid. */
/* untyped: opaque handle - the ResMdlInfo handle */
const ResMdlInfoData* res_mdl_info_data(const void* pInfo)
{
    if (!res_mdl_info_is_valid((void*)pInfo)) {
        nw4r::db::Panic(lbl_8058DD18, 57, lbl_8058DCFC, res_mdl_info_get_class_name(), (const char*)&lbl_807911A8);
    }
    return (const ResMdlInfoData*)res_mdl_info_ptr((void*)pInfo);
}

/* 0x8006FEC8 (0x58): the node id of position/normal matrix `mtxID`. */
/* untyped: opaque handle - the ResMdlInfo handle */
u32 res_mdl_info_get_node_of_pos_nrm_mtx(const void* pInfo, u32 mtxID)
{
    if (!(mtxID < res_mdl_info_num_pos_nrm_mtx(pInfo))) {
        nw4r::db::Panic(lbl_8058DD58, 103, lbl_8058DD28);
    }
    u32 ofs = res_mdl_info_data(pInfo)->mToPosNrmMtxTable;
    const u32* pTable = (const u32*)((const u8*)res_mdl_info_data(pInfo) + ofs + 4);
    return pTable[mtxID];
}

/* 0x8006FFDC (0x44): the number of position/normal matrices. */
/* untyped: opaque handle - the ResMdlInfo handle */
u32 res_mdl_info_num_pos_nrm_mtx(const void* pInfo)
{
    u32 ofs = res_mdl_info_data(pInfo)->mToPosNrmMtxTable;
    return *(const u32*)((const u8*)res_mdl_info_data(pInfo) + ofs);
}
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

/* 0x8006F738 (0x160): the matrix of a 'std' billboard: the Y axis of `pOut` (read as its column 1) normalised in the
 * XY plane, rotated onto the node's world scale (uniform when `bUniformScale`). */
void mtx34_calc_billboard_std(Mtx34* pOut, const Mtx34* pWorldArray, u32 bUniformScale, const Mtx34* pCamera, const nw4r::g3d::ResMdl* pMdl,
                              u32 mtxId)
{
    nw4r::math::VEC3 axis;
    setVec3(&axis, pOut->m[0][1], pOut->m[1][1], calcview_zero_f32);
    if (abs_f32(axis.x) >= calcview_epsilon_f32 || abs_f32(axis.y) >= calcview_epsilon_f32) {
        vec3_normalize_into(&axis, &axis);
        if (bUniformScale) {
            mtx34_from_rot2d_scale1(pOut, &axis.x, fn_8006F908((const f32*)&pWorldArray[mtxId], 0));
        } else {
            const Mtx34* pWorld = &pWorldArray[mtxId];
            f32 sx = fn_8006F908((const f32*)pWorld, 0);
            f32 sy = fn_8006F908((const f32*)pWorld, 1);
            mtx34_from_rot2d_scale3(pOut, &axis.x, sx, sy, fn_8006F908((const f32*)pWorld, 2));
        }
    } else {
        nw4r::db::Warning(lbl_8058D938, 0x24B, lbl_8058D94C);
        nw4r::math::MTX34Zero(pOut);
    }
}

/* 0x8006F934 (0x22C): the matrix of a 'std_persp' billboard: the node's up axis kept, the Z axis turned onto the eye
 * direction (the translation negated), the X axis their cross product, each scaled by the node's world scale. */
void mtx34_calc_billboard_std_persp(Mtx34* pOut, const Mtx34* pWorldArray, u32 bUniformScale, const Mtx34* pCamera, const nw4r::g3d::ResMdl* pMdl,
                                    u32 mtxId)
{
    nw4r::math::VEC3 right;
    VEC3_ctor(&right);
    nw4r::math::VEC3 up;
    setVec3(&up, pOut->m[0][1], pOut->m[1][1], pOut->m[2][1]);
    nw4r::math::VEC3 eye;
    setVec3(&eye, -pOut->m[0][3], -pOut->m[1][3], -pOut->m[2][3]);
    if (abs_f32(eye.x) >= calcview_epsilon_f32 || abs_f32(eye.y) >= calcview_epsilon_f32 ||
        abs_f32(eye.z) >= calcview_epsilon_f32) {
        vec3_normalize_into(&eye, &eye);
        vec3_cross(&right.x, &up.x, &eye.x);
        if (abs_f32(right.x) >= calcview_epsilon_f32 || abs_f32(right.y) >= calcview_epsilon_f32 ||
            abs_f32(right.z) >= calcview_epsilon_f32) {
            vec3_normalize_into(&right, &right);
            vec3_cross(&up.x, &eye.x, &right.x);
            if (bUniformScale) {
                f32 s = fn_8006F908((const f32*)&pWorldArray[mtxId], 0);
                mtx34_from_axes_scaled(pOut, &right.x, &up.x, &eye.x, s, s, s);
            } else {
                const Mtx34* pWorld = &pWorldArray[mtxId];
                f32 sx = fn_8006F908((const f32*)pWorld, 0);
                f32 sy = fn_8006F908((const f32*)pWorld, 1);
                mtx34_from_axes_scaled(pOut, &right.x, &up.x, &eye.x, sx, sy, fn_8006F908((const f32*)pWorld, 2));
            }
            return;
        }
    }
    nw4r::db::Warning(lbl_8058D938, 0x292, lbl_8058D984);
    nw4r::math::MTX34Zero(pOut);
}

/* A node handle word wrapped in a temporary, so a copy helper gets its address. */
struct CalcViewHandleWord {
    CalcViewHandleWord(u32 word) { mWord = word; }
    /* +0x00 */ u32 mWord;
}; /* size: 0x4 */

/* 0x8006FBBC (0x210): the matrix of a 'y' billboard: the node's axis expressed against its parent (the node's own
 * when it has none), normalised in the XY plane and applied with the node's world scale. */
void mtx34_calc_billboard_y(Mtx34* pOut, const Mtx34* pWorldArray, u32 bUniformScale, const Mtx34* pCamera,
                            const nw4r::g3d::ResMdl* pMdl, u32 mtxId)
{
    nw4r::math::VEC3 axis;
    VEC3_ctor(&axis);
    u32 node;
    u32 parent;
    u32 info = res_mdl_info_handle((void*)pMdl);
    const Mtx34* pWorld;
    int nodeId = res_mdl_info_get_node_of_pos_nrm_mtx(&info, mtxId);
    if (nodeId >= 0) {
        res_node_copy_ctor(&node, (u32*)&CalcViewHandleWord((u32)pMdl->GetResNode(nodeId).mpData).mWord);
        res_node_copy_ctor(&parent, &CalcViewHandleWord(res_node_get_parent_const((const ResHandle*)&node)).mWord);
        if (res_node_is_valid(&parent)) {
            u32 parentId = fn_8006FDCC(&parent);
            pWorld = &pWorldArray[mtxId];
            mtx34_axis_in_parent(&axis.x, (const f32*)pWorld, (const f32*)&pWorldArray[parentId]);
        } else {
            pWorld = &pWorldArray[mtxId];
            axis.x = pWorld->m[0][1];
            axis.y = pWorld->m[1][1];
            axis.z = calcview_zero_f32;
        }
    } else {
        pWorld = &pWorldArray[mtxId];
        axis.x = pWorld->m[0][1];
        axis.y = pWorld->m[1][1];
        axis.z = calcview_zero_f32;
    }
    if (abs_f32(axis.x) >= calcview_epsilon_f32 || abs_f32(axis.y) >= calcview_epsilon_f32) {
        vec3_normalize_into(&axis, &axis);
        if (bUniformScale) {
            mtx34_from_rot2d_scale1(pOut, &axis.x, fn_8006F908((const f32*)pWorld, 0));
        } else {
            f32 sx = fn_8006F908((const f32*)pWorld, 0);
            f32 sy = fn_8006F908((const f32*)pWorld, 1);
            mtx34_from_rot2d_scale3(pOut, &axis.x, sx, sy, fn_8006F908((const f32*)pWorld, 2));
        }
    } else {
        nw4r::db::Warning(lbl_8058D938, 0x2F8, lbl_8058D9C4);
        nw4r::math::MTX34Zero(pOut);
    }
}

/* 0x80070134 (0x2DC): the matrix of a 'y_persp' billboard: the Z axis turned onto the eye direction, the up axis taken
 * from the node against its parent (the node's own when it has none), the X axis their cross product. */
void mtx34_calc_billboard_y_persp(Mtx34* pOut, const Mtx34* pWorldArray, u32 bUniformScale, const Mtx34* pCamera,
                                  const nw4r::g3d::ResMdl* pMdl, u32 mtxId)
{
    nw4r::math::VEC3 right;
    VEC3_ctor(&right);
    nw4r::math::VEC3 up;
    VEC3_ctor(&up);
    nw4r::math::VEC3 eye;
    setVec3(&eye, -pOut->m[0][3], -pOut->m[1][3], -pOut->m[2][3]);
    u32 node;
    u32 parent;
    u32 info = res_mdl_info_handle((void*)pMdl);
    const Mtx34* pWorld;
    int nodeId = res_mdl_info_get_node_of_pos_nrm_mtx(&info, mtxId);
    if (nodeId >= 0) {
        res_node_copy_ctor(&node, (u32*)&CalcViewHandleWord((u32)pMdl->GetResNode(nodeId).mpData).mWord);
        res_node_copy_ctor(&parent, &CalcViewHandleWord(res_node_get_parent_const((const ResHandle*)&node)).mWord);
        if (res_node_is_valid(&parent)) {
            u32 parentId = fn_8006FDCC(&parent);
            pWorld = &pWorldArray[mtxId];
            mtx34_up_axis_in_parent(&up.x, (const f32*)pWorld, (const f32*)&pWorldArray[parentId]);
        } else {
            pWorld = &pWorldArray[mtxId];
            up.x = pWorld->m[0][1];
            up.y = pWorld->m[1][1];
            up.z = pWorld->m[2][1];
        }
    } else {
        pWorld = &pWorldArray[mtxId];
        up.x = pWorld->m[0][1];
        up.y = pWorld->m[1][1];
        up.z = pWorld->m[2][1];
    }
    if (abs_f32(eye.x) >= calcview_epsilon_f32 || abs_f32(eye.y) >= calcview_epsilon_f32 ||
        abs_f32(eye.z) >= calcview_epsilon_f32) {
        vec3_normalize_into(&eye, &eye);
        vec3_cross(&right.x, &up.x, &eye.x);
        if (abs_f32(right.x) >= calcview_epsilon_f32 || abs_f32(right.y) >= calcview_epsilon_f32 ||
            abs_f32(right.z) >= calcview_epsilon_f32) {
            vec3_normalize_into(&right, &right);
            vec3_cross(&up.x, &eye.x, &right.x);
            if (bUniformScale) {
                f32 s = fn_8006F908((const f32*)pWorld, 0);
                mtx34_from_axes_scaled(pOut, &right.x, &up.x, &eye.x, s, s, s);
            } else {
                f32 sx = fn_8006F908((const f32*)pWorld, 0);
                f32 sy = fn_8006F908((const f32*)pWorld, 1);
                mtx34_from_axes_scaled(pOut, &right.x, &up.x, &eye.x, sx, sy, fn_8006F908((const f32*)pWorld, 2));
            }
            return;
        }
    }
    nw4r::db::Warning(lbl_8058D938, 0x363, lbl_8058D9FC);
    nw4r::math::MTX34Zero(pOut);
}

/* 0x80070410 (0x1F0): the matrix of a 'y' rotating billboard: the node's up axis divided by its Y scale, the X axis its
 * XY-plane perpendicular, the Z axis their cross product. */
void mtx34_calc_billboard_y_rot(Mtx34* pOut, const Mtx34* pWorldArray, u32 bUniformScale, const Mtx34* pCamera, const nw4r::g3d::ResMdl* pMdl,
                                u32 mtxId)
{
    nw4r::math::VEC3 right;
    VEC3_ctor(&right);
    nw4r::math::VEC3 up;
    setVec3(&up, pOut->m[0][1], pOut->m[1][1], pOut->m[2][1]);
    nw4r::math::VEC3 axis;
    setVec3(&axis, up.y, -up.x, calcview_zero_f32);
    if (abs_f32(up.x) >= calcview_epsilon_f32 || abs_f32(up.y) >= calcview_epsilon_f32 ||
        abs_f32(up.z) >= calcview_epsilon_f32) {
        const Mtx34* pWorld = &pWorldArray[mtxId];
        f32 sy = fn_8006F908((const f32*)pWorld, 1);
        vec3_scale_in_place(&up, math_reciprocal(sy));
        if (abs_f32(axis.x) >= calcview_epsilon_f32 || abs_f32(axis.y) >= calcview_epsilon_f32) {
            vec3_normalize_into(&axis, &axis);
            vec3_cross(&right.x, &axis.x, &up.x);
            if (bUniformScale) {
                mtx34_from_axes_scaled(pOut, &axis.x, &up.x, &right.x, sy, sy, sy);
            } else {
                f32 sx = fn_8006F908((const f32*)pWorld, 0);
                mtx34_from_axes_scaled(pOut, &axis.x, &up.x, &right.x, sx, sy, fn_8006F908((const f32*)pWorld, 2));
            }
            return;
        }
    }
    nw4r::db::Warning(lbl_8058D938, 0x3AB, lbl_8058DA3C);
    nw4r::math::MTX34Zero(pOut);
}

/* 0x80070600 (0x220): the matrix of a 'y' billboard facing the eye: the node's up axis divided by its Y scale, the X axis
 * perpendicular to it and the eye direction (the translation negated), the Z axis their cross product. */
void mtx34_calc_billboard_y_rot_persp(Mtx34* pOut, const Mtx34* pWorldArray, u32 bUniformScale, const Mtx34* pCamera,
                                      const nw4r::g3d::ResMdl* pMdl, u32 mtxId)
{
    nw4r::math::VEC3 right;
    VEC3_ctor(&right);
    nw4r::math::VEC3 up;
    setVec3(&up, pOut->m[0][1], pOut->m[1][1], pOut->m[2][1]);
    nw4r::math::VEC3 eye;
    setVec3(&eye, -pOut->m[0][3], -pOut->m[1][3], -pOut->m[2][3]);
    if (abs_f32(up.x) >= calcview_epsilon_f32 || abs_f32(up.y) >= calcview_epsilon_f32 ||
        abs_f32(up.z) >= calcview_epsilon_f32) {
        const Mtx34* pWorld = &pWorldArray[mtxId];
        f32 sy = fn_8006F908((const f32*)pWorld, 1);
        vec3_scale_in_place(&up, math_reciprocal(sy));
        vec3_cross(&right.x, &up.x, &eye.x);
        if (abs_f32(right.x) >= calcview_epsilon_f32 || abs_f32(right.y) >= calcview_epsilon_f32 ||
            abs_f32(right.z) >= calcview_epsilon_f32) {
            vec3_normalize_into(&right, &right);
            vec3_cross(&eye.x, &right.x, &up.x);
            if (bUniformScale) {
                mtx34_from_axes_scaled(pOut, &right.x, &up.x, &eye.x, sy, sy, sy);
            } else {
                f32 sx = fn_8006F908((const f32*)pWorld, 0);
                mtx34_from_axes_scaled(pOut, &right.x, &up.x, &eye.x, sx, sy, fn_8006F908((const f32*)pWorld, 2));
            }
            return;
        }
    }
    nw4r::db::Warning(lbl_8058D938, 0x3F4, lbl_8058DA70);
    nw4r::math::MTX34Zero(pOut);
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

/* The billboard builders `g3d_calc_view` dispatches to by the attribute's billboard index (0: none). */
typedef void (*BillboardFunc)(Mtx34* pOut, const Mtx34* pWorldArray, u32 bUniformScale, const Mtx34* pCamera,
                              const nw4r::g3d::ResMdl* pMdl, u32 mtxId);

const BillboardFunc g3d_billboard_func_tbl[8] = {
    NULL,
    mtx34_calc_billboard_std,
    mtx34_calc_billboard_std_persp,
    mtx34_calc_billboard_y,
    mtx34_calc_billboard_y_persp,
    mtx34_calc_billboard_y_rot,
    mtx34_calc_billboard_y_rot_persp,
    NULL,
};

/* 0x80070820 (0x7E8): view matrices of a model in main memory: the camera times each world matrix, billboards
 * rebuilt in place, then the normal (and texture) matrices; the written ranges are flushed. */
void g3d_calc_view(nw4r::math::MTX34* pViewPosMtxArray, nw4r::math::MTX33* pViewNrmMtxArray,
                   const nw4r::math::MTX34* pWorldMtxArray, const u32* pWorldMtxAttribArray, u32 numMtx,
                   const nw4r::math::MTX34* pCamera, const nw4r::g3d::ResMdl* pMdl,
                   nw4r::math::MTX34* pViewTexMtxArray)
{
    G3D_POINTER_ASSERT(lbl_8058D938, pViewPosMtxArray, 0x41A,
                       "NW4R:Pointer Error\npViewPosArray(=%p) is not valid pointer.");
    G3D_POINTER_ASSERT(lbl_8058D938, pWorldMtxArray, 0x41B,
                       "NW4R:Pointer Error\npModelMtxArray(=%p) is not valid pointer.");
    G3D_POINTER_ASSERT(lbl_8058D938, pWorldMtxAttribArray, 0x41C,
                       "NW4R:Pointer Error\npModelMtxAttribArray(=%p) is not valid pointer.");
    G3D_POINTER_ASSERT(lbl_8058D938, pCamera, 0x41D, "NW4R:Pointer Error\npView(=%p) is not valid pointer.");
    if (numMtx != 0) {
        u32 mtxBytes = numMtx * 0x30;
        u32 posSize = round_up_32(mtxBytes);
        u32 nrmSize = round_up_32(numMtx * 0x24);
        u32 texSize = round_up_32(mtxBytes);
        if (numMtx > 1) {
            mtx34_concat_array(pViewPosMtxArray, pCamera, pWorldMtxArray, numMtx);
        } else {
            mtx34_concat(pViewPosMtxArray, pCamera, pWorldMtxArray);
        }
        Mtx34* pWork = (Mtx34*)g3d_billboard_work_mtx();
        for (u32 i = 0; i < numMtx; i++) {
            Mtx34 inverse;
            u32 node;
            u32 parentNode;
            ResHandle mdl;
            u32 info;
            u32 nodeTmp;
            u32 child;
            u32 parentInfo;
            u32 parentTmp;
            u32 parent;
            u32 attr = pWorldMtxAttribArray[i];
            int bbIdx = u8_cast(attr);
            if (bbIdx != 0) {
                if (!pMdl->IsValid()) {
                    nw4r::db::Panic(lbl_8058D938, 0x44A, "NW4R:Failed assertion resMdl.IsValid()");
                }
                if (!(bbIdx < 7)) {
                    nw4r::db::Panic(lbl_8058D938, 0x44B, "NW4R:Failed assertion bbidx < ResNodeData::NUM_BILLBOARD");
                }
                mdl.mpData = ((const ResHandle*)pMdl)->mpData;
                g3d_billboard_func_tbl[bbIdx](&pViewPosMtxArray[i], pWorldMtxArray, test_flag_bit29(attr), pCamera,
                                              (const nw4r::g3d::ResMdl*)&mdl, i);
                info = res_mdl_info_handle((void*)pMdl);
                int nodeId = res_mdl_info_get_node_of_pos_nrm_mtx(&info, i);
                if (nodeId < 0) {
                    nw4r::db::Panic(lbl_8058D938, 0x456, "NW4R:Failed assertion node_id >= 0");
                }
                nodeTmp = (u32)pMdl->GetResNode((u32)nodeId).mpData;
                res_node_copy_ctor(&node, &nodeTmp);
                bool bHasChild = false;
                if (res_node_is_valid(&node)) {
                    child = res_node_get_child((ResHandle*)&node);
                    if (res_node_is_valid(&child)) {
                        bHasChild = true;
                    }
                }
                if (bHasChild) {
                    MTX34_ctor(&inverse);
                    if (mtx34_inverse_affine(&inverse, &pWorldMtxArray[i]) == 1) {
                        mtx34_concat(&pWork[i], &pViewPosMtxArray[i], &inverse);
                    } else {
                        mtx34_identity(&pWork[i]);
                        pWork[i].m[0][3] = pCamera->m[0][3];
                        pWork[i].m[1][3] = pCamera->m[1][3];
                        pWork[i].m[2][3] = pCamera->m[2][3];
                    }
                }
            } else {
                parentInfo = res_mdl_info_handle((void*)pMdl);
                int parentNodeId = res_mdl_info_get_node_of_pos_nrm_mtx(&parentInfo, i);
                if (parentNodeId >= 0) {
                    parentTmp = (u32)pMdl->GetResNode((u32)parentNodeId).mpData;
                    res_node_copy_ctor(&parentNode, &parentTmp);
                    if (res_node_is_valid(&parentNode) && (res_node_ref_nonconst((ResHandle*)&parentNode)->mFlags & 0x400)) {
                        parent = (u32)pMdl->GetResNode((u32)res_node_ref_nonconst((ResHandle*)&parentNode)->mBillboardRefNodeID).mpData;
                        u32 parentId = fn_8006FDCC(&parent);
                        if (!(parentId < i)) {
                            nw4r::db::Panic(lbl_8058D938, 0x47F, "The billboard matrix hasn't be calculated yet.");
                        }
                        mtx34_concat(&pViewPosMtxArray[i], &pWork[parentId], &pWorldMtxArray[i]);
                    }
                }
            }
        }
        if (pViewNrmMtxArray != NULL) {
            for (u32 i = 0; i < numMtx; i++) {
                if (test_flag_bit29(pWorldMtxAttribArray[i]) != 0) {
                    if (pViewTexMtxArray != NULL) {
                        mtx34_copy_ps(&pViewTexMtxArray[i], &pViewPosMtxArray[i]);
                        pViewTexMtxArray[i].m[2][3] = calcview_zero_f32;
                        pViewTexMtxArray[i].m[1][3] = calcview_zero_f32;
                        pViewTexMtxArray[i].m[0][3] = calcview_zero_f32;
                    }
                    nw4r::math::MTX34ToMTX33(&pViewNrmMtxArray[i], &pViewPosMtxArray[i]);
                } else if (pViewTexMtxArray != NULL) {
                    mtx34_inverse_transpose(&pViewTexMtxArray[i], &pViewPosMtxArray[i]);
                    nw4r::math::MTX34ToMTX33(&pViewNrmMtxArray[i], &pViewTexMtxArray[i]);
                } else {
                    mtx34_to_mtx33(&pViewNrmMtxArray[i], &pViewPosMtxArray[i]);
                }
            }
        }
        g3d_dc_flush_range_nosync(pViewPosMtxArray, posSize);
        if (pViewNrmMtxArray != NULL) {
            g3d_dc_flush_range_nosync(pViewNrmMtxArray, nrmSize);
            if (pViewTexMtxArray != NULL) {
                g3d_dc_flush_range_nosync(pViewTexMtxArray, texSize);
            }
        }
    }
}

}  // extern "C"
