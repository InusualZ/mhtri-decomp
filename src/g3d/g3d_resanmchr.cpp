/*
 * g3d/g3d_resanmchr.cpp - nw4r g3d `ResAnmChr` character-animation channel evaluators: four channel kinds (a
 *   record of frame count +0x00 and frame rate +0x04, keys of stride 0xC float, 0x6 s16 or 0x4 u8), each a
 *   cubic-Hermite evaluator behind an offset/validity guard, and the per-node scale/rotation/translation readers.
 * RANGE. .text 0x8008A664-0x8008F6E8 (101 functions); extab, extabindex, .rodata 0x8056F710-0x8056F730, .data
 *   0x80590010-0x805903F0 (opens on "g3d_resanmchr.cpp"), .sdata2 0x80795ED0-0x80795F08.
 * NAMES. Map stems (the dump answers `zz_` placeholders).  GUESS: res_anm_chr_get_result_table (.rodata 0x8056F710,
 *   the eight per-node result builders `GetAnmResult` dispatches on node flag bits 22-24), res_anm_chr_load_qr7
 *   (0x8008B65C, the GQR7 load fn_8008B650 tail-calls).
 * RESIDUALS. none.
 * SHAPES. File-scope `#pragma peephole off`: retail keeps the split `clrlwi`+`slwi`/`cmpwi` forms.
 *   Definitions in address order (the `.data` strings and the `.sdata2` pool follow first use).  The channel
 *   guards take `frame` before `isConst` (retail loads f1 ahead of the flag test); the three readers advance their
 *   cursor parameter itself, read constants as `*(f32*)p++` and pass the type word to `Panic`; the result builders
 *   reassign one `anmData` cursor through the readers.
 *   The quantized loads and stores (fn_8008AFE4, fn_8008B78C, fn_8008BFBC, res_anm_chr_load_qr7, fn_8008B704,
 *   fn_8008BEF0) are inline `psq_l`/`psq_st` through GQR2/3/6/7.
 *   The key evaluators test `frame <= first`, `last <= frame` and `frame >= lo`, and hold the interval's
 *   reciprocal in a local before the Hermite call; the linear evaluators test `frame <= 0.0f`, declare `v0`
 *   before `delta` and load the next key into a local; the node setters declare `flags` before `mask`.  `fp_contract`
 *   stays on (fn_8008AED0 uses fused `fmadds`/`fmsubs`).
 */

#include "types.h"
#include "nw4r/g3d/res_common.h" /* IS_VALID_PTR (rule 1) */
#include "g3d/g3d_calcview.h" /* mtx34_copy_ps (rule 2) */
#include "g3d/g3d_scnroot.h" /* fn_80082F18 (rule 2) */
#include "g3d/g3d_anmchr.h"      /* math_reciprocal, ResAnmChr, fn_800628C8 (rule 2) */
#include "g3d/g3d_rescommon.h"   /* nw4r::g3d::ResDic (rule 2) */
#include "g3d/g3d_anmobj.h"      /* f32_select_nonneg (rule 2) */
#include "fn_8004CAD8.h"         /* anim_tick_angle, mtx34_identity, sqrt_f32 (rule 2) */
#include "mh3_pad.h"             /* copyVec3, setVec3, VEC3_ctor (rule 2) */
#include "MSL_C/alloc.h"         /* __fpclassifyf, owner MSL_C/alloc.cpp (rule 2) */
#include "nw4r/math_types.h"      /* MTX34RotXYZFIdx, MTX34Scale, MTX33Identity, ... (rule 2) */
#include "nw4r/math_triangular.h" /* Atan2FIdx (rule 2) */

#pragma pool_data off

/* Retail keeps the split `clrlwi`+`slwi`/`cmpwi` forms the peephole would fuse; `fp_contract` stays on (the
 * target's fn_8008AED0 uses fused `fmadds`/`fmsubs`). */
#pragma peephole off

namespace nw4r {
namespace db {

/* `Panic(const char* pFile, int line, const char* pFmt, ...)` - the trailing `e` in the map's mangling
 * is MWCC's vararg marker.  Declared in the namespace (C++ linkage), never as the mangled spelling
 * (rule 9). */
void Panic(const char* pFile, int line, const char* pFmt, ...);

} // namespace db
} // namespace nw4r

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

/* The `AnmObjChr`/`ResAnmChr` runtime object the frame/dirty-state walkers operate on.  +0x00 is the
 * state/flags word, +0x04 a 3-float scale, +0x10 a 3-float position and +0x1C a 12-float (3x4) matrix
 * whose last column (+0x28/+0x38/+0x48) holds the evaluated row.  Only the words the walkers touch are
 * evidenced, so the size is a lower bound. */
typedef struct {
    /* +0x00 */ u32 flags;
    /* +0x04 */ f32 scale[3];
    /* +0x10 */ f32 pos[3];
    /* +0x1C */ f32 mat[12];
} ResAnmChrObj; /* size: 0x4C - lower bound, an approximation */

/* The 3-component record the walkers copy is `Vec3f` as `g3d/g3d_anmscn.cpp` already defines it
 * (rule 1: a shared type is defined once, in the lexicographically first owner); this unit reads the
 * components through `f32*` cursors instead of redefining it. */

/* ================================================================================================== */
/* Definitions                                                                                         */
/* ================================================================================================== */

using nw4r::g3d::ResAnmChrNodeData;

/* The per-node result builders, indexed by the node flags' scale/rotate/translate presence bits (22-24). */
typedef void (*ResAnmChrGetResultFunc)(ResAnmChrObj* self, const u16* info, ResAnmChrNodeData* nodeData, f32 frame);

extern "C" {

/* This unit's own bodies, forward-declared so the helpers can call them in address order (rule 2: the
 * owner of these symbols is this file). */
f32 fn_8008A93C(u32* self, u32* p, f32 frame, s32 isConst);
f32 fn_8008B0B0(u32* self, u32* p, f32 frame, s32 isConst);
f32 fn_8008B80C(u32* self, u32* p, f32 frame, s32 isConst);
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
f32 res_anm_chr_load_qr7(const f32* p);
f32 fn_8008BF08(u32* iter, const f32* self);
f32 fn_8008DF64(f32 a, f32 b);
f32 fn_8008F6C4(u32 self, s32 index);
f32 anim_tick_angle(u16 count);
f32 sqrt_f32(f32 value);
s16 fn_8008B700(f32 value);
u8 fn_8008BE88(f32 value);
u8 fn_8008BE7C(u32* p);
u32 fn_8008C000(u32 self, u32 index);
u32* fn_8008BFC4(u32* out, u32* src);
f32 fn_8008BF5C(u32* iter);
f32 fn_8008BDAC(u32* iter);
u32 fn_8008BDEC(u32* iter, s32 index);
u32* fn_8008C038(f32* mtx, f32* rawRot, const u16* info, ResAnmChrNodeData* self, f32* p, f32 frame);
u32* fn_8008CF2C(f32* out, ResAnmChrNodeData* self, f32* p, f32 frame);
f32 fn_8008CB98(u32 self, u32* p, u16 index, f32 frame, s32 isConst);
f32 fn_8008C800(u32 self, u32* p, u16 index, f32 frame, s32 isConst);
f32 fn_8008C498(u32 self, u32* p, u16 index, f32 frame, s32 isConst);
void fn_8008C484(f32* out, f32 x, f32 y, f32 z);
ResAnmChrNodeData* fn_8008D9B8(u32 self, u32 idx);
void fn_8008D1AC(ResAnmChrObj* self, const u16* info, ResAnmChrNodeData* nodeData, f32 frame);
void fn_8008D2CC(ResAnmChrObj* self, const u16* info, ResAnmChrNodeData* nodeData, f32 frame);
f32 fn_8008C4A0(u32 self, u32* p, u16 index, f32 frame, s32 isConst);
f32 fn_8008C808(u32 self, u32* p, u16 index, f32 frame, s32 isConst);
f32 fn_8008CBA0(u32 self, u32* p, u16 index, f32 frame, s32 isConst);
f32 fn_8008C600(u32 self, u16 index, f32 frame);
f32 fn_8008C968(u32 self, u16 index, f32 frame);
f32 fn_8008CD00(u32 self, u16 index, f32 frame);
u32* fn_8008A664(f32* out, ResAnmChrNodeData* self, f32* p, f32 frame);

void fn_8008D25C(ResAnmChrObj* self, const u16* info, ResAnmChrNodeData* nodeData, f32 frame);
void fn_8008D3DC(ResAnmChrObj* self, const u16* info, ResAnmChrNodeData* nodeData, f32 frame);
void fn_8008D1C4(ResAnmChrObj* self, const u16* info, ResAnmChrNodeData* nodeData, f32 frame);
void fn_8008D47C(ResAnmChrObj* self, const u16* info, ResAnmChrNodeData* nodeData, f32 frame);
void fn_8008D30C(ResAnmChrObj* self, const u16* info, ResAnmChrNodeData* nodeData, f32 frame);
void fn_8008D528(ResAnmChrObj* self, const u16* info, ResAnmChrNodeData* nodeData, f32 frame);

f32 fn_8008B75C(u32 addr);
void fn_8008DA10(ResAnmChrObj* self, f32* out);
s32 fn_8008DC4C(ResAnmChrObj* self, f32* out);
void fn_8008DF8C(ResAnmChrObj* self, f32* out);
void fn_8008E1C0(ResAnmChrObj* self, f32* out);
void fn_8008E698(ResAnmChrObj* self, f32* out);
void fn_8008EB68(ResAnmChrObj* self, const f32* out);
void fn_8008EE68(ResAnmChrObj* self, const f32* out);
void fn_8008F148(ResAnmChrObj* self, const f32* out);
void fn_8008F3DC(ResAnmChrObj* self, const void* src);

f32 fn_8008A938(u32* self, u32* p, f32 frame, s32 isConst);
f32 fn_8008AED0(f32 v0, f32 t0, f32 v1, f32 t1, f32 rate, f32 span);
f32 fn_8008AF08(u32* p);
u32 fn_8008AF14(u32* iter, s32 index);
u32* fn_8008AF4C(u32* out, u32 v);
void fn_8008AF7C(u32* p, u32 v);
f32 fn_8008AFA4(u32* p);
void fn_8008AFB4(u32 addr, f32* out);
f32 fn_8008AFE4(register const f32* p);
f32 fn_8008AFEC(u32* p, u32 self);
f32 fn_8008AFF8(u32* p);
u32* fn_8008B004(u32* out, u32* src);
void fn_8008B034(u32* dst, u32* src);
u32 fn_8008B040(u32 iter, u32 index);
u32 fn_8008B078(u32 self, u32 offset);
u32 fn_8008B0A8(u32 self);
f32 fn_8008B0AC(u32* self, u32* p, f32 frame, s32 isConst);
u32 fn_8008B664(u32* iter, s32 index);
u32* fn_8008B69C(u32* out, u32 v);
void fn_8008B6CC(u32* p, u32 v);
s16 fn_8008B6F4(u32* p);
s16 fn_8008B704(register f32 value);
f32 fn_8008B71C(u32* iter, const f32* self);
f32 fn_8008B784(u32* iter);
f32 fn_8008B78C(register u32 addr);
u32* fn_8008B794(u32* out, u32* src);
void fn_8008B7C4(u32* dst, u32* src);
u32 fn_8008B7D0(u32 self, u32 index);
f32 fn_8008B808(u32* self, u32* p, f32 frame, s32 isConst);
u32* fn_8008BE24(u32* out, u32 v);
void fn_8008BE54(u32* p, u32 v);
u8 fn_8008BE8C(f32 value);
void fn_8008BEBC(const f32* value, u8* out);
u8 fn_8008BEF0(register f32 value);
f32 fn_8008BF64(u32 addr);
void fn_8008BF8C(u32 addr, f32* out);
f32 fn_8008BFBC(register u32 addr);
void fn_8008BFF4(u32* dst, u32* src);
f32 fn_8008C7F0(u32 self, u16 index);
f32 fn_8008CB58(const ResAnmChrU16Channel* p, u16 index);
f32 fn_8008CEF0(const ResAnmChrU8Channel* p, u16 index);
void fn_8008E408(ResAnmChrObj* self, f32* out);
void fn_8008E8D0(ResAnmChrObj* self, const f32* out);

extern const ResAnmChrGetResultFunc res_anm_chr_get_result_table[8];

const ResAnmChrGetResultFunc res_anm_chr_get_result_table[8] = {
    fn_8008D1AC,
    fn_8008D2CC,
    fn_8008D25C,
    fn_8008D3DC,
    fn_8008D1C4,
    fn_8008D47C,
    fn_8008D30C,
    fn_8008D528,
};

/* ------------------------------------------------------------------------------------------------ */
/* The float channel's key cursor helpers (0xC-byte stride).                                           */
/* ------------------------------------------------------------------------------------------------ */

/* Reads one node's three components from the value cells `keys` (constants, or channel pointers when the
 * `ScaleType`, flag bits 25-26, is non-zero; bit 27 reuses the first for all three); returns the advanced cursor. */
u32* fn_8008A664(f32* out, ResAnmChrNodeData* self, f32* p, f32 frame)
{
    u32 flags = self->flags;
    u32 scaleType = flags & 0x06000000;
    f32 v0;
    f32 v1;
    f32 v2;

    switch (scaleType) {
    case 0x00000000:
        v0 = *(f32*)p++;
        if (flags & 0x10) {
            v1 = v0;
            v2 = v0;
        } else {
            v1 = *(f32*)p++;
            v2 = *(f32*)p++;
        }
        break;
    case 0x02000000:
        v0 = fn_8008B808((u32*)self, (u32*)p++, frame, (flags & 0x2000) != 0);
        if (flags & 0x10) {
            v1 = v0;
            v2 = v0;
        } else {
            v1 = fn_8008B808((u32*)self, (u32*)p++, frame, (flags & 0x4000) != 0);
            v2 = fn_8008B808((u32*)self, (u32*)p++, frame, (flags & 0x8000) != 0);
        }
        break;
    case 0x04000000:
        v0 = fn_8008B0AC((u32*)self, (u32*)p++, frame, (flags & 0x2000) != 0);
        if (flags & 0x10) {
            v1 = v0;
            v2 = v0;
        } else {
            v1 = fn_8008B0AC((u32*)self, (u32*)p++, frame, (flags & 0x4000) != 0);
            v2 = fn_8008B0AC((u32*)self, (u32*)p++, frame, (flags & 0x8000) != 0);
        }
        break;
    case 0x06000000:
        v0 = fn_8008A938((u32*)self, (u32*)p++, frame, (flags & 0x2000) != 0);
        if (flags & 0x10) {
            v1 = v0;
            v2 = v0;
        } else {
            v1 = fn_8008A938((u32*)self, (u32*)p++, frame, (flags & 0x4000) != 0);
            v2 = fn_8008A938((u32*)self, (u32*)p++, frame, (flags & 0x8000) != 0);
        }
        break;
    default:
        nw4r::db::Panic("g3d_resanmchr.cpp", 0x298, "NW4R Internal error\nResAnmChrNodeData.flags has invalid ScaleType(flags=0x%08X, ScaleType=0x%X)", flags, scaleType);
        v0 = 0.0f;
        v1 = v0;
        v2 = v0;
        break;
    }
    out[0] = v0;
    out[1] = v1;
    out[2] = v2;
    return (u32*)p;
}

/* Tail entry into the float evaluator. */
f32 fn_8008A938(u32* self, u32* p, f32 frame, s32 isConst)
{
    return fn_8008A93C(self, p, frame, isConst);
}

/* Evaluates one float channel key cell at `frame`.  `p` holds either the constant value (`isConst`) or
 * a self-relative offset to the frame data, which resolves against `self`. */
f32 fn_8008A93C(u32* self, u32* p, f32 frame, s32 isConst)
{
    u32 data;
    u32 valid;

    if (isConst) {
        return *(f32*)p;
    }
    data = fn_8008B078((u32)self, *p);
    valid = IS_VALID_PTR(data);
    if (!valid) {
        nw4r::db::Panic("g3d_resanmchr.cpp", 0x1B7, "NW4R:Pointer Error\npAnm(=%p) is not valid pointer.", data);
    }
    return fn_8008AA8C(data, frame);
}

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
        nw4r::db::Panic("g3d_resanmchr.cpp", 0xFC, "NW4R:Pointer Error\npAnm(=%p) is not valid pointer.", self);
    }
    firstIdx = fn_8008B040(self, 0);
    fn_8008B004(&first, &firstIdx);
    lastIdx = fn_8008B040(self, ch->count - 1);
    fn_8008B004(&last, &lastIdx);
    if (frame <= fn_8008AFF8(&first)) {
        return fn_8008AFEC(&first, self);
    }
    if (fn_8008AFF8(&last) <= frame) {
        return fn_8008AFEC(&last, self);
    }
    delta = frame - fn_8008AFF8(&first);
    pos = math_f32_to_u16(ch->rate * (delta * anim_tick_angle(ch->count)));
    if (pos > ch->count - 1) {
        nw4r::db::Panic("g3d_resanmchr.cpp", 0x11A, "estimatePos is out of bounds(%f)\nestimatePos <= %f not satisfied.", (f64)pos, (f64)(ch->count - 1));
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
    frameValid = (frame >= loFrame) && (frame <= hiFrame);
    if (!frameValid) {
        nw4r::db::Panic("g3d_resanmchr.cpp", 0x150, "frame is out of bounds(%f)\n%f <= frame <= %f not satisfied.", frame, loFrame, hiFrame);
    }
    span = frame - loFrame;
    f32 invRange = math_reciprocal(hiFrame - loFrame);
    return fn_8008AED0(curVal, curTan, nextVal, nextTan, span * invRange, span);
}

/* The cubic-Hermite key interpolation.  `rate` is the normalised in-key position, `span` the key
 * length; v0/t0 and v1/t1 are the two keys' values and tangents. */
f32 fn_8008AED0(f32 v0, f32 t0, f32 v1, f32 t1, f32 rate, f32 span)
{
    f32 hm1 = rate - 1.0f;

    return (span * hm1 * ((hm1 * t0) + (rate * t1))) +
           ((rate * (rate * (((2.0f * rate) - 3.0f) * (v0 - v1)))) + v0);
}

/* The tangent of the current float key. */
f32 fn_8008AF08(u32* p)
{
    return ((ResAnmChrKeyF32*)*p)->tangent;
}

/* The float key at `index`. */
u32 fn_8008AF14(u32* iter, s32 index)
{
    u32 tmp;

    return *fn_8008AF4C(&tmp, *iter + index * 0xC);
}

/* The `ResCommon<T>` store-and-return shape. */
u32* fn_8008AF4C(u32* out, u32 v)
{
    fn_8008AF7C(out, v);
    return out;
}

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

/* Loads the float at `addr` into `out`. */
void fn_8008AFB4(u32 addr, f32* out)
{
    *out = fn_8008AFE4((const f32*)addr);
}

/* Loads the float at `p`. */
f32 fn_8008AFE4(register const f32* p)
{
    register f32 result;

    asm {
        psq_l result, 0(p), 1, 3
    }
    return result;
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

/* The `ResCommon<T>` copy-and-return shape. */
u32* fn_8008B004(u32* out, u32* src)
{
    fn_8008B034(out, src);
    return out;
}

/* Stores `src` through `dst`. */
void fn_8008B034(u32* dst, u32* src)
{
    *dst = *src;
}

/* The float channel's own frame halfword at `index` (+0x8 into each key). */
u32 fn_8008B040(u32 iter, u32 index)
{
    u32 tmp;

    return *fn_8008AF4C(&tmp, iter + index * 0xC + 0x8);
}

/* Resolves a self-relative offset against the resource base. */
u32 fn_8008B078(u32 self, u32 offset)
{
    return offset + fn_8008B0A8(self);
}

/* The identity the resource pointer resolver folds in. */
u32 fn_8008B0A8(u32 self)
{
    return self;
}

/* Tail entry into the s16 evaluator. */
f32 fn_8008B0AC(u32* self, u32* p, f32 frame, s32 isConst)
{
    return fn_8008B0B0(self, p, frame, isConst);
}

f32 fn_8008B0B0(u32* self, u32* p, f32 frame, s32 isConst)
{
    u32 data;
    u32 valid;

    if (isConst) {
        return *(f32*)p;
    }
    data = fn_8008B078((u32)self, *p);
    valid = IS_VALID_PTR(data);
    if (!valid) {
        nw4r::db::Panic("g3d_resanmchr.cpp", 0x1B7, "NW4R:Pointer Error\npAnm(=%p) is not valid pointer.", data);
    }
    return fn_8008B200(data, frame);
}

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
        nw4r::db::Panic("g3d_resanmchr.cpp", 0xFC, "NW4R:Pointer Error\npAnm(=%p) is not valid pointer.", self);
    }
    firstIdx = fn_8008B7D0(self, 0);
    fn_8008B794(&first, &firstIdx);
    lastIdx = fn_8008B7D0(self, ch->count - 1);
    fn_8008B794(&last, &lastIdx);
    if (frame <= fn_8008B784(&first)) {
        return fn_8008B71C(&first, (const f32*)self);
    }
    if (fn_8008B784(&last) <= frame) {
        return fn_8008B71C(&last, (const f32*)self);
    }
    delta = frame - fn_8008B784(&first);
    pos = math_f32_to_u16(ch->rate * (delta * anim_tick_angle(ch->count)));
    if (pos > ch->count - 1) {
        nw4r::db::Panic("g3d_resanmchr.cpp", 0x11A, "estimatePos is out of bounds(%f)\nestimatePos <= %f not satisfied.", (f64)pos, (f64)(ch->count - 1));
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
    frameValid = (frame >= loFrame) && (frame <= hiFrame);
    if (!frameValid) {
        nw4r::db::Panic("g3d_resanmchr.cpp", 0x150, "frame is out of bounds(%f)\n%f <= frame <= %f not satisfied.", frame, loFrame, hiFrame);
    }
    span = frame - loFrame;
    f32 invRange = math_reciprocal(hiFrame - loFrame);
    return fn_8008AED0(curVal, curTan, nextVal, nextTan, span * invRange, span);
}

/* The s16 key's value halfword read as a float (target keeps the paired-single load; see residuals). */
f32 fn_8008B650(u32* iter)
{
    return res_anm_chr_load_qr7(&((const ResAnmChrKeyF32*)*iter)->value);
}

/* Loads the quantized scalar at `p` through GQR7 (a paired-single load, so the key's packed form converts on
 * the way in). */
f32 res_anm_chr_load_qr7(register const f32* p)
{
    register f32 result;

    asm {
        psq_l result, 0(p), 1, 7
    }
    return result;
}

u32 fn_8008B664(u32* iter, s32 index)
{
    u32 tmp;

    return *fn_8008B69C(&tmp, *iter + index * 0x6);
}

u32* fn_8008B69C(u32* out, u32 v)
{
    fn_8008B6CC(out, v);
    return out;
}

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

s16 fn_8008B700(f32 value)
{
    return fn_8008B704(value);
}

/* Truncates a float to a short by reading the stored word's high halfword. */
s16 fn_8008B704(register f32 value)
{
    f32 buffer;
    register f32* pBuffer = &buffer;
    register s16 result;

    asm {
        psq_st value, 0(pBuffer), 1, 6
        lha result, 0(pBuffer)
    }
    return result;
}

/* The current s16 key's angle, scaled and biased by the object's rate/offset. */
f32 fn_8008B71C(u32* iter, const f32* self)
{
    return self[2] * fn_8008B75C(*iter + 2) + self[3];
}

f32 fn_8008B75C(u32 addr)
{
    f32 out;

    fn_8008AFB4(addr, &out);
    return out;
}

/* The s16 key's high halfword as a float. */
f32 fn_8008B784(u32* iter)
{
    return fn_8008B78C(*iter);
}

/* The float at `addr`. */
f32 fn_8008B78C(register u32 addr)
{
    register f32 result;

    asm {
        psq_l result, 0(addr), 1, 6
    }
    return result;
}

u32* fn_8008B794(u32* out, u32* src)
{
    fn_8008B7C4(out, src);
    return out;
}

void fn_8008B7C4(u32* dst, u32* src)
{
    *dst = *src;
}

u32 fn_8008B7D0(u32 self, u32 index)
{
    u32 tmp;

    return *fn_8008B69C(&tmp, self + index * 0x6 + 0x10);
}

/* Tail entry into the u8 evaluator. */
f32 fn_8008B808(u32* self, u32* p, f32 frame, s32 isConst)
{
    return fn_8008B80C(self, p, frame, isConst);
}

f32 fn_8008B80C(u32* self, u32* p, f32 frame, s32 isConst)
{
    u32 data;
    u32 valid;

    if (isConst) {
        return *(f32*)p;
    }
    data = fn_8008B078((u32)self, *p);
    valid = IS_VALID_PTR(data);
    if (!valid) {
        nw4r::db::Panic("g3d_resanmchr.cpp", 0x1B7, "NW4R:Pointer Error\npAnm(=%p) is not valid pointer.", data);
    }
    return fn_8008B95C(data, frame);
}

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
        nw4r::db::Panic("g3d_resanmchr.cpp", 0xFC, "NW4R:Pointer Error\npAnm(=%p) is not valid pointer.", self);
    }
    firstIdx = fn_8008C000(self, 0);
    fn_8008BFC4(&first, &firstIdx);
    lastIdx = fn_8008C000(self, ch->count - 1);
    fn_8008BFC4(&last, &lastIdx);
    if (frame <= fn_8008BF5C(&first)) {
        return fn_8008BF08(&first, (const f32*)self);
    }
    if (fn_8008BF5C(&last) <= frame) {
        return fn_8008BF08(&last, (const f32*)self);
    }
    delta = frame - fn_8008BF5C(&first);
    pos = math_f32_to_u16(ch->rate * (delta * anim_tick_angle(ch->count)));
    if (pos > ch->count - 1) {
        nw4r::db::Panic("g3d_resanmchr.cpp", 0x11A, "estimatePos is out of bounds(%f)\nestimatePos <= %f not satisfied.", (f64)pos, (f64)(ch->count - 1));
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
    frameValid = (frame >= loFrame) && (frame <= hiFrame);
    if (!frameValid) {
        nw4r::db::Panic("g3d_resanmchr.cpp", 0x150, "frame is out of bounds(%f)\n%f <= frame <= %f not satisfied.", frame, loFrame, hiFrame);
    }
    span = frame - loFrame;
    f32 invRange = math_reciprocal(hiFrame - loFrame);
    return fn_8008AED0(curVal, curTan, nextVal, nextTan, span * invRange, span);
}

/* Extracts the packed 12-bit angle of `iter` and evaluates it. */
f32 fn_8008BDAC(u32* iter)
{
    s32 v = (s32)(*(u32*)(*iter) & 0xFFF);
    s16 packed = (s16)((v << 20) >> 20);

    return fn_8008B78C((u32)&packed);
}

u32 fn_8008BDEC(u32* iter, s32 index)
{
    u32 tmp;

    return *fn_8008BE24(&tmp, *iter + index * 0x4);
}

u32* fn_8008BE24(u32* out, u32 v)
{
    fn_8008BE54(out, v);
    return out;
}

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

u8 fn_8008BE88(f32 value)
{
    return fn_8008BE8C(value);
}

u8 fn_8008BE8C(f32 value)
{
    u8 out;

    fn_8008BEBC(&value, &out);
    return out;
}

void fn_8008BEBC(const f32* value, u8* out)
{
    *out = fn_8008BEF0(*value);
}

/* Truncates a float to a byte by reading the stored word's high byte. */
u8 fn_8008BEF0(register f32 value)
{
    f32 buffer;
    register f32* pBuffer = &buffer;
    register u8 result;

    asm {
        psq_st value, 0(pBuffer), 1, 2
        lbz result, 0(pBuffer)
    }
    return result;
}

/* Extracts the packed 12-bit value of the byte channel's key and evaluates it, scaled and biased. */
f32 fn_8008BF08(u32* iter, const f32* self)
{
    u16 packed = (u16)((*(u32*)*iter & 0xFFF000) >> 12);

    return self[2] * fn_8008B75C((u32)&packed) + self[3];
}

f32 fn_8008BF5C(u32* iter)
{
    return fn_8008BF64(*iter);
}

f32 fn_8008BF64(u32 addr)
{
    f32 out;

    fn_8008BF8C(addr, &out);
    return out;
}

void fn_8008BF8C(u32 addr, f32* out)
{
    *out = fn_8008BFBC(addr);
}

/* The float at `addr` for the byte channel. */
f32 fn_8008BFBC(register u32 addr)
{
    register f32 result;

    asm {
        psq_l result, 0(addr), 1, 2
    }
    return result;
}

u32* fn_8008BFC4(u32* out, u32* src)
{
    fn_8008BFF4(out, src);
    return out;
}

void fn_8008BFF4(u32* dst, u32* src)
{
    *dst = *src;
}

u32 fn_8008C000(u32 self, u32 index)
{
    u32 tmp;

    return *fn_8008BE24(&tmp, self + index * 0x4 + 0x10);
}

/* Reads one node's three rotation components from the value cells `keys` (constants, channel pointers or the
 * frame-indexed tables of `info`'s frame count, by the `RotateType`, flag bits 27-29), builds the rotation
 * matrix into `mtx`, stores the raw angles in `rawRot` and returns the advanced cursor. */
u32* fn_8008C038(f32* mtx, f32* rawRot, const u16* info, ResAnmChrNodeData* self, f32* p, f32 frame)
{
    u32 flags = self->flags;
    u32 rotType = flags & 0x38000000;
    f32 v0;
    f32 v1;
    f32 v2;

    switch (rotType) {
    case 0x00000000:
        v0 = *(f32*)p++;
        v1 = *(f32*)p++;
        v2 = *(f32*)p++;
        break;
    case 0x08000000:
        v0 = fn_8008B808((u32*)self, (u32*)p++, frame, (flags & 0x10000) != 0);
        v1 = fn_8008B808((u32*)self, (u32*)p++, frame, (flags & 0x20000) != 0);
        v2 = fn_8008B808((u32*)self, (u32*)p++, frame, (flags & 0x40000) != 0);
        break;
    case 0x10000000:
        v0 = fn_8008B0AC((u32*)self, (u32*)p++, frame, (flags & 0x10000) != 0);
        v1 = fn_8008B0AC((u32*)self, (u32*)p++, frame, (flags & 0x20000) != 0);
        v2 = fn_8008B0AC((u32*)self, (u32*)p++, frame, (flags & 0x40000) != 0);
        break;
    case 0x18000000:
        v0 = fn_8008A938((u32*)self, (u32*)p++, frame, (flags & 0x10000) != 0);
        v1 = fn_8008A938((u32*)self, (u32*)p++, frame, (flags & 0x20000) != 0);
        v2 = fn_8008A938((u32*)self, (u32*)p++, frame, (flags & 0x40000) != 0);
        break;
    case 0x20000000:
        v0 = fn_8008CB98((u32)self, (u32*)p++, *info, frame, (flags & 0x10000) != 0);
        v1 = fn_8008CB98((u32)self, (u32*)p++, *info, frame, (flags & 0x20000) != 0);
        v2 = fn_8008CB98((u32)self, (u32*)p++, *info, frame, (flags & 0x40000) != 0);
        break;
    case 0x28000000:
        v0 = fn_8008C800((u32)self, (u32*)p++, *info, frame, (flags & 0x10000) != 0);
        v1 = fn_8008C800((u32)self, (u32*)p++, *info, frame, (flags & 0x20000) != 0);
        v2 = fn_8008C800((u32)self, (u32*)p++, *info, frame, (flags & 0x40000) != 0);
        break;
    case 0x30000000:
        v0 = fn_8008C498((u32)self, (u32*)p++, *info, frame, (flags & 0x10000) != 0);
        v1 = fn_8008C498((u32)self, (u32*)p++, *info, frame, (flags & 0x20000) != 0);
        v2 = fn_8008C498((u32)self, (u32*)p++, *info, frame, (flags & 0x40000) != 0);
        break;
    default:
        nw4r::db::Panic("g3d_resanmchr.cpp", 0x306, "NW4R Internal error\nResAnmChrNodeData.flags has invalid RotateType(flags=0x%08X, RotateType=0x%X)", flags, rotType);
        v0 = 0.0f;
        v1 = v0;
        v2 = v0;
        break;
    }
    fn_8008C484(mtx, v0, v1, v2);
    rawRot[0] = v0;
    rawRot[1] = v1;
    rawRot[2] = v2;
    return (u32*)p;
}

/* Scales a 3-float record by a constant and forwards it. */
void fn_8008C484(f32* out, f32 x, f32 y, f32 z)
{
    nw4r::math::MTX34RotXYZFIdx(reinterpret_cast<nw4r::math::MTX34*>(out), 0.7111111f * x, 0.7111111f * y, 0.7111111f * z);
}

/* The indexed guards truncate the key index to u16 and (for the u16 channel) hand it to the
 * evaluator.  The index is carried in r5, the const flag in r6. */
f32 fn_8008C498(u32 self, u32* p, u16 index, f32 frame, s32 isConst)
{
    return fn_8008C4A0(self, p, index, frame, isConst);
}

f32 fn_8008C4A0(u32 self, u32* p, u16 index, f32 frame, s32 isConst)
{
    u32 data;
    u32 valid;

    if (isConst) {
        return *(f32*)p;
    }
    data = fn_8008B078(self, *p);
    valid = IS_VALID_PTR(data);
    if (!valid) {
        nw4r::db::Panic("g3d_resanmchr.cpp", 0x1CF, "NW4R:Pointer Error\npAnm(=%p) is not valid pointer.", data);
    }
    return fn_8008C600(data, index, frame);
}

f32 fn_8008C600(u32 self, u16 index, f32 frame)
{
    u16 pos;
    f32 v0;
    f32 delta;
    u32 valid;

    valid = IS_VALID_PTR(self);
    if (!valid) {
        nw4r::db::Panic("g3d_resanmchr.cpp", 0x16C, "NW4R:Pointer Error\npAnm(=%p) is not valid pointer.", self);
    }
    pos = math_f32_to_u16(frame);
    if (frame <= 0.0f) {
        return fn_8008C7F0(self, 0);
    }
    if (index <= pos) {
        return fn_8008C7F0(self, index);
    }
    delta = frame - (f32)pos;
    v0 = fn_8008C7F0(self, pos);
    if (0.0f == delta) {
        return v0;
    }
    f32 v1 = fn_8008C7F0(self, (u16)(pos + 1));
    return delta * (v1 - v0) + v0;
}

/* A u16-keyed float table value. */
f32 fn_8008C7F0(u32 self, u16 index)
{
    return reinterpret_cast<const f32*>(self)[index];
}

f32 fn_8008C800(u32 self, u32* p, u16 index, f32 frame, s32 isConst)
{
    return fn_8008C808(self, p, index, frame, isConst);
}

f32 fn_8008C808(u32 self, u32* p, u16 index, f32 frame, s32 isConst)
{
    u32 data;
    u32 valid;

    if (isConst) {
        return *(f32*)p;
    }
    data = fn_8008B078(self, *p);
    valid = IS_VALID_PTR(data);
    if (!valid) {
        nw4r::db::Panic("g3d_resanmchr.cpp", 0x1CF, "NW4R:Pointer Error\npAnm(=%p) is not valid pointer.", data);
    }
    return fn_8008C968(data, index, frame);
}

f32 fn_8008C968(u32 self, u16 index, f32 frame)
{
    u16 pos;
    f32 v0;
    f32 delta;
    u32 valid;

    valid = IS_VALID_PTR(self);
    if (!valid) {
        nw4r::db::Panic("g3d_resanmchr.cpp", 0x16C, "NW4R:Pointer Error\npAnm(=%p) is not valid pointer.", self);
    }
    pos = math_f32_to_u16(frame);
    if (frame <= 0.0f) {
        return fn_8008CB58((const ResAnmChrU16Channel*)self, 0);
    }
    if (index <= pos) {
        return fn_8008CB58((const ResAnmChrU16Channel*)self, index);
    }
    delta = frame - (f32)pos;
    v0 = fn_8008CB58((const ResAnmChrU16Channel*)self, pos);
    if (0.0f == delta) {
        return v0;
    }
    f32 v1 = fn_8008CB58((const ResAnmChrU16Channel*)self, (u16)(pos + 1));
    return delta * (v1 - v0) + v0;
}

/* A u16 key value scaled by the channel's rate and biased by its offset. */
f32 fn_8008CB58(const ResAnmChrU16Channel* p, u16 index)
{
    return p->scale * (f32)p->values[index] + p->offset;
}

f32 fn_8008CB98(u32 self, u32* p, u16 index, f32 frame, s32 isConst)
{
    return fn_8008CBA0(self, p, index, frame, isConst);
}

f32 fn_8008CBA0(u32 self, u32* p, u16 index, f32 frame, s32 isConst)
{
    u32 data;
    u32 valid;

    if (isConst) {
        return *(f32*)p;
    }
    data = fn_8008B078(self, *p);
    valid = IS_VALID_PTR(data);
    if (!valid) {
        nw4r::db::Panic("g3d_resanmchr.cpp", 0x1CF, "NW4R:Pointer Error\npAnm(=%p) is not valid pointer.", data);
    }
    return fn_8008CD00(data, index, frame);
}

f32 fn_8008CD00(u32 self, u16 index, f32 frame)
{
    u16 pos;
    f32 v0;
    f32 delta;
    u32 valid;

    valid = IS_VALID_PTR(self);
    if (!valid) {
        nw4r::db::Panic("g3d_resanmchr.cpp", 0x16C, "NW4R:Pointer Error\npAnm(=%p) is not valid pointer.", self);
    }
    pos = math_f32_to_u16(frame);
    if (frame <= 0.0f) {
        return fn_8008CEF0((const ResAnmChrU8Channel*)self, 0);
    }
    if (index <= pos) {
        return fn_8008CEF0((const ResAnmChrU8Channel*)self, index);
    }
    delta = frame - (f32)pos;
    v0 = fn_8008CEF0((const ResAnmChrU8Channel*)self, pos);
    if (0.0f == delta) {
        return v0;
    }
    f32 v1 = fn_8008CEF0((const ResAnmChrU8Channel*)self, (u16)(pos + 1));
    return delta * (v1 - v0) + v0;
}

/* A u8 key value scaled by the channel's rate and biased by its offset. */
f32 fn_8008CEF0(const ResAnmChrU8Channel* p, u16 index)
{
    return p->scale * (f32)p->values[index] + p->offset;
}

/* Reads one node's three translation components from the value cells `keys` (constants or channel pointers, by
 * the `TransType`, flag bits 30-31) into `out`; returns the advanced cursor. */
u32* fn_8008CF2C(f32* out, ResAnmChrNodeData* self, f32* p, f32 frame)
{
    u32 flags = self->flags;
    u32 transType = flags & 0xC0000000;
    f32 v0;
    f32 v1;
    f32 v2;

    switch (transType) {
    case 0x00000000:
        v0 = *(f32*)p++;
        v1 = *(f32*)p++;
        v2 = *(f32*)p++;
        break;
    case 0x40000000:
        v0 = fn_8008B808((u32*)self, (u32*)p++, frame, (flags & 0x80000) != 0);
        v1 = fn_8008B808((u32*)self, (u32*)p++, frame, (flags & 0x100000) != 0);
        v2 = fn_8008B808((u32*)self, (u32*)p++, frame, (flags & 0x200000) != 0);
        break;
    case 0x80000000:
        v0 = fn_8008B0AC((u32*)self, (u32*)p++, frame, (flags & 0x80000) != 0);
        v1 = fn_8008B0AC((u32*)self, (u32*)p++, frame, (flags & 0x100000) != 0);
        v2 = fn_8008B0AC((u32*)self, (u32*)p++, frame, (flags & 0x200000) != 0);
        break;
    case 0xC0000000:
        v0 = fn_8008A938((u32*)self, (u32*)p++, frame, (flags & 0x80000) != 0);
        v1 = fn_8008A938((u32*)self, (u32*)p++, frame, (flags & 0x100000) != 0);
        v2 = fn_8008A938((u32*)self, (u32*)p++, frame, (flags & 0x200000) != 0);
        break;
    default:
        nw4r::db::Panic("g3d_resanmchr.cpp", 0x35C, "NW4R Internal error\nResAnmChrNodeData.flags has invalid TransType(flags=0x%08X, TransType=0x%X)", flags, transType);
        v0 = 0.0f;
        v1 = v0;
        v2 = v0;
        break;
    }
    out[0] = v0;
    out[1] = v1;
    out[2] = v2;
    return (u32*)p;
}

/* Zeroes the 3-float record at +0x4 and clears the matrix at +0x1C. */
void fn_8008D1AC(ResAnmChrObj* self, const u16* info, ResAnmChrNodeData* nodeData, f32 frame)
{
    (void)info;
    (void)nodeData;
    (void)frame;
    self->scale[0] = 1.0f;
    self->scale[1] = 1.0f;
    self->scale[2] = 1.0f;
    mtx34_identity(self->mat);
}

void fn_8008D1C4(ResAnmChrObj* self, const u16* info, ResAnmChrNodeData* nodeData, f32 frame)
{
    f32* anmData = nodeData->anms;
    nw4r::math::VEC3 rec;

    (void)info;
    VEC3_ctor(&rec);
    self->scale[0] = 1.0f;
    self->scale[1] = 1.0f;
    self->scale[2] = 1.0f;
    mtx34_identity(self->mat);
    fn_8008CF2C(&rec.x, nodeData, anmData, frame);
    self->mat[3] = rec.x;
    self->mat[7] = rec.y;
    self->mat[11] = rec.z;
}

void fn_8008D25C(ResAnmChrObj* self, const u16* info, ResAnmChrNodeData* nodeData, f32 frame)
{
    f32* anmData = nodeData->anms;
    self->scale[0] = 1.0f;
    self->scale[1] = 1.0f;
    self->scale[2] = 1.0f;
    fn_8008C038(self->mat, self->pos, info, nodeData, anmData, frame);
    self->flags |= 0x80000000;
    self->mat[3] = 0.0f;
    self->mat[7] = 0.0f;
    self->mat[11] = 0.0f;
}

/* Re-evaluates the record from the source key.  The frame is passed through untouched (the target sets no
 * `f1`, so the original call had an undefined frame argument). */
void fn_8008D2CC(ResAnmChrObj* self, const u16* info, ResAnmChrNodeData* nodeData, f32 frame)
{
    f32* anmData = nodeData->anms;
    (void)info;
    fn_8008A664(self->scale, nodeData, anmData, frame);
    mtx34_identity(self->mat);
}

void fn_8008D30C(ResAnmChrObj* self, const u16* info, ResAnmChrNodeData* nodeData, f32 frame)
{
    f32* anmData = nodeData->anms;
    nw4r::math::VEC3 rec;

    VEC3_ctor(&rec);
    self->scale[0] = 1.0f;
    self->scale[1] = 1.0f;
    self->scale[2] = 1.0f;
    anmData = (f32*)fn_8008C038(self->mat, self->pos, info, nodeData, anmData, frame);
    self->flags |= 0x80000000;
    fn_8008CF2C(&rec.x, nodeData, anmData, frame);
    self->mat[3] = rec.x;
    self->mat[7] = rec.y;
    self->mat[11] = rec.z;
}

void fn_8008D3DC(ResAnmChrObj* self, const u16* info, ResAnmChrNodeData* nodeData, f32 frame)
{
    f32* anmData = nodeData->anms;

    anmData = (f32*)fn_8008A664(self->scale, nodeData, anmData, frame);
    fn_8008C038(self->mat, self->pos, info, nodeData, anmData, frame);
    self->flags |= 0x80000000;
    self->mat[3] = 0.0f;
    self->mat[7] = 0.0f;
    self->mat[11] = 0.0f;
}

void fn_8008D47C(ResAnmChrObj* self, const u16* info, ResAnmChrNodeData* nodeData, f32 frame)
{
    f32* anmData = nodeData->anms;
    nw4r::math::VEC3 rec;

    (void)info;
    VEC3_ctor(&rec);
    anmData = (f32*)fn_8008A664(self->scale, nodeData, anmData, frame);
    mtx34_identity(self->mat);
    fn_8008CF2C(&rec.x, nodeData, anmData, frame);
    self->mat[3] = rec.x;
    self->mat[7] = rec.y;
    self->mat[11] = rec.z;
}

void fn_8008D528(ResAnmChrObj* self, const u16* info, ResAnmChrNodeData* nodeData, f32 frame)
{
    f32* anmData = nodeData->anms;
    nw4r::math::VEC3 rec;

    VEC3_ctor(&rec);
    anmData = (f32*)fn_8008A664(self->scale, nodeData, anmData, frame);
    anmData = (f32*)fn_8008C038(self->mat, self->pos, info, nodeData, anmData, frame);
    fn_8008CF2C(&rec.x, nodeData, anmData, frame);
    self->flags |= 0x80000000;
    self->mat[3] = rec.x;
    self->mat[7] = rec.y;
    self->mat[11] = rec.z;
}

/* 0x8008D5FC (0x3BC): evaluates node `id`'s scale, rotation and translation at `frame` into `pResult`. */
void nw4r::g3d::ResAnmChr::GetAnmResult(ChrAnmResult* pResult, u32 id, f32 frame) const
{
    u32 valid;
    bool finite;
    ResAnmChrNodeData* nodeData;
    u32 flags;

    valid = IS_VALID_PTR(this);
    if (!valid) {
        nw4r::db::Panic("g3d_resanmchr.cpp", 0x478, "NW4R:Pointer Error\nthis(=%p) is not valid pointer.", this);
    }
    valid = IS_VALID_PTR(pResult);
    if (!valid) {
        nw4r::db::Panic("g3d_resanmchr.cpp", 0x479, "NW4R:Pointer Error\npResult(=%p) is not valid pointer.", pResult);
    }
    finite = false;
    if (__fpclassifyf(frame) > 2 && __fpclassifyf(frame) != 1) {
        finite = true;
    }
    if (!finite) {
        nw4r::db::Panic("g3d_resanmchr.cpp", 0x47A, "NW4R:Floating Point Value Error(%f)\nframe is infinite or nan.", frame);
    }
    nodeData = fn_8008D9B8((u32)this, id);
    valid = IS_VALID_PTR(nodeData);
    if (!valid) {
        nw4r::db::Panic("g3d_resanmchr.cpp", 0x47D, "NW4R:Pointer Error\nnodeData(=%p) is not valid pointer.", nodeData);
    }
    const u16* info = &ref().numFrame;
    flags = nodeData->flags;
    pResult->flags = flags & 0x1FFF;
    res_anm_chr_get_result_table[(flags & 0x01C00000) >> 22](reinterpret_cast<ResAnmChrObj*>(pResult), info, nodeData, frame);
}

/* Looks up node `idx`'s animation record in the resource's node dictionary (no pointer guard). */
ResAnmChrNodeData* fn_8008D9B8(u32 self, u32 idx)
{
    u32 tmp = res_anm_chr_ofs_to_dic(reinterpret_cast<const nw4r::g3d::ResAnmChr*>(self), reinterpret_cast<const nw4r::g3d::ResAnmChr*>(self)->ref().toChrDataDic);

    return static_cast<ResAnmChrNodeData*>((*reinterpret_cast<nw4r::g3d::ResDic*>(&tmp))[(int)idx]);
}

void fn_8008DA10(ResAnmChrObj* self, f32* out)
{
    u32 valid;

    valid = IS_VALID_PTR(self);
    if (!valid) {
        nw4r::db::Panic("g3d_resanmchr.cpp", 0x499, "NW4R:Pointer Error\nthis(=%p) is not valid pointer.", self);
    }
    valid = IS_VALID_PTR(out);
    if (!valid) {
        nw4r::db::Panic("g3d_resanmchr.cpp", 0x49A, "NW4R:Pointer Error\npScale(=%p) is not valid pointer.", out);
    }
    if (self->flags & 8) {
        out[0] = 1.0f;
        out[1] = 1.0f;
        out[2] = 1.0f;
    } else {
        copyVec3((nw4r::math::VEC3*)out, (const nw4r::math::VEC3*)self->scale);
    }
}

s32 fn_8008DC4C(ResAnmChrObj* self, f32* out)
{
    u32 valid;
    f32 r;

    valid = IS_VALID_PTR(self);
    if (!valid) {
        nw4r::db::Panic("g3d_resanmchr.cpp", 0x4BB, "NW4R:Pointer Error\nthis(=%p) is not valid pointer.", self);
    }
    valid = IS_VALID_PTR(out);
    if (!valid) {
        nw4r::db::Panic("g3d_resanmchr.cpp", 0x4BC, "NW4R:Pointer Error\npRot(=%p) is not valid pointer.", out);
    }
    if (self->flags & 0x20) {
        out[0] = 0.0f;
        out[1] = 0.0f;
        out[2] = 0.0f;
        return 1;
    }
    if (self->flags & 0x80000000) {
        copyVec3((nw4r::math::VEC3*)out, (const nw4r::math::VEC3*)self->pos);
        return 1;
    }
    r = sqrt_f32(1.0f - self->mat[8] * self->mat[8]);
    if (r == 0.0f) {
        out[0] = fn_8008DF64(self->mat[2] + self->mat[5], self->mat[6] + self->mat[1]);
        out[1] = f32_select_nonneg(self->mat[8], -90.0f, 90.0f);
        out[2] = fn_8008DF64(self->mat[2] + self->mat[5], self->mat[6] - self->mat[1]);
    } else {
        out[0] = fn_8008DF64(self->mat[9], self->mat[10]);
        out[1] = fn_8008DF64(-self->mat[8], r);
        out[2] = fn_8008DF64(self->mat[4], self->mat[0]);
    }
    return 0;
}

/* The float normaliser. */
f32 fn_8008DF64(f32 a, f32 b)
{
    return 1.40625f * nw4r::math::Atan2FIdx(a, b);
}

void fn_8008DF8C(ResAnmChrObj* self, f32* out)
{
    u32 valid;

    valid = IS_VALID_PTR(self);
    if (!valid) {
        nw4r::db::Panic("g3d_resanmchr.cpp", 0x4F4, "NW4R:Pointer Error\nthis(=%p) is not valid pointer.", self);
    }
    valid = IS_VALID_PTR(out);
    if (!valid) {
        nw4r::db::Panic("g3d_resanmchr.cpp", 0x4F5, "NW4R:Pointer Error\npRot(=%p) is not valid pointer.", out);
    }
    if (self->flags & 0x20) {
        nw4r::math::MTX33Identity(reinterpret_cast<nw4r::math::MTX33*>(out));
    } else {
        nw4r::math::MTX34ToMTX33(reinterpret_cast<nw4r::math::MTX33*>(out), reinterpret_cast<const nw4r::math::MTX34*>(self->mat));
    }
}

void fn_8008E1C0(ResAnmChrObj* self, f32* out)
{
    u32 valid;

    valid = IS_VALID_PTR(self);
    if (!valid) {
        nw4r::db::Panic("g3d_resanmchr.cpp", 0x50D, "NW4R:Pointer Error\nthis(=%p) is not valid pointer.", self);
    }
    valid = IS_VALID_PTR(out);
    if (!valid) {
        nw4r::db::Panic("g3d_resanmchr.cpp", 0x50E, "NW4R:Pointer Error\npTrans(=%p) is not valid pointer.", out);
    }
    if (self->flags & 0x40) {
        out[0] = 0.0f;
        out[1] = 0.0f;
        out[2] = 0.0f;
    } else {
        out[0] = self->mat[3];
        out[1] = self->mat[7];
        out[2] = self->mat[11];
    }
}

void fn_8008E408(ResAnmChrObj* self, f32* out)
{
    u32 valid;
    u32 flags;

    valid = IS_VALID_PTR(self);
    if (!valid) {
        nw4r::db::Panic("g3d_resanmchr.cpp", 0x52A, "NW4R:Pointer Error\nthis(=%p) is not valid pointer.", self);
    }
    valid = IS_VALID_PTR(out);
    if (!valid) {
        nw4r::db::Panic("g3d_resanmchr.cpp", 0x52B, "NW4R:Pointer Error\npRT(=%p) is not valid pointer.", out);
    }
    flags = self->flags;
    if (flags & 0x20) {
        if (flags & 0x40) {
            mtx34_identity(out);
        } else {
            mtx34_identity(out);
            out[3] = self->mat[3];
            out[7] = self->mat[7];
            out[11] = self->mat[11];
        }
    } else if (flags & 0x40) {
        mtx34_copy_ps((Mtx34*)out, (const Mtx34*)self->mat);
        out[3] = 0.0f;
        out[7] = 0.0f;
        out[11] = 0.0f;
    } else {
        mtx34_copy_ps((Mtx34*)out, (const Mtx34*)self->mat);
    }
}

void fn_8008E698(ResAnmChrObj* self, f32* out)
{
    u32 valid;

    valid = IS_VALID_PTR(self);
    if (!valid) {
        nw4r::db::Panic("g3d_resanmchr.cpp", 0x55B, "NW4R:Pointer Error\nthis(=%p) is not valid pointer.", self);
    }
    valid = IS_VALID_PTR(out);
    if (!valid) {
        nw4r::db::Panic("g3d_resanmchr.cpp", 0x55C, "NW4R:Pointer Error\npMtx(=%p) is not valid pointer.", out);
    }
    fn_8008E408(self, out);
    if (!(self->flags & 8)) {
        nw4r::math::MTX34Scale(reinterpret_cast<nw4r::math::MTX34*>(out), reinterpret_cast<const nw4r::math::MTX34*>(out), reinterpret_cast<const nw4r::math::VEC3*>(self->scale));
    }
}

void fn_8008E8D0(ResAnmChrObj* self, const f32* out)
{
    u32 valid;
    u32 t;

    valid = IS_VALID_PTR(self);
    if (!valid) {
        nw4r::db::Panic("g3d_resanmchr.cpp", 0x572, "NW4R:Pointer Error\nthis(=%p) is not valid pointer.", self);
    }
    valid = IS_VALID_PTR(out);
    if (!valid) {
        nw4r::db::Panic("g3d_resanmchr.cpp", 0x573, "NW4R:Pointer Error\npScale(=%p) is not valid pointer.", out);
    }
    if (out[0] == 1.0f && out[1] == 1.0f && out[2] == 1.0f) {
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
    u32 flags;
    u32 mask;

    valid = IS_VALID_PTR(self);
    if (!valid) {
        nw4r::db::Panic("g3d_resanmchr.cpp", 0x59A, "NW4R:Pointer Error\nthis(=%p) is not valid pointer.", self);
    }
    valid = IS_VALID_PTR(out);
    if (!valid) {
        nw4r::db::Panic("g3d_resanmchr.cpp", 0x59B, "NW4R:Pointer Error\npRot(=%p) is not valid pointer.", out);
    }
    if (out[0] == 0.0f && out[1] == 0.0f && out[2] == 0.0f) {
        mask = 0x20;
        flags = self->flags;
        if (flags & 0x40) {
            mask |= 4;
            if (flags & 8) {
                mask |= 2;
            }
        }
        self->flags |= mask;
        self->mat[0] = 1.0f;
        self->mat[1] = 0.0f;
        self->mat[2] = 0.0f;
        self->mat[4] = 0.0f;
        self->mat[5] = 1.0f;
        self->mat[6] = 0.0f;
        self->mat[8] = 0.0f;
        self->mat[9] = 0.0f;
        self->mat[10] = 1.0f;
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
    u32 flags;
    u32 mask;

    valid = IS_VALID_PTR(self);
    if (!valid) {
        nw4r::db::Panic("g3d_resanmchr.cpp", 0x5DB, "NW4R:Pointer Error\nthis(=%p) is not valid pointer.", self);
    }
    valid = IS_VALID_PTR(out);
    if (!valid) {
        nw4r::db::Panic("g3d_resanmchr.cpp", 0x5DC, "NW4R:Pointer Error\npRot(=%p) is not valid pointer.", out);
    }
    if (out[0] == 1.0f && out[1] == 0.0f && out[2] == 0.0f &&
        out[3] == 0.0f && out[4] == 1.0f && out[5] == 0.0f &&
        out[6] == 0.0f && out[7] == 0.0f && out[8] == 1.0f) {
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
    nw4r::math::MTX33ToMTX34(reinterpret_cast<nw4r::math::MTX34*>(self->mat), reinterpret_cast<const nw4r::math::MTX33*>(out));
    self->flags &= 0x7FFFFFFF;
}

void fn_8008F148(ResAnmChrObj* self, const f32* out)
{
    u32 valid;
    u32 mask;

    valid = IS_VALID_PTR(self);
    if (!valid) {
        nw4r::db::Panic("g3d_resanmchr.cpp", 0x607, "NW4R:Pointer Error\nthis(=%p) is not valid pointer.", self);
    }
    valid = IS_VALID_PTR(out);
    if (!valid) {
        nw4r::db::Panic("g3d_resanmchr.cpp", 0x608, "NW4R:Pointer Error\npTrans(=%p) is not valid pointer.", out);
    }
    if (out[0] == 0.0f && out[1] == 0.0f && out[2] == 0.0f) {
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
    }
    self->mat[3] = out[0];
    self->mat[7] = out[1];
    self->mat[11] = out[2];
}

void fn_8008F3DC(ResAnmChrObj* self, const void* src)
{
    u32 valid;
    nw4r::math::VEC3 rec;
    f32 norm[3];

    valid = IS_VALID_PTR(self);
    if (!valid) {
        nw4r::db::Panic("g3d_resanmchr.cpp", 0x671, "NW4R:Pointer Error\nthis(=%p) is not valid pointer.", self);
    }
    valid = IS_VALID_PTR(src);
    if (!valid) {
        nw4r::db::Panic("g3d_resanmchr.cpp", 0x672, "NW4R:Pointer Error\npMtx(=%p) is not valid pointer.", src);
    }
    setVec3(&rec, fn_8008F6C4((u32)src, 0), fn_8008F6C4((u32)src, 1), fn_8008F6C4((u32)src, 2));
    fn_8008E8D0(self, (const f32*)&rec);
    if (self->flags & 8) {
        mtx34_copy_ps((Mtx34*)self->mat, (const Mtx34*)src);
    } else {
        setVec3((nw4r::math::VEC3*)norm, math_reciprocal(rec.x), math_reciprocal(rec.y), math_reciprocal(rec.z));
        nw4r::math::MTX34Scale(reinterpret_cast<nw4r::math::MTX34*>(self->mat), static_cast<const nw4r::math::MTX34*>(src), reinterpret_cast<const nw4r::math::VEC3*>(norm));
    }
    self->flags &= 0x7FFFFFFF;
}

/* The squared-length helper. */
f32 fn_8008F6C4(u32 self, s32 index)
{
    const f32* p = (const f32*)self + index;

    return sqrt_f32(p[0] * p[0] + p[4] * p[4] + p[8] * p[8]);
}

} /* extern "C" */
