/*
 * ef/ef_point.cpp - the point emitter form's spawn routine: validates `em`/`pm`/`params`, then spawns `count`
 *   points, each mapping the emitter's random rate through `0.66 +/- 0.34 * t` onto a circle, building the
 *   position/velocity pair (`ef_form_calc_velocity`) and handing it to the particle manager's slot +0x14.
 * RANGE. .text 0x800CD584-0x800CDB2C (1 function); extab 0x8000A5D4-0x8000A5DC, extabindex 0x80023C64-0x80023C70,
 *   .data 0x80594F98-0x80595058 (the `__FILE__` string "ef_point.cpp" and the three pointer-error messages),
 *   .sdata2 0x80796300-0x80796328.
 * FLAGS. `cflags_main`; file-wide `#pragma peephole off` (retail's `li r0,<off>; psq_lx` FPR restores) and
 *   `#pragma fp_contract off` (retail has no fused float op: `2.0f * rate - 1.0f`,
 *   `0.66f + 0.34f * t` and `1.0f - s * s` stay `fmuls` + `fadds`/`fsubs`).
 * NAMES. The emitter fields keep `field_0xNN` names: they are opaque arguments to the spawn call here.
 * RESIDUALS. `.text` is byte-identical; the `.sdata2` literals are our pool's `@N` where retail reads the claimed
 *   `lbl_8079630*` run.
 *   flipcheck: `.data` claimed, not emitted.
 * SHAPES. The `.sdata2` constants are literals (MWCC pools and hoists them, which fixes the `(...) * t` operand
 *   order); the file-wide `#pragma peephole off` keeps retail's `li r0,<off>; psq_lx` FPR restores; the spawn
 *   position and velocity go by value to `EfPm`'s virtual `CreateParticle` (vtable pointer at +0x1C, after a
 *   0x1C-byte non-polymorphic head): the call copies them (velocity first), evaluates the momentum before the
 *   life and dispatches through r3, as in retail.
 * SHAPES. The pointer assert is the `if (!okN && !test) okN-1 = FALSE;` chain (six materialised BOOLs, the first
 *   `if` carrying two tests), with `top_` computed after the six BOOLs (before them the mask takes r5 and every
 *   BOOL moves up one register).
 * SHAPES. The emission's float argument precedes the last integer one (nw4r's `Emission(..., u16 life, f32 lifeRnd,
 *   const MTX34* space)`), which orders the prologue's moves; the map row carries that mangling.
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

/* The non-polymorphic head of the particle manager: MWCC places the vtable pointer after it, at +0x1C. */
struct EfPmHead {
    /* +0x00 */ u8 pad_0x00[0x1C];
}; /* size: 0x1C */

/* The particle manager: its vtable pointer sits at +0x1C and the table's +0x14 slot is the spawn call this unit
 * makes (declared only, so this unit emits no table). */
struct EfPm : EfPmHead {
    /* +0x1C: the vtable pointer */
    virtual void slot_0x08();
    virtual void slot_0x0C();
    virtual void slot_0x10();
    virtual void CreateParticle(u16 life, EfVec3 pos, EfVec3 vel, s32 space, f32 momentum, u8 *inherit, s32 ref,
                                u16 remain);
}; /* size: 0x20 (lower bound) */

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


/* ef/nw4r math helpers; retail's relocations carry their plain map names, so they have C linkage. */
extern "C" {
extern u16 ef_form_calc_life(void *self, u16 id, f32 f, struct EfEmitter *em);
extern void ef_form_calc_velocity(void *self, nw4r::math::VEC3 *out, struct EfEmitter *em, nw4r::math::VEC3 *a,
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

#pragma peephole off
#pragma fp_contract off

void fn_800CD584(void *self, EfEmitter *em, EfPm *pm, s32 count, void *unused, void *params,
                 s32 id, f32 f, s32 arg7) {
    EfVec3 v_44, v_38, v_2C, v_20;
    s32 i;

#line 42
    NW4R_POINTER_ASSERT(em, lbl_80594FA8);
    NW4R_POINTER_ASSERT(pm, lbl_80594FDC);
    NW4R_POINTER_ASSERT(params, lbl_80595010);

    if (count >= 1) {
        for (i = 0; i < count; i++) {
            f32 rate, t, s, r, scale;

            setVec3(&v_44, 0.0f, 0.0f, 0.0f);
            VEC3_ctor(&v_38);
            rate = ef_random_float(&em->rate.counter);
            t = 2.0f * rate - 1.0f;
            if (t >= 0.0f)
                s = (0.66f + 0.34f * t) * t;
            else
                s = (0.66f - 0.34f * t) * t;
            v_38.x = s;
            r = sqrt_f32(1.0f - v_38.x * v_38.x);
            ef_sin_cos(&v_38.z, &v_38.y,
                        2.0f * (3.1415927f * ef_random_float(&em->rate.counter)));
            v_38.y = v_38.y * r;
            v_38.z = v_38.z * r;
            setVec3(&v_2C, v_38.x, 0.0f, v_38.z);
            ef_vec3_normalize_to(&v_2C, &v_2C);
            VEC3_ctor(&v_20);
            ef_form_calc_velocity(self, &v_20, em, &v_44, &v_38, &v_38, &v_2C);
            pm->CreateParticle(ef_form_calc_life(self, (u16)id, f, em), v_44, v_20, arg7,
                               1.0f + (0.01f * (f32)em->field_0x67) * ef_random_float(&em->rate.counter),
                               &em->field_0xFC, em->field_0xF8, em->field_0xE8);
        }
    }
}
