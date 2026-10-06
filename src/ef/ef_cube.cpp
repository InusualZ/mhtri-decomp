/*
 * ef/ef_cube.cpp - the cube emitter form: `fn_800C9DD0` normalises the two direction vectors, derives the spawn
 *   position and calls the particle manager's slot +0x14; `fn_800CA200` builds the cube's vertex grid and emits it
 *   through `fn_800C9DD0`.
 * RANGE. .text 0x800C9DD0-0x800CB948 (2 functions); extab 0x8000A55C-0x8000A56C, extabindex 0x80023BB0-0x80023BC8,
 *   .data 0x80594C68-0x80594D20 (the `__FILE__` string "ef_cube.cpp" first), .sdata2 0x80796240-0x80796270.
 *   Right edge: the source file changes to "ef_cylinder.cpp" and the `.sdata2` run jumps from `lbl_8079626C` to
 *   `lbl_80796270`.
 * FLAGS. `cflags_main`; file-wide `#pragma peephole off` (retail keeps the `li r0,136; psq_lx` FPR epilogue and
 *   the `clrlwi r4,r30,16` at the call site; `-opt nopeephole` gives the same object) and `#pragma fp_contract off`
 *   (retail keeps every `a*b+c` as two instructions; a temporary per site does the same).
 * NAMES. The map has only `fn_` stems for the range; the `Em` fields keep `field_0xNN` names.
 * RESIDUALS. `fn_800CA200__FUiP2EmP2PmUiUiPvUsUif` (0x800CA200-0x800CB948) is unwritten past its three pointer
 *   asserts (lines 94-96). Its known structure: a 14-element `Vec3` array at r1+8 worked on in pairs from the top;
 *   six blocks of `VEC3_ctor` x2, a fill, a scale by `ef_random_float(&em->field_0xEC)`, a `fabsf` compare against
 *   `lbl_80796268`, `fn_80463F98` + `math_reciprocal` + `sqrt_f32`, then `fn_800C9DD0(a, &pair[1], &pair[0], em, pm, d,
 *   f, e)`; 69 `fn_800C9DCC` (`fabsf` thunk) calls; loops over `n*n` and over `n`, the inner one a 4-state
 *   `switch` walking a ring.
 *  - `fn_800C9DD0__FUiP4Vec3P4Vec3P2EmP2PmUsfUi`: the `v3_copy`/`b_copy` copies move floats (`lfs`/`stfs`) where
 *    retail moves words (`lwz`/`stw`), and the slot +0x14 dispatch keeps `pm` in r29 (`lwz r11,28(r29)`) where
 *    retail goes through r3; declaration order, a named `Pm*` local, a cast and `-lang=c++` do not move it.
 *   flipcheck: `.data` claimed, not emitted; `.sdata2` 0x8 of 0x30 (the pool is declared, so the conversion
 *   constant reads our `@N`); `.text` 0x728 of 0x1B78; extab and extabindex differ in the unwritten row's record.
 *   Relocation names that differ from retail (pool constants, save helpers, statics): `_savegpr_18`,
 *     `lbl_80796260`, `lbl_8079624C`, `lbl_80796250`, `lbl_80796248`, `lbl_80796264`, `fn_800C9DCC`,
 *     `lbl_80796268`, `lbl_80796258`, `lbl_80796244`, `lbl_8079626C`, `VEC3_ctor`, `ef_random_float`,
 *     `fn_80463F04`, `fn_80463F98`, `math_reciprocal`, `sqrt_f32`, `fn_800C9DD0__FUiP4Vec3P4Vec3P2EmP2PmUsfUi`,
 *     `ef_random_u16`, `setVec3`, `_restgpr_18`.
 * SHAPES. The two definitions spell their integer parameters `unsigned int`: their manglings encode `Ui`, which
 *   `types.h`'s `u32` (`unsigned long`) would not.
 */

#include "types.h"
#include "mh3_pad/vec3.h" /* the owner header (rule 2) */
#include "ef/ef_emitter.h" /* ef_random_float (rule 2) */

/* nw4r::math::VEC3, the type assignVec3/vec3_length_sq and this unit's three vector locals use. */
typedef struct {
    f32 x; /* +0x0 */
    f32 y; /* +0x4 */
    f32 z; /* +0x8 */
} Vec3; /* size: 0xC */

/* The unit's own `.data` strings (claimed, declared, never defined): the source file name and one
 * message per checked pointer. */
extern char lbl_80594C68[]; /* "ef_cube.cpp" */
extern char lbl_80594C74[]; /* "NW4R:Pointer Error\nem(=%p) is not valid pointer." */
extern char lbl_80594CA8[]; /* "NW4R:Pointer Error\npm(=%p) is not valid pointer." */
extern char lbl_80594CDC[]; /* "NW4R:Pointer Error\nparams(=%p) is not valid pointer." */

/* The unit's own `.sdata2` pool (claimed, declared, never defined). */
extern f32 lbl_80796240; /* 0x00800000, FLT_MIN */
extern f32 lbl_80796244; /* 2.0f */
extern f32 lbl_80796248; /* 1.0f */
extern f32 lbl_8079624C; /* 0.0f */
extern f32 lbl_80796250; /* 0.01f */
extern f64 lbl_80796258; /* 0x4330000080000000, the u32 -> f64 magic */

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
    u8 pad_0x68[0x80]; /* +0x68 */
    u16 field_0xE8;    /* +0xE8 */
    u8 pad_0xEA[0x02]; /* +0xEA */
    u32 field_0xEC;    /* +0xEC */
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
        nw4r::db::Panic(lbl_80594C68, 43, lbl_80594C74, em);
    }
    ok = IS_VALID_PTR(pm);
    if (!ok) {
        nw4r::db::Panic(lbl_80594C68, 44, lbl_80594CA8, pm);
    }

    ef_vec3_normalize_to(c, c);
    assignVec3(&v1, b);
    if (vec3_length_sq(&v1) <= lbl_80796240) {
        v1.x = ef_random_float(&em->field_0xEC) * lbl_80796244 - lbl_80796248;
        v1.y = ef_random_float(&em->field_0xEC) * lbl_80796244 - lbl_80796248;
        v1.z = ef_random_float(&em->field_0xEC) * lbl_80796244 - lbl_80796248;
    }
    ef_vec3_normalize_to(&v1, &v1);
    assignVec3(&v2, b);
    v2.y = lbl_8079624C;
    if (vec3_length_sq(&v2) <= lbl_80796240) {
        v2.x = ef_random_float(&em->field_0xEC) * lbl_80796244 - lbl_80796248;
        v2.z = ef_random_float(&em->field_0xEC) * lbl_80796244 - lbl_80796248;
    }
    ef_vec3_normalize_to(&v2, &v2);
    /* This unit's `Vec3` (the map's `P4Vec3`) and the helper's `nw4r::math::VEC3` share one 0xC-byte
     * layout. */
    VEC3_ctor((nw4r::math::VEC3*)&v3);
    fn_800A99B4(a, &v3, em, b, c, &v1, &v2);
    v3_copy = v3;
    b_copy = *b;
    s = lbl_80796248 + lbl_80796250 * (f32)em->field_0x67 * ef_random_float(&em->field_0xEC);
    r = fn_800A9FB0(a, d, f, em);
    pm->vtbl->spawn_0x14(pm, r, &b_copy, &v3_copy, e, s, em->field_0xFC, em->field_0xF8,
                         em->field_0xE8);
}

/* Builds the effect cube's vertex grid and emits it through fn_800C9DD0 once per face; only the three
 * pointer asserts are written. */
void fn_800CA200(unsigned int a, Em* em, Pm* pm, unsigned int n, unsigned int flags, void* params, u16 d,
                 unsigned int e, f32 f)
{
    int ok;

    ok = IS_VALID_PTR(em);
    if (!ok) {
        nw4r::db::Panic(lbl_80594C68, 94, lbl_80594C74, em);
    }
    ok = IS_VALID_PTR(pm);
    if (!ok) {
        nw4r::db::Panic(lbl_80594C68, 95, lbl_80594CA8, pm);
    }
    ok = IS_VALID_PTR(params);
    if (!ok) {
        nw4r::db::Panic(lbl_80594C68, 96, lbl_80594CDC, params);
    }
}
