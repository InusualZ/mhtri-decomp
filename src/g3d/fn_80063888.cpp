/*
 * g3d/fn_80063888.cpp - nw4r g3d animation-object cluster: the type-name records (`.rodata`: `AnmObj`,
 *   `AnmObjChr`, `AnmObjChrNode`, `AnmObjChrBlend`, `AnmScn`, ...) and their list-insert steps,
 *   the checked `_ac.h` getters, `nw4r::g3d::PlayPolicy_Onetime`/`PlayPolicy_Loop` and the anim-object bodies
 *   of `g3d_anmobj.cpp`, `g3d_anmclr.cpp` and `g3d_anmscn.cpp`.
 * RANGE. .text 0x80063E60-0x800680A8 (139 functions); extab, extabindex, .rodata 0x8056F550-0x8056F578, .data
 *   0x8058C118-0x8058CC40, .sdata 0x80791158-0x80791178, .sdata2 0x80795D68-0x80795D90.  More than one TU:
 *   TestExistence__Q34nw4r3g3d12AnmObjMatClrCFUl cites "g3d_anmclr.cpp" (0x8058C118) and fn_800649CC onward "g3d_anmscn.cpp" (0x8058C288), a seam
 *   near 0x800649CC.  The left edge is `g3d/g3d_anmchr.cpp`'s end (the weak type-info members of its five vtables,
 *   0x80063888-0x80063E60, belong to that unit), the right edge `g3d/g3d_anmscn.cpp`.
 * NAMES. apply_clr_anm_result and AnmObjMatClr::TestExistence (0x80063E60) are GUESSES (nw4r's ApplyClrAnmResult,
 *   by the ScnMdl material pass).  res_mat_copy_ctor and res_mat_common_copy_ctor are GUESSES (the ResMat copy the ScnMdl buffer refill
 *   makes of GetResMat's result, and its one-word base copy).  The file keeps the map's stem (no one `__FILE__` names the run); the stems are defined `extern "C"`
 *   (playbook 48), `fn_80066C8C` keeps C++ linkage (map row `fn_80066C8C__FPv`), and the `PlayPolicy` pair sits in
 *   `nw4r::g3d`, as do ResMatChan's GetClassName and IsValid.  The cross-unit declarations are `g3d/fn_80063888.h`.
 *   GUESS: `vec3_copy_construct` (0x80067E54): copies three floats into `out` and returns it (a VEC3 copy).
 *   type_obj_set_name_texsrt_res is a GUESS, type_obj_set_name_texsrt_node is a GUESS,
 *   type_obj_set_name_texsrt_override is a GUESS (the type-name store copies the texture-SRT GetTypeObj members call;
 *   the dump's GXInitTexObjUserData at those addresses is an identical-body fold, not evidence).
 *   AnmScn's constructor and AnmObj::SetAnmFlag are nw4r's members.
 *   math_fmod is a GUESS; f32_select_nonneg is a GUESS; obj_delete_flagged is a GUESS; fog_set_color is a GUESS;
 *   res_anm_light_config_test_flag8 is a GUESS (the bodies' behaviour); anmobj_cpp_file and anmobj_start_lt_end_msg name the strings.
 * RESIDUALS. `fn_80066FA0` lacks retail's call to `fn_80066DB4` (+0x9C).  PlayPolicy_Loop is 96 % (the `fmr` after the first
 *   fmodf branch: retail keeps the result in the frame's register).  AnmScn's destructor (dtor_80065768) stays
 *   undefined: defining `AnmScn::~AnmScn` makes the explicit call from fn_80066080 pass -1 where retail passes 0.
 *   21 functions unwritten (objdiff scores
 *   them zero) in 17 runs: 0x80063E60-0x80063FC8,
 *   0x80064080-0x800640E4, 0x80064128-0x800646E8, 0x800648B0-0x8006497C (`PlayPolicy_Loop`), 0x800649CC-0x80064BD4,
 *   0x80064C24-0x80064CE0, 0x80064CF0-0x8006518C, 0x80065284-0x8006553C, 0x8006560C-0x800657C4,
 *   0x800659D8-0x80065ED8, 0x80065EF0-0x80065FFC, 0x800660DC-0x80066C68, 0x80066EF8-0x80066F2C,
 *   0x800670D4-0x800677D8, 0x80067824-0x80067A54, 0x80067B90-0x80067E54, 0x80067EFC-0x800680A8.
 *   Partial (12): fn_800646E8, fn_80064758, fn_8006497C, fn_80064988, fn_8006518C, fn_8006522C, fn_8006553C,
 *   fn_800655A4, fn_80065ED8, fn_80066080, fn_80066FA0.  AnmScn's constructor has an empty body by design (the
 *   compiler emits the G3dObj base call and the vtable store).
 *   flipcheck: `.text` 0xEF4 of 0x4248; extab 0x170 of 0x3A8; extabindex 0x228 of 0x360; `.rodata`, `.data` and `.sdata2` are claimed and not emitted; `.sdata` is
 *   0x4 of 0x20.
 * SHAPES. The unit compiles with `#pragma peephole off` throughout (retail keeps the unfused `clrlwi` + `cmpwi`, `extsh`,
 *   `addi r0` vtable-store and `mr r3` + `lwz r12,0(r3)` virtual-call forms; playbook idea 106).
 */

#include "types.h"
#include "nw4r/g3d/res_common.h" /* ResHandle (rule 1) */
#include "unsplit/g3d.h"      /* the math types it reads through the band header */
#include "g3d/fn_80075DCC.h" /* fn_8007A5E4/fn_8007A5A8/fn_8007A724, owned by g3d/fn_80075DCC.cpp (rule 2) */
#include "g3d/fn_80063888.h"
#include "g3d/g3d_anmchr.h"   /* the fn_8005Dxx helpers, the type-info members and G3dObj, owned by g3d/g3d_anmchr.cpp (rule 2) */
#include "g3d/g3d_resmat.h"   /* nw4r::g3d::ResMat / ResMatChan (rule 2) */
#include "fn_80047398.h" /* color_rgba_copy (rule 2) */
#include "mh3_pad.h"        /* VEC3_ctor, owned by mh3_pad.cpp (rule 2) */

#pragma peephole off

/* Forward declarations: this unit's own copy helpers. */
extern "C" void *fn_80064754(void *self);
extern "C" void fn_8006602C(u32 *dst, const u32 *src);
extern "C" void fn_80066D48(u32 *dst, const u32 *src);
extern "C" void fn_80066F94(u32 *dst, const u32 *src);
extern "C" void fn_800670C8(u32 *dst, const u32 *src);

/* A three-float vector record (the cluster's `.text` copies it field by field, so the struct is the
 * explicit-float-copy shape, not a word copy). */
typedef nw4r::math::VEC3 Vec3f; /* size: 0xC */

/* The cluster's `.rodata` animation type-name records (0x8056F500-0x8056F678): a length word followed by
 * the NUL-terminated name.  Referenced by address only. */
extern u8 lbl_8056F550[];
extern u8 lbl_8056F578[];
extern u8 lbl_8056F588[];

/* The type-name strings the plain getters below return by address (`.data`/`.rodata`, read-only). */
extern u8 lbl_8058C1D0[];
extern u8 lbl_8058C208[];
extern u8 lbl_8058C928[];
extern u8 lbl_8058CA70[];
extern u8 lbl_8058CB38[];
extern u8 lbl_8058CC04[];
extern u8 lbl_8058C1A0[];
extern u8 lbl_8058C1C0[];
extern u8 lbl_8058C1DC[];
extern u8 lbl_8058C1F8[];
extern u8 lbl_8058C214[];
extern u8 lbl_8058C230[];
extern u8 lbl_8058CA18[];
extern u8 lbl_8058CA58[];
extern u8 lbl_8058CC10[];
extern u8 lbl_8058CC2C[];
extern u8 lbl_8058C934[];
extern u8 lbl_8058C950[];
extern u8 lbl_8058C968[];
extern u8 lbl_8058C9AC[];
extern u8 lbl_8058C9C4[];
extern u8 lbl_8058CA00[];
extern u8 lbl_8058CA80[];
extern u8 lbl_8058CAA0[];
extern u8 lbl_8058CB44[];
extern u8 lbl_8058CB60[];
extern u8 lbl_8058CB78[];
extern u8 lbl_8058CB98[];
extern u8 lbl_8058CBA8[];
extern u8 lbl_8058CBC8[];
extern u8 lbl_8058CBD8[];
extern u8 lbl_8058CBF8[];
extern const char anmobj_cpp_file[];          /* "g3d_anmobj.cpp" */
extern const char anmobj_start_lt_end_msg[];  /* "NW4R:Failed assertion startFrame < endFrame" */

/* --------------------------------------------------------------------------------------------- *
 * More vtable-dispatch wrappers and the ownership-copy helpers (0x800640EC-0x800657C4).
 * --------------------------------------------------------------------------------------------- */

extern "C" u32 fn_8006553C(void *p)
{
    G3dObj *obj = (G3dObj *)p;
    u32 tmp = obj->vt->method_0x14(p);
    return (u32)reinterpret_cast<const nw4r::g3d::G3dObj::TypeObj*>(&tmp)->GetTypeName();
}

extern "C" u32 fn_800655A4(void *p)
{
    G3dObj *obj = (G3dObj *)p;
    u32 tmp = obj->vt->method_0x14(p);
    return (u32)reinterpret_cast<const nw4r::g3d::G3dObj::TypeObj*>(&tmp)->GetTypeName();
}

/* 0x800640EC (0x30): copy-constructs a material handle through its base-handle copy and returns the destination. */
extern "C" struct ResHandle *res_mat_copy_ctor(struct ResHandle *pDst, const struct ResHandle *pSrc)
{
    res_mat_common_copy_ctor(pDst, pSrc);
    return pDst;
}

extern "C" u32 fn_80064730(void *self)
{
    return *(u32 *)fn_80064754(self);
}

/* The two `fn_8005B1E4` forwarders (0x800647A0 stores a word through it; 0x80064834 sets -1). */
extern "C" void fn_8005B1E4(void *self, u32 value);

extern "C" void fn_800647A0(void *self, u32 *p)
{
    fn_8005B1E4(self, *p);
}

extern "C" void *fn_80064834(void *self)
{
    fn_8005B1E4(self, 0xFFFFFFFF);
    return self;
}

/* Installs the vtable at +0x00 after running the base constructor. */
typedef struct {
    /* +0x00 */ void *vt;
} G3dVtObj; /* size: 0x4 */

/* 0x800657C4 (0x40): constructs the scene animation with no parent. */
nw4r::g3d::AnmScn::AnmScn(MEMAllocator* pHeap) : G3dObj(pHeap, NULL)
{
}

/* 0x80065284 (0x44): the deleting destructor of a trivially destructible object. */
/* untyped: opaque handle - the object to free */
extern "C" void* obj_delete_flagged(void* pSelf, s16 flag)
{
    if (pSelf && flag > 0) {
        operator delete(pSelf);
    }
    return pSelf;
}

/* --------------------------------------------------------------------------------------------- *
 * The small word accessors (0x80063FC8-0x80064898).
 * --------------------------------------------------------------------------------------------- */

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
 * identity the two `*(u32*)` readers below pass their pointer through). */
extern "C" void *fn_80064754(void *self)
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

/* --------------------------------------------------------------------------------------------- *
 * The float and flag helpers (0x8006497C-0x80065ED8).
 * --------------------------------------------------------------------------------------------- */

/* 0x8006497C (0x8): `looped` when `frame` is not negative, else `oneTime`. */
extern "C" asm f32 f32_select_nonneg(register f32 frame, register f32 looped, register f32 oneTime)
{
    nofralloc
    fsel frame, frame, looped, oneTime
    blr
}

extern "C" void fn_80064BD4(u32 *p)
{
    *p |= 4;
}

/* Reads the +0x0 word through the identity helper (0x80064754), the shape the two readers share. */
extern "C" u32 fn_800651BC(void *self)
{
    return *(u32 *)fn_80064754(self);
}

extern "C" u16 fn_800651E0(u16 *p)
{
    return *p;
}

extern "C" u32 fn_80065574(void)
{
    const u8 *local;
    return (u32)*type_obj_set_name(&local, lbl_8056F578);
}

extern "C" u32 fn_800655DC(void)
{
    void *local;
    return (u32)*fn_8005DC60(&local, lbl_8056F588);
}

/* The placement `operator delete`: nothing to release. */
/* untyped: opaque handle passed through - the block and the placement address */
void operator delete(void* block, void* place)
{
}

/* --------------------------------------------------------------------------------------------- *
 * The checked `ResAnmScn` config accessors (0x80065828-0x8006605C): each resolves the object through
 * `fn_8006584C` (the checked getter that reads *self and panics out of `g3d_resanmscn_ac.h`) and reads
 * one field of the resolved config record.
 * --------------------------------------------------------------------------------------------- */

/* `ResAnmScnConfig` moved to g3d/fn_80063888.h (rule 1) when g3d/g3d_resanmlight.cpp became
 * the second consumer; `fn_8006584C`'s declaration moved with it (rule 2). */

extern "C" u16 fn_80065828(void *p)
{
    return fn_8006584C(p)->field_0x36;
}

extern "C" u16 fn_800658F0(void *p)
{
    return fn_8006584C(p)->field_0x3C;
}

extern "C" u16 fn_8006591C(void *p)
{
    return fn_8006584C(p)->field_0x3E;
}

extern "C" u16 fn_80065948(void *p)
{
    return fn_8006584C(p)->field_0x40;
}

extern "C" u16 fn_80065974(void *p)
{
    return fn_8006584C(p)->field_0x42;
}

extern "C" u16 fn_800659A0(void *p)
{
    return fn_8006584C(p)->field_0x44;
}

extern "C" u32 fn_80066038(void *p)
{
    return fn_8006584C(p)->field_0x38;
}

extern "C" u16 fn_8006605C(void *p)
{
    return fn_8006584C(p)->field_0x34;
}

/* The +0x20 `this`-adjustor thunks (0x80065914/40/6C/98): each hands its sub-record to the matching
 * reader.  The sub-record sits 0x20 B into the outer object (the adjustor's own `addi r3, r3, 0x20`). */
typedef struct {
    /* +0x00 */ u8 pad_0x00[0x20];
    /* +0x20 */ ResAnmScnConfig sub_0x20;
} ResAnmScnOwner; /* size: 0x66 (approximate - the +0x20 sub-record offset is the only evidence) */

extern "C" u16 fn_80065914(void *p)
{
    return fn_8006591C(&((ResAnmScnOwner *)p)->sub_0x20);
}

extern "C" u16 fn_80065940(void *p)
{
    return fn_80065948(&((ResAnmScnOwner *)p)->sub_0x20);
}

extern "C" u16 fn_8006596C(void *p)
{
    return fn_80065974(&((ResAnmScnOwner *)p)->sub_0x20);
}

extern "C" u16 fn_80065998(void *p)
{
    return fn_800659A0(&((ResAnmScnOwner *)p)->sub_0x20);
}

/* --------------------------------------------------------------------------------------------- *
 * The word-copy wrappers (0x80065FFC-0x80067098).
 * --------------------------------------------------------------------------------------------- */

extern "C" void *fn_80065FFC(void *dst, void *src)
{
    fn_8006602C((u32 *)dst, (const u32 *)src);
    return dst;
}

extern "C" void *fn_80066D18(void *dst, void *src)
{
    fn_80066D48((u32 *)dst, (const u32 *)src);
    return dst;
}

extern "C" void *fn_80066F64(void *dst, void *src)
{
    fn_80066F94((u32 *)dst, (const u32 *)src);
    return dst;
}

extern "C" void *fn_80067098(void *dst, void *src)
{
    fn_800670C8((u32 *)dst, (const u32 *)src);
    return dst;
}

/* --------------------------------------------------------------------------------------------- *
 * The resolved-resource accessors (0x80066C68-0x80067060): each resolves an object through a checked
 * getter and reads one field of it.
 * --------------------------------------------------------------------------------------------- */
typedef struct {
    /* +0x00 */ u8 pad_0x00[0x10];
    /* +0x10 */ u32 field_0x10;
} ResAnmAmbLightWord; /* size: 0x14 (approximate - only the +0x10 word this accessor reads is evidenced) */

/* `ResAnmLightConfig` and the `fn_80066DB4`/`fn_80066E80` declarations moved to
 * g3d/fn_80063888.h (rules 1 and 2) when g3d/g3d_resanmlight.cpp became the second consumer. */

extern "C" u32 fn_80066C68(void *p)
{
    return ((ResAnmAmbLightWord *)fn_80066C8C(p))->field_0x10;
}

extern "C" s32 fn_80066E2C(void *p)
{
    return fn_80066DB4(p)->field_0x14 != 0;
}

extern "C" u32 fn_80066E5C(void *p)
{
    return fn_80066E80(p)->field_0x14;
}

/* 0x80066EF8 (0x34): whether bit 3 of the light config's channel flags is set. */
/* untyped: opaque handle - the light-config handle */
extern "C" u32 res_anm_light_config_test_flag8(void *p)
{
    return (fn_80066E80(p)->flags & 8) != 0;
}

extern "C" u32 fn_80066F2C(void *p)
{
    return fn_80066E80(p)->field_0x10;
}

extern "C" u8 fn_80067060(void *p)
{
    return fn_80066DB4(p)->field_0x1A;
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

/* --------------------------------------------------------------------------------------------- *
 * The composite teardown destructors (0x80067B5C/0x80067E70/0x80067EB4): construct-or-clear each
 * sub-record of the object through `VEC3_ctor`/`fn_80064834`, then hand the object back.
 * --------------------------------------------------------------------------------------------- */
typedef struct {
    /* +0x00 */ u32 head_0x00;
    /* +0x04 */ u32 head_0x04;
    /* +0x08 */ Vec3f vec_0x08;
} G3dSingleVecObj; /* size: 0x14 */

typedef struct {
    /* +0x00 */ u8 pad_0x00[0xC];
    /* +0x0C */ u32 sub_0x0C;
} G3dSub0CObj; /* size: 0x10 */

extern "C" void *fn_80067B5C(void *self)
{
    fn_80064834(&((G3dSub0CObj *)self)->sub_0x0C);
    return self;
}

extern "C" nw4r::g3d::Camera::PostureInfo *camera_posture_info_ctor(nw4r::g3d::Camera::PostureInfo *self)
{
    VEC3_ctor((nw4r::math::VEC3 *)&self->mPosX);
    VEC3_ctor((nw4r::math::VEC3 *)&self->mTargetX);
    VEC3_ctor((nw4r::math::VEC3 *)&self->mUpX);
    return self;
}

extern "C" void *fn_80067EB4(void *self)
{
    VEC3_ctor(&((G3dSingleVecObj *)self)->vec_0x08);
    return self;
}

extern "C" u32 fn_800658B0(u32 *p)
{
    return *p;
}

/*
 * The single-vtable dispatch thunks (0x80064CE0, 0x80065ED8 ff.) and the flag readers.
 */

extern "C" void fn_80064CE0(void *self, const f32 *v)
{
    fn_8007A724(self, v[0], v[1], v[2]);
}

extern "C" s32 fn_8006518C(u32 *p)
{
    u32 masked = *p & 4;
    return masked != 0;
}

extern "C" s32 fn_800659C4(u32 *p)
{
    return *p != 0;
}

extern "C" u32 fn_80065ED8(u32 base, u32 size)
{
    u32 mask = ~(size - 1);
    return (size + base - 1) & mask;
}

extern "C" void fn_8006602C(u32 *dst, const u32 *src)
{
    *dst = *src;
}

/* --------------------------------------------------------------------------------------------- *
 * The word accessors of the ResAnmAmbLight cluster (0x80066CF0-0x800670C8).
 * --------------------------------------------------------------------------------------------- */

extern "C" u32 fn_80066CF0(u32 *p)
{
    return *p;
}

extern "C" void fn_80066D48(u32 *dst, const u32 *src)
{
    *dst = *src;
}

extern "C" u32 fn_80066E18(u32 *p)
{
    return *p;
}

extern "C" u32 fn_80066EE4(u32 *p)
{
    return *p;
}

extern "C" s32 fn_80066F50(u32 *p)
{
    return *p != 0;
}

extern "C" void fn_80066F94(u32 *dst, const u32 *src)
{
    *dst = *src;
}

extern "C" s32 fn_80067084(u32 *p)
{
    return *p != 0;
}

extern "C" void fn_800670C8(u32 *dst, const u32 *src)
{
    *dst = *src;
}

/* --------------------------------------------------------------------------------------------- *
 * The cluster's tail (0x80067A54-0x80067EE8).
 * --------------------------------------------------------------------------------------------- */

extern "C" u32 fn_80067A54(u32 *p)
{
    return *p;
}

extern "C" void *vec3_copy_construct(void *out, void *in)
{
    Vec3f *dst = (Vec3f *)out;
    const Vec3f *src = (const Vec3f *)in;
    dst->x = src->x;
    dst->y = src->y;
    dst->z = src->z;
    return out;
}

extern "C" s32 fn_80067EE8(const void *p)
{
    return *(const u32 *)p != 0;
}

/* --------------------------------------------------------------------------------------------- *
 * The label getters and flag/word accessors (0x8006403C-0x80066EEC).
 * --------------------------------------------------------------------------------------------- */

extern "C" u32 fn_8006403C(void)
{
    return (u32)lbl_8058C208;
}

/* 0x80064814 (0xC): returns the class name. */
const char* nw4r::g3d::ResMatChan::GetClassName()
{
    return (const char *)lbl_8058C1D0;
}

extern "C" u32 fn_800658B8(void)
{
    return (u32)lbl_8058CC04;
}

extern "C" u32 fn_80066CF8(void)
{
    return (u32)lbl_8058CA70;
}

extern "C" u32 fn_80066E20(void)
{
    return (u32)lbl_8058C928;
}

extern "C" u32 fn_80066EEC(void)
{
    return (u32)lbl_8058CB38;
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

extern "C" s32 fn_800658C4(u32 *p)
{
    return *p != 0;
}

extern "C" s32 fn_80066D04(u32 *p)
{
    return *p != 0;
}

extern "C" const u8 **type_obj_set_name_texsrt_override(const u8 **out, const u8 *v)
{
    *out = v;
    return out;
}

extern "C" void fn_80064BE4(u32 *p)
{
    *p |= 0x20;
}

extern "C" void fn_80064BF4(u32 *p)
{
    *p |= 0x10;
}

extern "C" void fn_800651A4(u32 *p)
{
    *p &= ~0x4u;
}

/* The flag helpers (0x80064988/0x800649B4) operate on the word at +0xC. */
typedef struct {
    /* +0x00 */ u8 pad_0x00[0xC];
    /* +0x0C */ u32 field_0x0C;
} G3dFlagWord; /* size: 0x10 (approximate - only the +0xC word is evidenced) */

/* 0x80064988 (0x2C): sets or clears animation flag `flag`. */
void nw4r::g3d::AnmObj::SetAnmFlag(AnmFlag flag, bool on)
{
    if (on)
        mFlags |= flag;
    else
        mFlags &= ~flag;
}

/* 0x800649B4 (0x18): whether any of `flag`'s bits is set. */
bool nw4r::g3d::AnmObj::TestAnmFlag(AnmFlag flag) const
{
    return (mFlags & flag) != 0;
}

/* The three-word records (0x800651B4-0x80065204). */
typedef struct {
    /* +0x00 */ u32 field_0x00;
    /* +0x04 */ u32 field_0x04;
    /* +0x08 */ u32 field_0x08;
} G3dWordTriple; /* size: 0xC */

extern "C" u32 fn_800651B4(G3dWordTriple *p)
{
    return p->field_0x04;
}

extern "C" u32 fn_800651E8(G3dWordTriple *p)
{
    return p->field_0x08;
}

extern "C" void fn_800651F0(G3dWordTriple *dst, G3dWordTriple *src)
{
    dst->field_0x00 = src->field_0x00;
    dst->field_0x04 = src->field_0x04;
}

extern "C" s32 fn_80065204(G3dWordTriple *p)
{
    s32 ret = 0;
    if (p->field_0x00 != 0 && p->field_0x04 != 0)
        ret = 1;
    return ret;
}

/* The key arrays (0x8006522C/0x8006527C): a count at +0x2 and an array of 0xC-byte keys at +0xC. */
typedef struct {
    /* +0x00 */ u8 data_0x00[0xC];
} G3dKey; /* size: 0xC */

typedef struct {
    /* +0x00 */ u16 pad_0x00;
    /* +0x02 */ u16 count_0x02;
    /* +0x04 */ u32 pad_0x04;
    /* +0x08 */ u32 pad_0x08;
    /* +0x0C */ G3dKey *array_0x0C;
} G3dKeyArray; /* size: 0x10 */

typedef struct {
    /* +0x00 */ void *field_0x00;
    /* +0x04 */ void *field_0x04;
} G3dPtrPair; /* size: 0x8 */

/* Out-of-range lookups store a NULL element but always store the array owner. */
extern "C" void fn_8006522C(G3dPtrPair *out, G3dKeyArray *self, s32 index)
{
    void *elem;
    if (index < (s32)self->count_0x02 && index >= 0)
        elem = &self->array_0x0C[index];
    else
        elem = 0;
    out->field_0x00 = self;
    out->field_0x04 = elem;
}

extern "C" G3dWordTriple *fn_80065264(G3dWordTriple *a, G3dWordTriple *b)
{
    return (b->field_0x00 < a->field_0x00) ? b : a;
}

extern "C" u16 fn_8006527C(G3dKeyArray *p)
{
    return p->count_0x02;
}

/* The two +0x20 adjustor thunks that feed this cluster's own readers. */
extern "C" u16 fn_80065820(void *p)
{
    return fn_80065828(&((ResAnmScnOwner *)p)->sub_0x20);
}

extern "C" u16 fn_800658E8(void *p)
{
    return fn_800658F0(&((ResAnmScnOwner *)p)->sub_0x20);
}

/* The three-float setter forwarder and the empty-base forwarder. */
extern "C" f32 fmodf(f32 x, f32 y);

/* 0x80064984 (0x4): the float remainder. */
extern "C" f32 math_fmod(f32 x, f32 y)
{
    return fmodf(x, y);
}

extern "C" void fn_80064C04(void *self, const f32 *v)
{
    fn_8007A5E4(self, v[0], v[1], v[2]);
}

extern "C" void fn_80064C14(void *self, const f32 *v)
{
    fn_8007A5A8(self, v[0], v[1], v[2]);
}

/* The first of the two `PlayPolicy` names: `Onetime` keeps its end frame. */
namespace nw4r {
namespace g3d {

f32 PlayPolicy_Onetime(f32 frame, f32 start, f32 end)
{
    (void)frame;
    (void)start;
    return end;
}

/* 0x800648B0 (0xCC): folds `frame` into [0, end - start) by the remainder, a negative frame wrapping from the end. */
f32 PlayPolicy_Loop(f32 start, f32 end, f32 frame)
{
    if (!(start < end)) {
        nw4r::db::Panic(anmobj_cpp_file, 38, anmobj_start_lt_end_msg);
    }
    f32 duration = end - start;
    frame = (frame >= 0.0f) ? math_fmod(frame, duration) : math_fmod(frame + duration, duration);
    return frame + f32_select_nonneg(frame, 0.0f, duration);
}

} /* namespace g3d */
} /* namespace nw4r */

/* --------------------------------------------------------------------------------------------- *
 * The checked getters (0x80063FD0-0x80066D54): resolve the object through the validity test, panic
 * through `nw4r::db::Panic` out of the owning `_ac.h` when it is not valid, then return the resolved
 * pointer (or one of its fields).  The panic shape is nw4r's `NW4R_ASSERT` (the file/line/message and
 * the `"ref"` reference-name argument).
 * --------------------------------------------------------------------------------------------- */
namespace nw4r {
namespace db {

void Panic(const char *file, int line, const char *message, ...);

} /* namespace db */
} /* namespace nw4r */

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

extern "C" ResAnmScnConfig *fn_8006584C(void *p)
{
    if (fn_800658C4((u32 *)p) == 0)
        nw4r::db::Panic((const char *)lbl_8058CC2C, 0x28, (const char *)lbl_8058CC10,
                        (const char *)fn_800658B8(), "ref");
    return (ResAnmScnConfig *)fn_800658B0((u32 *)p);
}

extern "C" u16 fn_80066D54(void *p)
{
    if (fn_80066DB4(p)->field_0x18 == 0xFFFF)
        nw4r::db::Panic((const char *)lbl_8058CA58, 0x79, (const char *)lbl_8058CA18);
    return fn_80066DB4(p)->field_0x18;
}

/* --------------------------------------------------------------------------------------------- *
 * The word-or/and forwarders and the remaining teardown destructors (0x800646E8-0x800677D8).
 * --------------------------------------------------------------------------------------------- */

extern "C" void fn_8005B1B4(u32 *self, u32 value);
extern "C" void dtor_80065768(void *self, s32 flag);

extern "C" void fn_800646E8(void *self, void *p, u32 bits)
{
    fn_8005B1B4((u32 *)self, bits | fn_80064730(p));
}

extern "C" void fn_80064758(void *self, void *p, u32 bits)
{
    fn_8005B1B4((u32 *)self, bits & fn_80064730(p));
}

extern "C" void *fn_80066080(void *self, s16 flags)
{
    if (self != 0) {
        dtor_80065768(self, 0);
        if (flags > 0)
            nw4r::g3d::G3dObj::operator delete(self);
    }
    return self;
}

typedef struct {
    /* +0x00 */ u8 pad_0x00[0x8];
    /* +0x08 */ Vec3f vec_0x08;
    /* +0x14 */ Vec3f vec_0x14;
    /* +0x20 */ u32 sub_0x20;
    /* +0x24 */ u8 pad_0x24[0x14];
    /* +0x38 */ u32 sub_0x38;
} G3dTeardownObj; /* size: 0x3C (approximate - only the sub-object offsets are evidenced) */

extern "C" void *fn_800677D8(void *self)
{
    G3dTeardownObj *obj = (G3dTeardownObj *)self;
    VEC3_ctor(&obj->vec_0x08);
    VEC3_ctor(&obj->vec_0x14);
    fn_80064834(&obj->sub_0x20);
    fn_80064834(&obj->sub_0x38);
    return self;
}

/* The three remaining checked getters (0x80066C8C/0x80066DB4/0x80066E80). */
void *fn_80066C8C(void *obj)
{
    if (fn_80066D04((u32 *)obj) == 0)
        nw4r::db::Panic((const char *)lbl_8058CAA0, 0x25, (const char *)lbl_8058CA80,
                        (const char *)fn_80066CF8(), "ref");
    return (void *)fn_80066CF0((u32 *)obj);
}

ResAnmLightConfig *fn_80066DB4(void *p)
{
    if (fn_80067084((u32 *)p) == 0)
        nw4r::db::Panic((const char *)lbl_8058C950, 0x27, (const char *)lbl_8058C934,
                        (const char *)fn_80066E20(), "ref");
    return (ResAnmLightConfig *)fn_80066E18((u32 *)p);
}

ResAnmLightConfig *fn_80066E80(void *p)
{
    if (fn_80066F50((u32 *)p) == 0)
        nw4r::db::Panic((const char *)lbl_8058CB60, 0x8f, (const char *)lbl_8058CB44,
                        (const char *)fn_80066EEC(), "ref");
    return (ResAnmLightConfig *)fn_80066EE4((u32 *)p);
}

/* The two resolved-object writers (0x80067A5C/0x80067AE4). */
typedef struct {
    /* +0x00 */ void *field_0x00;
} G3dPtrObj; /* size: 0x4 */

typedef struct {
    /* +0x00 */ u32 pad_0x00;
    /* +0x04 */ f32 field_0x04;
    /* +0x08 */ f32 field_0x08;
} G3dPairF32; /* size: 0xC */

extern "C" void fn_80067A5C(void *self, f32 a, f32 b)
{
    if (fn_800659C4((u32 *)self) == 0)
        nw4r::db::Panic((const char *)lbl_8058CBC8, 0x4b, (const char *)lbl_8058CBA8);
    if (fn_800659C4((u32 *)self) != 0) {
        G3dPairF32 *obj = (G3dPairF32 *)(void *)fn_80067A54((u32 *)self);
        obj->field_0x04 = a;
        obj->field_0x08 = b;
    }
}

/* 0x800679D4 (0x80): copies a colour into the fog record the handle names. */
/* untyped: opaque handle - the fog handle */
extern "C" void fog_set_color(void *self, const u8 *rgba)
{
    if (fn_800659C4((u32 *)self) == 0)
        nw4r::db::Panic((const char *)lbl_8058CBF8, 0x63, (const char *)lbl_8058CBD8);
    if (fn_800659C4((u32 *)self) != 0)
        color_rgba_copy((u8 *)fn_80067A54((u32 *)self) + 20, rgba);
}

extern "C" void fn_80067AE4(void *self, void *value)
{
    if (fn_800659C4((u32 *)self) == 0)
        nw4r::db::Panic((const char *)lbl_8058CB98, 0x41, (const char *)lbl_8058CB78);
    if (fn_800659C4((u32 *)self) != 0)
        ((G3dPtrObj *)(void *)fn_80067A54((u32 *)self))->field_0x00 = value;
}

/* The five +0x20 adjustor thunks that forward to the `0x8009xxxx` readers (each adds 0x20 to the
 * `this` pointer before the tail call). */
extern "C" void fn_80090110(void *p);
extern "C" void fn_800902E4(void *p);
extern "C" void fn_800904B8(void *p);
extern "C" void fn_8009068C(void *p);
extern "C" void fn_80090860(void *p);

extern "C" void fn_80065808(void *p)
{
    fn_80090110(&((ResAnmScnOwner *)p)->sub_0x20);
}

extern "C" void fn_80065810(void *p)
{
    fn_800902E4(&((ResAnmScnOwner *)p)->sub_0x20);
}

extern "C" void fn_80065818(void *p)
{
    fn_800904B8(&((ResAnmScnOwner *)p)->sub_0x20);
}

extern "C" void fn_800658D8(void *p)
{
    fn_8009068C(&((ResAnmScnOwner *)p)->sub_0x20);
}

extern "C" void fn_800658E0(void *p)
{
    fn_80090860(&((ResAnmScnOwner *)p)->sub_0x20);
}

/* The bounds-checked light-table reader (0x80066FA0): count = fn_80067060(self), an index past the end
 * panics, and the 0xFFFF entry sentinel panics out of `g3d_reslightset_ac.h`. */
typedef struct {
    /* +0x00 */ u8 pad_0x00[0x3C];
    /* +0x3C */ u16 entries_0x3C[1];
} ResAnmLightTable; /* size: 0x3E (approximate - only the +0x3C table is evidenced) */

extern "C" u16 fn_80066FA0(void *self, u32 index)
{
    u32 count = fn_80067060(self);
    ResAnmLightTable *table;
    if (index > count - 1)
        nw4r::db::Panic((const char *)lbl_8058CA00, 0x71, (const char *)lbl_8058C9C4,
                        index, 0, fn_80067060(self) - 1);
    table = (ResAnmLightTable *)fn_80066DB4(self);
    if (table->entries_0x3C[index] == 0xFFFF)
        nw4r::db::Panic((const char *)lbl_8058C9AC, 0x72, (const char *)lbl_8058C968);
    return table->entries_0x3C[index];
}
