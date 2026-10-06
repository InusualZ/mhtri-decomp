/*
 * ef/ef_cylinder.cpp - the cylinder emitter form: `fn_800CB948` emits one particle per step of a count loop,
 *   `fn_800CBFB0` is the entry that guards its three pointers, derives the emission parameters and drives it.
 * RANGE. .text 0x800CB948-0x800CC5B0 (2 functions); extab 0x8000A56C-0x8000A57C, extabindex 0x80023BC8-0x80023BE0,
 *   .data 0x80594D20-0x80594DE0 (the `__FILE__` string "ef_cylinder.cpp" first), .sdata2 0x80796270-0x807962B0.
 * FLAGS. `cflags_main`; file-wide `#pragma peephole off` (retail keeps the `li r0,<slot>; psq_lx` epilogue) and
 *   `#pragma fp_contract off` (retail has no fused `a*b+c`).
 * NAMES. The map has only `fn_` stems for the range.
 * RESIDUALS. `.text` is byte-identical.
 *   `.sdata2` is retail's run except that the 2^52 `(f32)(u16)` constant pools last (retail's
 *   `lbl_80796288`/`lbl_80796290` are adjacent), so the pool references read our `@N` labels.
 *   flipcheck: `.data` claimed, not emitted (`-pool off`, which has no pragma, is what reaches retail's per-string
 *   `lis`/`addi`).
 * SHAPES. The float constants are literals, so MWCC hoists them into registers; the `|scale|` clamps are
 *   `fabsf(x) > eps ? x : eps`, which loads the floor after the call as retail does.
 * SHAPES. The particle manager is a class whose vtable pointer follows a 0x1C-byte non-polymorphic head; the spawn
 *   is its virtual `CreateParticle`, taking the position and velocity by value: the call copies them (velocity
 *   first), evaluates the momentum before the life and dispatches through r3, as in retail.  `ef_form_calc_life`
 *   is declared with its owner's argument order and u16 result.
 * SHAPES. The emission's float argument precedes the last integer one (nw4r's `Emission(..., u16 life, f32 lifeRnd,
 *   const MTX34* space)`), which orders the prologue's moves; the map rows carry that mangling
 *   (the helper `fn_800CB948` takes the same order).
 * SHAPES. `#line 49` and `#line 140` put the two functions' `CHECK_PTR` sites on retail's lines 49-51 and 140-142.
 */

#pragma peephole off
#pragma fp_contract off

#include "types.h"
#include "nw4r/math.h" /* nw4r::math::VEC3 - the vector record these bodies work on (rule 11) */
#include "ef/ef_torus.h" /* fn_800C9DCC (rule 2) */
#include "ef/ef_emitter.h" /* ef_random_float (rule 2) */
#include "fn_8004CAD8.h"       /* sqrt_f32 - that unit owns the address and publishes it (rule 2) */
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */

/* The 3-float vector `nw4r/math.h` owns, spelled `VEC3` here: this unit's whole vector
 * API (`ef_vec3_normalize_to`, `assignVec3`, the spawn slot) works on it. */
typedef nw4r::math::VEC3 VEC3; /* size: 0x0C */

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
    u32 progress;         /* +0xEC  fixed-point progress read by ef_random_float */
    u8  pad_0xF0[0x08];   /* +0xF0 */
    u32 spawn_param;      /* +0xF8  forwarded to the particle's spawn slot */
    u8  spawn_extra;      /* +0xFC  address forwarded to the particle's spawn slot */
} EfWork;                 /* size: 0xFD (at least - the record continues past what this unit reads) */

/* The non-polymorphic head of the particle manager: MWCC places the vtable pointer after it, at +0x1C. */
struct EfParticleHead {
    u8  pad_0x00[0x1C];   /* +0x00 */
};                        /* size: 0x1C */

/* The particle manager `pm`: its vtable pointer sits at +0x1C and the table's +0x14 slot is the spawn call this
 * unit makes (declared only, so this unit emits no table). */
struct EfParticle : EfParticleHead {
    /* +0x1C: the vtable pointer */
    virtual void slot_0x08();
    virtual void slot_0x0C();
    virtual void slot_0x10();
    virtual void CreateParticle(u16 life, VEC3 pos, VEC3 vel, s32 space, f32 momentum, u8* inherit,
                                u32 reference, u16 remain);
};                        /* size: 0x20 */

/* `nw4r::db::Panic`, declared in its namespace so the front-end emits the map's mangling
 * (`Panic__Q24nw4r2dbFPCciPCce`). */
namespace nw4r { namespace db { void Panic(const char* file, int line, const char* fmt, ...); } }

/* nw4r::math and effect-library helpers; retail's relocations carry their plain map names, so they
 * have C linkage (`VEC3_ctor` comes from `mh3_pad.h`). */
extern "C" {
extern void ef_vec3_normalize_to(VEC3* out, VEC3* in);
extern void assignVec3(VEC3* out, VEC3* in);
extern void ef_sin_cos(f32* out_a, f32* out_b, f32 angle);
extern void ef_form_calc_velocity(s32 ctx, VEC3* out, EfWork* em, VEC3* pos, VEC3* a, VEC3* b, VEC3* c);
extern u16  ef_form_calc_life(s32 ctx, u16 id, f32 scale, EfWork* em);
extern f32  fmodf(f32 a, f32 b);
}

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

/* Emits `count` particles, rebuilding the emission transform each step and advancing the angle when the
 * effect is swept. */
void fn_800CB948(s32 ctx, EfWork* em, EfParticle* pm, s32 count, u32 flags, EfParams* params,
                 u16 id, f32 scale, s32 spawn_arg, f32 size_x, f32 size_y, f32 size_z, f32 angle,
                 f32 angle_step, f32 phase, f32 offset_y) {
    s32 i;

#line 49
    CHECK_PTR(lbl_80594D30, em);
    CHECK_PTR(lbl_80594D64, pm);
    CHECK_PTR(lbl_80594D98, params);

    for (i = 0; i < count; i++) {
        VEC3 v88, v76, v64, v52, v40;
        f32 cs, sn;
        f32 factor, rate, t;

        VEC3_ctor(&v88); /* `mh3_pad.h`'s C-linkage declaration takes nw4r::math::VEC3* */
        VEC3_ctor(&v76);
        t = ef_random_float(&em->progress);
        rate = params->rate_pct / 100.0f;
        if (flags & 0x01000000) {
            /* One argument, not two: the callee (0x80050BC0) reads only f1.  Retail's f2 at
             * 0x800CBD20 is the hoisted `1.0f - t` the else branch reuses (0x800CBD3C). */
            factor = sqrt_f32(t + (1.0f - t) * (rate * rate));
        } else {
            factor = t + rate * (1.0f - t);
        }
        if (!(flags & 0x00020000)) {
            angle = (params->range_end - params->range_begin) * ef_random_float(&em->progress);
        }
        ef_sin_cos(&cs, &sn, phase + angle);
        setVec3(&v64, cs, 0.0f, -sn);
        v88.x = size_x * (v64.x * factor);
        if (flags & 0x00020000) {
            v88.y = offset_y;
        } else {
            v88.y = size_y * ((2.0f * ef_random_float(&em->progress)) - 1.0f);
        }
        v88.z = size_z * (v64.z * factor);
        setVec3(&v52, v88.x, 0.0f, v88.z);
        ef_vec3_normalize_to(&v52, &v52);
        assignVec3(&v40, &v88);
        ef_vec3_normalize_to(&v40, &v40);
        ef_form_calc_velocity(ctx, &v76, em, &v88, &v52, &v40, &v64);
        pm->CreateParticle(ef_form_calc_life(ctx, (u16)id, scale, em), v88, v76, spawn_arg,
                           1.0f + 0.01f * (f32)em->scale_rate * ef_random_float(&em->progress),
                           &em->spawn_extra, em->spawn_param, em->spawn_id);
        if (flags & 0x00020000) {
            angle += angle_step;
        }
    }
}

/* Guards its pointers, derives the scale triplet, the parameter range and the per-step scale, then
 * sweeps the whole count or emits a single particle. */
void fn_800CBFB0(s32 ctx, EfWork* em, EfParticle* pm, s32 count, u32 flags, EfParams* params,
                 u16 id, f32 scale, s32 spawn_arg) {
    f32 scaleA, scaleB, scaleC, angle, range_phase, phase, angle_step, offset;
    s32 i;

#line 140
    CHECK_PTR(lbl_80594D30, em);
    CHECK_PTR(lbl_80594D64, pm);
    CHECK_PTR(lbl_80594D98, params);

    if (count < 1) {
        return;
    }
    scaleA = ef_fabsf(params->scale_a) > 1.1920929e-07f ? params->scale_a : 1.1920929e-07f;
    scaleB = ef_fabsf(params->scale_b) > 1.1920929e-07f ? params->scale_b : 1.1920929e-07f;
    if (flags & 0x02000000) {
        scaleC = scaleA;
    } else {
        scaleC = ef_fabsf(params->scale_c) > 1.1920929e-07f ? params->scale_c : 1.1920929e-07f;
    }
    angle = 0.0f;
    if (flags & 0x00040000) {
        phase = params->range_begin;
    } else {
        phase = 2.0f * (3.14159265f * ef_random_float(&em->progress));
    }
    if (flags & 0x00020000) {
        range_phase = fmodf(params->range_end - params->range_begin, 6.2831855f);
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
            fn_800CB948(ctx, em, pm, count, flags, params, id, scale, spawn_arg, scaleA, scaleB,
                        scaleC, angle, angle_step, phase, offset);
        }
    } else {
        fn_800CB948(ctx, em, pm, count, flags, params, id, scale, spawn_arg, scaleA, scaleB, scaleC,
                    angle, angle, phase, 0.0f);
    }
}
