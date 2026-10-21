/*
 * ef/ef_emitterform.cpp - the two helpers every emitter form calls: `EmitterForm::CalcVelocity` (asserts `out` and
 *   `em`, seeds from the emitter's direction, then a direction blend, a rotation, a random Euler spread, an
 *   axis-angle spread and a size jitter) and `EmitterForm::CalcLife`, which varies a 16-bit particle life per
 *   instance.
 * RANGE. .text 0x800A99B4-0x800AA18C (2 functions); extab 0x80009DF4-0x80009E04, extabindex 0x80023094-0x800230AC,
 *   .data 0x80592CD0-0x80592D50 (the `__FILE__` string "ef_emitterform.cpp" first), .sdata2 0x80796030-0x80796060.
 * FLAGS. `cflags_main`; file-wide `#pragma peephole off` (retail's epilogue is `li r0,N; psq_lx`) and `#pragma
 *   fp_contract off` (retail keeps every `a*b+c` as `fmuls`/`fadds`).
 * NAMES. The class is nw4r's (`nw4r::ef::EmitterForm`, declared in `ef/ef_emform.h`); the two members are GUESSES.
 *   GUESS: `CalcVelocity` (0x800A99B4): nw4r's spawn-velocity helper, from what it builds.
 *   GUESS: `CalcLife` (0x800A9FB0): nw4r's per-instance life helper, from what it returns.
 * RESIDUALS. 1 partial row, `CalcVelocity`: the two `0.0f != weight` tests load the constant into f1 where retail
 *   loads it into f0 (the operands of `fcmpu` and the three stores of the zeroing branch follow); tried: either
 *   operand order, `if (w)`, a chained or reversed zeroing, a `zero` local for one or both tests, an inverted test.
 *   flipcheck: `.text` only (`.data` and `.sdata2` are byte-identical).
 * NAMES. GUESS: `ef_emitterform_life_max` (0x80796058, map `lbl_80796058`): `CalcLife`'s upper clamp, 65535.0f.
 * SHAPES. The five stages of `CalcVelocity` are in retail's evaluation order.  The second axis-angle matrix is a
 *   full `Mtx34` (retail's 0x120 frame), and the spread's `z` term and the axis-angle adds keep retail's operand order.
 * SHAPES. The strings and float constants are literals under `#pragma pool_data off` (each string its own
 *   `lis`/`addi`), pooled in creation order; `ef_emitterform_life_max` is a named constant defined after `CalcLife`,
 *   so it pools after the `(f32)u16` conversion constant as in retail.
 */

#include "types.h"
#include "ef.h"
#include "ef/ef_emform.h" /* nw4r::ef::EmitterForm, the owner of these two members */
#include "g3d/g3d_calcview.h" /* fn_800710BC (rule 2) */
#include "fn_8004CAD8/mtx.h" /* the symbols deleted above (rule 2) */

/* nw4r::math helpers of `ef/ef_util.cpp` and `fn_8004CAD8.cpp`; retail's relocations carry their plain
 * map names, so they have C linkage. */
extern "C" {
void ef_mtx34_rotate_xyz(void* mtx, f32 x, f32 y, f32 z);               /* Euler rotation */
void mtx34_mult_vec3(void* out, const void* mtx, const void* in);   /* mulVecMat */
void vec3_scale_by(void* out, const void* in);                    /* copy */
void vec3_add_ps(void* out, const void* a, const void* b);      /* blend */
}

/* `nw4r::db::Panic`, declared in its namespace so the front-end emits the map's mangling
 * (`Panic__Q24nw4r2dbFPCciPCce`). */
namespace nw4r { namespace db { void Panic(const char* file, int line, const char* fmt, ...); } }

/* The engine's pointer validity test: the address has to fall in one of the memory regions the game
 * allocates from.  Retail materialises it as one `||` chain (`li`/`li ...,0` per region), so it must
 * stay a single expression assigned to a variable - an inline `if (!...)` compiles to branches. */
#define IS_VALID_PTR(p)                         \
    ((((u32)(p) & 0xFF000000) == 0x80000000) || \
     (((u32)(p) & 0xFF800000) == 0x81000000) || \
     (((u32)(p) & 0xF8000000) == 0x90000000) || \
     (((u32)(p) & 0xFF000000) == 0xC0000000) || \
     (((u32)(p) & 0xFF800000) == 0xC1000000) || \
     (((u32)(p) & 0xF8000000) == 0xD0000000) || \
     (((u32)(p) & 0xFFFFC000) == 0xE0000000))

#pragma peephole off
#pragma fp_contract off
#pragma pool_data off

/* The life clamp's upper bound; defined after `CalcLife` so its pool entry follows the conversion constant. */
extern const f32 ef_emitterform_life_max;

/* Builds the particle spawn transform `out` from the emitter work record `em`, `dir` (base direction),
 * `offset` and the two rotated axes `rot`/`rot2`; each stage sits behind a non-zero weight in `em`. */
void nw4r::ef::EmitterForm::CalcVelocity(Vec* out, EfWork* em, Vec* dir, Vec* offset, Vec* rot, Vec* rot2)
{
    Mtx34 mtx_a;
    VEC3 tmp;
    VEC3 axis_a;
    Mtx34 mtx_b;
    Mtx34 axis_b;
    f32 ang;
    f32 s1;
    f32 c1;
    f32 s2;
    f32 c2;
    f32 s3;
    f32 c3;
    f32 a1;
    f32 a2;
    f32 a3;
    f32 jitter;
    int ok;

    ok = IS_VALID_PTR(out);
    if (!ok) {
        nw4r::db::Panic("ef_emitterform.cpp", 48, "NW4R:Pointer Error\nresult(=%p) is not valid pointer.", out);
    }
    ok = IS_VALID_PTR(em);
    if (!ok) {
        nw4r::db::Panic("ef_emitterform.cpp", 49, "NW4R:Pointer Error\nem(=%p) is not valid pointer.", em);
    }

    VEC3_ctor(&tmp);

    if (0.0f != em->dir_weight) {
        vec3_scale_by(out, rot);
    } else {
        out->x = 0.0f;
        out->y = 0.0f;
        out->z = 0.0f;
    }

    if (0.0f != em->rot_weight) {
        vec3_scale_by(&tmp, rot2);
        vec3_add_ps(out, out, &tmp);
    }

    if (0.0f != em->spread_scale) {
        a1 = 2.0f * (3.1415927f * ef_random_float(&em->progress));
        a2 = 2.0f * (3.1415927f * ef_random_float(&em->progress));
        a3 = 2.0f * (3.1415927f * ef_random_float(&em->progress));
        ef_sin_cos(&s1, &c1, a1);
        ef_sin_cos(&s2, &c2, a2);
        ef_sin_cos(&s3, &c3, a3);

        out->x += em->spread_scale * (c3 * (c1 * s2) + s1 * s3);
        out->y += em->spread_scale * (s3 * (c1 * s2) - s1 * c3);
        out->z += c2 * (em->spread_scale * c1);
    }

    if (0.0f != em->offset_scale) {
        out->x += offset->x * em->offset_scale;
        out->y += offset->y * em->offset_scale;
        out->z += offset->z * em->offset_scale;
    }

    if (0.0f != em->axis_angle_scale) {
        if (0.0f == em->axis_angle_y) {
            MTX34_ctor(&mtx_a);
            ef_mtx34_rotate_xyz(&mtx_a, em->euler_x, em->euler_y, em->euler_z);
            setVec3(&axis_a, 0.0f, 1.0f, 0.0f);
            mtx34_mult_vec3(&axis_a, &mtx_a, &axis_a);
            out->x += em->axis_angle_scale * axis_a.x;
            out->y += em->axis_angle_scale * axis_a.y;
            out->z += em->axis_angle_scale * axis_a.z;
        } else {
            MTX34_ctor(&mtx_b);
            ang = 2.0f * (3.1415927f * ef_random_float(&em->progress));
            ef_mtx34_rotate_xyz(&mtx_b, em->axis_angle_y * ef_random_float(&em->progress), ang, 0.0f);
            MTX34_ctor(&axis_b);
            ef_mtx34_rotate_xyz(&axis_b, em->euler_x, em->euler_y, em->euler_z);
            mtx34_concat(&mtx_b, &axis_b, &mtx_b);
            out->x += mtx_b.m[0][1] * em->axis_angle_scale;
            out->y += mtx_b.m[1][1] * em->axis_angle_scale;
            out->z += mtx_b.m[2][1] * em->axis_angle_scale;
        }
    }

    if (em->size_jitter != 0) {
        jitter = 1.0f -
                  (0.01f * (f32)em->size_jitter) *
                      (2.0f * ef_random_float(&em->progress) - 1.0f);
        out->x *= jitter;
        out->y *= jitter;
        out->z *= jitter;
    }
}

/* Maps the emitter's 16-bit instance id `id` into the id the particle is spawned with: a per-instance
 * fractional offset from the emitter's progress, clamped to [1, 65535]. */
u16 nw4r::ef::EmitterForm::CalcLife(u16 id, f32 scale, EfWork* em)
{
    f32 v;
    int ok;

    ok = IS_VALID_PTR(em);
    if (!ok) {
        nw4r::db::Panic("ef_emitterform.cpp", 160, "NW4R:Pointer Error\nem(=%p) is not valid pointer.", em);
    }

    if (0.0f != scale) {
        v = scale * ((f32)(u16)id * ef_random_float(&em->progress));
        if ((f32)(u16)id - v < 1.0f) {
            id = 1;
        } else if ((f32)(u16)id - v > ef_emitterform_life_max) {
            id = 65535;
        } else {
            id = (u16)(id - (s32)v);
        }
    }
    return id;
}

const f32 ef_emitterform_life_max = 65535.0f;
