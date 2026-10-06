/*
 * ef/ef_point.cpp - the point emitter form's spawn routine: validates `em`/`pm`/`params`, then spawns `count`
 *   points, each mapping the emitter's random rate through `0.66 +/- 0.34 * t` onto a circle, building the
 *   position/velocity pair (`fn_800A99B4`) and handing it to the particle manager's slot +0x14.
 * RANGE. .text 0x800CD584-0x800CDB2C (1 function); extab 0x8000A5D4-0x8000A5DC, extabindex 0x80023C64-0x80023C70,
 *   .data 0x80594F98-0x80595058 (the `__FILE__` string "ef_point.cpp" and the three pointer-error messages),
 *   .sdata2 0x80796300-0x80796328.
 * FLAGS. `cflags_main`; file-wide `#pragma fp_contract off` (retail has no fused float op: `2.0f * rate - 1.0f`,
 *   `0.66f + 0.34f * t` and `1.0f - s * s` stay `fmuls` + `fadds`/`fsubs`).
 * NAMES. The emitter fields keep `field_0xNN` names: they are opaque arguments to the spawn call here.
 * RESIDUALS. 1 partial row, `fn_800CD584__FPvP9EfEmitterP4EfPmlPvPvllf` (ours 0x580 of 0x5A8):
 *  - the FPR restores: retail emits `li r0,<off>; psq_lx` per saved register, ours `psq_l <off>(r1)` (the size
 *    gap; `ef/ef_line.cpp` gets retail's form with `#pragma peephole off`);
 *  - the argument registers r29-r31 rotate and the float constants sit one FPR lower than retail's (pi/2^52/0.01
 *    in f22-f24 and the sqrt result in f25 there);
 *  - `(...) * t` is `fmuls f0,f1,f0` where retail has `fmuls f0,f0,f1`; literal constants fix the order but lose
 *    the `lbl_8079630*` pool relocations;
 *  - the `v_a`/`v_b` copies move floats (`lfs`/`stfs`) where retail moves words (`lwz`/`stw`);
 *  - the slot +0x14 dispatch loads through the saved `pm` (`lwz r11, 0x1C(r27)`) where retail goes through r3.
 *   flipcheck: `.data` claimed, not emitted; `.sdata2` 0x8 of the claimed 0x28; `.text` 0x580 of 0x5A8.
 * SHAPES. The pool constants are declared, never defined (playbook 29): literals pool under MWCC's own names.
 * SHAPES. The pointer assert is the `if (!okN && !test) okN-1 = FALSE;` chain (six materialised BOOLs, the first
 *   `if` carrying two tests), with `top_` computed after the six BOOLs (before them the mask takes r5 and every
 *   BOOL moves up one register).
 * SHAPES. `#line 42` puts the three assert sites on lines 42-44 (`li r4,{42,43,44}`).
 */

#include "types.h"
#include "nw4r/math.h" /* nw4r::math::VEC3 - the vector record these bodies work on (rule 11) */
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "ef/ef_emitter.h" /* ef_random_float (rule 2) */

/* --------------------------------------------------------------------------------------------- */
/* Types                                                                                          */
/* --------------------------------------------------------------------------------------------- */

/* The three-float vector the ef emitter calls pass around, under the ef band's local spelling. */
typedef nw4r::math::VEC3 EfVec3; /* size: 0x0C */

/* The emitter sub-object at `em + 0xEC` whose normalised progress `ef_random_float` returns. Only its
 * address is used here; `ef_random_float` reads the u32 at +0x00. */
typedef struct EfRate {
    /* +0x00 */ u32 counter;
    /* +0x04 */ u8 pad_0x04[0x08];
} EfRate; /* size: 0x0C */

/* The emitter the point is spawned from. Only the fields `fn_800CD584` touches are evidenced, so the
 * size is a lower bound. */
typedef struct EfEmitter {
    /* +0x00 */ u8 pad_0x00[0x67];
    /* +0x67 */ s8 field_0x67;             /* scaled by 0.01 and folded into the point's scale */
    /* +0x68 */ u8 pad_0x68[0xE8 - 0x68];
    /* +0xE8 */ u16 field_0xE8;            /* passed to the parameter manager */
    /* +0xEA */ u8 pad_0xEA[0xEC - 0xEA];
    /* +0xEC */ EfRate rate;
    /* +0xF8 */ s32 field_0xF8;            /* passed to the parameter manager */
    /* +0xFC */ u8 field_0xFC;             /* address passed to the parameter manager */
} EfEmitter; /* size: 0x100 (lower bound) */

/* The parameter manager's spawn method, reached through `pm->iface->fn_0x14`. */
typedef void (*EfPmSpawn)(void *self, u16 id, EfVec3 *a, EfVec3 *b, s32 arg7, u8 *p0, s32 p1,
                          u16 p2, f32 scale);

typedef struct EfPmIface {
    /* +0x00 */ u8 pad_0x00[0x14];
    /* +0x14 */ EfPmSpawn fn_0x14;
} EfPmIface; /* size: 0x18 (lower bound) */

typedef struct EfPm {
    /* +0x00 */ u8 pad_0x00[0x1C];
    /* +0x1C */ EfPmIface *iface;
} EfPm; /* size: 0x20 (lower bound) */

/* --------------------------------------------------------------------------------------------- */
/* Referenced symbols: the strings and pool constants of this unit's claimed `.data`/`.sdata2`,
 * declared, never defined. */
/* --------------------------------------------------------------------------------------------- */

/* `nw4r::db::Panic`, declared in its namespace so the front-end emits the map's mangling
 * (`Panic__Q24nw4r2dbFPCciPCce`). */
namespace nw4r { namespace db { void Panic(const char* file, int line, const char* fmt, ...); } }
extern const char lbl_80594F98[]; /* "ef_point.cpp" */
extern const char lbl_80594FA8[]; /* "NW4R:Pointer Error\nem(=%p) is not valid pointer." */
extern const char lbl_80594FDC[]; /* "NW4R:Pointer Error\npm(=%p) is not valid pointer." */
extern const char lbl_80595010[]; /* "NW4R:Pointer Error\nparams(=%p) is not valid pointer." */

extern const f32 lbl_80796300; /* 0.0f  */
extern const f32 lbl_80796304; /* 2.0f  */
extern const f32 lbl_80796308; /* 1.0f  */
extern const f32 lbl_8079630C; /* 0.66f */
extern const f32 lbl_80796310; /* 0.34f */
extern const f32 lbl_80796314; /* 3.14159274f */
extern const f32 lbl_80796318; /* 0.01f */

/* ef/nw4r math helpers; retail's relocations carry their plain map names, so they have C linkage. */
extern "C" {
extern u16 fn_800A9FB0(void *self, u16 id, struct EfEmitter *em, f32 f);
extern void fn_800A99B4(void *self, nw4r::math::VEC3 *out, struct EfEmitter *em, nw4r::math::VEC3 *a,
                        nw4r::math::VEC3 *b, nw4r::math::VEC3 *c, nw4r::math::VEC3 *d);
extern f32 sqrt_f32(f32 x);
extern void ef_sin_cos(f32 *a, f32 *b, f32 angle);
extern void ef_vec3_normalize_to(nw4r::math::VEC3 *a, nw4r::math::VEC3 *b);
}

/* --------------------------------------------------------------------------------------------- */
/* Assert                                                                                         */
/* --------------------------------------------------------------------------------------------- */

/* The library's pointer assert. `addr` must fall in one of the seven mapped memory ranges; the six
 * materialised BOOLs and the two-test first `if` are the target's exact shape. */
#define NW4R_POINTER_ASSERT(ptr, msg)                                                        \
    {                                                                                        \
        BOOL ok1_ = TRUE, ok2_ = TRUE, ok3_ = TRUE, ok4_ = TRUE, ok5_ = TRUE, ok6_ = TRUE;   \
        u32 top_ = (u32)(ptr) & 0xFF000000u;                                                 \
        if (!(top_ == 0x80000000u) &&                                                        \
            !(((u32)(ptr) & 0xFF800000u) == 0x81000000u))                                    \
            ok6_ = FALSE;                                                                    \
        if (!ok6_ && !(((u32)(ptr) & 0xF8000000u) == 0x90000000u))                           \
            ok5_ = FALSE;                                                                    \
        if (!ok5_ && !(top_ == 0xC0000000u))                                                 \
            ok4_ = FALSE;                                                                    \
        if (!ok4_ && !(((u32)(ptr) & 0xFF800000u) == 0xC1000000u))                           \
            ok3_ = FALSE;                                                                    \
        if (!ok3_ && !(((u32)(ptr) & 0xF8000000u) == 0xD0000000u))                           \
            ok2_ = FALSE;                                                                    \
        if (!ok2_ && !(((u32)(ptr) & 0xFFFFC000u) == 0xE0000000u))                           \
            ok1_ = FALSE;                                                                    \
        if (!ok1_)                                                                           \
            nw4r::db::Panic(lbl_80594F98, __LINE__, msg, (ptr));                   \
    }

/* --------------------------------------------------------------------------------------------- */

#pragma fp_contract off

void fn_800CD584(void *self, EfEmitter *em, EfPm *pm, s32 count, void *unused, void *params,
                 s32 id, s32 arg7, f32 f) {
    EfVec3 v_44, v_38, v_2C, v_20, v_a, v_b;
    s32 i;

#line 42
    NW4R_POINTER_ASSERT(em, lbl_80594FA8);
    NW4R_POINTER_ASSERT(pm, lbl_80594FDC);
    NW4R_POINTER_ASSERT(params, lbl_80595010);

    if (count >= 1) {
        for (i = 0; i < count; i++) {
            f32 rate, t, s, r, scale;

            setVec3(&v_44, lbl_80796300, lbl_80796300, lbl_80796300);
            VEC3_ctor(&v_38);
            rate = ef_random_float(&em->rate.counter);
            t = lbl_80796304 * rate - lbl_80796308;
            if (t >= lbl_80796300)
                s = (lbl_8079630C + lbl_80796310 * t) * t;
            else
                s = (lbl_8079630C - lbl_80796310 * t) * t;
            v_38.x = s;
            r = sqrt_f32(lbl_80796308 - v_38.x * v_38.x);
            ef_sin_cos(&v_38.z, &v_38.y,
                        lbl_80796304 * (lbl_80796314 * ef_random_float(&em->rate.counter)));
            v_38.y = v_38.y * r;
            v_38.z = v_38.z * r;
            setVec3(&v_2C, v_38.x, lbl_80796300, v_38.z);
            ef_vec3_normalize_to(&v_2C, &v_2C);
            VEC3_ctor(&v_20);
            fn_800A99B4(self, &v_20, em, &v_44, &v_38, &v_38, &v_2C);
            v_b = v_20;
            v_a = v_44;
            scale = lbl_80796308 +
                    (lbl_80796318 * (f32)em->field_0x67) * ef_random_float(&em->rate.counter);
            pm->iface->fn_0x14(pm, fn_800A9FB0(self, (u16)id, em, f), &v_a, &v_b, arg7,
                               &em->field_0xFC, em->field_0xF8, em->field_0xE8, scale);
        }
    }
}
