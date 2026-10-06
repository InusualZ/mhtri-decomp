/*
 * ef/ef_disc.cpp - the disc emitter form's emission entry (its class's single virtual, through the table
 *   `lbl_80594DD0`): guards `em`/`pm`/`params`, derives the two radii and the sweep range, then emits `count`
 *   particles in the XZ plane, advancing the angle when the effect is swept.  The flat sibling of
 *   `ef/ef_cylinder.cpp`, with the emission body inlined.
 * RANGE. .text 0x800CC5B0-0x800CCCF8 (1 function); extab 0x8000A57C-0x8000A584, extabindex 0x80023BE0-0x80023BEC,
 *   .data 0x80594DE0-0x80594E8C (the `__FILE__` string first), .sdata2 0x807962B0-0x807962E8 (the literal floats
 *   MWCC pools).
 * FLAGS. `cflags_main`; `#pragma peephole off` and `#pragma fp_contract off` around the function (no `rlwinm.` or
 *   `fmadds` in any target object of the 0x800BFFD4-0x800CCFB0 shapes).
 * NAMES. `em`/`pm`/`params` are the source's names, from the `NW4R:Pointer Error` strings of the three asserts.
 * RESIDUALS. 1 partial row, `fn_800CC5B0__FlP6EfWorkP10EfParticlelUlP8EfParamsUslf`:
 *  - the prologue saves `arg7` (`mr r30,r10`) one step early, and the saved arguments take r23-r30 where
 *    retail takes r22-r28 and r31;
 *  - the `v16`/`v28` copies move floats (`lfs`/`stfs`) where retail moves words (`lwz`/`stw`); `ef/ef_torus.cpp`
 *    gets retail's form by initialising its copies;
 *  - the spawn call's `fmr f1` is late and the slot +0x14 dispatch loads through the saved `pm` (`lwz r11,
 *    0x1C(r25)`) where retail goes through r3;
 *  - the `.sdata2` constants are our pool's `@N` where retail reads the claimed `lbl_807962B0` run.
 *   flipcheck: `.data` claimed, not emitted.
 * SHAPES. The `.sdata2` constants are literals, so MWCC pools and hoists them out of the loop.
 * SHAPES. This header's line count is load-bearing: with no `#line`, the three `EF_ASSERT_PTR` sites must stay
 *   on lines 42-44 (`__LINE__`).
 */









#include "ef.h"
#include "fn_8004CAD8.h"      /* sqrt_f32 - that unit owns the address and publishes it (rule 2) */
#pragma peephole off
#pragma fp_contract off

void fn_800CC5B0(s32 ctx, EfWork* em, EfParticle* pm, s32 count, u32 flags, EfParams* params,
                 u16 id, s32 arg7, f32 farg0) {
    u32 swept; s32 i; f32 scale_a, scale_b, angle, step, range;
    EF_ASSERT_PTR(lbl_80594DE0, lbl_80594DEC, em);
    EF_ASSERT_PTR(lbl_80594DE0, lbl_80594E20, pm);
    EF_ASSERT_PTR(lbl_80594DE0, lbl_80594E54, params);

    if (count < 1) {
        return;
    }

    scale_a = fn_800C9DCC(params->scale_x) > 1.1920929e-7f ? params->scale_x : 1.1920929e-7f;

    if (flags & 0x02000000) {
        scale_b = scale_a;
    } else {
        scale_b = fn_800C9DCC(params->scale_z) > 1.1920929e-7f ? params->scale_z : 1.1920929e-7f;
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
        VEC3 v88, v76, v64, v52, v40, v28, v16;
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
        fn_800A99B4(ctx, (Vec*)&v76, em, (Vec*)&v88, (Vec*)&v40, (Vec*)&v52, (Vec*)&v64);
        v16 = v76;
        v28 = v88;
        scale = 1.0f + 0.01f * (f32)em->scale_rate * ef_random_float(&em->progress);
        pm->slots->spawn(pm, fn_800A9FB0(ctx, id, farg0, em), (Vec*)&v28, (Vec*)&v16, arg7, &em->spawn_data,
                         em->spawn_extra, em->spawn_flag, scale);
        if (swept) {
            angle += step;
        }
    }
}

#pragma fp_contract on
#pragma peephole on
