/*
 * ef/ef_cube.cpp - the cube emitter form: `EmitterFormCube::Emission` builds the cube's even grid or random shell
 *   points and emits each through `ef_cube_emit`, which normalises the two direction vectors, derives the spawn
 *   velocity and calls the particle manager's `CreateParticle`.
 * RANGE. .text 0x800C9DD0-0x800CB948 (2 functions); extab 0x8000A55C-0x8000A56C, extabindex 0x80023BB0-0x80023BC8,
 *   .data 0x80594C68-0x80594D20 (the `__FILE__` string "ef_cube.cpp", the three assert messages, the class's
 *   table at 0x80594D14), .sdata2 0x80796240-0x80796270.
 * FLAGS. `cflags_main` plus `-pool off`; file-wide `#pragma peephole off` (retail keeps the `li r0,136; psq_lx` FPR
 *   epilogue and the `clrlwi r4,r30,16` at the call site) and `#pragma fp_contract off` (retail keeps every
 *   `a*b+c` as two instructions).
 * NAMES. The class and method are nw4r's.  `fabsf` (0x80463F04, `fabs` + `frsp`) and `tanf` (0x80463F98, the dump's
 *   name) are MSL's.
 *   GUESS: `ef_cube_emit` (0x800C9DD0): emits one cube point (the shared tail of the grid and random paths).
 *   GUESS (from their text, value and use): `ef_cube_file_name`, `ef_cube_err_em`, `ef_cube_err_pm`,
 *   `ef_cube_err_params`, `ef_cube_f32_flt_min`, `ef_cube_f32_two`, `ef_cube_f32_one`, `ef_cube_f32_zero`,
 *   `ef_cube_f32_percent`, `ef_cube_f64_int_bias`, `ef_cube_f32_min_size`, `ef_cube_f32_pi`, `ef_cube_f32_pi_epsilon`,
 *   `ef_cube_f32_minus_one`.
 * RESIDUALS.
 *  - `Emission`: the spiral's `dir = 0` is scheduled before the `n % 2` test where retail sets it after (tried: a
 *    ternary, if/else); the side faces' row counter takes r18 where retail's takes r23 (tried: reusing the spiral's
 *    `i`, the counter declarations' orders by hill climb).
 *  - flipcheck: `.text` only (`.data`, `.sdata2`, extab and extabindex are byte-identical).
 * SHAPES. The function is `nw4r::ef::EmitterFormCube::Emission` (declared in `ef/ef_emform.h`; this unit defines the key
 *   function, so the class's table is emitted here, after the strings).  The particle manager's spawn is its
 *   virtual `CreateParticle`, taking the position and velocity by value (copied velocity first, momentum
 *   evaluated before the life, dispatch through r3); the float argument precedes the space matrix.
 * SHAPES. The strings are global definitions in retail's order; `-pool off` (configure.py) gives each its own
 *   `lis`/`addi`.
 * SHAPES. The float constants are literals, pooled in creation order with the `(f32)int` bias after
 *   `ef_cube_emit`'s; MWCC hoists them out of the loops as retail does.  The three spiral/ring blocks
 *   declare their own counters in the order that reproduces retail's register assignment.
 */

#include "types.h"
#include "mh3_pad/vec3.h" /* the owner header (rule 2) */
#include "ef/ef_emitter.h" /* ef_random_float / ef_random_u16 (rule 2) */
#include "ef/ef_torus.h" /* ef_fabsf (rule 2) */
#include "MSL_C/alloc.h" /* fabsf / tanf (rule 2) */
#include "g3d/math_reciprocal.h" /* math_reciprocal (rule 2) */
#include "sqrt_f32.h" /* sqrt_f32 (rule 2) */
#include "ef/ef_emform.h" /* nw4r::ef::EmitterFormCube, EfWork, ParticleManager (rule 1) */

/* The unit's `.data` strings, in retail order: the file name and one message per checked pointer (the
 * class's table follows them). */
char ef_cube_file_name[] = "ef_cube.cpp";
char ef_cube_err_em[] = "NW4R:Pointer Error\nem(=%p) is not valid pointer.";
char ef_cube_err_pm[] = "NW4R:Pointer Error\npm(=%p) is not valid pointer.";
char ef_cube_err_params[] = "NW4R:Pointer Error\nparams(=%p) is not valid pointer.";

/* `nw4r::db::Panic`, declared in its namespace so the front-end emits the map's mangling
 * (`Panic__Q24nw4r2dbFPCciPCce`). */
namespace nw4r { namespace db { void Panic(const char* file, int line, const char* fmt, ...); } }

/* nw4r::math and effect-library helpers; retail's relocations carry their plain map names, so they
 * have C linkage. */
extern "C" {
extern void ef_vec3_normalize_to(VEC3* dst, VEC3* src);
extern f32 vec3_length_sq(const VEC3* v);
}

/* The cube form's parameter block: the half sizes and the hollow ratio in percent. */
typedef struct EfCubeParams {
    f32 sizeX;  /* +0x0 */
    f32 sizeY;  /* +0x4 */
    f32 sizeZ;  /* +0x8 */
    f32 hollow; /* +0xC */
} EfCubeParams; /* size: 0x10 */

#pragma peephole off
#pragma fp_contract off

/* The engine's pointer validity test: the address has to fall in one of the memory regions the game
 * allocates from. Retail materialises it as one `||` chain (`li`/`li ...,0` per region), so it must
 * stay a single expression assigned to a variable - an inline `if (!...)` compiles to branches. */
#define IS_VALID_PTR(p)                         \
    ((((u32)(p) & 0xFF000000) == 0x80000000) || \
     (((u32)(p) & 0xFF800000) == 0x81000000) || \
     (((u32)(p) & 0xF8000000) == 0x90000000) || \
     (((u32)(p) & 0xFF000000) == 0xC0000000) || \
     (((u32)(p) & 0xFF800000) == 0xC1000000) || \
     (((u32)(p) & 0xF8000000) == 0xD0000000) || \
     (((u32)(p) & 0xFFFFC000) == 0xE0000000))

/* Normalises the effect's two direction vectors, derives a spawn position from them, and hands the
 * result to the effect parameter object's spawn method. */
void ef_cube_emit(nw4r::ef::EmitterFormCube* form, VEC3* b, VEC3* c, EfWork* em, ParticleManager* pm, u16 d, f32 f,
                  const MTX34* e)
{
    VEC3 v1;
    VEC3 v2;
    VEC3 v3;
    f32 s;
    u16 r;
    int ok;

    ok = IS_VALID_PTR(em);
    if (!ok) {
        nw4r::db::Panic(ef_cube_file_name, 43, ef_cube_err_em, em);
    }
    ok = IS_VALID_PTR(pm);
    if (!ok) {
        nw4r::db::Panic(ef_cube_file_name, 44, ef_cube_err_pm, pm);
    }

    ef_vec3_normalize_to(c, c);
    assignVec3((Vec*)&v1, (Vec*)b);
    if (vec3_length_sq(&v1) <= 1.17549435e-38f) {
        v1.x = ef_random_float(&em->progress) * 2.0f - 1.0f;
        v1.y = ef_random_float(&em->progress) * 2.0f - 1.0f;
        v1.z = ef_random_float(&em->progress) * 2.0f - 1.0f;
    }
    ef_vec3_normalize_to(&v1, &v1);
    assignVec3((Vec*)&v2, (Vec*)b);
    v2.y = 0.0f;
    if (vec3_length_sq(&v2) <= 1.17549435e-38f) {
        v2.x = ef_random_float(&em->progress) * 2.0f - 1.0f;
        v2.z = ef_random_float(&em->progress) * 2.0f - 1.0f;
    }
    ef_vec3_normalize_to(&v2, &v2);
    /* This unit's `VEC3` (the map's `P4Vec3`) and the helper's `nw4r::math::VEC3` share one 0xC-byte
     * layout. */
    VEC3_ctor((nw4r::math::VEC3*)&v3);
    form->CalcVelocity((Vec*)&v3, em, (Vec*)b, (Vec*)c, (Vec*)&v1, (Vec*)&v2);
    pm->CreateParticle(form->CalcLife(d, f, em), *b, v3, e,
                       1.0f + 0.01f * (f32)em->scale_rate * ef_random_float(&em->progress),
                       &em->spawn_data, em->spawn_extra, em->spawn_flag);
}

/* The larger of two magnitudes through the shapes' `fabsf` thunk; retail evaluates each side twice. */
#define EF_CUBE_MAX_ABS(a, b) (ef_fabsf(a) > ef_fabsf(b) ? ef_fabsf(a) : ef_fabsf(b))

/* A uniform random value in [-1, 1). */
#define EF_CUBE_RAND_SIGNED(em) (2.0f * ef_random_float(&(em)->progress) - 1.0f)

/* Emits `n` particles from the cube: on an even grid over the six faces (flag 0x20000), or at random inside the
 * shell the hollow ratio leaves; each direction leans out of its face by the emitter's diffusion angle. */
void nw4r::ef::EmitterFormCube::Emission(EfWork* em, ParticleManager* pm, int n, u32 flags, f32* params, u16 d,
                                         f32 f, const MTX34* e)
{
    int ok;
    const EfCubeParams* cube;
    f32 sizeX;
    f32 sizeY;
    f32 sizeZ;
    f32 hollow;
    f32 shell;
    f32 angle;

    ok = IS_VALID_PTR(em);
    if (!ok) {
        nw4r::db::Panic(ef_cube_file_name, 94, ef_cube_err_em, em);
    }
    ok = IS_VALID_PTR(pm);
    if (!ok) {
        nw4r::db::Panic(ef_cube_file_name, 95, ef_cube_err_pm, pm);
    }
    ok = IS_VALID_PTR(params);
    if (!ok) {
        nw4r::db::Panic(ef_cube_file_name, 96, ef_cube_err_params, params);
    }
    if ((int)n < 1) {
        return;
    }

    cube = (const EfCubeParams*)params;
    sizeX = cube->sizeX;
    if (flags & 0x02000000) {
        sizeY = sizeX;
        sizeZ = sizeX;
    } else {
        sizeY = cube->sizeY;
        sizeZ = cube->sizeZ;
    }
    if (sizeX < 1e-5f) {
        sizeX = 1e-5f;
    }
    if (sizeY < 1e-5f) {
        sizeY = 1e-5f;
    }
    if (sizeZ < 1e-5f) {
        sizeZ = 1e-5f;
    }
    if (0.0f == cube->hollow) {
        hollow = 1e-5f;
    } else {
        hollow = 0.01f * cube->hollow;
    }
    shell = 1.0f - hollow;
    angle = em->spread;
    if (ef_fabsf(angle - 3.1415927f) < 3.74507039e-07f) {
        angle -= 3.74507039e-07f;
    }

    if (flags & 0x20000) {
        f32 step = 2.0f / (f32)(int)(n + 1);
        {
            int i;
            int col;
            int row;
            int dir;
            int run;
            int left;

            col = (int)n / 2 + 1;
            row = ((int)n - 1) / 2 + 1;
            dir = 0;
            if ((int)n % 2 == 0) {
                dir = 2;
            }
            run = 1;
            left = 1;

            /* The bottom face, walked as a spiral out from its centre. */
            for (i = 0; i < (int)(n * n); i++) {
                f32 u;
                f32 v;
                f32 px;
                f32 pz;
                f32 spread;
                f32 lean;
                VEC3 pos;
                VEC3 vel;

                if (i != 0) {
                    switch (dir) {
                    case 0:
                        row--;
                        break;
                    case 1:
                        col++;
                        break;
                    case 2:
                        row++;
                        break;
                    case 3:
                        col--;
                        break;
                    }
                    left--;
                    if (left <= 0) {
                        dir++;
                        if (dir == 4) {
                            dir = 0;
                        }
                        if (dir % 2 == 0) {
                            run++;
                        }
                        left = run;
                    }
                }
                u = (f32)col * step - 1.0f;
                v = (f32)row * step - 1.0f;
                px = u * sizeX;
                pz = v * sizeZ;
                spread = 1.0f - EF_CUBE_MAX_ABS(u, v);
                lean = EF_CUBE_MAX_ABS(u, v);
                VEC3_ctor((nw4r::math::VEC3*)&pos);
                VEC3_ctor((nw4r::math::VEC3*)&vel);
                pos.x = px;
                pos.y = -sizeY;
                pos.z = pz;
                if (0.0f != shell) {
                    f32 depth;
                    if (flags & 0x01000000) {
                        f32 r = ef_random_float(&em->progress);
                        r *= r;
                        depth = 1.0f - spread * (r * shell);
                    } else {
                        depth = 1.0f - spread * (shell * ef_random_float(&em->progress));
                    }
                    pos.y *= depth;
                }
                if (0.0f == angle ||
                    (fabsf(pos.x) < 1e-5f && fabsf(pos.z) < 1e-5f)) {
                    vel.x = 0.0f;
                    vel.y = -1.0f;
                    vel.z = 0.0f;
                } else {
                    f32 inv = math_reciprocal(tanf(lean * angle));
                    vel.x = pos.x;
                    vel.y = inv * -sqrt_f32(pos.x * pos.x + pos.z * pos.z);
                    vel.z = pos.z;
                }
                ef_cube_emit(this, &pos, &vel, em, pm, d, f, e);
            }
        }

        {
            int j;
            int k;

            /* The four side faces, one ring per row from the bottom up. */
            for (j = 1; j <= (int)n; j++) {
                f32 v = (f32)j * step - 1.0f;
                f32 y = v * sizeY;

                for (k = 1; k <= (int)n; k++) {
                    f32 u = (f32)k * step - 1.0f;
                    f32 pu = u * sizeX;
                    f32 spread = 1.0f - EF_CUBE_MAX_ABS(u, v);
                    f32 lean = EF_CUBE_MAX_ABS(u, v);
                    VEC3 pos;
                    VEC3 vel;

                    VEC3_ctor((nw4r::math::VEC3*)&pos);
                    VEC3_ctor((nw4r::math::VEC3*)&vel);
                    pos.x = pu;
                    pos.y = y;
                    pos.z = -sizeZ;
                    if (0.0f != shell) {
                        f32 depth;
                        if (flags & 0x01000000) {
                            f32 r = ef_random_float(&em->progress);
                            r *= r;
                            depth = 1.0f - spread * (r * shell);
                        } else {
                            depth = 1.0f - spread * (shell * ef_random_float(&em->progress));
                        }
                        pos.z *= depth;
                    }
                    if (0.0f == angle ||
                        (fabsf(pos.x) < 1e-5f && fabsf(pos.y) < 1e-5f)) {
                        vel.x = 0.0f;
                        vel.y = 0.0f;
                        vel.z = -1.0f;
                    } else {
                        f32 inv = math_reciprocal(tanf(lean * angle));
                        vel.x = pos.x;
                        vel.y = pos.y;
                        vel.z = inv * -sqrt_f32(pos.x * pos.x + pos.y * pos.y);
                    }
                    ef_cube_emit(this, &pos, &vel, em, pm, d, f, e);
                }
                for (k = 1; k <= (int)n; k++) {
                    f32 u = (f32)k * step - 1.0f;
                    f32 pu = u * sizeZ;
                    f32 spread = 1.0f - EF_CUBE_MAX_ABS(u, v);
                    f32 lean = EF_CUBE_MAX_ABS(u, v);
                    VEC3 pos;
                    VEC3 vel;

                    VEC3_ctor((nw4r::math::VEC3*)&pos);
                    VEC3_ctor((nw4r::math::VEC3*)&vel);
                    pos.x = sizeX;
                    pos.y = y;
                    pos.z = pu;
                    if (0.0f != shell) {
                        f32 depth;
                        if (flags & 0x01000000) {
                            f32 r = ef_random_float(&em->progress);
                            r *= r;
                            depth = 1.0f - spread * (r * shell);
                        } else {
                            depth = 1.0f - spread * (shell * ef_random_float(&em->progress));
                        }
                        pos.x *= depth;
                    }
                    if (0.0f == angle ||
                        (fabsf(pos.y) < 1e-5f && fabsf(pos.z) < 1e-5f)) {
                        vel.x = 1.0f;
                        vel.y = 0.0f;
                        vel.z = 0.0f;
                    } else {
                        f32 inv = math_reciprocal(tanf(lean * angle));
                        vel.x = inv * sqrt_f32(pos.y * pos.y + pos.z * pos.z);
                        vel.y = pos.y;
                        vel.z = pos.z;
                    }
                    ef_cube_emit(this, &pos, &vel, em, pm, d, f, e);
                }
                for (k = n; k >= 1; k--) {
                    f32 u = (f32)k * step - 1.0f;
                    f32 pu = u * sizeX;
                    f32 spread = 1.0f - EF_CUBE_MAX_ABS(u, v);
                    f32 lean = EF_CUBE_MAX_ABS(u, v);
                    VEC3 pos;
                    VEC3 vel;

                    VEC3_ctor((nw4r::math::VEC3*)&pos);
                    VEC3_ctor((nw4r::math::VEC3*)&vel);
                    pos.x = pu;
                    pos.y = y;
                    pos.z = sizeZ;
                    if (0.0f != shell) {
                        f32 depth;
                        if (flags & 0x01000000) {
                            f32 r = ef_random_float(&em->progress);
                            r *= r;
                            depth = 1.0f - spread * (r * shell);
                        } else {
                            depth = 1.0f - spread * (shell * ef_random_float(&em->progress));
                        }
                        pos.z *= depth;
                    }
                    if (0.0f == angle ||
                        (fabsf(pos.x) < 1e-5f && fabsf(pos.y) < 1e-5f)) {
                        vel.x = 0.0f;
                        vel.y = 0.0f;
                        vel.z = 1.0f;
                    } else {
                        f32 inv = math_reciprocal(tanf(lean * angle));
                        vel.x = pos.x;
                        vel.y = pos.y;
                        vel.z = inv * sqrt_f32(pos.x * pos.x + pos.y * pos.y);
                    }
                    ef_cube_emit(this, &pos, &vel, em, pm, d, f, e);
                }
                for (k = n; k >= 1; k--) {
                    f32 u = (f32)k * step - 1.0f;
                    f32 pu = u * sizeZ;
                    f32 spread = 1.0f - EF_CUBE_MAX_ABS(u, v);
                    f32 lean = EF_CUBE_MAX_ABS(u, v);
                    VEC3 pos;
                    VEC3 vel;

                    VEC3_ctor((nw4r::math::VEC3*)&pos);
                    VEC3_ctor((nw4r::math::VEC3*)&vel);
                    pos.x = -sizeX;
                    pos.y = y;
                    pos.z = pu;
                    if (0.0f != shell) {
                        f32 depth;
                        if (flags & 0x01000000) {
                            f32 r = ef_random_float(&em->progress);
                            r *= r;
                            depth = 1.0f - spread * (r * shell);
                        } else {
                            depth = 1.0f - spread * (shell * ef_random_float(&em->progress));
                        }
                        pos.x *= depth;
                    }
                    if (0.0f == angle ||
                        (fabsf(pos.y) < 1e-5f && fabsf(pos.z) < 1e-5f)) {
                        vel.x = -1.0f;
                        vel.y = 0.0f;
                        vel.z = 0.0f;
                    } else {
                        f32 inv = math_reciprocal(tanf(lean * angle));
                        vel.x = inv * -sqrt_f32(pos.y * pos.y + pos.z * pos.z);
                        vel.y = pos.y;
                        vel.z = pos.z;
                    }
                    ef_cube_emit(this, &pos, &vel, em, pm, d, f, e);
                }
            }
        }

        {
            int i;
            int col;
            int row;
            int dir;
            int run;
            int left;

            /* The top face, walked as a spiral in from its corner. */
            col = 1;
            row = 1;
            dir = 2;
            run = n - 1;
            left = run;
            for (i = 0; i < (int)(n * n); i++) {
                f32 u;
                f32 v;
                f32 px;
                f32 pz;
                f32 spread;
                f32 lean;
                VEC3 pos;
                VEC3 vel;

                if (i != 0) {
                    switch (dir) {
                    case 0:
                        col--;
                        break;
                    case 1:
                        row++;
                        break;
                    case 2:
                        col++;
                        break;
                    case 3:
                        row--;
                        break;
                    }
                    left--;
                    if (left <= 0) {
                        dir = dir == 0 ? 3 : dir - 1;
                        if (dir % 2 == 1 && (dir != 1 || run != (int)(n - 1))) {
                            run--;
                        }
                        left = run;
                    }
                }
                u = (f32)col * step - 1.0f;
                v = (f32)row * step - 1.0f;
                px = u * sizeX;
                pz = v * sizeZ;
                spread = 1.0f - EF_CUBE_MAX_ABS(u, v);
                lean = EF_CUBE_MAX_ABS(u, v);
                VEC3_ctor((nw4r::math::VEC3*)&pos);
                VEC3_ctor((nw4r::math::VEC3*)&vel);
                pos.x = px;
                pos.y = sizeY;
                pos.z = pz;
                if (0.0f != shell) {
                    f32 depth;
                    if (flags & 0x01000000) {
                        f32 r = ef_random_float(&em->progress);
                        r *= r;
                        depth = 1.0f - spread * (r * shell);
                    } else {
                        depth = 1.0f - spread * (shell * ef_random_float(&em->progress));
                    }
                    pos.y *= depth;
                }
                if (0.0f == angle ||
                    (fabsf(pos.x) < 1e-5f && fabsf(pos.z) < 1e-5f)) {
                    vel.x = 0.0f;
                    vel.y = 1.0f;
                    vel.z = 0.0f;
                } else {
                    f32 inv = math_reciprocal(tanf(lean * angle));
                    vel.x = pos.x;
                    vel.y = inv * sqrt_f32(pos.x * pos.x + pos.z * pos.z);
                    vel.z = pos.z;
                }
                ef_cube_emit(this, &pos, &vel, em, pm, d, f, e);
            }
        }
    } else {
        int i;

        for (i = 0; i < (int)n; i++) {
            f32 x;
            f32 y;
            f32 z;
            VEC3 pos;
            VEC3 vel;

            if (0.0f == hollow) {
                /* A solid cube: anywhere inside. */
                x = EF_CUBE_RAND_SIGNED(em);
                y = EF_CUBE_RAND_SIGNED(em);
                z = EF_CUBE_RAND_SIGNED(em);
            } else if (1.0f == hollow) {
                /* A hollow cube: on one of the six faces. */
                switch (ef_random_u16(&em->progress) % 6) {
                case 0:
                    x = EF_CUBE_RAND_SIGNED(em);
                    y = EF_CUBE_RAND_SIGNED(em);
                    z = 1.0f;
                    break;
                case 1:
                    x = EF_CUBE_RAND_SIGNED(em);
                    y = EF_CUBE_RAND_SIGNED(em);
                    z = -1.0f;
                    break;
                case 2:
                    x = EF_CUBE_RAND_SIGNED(em);
                    y = 1.0f;
                    z = EF_CUBE_RAND_SIGNED(em);
                    break;
                case 3:
                    x = EF_CUBE_RAND_SIGNED(em);
                    y = -1.0f;
                    z = EF_CUBE_RAND_SIGNED(em);
                    break;
                case 4:
                    x = 1.0f;
                    y = EF_CUBE_RAND_SIGNED(em);
                    z = EF_CUBE_RAND_SIGNED(em);
                    break;
                case 5:
                    x = -1.0f;
                    y = EF_CUBE_RAND_SIGNED(em);
                    z = EF_CUBE_RAND_SIGNED(em);
                    break;
                }
            } else {
                /* A shell: pick one of the three slab pairs by its volume, then a point inside it. */
                f32 slabY = sizeZ * (shell * (sizeX * sizeY));
                f32 slabZ = shell * (sizeZ * (hollow * (sizeX * sizeY)));
                f32 slabX = hollow * (sizeZ * (hollow * (sizeY * (sizeX * shell))));
                f32 pick = (slabY + slabZ + slabX) * ef_random_float(&em->progress);

                if (pick < slabY) {
                    x = EF_CUBE_RAND_SIGNED(em);
                    y = shell * EF_CUBE_RAND_SIGNED(em);
                    if (y >= 0.0f) {
                        y = 1.0f - y;
                    } else {
                        y = -1.0f - y;
                    }
                    z = EF_CUBE_RAND_SIGNED(em);
                } else if (pick < slabY + slabZ) {
                    x = EF_CUBE_RAND_SIGNED(em);
                    y = hollow * EF_CUBE_RAND_SIGNED(em);
                    z = shell * EF_CUBE_RAND_SIGNED(em);
                    if (z >= 0.0f) {
                        z = 1.0f - z;
                    } else {
                        z = -1.0f - z;
                    }
                } else {
                    x = shell * EF_CUBE_RAND_SIGNED(em);
                    if (x >= 0.0f) {
                        x = 1.0f - x;
                    } else {
                        x = -1.0f - x;
                    }
                    y = hollow * EF_CUBE_RAND_SIGNED(em);
                    z = hollow * EF_CUBE_RAND_SIGNED(em);
                }
            }

            VEC3_ctor((nw4r::math::VEC3*)&pos);
            setVec3((nw4r::math::VEC3*)&vel, 0.0f, 0.0f, 0.0f);
            if (y >= 0.0f && y >= ef_fabsf(x) && y >= ef_fabsf(z)) {
                vel.y = 1.0f;
            } else if (y < 0.0f && -y >= ef_fabsf(x) && -y >= ef_fabsf(z)) {
                vel.y = -1.0f;
            } else if (x >= 0.0f && x >= ef_fabsf(y) && x >= ef_fabsf(z)) {
                vel.x = 1.0f;
            } else if (x < 0.0f && -x >= ef_fabsf(y) && -x >= ef_fabsf(z)) {
                vel.x = -1.0f;
            } else if (z >= 0.0f) {
                vel.z = 1.0f;
            } else {
                vel.z = -1.0f;
            }
            pos.x = x * sizeX;
            pos.y = y * sizeY;
            pos.z = z * sizeZ;
            if (0.0f != angle) {
                if (0.0f != vel.x) {
                    f32 lean = EF_CUBE_MAX_ABS(pos.y, pos.z);
                    f32 inv = math_reciprocal(tanf(lean * angle));
                    vel.x *= inv * sqrt_f32(pos.y * pos.y + pos.z * pos.z);
                    vel.y = pos.y;
                    vel.z = pos.z;
                } else if (0.0f != vel.y) {
                    f32 lean = EF_CUBE_MAX_ABS(pos.x, pos.z);
                    f32 inv = math_reciprocal(tanf(lean * angle));
                    vel.x = pos.x;
                    vel.y *= inv * sqrt_f32(pos.x * pos.x + pos.z * pos.z);
                    vel.z = pos.z;
                } else {
                    f32 lean = EF_CUBE_MAX_ABS(pos.x, pos.y);
                    f32 inv = math_reciprocal(tanf(lean * angle));
                    vel.x = pos.x;
                    vel.y = pos.y;
                    vel.z *= inv * sqrt_f32(pos.x * pos.x + pos.y * pos.y);
                }
            }
            ef_cube_emit(this, &pos, &vel, em, pm, d, f, e);
        }
    }
}
