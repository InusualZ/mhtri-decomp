/*
 * auto/800CC5B0_fn_800CC5B0.c - the retail `ef_disc.cpp` unit, 0x800CC5B0..0x800CCCF8.
 *
 * One function: the `Disc` effect shape's emitter entry (the shape's single virtual method, dispatched
 * through the `ef_disc.cpp` vtable `lbl_80594DD0`).  It guards its `em`/`pm`/`params` pointers, derives
 * the two emission radii, the sweep angle range and the per-step angle from the parameter block and the
 * effect's progress, then runs the emission loop: for each of `count` steps it rebuilds the particle
 * position and direction from a sin/cos pair (`fn_8009C760`), orients the particle (`fn_8009C6F0` when
 * the emitter has a spread), converts the frame with `fn_800A99B4`, hands the result to the particle
 * manager's spawn slot and advances the emission angle when the effect is a swept one.
 *
 * The shape is the flat-disc sibling of `ef_cylinder.cpp` (0x800CB948..0x800CC5B0): both derive the same
 * scales and sweep range from `em`/`pm`/`params`, but the cylinder factors the emission body into a
 * separate function while the disc inlines it, and the disc keeps the particle in the XZ plane.
 *
 * `em`/`pm`/`params` are the source's own parameter names - recovered from the `NW4R:Pointer Error`
 * format strings, which the three asserts on lines 42-44 pass to `nw4r::db::Panic`.  The unit's own
 * `.sdata2` pool (0x807962B0..0x807962E8) is **compiler-generated**: the source writes the constants as
 * literals, so MWCC pools them (and can hoist them out of the loop) instead of treating them as the
 * reloadable `extern` variables a named-constant source would produce - that shape alone was worth
 * 89.2 % -> 96.9 %.  The pool range is not claimed in `splits.txt` yet, so a `range` request rides the
 * outbox; the `extab`/`extabindex` fragments travel with the code unit.
 *
 * The shape objects were built with the peephole pass and floating-point contraction off (there is no
 * `rlwinm.` or `fmadds` in any target object of the 0x800BFFD4..0x800CCFB0 region); the two pragmas
 * below scope that to this unit instead of changing the whole catch-all `auto` lib.
 *
 * Residual: the function is the target's size (1864 B) and scores 99.1 %.  The six open rows are the
 * prologue's `arg7` save slot (`mr r28,r10` one step early), the spawn call's `fmr f1`/`lwz r11`
 * placements, and the three `Panic` line numbers; every one was probed against several source shapes
 * and left as the allocator/scheduler residual it is.  Measurements: `.pi/notes/800cc5b0-fn-800cc5b0-39c9.md`.
 */

#include "ef.h"

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
        range = 2.0f * (3.1415927f * fn_800A8A08(&em->progress));
    }

    swept = flags & 0x00020000;
    if (swept) {
        f32 t = fn_80463F10(params->angle_end - params->angle_base, 6.2831855f);
        if (t < 0.0001917476f || t > 6.2829938f || em->split_count == 1) {
            step = (params->angle_end - params->angle_base) / (f32)em->split_count;
        } else {
            step = (params->angle_end - params->angle_base) / (f32)(em->split_count - 1);
        }
    }

    for (i = 0; i < count; i++) {
        Vec v88, v76, v64, v52, v40, v28, v16;
        f32 fC, f8;
        f32 scale, rate, t;

        fn_80043EA8((VEC3*)&v88);   /* the declaration takes the nw4r vector; same 3-float layout */
        fn_80043EA8((VEC3*)&v76);
        t = fn_800A8A08(&em->progress);
        rate = params->rate / 100.0f;
        if (flags & 0x01000000) {
            scale = fn_80050BC0(t + (1.0f - t) * (rate * rate), 1.0f - t);
        } else {
            scale = t + rate * (1.0f - t);
        }
        if (!swept) {
            angle = (params->angle_end - params->angle_base) * fn_800A8A08(&em->progress);
        }
        fn_8009C760(&fC, &f8, range + angle);
        fn_80041E8C(&v64, fC, 0.0f, -f8);
        v88.x = scale_a * (v64.x * scale);
        v88.y = 0.0f;
        v88.z = scale_b * (v64.z * scale);
        fn_80051490(&v52, &v64);
        fn_80043EA8((VEC3*)&v40);
        if (0.0f == em->spread) {
            v40.x = 0.0f;
            v40.y = 1.0f;
            v40.z = 0.0f;
        } else {
            f32 s = scale * em->spread;
            fn_8009C6F0(&v40, s);
            v40.z = -f8 * v40.x;
            v40.x = v40.x * fC;
        }
        fn_800A99B4(ctx, &v76, em, &v88, &v40, &v52, &v64);
        v16 = v76;
        v28 = v88;
        scale = 1.0f + 0.01f * (f32)em->scale_rate * fn_800A8A08(&em->progress);
        pm->slots->spawn(pm, fn_800A9FB0(ctx, id, farg0, em), &v28, &v16, arg7, &em->spawn_data,
                         em->spawn_extra, em->spawn_flag, scale);
        if (swept) {
            angle += step;
        }
    }
}

#pragma fp_contract on
#pragma peephole on
