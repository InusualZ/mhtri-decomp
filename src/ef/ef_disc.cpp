/*
 * ef/ef_disc.cpp - the disc emitter form (`EmitterFormDisc::Emission`): guards `em`/`pm`/`params`, derives the
 *   two radii and the sweep range, then emits `count` particles on the rim, each leaning out of the plane by
 *   the emitter's spread.
 * RANGE. .text 0x800CC5B0-0x800CCCF8 (1 function); extab and extabindex its one record each; .data
 *   0x80594DE0-0x80594E98 (the `__FILE__` string "ef_disc.cpp", the three assert messages, the class's table at
 *   0x80594E8C - the table ends the unit's `.data`, so `ef/ef_emform.cpp`'s run starts after it), .sdata2 its
 *   pool.
 * FLAGS. `cflags_main` plus `-pool off`; file-wide `#pragma peephole off` and `#pragma fp_contract off`.
 * NAMES. The class and method are nw4r's (the asserts' `em`/`pm`/`params` are its parameter names).
 *   GUESS: `ef_disc_file_name`, `ef_disc_err_em`, `ef_disc_err_pm`, `ef_disc_err_params` (their text).
 * RESIDUALS. none known: every section is byte-identical to the target object.
 * SHAPES. The function is `nw4r::ef::EmitterFormDisc::Emission` (the class is declared in `ef/ef_emform.h`; this unit
 *   defines its key function, so the compiler emits the class's table here, after the strings).  The
 *   particle manager's spawn is its virtual `CreateParticle`, taking the position and velocity by value: the call
 *   copies them (velocity first), evaluates the momentum before the life and dispatches through r3.  The float
 *   argument precedes the space matrix (nw4r's `Emission(..., u16 life, f32 lifeRnd, const MTX34* space)`).
 * SHAPES. The strings are global definitions in retail's order; `-pool off` (configure.py) gives each its own
 *   `lis`/`addi`, where the default pools them off one base register.
 * SHAPES. The `.sdata2` constants are literals, so MWCC pools and hoists them out of the loop.
 * SHAPES. `#line 42` puts the three `EF_ASSERT_PTR` sites on lines 42-44 (`__LINE__`).
 */









#include "ef/ef_emform.h" /* nw4r::ef::EmitterFormDisc, EfWork, ParticleManager (rule 1) */
#include "fn_8004CAD8.h"      /* sqrt_f32 - that unit owns the address and publishes it (rule 2) */
#pragma peephole off
#pragma fp_contract off

/* The unit's `.data` strings, in retail order: the file name and one message per checked pointer (the
 * class's table follows them). */
char ef_disc_file_name[] = "ef_disc.cpp";
char ef_disc_err_em[] = "NW4R:Pointer Error\nem(=%p) is not valid pointer.";
char ef_disc_err_pm[] = "NW4R:Pointer Error\npm(=%p) is not valid pointer.";
char ef_disc_err_params[] = "NW4R:Pointer Error\nparams(=%p) is not valid pointer.";

/* 0x800CC5B0 (0x748): emits `count` particles on the disc's rim (scaled inward by the rate), at random or swept
 * (flag 0x20000), each leaning out of the plane by the emitter's spread. */
void nw4r::ef::EmitterFormDisc::Emission(EfWork* em, ParticleManager* pm, int count, u32 flags, f32* paramBlock,
                                         u16 id, f32 farg0, const MTX34* arg7) {
    EfParams* params = (EfParams*)paramBlock;
    u32 swept; s32 i; f32 scale_a, scale_b, angle, step, range;
#line 42
    EF_ASSERT_PTR(ef_disc_file_name, ef_disc_err_em, em);
    EF_ASSERT_PTR(ef_disc_file_name, ef_disc_err_pm, pm);
    EF_ASSERT_PTR(ef_disc_file_name, ef_disc_err_params, params);

    if (count < 1) {
        return;
    }

    scale_a = ef_fabsf(params->scale_x) > 1.1920929e-7f ? params->scale_x : 1.1920929e-7f;

    if (flags & 0x02000000) {
        scale_b = scale_a;
    } else {
        scale_b = ef_fabsf(params->scale_z) > 1.1920929e-7f ? params->scale_z : 1.1920929e-7f;
    }

    angle = 0.0f;
    step = angle;

    if (flags & 0x00040000) {
        range = params->angle_base;
    } else {
        range = 2.0f * (3.1415927f * ef_random_float(&em->progress));
    }

    swept = flags & 0x00020000;
    if (swept) {
        f32 t = fmodf(params->angle_end - params->angle_base, 6.2831855f);
        if (t < 0.0001917476f || t > 6.2829938f || em->split_count == 1) {
            step = (params->angle_end - params->angle_base) / (f32)em->split_count;
        } else {
            step = (params->angle_end - params->angle_base) / (f32)(em->split_count - 1);
        }
    }

    for (i = 0; i < count; i++) {
        VEC3 v88, v76, v64, v52, v40;
        f32 fC, f8;
        f32 scale, rate, t;

        VEC3_ctor(&v88);   /* the declaration takes the nw4r vector; same 3-float layout */
        VEC3_ctor(&v76);
        t = ef_random_float(&em->progress);
        rate = params->rate / 100.0f;
        if (flags & 0x01000000) {
            /* One argument: the callee reads only f1; retail's `f2` is the hoisted `1.0f - t` the else
             * branch reuses, so the shared subexpression stays in the expression. */
            scale = sqrt_f32(t + (1.0f - t) * (rate * rate));
        } else {
            scale = t + rate * (1.0f - t);
        }
        if (!swept) {
            angle = (params->angle_end - params->angle_base) * ef_random_float(&em->progress);
        }
        ef_sin_cos(&fC, &f8, range + angle);
        setVec3(&v64, fC, 0.0f, -f8);
        v88.x = scale_a * (v64.x * scale);
        v88.y = 0.0f;
        v88.z = scale_b * (v64.z * scale);
        assignVec3((Vec*)&v52, (Vec*)&v64);
        VEC3_ctor(&v40);
        if (0.0f == em->spread) {
            v40.x = 0.0f;
            v40.y = 1.0f;
            v40.z = 0.0f;
        } else {
            f32 s = scale * em->spread;
            ef_vec_sin_cos((Vec*)&v40, s);
            v40.z = -f8 * v40.x;
            v40.x = v40.x * fC;
        }
        CalcVelocity((Vec*)&v76, em, (Vec*)&v88, (Vec*)&v40, (Vec*)&v52, (Vec*)&v64);
        pm->CreateParticle(CalcLife(id, farg0, em), v88, v76, arg7,
                           1.0f + 0.01f * (f32)em->scale_rate * ef_random_float(&em->progress), &em->spawn_data,
                           em->spawn_extra, em->spawn_flag);
        if (swept) {
            angle += step;
        }
    }
}

#pragma fp_contract on
#pragma peephole on
