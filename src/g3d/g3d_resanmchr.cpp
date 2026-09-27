/*
 * nw4r g3d: g3d_resanmchr.cpp - the `ResAnmChr` character-animation channel evaluators, `.text`
 * 0x8008A664-0x8008F6E8 (101 functions, 0x5084 B).
 *
 * Naming - which evidence class decided it.  Class 1 decides: the region's own `.data` pool holds the
 * bare source-file name `g3d_resanmchr.cpp` (lbl_80590010 at 0x80590010, the file argument of every
 * `nw4r::db::Panic` assert in the range, first at fn_8008A664 line 0x298) with the value-type format
 * strings and the `g3d_resanmchr_ac.h` inlined-assert header beside it.  `langcheck` says C++ (the
 * `.cpp` name and the `Panic__Q24nw4r2dbFPCciPCce` relocation), so the unit is
 * `src/g3d/g3d_resanmchr.cpp` in the existing g3d lib (Wii/1.3, cflags_g3d), exactly where its link
 * neighbours sit.  Class 2 FAILS: `dumpmap.py lookup 0x8008A664` answers `zz_008a664_`, a placeholder,
 * not a name.
 *
 * What the unit is.  Four animation channels share the shape - a channel record (+0x00 frame count,
 * +0x04 frame rate) with a key array whose stride is 0xC (float), 0x6 (s16) or 0x4 (u8) - and each has
 * a cubic-Hermite evaluator behind an offset/validity guard.  `fn_8008A664`/`fn_8008C038`/`fn_8008CF2C`
 * read one node's 3-component value (scale/rotation/translation) through the channel table selected by
 * the node-data flags.
 *
 * Section claim: `.text` 0x8008A664-0x8008F6E8, `extab` 0x80008F10-0x80009100 (62 records),
 * `extabindex` 0x80021A74-0x80021D5C (62 entries).  The boundaries are the functions before
 * (fn_8008A644) and after (fn_8008F6E8).
 *
 * Naming note: the symbol map has only `fn_XXXXXXXX` for this range (checked with
 * `python tools/symbols/dumpmap.py lookup 0x8008A664`, which answers the `zz_008a664_` placeholder,
 * and with config/RMHE08/symbols.txt, whose every `.text` entry in 0x8008A664..0x8008F6E8 is a bare
 * `fn_XXXXXXXX`).  The map's stems stand and are used as the identifiers.
 *
 * Measurement path: the unit is registered here for the first time, so MAIN has no split object for
 * the range; the source compiles with the g3d lib's real command line and each symbol is scored with
 * objdiff `report generate` against the retired per-range objects under build/RMHE08/obj/
 * (`auto_fn_8008A664_text.o`, `auto_03_8008A938_text.o`, ...).
 *
 * Reconstruction status: 98 of the 101 functions are reconstructed (estimated unit match ~85 %;
 * 75 byte-identical).  The measured per-symbol scores are recorded in
 * `.pi/outbox/8008a664-fn-8008a664-7a76.json`.  Still missing: `fn_8008C038` (0x8008C038, 0x44C B),
 * `fn_8008CF2C` (0x8008CF2C, 0x280 B) and `fn_8008D5FC` (0x8008D5FC, 0x3BC B) - the three per-node
 * dispatchers.  Residuals (recorded, not worked around) are listed at the bottom of this file.
 */

#include "types.h"
#include "nw4r/g3d/res_common.h" /* IS_VALID_PTR (rule 1) */
#include "unsplit/g3d.h"         /* fn_8007100C, fn_80082F18 (rule 2) */
#include "g3d/g3d_anmchr.h"      /* fn_800610AC, fn_800618BC, fn_800628C8 (rule 2) */
#include "g3d/fn_80063888.h"     /* fn_8006497C (rule 2) */
#include "fn_8004CAD8.h"         /* fn_800501E4, fn_800504D4, fn_80050BC0 (rule 2) */
#include "mh3_pad.h"             /* copyVec3, setVec3, VEC3_ctor (rule 2) */

/* fp_contract stays ON (cflags_g3d): the target's `fn_8008AED0` uses fused fmadds/fmsubs, so this unit
 * does not carry the `#pragma fp_contract off` its `g3d/g3d_resanm.c` sibling does.  `peephole off` is
 * file-scoped: the target keeps the split `clrlwi`+`slwi`/`cmpwi` forms (no record-form instruction),
 * which the peephole would fuse (the same finding as g3d_resanm's `fn_8008C7F0`). */
#pragma peephole off

namespace nw4r {
namespace db {

/* `Panic(const char* pFile, int line, const char* pFmt, ...)` - the trailing `e` in the map's mangling
 * is MWCC's vararg marker.  Declared in the namespace (C++ linkage), never as the mangled spelling
 * (rule 9). */
void Panic(const char* pFile, int line, const char* pFmt, ...);

} // namespace db
} // namespace nw4r

/* The panic file/format strings the target references as map symbols.  They are declared here as
 * externs rather than literals: MWCC's `-str reuse` pools repeated literals, while the target loads
 * each assert string with its own `lis`/`addi`.  They are unsplit `.data` owned by no registered unit,
 * so this is their declaration site. */
extern const char lbl_80590010[]; /* "g3d_resanmchr.cpp" */
extern const char lbl_80590028[]; /* the ScaleType format string */
extern const char lbl_80590088[]; /* the "not valid pointer" format string */
extern const char lbl_805900BC[]; /* the position-bounds format string */
extern const char lbl_80590100[]; /* the frame-bounds format string */
extern const char lbl_8059013C[]; /* the 7-value ScaleType format string */
extern const char lbl_805901A0[]; /* the rotation ScaleType format string */
extern const char lbl_80590200[]; /* the first pointer-error format string of the walkers */
extern const char lbl_805902E4[]; /* the second pointer-error format string (fn_8008DA10) */
extern const char lbl_8059031C[]; /* the second pointer-error format string (fn_8008DF8C/...) */
extern const char lbl_80590350[]; /* the second pointer-error format string (fn_8008E1C0/F148) */
extern const char lbl_80590388[]; /* the second pointer-error format string (fn_8008E408) */
extern const char lbl_805903BC[]; /* the second pointer-error format string (fn_8008E698) */

/* The float constants the target reads from `.sdata2`. */
extern const f32 lbl_80795ED0; /* 0.0f */
extern const f32 lbl_80795EE8; /* 1.0f */
extern const f32 lbl_80795EEC; /* 2.0f */
extern const f32 lbl_80795EF0; /* 3.0f */
extern const f32 lbl_80795EF4; /* the cast helper's scale */
extern const f32 lbl_80795EF8;
extern const f32 lbl_80795EFC;
extern const f32 lbl_80795F00;

/*
 * The nw4r g3d animation resources.
 *
 * A channel record is a frame count, a frame rate and a key array; four value widths share the shape
 * (the variants differ only in the key stride and the evaluator's value type).  The key structs are the
 * ones the stride and the loads evidence: 0xC for the float channel, 0x6 for the short channel and
 * 0x4 for the byte channel.
 */
typedef struct {
    /* +0x00 */ f32 frame;
    /* +0x04 */ f32 value;
    /* +0x08 */ f32 tangent;
} ResAnmChrKeyF32; /* size: 0xC */

typedef struct {
    /* +0x00 */ f32 frame;
    /* +0x04 */ s16 value;
    /* +0x06 */ s16 tangent;
} ResAnmChrKeyS16; /* size: 0x8 - the target advances keys by 0x6, so the tail of each record is
                        the next record's frame low half; the stride evidence is the 0x6 advance */

typedef struct {
    /* +0x00 */ f32 frame;
} ResAnmChrKeyU8; /* size: 0x4 - the target advances by 0x4; the packed value sits after the
                      frame's first word (see the residual list) */

/* A float channel: a key count, a frame rate and the key array. */
typedef struct {
    /* +0x00 */ u16 count;
    /* +0x02 */ u16 pad_0x02;
    /* +0x04 */ f32 rate;
    /* +0x08 */ ResAnmChrKeyF32 keys[];
} ResAnmChrChannelF32; /* size: 0x8, keys follow */

/* A short channel: the key array starts at +0x08. */
typedef struct {
    /* +0x00 */ u16 count;
    /* +0x02 */ u16 pad_0x02;
    /* +0x04 */ f32 rate;
    /* +0x08 */ u8 keys[];
} ResAnmChrChannelS16; /* size: 0x8, keys follow */

/* A byte channel: the key array starts at +0x08. */
typedef struct {
    /* +0x00 */ u16 count;
    /* +0x02 */ u16 pad_0x02;
    /* +0x04 */ f32 rate;
    /* +0x08 */ u8 keys[];
} ResAnmChrChannelU8; /* size: 0x8, keys follow */

/* A channel with a float scale/offset pair and a u16 value array at +0x8 (the packed-angle variant). */
typedef struct {
    /* +0x00 */ f32 scale;
    /* +0x04 */ f32 offset;
    /* +0x08 */ u16 values[];
} ResAnmChrU16Channel; /* size: 0x8, values follow */

/* A channel with a float scale/offset pair and a u8 value array at +0x8. */
typedef struct {
    /* +0x00 */ f32 scale;
    /* +0x04 */ f32 offset;
    /* +0x08 */ u8 values[];
} ResAnmChrU8Channel; /* size: 0x8, values follow */

/* The per-node animation data: a type-select word at +0x0 and the option/ScaleType flags at +0x4. */
typedef struct {
    /* +0x00 */ u32 type;
    /* +0x04 */ u32 flags;
} ResAnmChrNodeData; /* size: 0x8, lower bound (only these two words are evidenced) */

/* The `AnmObjChr`/`ResAnmChr` runtime object the frame/dirty-state walkers operate on.  +0x00 is the
 * state/flags word, +0x04 a 3-float scale, +0x10 a 3-float position and +0x1C a 12-float (3x4) matrix
 * whose last column (+0x28/+0x38/+0x48) holds the evaluated row.  Only the words the walkers touch are
 * evidenced, so the size is a lower bound. */
typedef struct {
    /* +0x00 */ u32 flags;
    /* +0x04 */ f32 scale[3];
    /* +0x10 */ f32 pos[3];
    /* +0x1C */ f32 mat[12];
} ResAnmChrObj; /* size: 0x4C - lower bound, an approximation

/* The 3-component record the walkers copy is `Vec3f` as `g3d/fn_80063888.cpp` already defines it
 * (rule 1: a shared type is defined once, in the lexicographically first owner); this unit reads the
 * components through `f32*` cursors instead of redefining it. */

/* ================================================================================================== */
/* Definitions                                                                                         */
/* ================================================================================================== */

extern "C" {

/* This unit's own bodies, forward-declared so the helpers can call them in address order (rule 2: the
 * owner of these symbols is this file). */
f32 fn_8008A93C(u32* self, u32* p, s32 isConst, f32 frame);
f32 fn_8008B0B0(u32* self, u32* p, s32 isConst, f32 frame);
f32 fn_8008B80C(u32* self, u32* p, s32 isConst, f32 frame);
f32 fn_8008AA8C(u32 self, f32 frame);
f32 fn_8008B200(u32 self, f32 frame);
f32 fn_8008B95C(u32 self, f32 frame);
f32 fn_8008AFB0(f32 frame);
void fn_8008AF84(u32* p, u32 unused);
void fn_8008AF94(u32* p, u32 unused);
void fn_8008B6D4(u32* p, u32 unused);
void fn_8008B6E4(u32* p, u32 unused);
void fn_8008BE5C(u32* p, u32 unused);
void fn_8008BE6C(u32* p, u32 unused);
f32 fn_8008B650(u32* iter);
f32 fn_8008BF08(u32* iter, const f32* self);
f32 fn_8008DF64(f32 a, f32 b);
f32 fn_8008F6C4(u32 self, s32 index);
f32 fn_800501E4(u16 count);
f32 fn_80050BC0(f32 value);
s16 fn_8008B700(f32 value);
u8 fn_8008BE88(f32 value);
u8 fn_8008BE7C(u32* p);
u32 fn_8008C000(u32 self, u32 index);
u32* fn_8008BFC4(u32* out, u32* src);
f32 fn_8008BF5C(u32* iter);
f32 fn_8008BDAC(u32* iter);
u32 fn_8008BDEC(u32* iter, s32 index);
void* fn_8008C038(void* out, void* self, u32 arg1, u32 arg2, const void* keys, f32 frame);
void* fn_8008CF2C(void* out, const void* a, const void* b, f32 frame);
f32 fn_8008C4A0(u32 self, u32* p, u16 index, s32 isConst, f32 frame);
f32 fn_8008C808(u32 self, u32* p, u16 index, s32 isConst, f32 frame);
f32 fn_8008CBA0(u32 self, u32* p, u16 index, s32 isConst, f32 frame);
f32 fn_8008C600(u32 self, u16 index, f32 frame);
f32 fn_8008C968(u32 self, u16 index, f32 frame);
f32 fn_8008CD00(u32 self, u16 index, f32 frame);
u32* fn_8008A664(f32* out, u32* self, f32* keys, f32 frame);

/* The unsplit `.text` this unit calls whose address band names no module (rule 2's documented gap:
 * 0x8045B9D8 and the 0x8050xxxx math block). */
s32 fn_8045B9D8(f32 value);
f32 fn_80500F60(void);
void fn_805012C4(f32* out);
void fn_805012E8(f32* out, const void* src);
void fn_8050133C(void* out, const void* src, const f32* v);
void fn_80501434(f32* out, f32 x, f32 y, f32 z);
void fn_80501594(void* out, const void* src);

/* ------------------------------------------------------------------------------------------------ */
/* The float channel's key cursor helpers (0xC-byte stride).                                           */
/* ------------------------------------------------------------------------------------------------ */

/* Stores `v` through the cursor. */
void fn_8008AF7C(u32* p, u32 v)
{
    *p = v;
}

/* Advances one 0xC-byte float key. */
void fn_8008AF84(u32* p, u32 unused)
{
    (void)unused;
    *p += 0xC;
}

/* Retreats one 0xC-byte float key. */
void fn_8008AF94(u32* p, u32 unused)
{
    (void)unused;
    *p -= 0xC;
}

/* The frame of the current float key. */
f32 fn_8008AFA4(u32* p)
{
    return ((ResAnmChrKeyF32*)*p)->frame;
}

/* An identity advance that carries the frame through. */
f32 fn_8008AFB0(f32 frame)
{
    return frame;
}

/* The `ResCommon<T>` store-and-return shape. */
u32* fn_8008AF4C(u32* out, u32 v)
{
    fn_8008AF7C(out, v);
    return out;
}

/* The float key at `index`. */
u32 fn_8008AF14(u32* iter, s32 index)
{
    u32 tmp;

    return *fn_8008AF4C(&tmp, *iter + index * 0xC);
}

/* The float channel's own frame halfword at `index` (+0x8 into each key). */
u32 fn_8008B040(u32 iter, u32 index)
{
    u32 tmp;

    return *fn_8008AF4C(&tmp, iter + index * 0xC + 0x8);
}

/* The value of the current float key. */
f32 fn_8008AFEC(u32* p, u32 self)
{
    (void)self;
    return ((ResAnmChrKeyF32*)*p)->value;
}

/* The frame of the current float key. */
f32 fn_8008AFF8(u32* p)
{
    return ((ResAnmChrKeyF32*)*p)->frame;
}

/* The tangent of the current float key. */
f32 fn_8008AF08(u32* p)
{
    return ((ResAnmChrKeyF32*)*p)->tangent;
}

/* Loads the float at `p`. */
f32 fn_8008AFE4(const f32* p)
{
    return *p;
}

/* Loads the float at `addr` into `out`. */
void fn_8008AFB4(u32 addr, f32* out)
{
    *out = fn_8008AFE4((const f32*)addr);
}

/* Stores `src` through `dst`. */
void fn_8008B034(u32* dst, u32* src)
{
    *dst = *src;
}

/* The `ResCommon<T>` copy-and-return shape. */
u32* fn_8008B004(u32* out, u32* src)
{
    fn_8008B034(out, src);
    return out;
}

/* The identity the resource pointer resolver folds in. */
u32 fn_8008B0A8(u32 self)
{
    return self;
}

/* Resolves a self-relative offset against the resource base. */
u32 fn_8008B078(u32 self, u32 offset)
{
    return offset + fn_8008B0A8(self);
}

/* Tail entry into the float evaluator. */
f32 fn_8008A938(u32* self, u32* p, s32 isConst, f32 frame)
{
    return fn_8008A93C(self, p, isConst, frame);
}

/* Evaluates one float channel key cell at `frame`.  `p` holds either the constant value (`isConst`) or
 * a self-relative offset to the frame data, which resolves against `self`. */
f32 fn_8008A93C(u32* self, u32* p, s32 isConst, f32 frame)
{
    u32 data;
    u32 valid;

    if (isConst) {
        return *(f32*)p;
    }
    data = fn_8008B078((u32)self, *p);
    valid = IS_VALID_PTR(data);
    if (!valid) {
        nw4r::db::Panic(lbl_80590010, 0x1B7, lbl_80590088, data);
    }
    return fn_8008AA8C(data, frame);
}

/* Tail entry into the s16 evaluator. */
f32 fn_8008B0AC(u32* self, u32* p, s32 isConst, f32 frame)
{
    return fn_8008B0B0(self, p, isConst, frame);
}

/* Tail entry into the u8 evaluator. */
f32 fn_8008B808(u32* self, u32* p, s32 isConst, f32 frame)
{
    return fn_8008B80C(self, p, isConst, frame);
}

/* The cubic-Hermite key interpolation.  `rate` is the normalised in-key position, `span` the key
 * length; v0/t0 and v1/t1 are the two keys' values and tangents. */
f32 fn_8008AED0(f32 v0, f32 t0, f32 v1, f32 t1, f32 rate, f32 span)
{
    f32 hm1 = rate - lbl_80795EE8;

    return (span * hm1 * ((hm1 * t0) + (rate * t1))) +
           ((rate * (rate * (((lbl_80795EEC * rate) - lbl_80795EF0) * (v0 - v1)))) + v0);
}

/* ------------------------------------------------------------------------------------------------ */
/* The short channel's key cursor helpers (0x6-byte stride).                                           */
/* ------------------------------------------------------------------------------------------------ */

void fn_8008B6CC(u32* p, u32 v)
{
    *p = v;
}

void fn_8008B6D4(u32* p, u32 unused)
{
    (void)unused;
    *p += 0x6;
}

void fn_8008B6E4(u32* p, u32 unused)
{
    (void)unused;
    *p -= 0x6;
}

s16 fn_8008B6F4(u32* p)
{
    return *(s16*)*p;
}

u32* fn_8008B69C(u32* out, u32 v)
{
    fn_8008B6CC(out, v);
    return out;
}

u32 fn_8008B664(u32* iter, s32 index)
{
    u32 tmp;

    return *fn_8008B69C(&tmp, *iter + index * 0x6);
}

u32 fn_8008B7D0(u32 self, u32 index)
{
    u32 tmp;

    return *fn_8008B69C(&tmp, self + index * 0x6 + 0x10);
}

/* The float at `addr`. */
f32 fn_8008B78C(u32 addr)
{
    return *(const f32*)addr;
}

/* The s16 key's high halfword as a float. */
f32 fn_8008B784(u32* iter)
{
    return fn_8008B78C(*iter);
}

f32 fn_8008B75C(u32 addr)
{
    f32 out;

    fn_8008AFB4(addr, &out);
    return out;
}

/* The current s16 key's angle, scaled and biased by the object's rate/offset. */
f32 fn_8008B71C(u32* iter, const f32* self)
{
    return self[2] * fn_8008B75C(*iter + 2) + self[3];
}

/* Extracts the packed 12-bit angle of `iter` and evaluates it. */
f32 fn_8008BDAC(u32* iter)
{
    s32 v = (s32)(*(u32*)(*iter) & 0xFFF);
    s16 packed = (s16)((v << 20) >> 20);

    return fn_8008B78C((u32)&packed);
}

/* A u16 key value scaled by the channel's rate and biased by its offset. */
f32 fn_8008CB58(const ResAnmChrU16Channel* p, u16 index)
{
    return p->scale * (f32)p->values[index] + p->offset;
}

/* A u8 key value scaled by the channel's rate and biased by its offset. */
f32 fn_8008CEF0(const ResAnmChrU8Channel* p, u16 index)
{
    return p->scale * (f32)p->values[index] + p->offset;
}

/* ------------------------------------------------------------------------------------------------ */
/* The byte channel's key cursor helpers (0x4-byte stride).                                            */
/* ------------------------------------------------------------------------------------------------ */

void fn_8008BE54(u32* p, u32 v)
{
    *p = v;
}

void fn_8008BE5C(u32* p, u32 unused)
{
    (void)unused;
    *p += 0x4;
}

void fn_8008BE6C(u32* p, u32 unused)
{
    (void)unused;
    *p -= 0x4;
}

u8 fn_8008BE7C(u32* p)
{
    return *(u8*)*p;
}

u32* fn_8008BE24(u32* out, u32 v)
{
    fn_8008BE54(out, v);
    return out;
}

u32 fn_8008BDEC(u32* iter, s32 index)
{
    u32 tmp;

    return *fn_8008BE24(&tmp, *iter + index * 0x4);
}

u32 fn_8008C000(u32 self, u32 index)
{
    u32 tmp;

    return *fn_8008BE24(&tmp, self + index * 0x4 + 0x10);
}

void fn_8008BFF4(u32* dst, u32* src)
{
    *dst = *src;
}

u32* fn_8008BFC4(u32* out, u32* src)
{
    fn_8008BFF4(out, src);
    return out;
}

void fn_8008B7C4(u32* dst, u32* src)
{
    *dst = *src;
}

u32* fn_8008B794(u32* out, u32* src)
{
    fn_8008B7C4(out, src);
    return out;
}

/* The float at `addr` for the byte channel. */
f32 fn_8008BFBC(u32 addr)
{
    return *(const f32*)addr;
}

void fn_8008BF8C(u32 addr, f32* out)
{
    *out = fn_8008BFBC(addr);
}

f32 fn_8008BF64(u32 addr)
{
    f32 out;

    fn_8008BF8C(addr, &out);
    return out;
}

f32 fn_8008BF5C(u32* iter)
{
    return fn_8008BF64(*iter);
}

/* Truncates a float to a short by reading the stored word's high halfword. */
s16 fn_8008B704(f32 value)
{
    union {
        f32 f;
        s16 s[2];
    } u;

    u.f = value;
    return u.s[0];
}

s16 fn_8008B700(f32 value)
{
    return fn_8008B704(value);
}

/* Truncates a float to a byte by reading the stored word's high byte. */
u8 fn_8008BEF0(f32 value)
{
    union {
        f32 f;
        u8 b[4];
    } u;

    u.f = value;
    return u.b[0];
}

void fn_8008BEBC(const f32* value, u8* out)
{
    *out = fn_8008BEF0(*value);
}

u8 fn_8008BE8C(f32 value)
{
    u8 out;

    fn_8008BEBC(&value, &out);
    return out;
}

u8 fn_8008BE88(f32 value)
{
    return fn_8008BE8C(value);
}

/* ------------------------------------------------------------------------------------------------ */
/* The record setup / normalise helpers.                                                               */
/* ------------------------------------------------------------------------------------------------ */

/* Scales a 3-float record by a constant and forwards it. */
void fn_8008C484(f32* out, f32 x, f32 y, f32 z)
{
    fn_80501434(out, lbl_80795EF4 * x, lbl_80795EF4 * y, lbl_80795EF4 * z);
}

/* A u16-keyed float table value. */
f32 fn_8008C7F0(u32 self, u16 index)
{
    return reinterpret_cast<const f32*>(self)[index];
}

/* Zeroes the 3-float record at +0x4 and clears the matrix at +0x1C. */
void fn_8008D1AC(ResAnmChrObj* self)
{
    self->scale[0] = lbl_80795EE8;
    self->scale[1] = lbl_80795EE8;
    self->scale[2] = lbl_80795EE8;
    fn_800504D4(self->mat);
}

/* Re-evaluates the record from the source key.  The frame is passed through untouched (the target sets no
 * `f1`, so the original call had an undefined frame argument). */
void fn_8008D2CC(ResAnmChrObj* self, u32 unused, const f32* key, f32 frame)
{
    (void)unused;
    fn_8008A664(self->scale, (u32*)key, (f32*)&key[2], frame);
    fn_800504D4(self->mat);
}

/* The float normaliser. */
f32 fn_8008DF64(f32 a, f32 b)
{
    (void)a;
    (void)b;
    return lbl_80795F00 * fn_80500F60();
}

/* The squared-length helper. */
f32 fn_8008F6C4(u32 self, s32 index)
{
    const f32* p = (const f32*)self + index;

    return fn_80050BC0(p[4] * p[4] + p[0] * p[0] + p[8] * p[8]);
}

/* ------------------------------------------------------------------------------------------------ */
/* The four channel guards: resolve the self-relative data pointer, check it against the Wii memory    */
/* map and hand off to the channel's cubic evaluator.                                                  */
/* ------------------------------------------------------------------------------------------------ */

f32 fn_8008B0B0(u32* self, u32* p, s32 isConst, f32 frame)
{
    u32 data;
    u32 valid;

    if (isConst) {
        return *(f32*)p;
    }
    data = fn_8008B078((u32)self, *p);
    valid = IS_VALID_PTR(data);
    if (!valid) {
        nw4r::db::Panic(lbl_80590010, 0x1B7, lbl_80590088, data);
    }
    return fn_8008B200(data, frame);
}

f32 fn_8008B80C(u32* self, u32* p, s32 isConst, f32 frame)
{
    u32 data;
    u32 valid;

    if (isConst) {
        return *(f32*)p;
    }
    data = fn_8008B078((u32)self, *p);
    valid = IS_VALID_PTR(data);
    if (!valid) {
        nw4r::db::Panic(lbl_80590010, 0x1B7, lbl_80590088, data);
    }
    return fn_8008B95C(data, frame);
}

/* The indexed guards truncate the key index to u16 and (for the u16 channel) hand it to the
 * evaluator.  The index is carried in r5, the const flag in r6. */
f32 fn_8008C498(u32 self, u32* p, u16 index, s32 isConst, f32 frame)
{
    return fn_8008C4A0(self, p, index, isConst, frame);
}

f32 fn_8008C4A0(u32 self, u32* p, u16 index, s32 isConst, f32 frame)
{
    u32 data;
    u32 valid;

    if (isConst) {
        return *(f32*)p;
    }
    data = fn_8008B078(self, *p);
    valid = IS_VALID_PTR(data);
    if (!valid) {
        nw4r::db::Panic(lbl_80590010, 0x1CF, lbl_80590088, data);
    }
    return fn_8008C600(data, index, frame);
}

f32 fn_8008C800(u32 self, u32* p, u16 index, s32 isConst, f32 frame)
{
    return fn_8008C808(self, p, index, isConst, frame);
}

f32 fn_8008C808(u32 self, u32* p, u16 index, s32 isConst, f32 frame)
{
    u32 data;
    u32 valid;

    if (isConst) {
        return *(f32*)p;
    }
    data = fn_8008B078(self, *p);
    valid = IS_VALID_PTR(data);
    if (!valid) {
        nw4r::db::Panic(lbl_80590010, 0x1CF, lbl_80590088, data);
    }
    return fn_8008C968(data, index, frame);
}

f32 fn_8008CB98(u32 self, u32* p, u16 index, s32 isConst, f32 frame)
{
    return fn_8008CBA0(self, p, index, isConst, frame);
}

f32 fn_8008CBA0(u32 self, u32* p, u16 index, s32 isConst, f32 frame)
{
    u32 data;
    u32 valid;

    if (isConst) {
        return *(f32*)p;
    }
    data = fn_8008B078(self, *p);
    valid = IS_VALID_PTR(data);
    if (!valid) {
        nw4r::db::Panic(lbl_80590010, 0x1CF, lbl_80590088, data);
    }
    return fn_8008CD00(data, index, frame);
}

/* ------------------------------------------------------------------------------------------------ */
/* The indexed-cursor evaluators (u16 key cursor, float table).                                        */
/* ------------------------------------------------------------------------------------------------ */

f32 fn_8008C600(u32 self, u16 index, f32 frame)
{
    u16 pos;
    f32 delta;
    f32 v0;
    u32 valid;

    valid = IS_VALID_PTR(self);
    if (!valid) {
        nw4r::db::Panic(lbl_80590010, 0x16C, lbl_80590088, self);
    }
    pos = fn_80082F18(frame);
    if (frame == lbl_80795ED0) {
        return fn_8008C7F0(self, 0);
    }
    if (index <= pos) {
        return fn_8008C7F0(self, index);
    }
    delta = frame - (f32)pos;
    v0 = fn_8008C7F0(self, pos);
    if (lbl_80795ED0 == delta) {
        return v0;
    }
    return (delta * (fn_8008C7F0(self, (u16)(pos + 1)) - v0)) + v0;
}

f32 fn_8008C968(u32 self, u16 index, f32 frame)
{
    u16 pos;
    f32 delta;
    f32 v0;
    u32 valid;

    valid = IS_VALID_PTR(self);
    if (!valid) {
        nw4r::db::Panic(lbl_80590010, 0x16C, lbl_80590088, self);
    }
    pos = fn_80082F18(frame);
    if (frame == lbl_80795ED0) {
        return fn_8008CB58((const ResAnmChrU16Channel*)self, 0);
    }
    if (index <= pos) {
        return fn_8008CB58((const ResAnmChrU16Channel*)self, index);
    }
    delta = frame - (f32)pos;
    v0 = fn_8008CB58((const ResAnmChrU16Channel*)self, pos);
    if (lbl_80795ED0 == delta) {
        return v0;
    }
    return (delta * (fn_8008CB58((const ResAnmChrU16Channel*)self, (u16)(pos + 1)) - v0)) + v0;
}

f32 fn_8008CD00(u32 self, u16 index, f32 frame)
{
    u16 pos;
    f32 delta;
    f32 v0;
    u32 valid;

    valid = IS_VALID_PTR(self);
    if (!valid) {
        nw4r::db::Panic(lbl_80590010, 0x16C, lbl_80590088, self);
    }
    pos = fn_80082F18(frame);
    if (frame == lbl_80795ED0) {
        return fn_8008CEF0((const ResAnmChrU8Channel*)self, 0);
    }
    if (index <= pos) {
        return fn_8008CEF0((const ResAnmChrU8Channel*)self, index);
    }
    delta = frame - (f32)pos;
    v0 = fn_8008CEF0((const ResAnmChrU8Channel*)self, pos);
    if (lbl_80795ED0 == delta) {
        return v0;
    }
    return (delta * (fn_8008CEF0((const ResAnmChrU8Channel*)self, (u16)(pos + 1)) - v0)) + v0;
}

/* ------------------------------------------------------------------------------------------------ */
/* The float channel's cubic-Hermite evaluator.                                                        */
/* ------------------------------------------------------------------------------------------------ */

f32 fn_8008AA8C(u32 self, f32 frame)
{
    ResAnmChrChannelF32* ch = (ResAnmChrChannelF32*)self;
    u32 first;
    u32 last;
    u32 key;
    u32 next;
    u32 firstIdx;
    u32 lastIdx;
    u32 keyIdx;
    u32 nextIdx;
    u16 pos;
    u32 valid;
    u32 frameValid;
    f32 delta;
    f32 curFrame;
    f32 curVal;
    f32 curTan;
    f32 nextVal;
    f32 nextTan;
    f32 loFrame;
    f32 hiFrame;
    f32 span;

    valid = IS_VALID_PTR(self);
    if (!valid) {
        nw4r::db::Panic(lbl_80590010, 0xFC, lbl_80590088, self);
    }
    firstIdx = fn_8008B040(self, 0);
    fn_8008B004(&first, &firstIdx);
    lastIdx = fn_8008B040(self, ch->count - 1);
    fn_8008B004(&last, &lastIdx);
    if (frame == fn_8008AFF8(&first)) {
        return fn_8008AFEC(&first, self);
    }
    if (fn_8008AFF8(&last) == frame) {
        return fn_8008AFEC(&last, self);
    }
    delta = frame - fn_8008AFF8(&first);
    pos = fn_80082F18(ch->rate * (delta * fn_800501E4(ch->count)));
    if (pos > ch->count - 1) {
        nw4r::db::Panic(lbl_80590010, 0x11A, lbl_805900BC, (f64)pos, (f64)(ch->count - 1));
    }
    keyIdx = fn_8008B040(self, (u16)pos);
    fn_8008B004(&key, &keyIdx);
    curFrame = fn_8008AFB0(frame);
    if (curFrame < fn_8008AFA4(&key)) {
        do {
            fn_8008AF94(&key, 0);
        } while (curFrame < fn_8008AFA4(&key));
    } else {
        do {
            fn_8008AF84(&key, 0);
        } while (fn_8008AFA4(&key) <= curFrame);
        fn_8008AF94(&key, 0);
    }
    if (frame == fn_8008AFF8(&key)) {
        return fn_8008AFEC(&key, self);
    }
    nextIdx = fn_8008AF14(&key, 1);
    fn_8008B004(&next, &nextIdx);
    curVal = fn_8008AFEC(&key, self);
    curTan = fn_8008AF08(&key);
    nextVal = fn_8008AFEC(&next, self);
    nextTan = fn_8008AF08(&next);
    loFrame = fn_8008AFF8(&key);
    hiFrame = fn_8008AFF8(&next);
    frameValid = (loFrame <= frame) && (frame <= hiFrame);
    if (!frameValid) {
        nw4r::db::Panic(lbl_80590010, 0x150, lbl_80590100, frame, loFrame, hiFrame);
    }
    span = frame - loFrame;
    return fn_8008AED0(curVal, curTan, nextVal, nextTan,
                       span * fn_800610AC(hiFrame - loFrame), span);
}

/* The s16 key's value halfword read as a float (target keeps the paired-single load; see residuals). */
f32 fn_8008B650(u32* iter)
{
    return ((const ResAnmChrKeyF32*)*iter)->value;
}

/* Extracts the packed 12-bit value of the byte channel's key and evaluates it, scaled and biased. */
f32 fn_8008BF08(u32* iter, const f32* self)
{
    u16 packed = (u16)((*(u32*)*iter & 0xFFF000) >> 12);

    return self[2] * fn_8008B75C((u32)&packed) + self[3];
}

/* ------------------------------------------------------------------------------------------------ */
/* The short channel's cubic-Hermite evaluator (mirrors fn_8008AA8C over the 0x6-byte key).           */
/* ------------------------------------------------------------------------------------------------ */

f32 fn_8008B200(u32 self, f32 frame)
{
    ResAnmChrChannelS16* ch = (ResAnmChrChannelS16*)self;
    u32 first;
    u32 last;
    u32 key;
    u32 next;
    u32 firstIdx;
    u32 lastIdx;
    u32 keyIdx;
    u32 nextIdx;
    u16 pos;
    u32 valid;
    u32 frameValid;
    f32 delta;
    f32 curFrame;
    f32 curVal;
    f32 curTan;
    f32 nextVal;
    f32 nextTan;
    f32 loFrame;
    f32 hiFrame;
    f32 span;
    s16 keyVal;

    valid = IS_VALID_PTR(self);
    if (!valid) {
        nw4r::db::Panic(lbl_80590010, 0xFC, lbl_80590088, self);
    }
    firstIdx = fn_8008B7D0(self, 0);
    fn_8008B794(&first, &firstIdx);
    lastIdx = fn_8008B7D0(self, ch->count - 1);
    fn_8008B794(&last, &lastIdx);
    if (frame == fn_8008B784(&first)) {
        return fn_8008B71C(&first, (const f32*)self);
    }
    if (fn_8008B784(&last) == frame) {
        return fn_8008B71C(&last, (const f32*)self);
    }
    delta = frame - fn_8008B784(&first);
    pos = fn_80082F18(ch->rate * (delta * fn_800501E4(ch->count)));
    if (pos > ch->count - 1) {
        nw4r::db::Panic(lbl_80590010, 0x11A, lbl_805900BC, (f64)pos, (f64)(ch->count - 1));
    }
    keyIdx = fn_8008B7D0(self, (u16)pos);
    fn_8008B794(&key, &keyIdx);
    keyVal = fn_8008B700(frame);
    if (keyVal < fn_8008B6F4(&key)) {
        do {
            fn_8008B6E4(&key, 0);
        } while (keyVal < fn_8008B6F4(&key));
    } else {
        do {
            fn_8008B6D4(&key, 0);
        } while (fn_8008B6F4(&key) <= keyVal);
        fn_8008B6E4(&key, 0);
    }
    if (frame == fn_8008B784(&key)) {
        return fn_8008B71C(&key, (const f32*)self);
    }
    nextIdx = fn_8008B664(&key, 1);
    fn_8008B794(&next, &nextIdx);
    curVal = fn_8008B71C(&key, (const f32*)self);
    curTan = fn_8008B650(&key);
    nextVal = fn_8008B71C(&next, (const f32*)self);
    nextTan = fn_8008B650(&next);
    loFrame = fn_8008B784(&key);
    hiFrame = fn_8008B784(&next);
    frameValid = (loFrame <= frame) && (frame <= hiFrame);
    if (!frameValid) {
        nw4r::db::Panic(lbl_80590010, 0x150, lbl_80590100, frame, loFrame, hiFrame);
    }
    span = frame - loFrame;
    return fn_8008AED0(curVal, curTan, nextVal, nextTan,
                       span * fn_800610AC(hiFrame - loFrame), span);
}

/* ------------------------------------------------------------------------------------------------ */
/* The byte channel's cubic-Hermite evaluator (mirrors fn_8008AA8C over the 0x4-byte key).            */
/* ------------------------------------------------------------------------------------------------ */

f32 fn_8008B95C(u32 self, f32 frame)
{
    ResAnmChrChannelU8* ch = (ResAnmChrChannelU8*)self;
    u32 first;
    u32 last;
    u32 key;
    u32 next;
    u32 firstIdx;
    u32 lastIdx;
    u32 keyIdx;
    u32 nextIdx;
    u16 pos;
    u32 valid;
    u32 frameValid;
    f32 delta;
    f32 curVal;
    f32 curTan;
    f32 nextVal;
    f32 nextTan;
    f32 loFrame;
    f32 hiFrame;
    f32 span;
    u8 keyVal;

    valid = IS_VALID_PTR(self);
    if (!valid) {
        nw4r::db::Panic(lbl_80590010, 0xFC, lbl_80590088, self);
    }
    firstIdx = fn_8008C000(self, 0);
    fn_8008BFC4(&first, &firstIdx);
    lastIdx = fn_8008C000(self, ch->count - 1);
    fn_8008BFC4(&last, &lastIdx);
    if (frame == fn_8008BF5C(&first)) {
        return fn_8008BF08(&first, (const f32*)self);
    }
    if (fn_8008BF5C(&last) == frame) {
        return fn_8008BF08(&last, (const f32*)self);
    }
    delta = frame - fn_8008BF5C(&first);
    pos = fn_80082F18(ch->rate * (delta * fn_800501E4(ch->count)));
    if (pos > ch->count - 1) {
        nw4r::db::Panic(lbl_80590010, 0x11A, lbl_805900BC, (f64)pos, (f64)(ch->count - 1));
    }
    keyIdx = fn_8008C000(self, (u16)pos);
    fn_8008BFC4(&key, &keyIdx);
    keyVal = fn_8008BE88(frame);
    if (keyVal < fn_8008BE7C(&key)) {
        do {
            fn_8008BE6C(&key, 0);
        } while (keyVal < fn_8008BE7C(&key));
    } else {
        do {
            fn_8008BE5C(&key, 0);
        } while (fn_8008BE7C(&key) <= keyVal);
        fn_8008BE6C(&key, 0);
    }
    if (frame == fn_8008BF5C(&key)) {
        return fn_8008BF08(&key, (const f32*)self);
    }
    nextIdx = fn_8008BDEC(&key, 1);
    fn_8008BFC4(&next, &nextIdx);
    curVal = fn_8008BF08(&key, (const f32*)self);
    curTan = fn_8008BDAC(&key);
    nextVal = fn_8008BF08(&next, (const f32*)self);
    nextTan = fn_8008BDAC(&next);
    loFrame = fn_8008BF5C(&key);
    hiFrame = fn_8008BF5C(&next);
    frameValid = (loFrame <= frame) && (frame <= hiFrame);
    if (!frameValid) {
        nw4r::db::Panic(lbl_80590010, 0x150, lbl_80590100, frame, loFrame, hiFrame);
    }
    span = frame - loFrame;
    return fn_8008AED0(curVal, curTan, nextVal, nextTan,
                       span * fn_800610AC(hiFrame - loFrame), span);
}

/* ------------------------------------------------------------------------------------------------ */
/* The per-node 3-component value reader.                                                              */
/* ------------------------------------------------------------------------------------------------ */

/* Reads one node's three components (a scale, a rotation or a translation) from the key cells `keys`,
 * selecting the key layout from the node-data `ScaleType` (flags bits 25-26).  `keys` is a list of
 * value cells: a constant float, or - for ScaleType != 0 - a pointer the channel evaluators resolve.
 * The uniform-flag (bit 27) reuses the first component for all three.  Returns the advanced cursor. */
u32* fn_8008A664(f32* out, u32* self, f32* keys, f32 frame)
{
    u32 flags = self[1];
    u32 scaleType = flags & 0x06000000;
    f32 v0;
    f32 v1;
    f32 v2;
    f32* p = keys;

    switch (scaleType) {
    case 0x00000000:
        v0 = *p++;
        if (flags & 0x10) {
            v1 = v0;
            v2 = v0;
        } else {
            v1 = *p++;
            v2 = *p++;
        }
        break;
    case 0x02000000:
        v0 = fn_8008B808((u32*)self, (u32*)p, (flags & 0x2000) != 0, frame);
        p++;
        if (flags & 0x10) {
            v1 = v0;
            v2 = v0;
        } else {
            v1 = fn_8008B808((u32*)self, (u32*)p, (flags & 0x4000) != 0, frame);
            p++;
            v2 = fn_8008B808((u32*)self, (u32*)p, (flags & 0x8000) != 0, frame);
            p++;
        }
        break;
    case 0x04000000:
        v0 = fn_8008B0AC((u32*)self, (u32*)p, (flags & 0x2000) != 0, frame);
        p++;
        if (flags & 0x10) {
            v1 = v0;
            v2 = v0;
        } else {
            v1 = fn_8008B0AC((u32*)self, (u32*)p, (flags & 0x4000) != 0, frame);
            p++;
            v2 = fn_8008B0AC((u32*)self, (u32*)p, (flags & 0x8000) != 0, frame);
            p++;
        }
        break;
    case 0x06000000:
        v0 = fn_8008A938((u32*)self, (u32*)p, (flags & 0x2000) != 0, frame);
        p++;
        if (flags & 0x10) {
            v1 = v0;
            v2 = v0;
        } else {
            v1 = fn_8008A938((u32*)self, (u32*)p, (flags & 0x4000) != 0, frame);
            p++;
            v2 = fn_8008A938((u32*)self, (u32*)p, (flags & 0x8000) != 0, frame);
            p++;
        }
        break;
    default:
        nw4r::db::Panic(lbl_80590010, 0x298, lbl_80590028, flags);
        v0 = lbl_80795ED0;
        v1 = v0;
        v2 = v0;
        break;
    }
    out[0] = v0;
    out[1] = v1;
    out[2] = v2;
    return (u32*)p;
}

/* ------------------------------------------------------------------------------------------------ */
/* The record setup helpers: zero the scale, build the matrix and evaluate the value row.              */
/* ------------------------------------------------------------------------------------------------ */

void fn_8008D1C4(ResAnmChrObj* self, u32 unused, const f32* key, f32 frame)
{
    nw4r::math::VEC3 rec;

    (void)unused;
    VEC3_ctor(&rec);
    self->scale[0] = lbl_80795EE8;
    self->scale[1] = lbl_80795EE8;
    self->scale[2] = lbl_80795EE8;
    fn_800504D4(self->mat);
    fn_8008CF2C(&rec, key, key + 2, frame);
    self->mat[3] = rec.x;
    self->mat[7] = rec.y;
    self->mat[11] = rec.z;
}

void fn_8008D25C(ResAnmChrObj* self, u32 arg1, const f32* key, f32 frame)
{
    self->scale[0] = lbl_80795EE8;
    self->scale[1] = lbl_80795EE8;
    self->scale[2] = lbl_80795EE8;
    fn_8008C038(self->mat, self->pos, arg1, (u32)key, key + 2, frame);
    self->flags |= 0x80000000;
    self->mat[3] = lbl_80795ED0;
    self->mat[7] = lbl_80795ED0;
    self->mat[11] = lbl_80795ED0;
}

void fn_8008D30C(ResAnmChrObj* self, u32 arg1, const f32* key, f32 frame)
{
    nw4r::math::VEC3 rec;
    void* row;

    VEC3_ctor(&rec);
    self->scale[0] = lbl_80795EE8;
    self->scale[1] = lbl_80795EE8;
    self->scale[2] = lbl_80795EE8;
    row = fn_8008C038(self->mat, self->pos, arg1, (u32)key, key + 2, frame);
    self->flags |= 0x80000000;
    fn_8008CF2C(&rec, key, row, frame);
    self->mat[3] = rec.x;
    self->mat[7] = rec.y;
    self->mat[11] = rec.z;
}

void fn_8008D3DC(ResAnmChrObj* self, u32 arg1, const f32* key, f32 frame)
{
    void* row;

    row = (void*)fn_8008A664(self->scale, (u32*)key, (f32*)&key[2], frame);
    fn_8008C038(self->mat, self->pos, arg1, (u32)key, row, frame);
    self->flags |= 0x80000000;
    self->mat[3] = lbl_80795ED0;
    self->mat[7] = lbl_80795ED0;
    self->mat[11] = lbl_80795ED0;
}

void fn_8008D47C(ResAnmChrObj* self, u32 unused, const f32* key, f32 frame)
{
    nw4r::math::VEC3 rec;
    void* row;

    (void)unused;
    VEC3_ctor(&rec);
    row = (void*)fn_8008A664(self->scale, (u32*)key, (f32*)&key[2], frame);
    fn_800504D4(self->mat);
    fn_8008CF2C(&rec, key, row, frame);
    self->mat[3] = rec.x;
    self->mat[7] = rec.y;
    self->mat[11] = rec.z;
}

void fn_8008D528(ResAnmChrObj* self, u32 arg1, const f32* key, f32 frame)
{
    nw4r::math::VEC3 rec;
    void* row;
    void* out;

    VEC3_ctor(&rec);
    row = (void*)fn_8008A664(self->scale, (u32*)key, (f32*)&key[2], frame);
    out = fn_8008C038(self->mat, self->pos, arg1, (u32)key, row, frame);
    fn_8008CF2C(&rec, key, out, frame);
    self->flags |= 0x80000000;
    self->mat[3] = rec.x;
    self->mat[7] = rec.y;
    self->mat[11] = rec.z;
}

/* ------------------------------------------------------------------------------------------------ */
/* The dirty-state walkers.                                                                            */
/* ------------------------------------------------------------------------------------------------ */

/* The resource-table step (no pointer guard). */
void fn_8008D9B8(u32 self, u32* out)
{
    u32 tmp = fn_800628C8((void*)self, (s32)*((u32*)fn_800618BC((void*)self) + 4));

    fn_80062750(&tmp, out);
}

void fn_8008DA10(ResAnmChrObj* self, f32* out)
{
    u32 valid;

    valid = IS_VALID_PTR(self);
    if (!valid) {
        nw4r::db::Panic(lbl_80590010, 0x499, lbl_80590200, self);
    }
    valid = IS_VALID_PTR(out);
    if (!valid) {
        nw4r::db::Panic(lbl_80590010, 0x49A, lbl_805902E4, out);
    }
    if (self->flags & 8) {
        out[0] = lbl_80795EE8;
        out[1] = lbl_80795EE8;
        out[2] = lbl_80795EE8;
    } else {
        copyVec3((nw4r::math::VEC3*)out, (const nw4r::math::VEC3*)self->scale);
    }
}

void fn_8008E1C0(ResAnmChrObj* self, f32* out)
{
    u32 valid;

    valid = IS_VALID_PTR(self);
    if (!valid) {
        nw4r::db::Panic(lbl_80590010, 0x50D, lbl_80590200, self);
    }
    valid = IS_VALID_PTR(out);
    if (!valid) {
        nw4r::db::Panic(lbl_80590010, 0x50E, lbl_80590350, out);
    }
    if (self->flags & 0x40) {
        out[0] = lbl_80795ED0;
        out[1] = lbl_80795ED0;
        out[2] = lbl_80795ED0;
    } else {
        out[0] = self->mat[3];
        out[1] = self->mat[7];
        out[2] = self->mat[11];
    }
}

void fn_8008DF8C(ResAnmChrObj* self, f32* out)
{
    u32 valid;

    valid = IS_VALID_PTR(self);
    if (!valid) {
        nw4r::db::Panic(lbl_80590010, 0x4F4, lbl_80590200, self);
    }
    valid = IS_VALID_PTR(out);
    if (!valid) {
        nw4r::db::Panic(lbl_80590010, 0x4F5, lbl_8059031C, out);
    }
    if (self->flags & 0x20) {
        fn_805012C4(out);
    } else {
        fn_805012E8(out, self->mat);
    }
}

void fn_8008F148(ResAnmChrObj* self, const f32* out)
{
    u32 valid;
    u32 mask;

    valid = IS_VALID_PTR(self);
    if (!valid) {
        nw4r::db::Panic(lbl_80590010, 0x607, lbl_80590200, self);
    }
    valid = IS_VALID_PTR(out);
    if (!valid) {
        nw4r::db::Panic(lbl_80590010, 0x608, lbl_80590350, out);
    }
    if (out[0] == lbl_80795ED0 && out[1] == lbl_80795ED0 && out[2] == lbl_80795ED0) {
        mask = 0x40;
        if (self->flags & 0x20) {
            mask |= 4;
            if (self->flags & 8) {
                mask |= 2;
            }
        }
        self->flags |= mask;
    } else {
        self->flags &= ~0x46;
        self->mat[3] = out[0];
        self->mat[7] = out[1];
        self->mat[11] = out[2];
    }
}

void fn_8008E408(ResAnmChrObj* self, f32* out)
{
    u32 valid;
    u32 flags;

    valid = IS_VALID_PTR(self);
    if (!valid) {
        nw4r::db::Panic(lbl_80590010, 0x52A, lbl_80590200, self);
    }
    valid = IS_VALID_PTR(out);
    if (!valid) {
        nw4r::db::Panic(lbl_80590010, 0x52B, lbl_80590388, out);
    }
    flags = self->flags;
    if (flags & 0x20) {
        if (flags & 0x40) {
            fn_800504D4(out);
        } else {
            fn_800504D4(out);
            out[3] = self->mat[3];
            out[7] = self->mat[7];
            out[11] = self->mat[11];
        }
    } else if (flags & 0x40) {
        fn_8007100C(out, self->mat);
        out[3] = lbl_80795ED0;
        out[7] = lbl_80795ED0;
        out[11] = lbl_80795ED0;
    } else {
        fn_8007100C(out, self->mat);
    }
}

void fn_8008E698(ResAnmChrObj* self, f32* out)
{
    u32 valid;

    valid = IS_VALID_PTR(self);
    if (!valid) {
        nw4r::db::Panic(lbl_80590010, 0x55B, lbl_80590200, self);
    }
    valid = IS_VALID_PTR(out);
    if (!valid) {
        nw4r::db::Panic(lbl_80590010, 0x55C, lbl_805903BC, out);
    }
    fn_8008E408(self, out);
    if (!(self->flags & 8)) {
        fn_8050133C(out, out, self->scale);
    }
}

void fn_8008E8D0(ResAnmChrObj* self, const f32* out)
{
    u32 valid;
    u32 t;

    valid = IS_VALID_PTR(self);
    if (!valid) {
        nw4r::db::Panic(lbl_80590010, 0x572, lbl_80590200, self);
    }
    valid = IS_VALID_PTR(out);
    if (!valid) {
        nw4r::db::Panic(lbl_80590010, 0x573, lbl_805902E4, out);
    }
    if (out[0] == lbl_80795EE8 && out[1] == lbl_80795EE8 && out[2] == lbl_80795EE8) {
        t = self->flags | 0x18;
        self->flags = t;
        if (t & 4) {
            self->flags = t | 2;
        }
    } else {
        t = self->flags & ~0x1A;
        self->flags = t;
        if (out[0] == out[1] && out[1] == out[2]) {
            self->flags = t | 0x10;
        }
    }
    copyVec3((nw4r::math::VEC3*)self->scale, (const nw4r::math::VEC3*)out);
}

void fn_8008EB68(ResAnmChrObj* self, const f32* out)
{
    u32 valid;
    u32 mask;
    u32 flags;

    valid = IS_VALID_PTR(self);
    if (!valid) {
        nw4r::db::Panic(lbl_80590010, 0x59A, lbl_80590200, self);
    }
    valid = IS_VALID_PTR(out);
    if (!valid) {
        nw4r::db::Panic(lbl_80590010, 0x59B, lbl_8059031C, out);
    }
    if (out[0] == lbl_80795ED0 && out[1] == lbl_80795ED0 && out[2] == lbl_80795ED0) {
        mask = 0x20;
        flags = self->flags;
        if (flags & 0x40) {
            mask |= 4;
            if (flags & 8) {
                mask |= 2;
            }
        }
        self->flags |= mask;
        self->mat[0] = lbl_80795EE8;
        self->mat[1] = lbl_80795ED0;
        self->mat[2] = lbl_80795ED0;
        self->mat[4] = lbl_80795ED0;
        self->mat[5] = lbl_80795EE8;
        self->mat[6] = lbl_80795ED0;
        self->mat[8] = lbl_80795ED0;
        self->mat[9] = lbl_80795ED0;
        self->mat[10] = lbl_80795EE8;
    } else {
        nw4r::math::VEC3 rec;

        setVec3(&rec, self->mat[3], self->mat[7], self->mat[11]);
        fn_8008C484(self->mat, out[0], out[1], out[2]);
        self->mat[3] = rec.x;
        self->mat[7] = rec.y;
        self->mat[11] = rec.z;
        copyVec3((nw4r::math::VEC3*)self->pos, (const nw4r::math::VEC3*)out);
        self->flags &= ~0x26;
    }
    self->flags |= 0x80000000;
}

void fn_8008EE68(ResAnmChrObj* self, const f32* out)
{
    u32 valid;
    u32 mask;
    u32 flags;

    valid = IS_VALID_PTR(self);
    if (!valid) {
        nw4r::db::Panic(lbl_80590010, 0x5DB, lbl_80590200, self);
    }
    valid = IS_VALID_PTR(out);
    if (!valid) {
        nw4r::db::Panic(lbl_80590010, 0x5DC, lbl_8059031C, out);
    }
    if (out[0] == lbl_80795EE8 && out[1] == lbl_80795ED0 && out[2] == lbl_80795ED0 &&
        out[3] == lbl_80795ED0 && out[4] == lbl_80795EE8 && out[5] == lbl_80795ED0 &&
        out[6] == lbl_80795ED0 && out[7] == lbl_80795ED0 && out[8] == lbl_80795EE8) {
        mask = 0x20;
        flags = self->flags;
        if (flags & 0x40) {
            mask |= 4;
            if (flags & 8) {
                mask |= 2;
            }
        }
        self->flags |= mask;
    } else {
        self->flags &= ~0x26;
    }
    fn_80501594(self->mat, out);
    self->flags &= 0x7FFFFFFF;
}

void fn_8008F3DC(ResAnmChrObj* self, const void* src)
{
    u32 valid;
    nw4r::math::VEC3 rec;
    f32 norm[3];

    valid = IS_VALID_PTR(self);
    if (!valid) {
        nw4r::db::Panic(lbl_80590010, 0x671, lbl_80590200, self);
    }
    valid = IS_VALID_PTR(src);
    if (!valid) {
        nw4r::db::Panic(lbl_80590010, 0x672, lbl_805903BC, src);
    }
    setVec3(&rec, fn_8008F6C4((u32)src, 0), fn_8008F6C4((u32)src, 1), fn_8008F6C4((u32)src, 2));
    fn_8008E8D0(self, (const f32*)&rec);
    if (self->flags & 8) {
        fn_8007100C(self->mat, src);
    } else {
        setVec3((nw4r::math::VEC3*)norm, fn_800610AC(rec.x), fn_800610AC(rec.y), fn_800610AC(rec.z));
        fn_8050133C(self->mat, src, norm);
    }
    self->flags &= 0x7FFFFFFF;
}

s32 fn_8008DC4C(ResAnmChrObj* self, f32* out)
{
    u32 valid;
    f32 r;

    valid = IS_VALID_PTR(self);
    if (!valid) {
        nw4r::db::Panic(lbl_80590010, 0x4BB, lbl_80590200, self);
    }
    valid = IS_VALID_PTR(out);
    if (!valid) {
        nw4r::db::Panic(lbl_80590010, 0x4BC, lbl_8059031C, out);
    }
    if (self->flags & 0x20) {
        out[0] = lbl_80795ED0;
        out[1] = lbl_80795ED0;
        out[2] = lbl_80795ED0;
        return 1;
    }
    if (self->flags & 0x80000000) {
        copyVec3((nw4r::math::VEC3*)out, (const nw4r::math::VEC3*)self->pos);
        return 1;
    }
    r = fn_80050BC0(lbl_80795EE8 - self->mat[8] * self->mat[8]);
    if (r == lbl_80795ED0) {
        out[0] = fn_8008DF64(self->mat[2] + self->mat[5], self->mat[6] + self->mat[1]);
        out[1] = fn_8006497C(self->mat[8], lbl_80795EF8, lbl_80795EFC);
        out[2] = fn_8008DF64(self->mat[2] + self->mat[5], self->mat[6] - self->mat[1]);
    } else {
        out[0] = fn_8008DF64(self->mat[9], self->mat[10]);
        out[1] = fn_8008DF64(-self->mat[8], r);
        out[2] = fn_8008DF64(self->mat[4], self->mat[0]);
    }
    return 0;
}

} /* extern "C" */

/*
 * Residuals - what still differs, measured with `recompile.py g3d/g3d_resanmchr.cpp --measure <sym>`:
 *
 *   fn_8008A664   81.04 %  the `ScaleType` compare chain holds the masked word in r7 in the target and
 *                          r3 here (the only differing instructions are the `rlwinm`/`subis` register);
 *                          the `switch` case order and the values themselves match.
 *   fn_8008C600 / C968 / CD00   97.62 %  the instruction stream is identical; the gap is the
 *                          relocation-normalised branch target encoding (the target is a standalone
 *                          per-symbol object, ours is one function of a 101-function unit).
 *   fn_8008AA8C / B200 / B95C   98.19-98.21 %  same class (1080/1092 target bytes; no instruction differs
 *                          under objdiff's pairing).
 *   fn_8008D30C 91.42 / D528 91.68 / D47C 92.56 / D3DC 94.45 %  the `fn_8008C038`/`fn_8008CF2C` argument
 *                          shaping (both callees are still missing, so their declared parameters are an
 *                          approximation) and register colouring.
 *   fn_8008EE68 97.96 / EB68 98.98 / F148 99.06 / E8D0 99.10 %  register colouring only.
 *   fn_8008B650 48.00 / AFE4 70.00 / B78C 70.00 / BFBC 70.00 %  the target loads a scalar float with
 *                          `psq_l fN,0(rN),1,qrN` where MWCC 0x0f emits `lfs`.  Same compiler-build
 *                          difference (retail `.comment` version 0x0e vs 0x0f) the sibling
 *                          `g3d/g3d_resanm.c` recorded for `fn_80089F78`; no source shape or flag
 *                          reproduces it.
 *   fn_8008B704 73.33 / BEF0 73.33 %  the target's float->short/byte truncation is `psq_st ...,1,qrN` +
 *                          `lha`/`lbz`; `union` bit-punning, a two-step temp and a plain cast all emit
 *                          `fctiwz`/`stfd`.  The same unreachable residual `g3d/g3d_resanm.c` documented
 *                          for `fn_80089F78`.
 *   fn_8008F6C4 99.78 %  two internal loads are ordered 0x10-then-0x00 here and 0x00-then-0x10 in the
 *                          target (one instruction pair).
 */
