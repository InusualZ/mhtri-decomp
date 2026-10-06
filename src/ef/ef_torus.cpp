/*
 * ef/ef_torus.cpp - the torus emitter form's emission entry (its class's single virtual, through the table
 *   `lbl_80594C58`) and a `b fn_80463F04` (`fabsf`) thunk the sibling shapes also call for the `|radius|` clamps.
 * RANGE. .text 0x800C9540-0x800C9DD0 (2 functions); extab 0x8000A554-0x8000A55C, extabindex 0x80023BA4-0x80023BB0,
 *   .data 0x80594BA8-0x80594C68 (the `__FILE__` string "ef_torus.cpp", the three assert formats, the table at
 *   0x80594C58), .sdata2 0x80796208-0x80796240 (the literal floats MWCC pools).
 * FLAGS. `cflags_main`; file-wide `#pragma peephole off` (retail keeps the FPR epilogue as `li r0,<slot>;
 *   psq_lx`) and `#pragma fp_contract off` (retail keeps every `a*b+c` as two instructions).
 * NAMES. The map has only `fn_` stems for the range.
 * RESIDUALS. 1 partial row, `fn_800C9540` (the `ef/ef_cube.cpp` and `ef/ef_disc.cpp` shapes share each one):
 *  - the prologue saves `spawn_arg` (`mr r28,r10`) one instruction early;
 *  - the two spawn argument copies take each other's stack slots (0x18/0x24);
 *  - the spawn call's `fmr f1,<scale>` is one slot late, and the slot +0x14 dispatch loads through the saved
 *    `pm` (`lwz r11, 0x1C(r24)`) where retail goes through r3 (playbook 22).
 *   flipcheck: `.data` claimed, not emitted (the strings are declared, never defined).
 * SHAPES. The `.sdata2` constants are literals, not `extern` floats, so MWCC pools and hoists them as retail does.
 * SHAPES. The spawn argument copies are initialised (`Vec x = y;`), not assigned: an initialiser emits retail's
 *   `lwz`/`stw` block where an assignment copies field-wise with `lfs`/`stfs`.
 * SHAPES. `#line 42` puts the three `EF_ASSERT_PTR` sites on lines 42-44.
 */

#include "ef.h"

#pragma peephole off
#pragma fp_contract off

/* The unit's own strings (its claimed `.data`, declared, never defined). */
extern char lbl_80594BA8[]; /* "ef_torus.cpp"                                        .data 0x80594BA8 */
extern char lbl_80594BB8[]; /* "NW4R:Pointer Error\nem(=%p) is not valid pointer."    .data 0x80594BB8 */
extern char lbl_80594BEC[]; /* "NW4R:Pointer Error\npm(=%p) is not valid pointer."    .data 0x80594BEC */
extern char lbl_80594C20[]; /* "NW4R:Pointer Error\nparams(=%p) is not valid pointer." .data 0x80594C20 */

extern "C" {

/* Helpers declared locally: fn_8009C484 (`ef/ef_util.cpp`'s vector normalise) and fn_80463F04
 * (the runtime's `fabsf`). */
extern void fn_8009C484(VEC3* out, VEC3* in);
extern f32 fn_80463F04(f32 x);

void fn_800C9540(s32 ctx, EfWork* em, EfParticle* pm, s32 count, u32 flags, EfParams* params,
                 u16 id, s32 spawn_arg, f32 scale) {
    f32 scale_a, scale_b, scale_c, phase, angle, step, tube, ratio;
    u32 swept;
    s32 i, total;

#line 42
    EF_ASSERT_PTR(lbl_80594BA8, lbl_80594BB8, em);
    EF_ASSERT_PTR(lbl_80594BA8, lbl_80594BEC, pm);
    EF_ASSERT_PTR(lbl_80594BA8, lbl_80594C20, params);

    if (count < 1) {
        return;
    }

    scale_a = fn_800C9DCC(params->scale_x) > 1.1920929e-7f ? params->scale_x : 1.1920929e-7f;
    scale_b = fn_800C9DCC(params->scale_z) > 1.1920929e-7f ? params->scale_z : 1.1920929e-7f;
    if (flags & 0x02000000) {
        scale_c = scale_a;
    } else {
        scale_c = fn_800C9DCC(params->scale_c) > 1.1920929e-7f ? params->scale_c : 1.1920929e-7f;
    }

    angle = 0.0f;
    step = angle;
    if (flags & 0x00040000) {
        phase = params->angle_base;
    } else {
        phase = 2.0f * (3.1415927f * ef_random_float(&em->progress));
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

    total = swept ? count * count : count;
    tube = 0.0f;
    for (i = 0; i < total; i++) {
        VEC3 v_pt, v_out, v_norm, v_flat, v_dir;
        f32 c1, s1, c2, s2;

        VEC3_ctor(&v_pt);
        VEC3_ctor(&v_out);
        ratio = (100.0f - params->rate) / (100.0f + params->rate);
        if (!swept) {
            angle = (params->angle_end - params->angle_base) * ef_random_float(&em->progress);
            tube = 2.0f * (3.1415927f * ef_random_float(&em->progress));
        }
        fn_8009C760(&c1, &s1, phase + angle);
        fn_8009C760(&c2, &s2, tube);

        v_pt.x = scale_a * (c1 + c1 * (ratio * s2)) / (1.0f + ratio);
        v_pt.y = scale_b * c2;
        v_pt.z = scale_c * (s1 * (-ratio * s2) - s1) / (1.0f + ratio);

        assignVec3((Vec*)&v_norm, (Vec*)&v_pt);
        fn_8009C484(&v_norm, &v_norm);
        setVec3(&v_flat, v_pt.x, 0.0f, v_pt.z);
        fn_8009C484(&v_flat, &v_flat);
        VEC3_ctor(&v_dir);
        if (ratio == 0.0f) {
            v_dir.x = c1 * (scale_a * s2);
            v_dir.y = scale_b * c2;
            v_dir.z = s1 * (-scale_c * s2);
        } else {
            v_dir.x = scale_a * (c1 * (ratio * s2)) / (1.0f + ratio);
            v_dir.y = scale_b * c2;
            v_dir.z = scale_c * (s1 * (-ratio * s2)) / (1.0f + ratio);
        }
        fn_8009C484(&v_dir, &v_dir);

        fn_800A99B4(ctx, (Vec*)&v_out, em, (Vec*)&v_pt, (Vec*)&v_dir, (Vec*)&v_norm, (Vec*)&v_flat);
        VEC3 v_out_copy = v_out;
        VEC3 v_pt_copy = v_pt;
        ratio = 1.0f + 0.01f * (f32)em->scale_rate * ef_random_float(&em->progress);
        pm->slots->spawn(pm, fn_800A9FB0(ctx, id, scale, em), (Vec*)&v_pt_copy, (Vec*)&v_out_copy, spawn_arg,
                         &em->spawn_data, em->spawn_extra, em->spawn_flag, ratio);

        if (swept) {
            if ((i + 1) % count == 0) {
                angle += step;
                tube = 0.0f;
            } else {
                tube += 6.2831855f / (f32)count;
            }
        }
    }
}

/* fabsf, reached through a 4-byte tail-call thunk in retail. */
f32 fn_800C9DCC(f32 x) {
    return fn_80463F04(x);
}

} /* extern "C" */

#pragma fp_contract on
#pragma peephole on
