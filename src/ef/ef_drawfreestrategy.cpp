/*
 * ef/ef_drawfreestrategy.cpp - nw4r::ef DrawFreeStrategy: the four-vertex quad emitter, the out-of-line GX FIFO
 *   writers, `Draw` (GX array state, then a camera-facing quad and an optional YZ quad per particle), the
 *   billboard and Euler/axis `MTX34` builders, the vertex-descriptor setup, the class's destructor and the
 *   DrawLineStrategy constructor.
 * RANGE. .text 0x800BE154-0x800BEF98 (11 functions); extab 0x8000A30C-0x8000A344, extabindex 0x80023838-0x8002388C,
 *   .rodata 0x8056F7D0-0x8056F830 (the XY and YZ quad corners), .data 0x805941F8-0x80594330 (the `__FILE__` string
 *   "ef_drawfreestrategy.cpp" first), .sdata 0x80791340-0x80791348, .sdata2 0x80796190-0x807961A8.
 *   Seam (playbook 80, measured): every strategy TU runs [constructor, ..., inline destructor], and each constructor
 *   installs the vtable at the end of its own TU's `.data`; so this class's constructor is the tail of `ef/ef_drawbillboardstrategy.cpp`'s range
 *   (0x800BE118) and the DrawLineStrategy constructor at this range's tail (0x800BEF5C) is the next TU's.
 * FLAGS. `cflags_main`; `#pragma peephole off` (retail keeps the unfused `clrlwi`/`extsh`/`extsb` in front of every
 *   narrowing FIFO store and byte load), now before the includes; `#pragma dont_inline on` at the end (the inline
 *   destructor calls its base out of line).
 * NAMES. GUESS: `ef_free_gx_position3f` (the 0x14 FIFO writer at +0x10 of
 *   `ef_free_gx_position`, reached by a tail call).
 *   GUESS (from the bodies, the billboard unit's scheme): `ef_free_gx_position` (hands a vector to the FIFO writer),
 *   GUESS: `ef_free_rotate_mtx` (the axis/Euler rotation from the particle's angles), `ef_free_setup_gx` (the GX setup).
 *   GUESS (the billboard unit's scheme): `ef_free_write_quad`, `ef_free_gx_end`, `ef_free_gx_u8`, `ef_free_tex_flag`,
 *   GUESS: `ef_free_scale_mtx` (the source rows scaled by (u,v,u) with the s/t correction).
 *   The data names spell their strings (`ef_free_file_str`, `ef_free_*_assert_str` after the asserted expression).
 * RESIDUALS. 2 partial rows:
 *  - `Draw` (0x800BE3C0): the three arguments are saved one register lower (r27-r29 against r28-r30) and one
 *    `lwz r6, 0x24(pm)` is scheduled earlier;
 *  - `ef_free_rotate_mtx`: the two products of the 0x24 component (`fmuls f6`, `fmuls f11`) are computed later and the
 *    float registers around them differ.
 *   Relocation names that differ from retail (pool constants - the literals' own pool): `ef_free_hundred`,
 *     `ef_free_epsilon`, `ef_free_int_to_double`, `ef_free_one`, `ef_free_zero`.
 *   flipcheck: `.data` claimed, not emitted; `.rodata` (the quad tables) and `.sdata` (the texcoord table) are
 *   emitted.
 *   `DrawLineStrategy` (constructor): the empty body is the whole source - the compiler
 *     emits the base call and the vtable store.
 * SHAPES. The GX setup's pointer assert is the NW4R expression form `(void)(ok || (Panic(...), 0))`: its value
 *   leaves retail's dead `li r0,0; cmpwi r0,0` after the call (playbook 103).
 */

#pragma peephole off

#include "types.h"
#include "nw4r/math.h"
#include "gx.h"
#include "ef.h"
#include "ef/ef_drawfreestrategy.h"
#include "nw4r/mtx34_mult_vec3.h" /* mtx34_mult_vec3 (rule 2) */
#include "ef/ef_drawlinestrategy.h" /* DrawLineStrategy, whose constructor closes this range */
#include "ef/ef_particle.h"
#include "ef/ef_particlemanager.h"
#include "ef/ef_drawstripestrategy.h" /* ef_draw_info_view_mtx/fn_800B54B4 (rule 2) */
#include "g3d/g3d_calcview.h" /* fn_800710BC (rule 2) */
#include "g3d/fn_80075DCC.h" /* fn_80077DF0, owned by g3d/fn_80075DCC.cpp (rule 2) */
#include "fn_8004CAD8/mtx.h" /* the matrix helpers */

/* `nw4r::db::Panic`, declared in its namespace so the front-end emits the map's mangling
 * (`Panic__Q24nw4r2dbFPCciPCce`). */
namespace nw4r {
namespace db {
void Panic(const char* file, int line, const char* fmt, ...);
}  // namespace db
}  // namespace nw4r

#ifdef __cplusplus
extern "C" {
#endif

#pragma fp_contract off

/* This unit's `.data`/`.rodata`/`.sdata`/`.sdata2` symbols (its claimed ranges), declared, never
 * defined. */
extern char ef_free_file_str[]; /* "ef_drawfreestrategy.cpp"                                 .data 0x805941F8 */
extern char ef_free_p_assert_str[]; /* "NW4R:Pointer Error\np(=%p) is not valid pointer."        .data 0x80594210 */
extern char ef_free_pm_assert_str[]; /* "NW4R:Pointer Error\npm(=%p) is not valid pointer."       .data 0x80594240 */
extern char ef_free_resource_assert_str[]; /* "NW4R:Pointer Error\npm->mResource(=%p) is not valid..."  .data 0x80594274 */
extern char ef_free_ed_assert_str[]; /* "NW4R:Pointer Error\n&ed(=%p) is not valid pointer."      .data 0x805942B0 */
extern char ef_free_pp_assert_str[]; /* "NW4R:Pointer Error\npp(=%p) is not valid pointer."       .data 0x805942E4 */
/* The unit quad in the XY plane and its crossing quad in the YZ plane (`.rodata`). */
static const Vec3 ef_free_quad_vtx[4] = {
    {-1.0f, -1.0f, 0.0f}, {-1.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 0.0f}, {1.0f, -1.0f, 0.0f}};
static const Vec3 ef_free_cross_vtx[4] = {
    {0.0f, -1.0f, 1.0f}, {0.0f, 1.0f, 1.0f}, {0.0f, 1.0f, -1.0f}, {0.0f, -1.0f, -1.0f}};
/* The quads' texture coordinates, (s, t) byte pairs the GX_VA_TEX0 array reads (`.sdata`). */
static u8 ef_free_texcoords[8] __attribute__((aligned(32))) = {0, 1, 0, 0, 1, 0, 1, 1};
extern f32 ef_free_hundred;    /* 100.0f                                                   .sdata2 0x80796190 */
extern f32 ef_free_epsilon;    /* FLT_EPSILON                                              .sdata2 0x80796194 */
extern f64 ef_free_int_to_double;    /* the int->double conversion magic (2^52 + 2^31)           .sdata2 0x80796198 */
extern f32 ef_free_one;    /* 1.0f                                                     .sdata2 0x807961A0 */
extern f32 ef_free_zero;    /* 0.0f                                                     .sdata2 0x807961A4 */

/* The GX entry points this unit calls (the SDK spells them, so they are `extern "C"`). */
extern void GXBegin(u32 prim, u32 vtxfmt, u32 nverts);
extern void GXLoadPosMtxImm(const MTX34* mtx, u32 id);
extern void GXEnableTexOffsets(u32 coord, u32 line, u32 point);
extern void GXSetArray(u32 attr, const void* base, u32 stride);
extern void GXClearVtxDesc(void);
extern void GXSetVtxDesc(u32 attr, u32 type);
extern void GXSetVtxAttrFmt(u32 vtxfmt, u32 attr, u32 cnt, u32 type, u32 frac);
extern void GXSetCurrentMtx(u32 id);

/* nw4r::math / effect-library helpers; the ef callees come from the headers included above. */
extern MTX34* mtx34_get_ptr(MTX34* mtx);                               /* returns its argument (size 0x4) */

/* The effect-particle record this unit walks.  Only the fields the walk reads are named; the record
 * continues past what is touched here.  `EfDrawState` (below) is a *different* view: its +0xAD byte lies
 * where this record stores the low byte of the +0xAC float, so the two are kept apart.
 * size: 0xCC (lower bound) */
typedef struct EfDrawFreeParticle {
    /* +0x00 */ u16 flags;
    /* +0x02 */ u8 pad_0x02[0x2E];
    /* +0x30 */ f32 scale_x;
    /* +0x34 */ u8 pad_0x34[0x04];
    /* +0x38 */ f32 scale_z;
    /* +0x3C */ u8 pad_0x3C[0x04];
    /* +0x40 */ Vec3 pos;
    /* +0x4C */ u8 pad_0x4C[0x4E];
    /* +0x9A */ u8 jitter_x;
    /* +0x9B */ u8 jitter_y;
    /* +0x9C */ u8 jitter_z;
    /* +0x9D */ u8 pad_0x9D[0x0F];
    /* +0xAC */ f32 offset_x;
    /* +0xB0 */ f32 offset_y;
    /* +0xB4 */ f32 offset_z;
    /* +0xB8 */ u8 pad_0xB8[0x10];
    /* +0xC8 */ void* scale_owner;
} EfDrawFreeParticle; /* size: 0xCC - a lower bound (the record continues past what this unit reads) */

/* The draw-state record `ed` returned by ef_resource_draw_setting. size: 0xB0 - a lower bound */
typedef struct EfDrawState {
    /* +0x00 */ u16 flags;
    /* +0x02 */ u8 pad_0x02[0xA7];
    /* +0xA9 */ s8 scale_a;         /* per-100 fade/scale factor (signed) */
    /* +0xAA */ s8 scale_b;
    /* +0xAB */ u8 pad_0xAB[0x02];
    /* +0xAD */ u8 draw_yz_quad;    /* == 1: also emit the YZ-plane quad */
    /* +0xAE */ u8 pad_0xAE;
    /* +0xAF */ u8 drawer_mode;     /* the rotation-axis selector ef_free_rotate_mtx switches on */
} EfDrawState; /* size: 0xB0 - a lower bound (the record continues past what this unit reads) */

/* --------------------------------------------------------------------------------------------- *
 * Forward declarations, so the definitions can sit in address order.
 * --------------------------------------------------------------------------------------------- */
void ef_free_write_quad(const MTX34* mtx, const Vec3* verts, u32 flag);
void ef_free_gx_end(void);
void ef_free_gx_u8(u8 value);
int ef_free_tex_flag(u32 value);
void ef_free_gx_position(const Vec3* v);
void ef_free_scale_mtx(MTX34* dst, const MTX34* m, f32 s, f32 t, f32 u, f32 v);
void ef_free_rotate_mtx(MTX34* dst, const Vec3* pp, u8 mode);
void ef_free_setup_gx(nw4r::ef::DrawFreeStrategy* self, const EfDrawInfo* info, EfDrawParticleManager* pm);

/* --------------------------------------------------------------------------------------------- *
 * 0x800BE154 - the four-vertex emitter.
 * --------------------------------------------------------------------------------------------- */

/* Transforms four input vectors through `mtx` and writes them as a GX_QUADS primitive; when `flag` is
 * set each corner is preceded by its 1-byte index.  The guard is the file's `__FILE__` assert. */
void ef_free_write_quad(const MTX34* mtx, const Vec3* verts, u32 flag) {
    Vec3 v0, v1, v2, v3;

    if (!IsValidPointer((u32)verts))
        nw4r::db::Panic(ef_free_file_str, 88, ef_free_p_assert_str, verts);

    VEC3_ctor(&v0);
    VEC3_ctor(&v1);
    VEC3_ctor(&v2);
    VEC3_ctor(&v3);
    mtx34_mult_vec3(&v0, mtx, &verts[0]);
    mtx34_mult_vec3(&v1, mtx, &verts[1]);
    mtx34_mult_vec3(&v2, mtx, &verts[2]);
    mtx34_mult_vec3(&v3, mtx, &verts[3]);

    GXBegin(0x80, 0, 4);
    ef_free_gx_position(&v0);
    if (ef_free_tex_flag(flag)) {
        ef_free_gx_u8(0);
    }
    ef_free_gx_position(&v1);
    if (ef_free_tex_flag(flag)) {
        ef_free_gx_u8(1);
    }
    ef_free_gx_position(&v2);
    if (ef_free_tex_flag(flag)) {
        ef_free_gx_u8(2);
    }
    ef_free_gx_position(&v3);
    if (ef_free_tex_flag(flag)) {
        ef_free_gx_u8(3);
    }
    ef_free_gx_end();
}

/* --------------------------------------------------------------------------------------------- *
 * 0x800BE374..0x800BE39C - the out-of-line GX FIFO writers and predicates.
 * --------------------------------------------------------------------------------------------- */

/* Ends the current FIFO command (the SDK's `GXEnd`, which writes nothing). */
void ef_free_gx_end(void) {}

/* Writes one u8 to the pipe. */
void ef_free_gx_u8(u8 value) {
    GXWGFifo.u8 = value;
}

/* Tests the low bit of a status word. */
int ef_free_tex_flag(u32 value) {
    return (value & 1) != 0;
}

static void ef_free_gx_position3f(f32 x, f32 y, f32 z);

/* Writes one vector's three components to the pipe through the FIFO writer that follows it. */
void ef_free_gx_position(const Vec3* v) {
    f32 x = v->x;
    f32 y = v->y;
    f32 z = v->z;
    ef_free_gx_position3f(x, y, z);
}

/* Writes three f32 to the pipe. */
static void ef_free_gx_position3f(f32 x, f32 y, f32 z) {
    GXWGFifo.f32 = x;
    GXWGFifo.f32 = y;
    GXWGFifo.f32 = z;
}

#ifdef __cplusplus
}
#endif

namespace nw4r {
namespace ef {

/* 0x800BE3C0 (0x578): draws each particle as a quad under its own rotation, plus a crossing quad for draw types
 * that ask for one. */
void DrawFreeStrategy::Draw(const EfDrawInfo& info, EfDrawParticleManager* pm) {
    EfDrawState* ed;
    EfDrawParticle* p;
    GetFirstDrawParticleFunc first;
    GetNextDrawParticleFunc next;
    MTX34 mtx_view, mtx_mgr;
    u32 flag;
    bool first_pass;
    f32 scale_a, scale_b, scale_x, scale_z;

    if (!IsValidPointer((u32)pm))
        nw4r::db::Panic(ef_free_file_str, 200, ef_free_pm_assert_str, pm);

    ef_free_setup_gx(this, &info, pm);

    if (!IsValidPointer((u32)pm->resource))
        nw4r::db::Panic(ef_free_file_str, 204, ef_free_resource_assert_str, pm->resource);

    ed = (EfDrawState*)ef_resource_draw_setting(pm->resource);

    if (!IsValidPointer((u32)ed))
        nw4r::db::Panic(ef_free_file_str, 206, ef_free_ed_assert_str, ed);

    flag = (mNumTexmap != 0);

    MTX34_ctor(&mtx_view);
    ef_pm_get_mtx(pm, &mtx_view);
    MTX34_ctor(&mtx_mgr);
    ef_draw_info_view_mtx(&info, &mtx_mgr);
    mtx34_concat(&mtx_mgr, &mtx_mgr, &mtx_view);
    GXLoadPosMtxImm(mtx34_get_ptr(&mtx_mgr), 0);

    scale_a = (f32)ed->scale_a / 100.0f;
    scale_b = (f32)ed->scale_b / 100.0f;

    first = GetGetFirstDrawParticleFunc(ed->flags & 0x800);
    next = GetGetNextDrawParticleFunc(ed->flags & 0x800);

    first_pass = true;
    for (p = first(pm); p != NULL; p = next(pm, p)) {
        MTX34 mtx_rot;
        MTX34 mtx_final;

        scale_x = ef_particle_get_scale((EfParticle*)p);
        if (scale_x < 1.1920929e-7f) {
            continue;
        }
        scale_z = ef_particle_get_scale_y((EfParticle*)p);
        if (scale_z < 1.1920929e-7f) {
            continue;
        }
        SetupGP(p, *(EfEmitterDrawSetting*)ed, info, first_pass, false);
        first_pass = false;

        ef_free_rotate_mtx(&mtx_rot, (const Vec3*)&((EfDrawFreeParticle*)p)->pos, ed->drawer_mode);
        ef_free_scale_mtx(&mtx_final, &mtx_rot, scale_a, scale_b, scale_x, scale_z);

        mtx_final.m[0][3] += ((EfDrawFreeParticle*)p)->offset_x;
        mtx_final.m[1][3] += ((EfDrawFreeParticle*)p)->offset_y;
        mtx_final.m[2][3] += ((EfDrawFreeParticle*)p)->offset_z;

        ef_free_write_quad(&mtx_final, (Vec3*)ef_free_quad_vtx, flag);
        if (ed->draw_yz_quad == 1) {
            ef_free_write_quad(&mtx_final, (Vec3*)ef_free_cross_vtx, flag);
        }
    }
}

}  // namespace ef
}  // namespace nw4r

#ifdef __cplusplus
extern "C" {
#endif

/* --------------------------------------------------------------------------------------------- *
 * 0x800BE938 - the rotation/billboard matrix builder.
 * --------------------------------------------------------------------------------------------- */

/* Builds an `MTX34` through `mtx34_set` from the source rows scaled by (u,v,u), with an `s`/`t`
 * correction in the trailing column of rows 0 and 1. */
void ef_free_scale_mtx(MTX34* dst, const MTX34* m, f32 s, f32 t, f32 u, f32 v) {
    f32 us = u * s;
    f32 vt = v * t;

    mtx34_set(dst,
                m->m[0][0] * u,
                m->m[0][1] * v,
                m->m[0][2] * u,
                s + (-m->m[0][0] * us - m->m[0][1] * vt),
                m->m[1][0] * u,
                m->m[1][1] * v,
                m->m[1][2] * u,
                t + (-m->m[1][0] * us - m->m[1][1] * vt),
                m->m[2][0] * u,
                m->m[2][1] * v,
                m->m[2][2] * u,
                -m->m[2][0] * us - m->m[2][1] * vt);
}

/* --------------------------------------------------------------------------------------------- *
 * 0x800BEA00 - the Euler/axis rotation builder.
 * --------------------------------------------------------------------------------------------- */

/* Builds an `MTX34` rotation from the particle's angle vector `pp`: mode 0 rotates about X, 1 about Y,
 * 2 about Z and any other value composes the full Euler rotation.  `ef_sin_cos` is the sin/cos pair. */
void ef_free_rotate_mtx(MTX34* dst, const Vec3* pp, u8 mode) {
    Vec3 v;
    f32 sx, cx, sy, cy, sz, cz;

    if (!IsValidPointer((u32)pp))
        nw4r::db::Panic(ef_free_file_str, 136, ef_free_pp_assert_str, pp);

    VEC3_ctor(&v);
    ef_particle_get_rotate((const void*)pp, &v);

    switch (mode) {
    case 0:
        ef_sin_cos(&sx, &cx, v.x);
        mtx34_set(dst, 1.0f, 0.0f, 0.0f, 0.0f,
                    0.0f, cx, -sx, 0.0f,
                    0.0f, sx, cx, 0.0f);
        break;
    case 1:
        ef_sin_cos(&sy, &cy, v.y);
        mtx34_set(dst, cy, 0.0f, sy, 0.0f,
                    0.0f, 1.0f, 0.0f, 0.0f,
                    -sy, 0.0f, cy, 0.0f);
        break;
    case 2:
        ef_sin_cos(&sz, &cz, v.z);
        mtx34_set(dst, cz, -sz, 0.0f, 0.0f,
                    sz, cz, 0.0f, 0.0f,
                    0.0f, 0.0f, 1.0f, 0.0f);
        break;
    default:
        ef_sin_cos(&sx, &cx, v.x);
        ef_sin_cos(&sy, &cy, v.y);
        ef_sin_cos(&sz, &cz, v.z);
        mtx34_set(dst,
                    cy * cz, sx * sy * cz - cx * sz, cx * sy * cz + sx * sz, 0.0f,
                    cy * sz, sx * sy * sz + cx * cz, cx * sy * sz - sx * cz, 0.0f,
                    -sy, cy * sx, cx * cy, 0.0f);
        break;
    }
}

/* --------------------------------------------------------------------------------------------- *
 * 0x800BED2C - the GX vertex-descriptor setup.
 * --------------------------------------------------------------------------------------------- */

/* Sets the free draw's vertex format: the position array, the indexed texcoord array when the state's
 * +0xD0 byte is set, the two attribute formats and the current matrix. */
void ef_free_setup_gx(nw4r::ef::DrawFreeStrategy* self, const EfDrawInfo* info, EfDrawParticleManager* pm) {
    EfEmitterDrawSetting* ed;

    (void)(IsValidPointer((u32)pm) || (nw4r::db::Panic(ef_free_file_str, 278, ef_free_pm_assert_str, pm), 0));

    ed = (EfEmitterDrawSetting*)ef_resource_draw_setting(pm->resource);
    self->InitGraphics(pm, *ed, *info);

    GXEnableTexOffsets(0, 1, 1);
    GXSetArray(0x0D, ef_free_texcoords, 2);
    GXClearVtxDesc();
    GXSetVtxDesc(9, 1);
    if (self->mNumTexmap) {
        GXSetVtxDesc(0x0D, 2);
    }
    GXSetVtxAttrFmt(0, 9, 1, 4, 0);
    GXSetVtxAttrFmt(0, 0x0D, 1, 0, 0);
    GXSetCurrentMtx(0);
}

#ifdef __cplusplus
}
#endif

namespace nw4r {
namespace ef {

/* 0x800BEF5C (0x3C): the DrawLineStrategy constructor (the next unit's class; see the unit header). */
DrawLineStrategy::DrawLineStrategy() {}

}  // namespace ef
}  // namespace nw4r

/* The destructors are the classes' inline ones, emitted at the end of this TU; retail calls each base destructor
 * out of line from them (`bl` + `extsh` of the delete flag), which a deferred inline body only gets with inlining
 * off at its point of emission. */
#pragma dont_inline on
