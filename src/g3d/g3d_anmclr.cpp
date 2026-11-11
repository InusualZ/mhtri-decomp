/*
 * g3d/g3d_anmclr.cpp - nw4r g3d `g3d_anmclr.cpp`: the material-colour animation object (`AnmObjMatClr`), the
 *   colour-result applier, the checked `_ac.h` getters of `ResAnmClr`/`ResMat`/`ResMatChan` it includes and the
 *   type-name store copies.
 * RANGE. .text 0x80063E60-0x800648A8; extab 0x8000776C-0x800078EC, extabindex 0x8001FBFC-0x8001FC8C, .rodata
 *   0x8056F550-0x8056F568, .data 0x8058C118-0x8058C248, .sdata 0x80791158-0x80791168.  The TU is `g3d_anmclr.cpp`:
 *   its one-copy `__FILE__` string (0x8058C118) is cited by AnmObjMatClr::TestExistence, and the next TU's
 *   (`g3d_anmobj.cpp`, 0x8058C248) by PlayPolicy_Loop; the extabindex records run 0x80064868 then 0x800648B0.
 *   The left edge is `g3d/g3d_anmchr.cpp`'s end, the right edge `g3d/g3d_anmobj.cpp`.  The link kept only three of
 *   the TU's own functions (TestExistence, apply_clr_anm_result, GetTypeObjStatic): the rest of `.text` is the weak
 *   accessor copies other units call, in the deferred order of two functions the link dead-stripped.
 * FLAGS. `cflags_g3d` (docs/g3d.md).
 * NAMES. apply_clr_anm_result and AnmObjMatClr::TestExistence (0x80063E60) are GUESSES (nw4r's ApplyClrAnmResult,
 *   by the ScnMdl material pass).  res_mat_copy_ctor and res_mat_common_copy_ctor are GUESSES (the ResMat copy the
 *   ScnMdl buffer refill makes of GetResMat's result, and its one-word base copy).  type_obj_set_name_texsrt_res,
 *   type_obj_set_name_texsrt_node and type_obj_set_name_texsrt_override are GUESSES (the type-name store copies the
 *   texture-SRT GetTypeObj members call; the dump's GXInitTexObjUserData at those addresses is an identical-body
 *   fold, not evidence).  ResMat and ResMatChan's ref, ptr, GetClassName and IsValid are nw4r's members.
 *   fn_80063FD0, fn_80064034, fn_8006403C and fn_80064048 are ResAnmClr::ref/ptr/GetClassName/IsValid (the
 *   `g3d_resanmclr_ac.h` asserts name the class), kept under the map stems because `g3d/g3d_resfile.cpp` calls the
 *   resolver by its stem and `g3d/g3d_resanm.cpp` holds the ResAnmClr identifier for a colour record (rename requests
 *   filed).  The cross-unit declarations are `g3d/g3d_anmclr.h`.
 *   res_mat_get_id (0x8006405C, ResMat::GetID: the block's index word, read by AnmObjTexSrtRes::Bind) is a GUESS.
 *   The ut::Color members at 0x800646E8 (operator|), 0x80064730 (ToU32), 0x80064754 (ToU32ref const), 0x80064758
 *   (operator&) and 0x800647A0 (operator=(const GXColor&)) are the compiler's weak copies.  The Color
 *   default constructor (0x80064834) is the weak copy apply_clr_anm_result's `ut::Color` local emits.
 *   anm_typename_AnmObjMatClr is the `.rodata` record by its string.  The six placing stubs (place_*) stand for
 *   the dead-stripped functions whose references ordered the copies.
 * RESIDUALS. The six placing stubs are 24 B of `.text` the link dead-strips (a tail call carries no unwind entry, so
 *   the extab names stay aligned).
 *   The `.sdata` fragment is 0xC B of statics where the claim is 0x10 B: the last word is the link's 8-byte
 *   alignment pad before `g3d/g3d_anmobj.cpp`'s fragment; `.rodata` 0x11 and `.data` 0x12B B pad the same way.
 * SHAPES. `#pragma pool_data off` (each assert string its own `lis`/`addi`).  Everything after TestExistence compiles
 *   under `#pragma dont_inline on`: retail calls every accessor and every ut::Color member out of line.  The `_ac.h`
 *   accessors are `__declspec(noinline) inline` weak copies: MWCC emits a copy after the first function that
 *   references it (several in one function: reverse reference order, each followed by its own callees), creates
 *   its string literals at its definition (weak `@STRING@` copies in reverse definition order after the TU's own
 *   strings, `static char` member names forward in `.sdata`), so the definitions stand in the `.data` tail's order
 *   and a one-call stub before each retail run places the code (the TexSrtRes store, then ResAnmClr::ref, then
 *   res_mat_get_id, then res_mat_copy_ctor after TestExistence; the TexSrtNode and TexSrtOverride stores after
 *   GetTypeObjStatic).  The unit compiles with `#pragma peephole off` throughout (retail keeps the unfused `clrlwi` +
 *   `cmpwi`, `extsh`, `addi r0` vtable-store and `mr r3` + `lwz r12,0(r3)` virtual-call forms; playbook idea 106).
 *   The pragma sits above the includes: a weak inline copy compiles under the pragma state of its definition in the
 *   header (with peephole on, operator| and operator& move `this` to r3 before copying `rgba` to r31).
 *   apply_clr_anm_result reaches the channel block through the argument itself
 *   (`reinterpret_cast<ResMatChan&>(chan).ref()` at each use) and one GXColor pointer local: a ResMatChan reference
 *   local moves the argument copy after the other two (`mr r28,r3` last) and a GXColor reference per block swaps
 *   r28/r29.
 */

#pragma peephole off

#include "types.h"
#include "nw4r/g3d/res_common.h" /* ResHandle (rule 1) */
#include "nw4r/db_assert.h"   /* nw4r::db::Panic, owner nw4r/db_assert.cpp (rule 2) */
#include "g3d/g3d_anmclr.h"
#include "g3d/g3d_anmchr.h"   /* type_obj_set_name_anmchr and G3dObj, owned by g3d/g3d_anmchr.cpp (rule 2) */
#include "g3d/g3d_resmat.h"   /* nw4r::g3d::ResMat / ResMatChan (rule 2) */
#include "g3d/g3d_pointer_assert.h" /* G3D_POINTER_ASSERT (rule 1) */
#include "g3d/g3d_resfile.h"   /* res_mat_tev_color_dc_store (rule 2) */
#include "nw4r/ut_Color.h"       /* nw4r::ut::Color */

#pragma pool_data off

/* 0x80063E60 (0x168): whether material `idx` has an animation bound. */
bool nw4r::g3d::AnmObjMatClr::TestExistence(u32 idx) const
{
    G3D_POINTER_ASSERT("g3d_anmclr.cpp", this, 52, "NW4R:Pointer Error\nthis(=%p) is not valid pointer.");
    if (!((int)idx <= mNumBinding - 1)) {
        nw4r::db::Panic("g3d_anmclr.cpp", 53, "int(matId) is out of bounds(%d)\nint(matId) <= %d not satisfied.", idx,
                        mNumBinding - 1);
    }
    return (mpBinding[idx] & 0xC000) == 0;
}

/* 0x8056F550 - the "AnmObjMatClr" type-name record: the name's size with its NUL, then the name. */
extern "C" const u8 anm_typename_AnmObjMatClr[] = {0x00, 0x00, 0x00, sizeof("AnmObjMatClr"),
                                                   'A', 'n', 'm', 'O', 'b', 'j', 'M', 'a', 't', 'C', 'l', 'r', 0x00};

#pragma dont_inline on

/* The ResAnmClr accessors stay under their map stems: `g3d/g3d_resfile.cpp` calls the resolver by its stem, and the
 * class name is taken by `g3d/g3d_resanm.cpp`'s colour record (requests dat-anmclr#3 and #4). */
extern "C" {
__declspec(noinline) inline u32 fn_80064034(struct ResHandle *p);
__declspec(noinline) inline const char *fn_8006403C(void);
__declspec(noinline) inline s32 fn_80064048(const struct ResHandle *p);
}

/* -------------------------------------------------------------------------------------------------- */
/* The `_ac.h` accessor instances the target keeps as weak out-of-line copies, defined in the order     */
/* the `.data` tail needs (its strings come out in reverse definition order).                          */
/* -------------------------------------------------------------------------------------------------- */

/* `ResAnmClr::ref` with the `g3d_resanmclr_ac.h` assert: the checked resolver `g3d/g3d_resfile.cpp`'s revision
 * check reads. */
/* untyped: opaque handle passed through - the ResAnmClr handle word its C caller hands in */
extern "C" __declspec(noinline) inline u32 fn_80063FD0(void *p)
{
    static char sMember[] = "ref";
    struct ResHandle *pSelf = reinterpret_cast<struct ResHandle *>(p);

    if (fn_80064048(pSelf) == 0) {
        nw4r::db::Panic("g3d_resanmclr_ac.h", 0x27, "%s::%s: Object not valid.", fn_8006403C(), sMember);
    }
    return fn_80064034(pSelf);
}

/* `ResAnmClr::GetClassName`: the class-name string the `%s::%s: Object not valid.` assert passes. */
extern "C" __declspec(noinline) inline const char *fn_8006403C(void)
{
    return "ResAnmClr";
}

/* 0x800647A8 (0x64): returns the channel block, panicking on a NULL handle. */
inline nw4r::g3d::ResMatChanData& nw4r::g3d::ResMatChan::ref()
{
    static char sMember[] = "ref";

    if (!IsValid()) {
        nw4r::db::Panic("g3d_resmat_ac.h", 0x1D1, "%s::%s: Object not valid.", GetClassName(), sMember);
    }
    return *ptr();
}

/* 0x80064814 (0xC): returns the class name. */
inline const char* nw4r::g3d::ResMatChan::GetClassName()
{
    return "ResMatChan";
}

/* 0x80064080 (0x64): returns the material block, panicking on a NULL handle. */
inline const nw4r::g3d::ResMatData& nw4r::g3d::ResMat::ref() const
{
    static char sMember[] = "ref";

    if (!IsValid()) {
        nw4r::db::Panic("g3d_resmat_ac.h", 621, "%s::%s: Object not valid.", GetClassName(), sMember);
    }
    return *ptr();
}

/* The string-free instances: each follows the copy that first references it. */

/* `ResAnmClr::ptr`: returns the block. */
extern "C" __declspec(noinline) inline u32 fn_80064034(struct ResHandle *p)
{
    return (u32)p->mpData;
}

/* `ResAnmClr::IsValid`: whether the handle is set. */
extern "C" __declspec(noinline) inline s32 fn_80064048(const struct ResHandle *p)
{
    return p->mpData != NULL;
}

/* 0x8006480C (0x8): returns the channel block. */
inline nw4r::g3d::ResMatChanData* nw4r::g3d::ResMatChan::ptr()
{
    return mpData;
}

/* 0x80064820 (0x14): whether the handle is set. */
inline bool nw4r::g3d::ResMatChan::IsValid() const
{
    return mpData != NULL;
}

/* 0x800640E4 (0x8): returns the material block. */
inline const nw4r::g3d::ResMatData* nw4r::g3d::ResMat::ptr() const
{
    return mpData;
}

/* 0x8006405C (0x24): the material's index in its model. */
extern "C" __declspec(noinline) inline u32 res_mat_get_id(const struct ResHandle *p)
{
    return reinterpret_cast<const nw4r::g3d::ResMat *>(p)->ref().id;
}

/* 0x8006411C (0xC): copies the base handle's data pointer. */
extern "C" __declspec(noinline) inline void res_mat_common_copy_ctor(struct ResHandle *pDst,
                                                                     const struct ResHandle *pSrc)
{
    pDst->mpData = pSrc->mpData;
}

/* 0x800640EC (0x30): copy-constructs a material handle through its base-handle copy and returns the destination. */
extern "C" __declspec(noinline) inline struct ResHandle *res_mat_copy_ctor(struct ResHandle *pDst,
                                                                            const struct ResHandle *pSrc)
{
    res_mat_common_copy_ctor(pDst, pSrc);
    return pDst;
}

/* The three type-name store copies the texture-SRT GetTypeObj members call: each stores `v` through `out`. */
extern "C" __declspec(noinline) inline const u8 **type_obj_set_name_texsrt_res(const u8 **out, const u8 *v)
{
    *out = v;
    return out;
}

extern "C" __declspec(noinline) inline const u8 **type_obj_set_name_texsrt_node(const u8 **out, const u8 *v)
{
    *out = v;
    return out;
}

extern "C" __declspec(noinline) inline const u8 **type_obj_set_name_texsrt_override(const u8 **out, const u8 *v)
{
    *out = v;
    return out;
}

/* -------------------------------------------------------------------------------------------------- */

/* The placing stubs: one tail call each (no frame, so no unwind entry), dead-stripped by the link.  Each places the
 * weak copy it calls, and the copies that one calls, right after itself. */

/* Places the TexSrtRes type-name store. */
extern "C" const u8 **place_tex_srt_res_name(const u8 **out, const u8 *v)
{
    return type_obj_set_name_texsrt_res(out, v);
}

/* Places the ResAnmClr resolver and its ptr, class-name and validity copies. */
/* untyped: opaque handle passed through - the ResAnmClr handle word its C caller hands in */
extern "C" u32 place_res_anm_clr_ref(void *pAnm)
{
    return fn_80063FD0(pAnm);
}

/* Places the material index getter and the ResMat ref/ptr copies it calls. */
extern "C" u32 place_res_mat_get_id(const struct ResHandle *pMat)
{
    return res_mat_get_id(pMat);
}

/* Places the material handle copy and its base-handle copy. */
extern "C" struct ResHandle *place_res_mat_copy(struct ResHandle *pDst, const struct ResHandle *pSrc)
{
    return res_mat_copy_ctor(pDst, pSrc);
}

/* 0x80064128 (0x5C0): applies a colour result to a material's channel and tev-colour blocks: each present colour is
 * masked by its write mask and or-ed with the animated value. */
extern "C" void apply_clr_anm_result(struct ResHandle chan, struct ResHandle tevColor,
                                     const nw4r::g3d::ClrAnmResult* pResult)
{
    nw4r::g3d::ResMatTevColor& tev = reinterpret_cast<nw4r::g3d::ResMatTevColor&>(tevColor);
    nw4r::ut::Color color;
    GXColor* pColor;
/* The working colour: the storage read as a ut::Color at each use (a reference would hold its address in a
 * register; retail re-forms `addi r3, r1, 0x7C` at every use). */

    if (pResult->bRgbaExist & 0xF) {
        if (pResult->bRgbaExist & 0x1) {
            pColor = &reinterpret_cast<nw4r::g3d::ResMatChan&>(chan).ref().chan[0].matColor;
            color = *pColor;
            *pColor = (color & pResult->rgbaMask[0]) | pResult->rgba[0];
        }
        if (pResult->bRgbaExist & 0x4) {
            pColor = &reinterpret_cast<nw4r::g3d::ResMatChan&>(chan).ref().chan[0].ambColor;
            color = *pColor;
            *pColor = (color & pResult->rgbaMask[2]) | pResult->rgba[2];
        }
        if (pResult->bRgbaExist & 0x2) {
            pColor = &reinterpret_cast<nw4r::g3d::ResMatChan&>(chan).ref().chan[1].matColor;
            color = *pColor;
            *pColor = (color & pResult->rgbaMask[1]) | pResult->rgba[1];
        }
        if (pResult->bRgbaExist & 0x8) {
            pColor = &reinterpret_cast<nw4r::g3d::ResMatChan&>(chan).ref().chan[1].ambColor;
            color = *pColor;
            *pColor = (color & pResult->rgbaMask[3]) | pResult->rgba[3];
        }
    }

    if (pResult->bRgbaExist & 0x7F0) {
        if (pResult->bRgbaExist & 0x10) {
            if (!tev.GXGetTevColor(GX_TEVREG0, &color))
                color = 0xFF;
            tev.GXSetTevColor(GX_TEVREG0, (color & pResult->rgbaMask[4]) | pResult->rgba[4]);
        }
        if (pResult->bRgbaExist & 0x20) {
            if (!tev.GXGetTevColor(GX_TEVREG1, &color))
                color = 0xFF;
            tev.GXSetTevColor(GX_TEVREG1, (color & pResult->rgbaMask[5]) | pResult->rgba[5]);
        }
        if (pResult->bRgbaExist & 0x40) {
            if (!tev.GXGetTevColor(GX_TEVREG2, &color))
                color = 0xFF;
            tev.GXSetTevColor(GX_TEVREG2, (color & pResult->rgbaMask[6]) | pResult->rgba[6]);
        }
        if (pResult->bRgbaExist & 0x80) {
            if (!tev.GXGetTevKColor(GX_KCOLOR0, &color))
                color = 0xFF;
            tev.GXSetTevKColor(GX_KCOLOR0, (color & pResult->rgbaMask[7]) | pResult->rgba[7]);
        }
        if (pResult->bRgbaExist & 0x100) {
            if (!tev.GXGetTevKColor(GX_KCOLOR1, &color))
                color = 0xFF;
            tev.GXSetTevKColor(GX_KCOLOR1, (color & pResult->rgbaMask[8]) | pResult->rgba[8]);
        }
        if (pResult->bRgbaExist & 0x200) {
            if (!tev.GXGetTevKColor(GX_KCOLOR2, &color))
                color = 0xFF;
            tev.GXSetTevKColor(GX_KCOLOR2, (color & pResult->rgbaMask[9]) | pResult->rgba[9]);
        }
        if (pResult->bRgbaExist & 0x400) {
            if (!tev.GXGetTevKColor(GX_KCOLOR3, &color))
                color = 0xFF;
            tev.GXSetTevKColor(GX_KCOLOR3, (color & pResult->rgbaMask[10]) | pResult->rgba[10]);
        }
        res_mat_tev_color_dc_store(&tevColor, 0);
    }
}

/* 0x80064868 (0x30): returns the AnmObjMatClr type. */
const nw4r::g3d::G3dObj::TypeObj nw4r::g3d::AnmObjMatClr::GetTypeObjStatic()
{
    const u8 *local;
    return *reinterpret_cast<const TypeObj*>(type_obj_set_name_anmchr(&local, anm_typename_AnmObjMatClr));
}

/* Places the TexSrtNode type-name store. */
extern "C" const u8 **place_tex_srt_node_name(const u8 **out, const u8 *v)
{
    return type_obj_set_name_texsrt_node(out, v);
}

/* Places the TexSrtOverride type-name store. */
extern "C" const u8 **place_tex_srt_override_name(const u8 **out, const u8 *v)
{
    return type_obj_set_name_texsrt_override(out, v);
}
