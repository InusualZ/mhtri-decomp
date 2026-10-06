/*
 * ef/ef_point.cpp - the point emitter form (`EmitterFormPoint::Emission`): validates `em`/`pm`/`params`, then
 *   spawns `count` points, each mapping the emitter's random rate through `0.66 +/- 0.34 * t` onto a circle,
 *   building the position/velocity pair (`CalcVelocity`) and handing it to the particle manager.
 * RANGE. .text 0x800CD584-0x800CDB2C (1 function); extab 0x8000A5D4-0x8000A5DC, extabindex 0x80023C64-0x80023C70,
 *   .data 0x80594F98-0x80595058 (the `__FILE__` string "ef_point.cpp", the three assert messages, the class's
 *   table at 0x80595048), .sdata2 0x80796300-0x80796328.
 * FLAGS. `cflags_main` plus `-pool off`; file-wide `#pragma peephole off` (retail's `li r0,<off>; psq_lx` FPR
 *   restores) and `#pragma fp_contract off` (retail has no fused float op).
 * NAMES. The class and method are nw4r's.
 *   GUESS: `ef_point_file_name`, `ef_point_err_em`, `ef_point_err_pm`, `ef_point_err_params` (their text).
 * RESIDUALS. none known: every section is byte-identical to the target object.
 * SHAPES. The function is `nw4r::ef::EmitterFormPoint::Emission` (the class is declared in `ef/ef_emform.h`; this unit
 *   defines its key function, so the compiler emits the class's table here, after the strings).  The
 *   particle manager's spawn is its virtual `CreateParticle`, taking the position and velocity by value: the call
 *   copies them (velocity first), evaluates the momentum before the life and dispatches through r3.  The float
 *   argument precedes the space matrix (nw4r's `Emission(..., u16 life, f32 lifeRnd, const MTX34* space)`).
 * SHAPES. The strings are global definitions in retail's order; `-pool off` (configure.py) gives each its own
 *   `lis`/`addi`, where the default pools them off one base register.
 * SHAPES. The `.sdata2` constants are literals (MWCC pools and hoists them in retail's order).
 * SHAPES. The pointer assert is the `if (!okN && !test) okN-1 = FALSE;` chain (six materialised BOOLs, the first
 *   `if` carrying two tests), with `top_` computed after the six BOOLs.
 * SHAPES. `#line 42` puts the three assert sites on lines 42-44 (`li r4,{42,43,44}`).
 */

#include "types.h"
#include "nw4r/math.h" /* nw4r::math::VEC3 - the vector record these bodies work on (rule 11) */
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "ef/ef_emitter.h" /* ef_random_float (rule 2) */
#include "ef/ef_emform.h" /* nw4r::ef::EmitterFormPoint, EfWork, ParticleManager (rule 1) */

/* --------------------------------------------------------------------------------------------- */
/* Types                                                                                          */
/* --------------------------------------------------------------------------------------------- */

/* The three-float vector the ef emitter calls pass around, under the ef band's local spelling. */
typedef nw4r::math::VEC3 EfVec3; /* size: 0x0C */


/* --------------------------------------------------------------------------------------------- */
/* Referenced symbols: the strings and pool constants of this unit's claimed `.data`/`.sdata2`,
 * declared, never defined. */
/* --------------------------------------------------------------------------------------------- */

/* `nw4r::db::Panic`, declared in its namespace so the front-end emits the map's mangling
 * (`Panic__Q24nw4r2dbFPCciPCce`). */
namespace nw4r { namespace db { void Panic(const char* file, int line, const char* fmt, ...); } }
/* The unit's `.data` strings, in retail order: the file name and one message per checked pointer (the
 * class's table follows them). */
char ef_point_file_name[] = "ef_point.cpp";
char ef_point_err_em[] = "NW4R:Pointer Error\nem(=%p) is not valid pointer.";
char ef_point_err_pm[] = "NW4R:Pointer Error\npm(=%p) is not valid pointer.";
char ef_point_err_params[] = "NW4R:Pointer Error\nparams(=%p) is not valid pointer.";


/* ef/nw4r math helpers; retail's relocations carry their plain map names, so they have C linkage. */
extern "C" {
extern f32 sqrt_f32(f32 x);
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
            nw4r::db::Panic(ef_point_file_name, __LINE__, msg, (ptr));                   \
    }

/* --------------------------------------------------------------------------------------------- */

#pragma peephole off
#pragma fp_contract off

void nw4r::ef::EmitterFormPoint::Emission(EfWork* em, ParticleManager* pm, int count, u32 flags, f32* params,
                                          u16 id, f32 f, const MTX34* arg7) {
    EfVec3 v_44, v_38, v_2C, v_20;
    s32 i;

#line 42
    NW4R_POINTER_ASSERT(em, ef_point_err_em);
    NW4R_POINTER_ASSERT(pm, ef_point_err_pm);
    NW4R_POINTER_ASSERT(params, ef_point_err_params);

    if (count >= 1) {
        for (i = 0; i < count; i++) {
            f32 rate, t, s, r, scale;

            setVec3(&v_44, 0.0f, 0.0f, 0.0f);
            VEC3_ctor(&v_38);
            rate = ef_random_float(&em->progress);
            t = 2.0f * rate - 1.0f;
            if (t >= 0.0f)
                s = (0.66f + 0.34f * t) * t;
            else
                s = (0.66f - 0.34f * t) * t;
            v_38.x = s;
            r = sqrt_f32(1.0f - v_38.x * v_38.x);
            ef_sin_cos(&v_38.z, &v_38.y,
                        2.0f * (3.1415927f * ef_random_float(&em->progress)));
            v_38.y = v_38.y * r;
            v_38.z = v_38.z * r;
            setVec3(&v_2C, v_38.x, 0.0f, v_38.z);
            ef_vec3_normalize_to(&v_2C, &v_2C);
            VEC3_ctor(&v_20);
            CalcVelocity((Vec*)&v_20, em, (Vec*)&v_44, (Vec*)&v_38, (Vec*)&v_38, (Vec*)&v_2C);
            pm->CreateParticle(CalcLife(id, f, em), v_44, v_20, arg7,
                               1.0f + (0.01f * (f32)em->scale_rate) * ef_random_float(&em->progress),
                               &em->spawn_data, em->spawn_extra, em->spawn_flag);
        }
    }
}
