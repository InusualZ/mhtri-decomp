/*
 * g3d/g3d_anmchr.cpp - nw4r g3d character animation (`g3d_anmchr.cpp`), led by the g3d work-memory accessors,
 *   with small constant/forwarder helpers.
 * RANGE. .text 0x8005CED0-0x80063E60 (149 functions); extab 0x800073C8-0x8000776C, extabindex
 *   0x8001F71C-0x8001FBFC, .rodata 0x8056F500-0x8056F510 (the "AnmObjChr" name record, read only here), .data 0x8058B410-0x8058C118, .bss 0x8066AE80-0x80682E80, .sdata 0x80791138-0x80791158,
 *   .sdata2 0x80795D48-0x80795D68.  The asserts from 0x8005CF10 on pass "g3d_anmchr.cpp" (.data 0x8058B410).
 * RANGE. Left edge 0x8005CED0: the font/debug-print console before it is `font/flfnt.cpp` (its header has the
 *   seam evidence).  fn_8005CED0-fn_8005CF04 return the 0x18000-byte `.bss` buffer 0x8066AE80-0x80682E80, read
 *   only by g3d calc units; they are nw4r g3d work memory and may be a TU of their own (unproven).  Right edge
 *   0x80063E60: the five vtables (.data 0x8058BCA8-0x8058BE24, inside this claim) point at 13 functions of
 *   0x80063888-0x80063E60 (the weak type-info members MWCC emits at the end of the TU that defines the vtables),
 *   the other 8 of the run are called only from inside it or from other g3d units (fn_80063964, fn_80063AC4), and
 *   0x80063E60 onward is called by g3d_calcmaterial, g3d_scnmdl and g3d_resfile; the extabindex records of the
 *   run end at 0x8001FBFC, the last record (0x80063E30) with its extab 0x80007764-0x8000776C.
 * NAMES. The map's own names and stems, the stems defined `extern "C"`.
 *   GUESS: `type_obj_set_name` (0x800638B8: stores a type-name record pointer through `out`, the weak type-info
 *   helper) and `anm_typename_AnmObj`/`_AnmObjChrNode`/`_AnmObjChrBlend`/`_G3dObj` (the `.rodata` records by their strings).
 * RESIDUALS. Unwritten (objdiff scores them zero): 72 of the 149 functions, every one except the 64 matched and
 *   these partial ones - fn_8005D27C, fn_8005DC30, fn_8005DCA0, fn_800604DC, fn_8006244C, fn_80062824, fn_800628AC,
 *   fn_800638C0, fn_800639D0, fn_80063B2C, fn_80063C00, fn_80063C94, fn_80063D68.
 *   The type-name records the 0x80063888-0x80063E60 functions read (`.rodata` 0x8056F500-0x8056F578) sit in
 *   `g3d/fn_80063888.cpp`'s `.rodata` claim (0x8056F510-0x8056F578): their seam is unmoved.
 *   flipcheck: `.text` 0xB38 of 0x6F90; extab 0x110 of 0x3A4; extabindex 0x198 of 0x4E0; `.rodata`, `.data`, `.bss`, `.sdata` and `.sdata2` are claimed and not emitted.
 *   Relocation names that differ from retail (pool constants, save helpers, statics): `lbl_80791148`,
 *     `lbl_807911E8`, `lbl_80791168`, `lbl_80791150`.
 * SHAPES. The ResName constructor is complete: its work is the base initialiser.
 */

#include "types.h"
#include "g3d/fn_80063888.h" /* the anm_typename_* records, owned by g3d/fn_80063888.cpp (rule 2) */
#include "g3d/fn_80075DCC.h" /* anm_typename_G3dObj, owned by g3d/fn_80075DCC.cpp (rule 2) */
#include "g3d/g3d_anmchr.h" /* this unit's own declarations, and the G3dObj dispatch record (rule 1) */
#include "g3d/g3d_resmat.h" /* nw4r::g3d::ResMdl (rule 2) */
#include "g3d/g3d_resnode.h" /* nw4r::g3d::ResNode (rule 2) */
#include "fn_8004CAD8.h"   /* mtx34_const_ptr's owner header (docs/plan.md 6.5, rule 2) */
#include "nw4r/math_arithmetic.h"   /* nw4r::math::detail::FExp, owner nw4r/math_arithmetic.cpp (rule 2) */

/* The teardown destructors fn_80063C00/fn_80063D68 call (this unit's, not yet written). */
extern "C" void dtor_8005E5E8(void *self, s32 flag);
extern "C" void dtor_8005E58C(void *self, s32 flag);

/* --- the SDK entry points this unit tail-calls (owners are unsplit; declared, never defined) --------- */

extern "C" void fn_804C6F40(u32, u32, void*);
extern "C" void fn_804C6D70(void*, u32);
extern "C" void fn_80061EC4(void*, f32);

/* The g3d math constant fn_8006244C scales by (`.sdata` 0x807911E8). */
extern f32 lbl_807911E8;

/* The `.sdata` strings fn_8005D27C / fn_800628AC hand back (`_SDA_BASE_`-relative), and the two
 * absolute constants the blocked-return helpers hand back (`.bss` / `.data`). */
extern u8 lbl_80791148[];
extern u8 lbl_80791150[];
extern u8 lbl_8066AE80[];
extern u8 lbl_8058BF14[];
extern const char anmchr_resnode_align_assert_msg[]; /* the ResNode alignment assertion */
extern const char anmchr_resnode_ac_file[]; /* "g3d_resnode_ac.h" */
extern const char anmchr_resdic_idx_bounds_msg[]; /* the ResDic index assertion */
extern const char anmchr_resdic_ac_file[]; /* "g3d_resdict.h" */
extern const char anmchr_resdic_align_assert_msg[]; /* the ResDic alignment assertion */
extern const char anmchr_resdic_ac_file_ctor[]; /* "g3d_resdict.h" */
extern u8 lbl_8056F500[];
extern u8 lbl_8056F538[];

/* --- bodies: the g3d work-memory accessors and tiny constant/identity helpers ---------------------- */

extern "C" u8* fn_8005CED0(void)
{
    return lbl_8066AE80;
}

extern "C" u8* fn_8005CEDC(void)
{
    return lbl_8066AE80 + 0x6000;
}

extern "C" u8* fn_8005CEEC(void)
{
    return lbl_8066AE80;
}

extern "C" u8* fn_8005CEF8(void)
{
    return lbl_8066AE80;
}

extern "C" u8* fn_8005CF04(void)
{
    return lbl_8066AE80;
}

extern "C" u32* fn_8005D0C4(u32** pp)
{
    return *pp;
}

extern "C" void fn_8005D114(u32* a, u32* b)
{
    (void)a;
    (void)b;
}

extern "C" void fn_8005D118(u32* dst, u32* src)
{
    *dst = *src;
}

/* 0x8005D210 (0x8): stores the block address. */
NW4R_G3D_RESCOMMON_CTOR(ResNodeData)

/* 0x8005D1AC (0x64): wraps `pData`, asserting its alignment. */
#pragma peephole off
/* untyped: opaque handle - the node block */
nw4r::g3d::ResNode::ResNode(void* pData) : ResCommon<ResNodeData>(pData) {
    if ((u32)pData & 0x3) {
        nw4r::db::Panic((const char*)anmchr_resnode_ac_file, 44, (const char*)anmchr_resnode_align_assert_msg);
    }
}
#pragma peephole on

extern "C" u8* fn_8005D27C(void)
{
    return lbl_80791148;
}

extern "C" void fn_8005D2F0(u32* dst, u32* src)
{
    *dst = *src;
}

/* 0x8005D2FC (0x14): tells whether the handle is set. */
bool nw4r::g3d::ResMdl::IsValid() const
{
    return mpData != NULL;
}

extern "C" void fn_8005D3E0(void *self)
{
}

extern "C" u32 fn_8005DC24(u32* p)
{
    return *p + 4;
}

extern "C" void **fn_8005DC60(void **out, void *v)
{
    *out = v;
    return out;
}

extern "C" void **fn_8005DCD0(void **out, void *v)
{
    *out = v;
    return out;
}

/* The two ``current chunk`` getters: stash a fixed descriptor address in a local through the setter,
 * then hand the local back (the setter is opaque, so the reload is real). */
extern "C" u32 fn_8005DC30(void)
{
    void *value;

    fn_8005DC60(&value, lbl_8056F500);
    return (u32)value;
}

extern "C" u32 fn_8005DCA0(void)
{
    void *value;

    fn_8005DCD0(&value, lbl_8056F538);
    return (u32)value;
}

/* The two-word copy pair the character-animation constructor uses. */
extern "C" void* fn_8005D0CC(u32* dst, u32* src)
{
    fn_8005D118(dst, src);
    fn_8005D114(dst + 1, src + 1);
    return dst;
}

extern "C" void fn_800600B8(u32* p, u32 v)
{
    p[1] = v;
}

extern "C" u32 fn_800600C0(u32* p)
{
    return p[1];
}

extern "C" u32 fn_800604CC(u32 a, u32 b)
{
    (void)a;
    return b;
}

extern "C" void fn_80060FEC(void)
{
}

extern "C" u32 fn_80061920(u32* p)
{
    return *p;
}

extern "C" u8* fn_80061928(void)
{
    return lbl_8058BF14;
}

extern "C" void fn_80061B30(u32* dst, u32* src)
{
    *dst = *src;
}

extern "C" f32 fn_80062024(f32* p)
{
    return *p;
}

extern "C" void fn_800621DC(f32* p, f32 v)
{
    p[1] = v;
}

extern "C" f32 fn_80062300(f32* p)
{
    return p[1];
}

/* 0x800626BC (0x8): stores the block address. */
NW4R_G3D_RESCOMMON_CTOR(nw4r::g3d::ResNameData)

/* untyped: opaque handle - the dictionary block */
extern "C" void* fn_800628A4(void* self)
{
    return *(void**)self;
}

extern "C" u8* fn_800628AC(void)
{
    return lbl_80791150;
}

/* 0x80062978 (0x8): stores the block address. */
NW4R_G3D_RESCOMMON_CTOR(nw4r::g3d::ResDicData)

extern "C" void fn_80062D88(u32* dst, u32* src)
{
    *dst = *src;
}

/* --- assignment helpers that return the object (nw4r's fluent setters) ------------------------------ */

/* 0x8006268C (0x30): wraps `pData`. */
/* untyped: opaque handle - the name block */
nw4r::g3d::ResName::ResName(void* pData) : ResCommon<ResNameData>(pData) {}

extern "C" void* fn_80061B00(void* self, u32* src)
{
    fn_80061B30((u32*)self, src);
    return self;
}

extern "C" void* fn_80062D58(void* self, u32* src)
{
    fn_80062D88((u32*)self, src);
    return self;
}

extern "C" void* fn_8005D2C0(void* self, u32* src)
{
    fn_8005D2F0((u32*)self, src);
    return self;
}

/* --- more one-instruction accessors ----------------------------------------------------------------- */

extern "C" u32 fn_80061934(u32* p)
{
    return *p != 0;
}

/* untyped: opaque handle - the dictionary block */
extern "C" u32 fn_800628B4(void* self)
{
    return *(u32*)self != 0;
}

/* Returns the word at +0 plus `offset`, or 0 when `offset` is 0 (nw4r's null-safe offset helper). */
extern "C" u32 fn_80062824(u32* p, u32 offset)
{
    u32 base = *p;

    if (offset != 0) {
        return base + offset;
    }
    return 0;
}

/* The align-up helper: (~(align - 1)) & (offset + align - 1). */
extern "C" u32 fn_800604DC(u32 offset, u32 align)
{
    u32 end = offset + align;

    return ~(align - 1) & (end - 1);
}

/* nw4r's four-float vector setter. */
extern "C" void dVector4Set(f32* dst, f32 x, f32 y, f32 z, f32 w)
{
    dst[0] = x;
    dst[1] = y;
    dst[2] = z;
    dst[3] = w;
}

/* --- forwarders into the SDK allocator / matrix helpers --------------------------------------------- */

extern "C" void* fn_80060F70(void* self, u32 a, u32 b)
{
    fn_804C6F40(a, b, self);
    return self;
}

extern "C" void* fn_80060FAC(void* self, u32 a)
{
    fn_804C6D70(self, mtx34_const_ptr(a));
    return self;
}

/* The animated-float advance: offset[0] += offset[1] * scale, then the virtual step. */
extern "C" void fn_8006244C(f32* p)
{
    f32 scale = lbl_807911E8;
    f32 value = p[1] * scale;

    fn_80061EC4(p, p[0] + value);
}

/* --- nw4r's ``fn_800626F4``-returns-this pair ------------------------------------------------------- */

extern "C" u32 fn_800626F4(u32 value)
{
    return value;
}

extern "C" u32 fn_800626C4(u32 a, u32 b)
{
    return b + fn_800626F4(a);
}

/* --- forwarders into nw4r math ---------------------------------------------------------------------- */

/* The exp forwarder (nw4r's inline `math::FExp`): a tail call with the argument untouched. */
extern "C" f32 fn_80060F6C(f32 x)
{
    return nw4r::math::detail::FExp(x);
}

/* 0x80062914 (0x64): wraps `pData`, asserting its alignment. */
#pragma peephole off
/* untyped: opaque handle - the dictionary block */
nw4r::g3d::ResDic::ResDic(void* pData) : ResCommon<ResDicData>(pData) {
    if ((u32)pData & 0x3) {
        nw4r::db::Panic((const char*)anmchr_resdic_ac_file_ctor, 84, (const char*)anmchr_resdic_align_assert_msg);
    }
}
#pragma peephole on

/* 0x80062750 (0xD4): returns the `idx`-th entry's data, asserting the index is in range. */
/* untyped: opaque handle - the entry's data block */
void* nw4r::g3d::ResDic::operator[](int idx) const {
    if (fn_800628B4((u32*)this)) {
        bool inRange = false;
        if (idx >= 0 && idx <= (s32)ref().numData - 1) {
            inRange = true;
        }
        if (!inRange) {
            nw4r::db::Panic((const char*)anmchr_resdic_ac_file, 42, (const char*)anmchr_resdic_idx_bounds_msg, idx, 0,
                            ref().numData - 1);
        }
        return (void*)fn_80062824((u32*)this, ref().entry[idx + 1].ofsData);
    }
    return NULL;
}

/* --------------------------------------------------------------------------------------------- *
 * The pointer-holder accessors (0x80063888-0x80063E60): a local pointer is filled in through one of
 * the two store helpers and the destination address is returned.
 * --------------------------------------------------------------------------------------------- */

/* Stores `v` through `out` and hands `out` back so the caller keeps its address in r3. */
extern "C" const u8 **type_obj_set_name(const u8 **out, const u8 *v)
{
    *out = v;
    return out;
}

extern "C" u32 fn_80063888(void)
{
    const u8 *local;
    return (u32)*type_obj_set_name(&local, anm_typename_AnmObj);
}

extern "C" u32 fn_800639A0(void)
{
    const u8 *local;
    return (u32)*type_obj_set_name(&local, anm_typename_G3dObj);
}

extern "C" u32 fn_80063A20(u32 *p)
{
    return *p;
}

extern "C" u32 fn_80063A28(void)
{
    const u8 *local;
    return (u32)*type_obj_set_name(&local, anm_typename_AnmObj);
}

extern "C" u32 fn_80063AC4(void)
{
    void *local;
    return (u32)*fn_8005DC60(&local, lbl_8056F500);
}

extern "C" void **fn_80063B24(void **out, void *v)
{
    *out = v;
    return out;
}

extern "C" u32 fn_80063AF4(void)
{
    void *local;
    return (u32)*fn_80063B24(&local, anm_typename_AnmObjChrNode);
}

extern "C" u32 fn_80063BD0(void)
{
    void *local;
    return (u32)*fn_80063B24(&local, anm_typename_AnmObjChrNode);
}

extern "C" void **fn_80063C8C(void **out, void *v)
{
    *out = v;
    return out;
}

extern "C" u32 fn_80063C5C(void)
{
    void *local;
    return (u32)*fn_80063C8C(&local, anm_typename_AnmObjChrBlend);
}

extern "C" u32 fn_80063D38(void)
{
    void *local;
    return (u32)*fn_80063C8C(&local, anm_typename_AnmObjChrBlend);
}

extern "C" u32 fn_80063E30(void)
{
    void *local;
    return (u32)*fn_8005DCD0(&local, lbl_8056F538);
}

/* --------------------------------------------------------------------------------------------- *
 * The vtable-dispatch wrappers (0x800638C0/0x80063B2C/0x80063C94): the object's word 0 is its vtable,
 * the entry at +0x14 runs with the object as its argument and a word is read back through the
 * `fn_8005DC24` helper.
 * --------------------------------------------------------------------------------------------- */

extern "C" u32 fn_800638C0(void *p)
{
    G3dObj *obj = (G3dObj *)p;
    u32 tmp = obj->vt->method_0x14(p);
    return fn_8005DC24(&tmp);
}

extern "C" u32 fn_80063B2C(void *p)
{
    G3dObj *obj = (G3dObj *)p;
    u32 tmp = obj->vt->method_0x14(p);
    return fn_8005DC24(&tmp);
}

extern "C" u32 fn_80063C94(void *p)
{
    G3dObj *obj = (G3dObj *)p;
    u32 tmp = obj->vt->method_0x14(p);
    return fn_8005DC24(&tmp);
}

/* The two equality readers: compare the words reached through each argument. */
extern "C" u32 fn_800639D0(u32 **a, u32 **b)
{
    return fn_80063A20((u32 *)b) == fn_80063A20((u32 *)a);
}

extern "C" u32 fn_80063964(void *self, u32 *other)
{
    u32 local = fn_800639A0();
    return fn_800639D0((u32 **)other, (u32 **)&local);
}

/* The five list-insert steps (0x800638F8-0x80063DC4): resolve a type name, compare the caller's key
 * against it through fn_800639D0, and on a miss run the previous step's insertion with a copy of the
 * key word.  They chain in address order. */
extern "C" u32 fn_800638F8(void *self, u32 *other)
{
    u32 res = fn_80063A28();
    if (fn_800639D0((u32 **)other, (u32 **)&res))
        return 1;
    u32 key = *other;
    return fn_80063964(self, &key);
}

extern "C" u32 fn_80063A58(void *self, u32 *other)
{
    u32 res = fn_80063AC4();
    if (fn_800639D0((u32 **)other, (u32 **)&res))
        return 1;
    u32 key = *other;
    return fn_800638F8(self, &key);
}

extern "C" u32 fn_80063B64(void *self, u32 *other)
{
    u32 res = fn_80063BD0();
    if (fn_800639D0((u32 **)other, (u32 **)&res))
        return 1;
    u32 key = *other;
    return fn_80063A58(self, &key);
}

extern "C" u32 fn_80063CCC(void *self, u32 *other)
{
    u32 res = fn_80063D38();
    if (fn_800639D0((u32 **)other, (u32 **)&res))
        return 1;
    u32 key = *other;
    return fn_80063B64(self, &key);
}

extern "C" u32 fn_80063DC4(void *self, u32 *other)
{
    u32 res = fn_80063E30();
    if (fn_800639D0((u32 **)other, (u32 **)&res))
        return 1;
    u32 key = *other;
    return fn_80063A58(self, &key);
}

/* The two deleting destructors (0x80063C00/0x80063D68): clear the base, and free only when the signed
 * flag is positive; the object is handed back either way. */
extern "C" void *fn_80063C00(void *self, s16 flags)
{
    if (self != 0) {
        dtor_8005E5E8(self, 0);
        if (flags > 0)
            fn_8005D3E0(self);
    }
    return self;
}

extern "C" void *fn_80063D68(void *self, s16 flags)
{
    if (self != 0) {
        dtor_8005E58C(self, 0);
        if (flags > 0)
            fn_8005D3E0(self);
    }
    return self;
}
