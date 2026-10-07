/*
 * ef/ef_drawbillboardstrategy.cpp - nw4r::ef DrawBillboardStrategy and DrawDirectionalStrategy: their
 *   constructors and inline destructors, the draw dispatch, the normal, Y and directional billboard walkers and their
 *   quad writers, the directional world- and view-space walkers with their rotation and local matrices, two copies
 *   of the out-of-line GX FIFO writers and four-vertex writers, the GX setup and the ahead-vector builders.
 * RANGE. .text 0x800B9A44-0x800BE154 (36 functions); extab 0x8000A23C-0x8000A30C, extabindex 0x80023700-0x80023838,
 *   .rodata 0x8056F770-0x8056F7D0, .data 0x80593E88-0x805941F8, .sdata 0x80791300-0x80791340, .sdata2
 *   0x80796150-0x80796190.  The range holds two TUs by their `__FILE__` strings: "ef_drawbillboardstrategy.cpp"
 *   (0x80593E88) and "ef_drawdirectionalstrategy.cpp" (0x80594070, its bodies from 0x800BBDEC); the seam between
 *   them is open.  Left edge: the "ef_drawstripestrategy.cpp" strings belong to the code before 0x800B9A44.
 *   Seam (playbook 80, measured): every strategy TU runs [constructor, ..., inline destructor], and each constructor
 *   installs the vtable at the end of its own TU's `.data`; so the DrawFreeStrategy constructor at this range's tail
 *   (0x800BE118) is the next TU's, and the directional class [0x800BBDEC, 0x800BE118) is a TU of its own.
 * FLAGS. `cflags_main`; `#pragma peephole off` before the includes (the GX FIFO writers keep the unfused `clrlwi`/
 *   `extsh`/`extsb` in front of every narrowing store); `#pragma dont_inline on` at the end (the inline destructors
 *   call their base out of line).
 * NAMES. The classes' virtuals carry their manglings; the helpers are `extern "C"`.  GUESS names, from the dispatch
 *   on the draw setting (type option 0/3 normal, 1 Y, 2 directional; the directional draw mode at +0xB2) and the
 *   bodies: `ef_billboard_draw_normal`/`_normal_quad`/`_draw_y`/`_y_quad`/`_draw_directional`/`_directional_quad`,
 *   `ef_billboard_quad_corners` (the inline paired-single corner writer), `ef_billboard_setup_gx`, `_write_quad`,
 *   `_gx_end`/`_gx_u8`/`_gx_position`/`_tex_flag`, `_vec2_set`, `_ahead_younger`/`_ahead_neighbours`;
 *   `ef_directional_draw_world`/`_draw_view`/`_local_mtx`/`_rotate_mtx`/`_setup_gx`/`_write_quad`/
 *   `_write_quad_center`/`_draw_mode`/`_gx_*`/`_tex_flag`; the strings `ef_billboard_*_str`/`ef_directional_*_str`,
 *   the constants `ef_billboard_percent`/`_zero`/`_one`, the tables `ef_billboard_texcoords`/
 *   `ef_directional_texcoords`/`ef_directional_quad_vtx`/`ef_directional_cross_vtx` by their contents.
 *   GUESS (from the body and its callers): `ef_billboard_draw_normal`, `ef_billboard_normal_quad`,
 *   GUESS: `ef_billboard_draw_y`, `ef_billboard_y_quad`, `ef_billboard_draw_directional`,
 *   GUESS: `ef_billboard_directional_quad`, `ef_billboard_gx_end`, `ef_billboard_gx_u8`, `ef_billboard_tex_flag`,
 *   GUESS: `ef_billboard_gx_position`, `ef_billboard_vec2_set`, `ef_directional_gx_end`, `ef_directional_gx_u8`,
 *   GUESS: `ef_directional_tex_flag`, `ef_directional_gx_position`, `ef_billboard_write_quad`,
 *   GUESS: `ef_directional_write_quad_center`, `ef_billboard_ahead_younger`, `ef_billboard_ahead_neighbours`,
 *   GUESS: `ef_billboard_gx_position3f` and `ef_directional_gx_position3f` (the 0x14 FIFO writers at +0x10 of the
 *   GUESS: two `_gx_position` rows, reached by a tail call), `ef_billboard_setup_gx`, `ef_directional_setup_gx`, `ef_directional_draw_world`,
 *   GUESS: `ef_directional_local_mtx`, `ef_directional_rotate_mtx`, `ef_directional_draw_view`,
 *   GUESS: `ef_directional_write_quad`, `ef_directional_draw_mode`.
 * SHAPES. The quad writers take the floats first and the pivot and flags last (the caller then sets r6/r7 after the
 *   float moves), and `ef_directional_local_mtx` takes the rotation last for the same reason; the loops' 0.0f, 0.5f
 *   and epsilon are literals so MWCC hoists them, the billboard 1.0f is the pool symbol; the texcoord tables are
 *   `__declspec(section ".sdata")` (retail reaches them `@sda21`); the first-ahead loops use their own iterator;
 *   `ef_billboard_normal_quad`'s unrolled branch is an `asm` block in an inline helper with `register` parameters
 *   (the target's own paired-single sequence, f21-f25 clobbered); the two GX setups' pointer asserts are the NW4R
 *   expression form `(void)(ok || (Panic(...), 0))`, whose value leaves retail's dead `li r0,0; cmpwi r0,0`
 *   (playbook 103).
 * RESIDUALS. Every row is written.  The source order differs from retail's, so `.text` and the extab and extabindex
 *   records run in another order.  1 partial row:
 *  - `ef_billboard_normal_quad` (0x800BA1E0): the `register` parameters the asm block needs keep the roll branch's
 *    temporaries out of f21-f31, so our frame saves f15-f20 too (0x1B0 against 0x150) and the roll branch's
 *    registers differ.
 *   Data: `.rodata` and `.sdata` are emitted and match; `.sdata2` is our own pool (the literals); `.data` holds only
 *   our two vtables - the strings stay `extern` because the range is two TUs (the three quad writers' strings, one
 *   "ef_drawbillboardstrategy.cpp" copy each, sit after the billboard vtable, and the directional TU's after them).
 *   flipcheck: `.text` 0x47F8 of 0x4710, extab 0xE0 of 0xD0, extabindex 0x150 of 0x138, `.data` 0x38 of 0x370,
 *   `.sdata2` 0x1C of 0x40.
 *   Relocation names that differ from retail (pool constants, save helpers, statics): `ef_billboard_zero`.
 *   `DrawBillboardStrategy` (constructor): the empty body is the whole source - the compiler
 *     emits the base call and the vtable store.
 *   `DrawDirectionalStrategy` (constructor): the empty body is the whole source - the compiler
 *     emits the base call and the vtable store.
 *   `DrawFreeStrategy` (constructor): the empty body is the whole source - the compiler
 *     emits the base call and the vtable store.
 */

#pragma peephole off

#include "ef.h"
#include "ef/ef_pointer_assert.h" /* EF_VALID_PTR_ASSERT */
#include "gx.h"
#include "sys_mem.h"
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "fn_8004CAD8/mtx.h" /* the matrix helpers (rule 2) */
#include "ef/fn_800AEE48.h" /* ef_vec3_normalize / ef_pm_next_alive / ef_pm_prev_alive, owned by ef_drawstripestrategy.cpp's range (rule 2) */
#include "ef/ef_drawstripestrategy.h" /* the walker family ef_ahead_manager_axis_y..ef_ahead_move_dir (rule 2) */
#include "ef/ef_drawbillboardstrategy.h"
#include "ef/ef_drawfreestrategy.h" /* DrawFreeStrategy, whose constructor closes this range */
#include "ef/ef_util.h"               /* ef_mtx34_column_length (rule 2) */
#include "ef/ef_particle.h"           /* ef_particle_get_scale / ef_particle_get_scale_y (rule 2) */
#include "ef/ef_particlemanager.h"    /* ef_pm_get_mtx (rule 2) */
#include "g3d/g3d_scnroot.h"          /* VEC2_ctor (rule 2) */
#include "g3d/g3d_calcview.h"         /* mtx34_concat (rule 2) */
#include "g3d/g3d_anmchr.h"           /* math_reciprocal (rule 2) */
#include "nw4r/mtx34_mult_vec3.h"     /* mtx34_mult_vec3 (rule 2) */
#include "fn_8004CAD8.h"              /* sqrt_f32 / PSVECSubtract (rule 2) */
#include "RVLGX/GXAttr.h"
#include "RVLGX/GXGeometry.h"
#include "ef/ef_get_life_status.h"     /* ef_get_life_status (rule 2) */
#include "ef/ef_particle_get_move_dir.h" /* ef_particle_get_move_dir (rule 2) */
#include "nw4r/fn_805012C4.h"         /* mtx34_rotate_vec3 (rule 2) */
#include "ef/ef_emitter_tex_flags.h"   /* ef_emitter_get_mtx (rule 2) */
#include "g3d/fn_80075DCC.h"           /* mtx34_set (rule 2) */

/* `nw4r::db::Panic`, declared in its namespace so the front-end emits the map's mangling
 * (`Panic__Q24nw4r2dbFPCciPCce`). */
#ifdef __cplusplus
namespace nw4r {
namespace db {
void Panic(const char* file, int line, const char* fmt, ...);
}  // namespace db
}  // namespace nw4r
#endif

#ifdef __cplusplus
extern "C" {
#endif


/* --------------------------------------------------------------------------------------------------
 * Declarations: a neighbouring unit's base constructor and this unit's unwritten walkers.
 * -------------------------------------------------------------------------------------------------- */

/* The walker family this unit's dispatch selects. */
void ef_billboard_draw_normal(nw4r::ef::DrawBillboardStrategy* self, const EfDrawInfo* info, EfDrawParticleManager* pm);
void ef_billboard_normal_quad(nw4r::ef::DrawBillboardStrategy* self, EfDrawParticle* p, const MTX34* mtx,
                              f32 axis_x, f32 axis_y, f32 unused, f32 cos_roll, f32 sin_roll, f32 scale_x,
                              f32 scale_y, const EfVec2* pivot, u32 flags);
void ef_billboard_gx_end(void);
void ef_billboard_gx_u8(u8 value);
int ef_billboard_tex_flag(u32 value);
void ef_billboard_gx_position(Vec* v);
void ef_billboard_write_quad(void* unused, void* mtx, Vec* a, Vec* b, u32 flags);
void ef_billboard_setup_gx(nw4r::ef::DrawStrategyImpl* self, const EfDrawInfo* em, EfDrawParticleManager* args);
void ef_billboard_draw_y(nw4r::ef::DrawBillboardStrategy* self, const EfDrawInfo* info, EfDrawParticleManager* pm);
void ef_billboard_y_quad(nw4r::ef::DrawBillboardStrategy* self, EfDrawParticle* p, const MTX34* mtx, f32 axis_x,
                         f32 axis_y, f32 unused, f32 view_y, f32 view_z, f32 scale_x, f32 scale_y,
                         const EfVec2* pivot, u32 flags);
void ef_billboard_vec2_set(f32* dst, f32 x, f32 y);
void ef_billboard_draw_directional(nw4r::ef::DrawBillboardStrategy* self, const EfDrawInfo* info,
                                   EfDrawParticleManager* pm);
void ef_billboard_directional_quad(nw4r::ef::DrawBillboardStrategy* self, EfDrawParticle* p, const MTX34* mtx,
                                   f32 axis_x, f32 axis_y, f32 unused, f32 cos_dir, f32 sin_dir, f32 scale_x,
                                   f32 scale_y, f32 stretch, const EfVec2* pivot, u32 flags);


/* The ahead-context argument block the walkers read: the particle handle and two cached positions. */
typedef struct EfAheadArgs {
    void* particle;    /* +0x00 */
    u8 pad_0x04[0x94]; /* +0x04 */
    VEC3 prev_pos;     /* +0x98  the previous resolved position */
    Vec pos;           /* +0xA4  the fallback position */
} EfAheadArgs; /* size: 0xB0 (lower bound, the record continues past what this unit reads) */

/* The particle/emitter record each walker advances: its world position sits at +0xAC. */
typedef struct EfWalkerObj {
    u8 pad_0x00[0xAC]; /* +0x00 */
    Vec world_pos;     /* +0xAC */
} EfWalkerObj; /* size: 0xB8 (lower bound, the record continues past what this unit reads) */

/* The particle's packed draw/rotate flag bytes (read by the flag accessor). */
typedef struct EfParticleFlags {
    u8 pad_0x00[0xB2]; /* +0x00 */
    u8 flags_0xB2;     /* +0xB2  draw/rotate flag bits */
} EfParticleFlags; /* size: 0xB3 (lower bound, the record continues past what this unit reads) */

/* The walker family the shape selector returns (defined outside this range). */
void ef_billboard_ahead_younger(Vec* out, EfAheadArgs* args, EfWalkerObj* em);
void ef_billboard_ahead_neighbours(Vec* out, EfAheadArgs* args, EfWalkerObj* em);
typedef void (*EfWalkerFn)(void);

/* The large per-particle emitters this unit dispatches to (defined further down). */
void ef_directional_draw_world(nw4r::ef::DrawDirectionalStrategy* self, const EfDrawInfo* info,
                               EfDrawParticleManager* pm);
void ef_directional_draw_view(nw4r::ef::DrawDirectionalStrategy* self, const EfDrawInfo* info,
                              EfDrawParticleManager* pm);
void ef_directional_write_quad_center(void* mtx, Vec* a, Vec* b, u32 flags);
void ef_directional_local_mtx(MTX34* out, u8 pivot_mode, f32 pivot_x, f32 pivot_y, f32 scale_x, f32 scale_y,
                              f32 stretch, const MTX34* rot);
void ef_directional_rotate_mtx(MTX34* out, EfDrawParticle* p, u8 axis);
void ef_directional_write_quad(void* mtx, Vec* src, u32 flags);
void ef_directional_setup_gx(nw4r::ef::DrawStrategyImpl* self, const EfDrawInfo* info, EfDrawParticleManager* args);
s32 ef_directional_draw_mode(void* unused, EfParticleFlags* self);

/* SDK GX state setters and the matrix helpers, declared locally. */
extern void GXLoadPosMtxImm(void* mtx, u32 id);
extern void GXSetCurrentMtx(u32 id);
/* The quads' texture coordinates, (s, t) byte pairs the GX_VA_TEX0 array reads (`.sdata`, 32-byte aligned). */
__declspec(section ".sdata") static u8 ef_billboard_texcoords[32] __attribute__((aligned(32))) = {
    0, 1, 0, 0, 1, 0, 1, 1};
__declspec(section ".sdata") static u8 ef_directional_texcoords[32] __attribute__((aligned(32))) = {
    0, 1, 0, 0, 1, 0, 1, 1};

/* The ahead-context walkers: the particle/emitter lookup helpers and the small vector helpers they
 * use (all still `fn_*`/SDK, defined outside this range). */
extern f32 ef_billboard_percent; /* 100.0f, the per-100 scale divisor   .sdata2 0x80796150 */
extern f32 ef_billboard_zero;    /* 0.0f                                .sdata2 0x80796154 */
extern f32 ef_billboard_one;     /* 1.0f                                .sdata2 0x80796158 */

/* This unit's `__FILE__`/assert strings and the tables the constructors write (its claimed `.data`),
 * declared, never defined. */
extern char ef_billboard_file_str[]; /* "ef_drawbillboardstrategy.cpp"                                 .data 0x80593E88 */
extern char ef_billboard_pm_assert_str[]; /* "NW4R:Pointer Error\npm(=%p) is not valid pointer."           .data 0x80593EA8 */
extern char ef_billboard_resource_assert_str[]; /* "NW4R:Pointer Error\npm->mResource(=%p) is not valid..."     .data 0x80593EDC */
extern char ef_billboard_ed_assert_str[]; /* "NW4R:Pointer Error\n&ed(=%p) is not valid pointer."         .data 0x80593F18 */
extern char ef_billboard_dir_quad_particle_str[]; /* "NW4R:Pointer Error
particle(=%p) ..." .data 0x80593F68 */
extern char ef_billboard_dir_quad_file_str[];     /* "ef_drawbillboardstrategy.cpp"          .data 0x80593FA0 */
extern char ef_billboard_y_quad_particle_str[];   /* "NW4R:Pointer Error
particle(=%p) ..." .data 0x80593FC0 */
extern char ef_billboard_y_quad_file_str[];       /* "ef_drawbillboardstrategy.cpp"          .data 0x80593FF8 */
extern char ef_billboard_quad_particle_str[];     /* "NW4R:Pointer Error
particle(=%p) ..." .data 0x80594018 */
extern char ef_billboard_quad_file_str[];         /* "ef_drawbillboardstrategy.cpp"          .data 0x80594050 */
extern char ef_directional_file_str[]; /* "ef_drawdirectionalstrategy.cpp"                             .data 0x80594070 */
extern char ef_directional_p_assert_str[]; /* "NW4R:Pointer Error\np(=%p) is not valid pointer."           .data 0x80594090 */
extern char ef_directional_pm_assert_str[]; /* "NW4R:Pointer Error\npm(=%p) is not valid pointer."           .data 0x805940C0 */
extern char ef_directional_resource_assert_str[]; /* "NW4R:Pointer Error\npm->mResource(=%p) is not valid..."     .data 0x805940F4 */
extern char ef_directional_ed_assert_str[]; /* "NW4R:Pointer Error\n&ed(=%p) is not valid pointer."         .data 0x80594130 */
extern char ef_directional_manager_em_assert_str[]; /* "NW4R:Pointer Error
pm->mManagerEM(=%p) ..."  .data 0x80594164 */
extern char ef_directional_pp_assert_str[];         /* "NW4R:Pointer Error
pp(=%p) ..."             .data 0x805941A4 */

/* --------------------------------------------------------------------------------------------------
 * The constructors and deleting destructors of this range.
 * -------------------------------------------------------------------------------------------------- */


#ifdef __cplusplus
}
#endif

namespace nw4r {
namespace ef {

/* 0x800B9A44 (0x3C): builds the strategy. */
DrawBillboardStrategy::DrawBillboardStrategy() {}

/* 0x800B9A80 (0x378): hands the draw to the point, stripe or tube path the draw setting's type option selects. */
void DrawBillboardStrategy::Draw(const EfDrawInfo& info, EfDrawParticleManager* pm) {
    EfEmitterShape* shape;

    if (!IsValidPointer((u32)pm)) {
        nw4r::db::Panic(ef_billboard_file_str, 502, ef_billboard_pm_assert_str, pm);
    }
    EF_VALID_PTR_ASSERT(ef_billboard_file_str, 503, ef_billboard_resource_assert_str, pm->resource);
    shape = (EfEmitterShape*)ef_resource_draw_setting(pm->resource);
    if (!IsValidPointer((u32)shape)) {
        nw4r::db::Panic(ef_billboard_file_str, 506, ef_billboard_ed_assert_str, shape);
    }
    switch (shape->shape_0xAD) {
    case 0:
    case 3:
        ef_billboard_draw_normal(this, &info, pm);
        break;
    case 1:
        ef_billboard_draw_y(this, &info, pm);
        break;
    case 2:
        ef_billboard_draw_directional(this, &info, pm);
        break;
    }
}

}  // namespace ef
}  // namespace nw4r

#ifdef __cplusplus
extern "C" {
#endif

#pragma fp_contract off

/* 0x800B9DF8 (0x3E8): draws every particle as a camera-facing quad, rolled with the view unless the type option is
 * the no-roll billboard. */
void ef_billboard_draw_normal(nw4r::ef::DrawBillboardStrategy* self, const EfDrawInfo* info, EfDrawParticleManager* pm) {
    MTX34 view;
    MTX34 mtx;
    EfVec2 pivot;
    EfEmitterDrawSetting* ed;
    u32 use_tex;
    f32 axis_x;
    f32 axis_y;
    f32 unused;
    f32 length;
    f32 cos_roll;
    f32 sin_roll;
    nw4r::ef::DrawStrategyImpl::GetFirstDrawParticleFunc get_first;
    nw4r::ef::DrawStrategyImpl::GetNextDrawParticleFunc get_next;
    bool first;
    EfDrawParticle* p;

    if (!IsValidPointer((u32)pm)) {
        nw4r::db::Panic(ef_billboard_file_str, 528, ef_billboard_pm_assert_str, pm);
    }
    MTX34_ctor(&view);
    ef_draw_info_view_mtx(info, &view);
    ed = (EfEmitterDrawSetting*)ef_resource_draw_setting(pm->resource);
    ef_billboard_setup_gx(self, info, pm);
    use_tex = self->mNumTexmap != 0;
    VEC2_ctor(&pivot);
    pivot.x = ed->scale_a / ef_billboard_percent;
    pivot.y = ed->scale_b / ef_billboard_percent;
    MTX34_ctor(&mtx);
    ef_pm_get_mtx(pm, &mtx);
    mtx34_concat(&mtx, &view, &mtx);
    axis_x = ef_mtx34_column_length((const f32*)&mtx, 0);
    axis_y = ef_mtx34_column_length((const f32*)&mtx, 1);
    unused = ef_billboard_zero;
    length = sqrt_f32(view.m[0][1] * view.m[0][1] + view.m[1][1] * view.m[1][1]);
    if (ed->type_option == 3 || ef_billboard_zero == length) {
        sin_roll = ef_billboard_zero;
        cos_roll = ef_billboard_one;
    } else {
        f32 inv = math_reciprocal(length);
        sin_roll = -view.m[0][1] * inv;
        cos_roll = view.m[1][1] * inv;
    }
    get_first = self->GetGetFirstDrawParticleFunc(ed->flags & 0x800);
    get_next = self->GetGetNextDrawParticleFunc(ed->flags & 0x800);
    first = true;
    for (p = get_first(pm); p != NULL; p = get_next(pm, p)) {
        f32 scale_x;
        f32 scale_y;

        scale_x = ef_particle_get_scale((EfParticle*)p);
        if (scale_x < ef_float_epsilon()) {
            continue;
        }
        scale_y = ef_particle_get_scale_y((EfParticle*)p);
        if (scale_y < ef_float_epsilon()) {
            continue;
        }
        self->SetupGP(p, *ed, *info, first, false);
        first = false;
        ef_billboard_normal_quad(self, p, &mtx, axis_x, axis_y, unused, cos_roll, sin_roll, scale_x, scale_y,
                                 &pivot, use_tex);
    }
}

/* Writes the four corners of an unrolled quad: the transformed position, offset by the scaled axes. */
inline void ef_billboard_quad_corners(register VEC3* out, register const EfDrawParticle* p, register const MTX34* mtx,
                                      register const EfVec2* pivot, register f32 axis_x, register f32 axis_y,
                                      register f32 cos_roll, register f32 sin_roll, register f32 scale_x,
                                      register f32 scale_y) {
    asm {
        psq_l      f0, 0xAC(p), 0, 0
        psq_l      f2, 0x00(mtx), 0, 0
        psq_l      f1, 0xB4(p), 1, 0
        ps_mul     f4, f2, f0
        psq_l      f3, 0x08(mtx), 0, 0
        ps_madd    f5, f3, f1, f4
        psq_l      f8, 0x10(mtx), 0, 0
        ps_sum0    f6, f5, f6, f5
        psq_l      f9, 0x18(mtx), 0, 0
        ps_mul     f10, f8, f0
        ps_madd    f11, f9, f1, f10
        psq_l      f2, 0x20(mtx), 0, 0
        ps_sum0    f12, f11, f12, f11
        psq_l      f3, 0x28(mtx), 0, 0
        ps_mul     f4, f2, f0
        ps_merge00 f25, f6, f12
        ps_madd    f5, f3, f1, f4
        ps_sum0    f21, f5, f6, f5
        psq_l      f7, 0(pivot), 0, 0
        ps_merge00 f13, sin_roll, cos_roll
        ps_merge00 f24, scale_x, scale_y
        ps_muls0   f22, f13, axis_x
        ps_merge10 f22, f22, f22
        ps_muls0   f23, f13, axis_y
        ps_neg     f13, f23
        ps_merge01 f23, f23, f13
        ps_nmsub   f7, f24, f7, f7
        ps_madds0  f13, f22, f7, f25
        ps_madds1  f25, f23, f7, f13
        psq_st     f21, 0x08(out), 1, 0
        psq_st     f21, 0x14(out), 1, 0
        psq_st     f21, 0x20(out), 1, 0
        psq_st     f21, 0x2C(out), 1, 0
        ps_muls0   f7, f22, f24
        ps_muls1   f13, f23, f24
        ps_add     f22, f7, f13
        ps_neg     f13, f13
        ps_add     f7, f7, f13
        ps_sub     f13, f25, f7
        psq_st     f13, 0x00(out), 0, 0
        ps_sub     f13, f25, f22
        psq_st     f13, 0x0C(out), 0, 0
        ps_add     f13, f25, f7
        psq_st     f13, 0x18(out), 0, 0
        ps_add     f13, f25, f22
        psq_st     f13, 0x24(out), 0, 0
    }
}

/* 0x800BA1E0 (0x508): writes one particle's quad around its transformed position, turned by the particle's own
 * roll when it has one. */
void ef_billboard_normal_quad(nw4r::ef::DrawBillboardStrategy* self, EfDrawParticle* p, const MTX34* mtx,
                              f32 axis_x, f32 axis_y, f32 unused, f32 cos_roll, f32 sin_roll, f32 scale_x,
                              f32 scale_y, const EfVec2* pivot, u32 flags) {
    VEC3 corner[4];
    VEC3 rotate;

    if (!IsValidPointer((u32)p)) {
        nw4r::db::Panic(ef_billboard_quad_file_str, 136, ef_billboard_quad_particle_str, p);
    }
    VEC3_ctor(&rotate);
    ef_particle_get_rotate(p, (Vec3*)&rotate);
    if (0.0f != rotate.z) {
        VEC3 center;
        VEC3 pos;
        VEC3 edge_a;
        VEC3 edge_b;
        f32 cos_z;
        f32 sin_z;
        f32 px;
        f32 py;
        f32 xc;
        f32 xs;
        f32 yc;
        f32 ys;
        f32 csx;
        f32 ssx;
        f32 csy;
        f32 ssy;
        f32 ox;
        f32 oy;
        f32 a;
        f32 b;
        f32 c;
        f32 d;
        f32 e;
        f32 f;
        f32 g;
        f32 h;

        VEC3_ctor(&center);
        VEC3_ctor(&pos);
        VEC3_ctor(&edge_a);
        VEC3_ctor(&edge_b);
        mtx34_mult_vec3(&center, mtx, (const VEC3*)&p->world_pos);
        px = pivot->x;
        py = pivot->y;
        xc = axis_x * cos_roll;
        xs = axis_x * sin_roll;
        yc = axis_y * cos_roll;
        ys = axis_y * sin_roll;
        ef_sin_cos(&sin_z, &cos_z, -rotate.z);
        csx = cos_z * scale_x;
        ssx = sin_z * scale_x;
        csy = cos_z * scale_y;
        ssy = sin_z * scale_y;
        ox = px - csx * px - ssy * py;
        oy = py + ssx * px - csy * py;
        pos.x = center.x + (xc * ox + ys * oy);
        pos.y = center.y + (xs * ox - yc * oy);
        pos.z = center.z;
        a = xc * csx;
        b = xc * ssy;
        c = ys * ssx;
        d = ys * csy;
        e = xs * csx;
        f = xs * ssy;
        g = yc * ssx;
        h = yc * csy;
        edge_a.x = a - b - c - d;
        edge_a.y = h + (g + (e - f));
        edge_a.z = 0.0f;
        edge_b.x = d + (a + b - c);
        edge_b.y = (g + (e + f)) - h;
        edge_b.z = 0.0f;
        ef_billboard_write_quad(self, &pos, (Vec*)&edge_a, (Vec*)&edge_b, flags);
    } else {
        VEC3* out;

        VEC3_ctor(&corner[0]);
        VEC3_ctor(&corner[1]);
        VEC3_ctor(&corner[2]);
        VEC3_ctor(&corner[3]);
        out = corner;
        ef_billboard_quad_corners(out, p, mtx, pivot, axis_x, axis_y, cos_roll, sin_roll, scale_x, scale_y);
        GXBegin(0x80, 0, 4);
        ef_billboard_gx_position((Vec*)out);
        if (ef_billboard_tex_flag(flags) != 0) {
            ef_billboard_gx_u8(0);
        }
        ef_billboard_gx_position((Vec*)&corner[1]);
        if (ef_billboard_tex_flag(flags) != 0) {
            ef_billboard_gx_u8(1);
        }
        ef_billboard_gx_position((Vec*)&corner[2]);
        if (ef_billboard_tex_flag(flags) != 0) {
            ef_billboard_gx_u8(2);
        }
        ef_billboard_gx_position((Vec*)&corner[3]);
        if (ef_billboard_tex_flag(flags) != 0) {
            ef_billboard_gx_u8(3);
        }
        ef_billboard_gx_end();
    }
}

/* 0x800BA854 (0x3DC): draws every live particle as a quad that turns about its Y axis toward the view. */
void ef_billboard_draw_y(nw4r::ef::DrawBillboardStrategy* self, const EfDrawInfo* info, EfDrawParticleManager* pm) {
    MTX34 view;
    MTX34 mtx;
    EfVec2 pivot;
    EfEmitterDrawSetting* ed;
    u32 use_tex;
    f32 axis_x;
    f32 axis_y;
    f32 unused;
    f32 length;
    f32 view_y;
    f32 view_z;
    nw4r::ef::DrawStrategyImpl::GetFirstDrawParticleFunc get_first;
    nw4r::ef::DrawStrategyImpl::GetNextDrawParticleFunc get_next;
    bool first;
    EfDrawParticle* p;

    if (!IsValidPointer((u32)pm)) {
        nw4r::db::Panic(ef_billboard_file_str, 593, ef_billboard_pm_assert_str, pm);
    }
    MTX34_ctor(&view);
    ef_draw_info_view_mtx(info, &view);
    ed = (EfEmitterDrawSetting*)ef_resource_draw_setting(pm->resource);
    ef_billboard_setup_gx(self, info, pm);
    use_tex = self->mNumTexmap != 0;
    ef_billboard_vec2_set((f32*)&pivot, ed->scale_a / ef_billboard_percent, ed->scale_b / ef_billboard_percent);
    MTX34_ctor(&mtx);
    ef_pm_get_mtx(pm, &mtx);
    mtx34_concat(&mtx, &view, &mtx);
    axis_x = ef_mtx34_column_length((const f32*)&mtx, 0);
    axis_y = ef_mtx34_column_length((const f32*)&mtx, 1);
    unused = ef_billboard_zero;
    length = sqrt_f32(view.m[1][1] * view.m[1][1] + view.m[2][1] * view.m[2][1]);
    if (ef_billboard_zero == length) {
        view_y = ef_billboard_zero;
        view_z = ef_billboard_one;
    } else {
        f32 inv = math_reciprocal(length);
        view_z = view.m[2][1] * inv;
        view_y = view.m[1][1] * inv;
    }
    get_first = self->GetGetFirstDrawParticleFunc(ed->flags & 0x800);
    get_next = self->GetGetNextDrawParticleFunc(ed->flags & 0x800);
    first = true;
    for (p = get_first(pm); p != NULL; p = get_next(pm, p)) {
        f32 scale_x;
        f32 scale_y;

        if (ef_get_life_status(p) != 1) {
            continue;
        }
        scale_x = ef_particle_get_scale((EfParticle*)p);
        if (scale_x < ef_float_epsilon()) {
            continue;
        }
        scale_y = ef_particle_get_scale_y((EfParticle*)p);
        if (scale_y < ef_float_epsilon()) {
            continue;
        }
        self->SetupGP(p, *ed, *info, first, false);
        first = false;
        ef_billboard_y_quad(self, p, &mtx, axis_x, axis_y, unused, view_y, view_z, scale_x, scale_y, &pivot, use_tex);
    }
}

/* 0x800BAC30 (0x380): writes one particle's Y-billboard quad around its transformed position, turned by the
 * particle's own roll when it has one. */
void ef_billboard_y_quad(nw4r::ef::DrawBillboardStrategy* self, EfDrawParticle* p, const MTX34* mtx, f32 axis_x,
                         f32 axis_y, f32 unused, f32 view_y, f32 view_z, f32 scale_x, f32 scale_y,
                         const EfVec2* pivot, u32 flags) {
    VEC3 rotate;
    VEC3 center;
    VEC3 pos;
    VEC3 edge_a;
    VEC3 edge_b;
    f32 yy;
    f32 yz;
    f32 px;
    f32 py;

    if (!IsValidPointer((u32)p)) {
        nw4r::db::Panic(ef_billboard_y_quad_file_str, 380, ef_billboard_y_quad_particle_str, p);
    }
    VEC3_ctor(&rotate);
    ef_particle_get_rotate(p, (Vec3*)&rotate);
    VEC3_ctor(&center);
    mtx34_mult_vec3(&center, mtx, (const VEC3*)&p->world_pos);
    yy = axis_y * view_y;
    yz = axis_y * view_z;
    px = pivot->x;
    py = pivot->y;
    VEC3_ctor(&pos);
    VEC3_ctor(&edge_a);
    VEC3_ctor(&edge_b);
    if (ef_billboard_zero != rotate.z) {
        f32 sin_z;
        f32 cos_z;
        f32 csx;
        f32 ssx;
        f32 csy;
        f32 ssy;
        f32 ox;
        f32 oy;

        ef_sin_cos(&sin_z, &cos_z, -rotate.z);
        csx = cos_z * scale_x;
        ssx = sin_z * scale_x;
        csy = cos_z * scale_y;
        ssy = sin_z * scale_y;
        oy = -py - ssx * px + csy * py;
        ox = px - csx * px - ssy * py;
        pos.x = center.x + axis_x * ox;
        pos.y = center.y + yy * oy;
        pos.z = center.z + yz * oy;
        edge_a.x = axis_x * (csx - ssy);
        edge_a.y = yy * (ssx + csy);
        edge_a.z = yz * (ssx + csy);
        edge_b.x = axis_x * (csx + ssy);
        edge_b.y = yy * (ssx - csy);
        edge_b.z = yz * (ssx - csy);
    } else {
        f32 oy;
        f32 ex;

        pos.x = center.x + axis_x * (px - px * scale_x);
        oy = py * scale_y - py;
        pos.y = center.y + yy * oy;
        pos.z = center.z + yz * oy;
        ex = axis_x * scale_x;
        edge_a.x = ex;
        edge_a.y = yy * scale_y;
        edge_a.z = yz * scale_y;
        edge_b.x = ex;
        edge_b.y = -yy * scale_y;
        edge_b.z = -yz * scale_y;
    }
    ef_billboard_write_quad(self, &pos, (Vec*)&edge_a, (Vec*)&edge_b, flags);
}

/* 0x800BAFBC (0x4CC): draws every live particle as a quad laid along its ahead vector, stretched by its speed when
 * the draw setting asks for it. */
void ef_billboard_draw_directional(nw4r::ef::DrawBillboardStrategy* self, const EfDrawInfo* info,
                                   EfDrawParticleManager* pm) {
    MTX34 view;
    MTX34 mtx;
    EfVec2 pivot;
    EfEmitterDrawSetting* ed;
    u32 use_tex;
    f32 axis_x;
    f32 axis_y;
    f32 unused;
    nw4r::ef::DrawStrategyImpl::CalcAheadFunc calc_ahead;
    nw4r::ef::DrawStrategyImpl::GetFirstDrawParticleFunc get_first;
    nw4r::ef::DrawStrategyImpl::GetNextDrawParticleFunc get_next;
    bool first;
    EfDrawParticle* p;

    if (!IsValidPointer((u32)pm)) {
        nw4r::db::Panic(ef_billboard_file_str, 662, ef_billboard_pm_assert_str, pm);
    }
    MTX34_ctor(&view);
    ef_draw_info_view_mtx(info, &view);
    ed = (EfEmitterDrawSetting*)ef_resource_draw_setting(pm->resource);
    ef_billboard_setup_gx(self, info, pm);
    use_tex = self->mNumTexmap != 0;
    VEC2_ctor(&pivot);
    pivot.x = ed->scale_a / ef_billboard_percent;
    pivot.y = ed->scale_b / ef_billboard_percent;
    MTX34_ctor(&mtx);
    ef_pm_get_mtx(pm, &mtx);
    mtx34_concat(&mtx, &view, &mtx);
    axis_x = ef_mtx34_column_length((const f32*)&mtx, 0);
    axis_y = ef_mtx34_column_length((const f32*)&mtx, 1);
    unused = ef_billboard_zero;
    nw4r::ef::DrawStrategyImpl::AheadContext ctx(&view, pm);
    calc_ahead = self->GetCalcAheadFunc(pm);
    get_first = self->GetGetFirstDrawParticleFunc(ed->flags & 0x800);
    get_next = self->GetGetNextDrawParticleFunc(ed->flags & 0x800);
    first = true;
    for (p = get_first(pm); p != NULL; p = get_next(pm, p)) {
        f32 scale_x;
        f32 scale_y;
        f32 length;
        f32 cos_dir;
        f32 sin_dir;
        f32 stretch;
        VEC3 ahead;

        if (ef_get_life_status(p) != 1) {
            continue;
        }
        scale_x = ef_particle_get_scale((EfParticle*)p);
        if (scale_x < ef_float_epsilon()) {
            continue;
        }
        scale_y = ef_particle_get_scale_y((EfParticle*)p);
        if (scale_y < ef_float_epsilon()) {
            continue;
        }
        self->SetupGP(p, *ed, *info, first, false);
        first = false;
        VEC3_ctor(&ahead);
        calc_ahead(&ahead, &ctx, p);
        mtx34_rotate_vec3(&ahead, &mtx, &ahead);
        length = sqrt_f32(ahead.x * ahead.x + ahead.y * ahead.y);
        if (0.0f == length) {
            sin_dir = 0.0f;
            cos_dir = ef_billboard_one;
        } else {
            f32 inv = math_reciprocal(length);
            sin_dir = -ahead.x * inv;
            cos_dir = ahead.y * inv;
        }
        stretch = ef_billboard_one;
        if (ed->type_option2 != 0) {
            VEC3 move;

            VEC3_ctor(&move);
            ef_particle_get_move_dir(p, &move);
            stretch += 0.5f * sqrt_f32(vec3_dot((const f32*)&move, (const f32*)&move)) / scale_y;
        }
        ef_billboard_directional_quad(self, p, &mtx, axis_x, axis_y, unused, cos_dir, sin_dir, scale_x, scale_y,
                                      stretch, &pivot, use_tex);
    }
}

/* 0x800BB488 (0x2C0): writes one particle's directional quad around its transformed position, its height
 * stretched by `stretch`. */
void ef_billboard_directional_quad(nw4r::ef::DrawBillboardStrategy* self, EfDrawParticle* p, const MTX34* mtx,
                                   f32 axis_x, f32 axis_y, f32 unused, f32 cos_dir, f32 sin_dir, f32 scale_x,
                                   f32 scale_y, f32 stretch, const EfVec2* pivot, u32 flags) {
    VEC3 center;
    VEC3 pos;
    VEC3 edge_a;
    VEC3 edge_b;
    f32 px;
    f32 py;
    f32 xc;
    f32 xs;
    f32 yc;
    f32 ys;
    f32 ox;
    f32 oy;
    f32 a;
    f32 b;
    f32 c;
    f32 d;

    if (!IsValidPointer((u32)p)) {
        nw4r::db::Panic(ef_billboard_dir_quad_file_str, 458, ef_billboard_dir_quad_particle_str, p);
    }
    VEC3_ctor(&center);
    mtx34_mult_vec3(&center, mtx, (const VEC3*)&p->world_pos);
    px = pivot->x;
    py = pivot->y;
    xc = axis_x * cos_dir;
    xs = axis_x * sin_dir;
    yc = axis_y * cos_dir;
    ys = axis_y * sin_dir;
    ox = px - scale_x * px;
    oy = scale_y * ((stretch + py) - ef_billboard_one) - py;
    setVec3(&pos, center.x + (xc * ox + ys * oy), center.y + (xs * ox - yc * oy), center.z);
    a = xc * scale_x;
    b = xs * scale_x;
    c = stretch * (ys * scale_y);
    d = stretch * (yc * scale_y);
    setVec3(&edge_a, a - c, b + d, ef_billboard_zero);
    setVec3(&edge_b, a + c, b - d, ef_billboard_zero);
    ef_billboard_write_quad(self, &pos, (Vec*)&edge_a, (Vec*)&edge_b, flags);
}

#pragma fp_contract reset

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
extern "C" {
#endif


/* --------------------------------------------------------------------------------------------------
 * The out-of-line GX FIFO writers, in address order.
 * -------------------------------------------------------------------------------------------------- */

/* Ends the current FIFO command (the SDK's `GXEnd`, which writes nothing). */
void ef_billboard_gx_end(void) {}

/* Writes one u8 to the pipe. */
void ef_billboard_gx_u8(u8 value) {
    GXWGFifo.u8 = value;
}

/* Tests the low bit of a status word. */
int ef_billboard_tex_flag(u32 value) {
    return (value & 1) != 0;
}

static void ef_billboard_gx_position3f(f32 x, f32 y, f32 z);

/* Writes a vector to the pipe through the FIFO writer that follows it. */
void ef_billboard_gx_position(Vec* v) {
    f32 x = v->x;
    f32 y = v->y;
    f32 z = v->z;
    ef_billboard_gx_position3f(x, y, z);
}

/* Writes three f32 to the pipe. */
static void ef_billboard_gx_position3f(f32 x, f32 y, f32 z) {
    GXWGFifo.f32 = x;
    GXWGFifo.f32 = y;
    GXWGFifo.f32 = z;
}

/* Writes a pair of f32 to the pipe. */
void ef_billboard_vec2_set(f32* dst, f32 x, f32 y) {
    dst[0] = x;
    dst[1] = y;
}

/* Ends the current FIFO command (second copy). */
void ef_directional_gx_end(void) {}

/* Writes one u8 to the pipe (second copy). */
void ef_directional_gx_u8(u8 value) {
    GXWGFifo.u8 = value;
}

/* Tests the low bit of a status word (second copy). */
int ef_directional_tex_flag(u32 value) {
    return (value & 1) != 0;
}

static void ef_directional_gx_position3f(f32 x, f32 y, f32 z);

/* Writes a vector to the pipe through the FIFO writer that follows it (second copy). */
void ef_directional_gx_position(Vec* v) {
    f32 x = v->x;
    f32 y = v->y;
    f32 z = v->z;
    ef_directional_gx_position3f(x, y, z);
}

/* Writes three f32 to the pipe (second copy). */
static void ef_directional_gx_position3f(f32 x, f32 y, f32 z) {
    GXWGFifo.f32 = x;
    GXWGFifo.f32 = y;
    GXWGFifo.f32 = z;
}

/* --------------------------------------------------------------------------------------------------
 * The indexed four-vertex stripe writers.  They expand a (matrix, two positions) pair into four
 * vertices through `subVec3`/`addVec3` and emit them to the pipe, tagging every other vertex
 * with its index when the draw record asks for it.
 * -------------------------------------------------------------------------------------------------- */

/* SDK GX entry points, declared locally. */

/* The stripe writer (first copy): four vertices from the matrix and two positions. */
void ef_billboard_write_quad(void* unused, void* mtx, Vec* a, Vec* b, u32 flags) {
    Vec v0;
    Vec v1;
    Vec v2;
    Vec v3;

    GXBegin(0x80, 0, 4);
    subVec3(&v0, mtx, a);
    ef_billboard_gx_position(&v0);
    if (ef_billboard_tex_flag(flags) != 0) {
        ef_billboard_gx_u8(0);
    }
    subVec3(&v1, mtx, b);
    ef_billboard_gx_position(&v1);
    if (ef_billboard_tex_flag(flags) != 0) {
        ef_billboard_gx_u8(1);
    }
    addVec3((VEC3*)&v2, (VEC3*)mtx, (VEC3*)a);
    ef_billboard_gx_position(&v2);
    if (ef_billboard_tex_flag(flags) != 0) {
        ef_billboard_gx_u8(2);
    }
    addVec3((VEC3*)&v3, (VEC3*)mtx, (VEC3*)b);
    ef_billboard_gx_position(&v3);
    if (ef_billboard_tex_flag(flags) != 0) {
        ef_billboard_gx_u8(3);
    }
    ef_billboard_gx_end();
}

/* The stripe writer (second copy): four vertices from the matrix and two positions. */
void ef_directional_write_quad_center(void* mtx, Vec* a, Vec* b, u32 flags) {
    Vec v0;
    Vec v1;
    Vec v2;
    Vec v3;

    GXBegin(0x80, 0, 4);
    subVec3(&v0, mtx, a);
    ef_directional_gx_position(&v0);
    if (ef_directional_tex_flag(flags) != 0) {
        ef_directional_gx_u8(0);
    }
    subVec3(&v1, mtx, b);
    ef_directional_gx_position(&v1);
    if (ef_directional_tex_flag(flags) != 0) {
        ef_directional_gx_u8(1);
    }
    addVec3((VEC3*)&v2, (VEC3*)mtx, (VEC3*)a);
    ef_directional_gx_position(&v2);
    if (ef_directional_tex_flag(flags) != 0) {
        ef_directional_gx_u8(2);
    }
    addVec3((VEC3*)&v3, (VEC3*)mtx, (VEC3*)b);
    ef_directional_gx_position(&v3);
    if (ef_directional_tex_flag(flags) != 0) {
        ef_directional_gx_u8(3);
    }
    ef_directional_gx_end();
}

#ifdef __cplusplus
}
#endif

namespace nw4r {
namespace ef {

/* 0x800BB93C (0x280): the ahead-vector builder the draw setting's direction type selects. */
DrawStrategyImpl::CalcAheadFunc DrawBillboardStrategy::GetCalcAheadFunc(EfDrawParticleManager* pm) {
    EfEmitterShape* shape;

    EF_VALID_PTR_ASSERT(ef_billboard_file_str, 784, ef_billboard_resource_assert_str, pm->resource);
    shape = (EfEmitterShape*)ef_resource_draw_setting(pm->resource);
    if (!IsValidPointer((u32)shape)) {
        nw4r::db::Panic(ef_billboard_file_str, 786, ef_billboard_ed_assert_str, shape);
    }
    switch (shape->shape_0xAE) {
    case 0:
        return (CalcAheadFunc)ef_ahead_move_dir;
    case 1:
        return (CalcAheadFunc)ef_ahead_from_emitter;
    case 2:
        return (CalcAheadFunc)ef_ahead_emitter_axis_y;
    case 3:
        return (CalcAheadFunc)ef_billboard_ahead_younger;
    case 4:
        return (CalcAheadFunc)ef_billboard_ahead_neighbours;
    default:
        return (CalcAheadFunc)ef_ahead_move_dir;
    }
}

}  // namespace ef
}  // namespace nw4r

#ifdef __cplusplus
extern "C" {
#endif


/* Resolves the ahead-context position of one particle against its emitter and neighbour. */
void ef_billboard_ahead_younger(Vec* out, EfAheadArgs* args, EfWalkerObj* em) {
    EfWalkerObj* particle = (EfWalkerObj*)ef_pm_next_alive(args->particle, em);

    if (particle != 0) {
        PSVECSubtract((f32*)out, (const f32*)&particle->world_pos, (const f32*)&em->world_pos);
    } else {
        PSVECSubtract((f32*)out, (const f32*)&em->world_pos, (const f32*)&args->pos);
    }
    if (ef_vec3_normalize(out) == 0) {
        copyVec3((nw4r::math::VEC3*)out, &args->prev_pos);
    }
}

/* Resolves the ahead-context position of one particle from its two neighbour particles. */
void ef_billboard_ahead_neighbours(Vec* out, EfAheadArgs* args, EfWalkerObj* em) {
    Vec a;
    Vec b;
    Vec c;
    Vec d;
    EfWalkerObj* first = (EfWalkerObj*)ef_pm_next_alive(args->particle, em);
    EfWalkerObj* second = (EfWalkerObj*)ef_pm_prev_alive(args->particle, em);

    setVec3((nw4r::math::VEC3*)&a, ef_billboard_zero, ef_billboard_zero, ef_billboard_zero);
    if (first != 0) {
        PSVECSubtract((f32*)&a, (const f32*)&first->world_pos, (const f32*)&em->world_pos);
        if (ef_vec3_normalize(&a) == 0) {
            copyVec3((nw4r::math::VEC3*)&a, setVec3((nw4r::math::VEC3*)&c, ef_billboard_zero, ef_billboard_zero,
                                                      ef_billboard_zero));
        }
    }
    setVec3((nw4r::math::VEC3*)&b, ef_billboard_zero, ef_billboard_zero, ef_billboard_zero);
    if (second != 0) {
        PSVECSubtract((f32*)&b, (const f32*)&second->world_pos, (const f32*)&em->world_pos);
        if (ef_vec3_normalize(&b) == 0) {
            copyVec3((nw4r::math::VEC3*)&b, setVec3((nw4r::math::VEC3*)&d, ef_billboard_zero, ef_billboard_zero,
                                                      ef_billboard_zero));
        }
    }
    PSVECSubtract((f32*)out, (const f32*)&a, (const f32*)&b);
    if (ef_vec3_normalize(out) == 0) {
        copyVec3((nw4r::math::VEC3*)out, &args->prev_pos);
    }
}

/* Sets the stripe GX state (texcoord array, vertex format and position matrix). */
void ef_billboard_setup_gx(nw4r::ef::DrawStrategyImpl* self, const EfDrawInfo* em, EfDrawParticleManager* args) {
    Mtx34 mtx;

    (void)(IsValidPointer((u32)args) ||
           (nw4r::db::Panic(ef_billboard_file_str, 746, ef_billboard_pm_assert_str, args), 0));
    self->InitGraphics(args, *(EfEmitterDrawSetting*)ef_resource_draw_setting(args->resource), *em);
    GXEnableTexOffsets(0, 1, 1);
    GXSetArray(0xD, ef_billboard_texcoords, 2);
    GXClearVtxDesc();
    GXSetVtxDesc(9, 1);
    if (self->mNumTexmap != 0) {
        GXSetVtxDesc(0xD, 2);
    }
    GXSetVtxAttrFmt(0, 9, 1, 4, 0);
    GXSetVtxAttrFmt(0, 0xD, 1, 0, 0);
    MTX34_ctor(&mtx);
    mtx34_identity(&mtx);
    GXLoadPosMtxImm(mtx34_get_ptr(&mtx), 0);
    GXSetCurrentMtx(0);
}

/* Sets the directional stripe GX state (texcoord array and vertex format). */
void ef_directional_setup_gx(nw4r::ef::DrawStrategyImpl* self, const EfDrawInfo* em, EfDrawParticleManager* args) {
    (void)(IsValidPointer((u32)args) ||
           (nw4r::db::Panic(ef_directional_file_str, 652, ef_directional_pm_assert_str, args), 0));
    self->InitGraphics(args, *(EfEmitterDrawSetting*)ef_resource_draw_setting(args->resource), *em);
    GXEnableTexOffsets(0, 1, 1);
    GXSetArray(0xD, ef_directional_texcoords, 2);
    GXClearVtxDesc();
    GXSetVtxDesc(9, 1);
    if (self->mNumTexmap != 0) {
        GXSetVtxDesc(0xD, 2);
    }
    GXSetVtxAttrFmt(0, 9, 1, 4, 0);
    GXSetVtxAttrFmt(0, 0xD, 1, 0, 0);
    GXSetCurrentMtx(0);
}

#ifdef __cplusplus
}
#endif

namespace nw4r {
namespace ef {

/* 0x800BBDEC (0x3C): builds the directional strategy. */
DrawDirectionalStrategy::DrawDirectionalStrategy() {}

/* 0x800BC1B4 (0x268): hands the draw to the point or tube path the particle flags select. */
void DrawDirectionalStrategy::Draw(const EfDrawInfo& info, EfDrawParticleManager* pm) {

    if (!IsValidPointer((u32)pm)) {
        nw4r::db::Panic(ef_directional_file_str, 342, ef_directional_pm_assert_str, pm);
    }
    ef_directional_setup_gx(this, &info, pm);
    EF_VALID_PTR_ASSERT(ef_directional_file_str, 346, ef_directional_resource_assert_str, pm->resource);
    if (ef_directional_draw_mode(this, (EfParticleFlags*)ef_resource_draw_setting(pm->resource)) != 1) {
        ef_directional_draw_world(this, &info, pm);
        return;
    }
    ef_directional_draw_view(this, &info, pm);
}

}  // namespace ef
}  // namespace nw4r

#ifdef __cplusplus
extern "C" {
#endif

/* The unit quad in the XY plane and its crossing quad in the YZ plane (`.rodata`). */
static const VEC3 ef_directional_quad_vtx[4] = {
    {-1.0f, -1.0f, 0.0f}, {-1.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 0.0f}, {1.0f, -1.0f, 0.0f}};
static const VEC3 ef_directional_cross_vtx[4] = {
    {0.0f, -1.0f, 1.0f}, {0.0f, 1.0f, 1.0f}, {0.0f, 1.0f, -1.0f}, {0.0f, -1.0f, -1.0f}};

#pragma fp_contract off

/* 0x800BC428 (0x8EC): draws every particle as a quad (two crossed quads for the cross type) in the manager's space,
 * each oriented by its rotation and by a frame built from its ahead vector. */
void ef_directional_draw_world(nw4r::ef::DrawDirectionalStrategy* self, const EfDrawInfo* info,
                               EfDrawParticleManager* pm) {
    MTX34 view;
    EfEmitterDrawSetting* ed;
    nw4r::ef::DrawStrategyImpl::CalcAheadFunc calc_ahead;
    u32 use_tex;
    VEC3 axis_y;
    VEC3 axis_z;
    VEC3 move;
    VEC3 side;
    VEC3 ahead;
    VEC3 normal;
    VEC3 pos;
    EfDrawParticle* head;
    f32 pivot_x;
    f32 pivot_y;
    nw4r::ef::DrawStrategyImpl::GetFirstDrawParticleFunc get_first;
    nw4r::ef::DrawStrategyImpl::GetNextDrawParticleFunc get_next;
    bool first;
    EfDrawParticle* p;

    if (!IsValidPointer((u32)pm)) {
        nw4r::db::Panic(ef_directional_file_str, 365, ef_directional_pm_assert_str, pm);
    }
    ef_directional_setup_gx(self, info, pm);
    EF_VALID_PTR_ASSERT(ef_directional_file_str, 369, ef_directional_resource_assert_str, pm->resource);
    ed = (EfEmitterDrawSetting*)ef_resource_draw_setting(pm->resource);
    if (!IsValidPointer((u32)ed)) {
        nw4r::db::Panic(ef_directional_file_str, 371, ef_directional_ed_assert_str, ed);
    }
    MTX34_ctor(&view);
    ef_draw_info_view_mtx(info, &view);
    nw4r::ef::DrawStrategyImpl::AheadContext ctx(&view, pm);
    calc_ahead = self->GetCalcAheadFunc(pm);
    use_tex = self->mNumTexmap != 0;
    VEC3_ctor(&axis_y);
    VEC3_ctor(&axis_z);
    if (ed->type_direction == 7) {
        MTX34 mtx;
        VEC3 col_y;
        VEC3 col_z;

        MTX34_ctor(&mtx);
        mtx34_concat(&mtx, &view, &ctx.manager_mtx);
        GXLoadPosMtxImm(mtx34_get_ptr(&mtx), 0);
        copyVec3(&axis_y, setVec3(&col_y, ctx.manager_mtx_inv.m[0][1], ctx.manager_mtx_inv.m[1][1],
                                  ctx.manager_mtx_inv.m[2][1]));
        copyVec3(&axis_z, setVec3(&col_z, ctx.manager_mtx_inv.m[0][2], ctx.manager_mtx_inv.m[1][2],
                                  ctx.manager_mtx_inv.m[2][2]));
        ef_vec3_normalize(&axis_z);
    } else {
        MTX34 mtx;
        MTX34 em_mtx;
        VEC3 col_y;
        VEC3 col_z;

        MTX34_ctor(&mtx);
        mtx34_concat(&mtx, &view, &ctx.manager_mtx);
        GXLoadPosMtxImm(mtx34_get_ptr(&mtx), 0);
        EF_VALID_PTR_ASSERT(ef_directional_file_str, 403, ef_directional_manager_em_assert_str, pm->emitter);
        MTX34_ctor(&em_mtx);
        ef_emitter_get_mtx(pm->emitter, &em_mtx);
        mtx34_concat(&em_mtx, &ctx.manager_mtx_inv, &em_mtx);
        copyVec3(&axis_y, setVec3(&col_y, em_mtx.m[0][1], em_mtx.m[1][1], em_mtx.m[2][1]));
        copyVec3(&axis_z, setVec3(&col_z, em_mtx.m[0][2], em_mtx.m[1][2], em_mtx.m[2][2]));
        ef_vec3_normalize(&axis_z);
    }
    for (head = (EfDrawParticle*)ef_pm_list_head((EfParticleState*)pm); head != NULL && head->ahead.x > 1.0f;
         head = (EfDrawParticle*)ef_pm_list_next(pm, head)) {
        copyVec3(&head->ahead, &axis_y);
    }
    pivot_x = ed->scale_a / 100.0f;
    pivot_y = ed->scale_b / 100.0f;
    get_first = self->GetGetFirstDrawParticleFunc(ed->flags & 0x800);
    get_next = self->GetGetNextDrawParticleFunc(ed->flags & 0x800);
    first = true;
    for (p = get_first(pm); p != NULL; p = get_next(pm, p)) {
        f32 scale_x;
        f32 scale_y;
        f32 stretch;
        MTX34 rotate_mtx;
        MTX34 local_mtx;
        MTX34 frame_mtx;
        MTX34 world_mtx;

        scale_x = ef_particle_get_scale((EfParticle*)p);
        if (scale_x < 1.1920929e-7f) {
            continue;
        }
        scale_y = ef_particle_get_scale_y((EfParticle*)p);
        if (scale_y < 1.1920929e-7f) {
            continue;
        }
        stretch = 1.0f;
        if (ed->type_option2 != 0) {
            VEC3_ctor(&move);
            ef_particle_get_move_dir(p, &move);
            stretch += 0.5f * sqrt_f32(vec3_dot((const f32*)&move, (const f32*)&move)) / scale_y;
        }
        self->SetupGP(p, *ed, *info, first, false);
        first = false;
        ef_directional_rotate_mtx(&rotate_mtx, p, ed->type_axis);
        ef_directional_local_mtx(&local_mtx, ed->type_option3, pivot_x, pivot_y, scale_x, scale_y, stretch,
                                 &rotate_mtx);
        VEC3_ctor(&side);
        VEC3_ctor(&ahead);
        VEC3_ctor(&normal);
        calc_ahead(&ahead, &ctx, p);
        vec3_cross((f32*)&normal, (const f32*)&p->ahead, (const f32*)&ahead);
        if (ef_vec3_normalize(&normal) == 0) {
            copyVec3(&normal, &axis_z);
        }
        vec3_cross((f32*)&side, (const f32*)&ahead, (const f32*)&normal);
        copyVec3(&p->ahead, &side);
        assignVec3((Vec*)&pos, (Vec*)&p->world_pos);
        mtx34_set(&frame_mtx, side.x, ahead.x, normal.x, pos.x, side.y, ahead.y, normal.y, pos.y, side.z, ahead.z,
                  normal.z, pos.z);
        MTX34_ctor(&world_mtx);
        mtx34_concat(&world_mtx, &frame_mtx, &local_mtx);
        ef_directional_write_quad(&world_mtx, (Vec*)ef_directional_quad_vtx, use_tex);
        if (ed->type_option == 1) {
            ef_directional_write_quad(&world_mtx, (Vec*)ef_directional_cross_vtx, use_tex);
        }
    }
}

/* 0x800BCD14 (0x184): builds the quad's local matrix: the rotation scaled by the particle's size and stretch and
 * moved so the quad turns about its pivot (on the Y axis, or on the Z axis for pivot mode 1). */
void ef_directional_local_mtx(MTX34* out, u8 pivot_mode, f32 pivot_x, f32 pivot_y, f32 scale_x, f32 scale_y,
                              f32 stretch, const MTX34* rot) {
    if (pivot_mode == 0) {
        f32 tx = scale_x * pivot_x;
        f32 sy = scale_y * stretch;
        f32 ty = scale_y * ((pivot_y + stretch) - 1.0f);

        mtx34_set(out, rot->m[0][0] * scale_x, rot->m[0][1] * sy, rot->m[0][2] * scale_x,
                  pivot_x + (-rot->m[0][0] * tx - rot->m[0][1] * ty), rot->m[1][0] * scale_x, rot->m[1][1] * sy,
                  rot->m[1][2] * scale_x, pivot_y + (-rot->m[1][0] * tx - rot->m[1][1] * ty), rot->m[2][0] * scale_x,
                  rot->m[2][1] * sy, rot->m[2][2] * scale_x, -rot->m[2][0] * tx - rot->m[2][1] * ty);
    } else {
        f32 tx = scale_x * pivot_x;
        f32 tz = scale_y * pivot_y;

        mtx34_set(out, rot->m[0][0] * scale_x, rot->m[0][2] * scale_y, rot->m[0][1] * scale_x,
                  pivot_x + (-rot->m[0][0] * tx - rot->m[0][2] * tz), rot->m[1][0] * scale_x, rot->m[1][2] * scale_y,
                  rot->m[1][1] * scale_x, -rot->m[1][0] * tx - rot->m[1][2] * tz, rot->m[2][0] * scale_x,
                  rot->m[2][2] * scale_y, rot->m[2][1] * scale_x, (-rot->m[2][0] * tx - rot->m[2][2] * tz) - pivot_y);
    }
}

/* 0x800BCE98 (0x39C): builds the rotation matrix of a particle's rotation about the X, Y or Z axis, or all three. */
void ef_directional_rotate_mtx(MTX34* out, EfDrawParticle* p, u8 axis) {
    VEC3 rotate;
    f32 sin_x;
    f32 cos_x;
    f32 sin_y;
    f32 cos_y;
    f32 sin_z;
    f32 cos_z;

    if (!IsValidPointer((u32)p)) {
        nw4r::db::Panic(ef_directional_file_str, 239, ef_directional_pp_assert_str, p);
    }
    VEC3_ctor(&rotate);
    ef_particle_get_rotate(p, (Vec3*)&rotate);
    sin_x = 0.0f;
    cos_x = 1.0f;
    sin_y = 0.0f;
    cos_y = 1.0f;
    sin_z = 0.0f;
    cos_z = 1.0f;
    switch (axis) {
    case 0:
        if (0.0f != rotate.x) {
            ef_sin_cos(&sin_x, &cos_x, -rotate.x);
        }
        mtx34_set(out, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, cos_x, -sin_x, 0.0f, 0.0f, sin_x, cos_x, 0.0f);
        break;
    case 1:
        if (0.0f != rotate.y) {
            ef_sin_cos(&sin_y, &cos_y, -rotate.y);
        }
        mtx34_set(out, cos_y, 0.0f, sin_y, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, -sin_y, 0.0f, cos_y, 0.0f);
        break;
    case 2:
        if (0.0f != rotate.z) {
            ef_sin_cos(&sin_z, &cos_z, -rotate.z);
        }
        mtx34_set(out, cos_z, -sin_z, 0.0f, 0.0f, sin_z, cos_z, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);
        break;
    default: {
        f32 sxsy;
        f32 cxsy;

        if (0.0f != rotate.x) {
            ef_sin_cos(&sin_x, &cos_x, -rotate.x);
        }
        if (0.0f != rotate.y) {
            ef_sin_cos(&sin_y, &cos_y, -rotate.y);
        }
        if (0.0f != rotate.z) {
            ef_sin_cos(&sin_z, &cos_z, -rotate.z);
        }
        sxsy = sin_x * sin_y;
        cxsy = cos_x * sin_y;
        mtx34_set(out, cos_y * cos_z, sxsy * cos_z - cos_x * sin_z, cxsy * cos_z + sin_x * sin_z, 0.0f, cos_y * sin_z,
                  sxsy * sin_z + cos_x * cos_z, cxsy * sin_z - sin_x * cos_z, 0.0f, -sin_y, cos_y * sin_x,
                  cos_x * cos_y, 0.0f);
        break;
    }
    }
}

/* 0x800BD234 (0x92C): draws every particle as a quad (two crossed quads for the cross type) in view space, each
 * oriented by its rotation and by a frame that keeps its ahead vector upright on screen. */
void ef_directional_draw_view(nw4r::ef::DrawDirectionalStrategy* self, const EfDrawInfo* info,
                              EfDrawParticleManager* pm) {
    MTX34 view;
    EfEmitterDrawSetting* ed;
    nw4r::ef::DrawStrategyImpl::CalcAheadFunc calc_ahead;
    u32 use_tex;
    MTX34 mtx;
    MTX34 pm_mtx;
    f32 axis_x;
    f32 axis_y;
    f32 axis_z;
    EfDrawParticle* head;
    f32 pivot_x;
    f32 pivot_y;
    nw4r::ef::DrawStrategyImpl::GetFirstDrawParticleFunc get_first;
    nw4r::ef::DrawStrategyImpl::GetNextDrawParticleFunc get_next;
    bool first;
    EfDrawParticle* p;
    VEC3 move;
    MTX34 rotate_mtx;
    MTX34 local_mtx;
    MTX34 frame_mtx;
    MTX34 world_mtx;
    VEC3 side;
    VEC3 up;
    VEC3 normal;
    VEC3 ahead;
    VEC3 pos;
    VEC3 center;
    VEC3 edge_a;
    VEC3 edge_b;
    VEC3 edge_c;
    VEC3 edge_d;
    VEC3 flat_side;
    VEC3 flat_normal;

    if (!IsValidPointer((u32)pm)) {
        nw4r::db::Panic(ef_directional_file_str, 512, ef_directional_pm_assert_str, pm);
    }
    ef_directional_setup_gx(self, info, pm);
    EF_VALID_PTR_ASSERT(ef_directional_file_str, 516, ef_directional_resource_assert_str, pm->resource);
    ed = (EfEmitterDrawSetting*)ef_resource_draw_setting(pm->resource);
    if (!IsValidPointer((u32)ed)) {
        nw4r::db::Panic(ef_directional_file_str, 518, ef_directional_ed_assert_str, ed);
    }
    MTX34_ctor(&view);
    ef_draw_info_view_mtx(info, &view);
    nw4r::ef::DrawStrategyImpl::AheadContext ctx(&view, pm);
    calc_ahead = self->GetCalcAheadFunc(pm);
    use_tex = self->mNumTexmap != 0;
    MTX34_ctor(&mtx);
    MTX34_ctor(&pm_mtx);
    ef_pm_get_mtx(pm, &pm_mtx);
    mtx34_concat(&mtx, &view, &pm_mtx);
    GXLoadPosMtxImm((void*)mtx34_const_ptr((u32)&ef_identity_mtx), 0);
    axis_x = ef_mtx34_column_length((const f32*)&mtx, 0);
    axis_y = ef_mtx34_column_length((const f32*)&mtx, 1);
    axis_z = ef_mtx34_column_length((const f32*)&mtx, 2);
    for (head = (EfDrawParticle*)ef_pm_list_head((EfParticleState*)pm); head != NULL && head->ahead.x > 1.0f;
         head = (EfDrawParticle*)ef_pm_list_next(pm, head)) {
        copyVec3(&head->ahead, &ef_zero_vec);
    }
    pivot_x = ed->scale_a / 100.0f;
    pivot_y = ed->scale_b / 100.0f;
    get_first = self->GetGetFirstDrawParticleFunc(ed->flags & 0x800);
    get_next = self->GetGetNextDrawParticleFunc(ed->flags & 0x800);
    first = true;
    for (p = get_first(pm); p != NULL; p = get_next(pm, p)) {
        f32 scale_x;
        f32 scale_y;
        f32 stretch;

        scale_x = ef_particle_get_scale((EfParticle*)p);
        if (scale_x < 1.1920929e-7f) {
            continue;
        }
        scale_y = ef_particle_get_scale_y((EfParticle*)p);
        if (scale_y < 1.1920929e-7f) {
            continue;
        }
        stretch = 1.0f;
        if (ed->type_option2 != 0) {
            VEC3_ctor(&move);
            ef_particle_get_move_dir(p, &move);
            stretch += 0.5f * sqrt_f32(vec3_dot((const f32*)&move, (const f32*)&move)) / scale_y;
        }
        self->SetupGP(p, *ed, *info, first, false);
        first = false;
        ef_directional_rotate_mtx(&rotate_mtx, p, ed->type_axis);
        ef_directional_local_mtx(&local_mtx, ed->type_option3, pivot_x, pivot_y, scale_x, scale_y, stretch,
                                 &rotate_mtx);
        VEC3_ctor(&side);
        VEC3_ctor(&up);
        VEC3_ctor(&normal);
        VEC3_ctor(&ahead);
        calc_ahead(&ahead, &ctx, p);
        mtx34_rotate_vec3(&ahead, &mtx, &ahead);
        copyVec3(&up, &ahead);
        ef_vec3_normalize(&up);
        copyVec3(&side, setVec3(&flat_side, ahead.y, -ahead.x, 0.0f));
        if (ef_vec3_normalize(&side) != 0) {
            copyVec3(&normal, setVec3(&flat_normal, -ahead.x * ahead.z, -ahead.y * ahead.z,
                                      ahead.x * ahead.x + ahead.y * ahead.y));
        } else {
            vec3_cross((f32*)&side, (const f32*)&ahead, (const f32*)&p->ahead);
            if (ef_vec3_normalize(&side) == 0) {
                copyVec3(&side, &ef_unit_x_vec);
            }
            vec3_cross((f32*)&normal, (const f32*)&side, (const f32*)&ahead);
        }
        ef_vec3_normalize(&normal);
        copyVec3(&p->ahead, &normal);
        VEC3_ctor(&pos);
        mtx34_mult_vec3(&pos, &mtx, (const VEC3*)&p->world_pos);
        mtx34_set(&frame_mtx, axis_x * side.x, axis_y * up.x, axis_z * normal.x, pos.x, axis_x * side.y,
                  axis_y * up.y, axis_z * normal.y, pos.y, axis_x * side.z, axis_y * up.z, axis_z * normal.z, pos.z);
        MTX34_ctor(&world_mtx);
        mtx34_concat(&world_mtx, &frame_mtx, &local_mtx);
        setVec3(&center, world_mtx.m[0][3], world_mtx.m[1][3], world_mtx.m[2][3]);
        setVec3(&edge_a, world_mtx.m[0][0] + world_mtx.m[0][1], world_mtx.m[1][0] + world_mtx.m[1][1],
                world_mtx.m[2][0] + world_mtx.m[2][1]);
        setVec3(&edge_b, world_mtx.m[0][0] - world_mtx.m[0][1], world_mtx.m[1][0] - world_mtx.m[1][1],
                world_mtx.m[2][0] - world_mtx.m[2][1]);
        ef_directional_write_quad_center(&center, (Vec*)&edge_a, (Vec*)&edge_b, use_tex);
        if (ed->type_option == 1) {
            setVec3(&edge_c, world_mtx.m[0][1] + world_mtx.m[0][2], world_mtx.m[1][1] + world_mtx.m[1][2],
                    world_mtx.m[2][1] + world_mtx.m[2][2]);
            setVec3(&edge_d, world_mtx.m[0][1] - world_mtx.m[0][2], world_mtx.m[1][1] - world_mtx.m[1][2],
                    world_mtx.m[2][1] - world_mtx.m[2][2]);
            ef_directional_write_quad_center(&center, (Vec*)&edge_c, (Vec*)&edge_d, use_tex);
        }
    }
}

#pragma fp_contract reset

/* The four-vertex matrix writer: transforms four positions by a matrix and emits them. */
void ef_directional_write_quad(void* mtx, Vec* src, u32 flags) {
    Vec v0;
    Vec v1;
    Vec v2;
    Vec v3;

    if (!IsValidPointer((u32)src)) {
        nw4r::db::Panic(ef_directional_file_str, 92, ef_directional_p_assert_str, src);
    }
    VEC3_ctor((VEC3*)&v0);
    VEC3_ctor((VEC3*)&v1);
    VEC3_ctor((VEC3*)&v2);
    VEC3_ctor((VEC3*)&v3);
    mtx34_mult_vec3((VEC3*)&v0, (const MTX34*)mtx, (const VEC3*)&src[0]);
    mtx34_mult_vec3((VEC3*)&v1, (const MTX34*)mtx, (const VEC3*)&src[1]);
    mtx34_mult_vec3((VEC3*)&v2, (const MTX34*)mtx, (const VEC3*)&src[2]);
    mtx34_mult_vec3((VEC3*)&v3, (const MTX34*)mtx, (const VEC3*)&src[3]);
    GXBegin(0x80, 0, 4);
    ef_directional_gx_position(&v0);
    if (ef_directional_tex_flag(flags) != 0) {
        ef_directional_gx_u8(0);
    }
    ef_directional_gx_position(&v1);
    if (ef_directional_tex_flag(flags) != 0) {
        ef_directional_gx_u8(1);
    }
    ef_directional_gx_position(&v2);
    if (ef_directional_tex_flag(flags) != 0) {
        ef_directional_gx_u8(2);
    }
    ef_directional_gx_position(&v3);
    if (ef_directional_tex_flag(flags) != 0) {
        ef_directional_gx_u8(3);
    }
    ef_directional_gx_end();
}

#ifdef __cplusplus
}
#endif

namespace nw4r {
namespace ef {

/* 0x800BDD34 (0x388): the ahead-vector builder the draw setting's direction type selects. */
DrawStrategyImpl::CalcAheadFunc DrawDirectionalStrategy::GetCalcAheadFunc(EfDrawParticleManager* pm) {
    EfEmitterShape* shape;

    if (!IsValidPointer((u32)pm)) {
        nw4r::db::Panic(ef_directional_file_str, 687, ef_directional_pm_assert_str, pm);
    }
    EF_VALID_PTR_ASSERT(ef_directional_file_str, 688, ef_directional_resource_assert_str, pm->resource);
    shape = (EfEmitterShape*)ef_resource_draw_setting(pm->resource);
    if (!IsValidPointer((u32)shape)) {
        nw4r::db::Panic(ef_directional_file_str, 691, ef_directional_ed_assert_str, shape);
    }
    switch (shape->shape_0xAE) {
    case 0:
        return (CalcAheadFunc)ef_ahead_move_dir;
    case 1:
        return (CalcAheadFunc)ef_ahead_from_emitter;
    case 2:
        return (CalcAheadFunc)ef_ahead_emitter_axis_y;
    case 3:
        return (CalcAheadFunc)ef_billboard_ahead_younger;
    case 5:
    case 7:
        return (CalcAheadFunc)ef_ahead_manager_axis_y;
    case 6:
        return (CalcAheadFunc)ef_billboard_ahead_neighbours;
    default:
        return (CalcAheadFunc)ef_ahead_move_dir;
    }
}

/* 0x800BE118 (0x3C): the DrawFreeStrategy constructor (the next unit's class; see the unit header). */
DrawFreeStrategy::DrawFreeStrategy() {}

}  // namespace ef
}  // namespace nw4r

#ifdef __cplusplus
extern "C" {
#endif

/* The first two draw/rotate flag bits of a particle. */
s32 ef_directional_draw_mode(void* unused, EfParticleFlags* self) {
    return self->flags_0xB2 & 3;
}

#ifdef __cplusplus
}
#endif

/* The destructors are the classes' inline ones, emitted at the end of this TU; retail calls each base destructor
 * out of line from them (`bl` + `extsh` of the delete flag), which a deferred inline body only gets with inlining
 * off at its point of emission. */
#pragma dont_inline on
