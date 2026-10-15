/*
 * g3d/g3d_anmshp.cpp - nw4r g3d `g3d_anmshp.cpp`: the shape animation object (`AnmObjShp`), the checked
 *   `ResAnmShp` resolver and the `ResVtxPos`/`ResVtxNrm`/`ResVtxClr`/`ResDic` accessors of the headers it includes.
 * RANGE. .text 0x8006946C-0x800697D4; extab 0x80007BB4-0x80007BEC, extabindex 0x8002004C-0x800200A0, .rodata
 *   0x8056F598-0x8056F5A8, .data 0x8058CC40-0x8058CD40, .sdata 0x80791188-0x80791190.  The TU is `g3d_anmshp.cpp`:
 *   its one-copy `__FILE__` string (0x8058CC40) is cited by anm_obj_shp_test_existence; the previous TU's (`g3d_anmscn.cpp`,
 *   0x8058C288) last by fn_80068FC4, the next TU's (`g3d_anmtexpat.cpp`, 0x8058CD40) first by
 *   AnmObjTexPat::TestExistence (0x800697D4).  The "AnmObjShp" record (0x8056F598) is read by
 *   AnmObjShp::GetTypeObjStatic, the last function of the range.
 * FLAGS. `cflags_g3d` (docs/g3d.md).
 * NAMES. ResVtxPos's ref/ptr/GetID/GetClassName/IsValid/copy constructor, ResVtxNrm::ptr, ResVtxClr::ptr,
 *   ResDic::GetNumData and AnmObjShp::GetTypeObjStatic are nw4r's members; the stems are defined `extern "C"`
 *   (playbook 48).  The cross-unit declarations are `g3d/g3d_anmshp.h`.
 *   anm_obj_shp_test_existence is AnmObjShp::TestExistence (the "int(vtxId)" assert), kept as a C function because
 *   `g3d/g3d_calcvtx.cpp` calls it that way.  anm_shp_res_reader is a GUESS (the
 *   stripped reader below).  anm_typename_AnmObjShp follows the `anm_typename_*` records (map row lbl_8056F598).
 * RESIDUALS. none in code.  .sdata: the two "ref" words are byte-identical but swapped - ResVtxPos::ref names the
 *   first and the `ResAnmShp` resolver the second in retail, ours the reverse (a trial flip links with exactly those
 *   two SDA21 low bytes different).  anm_shp_res_reader is extra in the object (linker-stripped, its extabindex
 *   record goes with it).
 * SHAPES. `#pragma peephole off` (playbook idea 106) and `#pragma pool_data off`.  The data is the unit's own:
 *   literals, a defined `TypeNameData` record and the `_ac.h` accessors as `inline` functions under
 *   `#pragma dont_inline`, defined ResVtxPos first (their strings land last in the inline tail) and referenced by
 *   one stripped reader in reverse retail order (MWCC emits a function's inline references last-referenced first).
 */

#include "types.h"
#include "nw4r/db_assert.h"   /* nw4r::db::Panic, owner nw4r/db_assert.cpp (rule 2) */
#include "g3d/g3d_anmshp.h"
#include "g3d/g3d_anmchr.h"   /* fn_8005DC60, res_dic_is_valid, res_dic_ptr and G3dObj, owned by g3d/g3d_anmchr.cpp (rule 2) */
#include "g3d/g3d_resvtx.h"   /* nw4r::g3d::ResVtxPos / ResVtxNrm / ResVtxClr (rule 1) */
#include "g3d/g3d_pointer_assert.h" /* G3D_POINTER_ASSERT (rule 1) */

#pragma peephole off

#pragma pool_data off

/* The AnmObjShp run-time type name GetTypeObjStatic points at. */
extern "C" const nw4r::g3d::TypeNameData<sizeof("AnmObjShp")> anm_typename_AnmObjShp = {sizeof("AnmObjShp"),
                                                                                         "AnmObjShp"};

/* The `_ac.h` accessors, defined `inline` below their referencers. */
extern "C" u32 fn_80069650(void* p);
extern "C" u32 fn_800696AC(void* p);
extern "C" void fn_80069798(u32* dst, const u32* src);
extern "C" const char* fn_80069658(void);

#pragma dont_inline on

namespace nw4r {
namespace g3d {

/* 0x800696C0 (0x24): returns the array's ID. */
inline u32 ResVtxPos::GetID() const {
    return ref().id;
}

/* 0x800696E4 (0x64): returns the position block, panicking on a NULL handle. */
inline const ResVtxPosData& ResVtxPos::ref() const {
    if (!IsValid()) {
        nw4r::db::Panic("g3d_resvtx_ac.h", 39, "%s::%s: Object not valid.", GetClassName(), "ref");
    }
    return *ptr();
}

/* 0x80069748 (0xC): returns the class name. */
inline const char* ResVtxPos::GetClassName() {
    return "ResVtxPos";
}

/* 0x80069754 (0x14): tells whether the handle is set. */
inline bool ResVtxPos::IsValid() const {
    return mpData != NULL;
}

}  // namespace g3d
}  // namespace nw4r

/* 0x800695EC (0x64): resolves the `ResAnmShp` handle, panicking on a NULL handle. */
extern "C" inline u32 fn_800695EC(void* p)
{
    if (fn_800696AC(p) == 0)
        nw4r::db::Panic("g3d_resanmshp_ac.h", 166, "%s::%s: Object not valid.", fn_80069658(), "ref");
    return fn_80069650(p);
}

/* 0x80069650 (0x8): the `ResAnmShp` block word. */
extern "C" inline u32 fn_80069650(void* p)
{
    return *(u32*)p;
}

/* 0x80069658 (0xC): the `ResAnmShp` class name. */
extern "C" inline const char* fn_80069658(void)
{
    return "ResAnmShp";
}

/* The dictionary header word the entry count sits in. */
typedef struct {
    /* +0x00 */ u8 pad_0x00[0x4];
    /* +0x04 */ u32 field_0x04;
} G3dResNameWord; /* size: 0x8 */

/* 0x80069664 (0x48): returns the dictionary's entry count, 0 for an empty handle. */
inline u32 nw4r::g3d::ResDic::GetNumData() const
{
    if (res_dic_is_valid(const_cast<ResDic*>(this)))
        return ((G3dResNameWord*)res_dic_ptr(const_cast<ResDic*>(this)))->field_0x04;
    return 0;
}

/* 0x800696AC (0x14): whether the `ResAnmShp` handle is set. */
extern "C" inline u32 fn_800696AC(void* p)
{
    return *(u32*)p != 0;
}

namespace nw4r {
namespace g3d {

/* 0x800695D4 (0x8): returns the colour block. */
inline const ResVtxClrData* ResVtxClr::ptr() const {
    return mpData;
}

/* 0x800695DC (0x8): returns the normal block. */
inline const ResVtxNrmData* ResVtxNrm::ptr() const {
    return mpData;
}

/* 0x800695E4 (0x8): returns the position block. */
inline const ResVtxPosData* ResVtxPos::ptr() const {
    return mpData;
}

}  // namespace g3d
}  // namespace nw4r


/* 0x8006946C (0x168): whether vertex set `idx` has an animation bound. */
extern "C" bool anm_obj_shp_test_existence(const void* p, s32 idx) /* untyped: opaque handle - the AnmObjShp the shape pass hands over */
{
    const nw4r::g3d::AnmObjShp* pShp = (const nw4r::g3d::AnmObjShp*)p;

    G3D_POINTER_ASSERT("g3d_anmshp.cpp", pShp, 54, "NW4R:Pointer Error\nthis(=%p) is not valid pointer.");
    if (!((int)idx <= pShp->mNumBinding - 1)) {
        nw4r::db::Panic("g3d_anmshp.cpp", 55, "int(vtxId) is out of bounds(%d)\nint(vtxId) <= %d not satisfied.", idx,
                        pShp->mNumBinding - 1);
    }
    return (pShp->mpBinding[idx] & 0xC000) == 0;
}

/* The shape-result reader retail's linker strips: its references make MWCC emit the inline accessors after it, in
 * retail's order (the compiler emits the accessors a function references last-referenced first). */
extern "C" u32 anm_shp_res_reader(const nw4r::g3d::ResVtxPos* pPos, ResHandle* pRes, const nw4r::g3d::ResDic* pDic,
                                   const nw4r::g3d::ResVtxNrm* pNrm, const nw4r::g3d::ResVtxClr* pClr)
{
    nw4r::g3d::ResVtxPos pos(pPos);
    u32 sum = pos.GetID();
    sum += fn_800696AC(pRes);
    sum += pDic->GetNumData();
    sum += fn_800695EC(pRes);
    sum += (u32)pPos->ptr();
    sum += (u32)pNrm->ptr();
    sum += (u32)pClr->ptr();
    return sum;
}

#pragma dont_inline reset

namespace nw4r {
namespace g3d {

/* 0x80069768 (0x30): copies the handle `pRhs` holds. */
inline ResVtxPos::ResVtxPos(const ResVtxPos* pRhs) {
    fn_80069798((u32*)this, (const u32*)pRhs);
}

}  // namespace g3d
}  // namespace nw4r

/* 0x80069798 (0xC): copies the handle word. */
extern "C" inline void fn_80069798(u32* dst, const u32* src)
{
    *dst = *src;
}

/* 0x800697A4 (0x30): returns the AnmObjShp type. */
const nw4r::g3d::G3dObj::TypeObj nw4r::g3d::AnmObjShp::GetTypeObjStatic()
{
    void* local;
    return *reinterpret_cast<const TypeObj*>(fn_8005DC60(&local, (void*)&anm_typename_AnmObjShp));
}
