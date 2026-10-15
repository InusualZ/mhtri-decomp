/*
 * g3d/g3d_anmclr.cpp - nw4r g3d `g3d_anmclr.cpp`: the material-colour animation object (`AnmObjMatClr`), the
 *   colour-result applier, the checked `_ac.h` getters of `ResMat`/`ResMatChan` it includes and the
 *   type-name store copies.
 * RANGE. .text 0x80063E60-0x800648A8; extab 0x8000776C-0x800078EC, extabindex 0x8001FBFC-0x8001FC8C, .rodata
 *   0x8056F550-0x8056F568, .data 0x8058C118-0x8058C248, .sdata 0x80791158-0x80791168.  The TU is `g3d_anmclr.cpp`:
 *   its one-copy `__FILE__` string (0x8058C118) is cited by AnmObjMatClr::TestExistence, and the next TU's
 *   (`g3d_anmobj.cpp`, 0x8058C248) by PlayPolicy_Loop; the extabindex records run 0x80064868 then 0x800648B0.
 *   The left edge is `g3d/g3d_anmchr.cpp`'s end, the right edge `g3d/g3d_anmobj.cpp`.
 * FLAGS. `cflags_g3d` (docs/g3d.md).
 * NAMES. apply_clr_anm_result and AnmObjMatClr::TestExistence (0x80063E60) are GUESSES (nw4r's ApplyClrAnmResult,
 *   by the ScnMdl material pass).  res_mat_copy_ctor and res_mat_common_copy_ctor are GUESSES (the ResMat copy the
 *   ScnMdl buffer refill makes of GetResMat's result, and its one-word base copy).  type_obj_set_name_texsrt_res,
 *   type_obj_set_name_texsrt_node and type_obj_set_name_texsrt_override are GUESSES (the type-name store copies the
 *   texture-SRT GetTypeObj members call; the dump's GXInitTexObjUserData at those addresses is an identical-body
 *   fold, not evidence).  ResMat/ResMatChan's ref, ptr, GetClassName and IsValid are nw4r's members.  The cross-unit
 *   declarations are `g3d/g3d_anmclr.h`.  color_to_u32_ref (0x80064754, the identity the word readers pass a colour
 *   through, nw4r::ut::Color's ToU32ref shape) and color_ctor_white (0x80064834, stores 0xFFFFFFFF through the word
 *   setter, Color's default constructor shape) are GUESSES.
 * RESIDUALS. Unwritten (objdiff scores them zero): 0x80063E60-0x80063FC8 (AnmObjMatClr::TestExistence),
 *   0x80064128-0x800646E8 (apply_clr_anm_result).
 *   flipcheck: `.text`, extab and extabindex are partial; `.rodata`, `.data` and `.sdata` are claimed and not emitted.
 * SHAPES. The unit compiles with `#pragma peephole off` throughout (retail keeps the unfused `clrlwi` + `cmpwi`,
 *   `extsh`, `addi r0` vtable-store and `mr r3` + `lwz r12,0(r3)` virtual-call forms; playbook idea 106).
 */

#include "types.h"
#include "nw4r/g3d/res_common.h" /* ResHandle (rule 1) */
#include "nw4r/db_assert.h"   /* nw4r::db::Panic, owner nw4r/db_assert.cpp (rule 2) */
#include "g3d/g3d_anmclr.h"
#include "g3d/g3d_anmchr.h"   /* type_obj_set_name_anmchr and G3dObj, owned by g3d/g3d_anmchr.cpp (rule 2) */
#include "g3d/g3d_resmat.h"   /* nw4r::g3d::ResMat / ResMatChan (rule 2) */

#pragma peephole off

/* The `.rodata` "AnmObjMatClr" type-name record (a length word followed by the NUL-terminated name). */
extern u8 lbl_8056F550[];

/* The `.data` assert strings and class names of the included `_ac.h` getters (read-only). */
extern u8 lbl_8058C1D0[];
extern u8 lbl_8058C208[];
extern u8 lbl_8058C1A0[];
extern u8 lbl_8058C1C0[];
extern u8 lbl_8058C1DC[];
extern u8 lbl_8058C1F8[];
extern u8 lbl_8058C214[];
extern u8 lbl_8058C230[];

/* 0x800640EC (0x30): copy-constructs a material handle through its base-handle copy and returns the destination. */
extern "C" struct ResHandle *res_mat_copy_ctor(struct ResHandle *pDst, const struct ResHandle *pSrc)
{
    res_mat_common_copy_ctor(pDst, pSrc);
    return pDst;
}

extern "C" u32 fn_80064730(void *self)
{
    return *color_to_u32_ref((u32 *)self);
}

/* The two `fn_8005B1E4` forwarders (0x800647A0 stores a word through it; 0x80064834 sets -1). */
extern "C" void fn_8005B1E4(void *self, u32 value);

extern "C" void fn_800647A0(void *self, u32 *p)
{
    fn_8005B1E4(self, *p);
}

extern "C" u32 *color_ctor_white(u32 *self)
{
    fn_8005B1E4(self, 0xFFFFFFFF);
    return self;
}

extern "C" const u8 **type_obj_set_name_texsrt_res(const u8 **out, const u8 *v)
{
    *out = v;
    return out;
}

extern "C" u32 fn_80064034(u32 *p)
{
    return *p;
}

/* 0x800640E4 (0x8): returns the material block. */
const nw4r::g3d::ResMatData* nw4r::g3d::ResMat::ptr() const
{
    return mpData;
}

/* 0x8006411C (0xC): copies the base handle's data pointer. */
extern "C" void res_mat_common_copy_ctor(struct ResHandle *pDst, const struct ResHandle *pSrc)
{
    pDst->mpData = pSrc->mpData;
}

/* Returns its argument unchanged; the callers read the word through it (0x80064754 is the 4-byte
 * identity the `*(u32*)` readers pass their pointer through). */
extern "C" u32 *color_to_u32_ref(u32 *self)
{
    return self;
}

extern "C" u32 fn_8006480C(u32 *p)
{
    return *p;
}

/* 0x80064868 (0x30): returns the AnmObjMatClr type. */
const nw4r::g3d::G3dObj::TypeObj nw4r::g3d::AnmObjMatClr::GetTypeObjStatic()
{
    void *local;
    return *reinterpret_cast<const TypeObj*>(type_obj_set_name_anmchr(&local, lbl_8056F550));
}

extern "C" const u8 **type_obj_set_name_texsrt_node(const u8 **out, const u8 *v)
{
    *out = v;
    return out;
}

typedef struct {
    /* +0x00 */ u8 pad_0x00[0xC];
    /* +0x0C */ u32 field_0x0C;
} ResAnmChrData; /* size: 0x10 (approximate - only the field this accessor reads is evidenced) */

/* 0x80064080 (0x64): returns the material block, panicking on a NULL handle. */
const nw4r::g3d::ResMatData& nw4r::g3d::ResMat::ref() const
{
    if (!IsValid())
        nw4r::db::Panic((const char *)lbl_8058C1C0, 621, (const char *)lbl_8058C1A0, GetClassName(), "ref");
    return *ptr();
}

extern "C" u32 fn_8006405C(void *p)
{
    return ((const ResAnmChrData *)&reinterpret_cast<const nw4r::g3d::ResMat *>(p)->ref())->field_0x0C;
}

extern "C" u32 fn_8006403C(void)
{
    return (u32)lbl_8058C208;
}

/* 0x80064814 (0xC): returns the class name. */
const char* nw4r::g3d::ResMatChan::GetClassName()
{
    return (const char *)lbl_8058C1D0;
}

extern "C" s32 fn_80064048(u32 *p)
{
    return *p != 0;
}

/* 0x80064820 (0x14): whether the handle is set. */
bool nw4r::g3d::ResMatChan::IsValid() const
{
    return mpData != NULL;
}

extern "C" const u8 **type_obj_set_name_texsrt_override(const u8 **out, const u8 *v)
{
    *out = v;
    return out;
}

/* The checked getters (0x80063FD0/0x800647A8): resolve the object through the validity test, panic through
 * `nw4r::db::Panic` out of the owning `_ac.h` when it is not valid, then return the resolved pointer. */
extern "C" u32 fn_80063FD0(void *p)
{
    if (fn_80064048((u32 *)p) == 0)
        nw4r::db::Panic((const char *)lbl_8058C230, 0x27, (const char *)lbl_8058C214,
                        (const char *)fn_8006403C(), "ref");
    return fn_80064034((u32 *)p);
}

/* 0x800647A8 (0x64): returns the channel block, panicking on a NULL handle. */
nw4r::g3d::ResMatChanData& nw4r::g3d::ResMatChan::ref()
{
    if (IsValid() == 0)
        nw4r::db::Panic((const char *)lbl_8058C1F8, 0x1d1, (const char *)lbl_8058C1DC,
                        GetClassName(), "ref");
    return *(ResMatChanData *)fn_8006480C((u32 *)this);
}

/* The word-or/and forwarders (0x800646E8/0x80064758). */
extern "C" void fn_8005B1B4(u32 *self, u32 value);

extern "C" void fn_800646E8(void *self, void *p, u32 bits)
{
    fn_8005B1B4((u32 *)self, bits | fn_80064730(p));
}

extern "C" void fn_80064758(void *self, void *p, u32 bits)
{
    fn_8005B1B4((u32 *)self, bits & fn_80064730(p));
}
