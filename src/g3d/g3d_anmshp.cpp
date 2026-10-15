/*
 * g3d/g3d_anmshp.cpp - nw4r g3d `g3d_anmshp.cpp`: the shape animation object (`AnmObjShp`), the checked
 *   `ResAnmShp` resolver and the `ResVtxPos`/`ResVtxNrm`/`ResVtxClr`/`ResDic` accessors of the headers it includes.
 * RANGE. .text 0x8006946C-0x800697D4; extab 0x80007BB4-0x80007BEC, extabindex 0x8002004C-0x800200A0, .rodata
 *   0x8056F598-0x8056F5A8, .data 0x8058CC40-0x8058CD40, .sdata 0x80791188-0x80791190.  The TU is `g3d_anmshp.cpp`:
 *   its one-copy `__FILE__` string (0x8058CC40) is cited by fn_8006946C; the previous TU's (`g3d_anmscn.cpp`,
 *   0x8058C288) last by fn_80068FC4, the next TU's (`g3d_anmtexpat.cpp`, 0x8058CD40) first by
 *   AnmObjTexPat::TestExistence (0x800697D4).  The "AnmObjShp" record (0x8056F598) is read by
 *   AnmObjShp::GetTypeObjStatic, the last function of the range.
 * FLAGS. `cflags_g3d` (docs/g3d.md).
 * NAMES. ResVtxPos's ref/ptr/GetID/GetClassName/IsValid/copy constructor, ResVtxNrm::ptr, ResVtxClr::ptr,
 *   ResDic::GetNumData and AnmObjShp::GetTypeObjStatic are nw4r's members; the stems are defined `extern "C"`
 *   (playbook 48).  The cross-unit declarations are `g3d/g3d_anmshp.h`.
 * RESIDUALS. Unwritten (objdiff scores it zero): 0x8006946C-0x800695D4 (fn_8006946C).
 *   flipcheck: `.text`, extab and extabindex are partial; `.rodata`, `.data` and `.sdata` are claimed and not emitted.
 * SHAPES. The unit compiles with `#pragma peephole off` (playbook idea 106) and `#pragma pool_data off`.
 */

#include "types.h"
#include "nw4r/db_assert.h"   /* nw4r::db::Panic, owner nw4r/db_assert.cpp (rule 2) */
#include "g3d/g3d_anmshp.h"
#include "g3d/g3d_anmchr.h"   /* fn_8005DC60, res_dic_is_valid, res_dic_ptr and G3dObj, owned by g3d/g3d_anmchr.cpp (rule 2) */
#include "g3d/g3d_resvtx.h"   /* nw4r::g3d::ResVtxPos / ResVtxNrm / ResVtxClr (rule 1) */

#pragma peephole off

#pragma pool_data off

/* The `.rodata` "AnmObjShp" type-name record and the `.data` class names and assert strings (claimed, not emitted). */
extern u8 lbl_8056F598[];
extern u8 lbl_8058CCC8[]; /* "ResAnmShp"     */
extern u8 lbl_8058CD04[]; /* "ResVtxPos"     */
extern u8 lbl_8058CCF0[];
extern u8 lbl_8058CCD4[];
extern u8 lbl_8058CD30[];
extern u8 lbl_8058CD10[];

extern "C" u32 fn_80069650(void *p)
{
    return *(u32 *)p;
}

extern "C" u32 fn_800696AC(void *p)
{
    return *(u32 *)p != 0;
}

extern "C" void fn_80069798(u32 *dst, const u32 *src)
{
    *dst = *src;
}

extern "C" u32 fn_80069658(void)
{
    return (u32)lbl_8058CCC8;
}

/* 0x800697A4 (0x30): returns the AnmObjShp type. */
const nw4r::g3d::G3dObj::TypeObj nw4r::g3d::AnmObjShp::GetTypeObjStatic()
{
    void *local;
    return *reinterpret_cast<const TypeObj*>(fn_8005DC60(&local, lbl_8056F598));
}

/* The dictionary header word the entry count sits in. */
typedef struct {
    /* +0x00 */ u8 pad_0x00[0x4];
    /* +0x04 */ u32 field_0x04;
} G3dResNameWord; /* size: 0x8 */

/* 0x80069664 (0x48): returns the dictionary's entry count, 0 for an empty handle. */
u32 nw4r::g3d::ResDic::GetNumData() const
{
    if (res_dic_is_valid(const_cast<ResDic*>(this)))
        return ((G3dResNameWord *)res_dic_ptr(const_cast<ResDic*>(this)))->field_0x04;
    return 0;
}

/* 0x800695EC (0x64): resolves the `ResAnmShp` handle, panicking on a NULL handle. */
extern "C" u32 fn_800695EC(void *p)
{
    if (fn_800696AC(p) == 0)
        nw4r::db::Panic((const char *)lbl_8058CCF0, 166, (const char *)lbl_8058CCD4,
                        (const char *)fn_80069658(), "ref");
    return fn_80069650(p);
}

namespace nw4r {
namespace g3d {

/* 0x800695D4 (0x8): returns the colour block. */
const ResVtxClrData* ResVtxClr::ptr() const {
    return mpData;
}

/* 0x800695DC (0x8): returns the normal block. */
const ResVtxNrmData* ResVtxNrm::ptr() const {
    return mpData;
}

/* 0x800695E4 (0x8): returns the position block. */
const ResVtxPosData* ResVtxPos::ptr() const {
    return mpData;
}

/* 0x800696C0 (0x24): returns the array's ID. */
u32 ResVtxPos::GetID() const {
    return ref().id;
}

/* 0x800696E4 (0x64): returns the position block, panicking on a NULL handle. */
const ResVtxPosData& ResVtxPos::ref() const {
    if (!IsValid()) {
        nw4r::db::Panic((const char*)lbl_8058CD30, 39, (const char*)lbl_8058CD10, GetClassName(), "ref");
    }
    return *ptr();
}

/* 0x80069748 (0xC): returns the class name. */
const char* ResVtxPos::GetClassName() {
    return (const char*)lbl_8058CD04;
}

/* 0x80069754 (0x14): tells whether the handle is set. */
bool ResVtxPos::IsValid() const {
    return mpData != NULL;
}

/* 0x80069768 (0x30): copies the handle `pRhs` holds. */
ResVtxPos::ResVtxPos(const ResVtxPos* pRhs) {
    fn_80069798((u32*)this, (const u32*)pRhs);
}

}  // namespace g3d
}  // namespace nw4r
