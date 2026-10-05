/*
 * ef/ef_torus.cpp - the retail `ef_torus.cpp` shape, 0x800C9540..0x800C9DD0 (2 functions).
 *
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with dumpmap:
 * `fn_800C9540 = .text:0x800C9540` and `fn_800C9DCC = .text:0x800C9DCC` are bare name = address rows)
 *
 * The original source file is named by the unit's own `.data` pool: the three `nw4r::db::Panic`
 * asserts in fn_800C9540 pass the bare string "ef_torus.cpp" (0x80594BA8, size 0xD) as the file
 * argument.  A `.cpp` `__FILE__` string is conclusive for the language, and the mangled
 * `Panic__Q24nw4r2dbFPCciPCce` callee agrees, so the unit is C++ and lives at
 * `src/ef/ef_torus.cpp` in the `ef` lib (`cflags_main`).  It sits between the
 * ef_drawsmoothstripestrategy run (ends 0x800C5DB8) and ef_cube (starts 0x800C9DD0); the seam is
 * the proposal's own boundary and is unproven (docs/plan.md 8.3).
 *
 * 0x800C9540 is the Torus shape's emitter entry (the shape's single virtual method, dispatched
 * through the `ef_torus.cpp` vtable `lbl_80594C58`).  It guards its `em`/`pm`/`params` pointers,
 * derives the three radii, the sweep phase and the per-step angle from `params` and the effect's
 * progress, then runs the emission loop: each step rebuilds the torus point and its surface
 * direction from two sin/cos pairs (`fn_8009C760`), converts the frame with `fn_800A99B4`, hands
 * the result to the particle manager's spawn slot and advances the tube/major angles when the
 * effect is a swept (`flags & 0x00020000`) one.
 *
 * 0x800C9DCC is a 4-byte `b fn_80463F04` thunk; `fn_80463F04` is `fabsf` (fabs + frsp).  Both the
 * shape and its siblings (`ef_cube.cpp`, `ef_cylinder.cpp`, `ef_disc.cpp`) call it for the
 * `|radius|` clamps.
 *
 * Codegen levers (both load-bearing, measured):
 *   * `#pragma peephole off` - retail keeps the paired-single FPR epilogue as `li r0,<slot>;
 *     psq_lx fN,r1,r0,0,0`; the peephole folds it to `psq_l`.  Same finding as every ef shape.
 *   * `#pragma fp_contract off` - retail keeps every `a*b+c` as two instructions; the default
 *     contracts them to `fmadds`/`fmsubs`.
 *   * the `.sdata2` constants are written as literals (not `extern` float variables), so MWCC pools
 *     and hoists them into registers - the `ef_disc.cpp` finding (89.2 % -> 96.9 % there).
 *
 * The unit's `.data` run (0x80594BA8 "ef_torus.cpp" + the three assert formats + the vtable
 * lbl_80594C58 at 0x80594C58) and its compiler-generated `.sdata2` pool (the literal floats,
 * 0x80796208..0x80796240) are **not** claimed in `splits.txt`: the strings are declared `extern`
 * and never defined here, so the object emits no `.data` (playbook 29, docs/plan.md 8.4).
 * Two `range` requests ride the outbox - the data pass claims/defines them once the source emits
 * the bytes.  The `extab`/`extabindex` fragments travel with the code unit and are claimed below.
 *
 * Measured (official `report generate` fuzzy_match_percent, against MAIN's retired single-function
 * targets build/RMHE08/obj/auto_fn_800C9540_text.o and auto_03_800C9DCC_text.o):
 *   fn_800C9540  99.2267 %  (.text 2188 B, ours 2188 B)
 *   fn_800C9DCC 100.0000 %  (.text 4 B, byte-identical)
 * The object's `.text` is 0x890 (the range's exact size), `extab` 0x8 and `extabindex` 0xC (the
 * target's exact sizes).  `build/tmp/rec_torus.py` is the scratch measurer (MAIN has no target
 * object and no ninja rule for a proposal unit, so `recompile.py <unit> --measure` cannot run).
 *
 * Residuals (fn_800C9540, 99.2267 % - 3 source-unreachable shapes, all the sibling shapes' known
 * ones):
 *   * the prologue saves `spawn_arg` (`mr r28,r10`) one instruction early; ef_cube's fn_800C9DD0
 *     and ef_disc's fn_800CC5B0 record the identical slot.
 *   * the spawn call's `fmr f1,<scale>` is scheduled one slot later than retail's, and retail
 *     colours `pm` into r3 (the `this` copy it just made) where ours keeps its incoming r24 and
 *     takes r11 for the vtable - the "retail's colouring is your mirror image" shape (playbook 22;
 *     ef_cube's fn_800C9DD0 residual is the same one).
 *   * the two spawn argument copies (`v_out_copy = v_out; v_pt_copy = v_pt;`) must be
 *     *initialised* (`Vec x = y;`), not assigned after declaration: MWCC copies a 12-byte scalar
 *     struct field-wise with `lfs`/`stfs` but emits retail's `lwz`/`stw` block when the copy is an
 *     aggregate initialiser (measured: assignment keeps `lfs`/`stfs`, initialisation reproduces
 *     the target's `lwz/lwz/stw/stw/lwz/stw`).  Getting this right moved 97.93 % -> 99.23 %.
 */

#include "ef.h"

#pragma peephole off
#pragma fp_contract off

/* The unit's own pooled strings (unclaimed - see the header). */
extern char lbl_80594BA8[]; /* "ef_torus.cpp"                                        .data 0x80594BA8 */
extern char lbl_80594BB8[]; /* "NW4R:Pointer Error\nem(=%p) is not valid pointer."    .data 0x80594BB8 */
extern char lbl_80594BEC[]; /* "NW4R:Pointer Error\npm(=%p) is not valid pointer."    .data 0x80594BEC */
extern char lbl_80594C20[]; /* "NW4R:Pointer Error\nparams(=%p) is not valid pointer." .data 0x80594C20 */

extern "C" {

/* Unsplit helpers whose address band names different modules on either side (the rule-2 gap):
 * fn_8009C484 at 0x8009C484 (gx band | ef band) and fn_80463F04 at 0x80463F04 (the runtime). */
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
        phase = 2.0f * (3.1415927f * fn_800A8A08(&em->progress));
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
            angle = (params->angle_end - params->angle_base) * fn_800A8A08(&em->progress);
            tube = 2.0f * (3.1415927f * fn_800A8A08(&em->progress));
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
        ratio = 1.0f + 0.01f * (f32)em->scale_rate * fn_800A8A08(&em->progress);
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
