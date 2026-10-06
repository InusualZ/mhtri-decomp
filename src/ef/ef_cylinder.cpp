/*
 * ef/ef_cylinder.cpp - the cylinder emitter form: `ef_cylinder_emit` emits one particle per step of a count loop,
 *   and `EmitterFormCylinder::Emission` guards its three pointers, derives the emission parameters and drives it.
 * RANGE. .text 0x800CB948-0x800CC5B0 (2 functions); extab 0x8000A56C-0x8000A57C, extabindex 0x80023BC8-0x80023BE0,
 *   .data 0x80594D20-0x80594DE0 (the `__FILE__` string "ef_cylinder.cpp", the three assert messages, the class's
 *   table at 0x80594DD0), .sdata2 0x80796270-0x807962B0.
 * FLAGS. `cflags_main` plus `-pool off`; file-wide `#pragma peephole off` (retail keeps the `li r0,<slot>; psq_lx`
 *   epilogue) and `#pragma fp_contract off` (retail has no fused `a*b+c`).
 * NAMES. The class and method are nw4r's.
 *   GUESS: `ef_cylinder_emit` (0x800CB948): the per-step emission loop `Emission` drives.
 *   GUESS: `ef_cylinder_file_name`, `ef_cylinder_err_em`, `ef_cylinder_err_pm`, `ef_cylinder_err_params` (their
 *   text).
 * RESIDUALS. `.text` and `.data` are byte-identical.  `.sdata2` is retail's run except that `Emission`'s 2^52
 *   `(f32)(u16)` constant pools after its floats where retail's pools before them (0x80796290), so the pool
 *   references read our `@N` labels.
 * SHAPES. The function is `nw4r::ef::EmitterFormCylinder::Emission` (declared in `ef/ef_emform.h`; this unit defines the key
 *   function, so the class's table is emitted here, after the strings).  The particle manager's spawn is its
 *   virtual `CreateParticle`, taking the position and velocity by value (copied velocity first, momentum
 *   evaluated before the life, dispatch through r3); the float argument precedes the space matrix.
 * SHAPES. The strings are global definitions in retail's order; `-pool off` (configure.py) gives each its own
 *   `lis`/`addi`.
 * SHAPES. The float constants are literals, so MWCC hoists them into registers; the `|scale|` clamps are
 *   `fabsf(x) > eps ? x : eps`, which loads the floor after the call as retail does.
 * SHAPES. `#line 49` and `#line 140` put the two functions' `CHECK_PTR` sites on retail's lines 49-51 and 140-142.
 */

#pragma peephole off
#pragma fp_contract off

#include "types.h"
#include "nw4r/math.h" /* nw4r::math::VEC3 - the vector record these bodies work on (rule 11) */
#include "ef/ef_torus.h" /* ef_fabsf (rule 2) */
#include "ef/ef_emitter.h" /* ef_random_float (rule 2) */
#include "fn_8004CAD8.h"       /* sqrt_f32 - that unit owns the address and publishes it (rule 2) */
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "ef/ef_emform.h" /* nw4r::ef::EmitterFormCylinder, EfWork, ParticleManager (rule 1) */

/* The 3-float vector `nw4r/math.h` owns, spelled `VEC3` here: this unit's whole vector
 * API (`ef_vec3_normalize_to`, `assignVec3`, the spawn slot) works on it. */
typedef nw4r::math::VEC3 VEC3; /* size: 0x0C */

/* The six-float parameter block `params`. */
typedef struct EfCylinderParams {
    f32 scale_a;         /* +0x00 */
    f32 rate_pct;        /* +0x04 */
    f32 range_begin;     /* +0x08 */
    f32 range_end;       /* +0x0C */
    f32 scale_b;         /* +0x10 */
    f32 scale_c;         /* +0x14 */
} EfCylinderParams;       /* size: 0x18 */


/* `nw4r::db::Panic`, declared in its namespace so the front-end emits the map's mangling
 * (`Panic__Q24nw4r2dbFPCciPCce`). */
namespace nw4r { namespace db { void Panic(const char* file, int line, const char* fmt, ...); } }

/* nw4r::math and effect-library helpers; retail's relocations carry their plain map names, so they
 * have C linkage (`VEC3_ctor` comes from `mh3_pad.h`). */
extern "C" {
extern void ef_vec3_normalize_to(VEC3* out, VEC3* in);
}

/* The unit's `.data` strings, in retail order: the file name and one message per checked pointer (the
 * class's table follows them). */
char ef_cylinder_file_name[] = "ef_cylinder.cpp";
char ef_cylinder_err_em[] = "NW4R:Pointer Error\nem(=%p) is not valid pointer.";
char ef_cylinder_err_pm[] = "NW4R:Pointer Error\npm(=%p) is not valid pointer.";
char ef_cylinder_err_params[] = "NW4R:Pointer Error\nparams(=%p) is not valid pointer.";

/* The file's pointer guard: the message comes from the call site, the line from `__LINE__`. */
#define CHECK_PTR(msg, ptr) \
    if (!IsValidPointer((u32)(ptr))) \
        nw4r::db::Panic(ef_cylinder_file_name, __LINE__, msg, (ptr))

/* Emits `count` particles, rebuilding the emission transform each step and advancing the angle when the
 * effect is swept. */
void ef_cylinder_emit(nw4r::ef::EmitterFormCylinder* form, EfWork* em, ParticleManager* pm, int count, u32 flags, EfCylinderParams* params,
                 u16 id, f32 scale, const MTX34* spawn_arg, f32 size_x, f32 size_y, f32 size_z, f32 angle,
                 f32 angle_step, f32 phase, f32 offset_y) {
    s32 i;

#line 49
    CHECK_PTR(ef_cylinder_err_em, em);
    CHECK_PTR(ef_cylinder_err_pm, pm);
    CHECK_PTR(ef_cylinder_err_params, params);

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
        assignVec3((Vec*)&v40, (Vec*)&v88);
        ef_vec3_normalize_to(&v40, &v40);
        form->CalcVelocity((Vec*)&v76, em, (Vec*)&v88, (Vec*)&v52, (Vec*)&v40, (Vec*)&v64);
        pm->CreateParticle(form->CalcLife(id, scale, em), v88, v76, spawn_arg,
                           1.0f + 0.01f * (f32)em->scale_rate * ef_random_float(&em->progress),
                           &em->spawn_data, em->spawn_extra, em->spawn_flag);
        if (flags & 0x00020000) {
            angle += angle_step;
        }
    }
}

/* Guards its pointers, derives the scale triplet, the parameter range and the per-step scale, then
 * sweeps the whole count or emits a single particle. */
void nw4r::ef::EmitterFormCylinder::Emission(EfWork* em, ParticleManager* pm, int count, u32 flags, f32* paramBlock,
                                             u16 id, f32 scale, const MTX34* spawn_arg) {
    EfCylinderParams* params = (EfCylinderParams*)paramBlock;
    f32 scaleA, scaleB, scaleC, angle, range_phase, phase, angle_step, offset;
    s32 i;

#line 140
    CHECK_PTR(ef_cylinder_err_em, em);
    CHECK_PTR(ef_cylinder_err_pm, pm);
    CHECK_PTR(ef_cylinder_err_params, params);

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
            ef_cylinder_emit(this, em, pm, count, flags, params, id, scale, spawn_arg, scaleA, scaleB,
                        scaleC, angle, angle_step, phase, offset);
        }
    } else {
        ef_cylinder_emit(this, em, pm, count, flags, params, id, scale, spawn_arg, scaleA, scaleB, scaleC,
                    angle, angle, phase, 0.0f);
    }
}
