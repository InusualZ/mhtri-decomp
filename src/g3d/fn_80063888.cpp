/*
 * nw4r g3d animation-object cluster - `.text` 0x80063888-0x800680A8 (164 functions, 18464 B).
 *
 * Registered once, at its final home (docs/plan.md 12), from the pooled proposal `80063888`.
 *
 * Name (evidence class 4, "nothing supports a single name"): the run is a maximal unclaimed run that
 * spans more than one original translation unit, so no one `__FILE__` string names it.  The discovery's
 * own language probe (`tools/units/attribution-queue.json`) reports
 * `sources: [g3d_anmclr.cpp, g3d_anmobj.cpp], conflict: true`; the region's `.rodata` fragment
 * (0x8056F500-0x8056F678) opens on the g3d animation type-name table (`AnmObj`, `AnmObjChr`,
 * `AnmObjChrNode`, `AnmObjChrBlend`, `AnmScn`, ...) and the first function that cites a `__FILE__` string
 * (`fn_80063E60`) cites `g3d_anmclr.cpp` (0x8058C118), while the tail cites `g3d_anmscn.cpp`
 * (0x8058C288, `fn_800649CC` onward) - two source files, with an internal seam near 0x800649CC.  The
 * dominant TU name `g3d_anmscn.cpp` already has a provisional registered home (`g3d/g3d_anmscn.cpp`,
 * 0x800680A8-0x800680CC), so the cluster keeps the map's own stem and the module is `g3d` (every covered
 * TU is nw4r g3d).  Both seams are recorded in the outbox for the batch re-split.
 *
 * Language: C++ (the discovery's `langcheck`: cxx true, confidence high - the run defines the mangled
 * `nw4r::g3d::PlayPolicy::Onetime/Loop` and calls the mangled `nw4r::db::Panic`).  The map's
 * `fn_XXXXXXXX` stems are the map's placeholders, so those bodies are defined `extern "C"` to keep the
 * bare symbol (playbook 48).  The one real C++ free function of the cluster, `fn_80066C8C`, keeps C++
 * linkage (its map name is the mangling `fn_80066C8C__FPv`) and is declared in the owner header.
 *
 * Seam: unproven.  The left edge 0x80063888 is where `g3d_anmchr.cpp`'s last function ends
 * (`fn_800636B0` + 0x1D8); the right edge 0x800680A8 is the start of the registered
 * `g3d/g3d_anmscn.cpp` and of the next proposal (`800680CC`).  This unit was registered with the
 * proposal's exact extent so it can be measured; the re-cut rides the batch.
 *
 * Sections: .text 0x80063888-0x800680A8, extab 0x800076C4-0x80007B14,
 * extabindex 0x8001FB00-0x8001FF5C.
 *
 * Naming note: the symbol map has only `fn_XXXXXXXX` for this range (checked with
 * `python tools/symbols/dumpmap.py lookup <addr>`: every unnamed entry is a bare `zz_XXXXXXXX_`
 * placeholder in the runtime dump too), so there is no real name to recover for those functions and
 * stylelint's rule 7 refuses the landing without this line.  The two name-map exceptions
 * (`PlayPolicy_Onetime/Loop`) are written through their namespace owner (rule 9) when reconstructed.
 *
 * The cross-unit declarations (rule 2) live in `include/g3d/fn_80063888.h`; four other units call into
 * this cluster and include that header.
 */

#include "types.h"
#include "unsplit/g3d.h"      /* unsplit g3d neighbours (rule 2) */
#include "g3d/fn_80075DCC.h" /* fn_8007A5E4/fn_8007A5A8/fn_8007A724, owned by g3d/fn_80075DCC.cpp (rule 2) */
#include "g3d/fn_80063888.h"
#include "mh3_pad.h"        /* VEC3_ctor, owned by mh3_pad.cpp (rule 2) */

/* The `g3d`-band helpers owned by unsplit units (their address band - bracketed by the `main` unit
 * `fn_8004C9A0.cpp` and this one - names no single module, so they stay local declarations). */
extern "C" void **fn_8005DC60(void **out, void *name);
extern "C" void **fn_8005DCD0(void **out, void *name);
extern "C" void fn_8006411C(u32 *dst, const u32 *src);
extern "C" void *fn_80064754(void *self);
extern "C" void fn_8006602C(u32 *dst, const u32 *src);
extern "C" void fn_80066D48(u32 *dst, const u32 *src);
extern "C" void fn_80066F94(u32 *dst, const u32 *src);
extern "C" void fn_800670C8(u32 *dst, const u32 *src);
extern "C" u32 fn_800638F8(void *self, u32 *other);
extern "C" u32 fn_80063A58(void *self, u32 *other);
extern "C" u32 fn_80063B64(void *self, u32 *other);
extern "C" void dtor_8005E5E8(void *self, s32 flag);
extern "C" void dtor_8005E58C(void *self, s32 flag);
extern "C" void fn_8005D3E0(void *self);

/* A three-float vector record (the cluster's `.text` copies it field by field, so the struct is the
 * explicit-float-copy shape, not a word copy). */
typedef struct {
    /* +0x00 */ f32 x;
    /* +0x04 */ f32 y;
    /* +0x08 */ f32 z;
} Vec3f; /* size: 0xC */

/* The cluster's `.rodata` animation type-name records (0x8056F500-0x8056F678): a length word followed by
 * the NUL-terminated name.  Referenced by address only. */
extern u8 lbl_8056F500[];
extern u8 lbl_8056F510[];
extern u8 lbl_8056F524[];
extern u8 lbl_8056F538[];
extern u8 lbl_8056F550[];
extern u8 lbl_8056F568[];
extern u8 lbl_8056F578[];
extern u8 lbl_8056F588[];
extern u8 lbl_8056F668[];

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

/* --------------------------------------------------------------------------------------------- *
 * The pointer-holder accessors (0x80063888-0x80063E60): a local pointer is filled in through one of
 * the two store helpers and the destination address is returned.
 * --------------------------------------------------------------------------------------------- */

/* Stores `v` through `out` and hands `out` back so the caller keeps its address in r3. */
extern "C" void **fn_800638B8(void **out, void *v)
{
    *out = v;
    return out;
}

extern "C" u32 fn_80063888(void)
{
    void *local;
    return (u32)*fn_800638B8(&local, lbl_8056F568);
}

extern "C" u32 fn_800639A0(void)
{
    void *local;
    return (u32)*fn_800638B8(&local, lbl_8056F668);
}

extern "C" u32 fn_80063A20(u32 *p)
{
    return *p;
}

extern "C" u32 fn_80063A28(void)
{
    void *local;
    return (u32)*fn_800638B8(&local, lbl_8056F568);
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
    return (u32)*fn_80063B24(&local, lbl_8056F510);
}

extern "C" u32 fn_80063BD0(void)
{
    void *local;
    return (u32)*fn_80063B24(&local, lbl_8056F510);
}

extern "C" void **fn_80063C8C(void **out, void *v)
{
    *out = v;
    return out;
}

extern "C" u32 fn_80063C5C(void)
{
    void *local;
    return (u32)*fn_80063C8C(&local, lbl_8056F524);
}

extern "C" u32 fn_80063D38(void)
{
    void *local;
    return (u32)*fn_80063C8C(&local, lbl_8056F524);
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

extern "C" u32 fn_8005DC24(u32 *p);

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

/* --------------------------------------------------------------------------------------------- *
 * More vtable-dispatch wrappers and the ownership-copy helpers (0x800640EC-0x800657C4).
 * --------------------------------------------------------------------------------------------- */

extern "C" u32 fn_8006553C(void *p)
{
    G3dObj *obj = (G3dObj *)p;
    u32 tmp = obj->vt->method_0x14(p);
    return fn_8005DC24(&tmp);
}

extern "C" u32 fn_800655A4(void *p)
{
    G3dObj *obj = (G3dObj *)p;
    u32 tmp = obj->vt->method_0x14(p);
    return fn_8005DC24(&tmp);
}

/* Copies one word through the `fn_8006411C` helper and hands the destination back. */
extern "C" void *fn_800640EC(void *dst, void *src)
{
    fn_8006411C((u32 *)dst, (const u32 *)src);
    return dst;
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

extern u8 lbl_8058C898[];
extern "C" void fn_8005D428(void *self, void *a, u32 flag);

extern "C" void *fn_800657C4(void *self, void *a)
{
    fn_8005D428(self, a, 0);
    ((G3dVtObj *)self)->vt = lbl_8058C898;
    return self;
}

/* --------------------------------------------------------------------------------------------- *
 * The small word accessors (0x80063FC8-0x80064898).
 * --------------------------------------------------------------------------------------------- */

extern "C" void fn_80063FC8(void **out, void *v)
{
    *out = v;
}

extern "C" u32 fn_80064034(u32 *p)
{
    return *p;
}

extern "C" u32 fn_800640E4(u32 *p)
{
    return *p;
}

extern "C" void fn_8006411C(u32 *dst, const u32 *src)
{
    *dst = *src;
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

extern "C" u32 fn_80064868(void)
{
    void *local;
    return (u32)*fn_8005DCD0(&local, lbl_8056F550);
}

extern "C" void fn_80064898(void **out, void *v)
{
    *out = v;
}

/* --------------------------------------------------------------------------------------------- *
 * The float and flag helpers (0x8006497C-0x80065ED8).
 * --------------------------------------------------------------------------------------------- */

extern "C" f32 fn_8006497C(f32 frame, f32 looped, f32 oneTime)
{
    return (f32)__fsel(frame, looped, oneTime);
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
    void *local;
    return (u32)*fn_800638B8(&local, lbl_8056F578);
}

extern "C" u32 fn_800655DC(void)
{
    void *local;
    return (u32)*fn_8005DC60(&local, lbl_8056F588);
}

extern "C" void fn_80065804(void)
{
}

/* --------------------------------------------------------------------------------------------- *
 * The checked `ResAnmScn` config accessors (0x80065828-0x8006605C): each resolves the object through
 * `fn_8006584C` (the checked getter that reads *self and panics out of `g3d_resanmscn_ac.h`) and reads
 * one field of the resolved config record.
 * --------------------------------------------------------------------------------------------- */

/* `ResAnmScnConfig` moved to include/g3d/fn_80063888.h (rule 1) when g3d/g3d_resanmlight.cpp became
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
 * include/g3d/fn_80063888.h (rules 1 and 2) when g3d/g3d_resanmlight.cpp became the second consumer. */

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

extern "C" ResAnmChrData *fn_80064080(void *p);

extern "C" u32 fn_8006405C(void *p)
{
    return fn_80064080(p)->field_0x0C;
}

/* --------------------------------------------------------------------------------------------- *
 * The composite teardown destructors (0x80067B5C/0x80067E70/0x80067EB4): construct-or-clear each
 * sub-record of the object through `VEC3_ctor`/`fn_80064834`, then hand the object back.
 * --------------------------------------------------------------------------------------------- */

typedef struct {
    /* +0x00 */ u32 head_0x00;
    /* +0x04 */ Vec3f vec_0x04;
    /* +0x10 */ Vec3f vec_0x10;
    /* +0x1C */ Vec3f vec_0x1C;
} G3dTripleVecObj; /* size: 0x28 */

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

extern "C" void *fn_80067E70(void *self)
{
    G3dTripleVecObj *obj = (G3dTripleVecObj *)self;
    VEC3_ctor(&obj->vec_0x04);
    VEC3_ctor(&obj->vec_0x10);
    VEC3_ctor(&obj->vec_0x1C);
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

extern "C" void *fn_80067E54(void *out, void *in)
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
 * The label getters and flag/word accessors recovered from the per-function target objects
 * (0x8006403C-0x80066EEC).
 * --------------------------------------------------------------------------------------------- */

extern "C" u32 fn_8006403C(void)
{
    return (u32)lbl_8058C208;
}

extern "C" u32 fn_80064814(void)
{
    return (u32)lbl_8058C1D0;
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

extern "C" s32 fn_80064820(void *p)
{
    return *(u32 *)p != 0;
}

extern "C" s32 fn_800658C4(u32 *p)
{
    return *p != 0;
}

extern "C" s32 fn_80066D04(u32 *p)
{
    return *p != 0;
}

extern "C" void fn_800648A0(void **out, void *v)
{
    *out = v;
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

extern "C" void fn_80064988(G3dFlagWord *self, u32 bits, s32 set)
{
    if (set != 0)
        self->field_0x0C |= bits;
    else
        self->field_0x0C &= ~bits;
}

/* `fn_800649B4` is declared in `include/g3d/fn_80063888.h` with the object as an opaque `void*` (the
 * ScnMdl pointer `g3d/g3d_scnmdl.cpp`'s fn_8007EA08 passes unchanged, per the target object's
 * `li r4,4; b` tail call) and this file's definition has to spell the same C-linkage parameter type
 * or the two declarations collide as illegal overloading.  The body casts to the local
 * `G3dFlagWord` view, which is the record the target's body reads at +0xC. */
extern "C" s32 fn_800649B4(void *pSelf, u32 bits)
{
    G3dFlagWord *self = (G3dFlagWord *)pSelf;

    return (self->field_0x0C & bits) != 0;
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
extern "C" void fn_80463F10(void *p);

extern "C" void fn_80064984(void *p)
{
    fn_80463F10(p);
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

extern "C" u32 fn_800647A8(void *p)
{
    if (fn_80064820(p) == 0)
        nw4r::db::Panic((const char *)lbl_8058C1F8, 0x1d1, (const char *)lbl_8058C1DC,
                        (const char *)fn_80064814(), "ref");
    return fn_8006480C((u32 *)p);
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
            fn_8005D3E0(self);
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
