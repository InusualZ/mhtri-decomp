/*
 * ef/ef_drawdirectionalstrategy.cpp - nw4r::ef DrawDirectionalStrategy: its constructor and inline destructor, the
 *   draw dispatch, the world- and view-space walkers with their rotation and local matrices, the four-vertex writers
 *   with their out-of-line GX FIFO writers, the GX setup and the ahead-vector builder.
 * RANGE. .text 0x800BBDEC-0x800BE118 (17 functions); extab 0x8000A2AC-0x8000A304, extabindex 0x800237A8-0x8002382C,
 *   .rodata 0x8056F770-0x8056F7D0, .data 0x80594070-0x805941F8 (the `__FILE__` string "ef_drawdirectionalstrategy.cpp"
 *   first, the class's table last), .sdata 0x80791320-0x80791340, .sdata2 0x80796170-0x80796190.
 * FLAGS. `cflags_main`; `#pragma peephole off` before the includes, `#pragma pool_data off` (each string its own
 *   `lis`/`addi`), `#pragma dont_inline on` at the end (the inline destructor calls its base out of line).
 * NAMES. The class's virtuals carry their manglings; the helpers are `extern "C"`.  GUESS names, from the draw mode
 *   at +0xB2 and the bodies: `ef_directional_draw_world`, `ef_directional_draw_view`, `ef_directional_local_mtx`,
 *   GUESS: `ef_directional_rotate_mtx`, `ef_directional_setup_gx`, `ef_directional_write_quad`,
 *   GUESS: `ef_directional_write_quad_center`, `ef_directional_draw_mode`, `ef_directional_gx_end`,
 *   GUESS: `ef_directional_gx_u8`, `ef_directional_tex_flag`, `ef_directional_gx_position`,
 *   GUESS: `ef_directional_gx_position3f`; the strings `ef_directional_*_str` by their text, the tables
 *   GUESS: `ef_directional_texcoords`, `ef_directional_quad_vtx`, `ef_directional_cross_vtx` by their contents.
 * RESIDUALS. none known.
 * SHAPES. The functions are defined in retail's address order; the strings are global definitions in retail's order.
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
#include "RVLGX/GXTransform.h"
#include "ef/ef_get_life_status.h"     /* ef_get_life_status (rule 2) */
#include "ef/ef_particle_get_move_dir.h" /* ef_particle_get_move_dir (rule 2) */
#include "nw4r/fn_805012C4.h"         /* mtx34_rotate_vec3 (rule 2) */
#include "ef/ef_emitter_tex_flags.h"   /* ef_emitter_get_mtx (rule 2) */
#include "g3d/g3d_draw.h" /* mtx34_set (rule 2) */

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

/* The particle's packed draw/rotate flag bytes (read by the flag accessor). */
typedef struct EfParticleFlags {
    u8 pad_0x00[0xB2]; /* +0x00 */
    u8 flags_0xB2;     /* +0xB2  draw/rotate flag bits */
} EfParticleFlags; /* size: 0xB3 (lower bound, the record continues past what this unit reads) */


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

/* The out-of-line GX FIFO writers (defined after their first user). */
void ef_directional_gx_end(void);
void ef_directional_gx_u8(u8 value);
int ef_directional_tex_flag(u32 value);
void ef_directional_gx_position(Vec* v);


#pragma pool_data off

/* The unit's `.data` strings, in retail order (the class's table follows them). */
char ef_directional_file_str[] = "ef_drawdirectionalstrategy.cpp";
char ef_directional_p_assert_str[] = "NW4R:Pointer Error\np(=%p) is not valid pointer.";
char ef_directional_pm_assert_str[] = "NW4R:Pointer Error\npm(=%p) is not valid pointer.";
char ef_directional_resource_assert_str[] = "NW4R:Pointer Error\npm->mResource(=%p) is not valid pointer.";
char ef_directional_ed_assert_str[] = "NW4R:Pointer Error\n&ed(=%p) is not valid pointer.";
char ef_directional_manager_em_assert_str[] = "NW4R:Pointer Error\npm->mManagerEM(=%p) is not valid pointer.";
char ef_directional_pp_assert_str[] = "NW4R:Pointer Error\npp(=%p) is not valid pointer.";

/* The quads' texture coordinates, (s, t) byte pairs the GX_VA_TEX0 array reads (`.sdata`, 32-byte aligned). */
__declspec(section ".sdata") static u8 ef_directional_texcoords[32] __attribute__((aligned(32))) = {
    0, 1, 0, 0, 1, 0, 1, 1};

/* The unit quad in the XY plane and its crossing quad in the YZ plane (`.rodata`). */
static const VEC3 ef_directional_quad_vtx[4] = {
    {-1.0f, -1.0f, 0.0f}, {-1.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 0.0f}, {1.0f, -1.0f, 0.0f}};
static const VEC3 ef_directional_cross_vtx[4] = {
    {0.0f, -1.0f, 1.0f}, {0.0f, 1.0f, 1.0f}, {0.0f, 1.0f, -1.0f}, {0.0f, -1.0f, -1.0f}};

#ifdef __cplusplus
}
#endif

namespace nw4r {
namespace ef {

/* 0x800BBDEC (0x3C): builds the directional strategy. */
DrawDirectionalStrategy::DrawDirectionalStrategy() {}

}  // namespace ef
}  // namespace nw4r

#ifdef __cplusplus
extern "C" {
#endif

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

/* The first two draw/rotate flag bits of a particle. */
s32 ef_directional_draw_mode(void* unused, EfParticleFlags* self) {
    return self->flags_0xB2 & 3;
}

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
        GXLoadPosMtxImm((const f32(*)[4])mtx34_get_ptr(&mtx), 0);
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
        GXLoadPosMtxImm((const f32(*)[4])mtx34_get_ptr(&mtx), 0);
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
    GXLoadPosMtxImm((const f32(*)[4])mtx34_const_ptr((u32)&ef_identity_mtx), 0);
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

}  // namespace ef
}  // namespace nw4r

/* The destructors are the classes' inline ones, emitted at the end of this TU; retail calls each base destructor
 * out of line from them (`bl` + `extsh` of the delete flag), which a deferred inline body only gets with inlining
 * off at its point of emission. */
#pragma dont_inline on
