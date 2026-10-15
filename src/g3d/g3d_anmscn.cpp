/*
 * g3d/g3d_anmscn.cpp - nw4r g3d `g3d_anmscn.cpp`: the scene-animation object (`AnmScn`), the checked `_ac.h`
 *   getters of the `ResAnmScn`/`ResAnmAmbLight`/`ResAnmLight`/`ResAnmFog`/`ResAnmCamera` headers it includes, their
 *   word accessors and the list-insert chain steps.
 * RANGE. .text 0x800649CC-0x8006946C; extab 0x800078F4-0x80007BB4, extabindex 0x8001FC98-0x8002004C, .rodata
 *   0x8056F578-0x8056F598, .data 0x8058C288-0x8058CC40, .sdata 0x80791170-0x80791188, .sdata2 0x80795D70-0x80795D90.
 *   The TU is `g3d_anmscn.cpp`: its one-copy `__FILE__` string (0x8058C288) is cited from fn_800649CC through
 *   fn_80068FC4; the previous TU's (`g3d_anmobj.cpp`, 0x8058C248) by PlayPolicy_Loop and the next TU's
 *   (`g3d_anmshp.cpp`, 0x8058CC40) from anm_obj_shp_test_existence on.  The `.rodata` records 0x8056F578/0x8056F588 and the `.sdata`
 *   words 0x80791170-0x80791184 are read only from inside the range.
 * FLAGS. `cflags_g3d`'s `-O3` is measured on fn_800680A8 (docs/g3d.md, playbook 27).
 * NAMES. The stems are defined `extern "C"` (playbook 48); `fn_80066C8C` and `fn_800680A8` keep C++ linkage (map rows
 *   `fn_80066C8C__FPv`, `fn_800680A8__FPv`; the source spells the bare stem, playbook 50).  The cross-unit
 *   declarations are `g3d/g3d_anmscn.h`.
 *   GUESS: `vec3_copy_construct` (0x80067E54): copies three floats into `out` and returns it (a VEC3 copy).
 *   AnmScn's constructor is nw4r's member.  obj_delete_flagged is a GUESS; fog_set_color is a GUESS;
 *   res_anm_light_config_test_flag8 is a GUESS (the bodies' behaviour).
 * RESIDUALS. `fn_80066FA0` lacks retail's call to `fn_80066DB4` (+0x9C).  AnmScn's destructor (dtor_80065768) stays
 *   undefined: defining `AnmScn::~AnmScn` makes the explicit call from fn_80066080 pass -1 where retail passes 0.
 *   Unwritten (31 functions, objdiff scores them zero): 0x800649CC-0x80064BD4, 0x80064C24-0x80064CE0,
 *   0x80064CF0-0x8006518C, 0x800652C8-0x8006553C, 0x8006560C-0x800657C4, 0x800659D8-0x80065ED8,
 *   0x80065EF0-0x80065FFC, 0x800660DC-0x80066C68, 0x800670D4-0x800677D8, 0x80067824-0x800679D4,
 *   0x80067B90-0x80067E54, 0x80067EFC-0x800680A8, 0x800680CC-0x80068634, 0x800686FC-0x800689B0,
 *   0x80068A78-0x80069334.
 *   Partial: fn_8006522C, fn_8006553C, fn_800655A4, fn_80065ED8, fn_80066FA0.
 *   AnmScn's constructor has an empty body by design (the compiler emits the G3dObj base call and the vtable store).
 *   flipcheck: `.text`, extab and extabindex are partial; `.rodata`, `.data` and `.sdata2` are claimed and not
 *   emitted.
 * SHAPES. The unit compiles with `#pragma peephole off` throughout (retail keeps the unfused `clrlwi` + `cmpwi`, `extsh`,
 *   `addi r0` vtable-store and `mr r3` + `lwz r12,0(r3)` virtual-call forms; playbook idea 106); the
 *   `ResAnmFog`/`ResAnmCamera` accessors at the end compile under `#pragma pool_data off`.
 */

#include "types.h"
#include "nw4r/g3d/res_common.h" /* ResHandle (rule 1) */
#include "unsplit/g3d.h"      /* the math types it reads through the band header */
#include "g3d/g3d_light.h" /* fn_8007A5A8 / fn_8007A5E4 / fn_8007A724 (rule 2) */
#include "g3d/g3d_anmscn.h"
#include "g3d/g3d_anmclr.h"   /* color_to_u32_ref, color_ctor_white, owned by g3d/g3d_anmclr.cpp (rule 2) */
#include "g3d/g3d_anmchr.h"   /* the fn_8005Dxx helpers, the type-info members and G3dObj, owned by g3d/g3d_anmchr.cpp (rule 2) */
#include "g3d/g3d_resmat.h"   /* nw4r::g3d::ResMat / ResMatChan (rule 2) */
#include "fn_80047398.h" /* color_rgba_copy (rule 2) */
#include "mh3_pad.h"        /* VEC3_ctor, owned by mh3_pad.cpp (rule 2) */

#pragma peephole off

/* Forward declarations: this unit's own copy helpers. */
extern "C" void fn_8006602C(u32 *dst, const u32 *src);
extern "C" void fn_80066D48(u32 *dst, const u32 *src);
extern "C" void fn_80066F94(u32 *dst, const u32 *src);
extern "C" void fn_800670C8(u32 *dst, const u32 *src);

/* A three-float vector record (the cluster's `.text` copies it field by field, so the struct is the
 * explicit-float-copy shape, not a word copy). */
typedef nw4r::math::VEC3 Vec3f; /* size: 0xC */

/* The cluster's `.rodata` animation type-name records (0x8056F500-0x8056F678): a length word followed by
 * the NUL-terminated name.  Referenced by address only. */
extern u8 lbl_8056F578[];
extern u8 lbl_8056F588[];

/* The type-name strings the plain getters below return by address (`.data`/`.rodata`, read-only). */
extern u8 lbl_8058C928[];
extern u8 lbl_8058CA70[];
extern u8 lbl_8058CB38[];
extern u8 lbl_8058CC04[];
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
 * The float and flag helpers (0x8006497C-0x80065ED8).
 * --------------------------------------------------------------------------------------------- */

extern "C" void fn_80064BD4(u32 *p)
{
    *p |= 4;
}

/* Reads the +0x0 word through the identity helper (0x80064754), the shape the two readers share. */
extern "C" u32 fn_800651BC(void *self)
{
    return *color_to_u32_ref((u32 *)self);
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

/* --------------------------------------------------------------------------------------------- *
 * The composite teardown destructors (0x80067B5C/0x80067E70/0x80067EB4): construct-or-clear each
 * sub-record of the object through `VEC3_ctor`/`color_ctor_white`, then hand the object back.
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
    color_ctor_white(&((G3dSub0CObj *)self)->sub_0x0C);
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

extern "C" s32 fn_800658C4(u32 *p)
{
    return *p != 0;
}

extern "C" s32 fn_80066D04(u32 *p)
{
    return *p != 0;
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
extern "C" void fn_80064C04(void *self, const f32 *v)
{
    fn_8007A5E4(self, v[0], v[1], v[2]);
}

extern "C" void fn_80064C14(void *self, const f32 *v)
{
    fn_8007A5A8(self, v[0], v[1], v[2]);
}

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
 * The remaining teardown destructors (0x80066080-0x800677D8).
 * --------------------------------------------------------------------------------------------- */

extern "C" void dtor_80065768(void *self, s32 flag);

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
    color_ctor_white(&obj->sub_0x20);
    color_ctor_white(&obj->sub_0x38);
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

/* 0x800680A8 (0x24): resolves the ambient-light handle through the checked getter and reads the word at +0xC. */
unsigned int fn_800680A8(void *obj)
{
    return *(unsigned int *)((char *)fn_80066C8C(obj) + 0xC);
}

/* --------------------------------------------------------------------------------------------- *
 * The `ResAnmFog`/`ResAnmCamera` accessors (0x80068634-0x80068A6C) and the list-insert chain steps
 * (0x80069334-0x8006943C).
 * --------------------------------------------------------------------------------------------- */

#pragma pool_data off

extern u8 lbl_8058CAFC[]; /* "ResAnmFog"     */
extern u8 lbl_8058CAB8[]; /* "ResAnmCamera"  */
extern u8 lbl_8058CB24[];
extern u8 lbl_8058CB08[];
extern u8 lbl_8058CAE4[];
extern u8 lbl_8058CAC8[];

extern "C" u32 fn_80068698(void *p)
{
    return *(u32 *)p;
}

extern "C" u32 fn_80068A14(void *p)
{
    return *(u32 *)p;
}

extern "C" u32 fn_800686AC(void *p)
{
    return *(u32 *)p != 0;
}

extern "C" u32 fn_80068A28(void *p)
{
    return *(u32 *)p != 0;
}

extern "C" void fn_800686F0(u32 *dst, const u32 *src)
{
    *dst = *src;
}

extern "C" void fn_80068A6C(u32 *dst, const u32 *src)
{
    *dst = *src;
}

extern "C" u32 fn_800686A0(void)
{
    return (u32)lbl_8058CAFC;
}

extern "C" u32 fn_80068A1C(void)
{
    return (u32)lbl_8058CAB8;
}

extern "C" void *fn_800686C0(void *self, void *src)
{
    fn_800686F0((u32 *)self, (const u32 *)src);
    return self;
}

extern "C" void *fn_80068A3C(void *self, void *src)
{
    fn_80068A6C((u32 *)self, (const u32 *)src);
    return self;
}

extern "C" u32 fn_800693A0(void)
{
    const u8 *local;
    return (u32)*type_obj_set_name(&local, lbl_8056F578);
}

extern "C" u32 fn_8006943C(void)
{
    void *local;
    return (u32)*fn_8005DC60(&local, lbl_8056F588);
}

extern "C" u32 fn_80069334(void *self, u32 *key)
{
    u32 res = fn_800693A0();

    if ((*reinterpret_cast<const nw4r::g3d::G3dObj::TypeObj*>(key) == *reinterpret_cast<const nw4r::g3d::G3dObj::TypeObj*>(&res)))
        return 1;
    return reinterpret_cast<const nw4r::g3d::G3dObj*>(self)->nw4r::g3d::G3dObj::IsDerivedFrom(*reinterpret_cast<const nw4r::g3d::G3dObj::TypeObj*>(key));
}

extern "C" u32 fn_800693D0(void *self, u32 *key)
{
    u32 res = fn_8006943C();

    if ((*reinterpret_cast<const nw4r::g3d::G3dObj::TypeObj*>(key) == *reinterpret_cast<const nw4r::g3d::G3dObj::TypeObj*>(&res)))
        return 1;
    u32 local = *key;
    return fn_80069334(self, &local);
}

extern "C" u32 fn_80068634(void *p)
{
    if (fn_800686AC(p) == 0)
        nw4r::db::Panic((const char *)lbl_8058CB24, 62, (const char *)lbl_8058CB08,
                        (const char *)fn_800686A0(), "ref");
    return fn_80068698(p);
}

extern "C" u32 fn_800689B0(void *p)
{
    if (fn_80068A28(p) == 0)
        nw4r::db::Panic((const char *)lbl_8058CAE4, 125, (const char *)lbl_8058CAC8,
                        (const char *)fn_80068A1C(), "ref");
    return fn_80068A14(p);
}
