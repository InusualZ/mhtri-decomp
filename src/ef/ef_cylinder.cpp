/* auto/800CB948_fn_800CB948.c - the retail `ef_cylinder.cpp` unit, 0x800CB948..0x800CC5B0.
 *
 * Two functions: fn_800CB948 emits one particle per iteration of a count loop, fn_800CBFB0 is the entry
 * point that guards its three pointers, derives the emission parameters and drives the loop.  Both are
 * `em`/`pm`/`params` guarded by the shared `CHECK_PTR` macro, whose `__LINE__` retail stamped into the
 * Panic calls (49-51 and 140-142).
 *
 * Measured with `python tools/units/recompile.py auto/800CB948_fn_800CB948.c --measure <symbol>`:
 *   fn_800CB948 97.29 % (target .text 0x668, ours 0x664)   fn_800CBFB0 96.09 % (0x600 / 0x600)
 * The instruction mix, the frames (384 / 224), every stack slot and the `extab`/`extabindex` fragments
 * are the target's.  What is left:
 *   - fn_800CB948's float registers are retail's set rotated by one: retail holds 0.01f in f18 and the
 *     `(f32)(s8)` conversion's 2^52+2^31 constant in f31, ours holds the double in f18 and 0.01f in f19,
 *     so every other float register sits one higher.  Only that constant's web priority differs.  Tried
 *     and did not move it: splitting the scale statement, a `hundredth` temporary, swapping the
 *     multiply's operands, and making the angle a modified parameter (which is what put farg4 in f23).
 *   - three fn_800CB948 instructions pick a different scratch register: the u16 narrowing of
 *     fn_800A9FB0's result goes into r4 where retail copies it to r0 first, and the spawn slot's two
 *     vtable loads use r11/r12 where retail reuses r12.  fn_800CBFB0's scheduler moves `mr r10,r31`
 *     (the 8th argument) one slot earlier than retail.
 *
 * The three pragmas below are per-unit flag deviations, each with its instruction evidence: the retail
 * object has no fused `a*b+c`, keeps the `li r0,<slot>; psq_lx` paired-single epilogue, and its
 * `__LINE__` values are the retail file's.  The declarations this file needs cannot live in a sibling
 * header (`cflags_main` has no `-gccinc`, so MWCC does not search the source's own directory), so the
 * line counter is realigned with `#line` instead of padding.
 *
 * The unit's `.data` (0x80594D20..0x80594DCD) and `.sdata2` (0x80796270..0x807962B0) runs are **not**
 * claimed in `splits.txt` yet, so the four strings are declared `extern` and never defined
 * (docs/matching.md 29).  Our object still emits `.sdata2` (0x40 B) - the float constants have to stay
 * literals for the compiler to hoist them into registers - and that pool is the retail run byte for byte
 * except that the 2^52 `(f32)(u16)` conversion constant is pooled last instead of beside the 2^52+2^31
 * one (retail's lbl_80796288/lbl_80796290 are adjacent).  Retail's `.data` is the same 0xAD bytes the
 * string literals would produce, but reaching them needs `-pool off` (without it MWCC reaches the pool
 * through a `@stringBase0` base register instead of retail's per-string `lis`/`addi`) and `-pool off`
 * has no source pragma; a `range` and `flag` request for all of this rides the outbox.
 */

/* The retail object was built with the peephole pass off and FP contraction off: its paired-single
 * epilogue keeps the `li r0,<slot>; psq_lx` form (ours folds it into `psq_l <slot>(r1)`) and every
 * `a*b+c` stays two instructions (ours fuses them into `fmadds`/`fmsubs`). */
#pragma peephole off
#pragma fp_contract off

#include "types.h"
#include "unsplit/ef.h"

/* A 3-float vector. */
typedef struct Vec {
    f32 x;                /* +0x00 */
    f32 y;                /* +0x04 */
    f32 z;                /* +0x08 */
} Vec;                    /* size: 0x0C */

/* The six-float parameter block `params`. */
typedef struct EfParams {
    f32 scale_a;         /* +0x00 */
    f32 rate_pct;        /* +0x04 */
    f32 range_begin;     /* +0x08 */
    f32 range_end;       /* +0x0C */
    f32 scale_b;         /* +0x10 */
    f32 scale_c;         /* +0x14 */
} EfParams;               /* size: 0x18 */

/* The effect work record `em`.  Only the fields this unit touches are named. */
typedef struct EfWork {
    u8  pad_0x00[0x32];   /* +0x00 */
    u16 split_count;      /* +0x32  divides the parameter range in fn_800CBFB0 */
    u8  pad_0x34[0x33];   /* +0x34 */
    s8  scale_rate;       /* +0x67  per-frame scale step, in hundredths */
    u8  pad_0x68[0x80];   /* +0x68 */
    u16 spawn_id;         /* +0xE8  forwarded to the particle's spawn slot */
    u32 progress;         /* +0xEC  fixed-point progress read by fn_800A8A08 */
    u8  pad_0xF0[0x08];   /* +0xF0 */
    u32 spawn_param;      /* +0xF8  forwarded to the particle's spawn slot */
    u8  spawn_extra;      /* +0xFC  address forwarded to the particle's spawn slot */
} EfWork;                 /* size: 0xFD (at least - the record continues past what this unit reads) */

/* The particle manager `pm`: a pointer at +0x1C to its slot table. */
typedef struct EfParticle EfParticle;

typedef struct EfParticleSlots {
    u8  pad_0x00[0x14];   /* +0x00 */
    void (*spawn)(EfParticle* self, u16 id, Vec* pos, Vec* dir, s32 param, u8* extra, /* +0x14 */
                  u32 work_param, u16 work_id, f32 scale);
} EfParticleSlots;        /* size: 0x18 */

struct EfParticle {
    u8  pad_0x00[0x1C];   /* +0x00 */
    EfParticleSlots* slots; /* +0x1C */
};                        /* size: 0x20 */

/* nw4r::db::Panic. The map already carries its real C++ mangling
 * (Panic__Q24nw4r2dbFPCciPCce), and declaring that spelling as a C++ identifier re-mangles it
 * (Panic__Q24nw4r2dbFPCciPCce__FPCciPCce) - which only shows up at LINK time, so a NonMatching
 * unit hides it until it is flipped. Declare the real thing and the front-end reproduces the
 * map's spelling exactly: tools/units/mangle.py confirms it. */
namespace nw4r { namespace db { void Panic(const char* file, int line, const char* fmt, ...); } }
extern void fn_80043EA8(void* p);
extern void fn_80041E8C(Vec* out, f32 x, f32 y, f32 z);
extern void fn_8009C484(Vec* out, Vec* in);
extern void fn_80051490(Vec* out, Vec* in);
extern void fn_8009C760(f32* out_a, f32* out_b, f32 angle);
extern void fn_800A99B4(s32 ctx, Vec* out, EfWork* em, Vec* pos, Vec* a, Vec* b, Vec* c);
extern u32  fn_800A9FB0(s32 ctx, u16 id, EfWork* em, f32 scale);
extern f32  fn_800A8A08(void* progress);
extern f32  fn_80050BC0(f32 a, f32 b);
extern f32  fn_80463F10(f32 a, f32 b);

extern char lbl_80594D20[]; /* "ef_cylinder.cpp"                .data  0x80594D20 */
extern char lbl_80594D30[]; /* "NW4R:Pointer Error\nem(=%p)..." .data  0x80594D30 */
extern char lbl_80594D64[]; /* "NW4R:Pointer Error\npm(=%p)..." .data  0x80594D64 */
extern char lbl_80594D98[]; /* "NW4R:Pointer Error\nparams(=%p)..." .data 0x80594D98 */


/* Retail's pointer validity test: the Wii's cached/uncached MEM1 and MEM2 windows plus the
 * 0xE0000000 register page.  Inlined into every caller (`-inline noauto` only inlines `inline`). */
inline int IsValidPointer(u32 ptr) {
    return ((ptr & 0xFF000000) == 0x80000000)
        || ((ptr & 0xFF800000) == 0x81000000)
        || ((ptr & 0xF8000000) == 0x90000000)
        || ((ptr & 0xFF000000) == 0xC0000000)
        || ((ptr & 0xFF800000) == 0xC1000000)
        || ((ptr & 0xF8000000) == 0xD0000000)
        || ((ptr & 0xFFFFC000) == 0xE0000000);
}

/* The file's pointer guard: the message comes from the call site, the line from `__LINE__`. */
#define CHECK_PTR(msg, ptr) \
    if (!IsValidPointer((u32)(ptr))) \
        nw4r::db::Panic(lbl_80594D20, __LINE__, msg, (ptr))

/* Emits the effect's particles: for each of `count` steps it rebuilds the emission transform from the
 * parameter block and the effect's progress, hands the result to the particle manager's spawn slot and
 * advances the emission angle when the effect is a swept one. */
void fn_800CB948(s32 ctx, EfWork* em, EfParticle* pm, s32 count, u32 flags, EfParams* params,
                 u16 id, s32 spawn_arg, f32 scale, f32 size_x, f32 size_y, f32 size_z, f32 angle,
                 f32 angle_step, f32 phase, f32 offset_y) {
    s32 i;

#line 49
    CHECK_PTR(lbl_80594D30, em);
    CHECK_PTR(lbl_80594D64, pm);
    CHECK_PTR(lbl_80594D98, params);

    for (i = 0; i < count; i++) {
        Vec v88, v76, v64, v52, v40, v28, v16;
        f32 cs, sn;
        f32 factor, rate, t;

        fn_80043EA8(&v88);
        fn_80043EA8(&v76);
        t = fn_800A8A08(&em->progress);
        rate = params->rate_pct / 100.0f;
        if (flags & 0x01000000) {
            factor = fn_80050BC0(t + (1.0f - t) * (rate * rate), 1.0f - t);
        } else {
            factor = t + rate * (1.0f - t);
        }
        if (!(flags & 0x00020000)) {
            angle = (params->range_end - params->range_begin) * fn_800A8A08(&em->progress);
        }
        fn_8009C760(&cs, &sn, phase + angle);
        fn_80041E8C(&v64, cs, 0.0f, -sn);
        v88.x = size_x * (v64.x * factor);
        if (flags & 0x00020000) {
            v88.y = offset_y;
        } else {
            v88.y = size_y * ((2.0f * fn_800A8A08(&em->progress)) - 1.0f);
        }
        v88.z = size_z * (v64.z * factor);
        fn_80041E8C(&v52, v88.x, 0.0f, v88.z);
        fn_8009C484(&v52, &v52);
        fn_80051490(&v40, &v88);
        fn_8009C484(&v40, &v40);
        fn_800A99B4(ctx, &v76, em, &v88, &v52, &v40, &v64);
        v16 = v76;
        v28 = v88;
        factor = 1.0f + 0.01f * (f32)em->scale_rate * fn_800A8A08(&em->progress);
        pm->slots->spawn(pm, (u16)fn_800A9FB0(ctx, (u16)id, em, scale), &v28, &v16, spawn_arg,
                         &em->spawn_extra, em->spawn_param, em->spawn_id, factor);
        if (flags & 0x00020000) {
            angle += angle_step;
        }
    }
}

/* The entry point: guards its pointers, then - when at least one particle was asked for - derives the
 * scale triplet, the parameter range and the per-step scale from the parameter block and the effect's
 * progress and either sweeps the whole count or emits a single particle. */
void fn_800CBFB0(s32 ctx, EfWork* em, EfParticle* pm, s32 count, u32 flags, EfParams* params,
                 u16 id, s32 spawn_arg, f32 scale) {
    f32 scaleA, scaleB, scaleC, angle, range_phase, phase, angle_step, offset;
    s32 i;

#line 140
    CHECK_PTR(lbl_80594D30, em);
    CHECK_PTR(lbl_80594D64, pm);
    CHECK_PTR(lbl_80594D98, params);

    if (count < 1) {
        return;
    }
    scaleA = 1.1920929e-07f;
    if (fn_800C9DCC(params->scale_a) > scaleA) {
        scaleA = params->scale_a;
    }
    scaleB = 1.1920929e-07f;
    if (fn_800C9DCC(params->scale_b) > scaleB) {
        scaleB = params->scale_b;
    }
    if (flags & 0x02000000) {
        scaleC = scaleA;
    } else {
        scaleC = 1.1920929e-07f;
        if (fn_800C9DCC(params->scale_c) > scaleC) {
            scaleC = params->scale_c;
        }
    }
    angle = 0.0f;
    if (flags & 0x00040000) {
        phase = params->range_begin;
    } else {
        phase = 2.0f * (3.14159265f * fn_800A8A08(&em->progress));
    }
    if (flags & 0x00020000) {
        range_phase = fn_80463F10(params->range_end - params->range_begin, 6.2831855f);
        if (range_phase < 0.000191747604f || range_phase > 6.28299379f || em->split_count == 1) {
            angle_step = (params->range_end - params->range_begin) / (f32)em->split_count;
        } else {
            angle_step = (params->range_end - params->range_begin) / (f32)(em->split_count - 1);
        }
        for (i = 0; i < count; i++) {
            if (count <= 1) {
                offset = 0.0f;
            } else {
                offset = 2.0f * (scaleB * ((f32)i / (f32)(count - 1) - 0.5f));
            }
            fn_800CB948(ctx, em, pm, count, flags, params, id, spawn_arg, scale, scaleA, scaleB,
                        scaleC, angle, angle_step, phase, offset);
        }
    } else {
        fn_800CB948(ctx, em, pm, count, flags, params, id, spawn_arg, scale, scaleA, scaleB, scaleC,
                    angle, angle, phase, 0.0f);
    }
}
