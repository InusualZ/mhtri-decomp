/*
 * ef/ef_sphere.cpp - the sphere emitter form (`EmitterFormSphere::Emission`) and its `cosf` thunk.
 * RANGE. .text 0x800CDB2C-0x800CE5A8 (2 functions); extab 0x8000A5DC-0x8000A5E4, extabindex 0x80023C70-0x80023C7C,
 *   .data 0x80595058-0x80595118 (the `__FILE__` string "ef_sphere.cpp", the three assert messages, the class's
 *   table at 0x80595108), .sdata2 0x80796328-0x80796360.  Left edge: `ef/ef_point.cpp` ends there.  Right edge:
 *   `ef_cosf` is called only from `Emission`, while `fn_800CE5A8` (the 0x84D0 `system_w` side-table size) is
 *   called only from `ef/system_core.cpp`, which starts there.
 * FLAGS. `cflags_main` plus `-pool off`; file-wide `#pragma peephole off` and `#pragma fp_contract off`, as the
 *   sibling shapes.
 * NAMES. The class and method are nw4r's.  `cosf` (0x80463E50) is MSL's: its body calls `cos` (0x80467918); the
 *   shared dump's `tanf` there is wrong.
 *   GUESS: `ef_cosf` (0x800CE5A4): the out-of-line `cosf` thunk (`b cosf`), the sphere's twin of `ef_fabsf`.
 *   GUESS: `ef_sphere_file_name`, `ef_sphere_err_em`, `ef_sphere_err_pm`, `ef_sphere_err_params` (their text).
 * RESIDUALS.
 *  - `Emission`: the register colouring differs from retail's - `flags` takes r24 where retail's takes r23, the
 *    ring counter r23 where retail's takes r25, and a few loop floats sit one or two FPRs off; the `.sdata2`
 *    literals are our pool's `@N`.
 * SHAPES. The function is `nw4r::ef::EmitterFormSphere::Emission` (declared in `ef/ef_emform.h`; this unit defines the key
 *   function, so the class's table is emitted here, after the strings).  The particle manager's spawn is its
 *   virtual `CreateParticle`, taking the position and velocity by value (copied velocity first, momentum
 *   evaluated before the life, dispatch through r3); the float argument precedes the space matrix.
 * SHAPES. The strings are global definitions in retail's order; `-pool off` (configure.py) gives each its own
 *   `lis`/`addi`.
 * SHAPES. The `.sdata2` constants are literals, so MWCC pools and hoists them as retail does.
 * SHAPES. `#line 42` puts the three `EF_ASSERT_PTR` sites on lines 42-44.
 */

#include "ef.h"
#include "ef/ef_emform.h" /* nw4r::ef::EmitterFormSphere, ParticleManager (rule 1) */
#include "ef/ef_vec3_normalize_to.h" /* ef_vec3_normalize_to (rule 2) */
#include "MSL_C/alloc.h"             /* cosf (rule 2) */

#pragma peephole off
#pragma fp_contract off


/* The unit's `.data` strings, in retail order: the file name and one message per checked pointer (the
 * class's table follows them). */
char ef_sphere_file_name[] = "ef_sphere.cpp";
char ef_sphere_err_em[] = "NW4R:Pointer Error\nem(=%p) is not valid pointer.";
char ef_sphere_err_pm[] = "NW4R:Pointer Error\npm(=%p) is not valid pointer.";
char ef_sphere_err_params[] = "NW4R:Pointer Error\nparams(=%p) is not valid pointer.";

extern "C" {

f32 ef_cosf(f32 x);

} /* extern "C" */

/* 0x800CDB2C (0xA78): emits `count` particles from the sphere: on latitude rings (flag 0x20000; `count * count * 4
 * + 2` of them, the ring sizes growing by four to the equator and shrinking after it) or at random inside the
 * shell the hollow ratio leaves. */
void nw4r::ef::EmitterFormSphere::Emission(EfWork* em, ParticleManager* pm, int count, u32 flags, f32* paramBlock,
                                           u16 id, f32 scale, const MTX34* spawn_arg) {
    EfParams* params = (EfParams*)paramBlock;
    f32 dist;
    f32 hollow, radius_x, radius_y, radius_z, base;

#line 42
    EF_ASSERT_PTR(ef_sphere_file_name, ef_sphere_err_em, em);
    EF_ASSERT_PTR(ef_sphere_file_name, ef_sphere_err_pm, pm);
    EF_ASSERT_PTR(ef_sphere_file_name, ef_sphere_err_params, params);

    if (count < 1) {
        return;
    }

    hollow = params->rate / 100.0f;
    radius_x = ef_fabsf(params->scale_x) > 1.1920929e-7f ? params->scale_x : 1.1920929e-7f;
    if (flags & 0x02000000) {
        radius_y = radius_x;
        radius_z = radius_x;
    } else {
        radius_y = ef_fabsf(params->scale_z) > 1.1920929e-7f ? params->scale_z : 1.1920929e-7f;
        radius_z = ef_fabsf(params->scale_c) > 1.1920929e-7f ? params->scale_c : 1.1920929e-7f;
    }
    base = params->angle_base;
    if (!(flags & 0x00040000)) {
        base += 2.0f * (3.1415927f * ef_random_float(&em->progress));
    }

    if (flags & 0x00020000) {
        s32 rows;
        s32 rings = count * 2 + 1;
        s32 ring = 0;
        s32 per_ring = 1;
        s32 slot = 0;
        f32 sweep = params->angle_end - params->angle_base;
        u32 closed = 0;
        s32 i;
        f32 t = fmodf(sweep, 6.2831855f);

        if (t < 0.0001917476f || t > 6.2829938f) {
            closed = 1;
        }
        rows = 0;
        for (i = 0; i < count * (count * 4) + 2; i++) {
            f32 theta = 1.5707964f + 3.1415927f * ((f32)ring / (f32)(rings - 1));
            f32 phi;
            f32 r;
            VEC3 pos, out, norm, side;

            if (per_ring == 1) {
                phi = base;
            } else if (closed) {
                phi = base + sweep * ((f32)slot / (f32)per_ring);
            } else {
                phi = base + sweep * ((f32)slot / (f32)(per_ring - 1));
            }
            slot++;
            if (slot == per_ring) {
                slot = 0;
                rows += 2;
                ring++;
                if (rows < rings) {
                    per_ring = per_ring != 1 ? per_ring + 4 : per_ring + 3;
                } else {
                    per_ring = per_ring == 4 ? 1 : per_ring - 4;
                }
            }
            r = ef_random_float(&em->progress);
            if (flags & 0x01000000) {
                r = 1.0f - r * (r * r);
            }
            dist = hollow + r * (1.0f - hollow);

            VEC3_ctor(&pos);
            VEC3_ctor(&out);
            VEC3_ctor(&norm);
            VEC3_ctor(&side);
            side.y = 0.0f;
            ef_sin_cos(&side.x, &side.z, phi);
            side.x *= -1.0f;
            ef_vec_sin_cos((Vec*)&pos.y, theta);
            pos.x = side.x * (pos.z * (radius_x * dist));
            pos.y *= -radius_y * dist;
            pos.z = pos.z * (side.z * (radius_z * dist));
            ef_vec3_normalize_to(&norm, &pos);
            if (ef_cosf(theta) < 0.0f) {
                side.x = -side.x;
                side.z = -side.z;
            }
            CalcVelocity((Vec*)&out, em, (Vec*)&pos, (Vec*)&norm, (Vec*)&norm, (Vec*)&side);
            pm->CreateParticle(CalcLife(id, scale, em), pos, out, spawn_arg,
                               1.0f + 0.01f * (f32)em->scale_rate * ef_random_float(&em->progress), &em->spawn_data,
                               em->spawn_extra, em->spawn_flag);
        }
        return;
    }

    for (s32 i = 0; i < count; i++) {
        f32 r;
        f32 phi;
        f32 sin_lat, cos_lat, sin_lon, cos_lon;
        VEC3 pos, out, norm, flat;

        VEC3_ctor(&pos);
        VEC3_ctor(&out);
        r = ef_random_float(&em->progress);
        hollow = params->rate / 100.0f;
        if (flags & 0x01000000) {
            r = 1.0f - r * (r * r);
            dist = r + hollow * (1.0f - r);
        } else {
            dist = r + hollow * (1.0f - r);
        }
        phi = base + (params->angle_end - params->angle_base) * ef_random_float(&em->progress);
        ef_sin_cos(&sin_lat, &cos_lat, 1.5707964f + 3.1415927f * ef_random_float(&em->progress));
        ef_sin_cos(&sin_lon, &cos_lon, phi);
        pos.x = sin_lon * (-cos_lat * (radius_x * dist));
        pos.y = -sin_lat * (radius_y * dist);
        pos.z = cos_lon * (cos_lat * (radius_z * dist));
        VEC3_ctor(&norm);
        ef_vec3_normalize_to(&norm, &pos);
        assignVec3((Vec*)&flat, (Vec*)&pos);
        flat.y = 0.0f;
        ef_vec3_normalize_to(&flat, &flat);
        CalcVelocity((Vec*)&out, em, (Vec*)&pos, (Vec*)&norm, (Vec*)&norm, (Vec*)&flat);
        pm->CreateParticle(CalcLife(id, scale, em), pos, out, spawn_arg,
                           1.0f + 0.01f * (f32)em->scale_rate * ef_random_float(&em->progress), &em->spawn_data,
                           em->spawn_extra, em->spawn_flag);
    }
}

extern "C" {

/* 0x800CE5A4 (0x4): cosf, reached through a 4-byte tail-call thunk. */
f32 ef_cosf(f32 x) {
    return cosf(x);
}

} /* extern "C" */
