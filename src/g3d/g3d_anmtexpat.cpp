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
 *   (playbook 48).  The cross-unit declarations are `g3d/g3d_anmtexpat.h`.  anm_tex_pat_res_data is a GUESS (the
 *   stripped reader below); anm_typename_AnmObjTexPat follows the `anm_typename_*` records (map row lbl_8056F5A8).
 * RESIDUALS. apply_tex_pat_anm_result: the palette loop's index takes r26 and the three palette values r27-r29
 *   where retail has r30 and r26-r28 (tried: a shared index, renamed index, a u16 count local); every other
 *   instruction matches.  `.data`, `.sdata` and `.rodata` are byte-identical to retail.
 *   anm_tex_pat_res_data is extra in the object (linker-stripped).
 * SHAPES. `#pragma peephole off` (playbook idea 106), `#pragma pool_data off` and `#pragma dont_inline on` over the
 *   accessors: the `_ac.h` accessors and the handle copies are `inline` (weak copies after their first referrer,
 *   their strings in the inline tail), GetClassName defined after the resolver so "ResAnmTexPat" leads the tail.
 */

#include "types.h"
#include "nw4r/db_assert.h"   /* nw4r::db::Panic, owner nw4r/db_assert.cpp (rule 2) */
#include "g3d/g3d_anmtexpat.h"
#include "g3d/g3d_anmchr.h"   /* type_obj_set_name_anmchr and G3dObj, owned by g3d/g3d_anmchr.cpp (rule 2) */
#include "g3d/g3d_resmat.h"   /* nw4r::g3d::ResTexObj / ResTlutObj (rule 2) */
#include "g3d/g3d_resshp.h"   /* nw4r::g3d::ResTex / ResPltt (rule 2) */
#include "RVLGX/GXTexture.h"  /* GXInitTexObj, GXGetTexObjLODAll, GXInitTlutObj, owner RVLGX/GXTexture.c (rule 2) */
#include "g3d/g3d_pointer_assert.h" /* G3D_POINTER_ASSERT (rule 1) */

#pragma peephole off

#pragma pool_data off

/* The AnmObjTexPat run-time type name GetTypeObjStatic points at. */
extern "C" const nw4r::g3d::TypeNameData<sizeof("AnmObjTexPat")> anm_typename_AnmObjTexPat = {
    sizeof("AnmObjTexPat"), "AnmObjTexPat"};

#pragma dont_inline on

/* The `g3d_resanmtexpat_ac.h` accessors of `ResAnmTexPat` and the handle copies: `inline`, so MWCC emits them as
 * weak copies after the function that references them and their strings after the unit's own. */

/* 0x800699A0 (0x8): the `ResAnmTexPat` block word. */
extern "C" inline u32 fn_800699A0(void* p)
{
    return *(u32*)p;
}

/* 0x800699B4 (0x14): whether the `ResAnmTexPat` handle is set. */
extern "C" inline u32 fn_800699B4(void* p)
{
    return *(u32*)p != 0;
}

extern "C" const char* fn_800699A8(void);

/* 0x8006993C (0x64): resolves the `ResAnmTexPat` handle, panicking on a NULL handle. */
extern "C" inline u32 fn_8006993C(void* p)
{
    if (fn_800699B4(p) == 0)
        nw4r::db::Panic("g3d_resanmtexpat_ac.h", 92, "%s::%s: Object not valid.", fn_800699A8(), "ref");
    return fn_800699A0(p);
}

/* 0x800699A8 (0xC): the `ResAnmTexPat` class name. */
extern "C" inline const char* fn_800699A8(void)
{
    return "ResAnmTexPat";
}

/* 0x80069C08 (0xC): copies the palette handle word. */
extern "C" inline void fn_80069C08(u32* dst, const u32* src)
{
    *dst = *src;
}

/* 0x80069BD8 (0x30): copy-constructs a palette handle and returns the destination. */
extern "C" inline void* res_pltt_copy_ctor(void* self, void* src)
{
    fn_80069C08((u32*)self, (const u32*)src);
    return self;
}

/* 0x80069C44 (0xC): copies the texture handle word. */
extern "C" inline void fn_80069C44(u32* dst, const u32* src)
{
    *dst = *src;
}

/* 0x80069C14 (0x30): copy-constructs a texture handle and returns the destination. */
extern "C" inline void* res_tex_copy_ctor(void* self, void* src)
{
    fn_80069C44((u32*)self, (const u32*)src);
    return self;
}

/* 0x800697D4 (0x168): whether material `idx` has an animation bound. */
bool nw4r::g3d::AnmObjTexPat::TestExistence(u32 idx) const
{
    G3D_POINTER_ASSERT("g3d_anmtexpat.cpp", this, 57, "NW4R:Pointer Error\nthis(=%p) is not valid pointer.");
    if (!((int)idx <= mNumBinding - 1)) {
        nw4r::db::Panic("g3d_anmtexpat.cpp", 58, "int(matId) is out of bounds(%d)\nint(matId) <= %d not satisfied.", idx,
                        mNumBinding - 1);
    }
    return (mpBinding[idx] & 0xC000) == 0;
}

/* The texture-pattern resource reader retail's linker strips: a frameless tail call (no unwind record) whose
 * reference makes MWCC emit the `ResAnmTexPat` accessors right after it, in retail's order. */
extern "C" u32 anm_tex_pat_res_data(ResHandle* pRes)
{
    return fn_8006993C(pRes);
}

/* 0x800699C8 (0x210): re-initialises the material's texture objects from the animated textures and its lookup
 * tables from the animated palettes, keeping each texture object's filter and LOD settings. */
extern "C" void apply_tex_pat_anm_result(struct ResHandle texObj, struct ResHandle tlutObj,
                                         const nw4r::g3d::TexPatAnmResult* pResult)
{
    u32 texFlags = pResult->bTexExist;
    for (u32 i = 0; texFlags != 0; texFlags >>= 1, i++) {
        if (texFlags & 1) {
            ResHandle tex;
            res_tex_copy_ctor(&tex, (void*)&pResult->tex[i]);
            GXTexObj* pObj = reinterpret_cast<nw4r::g3d::ResTexObj*>(&texObj)->GetTexObj((_GXTexMapID)i);
            u32 minFilt;
            u32 magFilt;
            f32 minLod;
            f32 maxLod;
            f32 lodBias;
            u32 aniso;
            void* pImage;
            u32 fmt;
            u32 fmtCI;
            u16 width;
            u16 height;
            u8 biasClamp;
            u8 edgeLod;
            u8 mipmap;
            GXGetTexObjLODAll(pObj, &minFilt, &magFilt, &minLod, &maxLod, &lodBias, &biasClamp, &edgeLod, &aniso);
            u32 wrapS = GXGetTexObjWrapS(pObj);
            u32 wrapT = GXGetTexObjWrapT(pObj);
            if (reinterpret_cast<nw4r::g3d::ResTex*>(&tex)->IsCIFmt()) {
                reinterpret_cast<nw4r::g3d::ResTex*>(&tex)->GetTexObjCIParam(&pImage, &width, &height, &fmtCI, &minLod, &maxLod, &mipmap);
                GXInitTexObjCI(pObj, pImage, width, height, fmtCI, wrapS, wrapT, mipmap, i);
            } else {
                reinterpret_cast<nw4r::g3d::ResTex*>(&tex)->GetTexObjParam(&pImage, &width, &height, &fmt, &minLod, &maxLod, &mipmap);
                GXInitTexObj(pObj, pImage, width, height, fmt, wrapS, wrapT, mipmap);
            }
            GXInitTexObjLOD(pObj, minFilt, magFilt, minLod, maxLod, lodBias, biasClamp, edgeLod, aniso);
        }
    }
    u32 plttFlags = pResult->bPlttExist;
    for (u32 i = 0; plttFlags != 0; plttFlags >>= 1, i++) {
        if (plttFlags & 1) {
            ResHandle pltt;
            res_pltt_copy_ctor(&pltt, (void*)&pResult->pltt[i]);
            void* pData = reinterpret_cast<nw4r::g3d::ResPltt*>(&pltt)->GetPlttData();
            u32 plttFmt = reinterpret_cast<nw4r::g3d::ResPltt*>(&pltt)->GetFmt();
            u32 numEntries = reinterpret_cast<nw4r::g3d::ResPltt*>(&pltt)->GetNumEntries();
            GXInitTlutObj(reinterpret_cast<nw4r::g3d::ResTlutObj*>(&tlutObj)->GetTlut((_GXTlut)i), pData, plttFmt,
                          numEntries);
        }
    }
}

#pragma dont_inline reset

/* 0x80069C50 (0x30): returns the AnmObjTexPat type. */
const nw4r::g3d::G3dObj::TypeObj nw4r::g3d::AnmObjTexPat::GetTypeObjStatic()
{
    void* local;
    return *reinterpret_cast<const TypeObj*>(type_obj_set_name_anmchr(&local, (void*)&anm_typename_AnmObjTexPat));
}
