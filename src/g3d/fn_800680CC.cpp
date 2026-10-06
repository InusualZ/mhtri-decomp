/*
 * g3d/fn_800680CC.cpp - nw4r g3d animation-object cluster: the `g3d_anmscn.cpp`, `g3d_anmshp.cpp`,
 *   `g3d_anmtexpat.cpp` and `g3d_anmtexsrt.cpp` bodies - accessor families (getters, `!= 0` readers, word copies,
 *   type-name returners, `Init`/`Set` forwarders, name readers) and the `TestScnAnm`/attach/detach bodies.
 * RANGE. .text 0x800680CC-0x8006EAC0 (133 functions); extab, extabindex, .rodata 0x8056F598-0x8056F628, .data
 *   0x8058CC40-0x8058D6C0, .sdata 0x80791188-0x807911A0, .sdata2 0x80795D90-0x80795DA8.  Four TUs by the `__FILE__`
 *   each body cites: fn_800680CC "g3d_anmscn.cpp" (lbl_8058C288), fn_8006946C "g3d_anmshp.cpp" (lbl_8058CC40),
 *   fn_800697D4 "g3d_anmtexpat.cpp" (lbl_8058CD40), fn_80069CF4 onward "g3d_anmtexsrt.cpp" (lbl_8058CE10).  The left
 *   edge is `g3d/g3d_anmscn.cpp`, the right edge `g3d/g3d_anmvis.cpp`.
 * NAMES. The file keeps the map's stem (`g3d_anmscn.cpp` already names `g3d/g3d_anmscn.cpp`); the stems are defined
 *   `extern "C"` (playbook 48).
 * RESIDUALS. 60 functions unwritten (objdiff scores them zero) in 20 runs: 0x800680CC-0x80068634,
 *   0x800686FC-0x800689B0, 0x80068A78-0x80069334, 0x8006946C-0x800695D4, 0x800696C0-0x800696E4,
 *   0x800697D4-0x8006993C, 0x800699C8-0x80069BD8, 0x80069C80-0x8006A38C, 0x8006A42C-0x8006AA4C,
 *   0x8006AAA8-0x8006C650, 0x8006C668-0x8006CDBC, 0x8006CE48-0x8006D000, 0x8006D084-0x8006D9A0,
 *   0x8006DA48-0x8006E2A8, 0x8006E338-0x8006E668, 0x8006E764-0x8006E794, 0x8006E838-0x8006E868,
 *   0x8006E8C4-0x8006E8F4, 0x8006E998-0x8006E9C8, 0x8006EA90-0x8006EAC0.
 *   Partial (10): fn_8006A38C, fn_8006A3F4, dtor_8006AA4C, fn_8006C650, fn_8006D9FC, fn_8006E668, fn_8006E794,
 *   fn_8006E868, fn_8006E8F4, fn_8006E9C8.  fn_8006C650: retail materialises `~(b - 1)` with an explicit `not`
 *   where MWCC folds it into `andc` (one instruction fewer).
 *   flipcheck: `.text` 0xC8C of 0x69F4; `.rodata`, `.data` and `.sdata2` are claimed and not emitted; `.sdata` is
 *   0x4 of 0x18.
 */

#include "types.h"
#include "g3d/fn_80063888.h"
#include "g3d/g3d_resvtx.h"
#include "g3d/g3d_resmat.h"   /* nw4r::g3d::ResTexSrt (rule 2) */
#include "g3d/g3d_anmchr.h"  /* the fn_8005Dxx / fn_80062xx helpers the g3d_anmchr.cpp range now owns (rule 2) */

/* The `.rodata` animation name records the helper-backed readers pass by address: lbl_8056F598.. are this unit's
 * (claimed, not emitted), lbl_8056F578/lbl_8056F588 are unclaimed. */
extern u8 lbl_8056F578[];
extern u8 lbl_8056F588[];
extern u8 lbl_8056F598[];
extern u8 lbl_8056F5A8[];
extern u8 lbl_8056F5C0[];

/* The `.data` type-name strings the plain returners hand back by address (this unit's `.data`, and
 * `g3d/fn_80063888.cpp`'s for lbl_8058CAFC/lbl_8058CAB8; claimed, not emitted). */
extern u8 lbl_8058CAFC[]; /* "ResAnmFog"     */
extern u8 lbl_8058CAB8[]; /* "ResAnmCamera"  */
extern u8 lbl_8058CCC8[]; /* "ResAnmShp"     */
extern u8 lbl_8058CD04[]; /* "ResVtxPos"     */
extern u8 lbl_8058CDC8[]; /* "ResAnmTexPat"  */
extern u8 lbl_8058D678[]; /* "ResAnmTexSrt"  */
extern u8 lbl_8058D5F8[]; /* "ResTexSrt"     */

/* --------------------------------------------------------------------------------------------- *
 * The word accessors (0x80068698-0x8006E6B4): read / test / copy the body word at +0x0.
 * --------------------------------------------------------------------------------------------- */

extern "C" u32 fn_80068698(void *p)
{
    return *(u32 *)p;
}

extern "C" u32 fn_80068A14(void *p)
{
    return *(u32 *)p;
}

extern "C" u32 fn_80069650(void *p)
{
    return *(u32 *)p;
}

extern "C" u32 fn_800699A0(void *p)
{
    return *(u32 *)p;
}

extern "C" u32 fn_8006CE20(void *p)
{
    return *(u32 *)p;
}

/* 0x8006E310 (0x8): returns the texture-SRT block. */
nw4r::g3d::ResTexSrtData* nw4r::g3d::ResTexSrt::ptr()
{
    return static_cast<ResTexSrtData *>(mpData);
}

extern "C" u32 fn_800686AC(void *p)
{
    return *(u32 *)p != 0;
}

extern "C" u32 fn_80068A28(void *p)
{
    return *(u32 *)p != 0;
}

extern "C" u32 fn_800696AC(void *p)
{
    return *(u32 *)p != 0;
}

extern "C" u32 fn_800699B4(void *p)
{
    return *(u32 *)p != 0;
}

extern "C" u32 fn_8006CE34(void *p)
{
    return *(u32 *)p != 0;
}

/* 0x8006E324 (0x14): tells whether the handle is set. */
bool nw4r::g3d::ResTexSrt::IsValid() const
{
    return mpData != NULL;
}

/* 0x8006E6B4 (0x14): tells whether the handle is set. */
bool nw4r::g3d::ResMatIndMtxAndScale::IsValid() const
{
    return mpData != NULL;
}

extern "C" void fn_800686F0(u32 *dst, const u32 *src)
{
    *dst = *src;
}

extern "C" void fn_80068A6C(u32 *dst, const u32 *src)
{
    *dst = *src;
}

extern "C" void fn_80069798(u32 *dst, const u32 *src)
{
    *dst = *src;
}

extern "C" void fn_80069C08(u32 *dst, const u32 *src)
{
    *dst = *src;
}

extern "C" void fn_80069C44(u32 *dst, const u32 *src)
{
    *dst = *src;
}

extern "C" void fn_8006D030(u32 *dst, const u32 *src)
{
    *dst = *src;
}

/* --------------------------------------------------------------------------------------------- *
 * The type-name returners (0x800686A0-0x8006E318): hand back the address of the `.data` name record.
 * --------------------------------------------------------------------------------------------- */

extern "C" u32 fn_800686A0(void)
{
    return (u32)lbl_8058CAFC;
}

extern "C" u32 fn_80068A1C(void)
{
    return (u32)lbl_8058CAB8;
}

extern "C" u32 fn_80069658(void)
{
    return (u32)lbl_8058CCC8;
}

extern "C" u32 fn_800699A8(void)
{
    return (u32)lbl_8058CDC8;
}

extern "C" u32 fn_8006CE28(void)
{
    return (u32)lbl_8058D678;
}

/* 0x8006E318 (0xC): returns the class name. */
const char *nw4r::g3d::ResTexSrt::GetClassName()
{
    return (const char *)lbl_8058D5F8;
}

/* --------------------------------------------------------------------------------------------- *
 * The two argument-less bodies (0x8006D9A0/0x8006E2A8): `blr`, no work.
 * --------------------------------------------------------------------------------------------- */

extern "C" void fn_8006D9A0(void)
{
}

extern "C" void fn_8006E2A8(void)
{
}

/* --------------------------------------------------------------------------------------------- *
 * The float helpers (0x8006E690/0x8006E6A4): a bit reinterpret and an `fmax`.
 * --------------------------------------------------------------------------------------------- */

extern "C" u32 fn_8006E690(f32 x)
{
    return *(u32 *)&x;
}

extern "C" f32 fn_8006E6A4(f32 x, f32 y)
{
    if (x < y)
        return y;
    return x;
}

/* Rounds `a` up to the next multiple of `b`. */
extern "C" u32 fn_8006C650(u32 a, u32 b)
{
    return (a + b - 1) & -b;
}

/* --------------------------------------------------------------------------------------------- *
 * The store-then-return forwarders (0x800686C0-0x8006D000): write through the word copier beside each
 * one and hand the original `self` back in r3.
 * --------------------------------------------------------------------------------------------- */

extern "C" void fn_800686F0(u32 *dst, const u32 *src);

extern "C" void *fn_800686C0(void *self, void *src)
{
    fn_800686F0((u32 *)self, (const u32 *)src);
    return self;
}

extern "C" void fn_80068A6C(u32 *dst, const u32 *src);

extern "C" void *fn_80068A3C(void *self, void *src)
{
    fn_80068A6C((u32 *)self, (const u32 *)src);
    return self;
}

extern "C" void fn_80069C08(u32 *dst, const u32 *src);

extern "C" void *fn_80069BD8(void *self, void *src)
{
    fn_80069C08((u32 *)self, (const u32 *)src);
    return self;
}

extern "C" void fn_80069C44(u32 *dst, const u32 *src);

extern "C" void *fn_80069C14(void *self, void *src)
{
    fn_80069C44((u32 *)self, (const u32 *)src);
    return self;
}

extern "C" void fn_8006D030(u32 *dst, const u32 *src);

extern "C" void *fn_8006D000(void *self, void *src)
{
    fn_8006D030((u32 *)self, (const u32 *)src);
    return self;
}

/* --------------------------------------------------------------------------------------------- *
 * The helper-backed name readers (0x800693A0-0x8006E734): store the name record through one of the
 * unsplit store helpers and read the word back through the returned address.
 * --------------------------------------------------------------------------------------------- */

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

extern "C" u32 fn_800697A4(void)
{
    void *local;
    return (u32)*fn_8005DC60(&local, lbl_8056F598);
}

extern "C" u32 fn_80069C50(void)
{
    void *local;
    return (u32)*fn_8005DCD0(&local, lbl_8056F5A8);
}

extern "C" u32 fn_8006A3C4(void)
{
    void *local;
    return (u32)*fn_8005DCD0(&local, lbl_8056F5C0);
}

extern "C" u32 fn_8006E734(void)
{
    void *local;
    return (u32)*fn_8005DCD0(&local, lbl_8056F5C0);
}

/* --------------------------------------------------------------------------------------------- *
 * The list-insert chain steps (0x80069334-0x8006EA24): resolve a name record, compare the caller's key
 * against it through the `fn_800639D0` helper, and on a miss run the previous step's insertion with a
 * copy of the key word (the shape `g3d/fn_80063888.cpp`'s `fn_800638F8` uses beside them).
 * --------------------------------------------------------------------------------------------- */

/* The three name readers this cluster's own insert steps chain into, defined in the helper section
 * beside their `fn_80063898`-family siblings. */
extern "C" u32 fn_8006E838(void);
extern "C" u32 fn_8006E998(void);
extern "C" u32 fn_8006EA90(void);

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

extern "C" u32 fn_8006E6C8(void *self, u32 *key)
{
    u32 res = fn_8006E734();

    if ((*reinterpret_cast<const nw4r::g3d::G3dObj::TypeObj*>(key) == *reinterpret_cast<const nw4r::g3d::G3dObj::TypeObj*>(&res)))
        return 1;
    return reinterpret_cast<const nw4r::g3d::AnmObj*>(self)->nw4r::g3d::AnmObj::IsDerivedFrom(*reinterpret_cast<const nw4r::g3d::G3dObj::TypeObj*>(key));
}

extern "C" u32 fn_8006E7CC(void *self, u32 *key)
{
    u32 res = fn_8006E838();

    if ((*reinterpret_cast<const nw4r::g3d::G3dObj::TypeObj*>(key) == *reinterpret_cast<const nw4r::g3d::G3dObj::TypeObj*>(&res)))
        return 1;
    u32 local = *key;
    return fn_8006E6C8(self, &local);
}

extern "C" u32 fn_8006E92C(void *self, u32 *key)
{
    u32 res = fn_8006E998();

    if ((*reinterpret_cast<const nw4r::g3d::G3dObj::TypeObj*>(key) == *reinterpret_cast<const nw4r::g3d::G3dObj::TypeObj*>(&res)))
        return 1;
    u32 local = *key;
    return fn_8006E7CC(self, &local);
}

extern "C" u32 fn_8006EA24(void *self, u32 *key)
{
    u32 res = fn_8006EA90();

    if ((*reinterpret_cast<const nw4r::g3d::G3dObj::TypeObj*>(key) == *reinterpret_cast<const nw4r::g3d::G3dObj::TypeObj*>(&res)))
        return 1;
    u32 local = *key;
    return fn_8006E6C8(self, &local);
}

/* --------------------------------------------------------------------------------------------- *
 * The teardown destructors and the checked word reader (0x8006AA4C-0x8006E9C8).
 * --------------------------------------------------------------------------------------------- */

/* The g3d/main-band teardown helper this unit declares itself; fn_8005D384/G3dObj::operator delete/fn_800628A4/
 * fn_800628B4/fn_80062914 come from `g3d/g3d_anmchr.h`. */
extern "C" void dtor_8006AAA8(void *self, s32 flag);

typedef struct {
    /* +0x00 */ u8 pad_0x00[0x4];
    /* +0x04 */ u32 field_0x04;
} G3dResNameWord; /* size: 0x8 */

/* 0x80069664 (0x48): returns the dictionary's entry count, 0 for an empty handle. */
u32 nw4r::g3d::ResDic::GetNumData() const
{
    if (fn_800628B4(const_cast<ResDic*>(this)))
        return ((G3dResNameWord *)fn_800628A4(const_cast<ResDic*>(this)))->field_0x04;
    return 0;
}

extern "C" void *dtor_8006AA4C(void *self, s32 flag)
{
    if (self != 0) {
        dtor_8005D384(self, 0);
        if ((s16)flag > 0)
            nw4r::g3d::G3dObj::operator delete(self);
    }
    return self;
}

extern "C" void *fn_8006E868(void *self, s32 flag)
{
    if (self != 0) {
        dtor_8006AAA8(self, 0);
        if ((s16)flag > 0)
            nw4r::g3d::G3dObj::operator delete(self);
    }
    return self;
}

extern "C" void *fn_8006E9C8(void *self, s32 flag)
{
    if (self != 0) {
        dtor_8006AA4C(self, 0);
        if ((s16)flag > 0)
            nw4r::g3d::G3dObj::operator delete(self);
    }
    return self;
}

/* --------------------------------------------------------------------------------------------- *
 * The float-packed field reader (0x8006E668) and the indexed resource reader (0x8006D9FC).
 * --------------------------------------------------------------------------------------------- */

extern "C" s32 fn_8006E668(f32 x)
{
    return (s32)(((fn_8006E690(x) >> 15) & 0xFF) - 127);
}

extern "C" u32 fn_8006D9FC(void *self, u32 idx)
{
    u32 base = *(u32 *)self;

    if (idx != 0)
        return (u32)nw4r::g3d::ResDic((void *)(base + idx)).mpData;
    return (u32)nw4r::g3d::ResDic((void *)0).mpData;
}

/* --------------------------------------------------------------------------------------------- *
 * The seven checked getters (0x80068634-0x8006E2AC): resolve the object through the validity test,
 * panic through `nw4r::db::Panic` out of the owning `_ac.h` when it is not valid, then read the body
 * word through the resolver beside it.  The shape is nw4r's `NW4R_ASSERT` (file/line/message and the
 * `"ref"` reference-name argument).
 * --------------------------------------------------------------------------------------------- */

namespace nw4r {
namespace db {

void Panic(const char *file, int line, const char *message, ...);

} /* namespace db */
} /* namespace nw4r */

extern u8 lbl_8058CB24[];
extern u8 lbl_8058CB08[];
extern u8 lbl_8058CAE4[];
extern u8 lbl_8058CAC8[];
extern u8 lbl_8058CCF0[];
extern u8 lbl_8058CCD4[];
extern u8 lbl_8058CD30[];
extern u8 lbl_8058CD10[];
extern u8 lbl_8058CDF4[];
extern u8 lbl_8058CDD8[];
extern u8 lbl_8058D6A4[];
extern u8 lbl_8058D688[];
extern u8 lbl_8058D620[];
extern u8 lbl_8058D604[];

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

extern "C" u32 fn_800695EC(void *p)
{
    if (fn_800696AC(p) == 0)
        nw4r::db::Panic((const char *)lbl_8058CCF0, 166, (const char *)lbl_8058CCD4,
                        (const char *)fn_80069658(), "ref");
    return fn_80069650(p);
}

extern "C" u32 fn_8006993C(void *p)
{
    if (fn_800699B4(p) == 0)
        nw4r::db::Panic((const char *)lbl_8058CDF4, 92, (const char *)lbl_8058CDD8,
                        (const char *)fn_800699A8(), "ref");
    return fn_800699A0(p);
}

extern "C" u32 fn_8006CDBC(void *p)
{
    if (fn_8006CE34(p) == 0)
        nw4r::db::Panic((const char *)lbl_8058D6A4, 38, (const char *)lbl_8058D688,
                        (const char *)fn_8006CE28(), "ref");
    return fn_8006CE20(p);
}

/* 0x8006E2AC (0x64): returns the texture-SRT block, panicking on a NULL handle. */
nw4r::g3d::ResTexSrtData& nw4r::g3d::ResTexSrt::ref()
{
    if (!IsValid())
        nw4r::db::Panic((const char *)lbl_8058D620, 107, (const char *)lbl_8058D604, GetClassName(), "ref");
    return *ptr();
}

/* --------------------------------------------------------------------------------------------- *
 * The three field readers of the resolved `ResAnmTexSrt` record (0x8006D03C/0x8006D060/0x8006D9A4).
 * --------------------------------------------------------------------------------------------- */

typedef struct {
    /* +0x00 */ u8 pad_0x00[0x10];
    /* +0x10 */ u32 field_0x10;
    /* +0x14 */ u8 pad_0x14[0xC];
    /* +0x20 */ u16 field_0x20;
    /* +0x22 */ u8 pad_0x22[0x6];
    /* +0x28 */ u32 field_0x28;
} ResAnmTexSrtData; /* size: 0x2C (approximate - only the three fields read here are evidenced) */

extern "C" u16 fn_8006D060(void *p)
{
    return ((ResAnmTexSrtData *)(void *)fn_8006CDBC(p))->field_0x20;
}

extern "C" u32 fn_8006D03C(void *p)
{
    return ((ResAnmTexSrtData *)(void *)fn_8006CDBC(p))->field_0x28;
}

extern "C" u32 fn_8006D9A4(void *self, void *key)
{
    u32 tmp = fn_8006D9FC(self, ((ResAnmTexSrtData *)(void *)fn_8006CDBC(self))->field_0x10);

    return (u32)(*reinterpret_cast<nw4r::g3d::ResDic *>(&tmp))[(int)key];
}

/* --------------------------------------------------------------------------------------------- *
 * The vtable-dispatch wrappers (0x8006A38C/0x8006A3F4/0x8006E794/0x8006E8F4): the body word 0 is its
 * vtable, the entry at +0x14 runs with the object and a word is read back through the `TypeObj::GetTypeName`
 * helper (the shape `g3d/fn_80063888.cpp` uses for its three wrappers).
 * --------------------------------------------------------------------------------------------- */

extern "C" u32 fn_8006A38C(void *p)
{
    G3dObj *obj = (G3dObj *)p;
    u32 tmp = obj->vt->method_0x14(p);

    return (u32)reinterpret_cast<const nw4r::g3d::G3dObj::TypeObj*>(&tmp)->GetTypeName();
}

extern "C" u32 fn_8006A3F4(void *p)
{
    G3dObj *obj = (G3dObj *)p;
    u32 tmp = obj->vt->method_0x14(p);

    return (u32)reinterpret_cast<const nw4r::g3d::G3dObj::TypeObj*>(&tmp)->GetTypeName();
}

extern "C" u32 fn_8006E794(void *p)
{
    G3dObj *obj = (G3dObj *)p;
    u32 tmp = obj->vt->method_0x14(p);

    return (u32)reinterpret_cast<const nw4r::g3d::G3dObj::TypeObj*>(&tmp)->GetTypeName();
}

extern "C" u32 fn_8006E8F4(void *p)
{
    G3dObj *obj = (G3dObj *)p;
    u32 tmp = obj->vt->method_0x14(p);

    return (u32)reinterpret_cast<const nw4r::g3d::G3dObj::TypeObj*>(&tmp)->GetTypeName();
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
