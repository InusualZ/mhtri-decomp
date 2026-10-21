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
 *   declarations are `g3d/g3d_anmclr.h`.  color_to_u32_ref (0x80064754, nw4r::ut::Color::ToU32ref const) and
 *   color_ctor_white (0x80064834, Color's default constructor) are GUESSES kept as C stems: anmscn's result
 *   constructors call them.  The ut::Color operators at 0x800646E8 (operator|), 0x80064730 (ToU32), 0x80064758
 *   (operator&) and 0x800647A0 (operator=(const GXColor&)) are the compiler's weak copies (map renames requested).
 *   res_mat_get_id (0x8006405C, ResMat::GetID: the block's index word, read by AnmObjTexSrtRes::Bind) is a GUESS.
 * RESIDUALS. Color() and ToU32ref const are emitted twice (the C stems above and the weak copies apply_clr_anm_result
 *   needs) until anmscn's result constructors become implicit constructors (math::VEC3 needs its constructor).
 *   apply_clr_anm_result's calls to Color::operator=(u32), the GXColor copy and the destructor name the weak copies
 *   (`__as__Q34nw4r2ut5ColorFUl`, `__as__8_GXColorFRC8_GXColor`) that the map holds as fn_8005B1E4 and
 *   color_rgba_copy.
 *   flipcheck: `.text` order, extab and extabindex differ; `.rodata`, `.data` and `.sdata` are claimed and not
 *   emitted (the ResAnmClr/ResMatChan/ResMat `_ac.h` getters are C stems reading the strings by map label).
 * SHAPES. `#pragma pool_data off` (each assert string its own `lis`/`addi`).  apply_clr_anm_result and the rest
 *   of the file compile under `#pragma dont_inline on`: retail calls ResMatChan::ref and every ut::Color member out
 *   of line, and the weak copies MWCC emits at the end of the file follow apply_clr_anm_result in retail order.
 *   The unit compiles with `#pragma peephole off` throughout (retail keeps the unfused `clrlwi` + `cmpwi`, `extsh`,
 *   `addi r0` vtable-store and `mr r3` + `lwz r12,0(r3)` virtual-call forms; playbook idea 106).  The pragma sits
 *   above the includes: a weak inline copy compiles under the pragma state of its definition in the header (with
 *   peephole on, operator| and operator& move `this` to r3 before copying `rgba` to r31).
 * SHAPES. apply_clr_anm_result reaches the channel block through the argument itself
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

/* ut::Color::operator=(u32) (0x8005B1E4, the weak copy font/flfnt.cpp's range holds). */
extern "C" void fn_8005B1E4(void *self, u32 value);

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
    const u8 *local;
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

/* 0x8006405C (0x24): the material's index in its model. */
extern "C" u32 res_mat_get_id(const struct ResHandle *p)
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

#pragma dont_inline on

/* 0x80064128 (0x5C0): applies a colour result to a material's channel and tev-colour blocks: each present colour is
 * masked by its write mask and or-ed with the animated value. */
extern "C" void apply_clr_anm_result(struct ResHandle chan, struct ResHandle tevColor,
                                     const nw4r::g3d::ClrAnmResult* pResult)
{
    nw4r::g3d::ResMatTevColor& tev = reinterpret_cast<nw4r::g3d::ResMatTevColor&>(tevColor);
    nw4r::ut::Color color;
    GXColor* pColor;

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

/* dont_inline stays on to the end of the file: the weak ut::Color copies are compiled here. */
