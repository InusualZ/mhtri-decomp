/*
 * auto/800898B0_fn_800898B0.c - nw4r g3d animation channel evaluation, 18 functions,
 * 0x800898B0..0x8008A28C.
 *
 * The unit's panic strings name nw4r's files: "g3d_resanm.cpp" (fn_800898B0 / fn_80089C6C),
 * "g3d_resanmamblight.cpp" (fn_8008A000), "g3d_resanmscn_ac.h" (fn_80089F94) and
 * "g3d_resuser_ac.h" (fn_8008A220). The map has no names here, so the functions keep their
 * provisional fn_* names (docs/plan.md 6.5 rule 7 is exempt under src/auto/).
 *
 * Registered NonMatching in configure.py, lib auto (cflags_main: -O3 -inline noauto
 * -Cpp_exceptions on); the retail object carries extab 0x58 + extabindex 0x84.
 *
 * Load-bearing source shapes:
 *   - the retail object contains no record-form instruction at all (its compares are
 *     `clrlwi rX,rY,30` + `cmpwi rX,0` where cflags_main's peephole fuses them into
 *     `clrlwi.`), so the file carries `#pragma peephole off` (playbook 21 / 28);
 *   - the retail object contains no fused multiply-add, so `#pragma fp_contract off`;
 *   - nw4r's pointer-validity macro is consumed as a *value* (`u32 valid = IS_VALID_PTR(p);
 *     if (!valid) ...`), which is what makes MWCC materialise the flag cascade instead of a
 *     short-circuit branch chain.
 *
 * Residual: fn_80089F78 only.  The retail `(f32)->s16` conversion is `addi r3,r1,8` +
 * `psq_st f1,0(r3),1,qr5` + `lha`; every compiler in build/compilers (all GC and Wii
 * versions) emits `fctiwz`/`stfd`/`lwz` for `(s16)value`, no source shape tried reproduces the
 * quantized store, and the retail object's `.comment` is version 0x0e against our 0x0f - a
 * compiler-build difference, not a flag one (`.pi/notes/800898b0-fn-800898b0-acfb.md`).
 */

#pragma peephole off
#pragma fp_contract off

typedef unsigned char u8;
typedef unsigned short u16;
typedef signed short s16;
typedef unsigned long u32;
typedef signed long s32;
typedef float f32;

/* nw4r::db::Panic(const char*, int, const char*, ...) */
extern void Panic__Q24nw4r2dbFPCciPCce(const char *file, int line, const char *msg, ...);

/* nw4r math / resource helpers owned by unsplit units. */
extern f32 fn_800501E4(u16 value);
extern u16 fn_80082F18(f32 value);
extern f32 fn_800610AC(f32 value);
extern void fn_8005B1B4(u32 *self, u32 value);
extern u32 *fn_80066C8C(void *obj);
extern f32 fn_80463F34(f32 *out, f32 frame);
extern u32 fn_800651BC(void *self);
extern void dtor_8005B228(void *self, s32 flag);

/* A single animation key. */
typedef struct {
    f32 frame;   /* +0x00 */
    f32 value;   /* +0x04 */
    f32 tangent; /* +0x08 */
} ResAnmChrKey; /* size: 0xC */

/* A float channel: a key count, a rate and the key array. */
typedef struct {
    u16 count;           /* +0x00 */
    u16 pad_0x02;        /* +0x02 */
    f32 rate;            /* +0x04 */
    ResAnmChrKey keys[]; /* +0x08 */
} ResAnmChrChannel; /* size: 0x8 */

/* A 4-byte color. */
typedef struct {
    u8 b0; /* +0x00 */
    u8 b1; /* +0x01 */
    u8 b2; /* +0x02 */
    u8 b3; /* +0x03 */
} ResAnmClr; /* size: 0x4 */

/* The resolved animation object fn_80066C8C returns: a type word, a frame count and the
 * channel data at +0x18. */
typedef struct {
    u32 unused_0x00;  /* +0x00 */
    u32 field_0x04;   /* +0x04 */
    u8 pad_0x08[0xC]; /* +0x08 */
    u32 field_0x14;   /* +0x14 */
    u32 field_0x18;   /* +0x18 */
} ResAnmAmbLightData; /* size: 0x1C */

/* The result object fn_8008A000 fills in. */
typedef struct {
    u32 field_0x00; /* +0x00 */
    u32 field_0x04; /* +0x04 */
} ResAnmAmbLightResult; /* size: 0x8 */

/* Forward declarations for the unit's own functions. */
f32 fn_800898B0(ResAnmChrChannel *pData, f32 frame);
f32 fn_80089B88(f32 v0, f32 t0, f32 v1, f32 t1, f32 delta, f32 span);
u32 fn_80089C6C(u32 *arg0, f32 frame);
ResAnmClr *fn_80089E9C(ResAnmClr *self, u32 b0, u32 b1, u32 b2, u32 b3);
void fn_80089ECC(ResAnmClr *self, u32 b0, u32 b1, u32 b2, u32 b3);
u8 fn_80089EF0(s32 from, s32 to, s16 factor);
s16 fn_80089F14(f32 value);
void fn_80089F44(f32 *value, s16 *out);
s16 fn_80089F78(f32 value);
f32 fn_80089F90(f32 *out, f32 frame);
void *fn_80089F94(void *self, u32 value);
void fn_80089FF8(u32 *self, u32 value);
void fn_8008A000(void *arg0, ResAnmAmbLightResult *pResult, f32 frame);
s32 fn_8008A188(u32 *self, s32 flag, f32 frame);
f32 fn_8008A1A8(u16 *count, f32 frame);
s32 fn_8008A204(u32 *self, s32 offset);
void *fn_8008A220(void *self, u32 value);
void fn_8008A284(u32 *self, u32 value);

/*
 * The pointer-range check nw4r's resource macros expand to: a pointer is valid when it lies in
 * one of the Wii memory regions.
 */
#define IS_VALID_PTR(p) ( \
    (((u32)(p) & 0xFF000000) == 0x80000000) || \
    (((u32)(p) & 0xFF800000) == 0x81000000) || \
    (((u32)(p) & 0xF8000000) == 0x90000000) || \
    (((u32)(p) & 0xFF000000) == 0xC0000000) || \
    (((u32)(p) & 0xFF800000) == 0xC1000000) || \
    (((u32)(p) & 0xF8000000) == 0xD0000000) || \
    (((u32)(p) & 0xFFFFC000) == 0xE0000000))

/* Evaluates one float channel at a frame and returns the interpolated value. */
f32 fn_800898B0(ResAnmChrChannel *pData, f32 frame)
{
    ResAnmChrKey *pKey;
    ResAnmChrKey *pLast;
    u16 pos;
    f32 delta;

    u32 valid = IS_VALID_PTR(pData);

    if (!valid) {
        Panic__Q24nw4r2dbFPCciPCce("g3d_resanm.cpp", 105,
            "NW4R:Pointer Error\npData(=%p) is not valid pointer.", pData);
    }
    pLast = &pData->keys[pData->count - 1];
    if (frame <= pData->keys[0].frame) {
        return pData->keys[0].value;
    }
    if (pLast->frame <= frame) {
        return pLast->value;
    }
    delta = frame - pData->keys[0].frame;
    pos = fn_80082F18(delta * fn_800501E4(pData->count) * pData->rate);
    if (pos > pData->count - 1) {
        Panic__Q24nw4r2dbFPCciPCce("g3d_resanm.cpp", 135,
            "estimatePos is out of bounds(%f)\nestimatePos <= %f not satisfied.",
            (f32)pos, (f32)(pData->count - 1));
    }
    pKey = &pData->keys[pos];
    if (frame < pKey->frame) {
        do {
            pKey--;
        } while (frame < pKey->frame);
    } else {
        do {
            pKey++;
        } while (pKey->frame <= frame);
        pKey--;
    }
    if (frame == pKey->frame) {
        return pKey->value;
    }
    {
        u32 frameValid = pKey->frame <= frame && frame <= pKey[1].frame;

        if (!frameValid) {
            Panic__Q24nw4r2dbFPCciPCce("g3d_resanm.cpp", 176,
                "frame is out of bounds(%f)\n%f <= frame <= %f not satisfied.", frame,
                pKey->frame, pKey[1].frame);
        }
    }
    return fn_80089B88(pKey->value, pKey->tangent, pKey[1].value, pKey[1].tangent,
                       frame - pKey->frame, pKey[1].frame - pKey->frame);
}

/* Hermite-interpolates between two keys over a span. */
f32 fn_80089B88(f32 v0, f32 t0, f32 v1, f32 t1, f32 delta, f32 span)
{
    f32 h = delta * fn_800610AC(span);
    f32 hm1 = h - 1.0f;

    return v0 + (h * (h * (((2.0f * h) - 3.0f) * (v0 - v1)))) +
           (delta * hm1 * ((hm1 * t0) + (h * t1)));
}

/* Evaluates one color channel at a frame and returns the interpolated color. */
u32 fn_80089C6C(u32 *arg0, f32 frame)
{
    f32 sp14;
    u32 sp10;
    u32 spC;
    ResAnmClr sp8;
    f32 value;
    s32 index;
    s16 factor;

    u32 valid = IS_VALID_PTR(arg0);

    if (!valid) {
        Panic__Q24nw4r2dbFPCciPCce("g3d_resanm.cpp", 206,
            "NW4R:Pointer Error\npData(=%p) is not valid pointer.", arg0);
    }
    value = (f32)fn_80089F90(&sp14, frame);
    index = (s32)sp14;
    if (value == 0.0f) {
        return arg0[index];
    }
    fn_8005B1B4(&sp10, arg0[index]);
    fn_8005B1B4(&spC, arg0[index + 1]);
    factor = fn_80089F14(32768.0f * value);
    fn_80089E9C(&sp8,
        fn_80089EF0(((ResAnmClr *)&sp10)->b0, ((ResAnmClr *)&spC)->b0, factor),
        fn_80089EF0(((ResAnmClr *)&sp10)->b1, ((ResAnmClr *)&spC)->b1, factor),
        fn_80089EF0(((ResAnmClr *)&sp10)->b2, ((ResAnmClr *)&spC)->b2, factor),
        fn_80089EF0(((ResAnmClr *)&sp10)->b3, ((ResAnmClr *)&spC)->b3, factor));
    return fn_800651BC(&sp8);
}

/* Constructs a 4-byte color from its four byte components. */
ResAnmClr *fn_80089E9C(ResAnmClr *self, u32 b0, u32 b1, u32 b2, u32 b3)
{
    fn_80089ECC(self, b0, b1, b2, b3);
    return self;
}

/* Stores a 4-byte color from its four byte components. */
void fn_80089ECC(ResAnmClr *self, u32 b0, u32 b1, u32 b2, u32 b3)
{
    self->b0 = b0;
    self->b1 = b1;
    self->b2 = b2;
    self->b3 = b3;
}

/* Linearly interpolates one byte between two values with a 15-bit factor. */
u8 fn_80089EF0(s32 from, s32 to, s16 factor)
{
    return from + ((((to & 0xFF) - (from & 0xFF)) * factor) >> 15);
}

/* Converts a float to a signed short. */
s16 fn_80089F14(f32 value)
{
    s16 out;

    fn_80089F44(&value, &out);
    return out;
}

/* Converts a float through a pointer and writes the signed short result. */
void fn_80089F44(f32 *value, s16 *out)
{
    *out = fn_80089F78(*value);
}

/* Truncates a float to a signed short. */
s16 fn_80089F78(f32 value)
{
    return (s16)value;
}

/* Tail call into the frame-index helper. */
f32 fn_80089F90(f32 *out, f32 frame)
{
    return fn_80463F34(out, frame);
}

/* Stores a pointer and asserts its 4-byte alignment. */
void *fn_80089F94(void *self, u32 value)
{
    fn_80089FF8(self, value);
    if (value & 3) {
        Panic__Q24nw4r2dbFPCciPCce("g3d_resanmscn_ac.h", 40,
            "NW4R:Failed assertion !((u32)p & 0x3)");
    }
    return self;
}

/* Stores a word. */
void fn_80089FF8(u32 *self, u32 value)
{
    *self = value;
}

/* Evaluates an ambient-light channel at a frame. */
void fn_8008A000(void *arg0, ResAnmAmbLightResult *pResult, f32 frame)
{
    ResAnmAmbLightData *res;
    u32 flags;

    u32 valid = IS_VALID_PTR(pResult);
    if (!valid) {
        Panic__Q24nw4r2dbFPCciPCce("g3d_resanmamblight.cpp", 44,
            "NW4R:Pointer Error\npResult(=%p) is not valid pointer.", pResult);
    }
    res = (ResAnmAmbLightData *)fn_80066C8C(arg0);
    flags = res->field_0x14;
    fn_8008A1A8((u16 *)(fn_8008A204((u32 *)arg0, res->field_0x04) + 0x34), frame);
    pResult->field_0x00 = flags & 3;
    pResult->field_0x04 = fn_8008A188(&res->field_0x18, (flags & 0x80000000) != 0, frame);
}

/* Returns a sub-resource pointer, or evaluates it when it has no own data. */
s32 fn_8008A188(u32 *self, s32 flag, f32 frame)
{
    if (flag) {
        return *self;
    }
    return fn_80089C6C((u32 *)((u8 *)self + *self), frame);
}

/* Clamps a frame into the range of a channel. */
f32 fn_8008A1A8(u16 *count, f32 frame)
{
    if (frame <= 0.0f) {
        return 0.0f;
    }
    if ((f32)*count <= frame) {
        return (f32)*count;
    }
    return frame;
}

/* Returns a resource offset pointer, or null when the offset is zero. */
s32 fn_8008A204(u32 *self, s32 offset)
{
    s32 base = *self;

    if (offset != 0) {
        base += offset;
        return base;
    }
    return 0;
}

/* Stores a pointer and asserts its 4-byte alignment. */
void *fn_8008A220(void *self, u32 value)
{
    fn_8008A284(self, value);
    if (value & 3) {
        Panic__Q24nw4r2dbFPCciPCce("g3d_resuser_ac.h", 87,
            "NW4R:Failed assertion !((u32)p & 0x3)");
    }
    return self;
}

/* Stores a word. */
void fn_8008A284(u32 *self, u32 value)
{
    *self = value;
}
