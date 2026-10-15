/*
 * ef/ef_postfield.cpp - nw4r::ef::PostField (the per-frame field step over a manager's particles: its collision,
 *   speed-limit, random and convergence sub-steps) and the res_drawparam_ac.h / res_emitterparam_ac.h inline
 *   accessors its TU emits, with their users.
 * RANGE. .text 0x800AEE48-0x800B2878 (29 functions); extab 0x80009F74-0x8000A044, extabindex 0x800232D4-0x8002340C,
 *   .data 0x80593580-0x80593690 (the `__FILE__` string "ef_postfield.cpp" first, then the header strings
 *   "res_drawparam_ac.h" 0x805935E4 and "res_emitterparam_ac.h" 0x8059362C/0x80593678), .sdata2
 *   0x807960E8-0x80796120 (read only by 0x800AEE48-0x800B1C80).  Right edge 0x800B2878: the three inline accessors
 *   that cite the header strings (0x800B23A4, 0x800B2598, 0x800B26F8) are each followed by their users (0x800B24BC..
 *   0x800B2590, 0x800B26B0/0x800B26D4, 0x800B2810/0x800B2840), and `ef/ef_resource.cpp` opens with the resource
 *   singleton's getter 0x800B2878 (it reads `.bss` 0x80694598, the `.ctors` entry's object).
 * FLAGS. `cflags_main`; file-wide `#pragma peephole off` and `#pragma fp_contract off` (retail keeps every
 *   `fmuls` + `fadds` pair unfused: the cylinder and circle tests lose 3 % each with it on); file-wide
 *   `#pragma pool_data off` (each string its own `lis`/`addi`); `#pragma dont_inline on` over the accessor tail
 *   (retail calls the three `_ac.h` accessors out of line).
 * NAMES. The file name is the TU's own `__FILE__` string (0x80593580), not a guess.  GUESS (from the bodies and the
 *   GUESS (collision dispatch, shapes 0 plane .. 5 cylinder): `ef_pf_collision_sphere`, `ef_pf_collision_cube`,
 *   GUESS: `ef_pf_collision_cylinder`, `ef_pf_collision_circle`, `ef_pf_collision_rectangle`, `ef_pf_collision_plane`,
 *   GUESS: `ef_pf_collision_test`, `ef_pf_hit_plane`, `ef_pf_hit_rectangle`, `ef_pf_hit_circle`, `ef_pf_hit_sphere`,
 *   GUESS: `ef_pf_hit_cylinder`, `ef_pf_hit_cube`, `ef_pf_collide`, `ef_pf_calc_particle`, `ef_vec3_lerp`,
 *   GUESS: `ef_res_drawparam_data`, `ef_res_emitterparam_cdata`, `ef_res_emitterparam_data`; the `.sdata2`
 *   constants `ef_pf_*` and the strings by their values.
 *   GUESS (from the bodies): `vec3_sub_assign` (nw4r's `VEC3::operator-=`), `ef_res_drawparam_set_flag_0400`,
 *   GUESS: `ef_res_slot_set_emitter_desc`, `ef_res_slot_set_params` (the dump's folded `GXInitTexObjUserData`),
 *   GUESS: `ef_res_slot_bind_emitter_desc`, `ef_res_slot_bind_params`, `ef_res_emitterparam_scale`,
 *   GUESS: `ef_res_emitterparam_vec`, `ef_res_emitterparam_set_scale`, `ef_res_emitterparam_set_vec`.
 *   GUESS: `ef_res_drawparam_ref`, `ef_res_emitterparam_cref`, `ef_res_emitterparam_ref` (the dead-stripped
 *   readers that place the accessors' weak copies).
 * SHAPES. `ef_vec3_lerp` is an `asm` function (retail's own paired-single body).  The hit tests take the crossing's
 *   step as one `f32 none = 2` returned at the end.  Every pool constant is a literal (the unit emits its own
 *   `.sdata2` pool in first-use order); `ef_pf_calc_particle` takes a non-const `EfPostFieldInfo*` (retail reloads
 *   the scale after each store) and switches on the collision mode as `u32` (`cmplwi`).
 * SHAPES. The parameter accessors use `EF_VALID_PTR_ASSERT` (`ef/ef_pointer_assert.h`): the flags are set
 *   before the pointer is read, as retail's.
 * SHAPES. The functions are defined in retail's address order: `ef_vec3_lerp` after `ef_pf_hit_plane` and
 *   `vec3_sub_assign` after `ef_pf_hit_sphere`, their first callers.  The three `_ac.h` accessors are `inline`
 *   (their strings are the weak `@STRING@` tail after the unit's literals, one copy per accessor); each weak copy
 *   follows a dead-stripped one-call reader (`ef_res_drawparam_ref`, `ef_res_emitterparam_cref`,
 *   `ef_res_emitterparam_ref`), and the accessors are defined cdata, data, drawparam so the tail comes out in
 *   retail's reverse order.
 * RESIDUALS. none in code or data.  flipcheck reads the `.sdata2` layout as different: the map names the pool
 *   words (`ef_pf_*`) where our object's literals are anonymous `@` constants of the same bytes at the same
 *   offsets.
 */

#include "types.h"
#include "gx.h"
#include "ef.h"
#include "ef/ef_pointer_assert.h" /* EF_VALID_PTR_ASSERT */
#include "ef/ef_drawstrategyimpl.h"
#include "ef/fn_800AEE48.h"
#include "ef/ef_postfield.h"
#include "sys_mem.h"
#include "unsplit/ef.h"
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "fn_8004CAD8.h" /* vec3_length_sq, abs_f32 (rule 2) */
#include "nw4r/mtx34_mult_vec3.h" /* mtx34_mult_vec3 (rule 2) */
#include "g3d/mtx34_inverse.h" /* mtx34_inverse (rule 2) */
#include "ef/ef_creationqueue.h" /* the creation-queue Add paths (rule 2) */
#include "ef/ef_vec3_normalize_to.h" /* ef_vec3_normalize_to (rule 2) */
#include "g3d/fn_8005AA28.h" /* mtx34_trans_apply (rule 2) */
#include "g3d/g3d_calcview.h" /* mtx34_concat (rule 2) */
#include "ef/ef_vec3_dist_sq.h" /* ef_vec3_dist_sq, ef_mtx34_copy (rule 2) */
#include "ef/ef_mtx34_scale_columns.h" /* ef_mtx34_scale_columns, ef_mtx34_rotate_xyz (rule 2) */

#ifdef __cplusplus
namespace nw4r {
namespace db {
void Panic(const char* file, int line, const char* fmt, ...);
}  // namespace db
}  // namespace nw4r
extern "C" {
#endif

#pragma peephole off
#pragma fp_contract off
#pragma pool_data off

/* This TU's `__FILE__`/assert strings (its claimed `.data`), declared, never defined. */
/* The pool constants without an exact short spelling (`.sdata2`, emitted in first-use order). */
#define EF_PF_EPSILON 1.1920929e-07f        /* FLT_EPSILON */
#define EF_PF_ONE_ULP1 1.0000001f           /* 1.0f + 1 ulp */
#define EF_PF_ONE_ULP2 1.0000002f           /* 1.0f + 2 ulp */
#define EF_PF_ONE_PLUS_1E5 1.00001f
#define EF_PF_ONE_MINUS_1E5 0.99999f
#define EF_PF_TINY 1.4210855e-14f           /* 2^-46 */


/* The post-field's collision record (`nw4r::ef::PostFieldInfo`): the shape, its axis/option byte, the response flags
 * and the scale the shape is normalised by. */
typedef struct EfPostFieldInfo {
    /* +0x00 */ u8 pad_0x00[0x24];
    /* +0x24 */ f32 speed_limit;     /* the largest speed the speed mode allows */
    /* +0x28 */ u8 speed_mode;       /* 1 clamp the speed, 2 kill a particle above it */
    /* +0x29 */ u8 shape;            /* 0 plane, 1 rectangle, 2 circle, 3 cube, 4 sphere, 5 cylinder */
    /* +0x2A */ u8 option;           /* the plane axis, or a sphere's hemisphere bits */
    /* +0x2B */ u8 collision_mode;   /* 0 sweep the step, 1 act while inside, 2 act while outside */
    /* +0x2C */ u16 flags;           /* bit 0 follow the emitter, 1 reflect, 3 stop, 4/5 spawn on hit, 6 kill */
    /* +0x2E */ u16 start_age;       /* the age before which the field does nothing */
    /* +0x30 */ VEC3 scale;          /* the shape's extent: positions are divided by it */
    /* +0x3C */ u8 spawn_setting[0x0A]; /* the creation-queue setting record handed to the spawn calls */
} EfPostFieldInfo; /* size: 0x46 (lower bound) */

/* The field's own transform: its shape's scale, rotation and translation. */
typedef struct EfPostFieldTransform {
    /* +0x00 */ VEC3 scale;
    /* +0x0C */ VEC3 rotate;
    /* +0x18 */ VEC3 translate;
} EfPostFieldTransform; /* size: 0x24 */

/* 0x800AEE48 (0x13C): where `pos` (in the field's unit space) lies against the unit sphere: 3 outside, 1 on its
 * surface (or on the cut plane of a hemisphere), 2 inside; `option` bit 0 keeps the upper half, bit 1 the lower. */
s32 ef_pf_collision_sphere(const VEC3* pos, u8 option) {
    VEC3 p;
    f32 length;

    assignVec3((Vec*)&p, (Vec*)pos);
    if (p.y < -1.0f || p.y > 1.0f) {
        return 3;
    }
    if ((option & 1) != 0 && p.y < 0.0f) {
        return 3;
    }
    if ((option & 2) != 0 && p.y > 0.0f) {
        return 3;
    }
    if (p.x < -1.0f || p.x > 1.0f) {
        return 3;
    }
    if (p.z < -1.0f || p.z > 1.0f) {
        return 3;
    }
    length = vec3_length_sq((const f32*)&p);
    if (length > 1.0f) {
        return 3;
    }
    if (1.0f == length) {
        return 1;
    }
    if (option != 0 && 0.0f == p.y) {
        return 1;
    }
    return 2;
}

/* 0x800AEF84 (0xE4): where `pos` lies against the unit cube: 3 outside, 1 on a face, 2 inside. */
s32 ef_pf_collision_cube(const VEC3* pos, u8 option) {
    VEC3 p;
    bool surface;

    assignVec3((Vec*)&p, (Vec*)pos);
    surface = false;
    if (p.x < -1.0f || p.x > 1.0f) {
        return 3;
    }
    if (1.0f == p.x || -1.0f == p.x) {
        surface = true;
    }
    if (p.y < -1.0f || p.y > 1.0f) {
        return 3;
    }
    if (1.0f == p.y || -1.0f == p.y) {
        surface = true;
    }
    if (p.z < -1.0f || p.z > 1.0f) {
        return 3;
    }
    if (surface || 1.0f == p.z || -1.0f == p.z) {
        return 1;
    }
    return 2;
}

/* 0x800AF068 (0xCC): where `pos` lies against the unit cylinder (axis Y): 3 outside, 1 on its surface, 2 inside. */
s32 ef_pf_collision_cylinder(const VEC3* pos, u8 option) {
    VEC3 p;
    f32 radius;

    assignVec3((Vec*)&p, (Vec*)pos);
    if (p.y < -1.0f || p.y > 1.0f) {
        return 3;
    }
    if (p.x < -1.0f || p.x > 1.0f) {
        return 3;
    }
    if (p.z < -1.0f || p.z > 1.0f) {
        return 3;
    }
    radius = p.x * p.x + p.z * p.z;
    if (radius > 1.0f) {
        return 3;
    }
    if (1.0f == radius) {
        return 1;
    }
    if (-1.0f == p.y || 1.0f == p.y) {
        return 1;
    }
    return 2;
}

/* 0x800AF134 (0x12C): where `pos` lies against the unit disc in the plane `axis` selects: 3 off the plane, 4 on it
 * outside the disc, 1 on the disc. */
s32 ef_pf_collision_circle(const VEC3* pos, u8 axis) {
    VEC3 p;

    VEC3_ctor(&p);
    switch (axis) {
    case 0:
        copyVec3(&p, pos);
        break;
    case 1:
        p.x = pos->x;
        p.y = pos->z;
        p.z = -pos->y;
        break;
    case 2:
        p.x = -pos->y;
        p.y = pos->x;
        p.z = pos->z;
        break;
    }
    if (p.y > 0.0f) {
        return 3;
    }
    if (p.y < 0.0f) {
        return 3;
    }
    if (p.x < -1.0f || p.x > 1.0f) {
        return 4;
    }
    if (p.z < -1.0f || p.z > 1.0f) {
        return 4;
    }
    if (p.x * p.x + p.z * p.z > 1.0f) {
        return 4;
    }
    return 1;
}

/* 0x800AF260 (0x110): where `pos` lies against the unit square in the plane `axis` selects: 3 off the plane, 4 on
 * it outside the square, 1 on the square. */
s32 ef_pf_collision_rectangle(const VEC3* pos, u8 axis) {
    VEC3 p;

    VEC3_ctor(&p);
    switch (axis) {
    case 0:
        copyVec3(&p, pos);
        break;
    case 1:
        p.x = pos->x;
        p.y = pos->z;
        p.z = -pos->y;
        break;
    case 2:
        p.x = -pos->y;
        p.y = pos->x;
        p.z = pos->z;
        break;
    }
    if (p.y > 0.0f) {
        return 3;
    }
    if (p.y < 0.0f) {
        return 3;
    }
    if (p.x < -1.0f || p.x > 1.0f) {
        return 4;
    }
    if (p.z < -1.0f || p.z > 1.0f) {
        return 4;
    }
    return 1;
}

/* 0x800AF370 (0xD0): which side of the plane `axis` selects `pos` lies on: 2 above, 3 below, 1 on it. */
s32 ef_pf_collision_plane(const VEC3* pos, u8 axis) {
    VEC3 p;

    VEC3_ctor(&p);
    switch (axis) {
    case 0:
        copyVec3(&p, pos);
        break;
    case 1:
        p.x = pos->x;
        p.y = pos->z;
        p.z = -pos->y;
        break;
    case 2:
        p.x = -pos->y;
        p.y = pos->x;
        p.z = pos->z;
        break;
    }
    if (p.y > 0.0f) {
        return 2;
    }
    if (p.y < 0.0f) {
        return 3;
    }
    return 1;
}

/* 0x800AF440 (0xD4): the collision test of `shape` (plane, rectangle, circle, cube, sphere, cylinder). */
s32 ef_pf_collision_test(u8 shape, const VEC3* pos, u8 option) {
    switch (shape) {
    case 0:
        return ef_pf_collision_plane(pos, option);
    case 1:
        return ef_pf_collision_rectangle(pos, option);
    case 2:
        return ef_pf_collision_circle(pos, option);
    case 3:
        return ef_pf_collision_cube(pos, option);
    case 4:
        return ef_pf_collision_sphere(pos, option);
    case 5:
        return ef_pf_collision_cylinder(pos, option);
    default:
        nw4r::db::Panic("ef_postfield.cpp", 302, "NW4R:Failed assertion 0");
        return 1;
    }
}

/* 0x800AF9FC (0x2C): `out = a + (b - a) * t`; defined after its first caller, as retail places it. */
void ef_vec3_lerp(VEC3* out, const VEC3* a, const VEC3* b, f32 t);

/* 0x800AF514 (0x4E8): the step from `prev` to `cur` against the plane the field's axis selects: the crossing's
 * parameter `t` (2 for none), the crossing point in `hit` and, in world space, in `hit_world`; a reflecting field
 * also turns the velocity and the step around and nudges the hit point off the plane. */
f32 ef_pf_hit_plane(const MTX34* mtx, const EfPostFieldInfo* info, const VEC3* cur, const VEC3* prev, VEC3* dir,
                    VEC3* velocity, VEC3* hit, VEC3* hit_world) {
    f32 none = 2.0f;

    switch (info->option) {
    case 0:
        if (abs_f32(prev->y) < EF_PF_EPSILON && abs_f32(cur->y) < EF_PF_EPSILON) {
            return 2.0f;
        }
        if (prev->y > 0.0f && cur->y < EF_PF_EPSILON) {
            VEC3 unused_a1;
            f32 t;

            VEC3_ctor(&unused_a1);
            t = -prev->y / dir->y;
            ef_vec3_lerp(hit, prev, cur, t);
            if ((info->flags & 2) != 0) {
                velocity->y *= -1.0f;
                dir->y *= -1.0f;
                hit->y += EF_PF_EPSILON;
            }
            mtx34_mult_vec3(hit_world, mtx, hit);
            return t;
        }
        if (prev->y < 0.0f && cur->y > EF_PF_EPSILON) {
            VEC3 unused_a2;
            f32 t;

            VEC3_ctor(&unused_a2);
            t = -prev->y / dir->y;
            ef_vec3_lerp(hit, prev, cur, t);
            if ((info->flags & 2) != 0) {
                velocity->y *= -1.0f;
                dir->y *= -1.0f;
                hit->y -= EF_PF_EPSILON;
            }
            mtx34_mult_vec3(hit_world, mtx, hit);
            return t;
        }
        return 2.0f;
    case 1:
        if (abs_f32(prev->z) < EF_PF_EPSILON && abs_f32(cur->z) < EF_PF_EPSILON) {
            return 2.0f;
        }
        if (prev->z > 0.0f && cur->z < EF_PF_EPSILON) {
            VEC3 unused_b1;
            f32 t;

            VEC3_ctor(&unused_b1);
            t = -prev->z / dir->z;
            ef_vec3_lerp(hit, prev, cur, t);
            if ((info->flags & 2) != 0) {
                velocity->z *= -1.0f;
                dir->z *= -1.0f;
                hit->z += EF_PF_EPSILON;
            }
            mtx34_mult_vec3(hit_world, mtx, hit);
            return t;
        }
        if (prev->z < 0.0f && cur->z > EF_PF_EPSILON) {
            VEC3 unused_b2;
            f32 t;

            VEC3_ctor(&unused_b2);
            t = -prev->z / dir->z;
            ef_vec3_lerp(hit, prev, cur, t);
            if ((info->flags & 2) != 0) {
                velocity->z *= -1.0f;
                dir->z *= -1.0f;
                hit->z -= EF_PF_EPSILON;
            }
            mtx34_mult_vec3(hit_world, mtx, hit);
            return t;
        }
        return 2.0f;
    case 2:
        if (abs_f32(prev->x) < EF_PF_EPSILON && abs_f32(cur->x) < EF_PF_EPSILON) {
            return 2.0f;
        }
        if (prev->x > 0.0f && cur->x < EF_PF_EPSILON) {
            VEC3 unused_c1;
            f32 t;

            VEC3_ctor(&unused_c1);
            t = -prev->x / dir->x;
            ef_vec3_lerp(hit, prev, cur, t);
            if ((info->flags & 2) != 0) {
                velocity->x *= -1.0f;
                dir->x *= -1.0f;
                hit->x += EF_PF_EPSILON;
            }
            mtx34_mult_vec3(hit_world, mtx, hit);
            return t;
        }
        if (prev->x < 0.0f && cur->x > EF_PF_EPSILON) {
            VEC3 unused_c2;
            f32 t;

            VEC3_ctor(&unused_c2);
            t = -prev->x / dir->x;
            ef_vec3_lerp(hit, prev, cur, t);
            if ((info->flags & 2) != 0) {
                velocity->x *= -1.0f;
                dir->x *= -1.0f;
                hit->x -= EF_PF_EPSILON;
            }
            mtx34_mult_vec3(hit_world, mtx, hit);
            return t;
        }
        return 2.0f;
    }
    return none;
}


/* 0x800AF9FC (0x2C): `out = a + (b - a) * t` (nw4r's paired-single VEC3 interpolation). */
asm void ef_vec3_lerp(register VEC3* out, register const VEC3* a, register const VEC3* b, register f32 t) {
    nofralloc
    psq_l      f0, 0(a), 0, 0
    psq_l      f2, 0(b), 0, 0
    ps_sub     f2, f2, f0
    ps_madds0  f2, f2, t, f0
    psq_st     f2, 0(out), 0, 0
    psq_l      f0, 8(a), 1, 0
    psq_l      f2, 8(b), 1, 0
    ps_sub     f2, f2, f0
    ps_madds0  f2, f2, t, f0
    psq_st     f2, 8(out), 1, 0
    blr
}

/* 0x800AFA28 (0x608): as the plane test, with the crossing point inside the unit square. */
f32 ef_pf_hit_rectangle(const MTX34* mtx, const EfPostFieldInfo* info, const VEC3* cur, const VEC3* prev,
                      VEC3* dir, VEC3* velocity, VEC3* hit, VEC3* hit_world) {
    f32 none = 2.0f;

    switch (info->option) {
    case 0:
        if (abs_f32(prev->y) < EF_PF_EPSILON && abs_f32(cur->y) < EF_PF_EPSILON) {
            return 2.0f;
        }
        if (prev->y > 0.0f && cur->y < EF_PF_EPSILON) {
            VEC3 unused_a1;
            f32 t;

            VEC3_ctor(&unused_a1);
            t = -prev->y / dir->y;
            ef_vec3_lerp(hit, prev, cur, t);
            if (abs_f32(hit->x) < EF_PF_ONE_ULP1 && abs_f32(hit->z) < EF_PF_ONE_ULP1) {
                if ((info->flags & 2) != 0) {
                    velocity->y *= -1.0f;
                    dir->y *= -1.0f;
                    hit->y += EF_PF_EPSILON;
                }
                mtx34_mult_vec3(hit_world, mtx, hit);
                return t;
            }
            return 2.0f;
        }
        if (prev->y < 0.0f && cur->y > EF_PF_EPSILON) {
            VEC3 unused_a2;
            f32 t;

            VEC3_ctor(&unused_a2);
            t = -prev->y / dir->y;
            ef_vec3_lerp(hit, prev, cur, t);
            if (abs_f32(hit->x) < EF_PF_ONE_ULP1 && abs_f32(hit->z) < EF_PF_ONE_ULP1) {
                if ((info->flags & 2) != 0) {
                    velocity->y *= -1.0f;
                    dir->y *= -1.0f;
                    hit->y -= EF_PF_EPSILON;
                }
                mtx34_mult_vec3(hit_world, mtx, hit);
                return t;
            }
            return 2.0f;
        }
        return 2.0f;
    case 1:
        if (abs_f32(prev->z) < EF_PF_EPSILON && abs_f32(cur->z) < EF_PF_EPSILON) {
            return 2.0f;
        }
        if (prev->z > 0.0f && cur->z < EF_PF_EPSILON) {
            VEC3 unused_b1;
            f32 t;

            VEC3_ctor(&unused_b1);
            t = -prev->z / dir->z;
            ef_vec3_lerp(hit, prev, cur, t);
            if (abs_f32(hit->x) < EF_PF_ONE_ULP1 && abs_f32(hit->y) < EF_PF_ONE_ULP1) {
                if ((info->flags & 2) != 0) {
                    velocity->z *= -1.0f;
                    dir->z *= -1.0f;
                    hit->z += EF_PF_EPSILON;
                }
                mtx34_mult_vec3(hit_world, mtx, hit);
                return t;
            }
            return 2.0f;
        }
        if (prev->z < 0.0f && cur->z > EF_PF_EPSILON) {
            VEC3 unused_b2;
            f32 t;

            VEC3_ctor(&unused_b2);
            t = -prev->z / dir->z;
            ef_vec3_lerp(hit, prev, cur, t);
            if (abs_f32(hit->x) < EF_PF_ONE_ULP1 && abs_f32(hit->y) < EF_PF_ONE_ULP1) {
                if ((info->flags & 2) != 0) {
                    velocity->z *= -1.0f;
                    dir->z *= -1.0f;
                    hit->z -= EF_PF_EPSILON;
                }
                mtx34_mult_vec3(hit_world, mtx, hit);
                return t;
            }
            return 2.0f;
        }
        return 2.0f;
    case 2:
        if (abs_f32(prev->x) < EF_PF_EPSILON && abs_f32(cur->x) < EF_PF_EPSILON) {
            return 2.0f;
        }
        if (prev->x > 0.0f && cur->x < EF_PF_EPSILON) {
            VEC3 unused_c1;
            f32 t;

            VEC3_ctor(&unused_c1);
            t = -prev->x / dir->x;
            ef_vec3_lerp(hit, prev, cur, t);
            if (abs_f32(hit->y) < EF_PF_ONE_ULP1 && abs_f32(hit->z) < EF_PF_ONE_ULP1) {
                if ((info->flags & 2) != 0) {
                    velocity->x *= -1.0f;
                    dir->x *= -1.0f;
                    hit->x += EF_PF_EPSILON;
                }
                mtx34_mult_vec3(hit_world, mtx, hit);
                return t;
            }
            return 2.0f;
        }
        if (prev->x < 0.0f && cur->x > EF_PF_EPSILON) {
            VEC3 unused_c2;
            f32 t;

            VEC3_ctor(&unused_c2);
            t = -prev->x / dir->x;
            ef_vec3_lerp(hit, prev, cur, t);
            if (abs_f32(hit->y) < EF_PF_ONE_ULP1 && abs_f32(hit->z) < EF_PF_ONE_ULP1) {
                if ((info->flags & 2) != 0) {
                    velocity->x *= -1.0f;
                    dir->x *= -1.0f;
                    hit->x -= EF_PF_EPSILON;
                }
                mtx34_mult_vec3(hit_world, mtx, hit);
                return t;
            }
            return 2.0f;
        }
        return 2.0f;
    }
    return none;
}


/* 0x800B0030 (0x5D8): as the plane test, with the crossing point inside the unit disc. */
f32 ef_pf_hit_circle(const MTX34* mtx, const EfPostFieldInfo* info, const VEC3* cur, const VEC3* prev,
                      VEC3* dir, VEC3* velocity, VEC3* hit, VEC3* hit_world) {
    f32 none = 2.0f;

    switch (info->option) {
    case 0:
        if (abs_f32(prev->y) < EF_PF_EPSILON && abs_f32(cur->y) < EF_PF_EPSILON) {
            return 2.0f;
        }
        if (prev->y > 0.0f && cur->y < EF_PF_EPSILON) {
            VEC3 unused_a1;
            f32 t;

            VEC3_ctor(&unused_a1);
            t = -prev->y / dir->y;
            ef_vec3_lerp(hit, prev, cur, t);
            if (hit->x * hit->x + hit->z * hit->z < EF_PF_ONE_ULP2) {
                if ((info->flags & 2) != 0) {
                    velocity->y *= -1.0f;
                    dir->y *= -1.0f;
                    hit->y += EF_PF_EPSILON;
                }
                mtx34_mult_vec3(hit_world, mtx, hit);
                return t;
            }
            return 2.0f;
        }
        if (prev->y < 0.0f && cur->y > EF_PF_EPSILON) {
            VEC3 unused_a2;
            f32 t;

            VEC3_ctor(&unused_a2);
            t = -prev->y / dir->y;
            ef_vec3_lerp(hit, prev, cur, t);
            if (hit->x * hit->x + hit->z * hit->z < EF_PF_ONE_ULP2) {
                if ((info->flags & 2) != 0) {
                    velocity->y *= -1.0f;
                    dir->y *= -1.0f;
                    hit->y -= EF_PF_EPSILON;
                }
                mtx34_mult_vec3(hit_world, mtx, hit);
                return t;
            }
            return 2.0f;
        }
        return 2.0f;
    case 1:
        if (abs_f32(prev->z) < EF_PF_EPSILON && abs_f32(cur->z) < EF_PF_EPSILON) {
            return 2.0f;
        }
        if (prev->z > 0.0f && cur->z < EF_PF_EPSILON) {
            VEC3 unused_b1;
            f32 t;

            VEC3_ctor(&unused_b1);
            t = -prev->z / dir->z;
            ef_vec3_lerp(hit, prev, cur, t);
            if (hit->x * hit->x + hit->y * hit->y < EF_PF_ONE_ULP2) {
                if ((info->flags & 2) != 0) {
                    velocity->z *= -1.0f;
                    dir->z *= -1.0f;
                    hit->z += EF_PF_EPSILON;
                }
                mtx34_mult_vec3(hit_world, mtx, hit);
                return t;
            }
            return 2.0f;
        }
        if (prev->z < 0.0f && cur->z > EF_PF_EPSILON) {
            VEC3 unused_b2;
            f32 t;

            VEC3_ctor(&unused_b2);
            t = -prev->z / dir->z;
            ef_vec3_lerp(hit, prev, cur, t);
            if (hit->x * hit->x + hit->y * hit->y < EF_PF_ONE_ULP2) {
                if ((info->flags & 2) != 0) {
                    velocity->z *= -1.0f;
                    dir->z *= -1.0f;
                    hit->z -= EF_PF_EPSILON;
                }
                mtx34_mult_vec3(hit_world, mtx, hit);
                return t;
            }
            return 2.0f;
        }
        return 2.0f;
    case 2:
        if (abs_f32(prev->x) < EF_PF_EPSILON && abs_f32(cur->x) < EF_PF_EPSILON) {
            return 2.0f;
        }
        if (prev->x > 0.0f && cur->x < EF_PF_EPSILON) {
            VEC3 unused_c1;
            f32 t;

            VEC3_ctor(&unused_c1);
            t = -prev->x / dir->x;
            ef_vec3_lerp(hit, prev, cur, t);
            if (hit->y * hit->y + hit->z * hit->z < EF_PF_ONE_ULP2) {
                if ((info->flags & 2) != 0) {
                    velocity->x *= -1.0f;
                    dir->x *= -1.0f;
                    hit->x += EF_PF_EPSILON;
                }
                mtx34_mult_vec3(hit_world, mtx, hit);
                return t;
            }
            return 2.0f;
        }
        if (prev->x < 0.0f && cur->x > EF_PF_EPSILON) {
            VEC3 unused_c2;
            f32 t;

            VEC3_ctor(&unused_c2);
            t = -prev->x / dir->x;
            ef_vec3_lerp(hit, prev, cur, t);
            if (hit->y * hit->y + hit->z * hit->z < EF_PF_ONE_ULP2) {
                if ((info->flags & 2) != 0) {
                    velocity->x *= -1.0f;
                    dir->x *= -1.0f;
                    hit->x -= EF_PF_EPSILON;
                }
                mtx34_mult_vec3(hit_world, mtx, hit);
                return t;
            }
            return 2.0f;
        }
        return 2.0f;
    }
    return none;
}


/* 0x800B0608 (0x588): the step from `prev` to `cur` against the unit sphere (a hemisphere's cap disc included):
 * the nearest crossing's `t` (2 for none) and point; a reflecting field mirrors the step and the velocity about the
 * surface normal and nudges the point off the surface. */
f32 ef_pf_hit_sphere(const MTX34* mtx, const EfPostFieldInfo* info, const VEC3* cur, const VEC3* prev, VEC3* dir,
                     VEC3* velocity, VEC3* hit, VEC3* hit_world) {
    f32 best = 2.0f;
    f32 prev_sq = vec3_length_sq((const f32*)prev);
    f32 dir_sq;
    f32 b;
    f32 disc;
    f32 root;
    f32 minus_two_b;
    f32 t0;
    f32 t1;

    vec3_length_sq((const f32*)cur);
    dir_sq = vec3_length_sq((const f32*)dir);
    b = vec3_dot((const f32*)prev, (const f32*)dir);
    disc = 4.0f * b * b - 4.0f * dir_sq * (prev_sq - 1.0f);
    if (disc < 0.0f) {
        return 2.0f;
    }
    root = sqrt_f32(disc);
    minus_two_b = -2.0f * b;
    t0 = (minus_two_b - root) / (2.0f * dir_sq);
    t1 = (minus_two_b + root) / (2.0f * dir_sq);
    if (t0 > 0.0f && t0 < EF_PF_ONE_ULP1) {
        ef_vec3_lerp(hit, prev, cur, t0);
        if (!(info->option == 1 && hit->y < -EF_PF_EPSILON) && !(info->option == 2 && hit->y > EF_PF_EPSILON)) {
            best = t0;
        }
    }
    if (t1 > 0.0f && t1 < EF_PF_ONE_ULP1 && t1 < best) {
        VEC3 far_hit;

        VEC3_ctor(&far_hit);
        ef_vec3_lerp(&far_hit, prev, cur, t1);
        if (!(info->option == 1 && far_hit.y < -EF_PF_EPSILON) &&
            !(info->option == 2 && far_hit.y > EF_PF_EPSILON)) {
            copyVec3(hit, &far_hit);
            best = t1;
        }
    }
    if ((u8)(info->option + 255) <= 1 && !(abs_f32(prev->y) < EF_PF_EPSILON && abs_f32(cur->y) < EF_PF_EPSILON)) {
        if (prev->y > 0.0f && cur->y < EF_PF_EPSILON) {
            VEC3 cap_hit_a;
            f32 t;

            VEC3_ctor(&cap_hit_a);
            t = -prev->y / dir->y;
            ef_vec3_lerp(&cap_hit_a, prev, cur, t);
            if (t < best && cap_hit_a.x * cap_hit_a.x + cap_hit_a.z * cap_hit_a.z < EF_PF_ONE_ULP2) {
                copyVec3(hit, &cap_hit_a);
                if ((info->flags & 2) != 0) {
                    velocity->y *= -1.0f;
                    dir->y *= -1.0f;
                    hit->y += EF_PF_EPSILON;
                }
                mtx34_mult_vec3(hit_world, mtx, hit);
                return t;
            }
        } else if (prev->y < 0.0f && cur->y > EF_PF_EPSILON) {
            VEC3 cap_hit_b;
            f32 t;

            VEC3_ctor(&cap_hit_b);
            t = -prev->y / dir->y;
            ef_vec3_lerp(&cap_hit_b, prev, cur, t);
            if (t < best && cap_hit_b.x * cap_hit_b.x + cap_hit_b.z * cap_hit_b.z < EF_PF_ONE_ULP2) {
                copyVec3(hit, &cap_hit_b);
                if ((info->flags & 2) != 0) {
                    velocity->y *= -1.0f;
                    dir->y *= -1.0f;
                    hit->y -= EF_PF_EPSILON;
                }
                mtx34_mult_vec3(hit_world, mtx, hit);
                return t;
            }
        }
    }
    if (best < 2.0f) {
        if ((info->flags & 2) != 0) {
            MTX34 frame;
            VEC3 local;
            VEC3 normal_part;
            f32 r = sqrt_f32(1.0f - hit->z * hit->z);
            f32 s;
            f32 c;

            if (r > EF_PF_EPSILON) {
                c = hit->y / r;
                s = -hit->x / r;
            } else {
                c = 1.0f;
                s = 0.0f;
            }
            MTX34_ctor(&frame);
            frame.m[0][0] = c;
            frame.m[0][1] = hit->x;
            frame.m[0][2] = hit->z * s;
            frame.m[0][3] = 0.0f;
            frame.m[1][0] = s;
            frame.m[1][1] = hit->y;
            frame.m[1][2] = -c * hit->z;
            frame.m[1][3] = 0.0f;
            frame.m[2][0] = 0.0f;
            frame.m[2][1] = hit->z;
            frame.m[2][2] = r;
            frame.m[2][3] = 0.0f;
            mtx34_inverse(&frame, &frame);
            VEC3_ctor(&local);
            VEC3_ctor(&normal_part);
            mtx34_mult_vec3(&local, &frame, dir);
            vec3_scale_by((f32*)&normal_part, (const f32*)hit, local.y);
            vec3_sub_assign((Vec*)dir, (Vec*)&normal_part);
            vec3_sub_assign((Vec*)dir, (Vec*)&normal_part);
            mtx34_mult_vec3(&local, &frame, velocity);
            vec3_scale_by((f32*)&normal_part, (const f32*)hit, local.y);
            vec3_sub_assign((Vec*)velocity, (Vec*)&normal_part);
            vec3_sub_assign((Vec*)velocity, (Vec*)&normal_part);
            if (prev_sq > 1.0f) {
                vec3_scale_by((f32*)hit, (const f32*)hit, EF_PF_ONE_PLUS_1E5);
            } else {
                vec3_scale_by((f32*)hit, (const f32*)hit, EF_PF_ONE_MINUS_1E5);
            }
        }
        mtx34_mult_vec3(hit_world, mtx, hit);
    }
    return best;
}


/* Subtracts `b` from `self` in place and returns `self`. */
Vec* vec3_sub_assign(Vec* self, Vec* b) {
    PSVECSubtract((f32*)self, (const f32*)self, (const f32*)b);
    return self;
}

/* 0x800B0BC8 (0x6E0): the step from `prev` to `cur` against the unit cylinder (axis Y, caps at y = +-1): the nearest
 * crossing's `t` (2 for none) and point; a reflecting field mirrors the step and the velocity about the side's
 * normal (or the cap's) and nudges the point off the surface. */
f32 ef_pf_hit_cylinder(const MTX34* mtx, const EfPostFieldInfo* info, const VEC3* cur, const VEC3* prev, VEC3* dir,
                       VEC3* velocity, VEC3* hit, VEC3* hit_world) {
    f32 best = 2.0f;
    f32 prev_sq = prev->x * prev->x + prev->z * prev->z;
    f32 dir_sq = dir->x * dir->x + dir->z * dir->z;
    f32 b = prev->x * dir->x + prev->z * dir->z;
    f32 disc;
    f32 root;
    f32 minus_two_b;
    f32 t0;
    f32 t1;

    disc = 4.0f * b * b - 4.0f * dir_sq * (prev_sq - 1.0f);
    if (disc < 0.0f) {
        return best;
    }
    root = sqrt_f32(disc);
    minus_two_b = -2.0f * b;
    t0 = (minus_two_b - root) / (2.0f * dir_sq);
    t1 = (minus_two_b + root) / (2.0f * dir_sq);
    if (t0 > 0.0f && t0 < EF_PF_ONE_ULP1) {
        ef_vec3_lerp(hit, prev, cur, t0);
        if (abs_f32(hit->y) < EF_PF_ONE_ULP1) {
            best = t0;
        }
    }
    if (t1 > 0.0f && t1 < EF_PF_ONE_ULP1 && t1 < best) {
        VEC3 far_hit;

        VEC3_ctor(&far_hit);
        ef_vec3_lerp(&far_hit, prev, cur, t1);
        if (abs_f32(far_hit.y) < EF_PF_ONE_ULP1) {
            copyVec3(hit, &far_hit);
            best = t1;
        }
    }
    {
        f32 level = 1.0f;

        if (!(abs_f32(prev->y - level) < EF_PF_EPSILON && abs_f32(cur->y - level) < EF_PF_EPSILON)) {
            if (prev->y > level && cur->y < EF_PF_EPSILON + level) {
                VEC3 cap_hit_a1;

                VEC3_ctor(&cap_hit_a1);
                t1 = (level - prev->y) / dir->y;
                ef_vec3_lerp(&cap_hit_a1, prev, cur, t1);
                if (t1 < best && cap_hit_a1.x * cap_hit_a1.x + cap_hit_a1.z * cap_hit_a1.z < EF_PF_ONE_ULP2) {
                    copyVec3(hit, &cap_hit_a1);
                    if ((info->flags & 2) != 0) {
                        velocity->y *= -1.0f;
                        dir->y *= -1.0f;
                        hit->y += EF_PF_EPSILON;
                    }
                    mtx34_mult_vec3(hit_world, mtx, hit);
                    return t1;
                }
            } else if (prev->y < level && cur->y > EF_PF_EPSILON + level) {
                VEC3 cap_hit_a2;

                VEC3_ctor(&cap_hit_a2);
                t1 = (level - prev->y) / dir->y;
                ef_vec3_lerp(&cap_hit_a2, prev, cur, t1);
                if (t1 < best && cap_hit_a2.x * cap_hit_a2.x + cap_hit_a2.z * cap_hit_a2.z < EF_PF_ONE_ULP2) {
                    copyVec3(hit, &cap_hit_a2);
                    if ((info->flags & 2) != 0) {
                        velocity->y *= -1.0f;
                        dir->y *= -1.0f;
                        hit->y -= EF_PF_EPSILON;
                    }
                    mtx34_mult_vec3(hit_world, mtx, hit);
                    return t1;
                }
            }
        }
    }
    {
        f32 level = -1.0f;

        if (!(abs_f32(prev->y - level) < EF_PF_EPSILON && abs_f32(cur->y - level) < EF_PF_EPSILON)) {
            if (prev->y > level && cur->y < EF_PF_EPSILON + level) {
                VEC3 cap_hit_b1;

                VEC3_ctor(&cap_hit_b1);
                t1 = (level - prev->y) / dir->y;
                ef_vec3_lerp(&cap_hit_b1, prev, cur, t1);
                if (t1 < best && cap_hit_b1.x * cap_hit_b1.x + cap_hit_b1.z * cap_hit_b1.z < EF_PF_ONE_ULP2) {
                    copyVec3(hit, &cap_hit_b1);
                    if ((info->flags & 2) != 0) {
                        velocity->y *= -1.0f;
                        dir->y *= -1.0f;
                        hit->y += EF_PF_EPSILON;
                    }
                    mtx34_mult_vec3(hit_world, mtx, hit);
                    return t1;
                }
            } else if (prev->y < level && cur->y > EF_PF_EPSILON + level) {
                VEC3 cap_hit_b2;

                VEC3_ctor(&cap_hit_b2);
                t1 = (level - prev->y) / dir->y;
                ef_vec3_lerp(&cap_hit_b2, prev, cur, t1);
                if (t1 < best && cap_hit_b2.x * cap_hit_b2.x + cap_hit_b2.z * cap_hit_b2.z < EF_PF_ONE_ULP2) {
                    copyVec3(hit, &cap_hit_b2);
                    if ((info->flags & 2) != 0) {
                        velocity->y *= -1.0f;
                        dir->y *= -1.0f;
                        hit->y -= EF_PF_EPSILON;
                    }
                    mtx34_mult_vec3(hit_world, mtx, hit);
                    return t1;
                }
            }
        }
    }
    if (best < 2.0f) {
        if ((info->flags & 2) != 0) {
            VEC3 normal;
            f32 d;

            VEC3_ctor(&normal);
            sqrt_f32(dir->x * dir->x + dir->z * dir->z);
            d = dir->x * hit->x + dir->z * hit->z;
            normal.x = hit->x;
            normal.y = 0.0f;
            normal.z = hit->z;
            vec3_normalize_into(&normal, &normal);
            vec3_scale_by((f32*)&normal, (const f32*)&normal, d);
            vec3_sub_assign((Vec*)dir, (Vec*)&normal);
            vec3_sub_assign((Vec*)dir, (Vec*)&normal);
            sqrt_f32(velocity->x * velocity->x + velocity->z * velocity->z);
            d = velocity->x * hit->x + velocity->z * hit->z;
            normal.x = hit->x;
            normal.y = 0.0f;
            normal.z = hit->z;
            vec3_normalize_into(&normal, &normal);
            vec3_scale_by((f32*)&normal, (const f32*)&normal, d);
            vec3_sub_assign((Vec*)velocity, (Vec*)&normal);
            vec3_sub_assign((Vec*)velocity, (Vec*)&normal);
            if (prev_sq > 1.0f) {
                vec3_scale_by((f32*)hit, (const f32*)hit, EF_PF_ONE_PLUS_1E5);
            } else {
                vec3_scale_by((f32*)hit, (const f32*)hit, EF_PF_ONE_MINUS_1E5);
            }
        }
        mtx34_mult_vec3(hit_world, mtx, hit);
    }
    return best;
}


/* 0x800B12A8 (0x648): the step from `prev` to `cur` against the unit cube: the nearest face crossing's `t` (2 for
 * none) and point; a reflecting field turns the step and the velocity around on the hit faces' axes and snaps the
 * point onto the face. */
f32 ef_pf_hit_cube(const MTX34* mtx, const EfPostFieldInfo* info, const VEC3* cur, const VEC3* prev, VEC3* dir,
                   VEC3* velocity, VEC3* hit, VEC3* hit_world) {
    f32 best = 2.0f;
    f32 t_px = best;
    f32 t_nx = best;
    f32 t_py = best;
    f32 t_ny = best;
    f32 t_pz = best;
    f32 t_nz = best;
    VEC3 face_hit;
    f32 t;

    VEC3_ctor(&face_hit);
    t = (1.0f - prev->x) / dir->x;
    if (t > 0.0f && t <= EF_PF_ONE_ULP1) {
        ef_vec3_lerp(&face_hit, prev, cur, t);
        if (abs_f32(face_hit.y) <= EF_PF_ONE_ULP1 && abs_f32(face_hit.z) <= EF_PF_ONE_ULP1) {
            t_px = t;
            if (t < best) {
                best = t;
            }
        }
    }
    t = (-1.0f - prev->x) / dir->x;
    if (t > 0.0f && t <= EF_PF_ONE_ULP1) {
        ef_vec3_lerp(&face_hit, prev, cur, t);
        if (abs_f32(face_hit.y) <= EF_PF_ONE_ULP1 && abs_f32(face_hit.z) <= EF_PF_ONE_ULP1) {
            t_nx = t;
            if (t < best) {
                best = t;
            }
        }
    }
    t = (1.0f - prev->y) / dir->y;
    if (t > 0.0f && t <= EF_PF_ONE_ULP1) {
        ef_vec3_lerp(&face_hit, prev, cur, t);
        if (abs_f32(face_hit.x) <= EF_PF_ONE_ULP1 && abs_f32(face_hit.z) <= EF_PF_ONE_ULP1) {
            t_py = t;
            if (t < best) {
                best = t;
            }
        }
    }
    t = (-1.0f - prev->y) / dir->y;
    if (t > 0.0f && t <= EF_PF_ONE_ULP1) {
        ef_vec3_lerp(&face_hit, prev, cur, t);
        if (abs_f32(face_hit.x) <= EF_PF_ONE_ULP1 && abs_f32(face_hit.z) <= EF_PF_ONE_ULP1) {
            t_ny = t;
            if (t < best) {
                best = t;
            }
        }
    }
    t = (1.0f - prev->z) / dir->z;
    if (t > 0.0f && t <= EF_PF_ONE_ULP1) {
        ef_vec3_lerp(&face_hit, prev, cur, t);
        if (abs_f32(face_hit.x) <= EF_PF_ONE_ULP1 && abs_f32(face_hit.y) <= EF_PF_ONE_ULP1) {
            t_pz = t;
            if (t < best) {
                best = t;
            }
        }
    }
    t = (-1.0f - prev->z) / dir->z;
    if (t > 0.0f && t <= EF_PF_ONE_ULP1) {
        ef_vec3_lerp(&face_hit, prev, cur, t);
        if (abs_f32(face_hit.x) <= EF_PF_ONE_ULP1 && abs_f32(face_hit.y) <= EF_PF_ONE_ULP1) {
            t_nz = t;
            if (t < best) {
                best = t;
            }
        }
    }
    if (2.0f == best) {
        return 2.0f;
    }
    hit->x = prev->x + best * (cur->x - prev->x);
    hit->y = prev->y + best * (cur->y - prev->y);
    hit->z = prev->z + best * (cur->z - prev->z);
    mtx34_mult_vec3(hit_world, mtx, hit);
    if ((info->flags & 2) != 0) {
        if (abs_f32(t_px - best) <= EF_PF_EPSILON || abs_f32(t_nx - best) <= EF_PF_EPSILON) {
            velocity->x *= -1.0f;
            dir->x *= -1.0f;
        }
        if (abs_f32(t_py - best) <= EF_PF_EPSILON || abs_f32(t_ny - best) <= EF_PF_EPSILON) {
            velocity->y *= -1.0f;
            dir->y *= -1.0f;
        }
        if (abs_f32(t_pz - best) <= EF_PF_EPSILON || abs_f32(t_nz - best) <= EF_PF_EPSILON) {
            velocity->z *= -1.0f;
            dir->z *= -1.0f;
        }
        if (abs_f32(t_px - best) <= EF_PF_EPSILON) {
            hit->x = 1.0f;
        }
        if (abs_f32(t_nx - best) <= EF_PF_EPSILON) {
            hit->x = -1.0f;
        }
        if (abs_f32(t_py - best) <= EF_PF_EPSILON) {
            hit->y = 1.0f;
        }
        if (abs_f32(t_ny - best) <= EF_PF_EPSILON) {
            hit->y = -1.0f;
        }
        if (abs_f32(t_pz - best) <= EF_PF_EPSILON) {
            hit->z = 1.0f;
        }
        if (abs_f32(t_nz - best) <= EF_PF_EPSILON) {
            hit->z = -1.0f;
        }
    }
    return best;
}


/* 0x800B18F0 (0x390): one particle step against the field's shape: the shape's hit test (0 when it is missed);
 * a reflecting or stopping field carries the rest of the step past the hit point into `out_pos`/`out_velocity`
 * (scaled back to world), a killing field flags the particle, and the spawn flags queue a creation at the hit. */
s32 ef_pf_collide(const MTX34* mtx, const MTX34* world_mtx, const EfPostFieldInfo* info, EffectHandle* eh,
                  EfDrawParticle* p, const VEC3* prev, const VEC3* cur, VEC3* velocity, u8* killed, VEC3* hit,
                  VEC3* hit_world, VEC3* out_pos, VEC3* out_velocity) {
    VEC3 dir;
    f32 t;

    ef_pf_collision_test(info->shape, cur, info->option);
    VEC3_ctor(&dir);
    PSVECSubtract((f32*)&dir, (const f32*)cur, (const f32*)prev);
    t = 2.0f;
    switch (info->shape) {
    case 0:
        t = ef_pf_hit_plane(mtx, info, cur, prev, &dir, velocity, hit, hit_world);
        break;
    case 1:
        t = ef_pf_hit_rectangle(mtx, info, cur, prev, &dir, velocity, hit, hit_world);
        break;
    case 2:
        t = ef_pf_hit_circle(mtx, info, cur, prev, &dir, velocity, hit, hit_world);
        break;
    case 3:
        t = ef_pf_hit_cube(mtx, info, cur, prev, &dir, velocity, hit, hit_world);
        break;
    case 4:
        t = ef_pf_hit_sphere(mtx, info, cur, prev, &dir, velocity, hit, hit_world);
        break;
    case 5:
        t = ef_pf_hit_cylinder(mtx, info, cur, prev, &dir, velocity, hit, hit_world);
        break;
    default:
        nw4r::db::Panic("ef_postfield.cpp", 1418, "NW4R:Failed assertion 0");
        break;
    }
    if (t > 1.5f) {
        return 0;
    }
    if ((info->flags & 2) != 0 || (info->flags & 8) != 0) {
        VEC3 rest;

        mtx34_mult_vec3(out_velocity, world_mtx, velocity);
        out_velocity->x *= info->scale.x;
        out_velocity->y *= info->scale.y;
        out_velocity->z *= info->scale.z;
        VEC3_ctor(&rest);
        vec3_scale_by((f32*)&rest, (const f32*)&dir, 1.0f - t);
        mtx34_mult_vec3(&rest, world_mtx, &rest);
        rest.x *= info->scale.x;
        rest.y *= info->scale.y;
        rest.z *= info->scale.z;
        vec3_add_ps(out_pos, hit_world, &rest);
    }
    if ((info->flags & 0x40) != 0) {
        *killed = 1;
        copyVec3(out_pos, hit_world);
    }
    if ((info->flags & 0x10) != 0 && eh != NULL) {
        ef_creation_queue_add_type0((CreationQueue*)p->manager->emitter->effect->system->creation_queue,
                                    (const Setting*)info->spawn_setting, (EffectManager*)p, eh, p->life, hit_world,
                                    NULL);
    }
    if ((info->flags & 0x20) != 0 && eh != NULL) {
        ef_creation_queue_add_type1((CreationQueue*)p->manager->emitter->effect->system->creation_queue,
                                    (const Setting*)info->spawn_setting, (EffectManager*)p, eh, p->life, hit_world,
                                    NULL);
    }
    return 1;
}


/* 0x800B1C80 (0x724): applies the post field to one particle: the field's matrix (its transform, optionally the
 * emitter's position, the manager's matrix), the shape test of the particle's position, the speed limit, then
 * either the inside/outside action or the swept step with up to five bounces; 0 when the particle was moved or
 * killed here. */
s32 ef_pf_calc_particle(EfDrawParticle* p, const EfPostFieldTransform* xform, EfPostFieldInfo* info,
                        EffectHandle* eh, const MTX34* emitter_mtx, const MTX34* pm_mtx, const VEC3* pos,
                        VEC3* offset, VEC3* velocity, u8* killed) {
    MTX34 mtx;
    MTX34 rot;
    MTX34 rot_mtx;
    MTX34 inv;
    MTX34 rot_inv;
    VEC3 emitter_pos;
    VEC3 local_pos;
    VEC3 local_next;
    VEC3 local_vel;
    u8 state;
    u8 prev_state;
    f32 speed_sq;
    s32 i;

    MTX34_ctor(&mtx);
    mtx34_identity(&mtx);
    ef_mtx34_scale_columns((f32*)&mtx, (const f32*)&xform->scale, (const f32*)&mtx);
    MTX34_ctor(&rot);
    ef_mtx34_rotate_xyz((f32*)&rot, xform->rotate.x, xform->rotate.y, xform->rotate.z);
    mtx34_concat(&mtx, &rot, &mtx);
    mtx34_trans_apply(&mtx, &xform->translate, &mtx);
    if ((info->flags & 1) != 0) {
        setVec3(&emitter_pos, emitter_mtx->m[0][3], emitter_mtx->m[1][3], emitter_mtx->m[2][3]);
        mtx34_trans_apply(&mtx, &emitter_pos, &mtx);
    }
    mtx34_concat(&mtx, pm_mtx, &mtx);
    ef_mtx34_copy(&rot_mtx, &mtx);
    rot_mtx.m[0][3] = 0.0f;
    rot_mtx.m[1][3] = 0.0f;
    rot_mtx.m[2][3] = 0.0f;
    MTX34_ctor(&inv);
    mtx34_inverse(&inv, &mtx);
    ef_mtx34_copy(&rot_inv, &inv);
    rot_inv.m[0][3] = 0.0f;
    rot_inv.m[1][3] = 0.0f;
    rot_inv.m[2][3] = 0.0f;
    VEC3_ctor(&local_pos);
    mtx34_mult_vec3(&local_pos, &inv, pos);
    state = ef_pf_collision_test(info->shape, &local_pos, info->option);
    prev_state = p->collision_state;
    p->collision_state = state;
    speed_sq = vec3_length_sq((const f32*)velocity);
    switch (info->speed_mode) {
    case 1:
        if (speed_sq > info->speed_limit * info->speed_limit) {
            ef_vec3_normalize_to(velocity, velocity);
            vec3_scale_by((f32*)velocity, (const f32*)velocity, info->speed_limit);
        }
        break;
    case 2:
        if (speed_sq > info->speed_limit * info->speed_limit) {
            *killed = 1;
        }
        break;
    }
    if (*killed != 0) {
        return 1;
    }
    if (p->age < info->start_age) {
        return 1;
    }
    if (info->flags == 0) {
        return 1;
    }
    switch ((u32)info->collision_mode) {
    case 1:
        if (state != 2) {
            return 1;
        }
    case 2:
        if (info->collision_mode == 2 && state != 3) {
            return 1;
        }
        if ((info->flags & 8) != 0) {
            offset->x *= info->scale.x;
            offset->y *= info->scale.y;
            offset->z *= info->scale.z;
            velocity->x *= info->scale.x;
            velocity->y *= info->scale.y;
            velocity->z *= info->scale.z;
            switch (info->speed_mode) {
            case 1:
                if (vec3_length_sq((const f32*)velocity) > info->speed_limit * info->speed_limit) {
                    ef_vec3_normalize_to(velocity, velocity);
                    vec3_scale_by((f32*)velocity, (const f32*)velocity, info->speed_limit);
                }
                break;
            case 2:
                if (vec3_length_sq((const f32*)velocity) > info->speed_limit * info->speed_limit) {
                    *killed = 1;
                }
                break;
            }
        }
        if ((info->flags & 0x40) != 0) {
            *killed = 1;
        }
        if ((info->flags & 0x10) != 0 && eh != NULL) {
            ef_creation_queue_add_type0((CreationQueue*)p->manager->emitter->effect->system->creation_queue,
                                        (const Setting*)info->spawn_setting, (EffectManager*)p, eh, p->life, NULL,
                                        NULL);
        }
        if ((info->flags & 0x20) != 0 && eh != NULL) {
            ef_creation_queue_add_type1((CreationQueue*)p->manager->emitter->effect->system->creation_queue,
                                        (const Setting*)info->spawn_setting, (EffectManager*)p, eh, p->life, NULL,
                                        NULL);
        }
        return 1;
    }
    VEC3_ctor(&local_next);
    local_next.x = pos->x + p->step * (offset->x + velocity->x);
    local_next.y = pos->y + p->step * (offset->y + velocity->y);
    local_next.z = pos->z + p->step * (offset->z + velocity->z);
    mtx34_mult_vec3(&local_next, &inv, &local_next);
    VEC3_ctor(&local_vel);
    mtx34_mult_vec3(&local_vel, &rot_inv, velocity);
    for (i = 0; i < 5; i++) {
        VEC3 hit;
        VEC3 hit_world;
        VEC3 out_pos;
        VEC3 out_vel;
        s32 collided;

        VEC3_ctor(&hit);
        VEC3_ctor(&hit_world);
        VEC3_ctor(&out_pos);
        VEC3_ctor(&out_vel);
        if (ef_vec3_dist_sq(&local_pos, &local_next) < 1.4210855e-14f) {
            collided = 0;
        } else {
            VEC3 step_vel = local_vel;

            collided = ef_pf_collide(&mtx, &rot_mtx, info, eh, p, &local_pos, &local_next, &step_vel, killed, &hit,
                                     &hit_world, &out_pos, &out_vel);
        }
        if (collided != 0) {
            copyVec3((VEC3*)&p->world_pos, &out_pos);
            copyVec3(&p->velocity, &out_vel);
            p->collision_state |= 0x80;
            if (*killed != 0) {
                return 0;
            }
            state = 1;
            copyVec3(&local_pos, &hit);
            mtx34_mult_vec3(&local_next, &inv, &out_pos);
            mtx34_mult_vec3(&local_vel, &rot_inv, &out_vel);
        } else {
            if (i == 0) {
                if (p->age > info->start_age &&
                    ((prev_state == 2 && (state == 3 || state == 1)) ||
                     (prev_state == 3 && (u8)(state + 255) <= 1))) {
                    if ((info->flags & 0x40) != 0) {
                        *killed = 1;
                    }
                    if ((info->flags & 0x10) != 0 && eh != NULL) {
                        ef_creation_queue_add_type0(
                            (CreationQueue*)p->manager->emitter->effect->system->creation_queue,
                            (const Setting*)info->spawn_setting, (EffectManager*)p, eh, p->life, NULL, NULL);
                    }
                    if ((info->flags & 0x20) != 0 && eh != NULL) {
                        ef_creation_queue_add_type1(
                            (CreationQueue*)p->manager->emitter->effect->system->creation_queue,
                            (const Setting*)info->spawn_setting, (EffectManager*)p, eh, p->life, NULL, NULL);
                    }
                }
                return 1;
            }
            switch (info->speed_mode) {
            case 1:
                if (vec3_length_sq((const f32*)&p->velocity) > info->speed_limit * info->speed_limit) {
                    ef_vec3_normalize_to(&p->velocity, &p->velocity);
                    vec3_scale_by((f32*)&p->velocity, (const f32*)&p->velocity, info->speed_limit);
                }
                break;
            case 2:
                if (vec3_length_sq((const f32*)&p->velocity) > info->speed_limit * info->speed_limit) {
                    *killed = 1;
                }
                break;
            }
            return 0;
        }
    }
    *killed = 1;
    return 0;
}


/* --------------------------------------------------------------------------------------------- *
 * The resource-parameter accessor family (res_drawparam_ac.h / res_emitterparam_ac.h, 0x800B23A4..0x800B2878).
 * --------------------------------------------------------------------------------------------- */

/* The `RES_ACCESS` record: its first word is the checked `mData` pointer, and `ef_res_drawparam_data` asserts
 * and returns it (the out-of-line `IsValidPointer` instantiation - `ef.h`). */
typedef struct EfResDrawParam {
    u8 pad_0x00[0x94];   /* +0x00 */
    u8 field_0x94[0x4C]; /* +0x94  the parameter payload the accessors return */
} EfResDrawParam; /* size: 0xE0 */

/* The state word `ef_res_drawparam_data` returns (a `Res*` block whose flags sit at +0). */
typedef struct EfResState {
    u16 flags_0x00; /* +0x00 */
} EfResState; /* size: 0x02 */

/* A record whose first field holds a pointer (the two `sts`-style setters store one). */
typedef struct EfResSlot {
    void* field_0x00; /* +0x00 */
} EfResSlot; /* size: 0x04 */

/* The `res_*_ac.h` accessor record: its first word is the resource block it reads. */
typedef struct EfResAccessor {
    /* +0x00 */ void* data;
} EfResAccessor; /* size: 0x4 */

extern EfResDrawParam* fn_800A5484(void* arg);
extern EfResDrawParam* ef_res_emitter_desc(EfResDrawParam* arg);

/* The resolved parameter block the +0x4C/+0x54 accessors read. */
typedef struct EfResParams {
    u8 pad_0x00[0x4C]; /* +0x00 */
    f32 field_0x4C;    /* +0x4C */
    u8 pad_0x50[0x04]; /* +0x50 */
    VEC3 field_0x54;   /* +0x54 */
    u8 pad_0x60[0x34]; /* +0x60 */
} EfResParams; /* size: 0x94 */

/* The three `_ac.h` accessors are `inline`: the callers below keep them out of line, MWCC places each weak copy
 * after the first function that references it and emits its strings after the unit's own. */
EfResState* ef_res_drawparam_data(const EfResAccessor* self);
EfResParams* ef_res_emitterparam_cdata(const EfResAccessor* self);
EfResParams* ef_res_emitterparam_data(const EfResAccessor* self);
void ef_res_slot_set_emitter_desc(EfResSlot* self, void* value);
void ef_res_slot_set_params(EfResSlot* self, void* value);

#pragma dont_inline on

/* 0x800B2598 (0x118): the emitter-parameter accessor's checked block (res_emitterparam_ac.h, the const copy). */
inline EfResParams* ef_res_emitterparam_cdata(const EfResAccessor* self) {
    EF_VALID_PTR_ASSERT("res_emitterparam_ac.h", 97, "NW4R:Pointer Error\nmData(=%p) is not valid pointer.", self->data);
    return (EfResParams*)self->data;
}

/* 0x800B26F8 (0x118): the emitter-parameter accessor's checked block (res_emitterparam_ac.h). */
inline EfResParams* ef_res_emitterparam_data(const EfResAccessor* self) {
    EF_VALID_PTR_ASSERT("res_emitterparam_ac.h", 90, "NW4R:Pointer Error\nmData(=%p) is not valid pointer.", self->data);
    return (EfResParams*)self->data;
}

/* 0x800B23A4 (0x118): the draw-parameter accessor's checked block (res_drawparam_ac.h). */
inline EfResState* ef_res_drawparam_data(const EfResAccessor* self) {
    EF_VALID_PTR_ASSERT("res_drawparam_ac.h", 90, "NW4R:Pointer Error\nmData(=%p) is not valid pointer.", self->data);
    return (EfResState*)self->data;
}

/* Returns the draw-parameter block.  Nothing calls it, so the linker strips it, as it stripped retail's reader
 * of the accessor; a frameless tail call, it adds no unwind record. */
EfResState* ef_res_drawparam_ref(const EfResAccessor* self) {
    return ef_res_drawparam_data(self);
}

/* Sets or clears the 0x400 flag on the accessor's state word. */
void ef_res_drawparam_set_flag_0400(void* self, int enable) {
    if (enable != 0) {
        ef_res_drawparam_data((EfResAccessor*)self)->flags_0x00 |= 0x400;
    } else {
        ef_res_drawparam_data((EfResAccessor*)self)->flags_0x00 &= ~0x400;
    }
}

/* Resolves the parameter through the two accessors and stores it. */
void ef_res_slot_bind_emitter_desc(EfResSlot* self, void* arg) {
    ef_res_slot_set_emitter_desc(self, ef_res_emitter_desc(fn_800A5484(arg)));
}

/* Stores a pointer into the record's first field. */
void ef_res_slot_set_emitter_desc(EfResSlot* self, void* value) {
    self->field_0x00 = value;
}

/* Resolves the parameter and stores the second block's +0x94 field. */
void ef_res_slot_bind_params(EfResSlot* self, void* arg) {
    ef_res_slot_set_params(self, &ef_res_emitter_desc(fn_800A5484(arg))->field_0x94);
}

/* Stores a pointer into the record's first field. */
void ef_res_slot_set_params(EfResSlot* self, void* value) {
    self->field_0x00 = value;
}

/* Returns the const emitter-parameter block; stripped by the linker like `ef_res_drawparam_ref`. */
EfResParams* ef_res_emitterparam_cref(const EfResAccessor* self) {
    return ef_res_emitterparam_cdata(self);
}

/* The resolved parameter's scale. */
f32 ef_res_emitterparam_scale(void* self) {
    return ef_res_emitterparam_cdata((EfResAccessor*)self)->field_0x4C;
}

/* The resolved parameter's second block. */
void* ef_res_emitterparam_vec(void* self) {
    return &ef_res_emitterparam_cdata((EfResAccessor*)self)->field_0x54;
}

/* Returns the emitter-parameter block; stripped by the linker like `ef_res_drawparam_ref`. */
EfResParams* ef_res_emitterparam_ref(const EfResAccessor* self) {
    return ef_res_emitterparam_data(self);
}

/* Sets the resolved parameter's scale. */
void ef_res_emitterparam_set_scale(void* self, f32 value) {
    ef_res_emitterparam_data((EfResAccessor*)self)->field_0x4C = value;
}

/* Copies a source block over the resolved parameter's second block. */
void ef_res_emitterparam_set_vec(void* self, const nw4r::math::VEC3* src) {
    copyVec3(&ef_res_emitterparam_data((EfResAccessor*)self)->field_0x54, src);
}

#pragma dont_inline reset

#ifdef __cplusplus
}
#endif
