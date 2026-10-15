/*
 * g3d/g3d_anmtexpat.cpp - nw4r g3d `g3d_anmtexpat.cpp`: the texture-pattern animation object (`AnmObjTexPat`),
 *   the texture-pattern result applier and the checked `ResAnmTexPat` resolver with its word accessors.
 * RANGE. .text 0x800697D4-0x80069C80; extab 0x80007BEC-0x80007C1C, extabindex 0x800200A0-0x800200E8, .rodata
 *   0x8056F5A8-0x8056F5C0, .data 0x8058CD40-0x8058CE10, .sdata 0x80791190-0x80791198.  The TU is
 *   `g3d_anmtexpat.cpp`: its one-copy `__FILE__` string (0x8058CD40) is cited by AnmObjTexPat::TestExistence; the
 *   next TU's (`g3d_anmtexsrt.cpp`, 0x8058CE10) first by AnmObjTexSrt::TestExistence, and the AnmObjTexSrt
 *   constructor (0x80069C80) stores that TU's vtable.  The "AnmObjTexPat" record (0x8056F5A8) is read by
 *   AnmObjTexPat::GetTypeObjStatic, the last function of the range.
 * FLAGS. `cflags_g3d` (docs/g3d.md).
 * NAMES. apply_tex_pat_anm_result and AnmObjTexPat::TestExistence (0x800697D4, the "g3d_anmtexpat.cpp" asserts) are
 *   GUESSES (nw4r's ApplyTexPatAnmResult, by the ScnMdl material pass that hands it the copied handles and the
 *   animation's result).  AnmObjTexPat::GetTypeObjStatic is nw4r's member; the stems are defined `extern "C"`
 *   (playbook 48).  The cross-unit declarations are `g3d/g3d_anmtexpat.h`.
 * RESIDUALS. Unwritten (objdiff scores them zero): 0x800697D4-0x8006993C (AnmObjTexPat::TestExistence),
 *   0x800699C8-0x80069BD8 (apply_tex_pat_anm_result).
 *   flipcheck: `.text`, extab and extabindex are partial; `.rodata`, `.data` and `.sdata` are claimed and not emitted.
 * SHAPES. The unit compiles with `#pragma peephole off` (playbook idea 106) and `#pragma pool_data off`.
 */

#include "types.h"
#include "nw4r/db_assert.h"   /* nw4r::db::Panic, owner nw4r/db_assert.cpp (rule 2) */
#include "g3d/g3d_anmtexpat.h"
#include "g3d/g3d_anmchr.h"   /* type_obj_set_name_anmchr and G3dObj, owned by g3d/g3d_anmchr.cpp (rule 2) */

#pragma peephole off

#pragma pool_data off

/* The `.rodata` "AnmObjTexPat" type-name record and the `.data` class name and assert strings (claimed, not
 * emitted). */
extern u8 lbl_8056F5A8[];
extern u8 lbl_8058CDC8[]; /* "ResAnmTexPat"  */
extern u8 lbl_8058CDF4[];
extern u8 lbl_8058CDD8[];

extern "C" u32 fn_800699A0(void *p)
{
    return *(u32 *)p;
}

extern "C" u32 fn_800699B4(void *p)
{
    return *(u32 *)p != 0;
}

extern "C" void fn_80069C08(u32 *dst, const u32 *src)
{
    *dst = *src;
}

extern "C" void fn_80069C44(u32 *dst, const u32 *src)
{
    *dst = *src;
}

extern "C" u32 fn_800699A8(void)
{
    return (u32)lbl_8058CDC8;
}

/* The store-then-return forwarders: write through the word copier and hand `self` back. */
extern "C" void *fn_80069BD8(void *self, void *src)
{
    fn_80069C08((u32 *)self, (const u32 *)src);
    return self;
}

extern "C" void *fn_80069C14(void *self, void *src)
{
    fn_80069C44((u32 *)self, (const u32 *)src);
    return self;
}

/* 0x80069C50 (0x30): returns the AnmObjTexPat type. */
const nw4r::g3d::G3dObj::TypeObj nw4r::g3d::AnmObjTexPat::GetTypeObjStatic()
{
    void *local;
    return *reinterpret_cast<const TypeObj*>(type_obj_set_name_anmchr(&local, lbl_8056F5A8));
}

/* 0x8006993C (0x64): resolves the `ResAnmTexPat` handle, panicking on a NULL handle. */
extern "C" u32 fn_8006993C(void *p)
{
    if (fn_800699B4(p) == 0)
        nw4r::db::Panic((const char *)lbl_8058CDF4, 92, (const char *)lbl_8058CDD8,
                        (const char *)fn_800699A8(), "ref");
    return fn_800699A0(p);
}
