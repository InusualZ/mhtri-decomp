/*
 * ef/ef_cube.cpp - the cube emitter form: `fn_800C9DD0` normalises the two direction vectors, derives the spawn
 *   position and calls the particle manager's slot +0x14; `fn_800CA200` builds the cube's vertex grid and emits it
 *   through `fn_800C9DD0`.
 * RANGE. .text 0x800C9DD0-0x800CB948 (2 functions); extab 0x8000A55C-0x8000A56C, extabindex 0x80023BB0-0x80023BC8,
 *   .data 0x80594C68-0x80594D20 (the `__FILE__` string "ef_cube.cpp" first), .sdata2 0x80796240-0x80796270.
 *   Right edge: the source file changes to "ef_cylinder.cpp" and the `.sdata2` run jumps from `ef_cube_f32_minus_one` to
 *   `lbl_80796270`.
 * FLAGS. `cflags_main`; file-wide `#pragma peephole off` (retail keeps the `li r0,136; psq_lx` FPR epilogue and
 *   the `clrlwi r4,r30,16` at the call site; `-opt nopeephole` gives the same object) and `#pragma fp_contract off`
 *   (retail keeps every `a*b+c` as two instructions; a temporary per site does the same).
 * NAMES. The map has only `fn_` stems for the two functions; the `Em` fields keep `field_0xNN` names where nothing
 *   names them.  `fabsf` (0x80463F04, `fabs` + `frsp`) and `tanf` (0x80463F98, the dump's name) are MSL's.
 *   GUESS (from their text, value and use): `ef_cube_file_name`, `ef_cube_err_em`, `ef_cube_err_pm`,
 *   `ef_cube_err_params`, `ef_cube_f32_flt_min`, `ef_cube_f32_two`, `ef_cube_f32_one`, `ef_cube_f32_zero`,
 *   `ef_cube_f32_percent`, `ef_cube_f64_int_bias`, `ef_cube_f32_min_size`, `ef_cube_f32_pi`, `ef_cube_f32_pi_epsilon`,
 *   `ef_cube_f32_minus_one`; the fields `Em::diffusionAngle` (+0x78) and `Em::random` (+0xEC).
 * RESIDUALS.
 *  - `fn_800CA200__FUiP2EmP2PmUiUiPvUsUif`: `e` is moved into r31 before `d` where retail moves it after; the
 *    spiral's `dir = 0` is scheduled before the `n % 2` test where retail sets it after; the side faces' row counter
 *    takes r18 where retail's takes r23; and the `ef_random_u16(...) % 6` keeps a `clrlwi r4,r3,16` of the u16
 *    return that retail does not (4 bytes longer).
 *   Relocation names that differ from retail (pool constants): `ef_cube_f64_int_bias` (the int-to-float bias reads
 *     our `@N` pool copy).
 *  - `fn_800C9DD0__FUiP4Vec3P4Vec3P2EmP2PmUsfUi`: the `v3_copy`/`b_copy` copies move floats (`lfs`/`stfs`) where
 *    retail moves words (`lwz`/`stw`), and the slot +0x14 dispatch keeps `pm` in r29 (`lwz r11,28(r29)`) where
 *    retail goes through r3; declaration order, a named `Pm*` local, a cast and `-lang=c++` do not move it.
 *  - flipcheck: `.data` claimed, not emitted; `.sdata2` 0x8 of 0x30 (the pool is declared, so the conversion
 *    constant reads our `@N`); `.text` 0x1B7C of 0x1B78; extabindex differs in the cube row's size word.
 * SHAPES. The pool constants are declared `extern const`: retail hoists their loads out of every loop, which a
 *   non-`const` declaration (the calls might write it) prevents.  The three spiral/ring blocks declare their own
 *   counters in the order that reproduces retail's register assignment.
 * SHAPES. The two definitions spell their integer parameters `unsigned int`: their manglings encode `Ui`, which
 *   `types.h`'s `u32` (`unsigned long`) would not.
 */

#include "types.h"
#include "mh3_pad/vec3.h" /* the owner header (rule 2) */
#include "ef/ef_emitter.h" /* ef_random_float / ef_random_u16 (rule 2) */
#include "ef/ef_torus.h" /* ef_fabsf (rule 2) */
#include "MSL_C/alloc.h" /* fabsf / tanf (rule 2) */
#include "g3d/math_reciprocal.h" /* math_reciprocal (rule 2) */
#include "sqrt_f32.h" /* sqrt_f32 (rule 2) */

/* nw4r::math::VEC3, the type assignVec3/vec3_length_sq and this unit's three vector locals use. */
typedef struct {
    f32 x; /* +0x0 */
    f32 y; /* +0x4 */
    f32 z; /* +0x8 */
} Vec3; /* size: 0xC */

/* The unit's own `.data` strings (claimed, declared, never defined): the source file name and one
 * message per checked pointer. */
extern char ef_cube_file_name[]; /* "ef_cube.cpp" */
extern char ef_cube_err_em[]; /* "NW4R:Pointer Error\nem(=%p) is not valid pointer." */
extern char ef_cube_err_pm[]; /* "NW4R:Pointer Error\npm(=%p) is not valid pointer." */
extern char ef_cube_err_params[]; /* "NW4R:Pointer Error\nparams(=%p) is not valid pointer." */

/* The unit's own `.sdata2` pool (claimed, declared, never defined). */
extern const f32 ef_cube_f32_flt_min; /* 0x00800000, FLT_MIN */
extern const f32 ef_cube_f32_two; /* 2.0f */
extern const f32 ef_cube_f32_one; /* 1.0f */
extern const f32 ef_cube_f32_zero; /* 0.0f */
extern const f32 ef_cube_f32_percent; /* 0.01f */
extern const f64 ef_cube_f64_int_bias; /* 0x4330000080000000, the u32 -> f64 magic */
extern const f32 ef_cube_f32_min_size; /* 1e-5f: the smallest size, hollow ratio and direction component */
extern const f32 ef_cube_f32_pi; /* pi */
extern const f32 ef_cube_f32_pi_epsilon; /* the step a diffusion angle of pi is pulled back by */
extern const f32 ef_cube_f32_minus_one; /* -1.0f */

/* `nw4r::db::Panic`, declared in its namespace so the front-end emits the map's mangling
 * (`Panic__Q24nw4r2dbFPCciPCce`). */
namespace nw4r { namespace db { void Panic(const char* file, int line, const char* fmt, ...); } }

/* nw4r::math and effect-library helpers; retail's relocations carry their plain map names, so they
 * have C linkage. */
extern "C" {
extern void ef_vec3_normalize_to(Vec3* dst, Vec3* src);
extern void assignVec3(Vec3* dst, const Vec3* src);
extern f32 vec3_length_sq(const Vec3* v);
extern u16 fn_800A9FB0(u32 a, u16 b, f32 f, void* em);
extern void fn_800A99B4(u32 a, Vec3* b, void* em, Vec3* c, Vec3* d, Vec3* e, Vec3* f);
}

/* The "em" object this unit is handed: an effect manager. Only the fields the two functions touch are
 * named, and they are named by offset because nothing in the dump names them. */
typedef struct Em Em;
struct Em {
    u8 pad_0x00[0x67]; /* +0x00 */
    s8 field_0x67;     /* +0x67 */
    u8 pad_0x68[0x10]; /* +0x68 */
    f32 diffusionAngle; /* +0x78: how far a direction leans out of its face */
    u8 pad_0x7C[0x6C]; /* +0x7C */
    u16 field_0xE8;    /* +0xE8 */
    u8 pad_0xEA[0x02]; /* +0xEA */
    u32 random;        /* +0xEC: the emitter's random state */
    u8 pad_0xF0[0x08]; /* +0xF0 */
    u32 field_0xF8;    /* +0xF8 */
    u8 field_0xFC[4];  /* +0xFC */
}; /* size: 0x100 (approx: only the accessed offsets are evidenced) */

/* The "pm" object: its vtable pointer sits at +0x1C and slot 5 is the spawn call this unit makes. */
typedef struct Pm Pm;
typedef struct PmVtbl PmVtbl;
struct PmVtbl {
    u8 pad_0x00[0x14];                                                  /* +0x00 */
    void (*spawn_0x14)(Pm* self, u16 a, Vec3* b, Vec3* c, u32 d, f32 e, /* +0x14 */
                       u8* f, u32 g, u16 h);
}; /* size: 0x18 */
struct Pm {
    u8 pad_0x00[0x1C]; /* +0x00 */
    PmVtbl* vtbl;      /* +0x1C */
}; /* size: 0x20 */

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
void fn_800C9DD0(unsigned int a, Vec3* b, Vec3* c, Em* em, Pm* pm, u16 d, f32 f, unsigned int e)
{
    Vec3 v1;
    Vec3 v2;
    Vec3 v3;
    Vec3 b_copy;
    Vec3 v3_copy;
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
    assignVec3(&v1, b);
    if (vec3_length_sq(&v1) <= ef_cube_f32_flt_min) {
        v1.x = ef_random_float(&em->random) * ef_cube_f32_two - ef_cube_f32_one;
        v1.y = ef_random_float(&em->random) * ef_cube_f32_two - ef_cube_f32_one;
        v1.z = ef_random_float(&em->random) * ef_cube_f32_two - ef_cube_f32_one;
    }
    ef_vec3_normalize_to(&v1, &v1);
    assignVec3(&v2, b);
    v2.y = ef_cube_f32_zero;
    if (vec3_length_sq(&v2) <= ef_cube_f32_flt_min) {
        v2.x = ef_random_float(&em->random) * ef_cube_f32_two - ef_cube_f32_one;
        v2.z = ef_random_float(&em->random) * ef_cube_f32_two - ef_cube_f32_one;
    }
    ef_vec3_normalize_to(&v2, &v2);
    /* This unit's `Vec3` (the map's `P4Vec3`) and the helper's `nw4r::math::VEC3` share one 0xC-byte
     * layout. */
    VEC3_ctor((nw4r::math::VEC3*)&v3);
    fn_800A99B4(a, &v3, em, b, c, &v1, &v2);
    v3_copy = v3;
    b_copy = *b;
    s = ef_cube_f32_one + ef_cube_f32_percent * (f32)em->field_0x67 * ef_random_float(&em->random);
    r = fn_800A9FB0(a, d, f, em);
    pm->vtbl->spawn_0x14(pm, r, &b_copy, &v3_copy, e, s, em->field_0xFC, em->field_0xF8,
                         em->field_0xE8);
}

/* The larger of two magnitudes through the shapes' `fabsf` thunk; retail evaluates each side twice. */
#define EF_CUBE_MAX_ABS(a, b) (ef_fabsf(a) > ef_fabsf(b) ? ef_fabsf(a) : ef_fabsf(b))

/* A uniform random value in [-1, 1). */
#define EF_CUBE_RAND_SIGNED(em) (ef_cube_f32_two * ef_random_float(&(em)->random) - ef_cube_f32_one)

/* Emits `n` particles from the cube: on an even grid over the six faces (flag 0x20000), or at random inside the
 * shell the hollow ratio leaves; each direction leans out of its face by the emitter's diffusion angle. */
void fn_800CA200(unsigned int a, Em* em, Pm* pm, unsigned int n, unsigned int flags, void* params, u16 d,
                 unsigned int e, f32 f)
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
    if (sizeX < ef_cube_f32_min_size) {
        sizeX = ef_cube_f32_min_size;
    }
    if (sizeY < ef_cube_f32_min_size) {
        sizeY = ef_cube_f32_min_size;
    }
    if (sizeZ < ef_cube_f32_min_size) {
        sizeZ = ef_cube_f32_min_size;
    }
    if (ef_cube_f32_zero == cube->hollow) {
        hollow = ef_cube_f32_min_size;
    } else {
        hollow = ef_cube_f32_percent * cube->hollow;
    }
    shell = ef_cube_f32_one - hollow;
    angle = em->diffusionAngle;
    if (ef_fabsf(angle - ef_cube_f32_pi) < ef_cube_f32_pi_epsilon) {
        angle -= ef_cube_f32_pi_epsilon;
    }

    if (flags & 0x20000) {
        f32 step = ef_cube_f32_two / (f32)(int)(n + 1);
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
                Vec3 pos;
                Vec3 vel;

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
                u = (f32)col * step - ef_cube_f32_one;
                v = (f32)row * step - ef_cube_f32_one;
                px = u * sizeX;
                pz = v * sizeZ;
                spread = ef_cube_f32_one - EF_CUBE_MAX_ABS(u, v);
                lean = EF_CUBE_MAX_ABS(u, v);
                VEC3_ctor((nw4r::math::VEC3*)&pos);
                VEC3_ctor((nw4r::math::VEC3*)&vel);
                pos.x = px;
                pos.y = -sizeY;
                pos.z = pz;
                if (ef_cube_f32_zero != shell) {
                    f32 depth;
                    if (flags & 0x01000000) {
                        f32 r = ef_random_float(&em->random);
                        r *= r;
                        depth = ef_cube_f32_one - spread * (r * shell);
                    } else {
                        depth = ef_cube_f32_one - spread * (shell * ef_random_float(&em->random));
                    }
                    pos.y *= depth;
                }
                if (ef_cube_f32_zero == angle ||
                    (fabsf(pos.x) < ef_cube_f32_min_size && fabsf(pos.z) < ef_cube_f32_min_size)) {
                    vel.x = ef_cube_f32_zero;
                    vel.y = ef_cube_f32_minus_one;
                    vel.z = ef_cube_f32_zero;
                } else {
                    f32 inv = math_reciprocal(tanf(lean * angle));
                    vel.x = pos.x;
                    vel.y = inv * -sqrt_f32(pos.x * pos.x + pos.z * pos.z);
                    vel.z = pos.z;
                }
                fn_800C9DD0(a, &pos, &vel, em, pm, d, f, e);
            }
        }

        {
            int j;
            int k;

            /* The four side faces, one ring per row from the bottom up. */
            for (j = 1; j <= (int)n; j++) {
                f32 v = (f32)j * step - ef_cube_f32_one;
                f32 y = v * sizeY;

                for (k = 1; k <= (int)n; k++) {
                    f32 u = (f32)k * step - ef_cube_f32_one;
                    f32 pu = u * sizeX;
                    f32 spread = ef_cube_f32_one - EF_CUBE_MAX_ABS(u, v);
                    f32 lean = EF_CUBE_MAX_ABS(u, v);
                    Vec3 pos;
                    Vec3 vel;

                    VEC3_ctor((nw4r::math::VEC3*)&pos);
                    VEC3_ctor((nw4r::math::VEC3*)&vel);
                    pos.x = pu;
                    pos.y = y;
                    pos.z = -sizeZ;
                    if (ef_cube_f32_zero != shell) {
                        f32 depth;
                        if (flags & 0x01000000) {
                            f32 r = ef_random_float(&em->random);
                            r *= r;
                            depth = ef_cube_f32_one - spread * (r * shell);
                        } else {
                            depth = ef_cube_f32_one - spread * (shell * ef_random_float(&em->random));
                        }
                        pos.z *= depth;
                    }
                    if (ef_cube_f32_zero == angle ||
                        (fabsf(pos.x) < ef_cube_f32_min_size && fabsf(pos.y) < ef_cube_f32_min_size)) {
                        vel.x = ef_cube_f32_zero;
                        vel.y = ef_cube_f32_zero;
                        vel.z = ef_cube_f32_minus_one;
                    } else {
                        f32 inv = math_reciprocal(tanf(lean * angle));
                        vel.x = pos.x;
                        vel.y = pos.y;
                        vel.z = inv * -sqrt_f32(pos.x * pos.x + pos.y * pos.y);
                    }
                    fn_800C9DD0(a, &pos, &vel, em, pm, d, f, e);
                }
                for (k = 1; k <= (int)n; k++) {
                    f32 u = (f32)k * step - ef_cube_f32_one;
                    f32 pu = u * sizeZ;
                    f32 spread = ef_cube_f32_one - EF_CUBE_MAX_ABS(u, v);
                    f32 lean = EF_CUBE_MAX_ABS(u, v);
                    Vec3 pos;
                    Vec3 vel;

                    VEC3_ctor((nw4r::math::VEC3*)&pos);
                    VEC3_ctor((nw4r::math::VEC3*)&vel);
                    pos.x = sizeX;
                    pos.y = y;
                    pos.z = pu;
                    if (ef_cube_f32_zero != shell) {
                        f32 depth;
                        if (flags & 0x01000000) {
                            f32 r = ef_random_float(&em->random);
                            r *= r;
                            depth = ef_cube_f32_one - spread * (r * shell);
                        } else {
                            depth = ef_cube_f32_one - spread * (shell * ef_random_float(&em->random));
                        }
                        pos.x *= depth;
                    }
                    if (ef_cube_f32_zero == angle ||
                        (fabsf(pos.y) < ef_cube_f32_min_size && fabsf(pos.z) < ef_cube_f32_min_size)) {
                        vel.x = ef_cube_f32_one;
                        vel.y = ef_cube_f32_zero;
                        vel.z = ef_cube_f32_zero;
                    } else {
                        f32 inv = math_reciprocal(tanf(lean * angle));
                        vel.x = inv * sqrt_f32(pos.y * pos.y + pos.z * pos.z);
                        vel.y = pos.y;
                        vel.z = pos.z;
                    }
                    fn_800C9DD0(a, &pos, &vel, em, pm, d, f, e);
                }
                for (k = n; k >= 1; k--) {
                    f32 u = (f32)k * step - ef_cube_f32_one;
                    f32 pu = u * sizeX;
                    f32 spread = ef_cube_f32_one - EF_CUBE_MAX_ABS(u, v);
                    f32 lean = EF_CUBE_MAX_ABS(u, v);
                    Vec3 pos;
                    Vec3 vel;

                    VEC3_ctor((nw4r::math::VEC3*)&pos);
                    VEC3_ctor((nw4r::math::VEC3*)&vel);
                    pos.x = pu;
                    pos.y = y;
                    pos.z = sizeZ;
                    if (ef_cube_f32_zero != shell) {
                        f32 depth;
                        if (flags & 0x01000000) {
                            f32 r = ef_random_float(&em->random);
                            r *= r;
                            depth = ef_cube_f32_one - spread * (r * shell);
                        } else {
                            depth = ef_cube_f32_one - spread * (shell * ef_random_float(&em->random));
                        }
                        pos.z *= depth;
                    }
                    if (ef_cube_f32_zero == angle ||
                        (fabsf(pos.x) < ef_cube_f32_min_size && fabsf(pos.y) < ef_cube_f32_min_size)) {
                        vel.x = ef_cube_f32_zero;
                        vel.y = ef_cube_f32_zero;
                        vel.z = ef_cube_f32_one;
                    } else {
                        f32 inv = math_reciprocal(tanf(lean * angle));
                        vel.x = pos.x;
                        vel.y = pos.y;
                        vel.z = inv * sqrt_f32(pos.x * pos.x + pos.y * pos.y);
                    }
                    fn_800C9DD0(a, &pos, &vel, em, pm, d, f, e);
                }
                for (k = n; k >= 1; k--) {
                    f32 u = (f32)k * step - ef_cube_f32_one;
                    f32 pu = u * sizeZ;
                    f32 spread = ef_cube_f32_one - EF_CUBE_MAX_ABS(u, v);
                    f32 lean = EF_CUBE_MAX_ABS(u, v);
                    Vec3 pos;
                    Vec3 vel;

                    VEC3_ctor((nw4r::math::VEC3*)&pos);
                    VEC3_ctor((nw4r::math::VEC3*)&vel);
                    pos.x = -sizeX;
                    pos.y = y;
                    pos.z = pu;
                    if (ef_cube_f32_zero != shell) {
                        f32 depth;
                        if (flags & 0x01000000) {
                            f32 r = ef_random_float(&em->random);
                            r *= r;
                            depth = ef_cube_f32_one - spread * (r * shell);
                        } else {
                            depth = ef_cube_f32_one - spread * (shell * ef_random_float(&em->random));
                        }
                        pos.x *= depth;
                    }
                    if (ef_cube_f32_zero == angle ||
                        (fabsf(pos.y) < ef_cube_f32_min_size && fabsf(pos.z) < ef_cube_f32_min_size)) {
                        vel.x = ef_cube_f32_minus_one;
                        vel.y = ef_cube_f32_zero;
                        vel.z = ef_cube_f32_zero;
                    } else {
                        f32 inv = math_reciprocal(tanf(lean * angle));
                        vel.x = inv * -sqrt_f32(pos.y * pos.y + pos.z * pos.z);
                        vel.y = pos.y;
                        vel.z = pos.z;
                    }
                    fn_800C9DD0(a, &pos, &vel, em, pm, d, f, e);
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
                Vec3 pos;
                Vec3 vel;

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
                u = (f32)col * step - ef_cube_f32_one;
                v = (f32)row * step - ef_cube_f32_one;
                px = u * sizeX;
                pz = v * sizeZ;
                spread = ef_cube_f32_one - EF_CUBE_MAX_ABS(u, v);
                lean = EF_CUBE_MAX_ABS(u, v);
                VEC3_ctor((nw4r::math::VEC3*)&pos);
                VEC3_ctor((nw4r::math::VEC3*)&vel);
                pos.x = px;
                pos.y = sizeY;
                pos.z = pz;
                if (ef_cube_f32_zero != shell) {
                    f32 depth;
                    if (flags & 0x01000000) {
                        f32 r = ef_random_float(&em->random);
                        r *= r;
                        depth = ef_cube_f32_one - spread * (r * shell);
                    } else {
                        depth = ef_cube_f32_one - spread * (shell * ef_random_float(&em->random));
                    }
                    pos.y *= depth;
                }
                if (ef_cube_f32_zero == angle ||
                    (fabsf(pos.x) < ef_cube_f32_min_size && fabsf(pos.z) < ef_cube_f32_min_size)) {
                    vel.x = ef_cube_f32_zero;
                    vel.y = ef_cube_f32_one;
                    vel.z = ef_cube_f32_zero;
                } else {
                    f32 inv = math_reciprocal(tanf(lean * angle));
                    vel.x = pos.x;
                    vel.y = inv * sqrt_f32(pos.x * pos.x + pos.z * pos.z);
                    vel.z = pos.z;
                }
                fn_800C9DD0(a, &pos, &vel, em, pm, d, f, e);
            }
        }
    } else {
        int i;

        for (i = 0; i < (int)n; i++) {
            f32 x;
            f32 y;
            f32 z;
            Vec3 pos;
            Vec3 vel;

            if (ef_cube_f32_zero == hollow) {
                /* A solid cube: anywhere inside. */
                x = EF_CUBE_RAND_SIGNED(em);
                y = EF_CUBE_RAND_SIGNED(em);
                z = EF_CUBE_RAND_SIGNED(em);
            } else if (ef_cube_f32_one == hollow) {
                /* A hollow cube: on one of the six faces. */
                switch (ef_random_u16(&em->random) % 6) {
                case 0:
                    x = EF_CUBE_RAND_SIGNED(em);
                    y = EF_CUBE_RAND_SIGNED(em);
                    z = ef_cube_f32_one;
                    break;
                case 1:
                    x = EF_CUBE_RAND_SIGNED(em);
                    y = EF_CUBE_RAND_SIGNED(em);
                    z = ef_cube_f32_minus_one;
                    break;
                case 2:
                    x = EF_CUBE_RAND_SIGNED(em);
                    y = ef_cube_f32_one;
                    z = EF_CUBE_RAND_SIGNED(em);
                    break;
                case 3:
                    x = EF_CUBE_RAND_SIGNED(em);
                    y = ef_cube_f32_minus_one;
                    z = EF_CUBE_RAND_SIGNED(em);
                    break;
                case 4:
                    x = ef_cube_f32_one;
                    y = EF_CUBE_RAND_SIGNED(em);
                    z = EF_CUBE_RAND_SIGNED(em);
                    break;
                case 5:
                    x = ef_cube_f32_minus_one;
                    y = EF_CUBE_RAND_SIGNED(em);
                    z = EF_CUBE_RAND_SIGNED(em);
                    break;
                }
            } else {
                /* A shell: pick one of the three slab pairs by its volume, then a point inside it. */
                f32 slabY = sizeZ * (shell * (sizeX * sizeY));
                f32 slabZ = shell * (sizeZ * (hollow * (sizeX * sizeY)));
                f32 slabX = hollow * (sizeZ * (hollow * (sizeY * (sizeX * shell))));
                f32 pick = (slabY + slabZ + slabX) * ef_random_float(&em->random);

                if (pick < slabY) {
                    x = EF_CUBE_RAND_SIGNED(em);
                    y = shell * EF_CUBE_RAND_SIGNED(em);
                    if (y >= ef_cube_f32_zero) {
                        y = ef_cube_f32_one - y;
                    } else {
                        y = ef_cube_f32_minus_one - y;
                    }
                    z = EF_CUBE_RAND_SIGNED(em);
                } else if (pick < slabY + slabZ) {
                    x = EF_CUBE_RAND_SIGNED(em);
                    y = hollow * EF_CUBE_RAND_SIGNED(em);
                    z = shell * EF_CUBE_RAND_SIGNED(em);
                    if (z >= ef_cube_f32_zero) {
                        z = ef_cube_f32_one - z;
                    } else {
                        z = ef_cube_f32_minus_one - z;
                    }
                } else {
                    x = shell * EF_CUBE_RAND_SIGNED(em);
                    if (x >= ef_cube_f32_zero) {
                        x = ef_cube_f32_one - x;
                    } else {
                        x = ef_cube_f32_minus_one - x;
                    }
                    y = hollow * EF_CUBE_RAND_SIGNED(em);
                    z = hollow * EF_CUBE_RAND_SIGNED(em);
                }
            }

            VEC3_ctor((nw4r::math::VEC3*)&pos);
            setVec3((nw4r::math::VEC3*)&vel, ef_cube_f32_zero, ef_cube_f32_zero, ef_cube_f32_zero);
            if (y >= ef_cube_f32_zero && y >= ef_fabsf(x) && y >= ef_fabsf(z)) {
                vel.y = ef_cube_f32_one;
            } else if (y < ef_cube_f32_zero && -y >= ef_fabsf(x) && -y >= ef_fabsf(z)) {
                vel.y = ef_cube_f32_minus_one;
            } else if (x >= ef_cube_f32_zero && x >= ef_fabsf(y) && x >= ef_fabsf(z)) {
                vel.x = ef_cube_f32_one;
            } else if (x < ef_cube_f32_zero && -x >= ef_fabsf(y) && -x >= ef_fabsf(z)) {
                vel.x = ef_cube_f32_minus_one;
            } else if (z >= ef_cube_f32_zero) {
                vel.z = ef_cube_f32_one;
            } else {
                vel.z = ef_cube_f32_minus_one;
            }
            pos.x = x * sizeX;
            pos.y = y * sizeY;
            pos.z = z * sizeZ;
            if (ef_cube_f32_zero != angle) {
                if (ef_cube_f32_zero != vel.x) {
                    f32 lean = EF_CUBE_MAX_ABS(pos.y, pos.z);
                    f32 inv = math_reciprocal(tanf(lean * angle));
                    vel.x *= inv * sqrt_f32(pos.y * pos.y + pos.z * pos.z);
                    vel.y = pos.y;
                    vel.z = pos.z;
                } else if (ef_cube_f32_zero != vel.y) {
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
            fn_800C9DD0(a, &pos, &vel, em, pm, d, f, e);
        }
    }
}
