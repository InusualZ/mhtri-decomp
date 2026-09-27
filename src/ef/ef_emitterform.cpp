/*
 * ef/ef_emitterform.cpp - the nw4r effect emitter-form transform builder.
 *
 * The range this unit owns is `.text` 0x800A99B4..0x800AA18C (2 functions, 0x7D8 B), reconstructed
 * from the split object `build/RMHE08/obj/auto/800A99B4_fn_800A99B4.o`.  The name is evidence class 1:
 * the object's two `nw4r::db::Panic` calls pass the bare `__FILE__` string "ef_emitterform.cpp"
 * (`lbl_80592CD0`, read from the DOL at that address), so the module is `ef` and the extension `.cpp`.
 * The second half of the proposal range (0x800AA18C..0x800AB658) is a *different* translation unit:
 * every one of its Panic calls names "ef_particle.cpp" (`lbl_80592D50`), so it is registered separately
 * as `ef/ef_particle.cpp`.
 *
 * fn_800A99B4 builds a spawn transform out of an `EfWork` record: it asserts the result pointer and the
 * work record, seeds the result from the emitter's direction vector (or zero), then composes four
 * optional stages - a direction blend, a rotation, a random Euler spread and an axis-angle spread -
 * followed by a per-frame size jitter.  fn_800A9FB0 scales a 16-bit id into a per-instance variation of
 * that id.
 *
 * State (measured with `recompile.py`'s report path, target object
 * `build/RMHE08/obj/auto/800A99B4_fn_800A99B4.o`):
 *   fn_800A9FB0  100.00 %
 *   fn_800A99B4   90.97 %  - residual is register allocation and the 0x120-byte frame: retail keeps
 *     two separate matrices/vectors (r1+0x68 and r1+0x98), this build's allocation of the two
 *     axis-angle branches and of the sincos pair differs, and the spread expression's `out->z` term
 *     associates as `c2 * (em->spread_scale * c1)` in retail where this source writes
 *     `em->spread_scale * (c2 * c1)`.  Two pragmas are load-bearing:
 *     `#pragma peephole off` (retail's epilogue is `li r0,N; psq_lx`, the peephole rewrites it to
 *     `psq_l N(r1)`) and `#pragma fp_contract off` (retail keeps every `a*b+c` as fmuls/fadds).
 *
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with
 * `python tools/symbols/dumpmap.py lookup <addr>` for all 21 symbols of the proposal - every one
 * resolves to `map=fn_XXXXXXXX`, and the shared dump has no name for any of the helpers either).
 */

#include "types.h"
#include "ef.h"
#include "unsplit/g3d.h"
#include "fn_8004CAD8/mtx.h" /* the symbols deleted above (rule 2) */

/* This unit's own pooled data (still another unit's range in splits.txt - declared, never defined). */
extern char lbl_80592CD0[]; /* "ef_emitterform.cpp"                                          .data */
extern char lbl_80592CE4[]; /* "NW4R:Pointer Error\nresult(=%p) is not valid pointer."       .data */
extern char lbl_80592D1C[]; /* "NW4R:Pointer Error\nem(=%p) is not valid pointer."           .data */

/* This unit's own .sdata2 pool (unclaimed; the split owns no data section). */
extern f32 lbl_80796030; /* 0.0f                    .sdata2 */
extern f32 lbl_80796034; /* 2.0f                    .sdata2 */
extern f32 lbl_80796038; /* pi                      .sdata2 */
extern f32 lbl_8079603C; /* 1.0f                    .sdata2 */
extern f32 lbl_80796040; /* 0.01f                   .sdata2 */
extern f64 lbl_80796048; /* 0x4330000080000000, the int -> f64 magic  .sdata2 */
extern f32 lbl_80796058; /* 65535.0f                .sdata2 */

/* nw4r::math helpers, all still `fn_*` in the symbol map and unsplit (no owner file to move the
 * declaration to - the rule-2 gap the campaign records for an unsplit address).  The target object
 * references them by their plain `fn_XXXXXXXX` map name, so they are declared with C linkage here;
 * a C++ spelling mangles them and the reloc no longer pairs (relocaudit). */
extern "C" {
                                    /* unit matrix */
void fn_8009CA30(void* mtx, f32 x, f32 y, f32 z);               /* Euler rotation */
void fn_800514FC(void* out, const void* mtx, const void* in);   /* mulVecMat */
void fn_80051424(void* out, const void* in);                    /* copy */
void fn_800513CC(void* out, const void* a, const void* b);      /* blend */
}

/* nw4r::db::Panic.  The map already carries its real C++ mangling, and declaring the C++ spelling is
 * what reproduces the map's symbol exactly (tools/units/mangle.py). */
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

/* Builds the particle spawn transform `out` from the emitter work record `em`.
 *
 * `dir` is the emitter's base direction, `offset` the emitter's position offset and `rot`/`rot2`
 * (params 6 and 7, `b`/`c` in `nw4r::ef`'s own vocabulary) the two rotated axis vectors.  Stages 1-5
 * each sit behind a non-zero weight in `em`; retail evaluates them in this order, so the source order
 * is load-bearing. */
void fn_800A99B4(s32 ctx, Vec* out, EfWork* em, Vec* dir, Vec* offset, Vec* rot, Vec* rot2)
{
    Mtx34 mtx_a;
    VEC3 axis_a;
    Mtx34 mtx_b;
    Vec axis_b;
    VEC3 tmp;
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
        nw4r::db::Panic(lbl_80592CD0, 48, lbl_80592CE4, out);
    }
    ok = IS_VALID_PTR(em);
    if (!ok) {
        nw4r::db::Panic(lbl_80592CD0, 49, lbl_80592D1C, em);
    }

    VEC3_ctor(&tmp);

    if (lbl_80796030 != em->dir_weight) {
        fn_80051424(out, rot);
    } else {
        out->x = lbl_80796030;
        out->y = lbl_80796030;
        out->z = lbl_80796030;
    }

    if (lbl_80796030 != em->rot_weight) {
        fn_80051424(&tmp, rot2);
        fn_800513CC(out, out, &tmp);
    }

    if (lbl_80796030 != em->spread_scale) {
        a1 = lbl_80796034 * (lbl_80796038 * fn_800A8A08(&em->progress));
        a2 = lbl_80796034 * (lbl_80796038 * fn_800A8A08(&em->progress));
        a3 = lbl_80796034 * (lbl_80796038 * fn_800A8A08(&em->progress));
        fn_8009C760(&s1, &c1, a1);
        fn_8009C760(&s2, &c2, a2);
        fn_8009C760(&s3, &c3, a3);

        out->x += em->spread_scale * (c3 * (c1 * s2) + s1 * s3);
        out->y += em->spread_scale * (s3 * (c1 * s2) - s1 * c3);
        out->z += em->spread_scale * (c2 * c1);
    }

    if (lbl_80796030 != em->offset_scale) {
        out->x += offset->x * em->offset_scale;
        out->y += offset->y * em->offset_scale;
        out->z += offset->z * em->offset_scale;
    }

    if (lbl_80796030 != em->axis_angle_scale) {
        if (lbl_80796030 == em->axis_angle_y) {
            MTX34_ctor(&mtx_a);
            fn_8009CA30(&mtx_a, em->euler_x, em->euler_y, em->euler_z);
            setVec3(&axis_a, lbl_80796030, lbl_8079603C, lbl_80796030);
            fn_800514FC(&axis_a, &mtx_a, &axis_a);
            out->x += em->axis_angle_scale * axis_a.x;
            out->y += em->axis_angle_scale * axis_a.y;
            out->z += em->axis_angle_scale * axis_a.z;
        } else {
            f32 ang;

            MTX34_ctor(&mtx_b);
            ang = lbl_80796034 * (lbl_80796038 * fn_800A8A08(&em->progress));
            fn_8009CA30(&mtx_b, em->axis_angle_y * fn_800A8A08(&em->progress), ang, lbl_80796030);
            /* `axis_b` is the matrix record; its declaration is short (see the unit header). */
            MTX34_ctor((MTX34*)&axis_b);
            fn_8009CA30(&axis_b, em->euler_x, em->euler_y, em->euler_z);
            fn_800710BC(&mtx_b, (const Mtx34*)&axis_b, &mtx_b);
            out->x += em->axis_angle_scale * mtx_b.m[0][1];
            out->y += em->axis_angle_scale * mtx_b.m[1][1];
            out->z += em->axis_angle_scale * mtx_b.m[2][1];
        }
    }

    if (em->size_jitter != 0) {
        jitter = lbl_8079603C -
                  (lbl_80796040 * (f32)em->size_jitter) *
                      (lbl_80796034 * fn_800A8A08(&em->progress) - lbl_8079603C);
        out->x *= jitter;
        out->y *= jitter;
        out->z *= jitter;
    }
}

/* Maps the emitter's 16-bit instance id `id` into the id the particle is spawned with: a per-instance
 * fractional offset from the emitter's progress, clamped to [1, 65535]. */
u16 fn_800A9FB0(s32 ctx, u16 id, f32 scale, EfWork* em)
{
    f32 v;
    int ok;

    ok = IS_VALID_PTR(em);
    if (!ok) {
        nw4r::db::Panic(lbl_80592CD0, 160, lbl_80592D1C, em);
    }

    if (lbl_80796030 != scale) {
        v = scale * ((f32)(u16)id * fn_800A8A08(&em->progress));
        if ((f32)(u16)id - v < lbl_8079603C) {
            id = 1;
        } else if ((f32)(u16)id - v > lbl_80796058) {
            id = 65535;
        } else {
            id = (u16)(id - (s32)v);
        }
    }
    return id;
}
