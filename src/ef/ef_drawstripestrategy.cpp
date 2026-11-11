/*
 * ef/ef_drawstripestrategy.cpp - nw4r::ef DrawStripeStrategy and the inline helpers its TU emits first: the class's
 *   constructor, the out-of-line DrawStrategyImpl/DrawStrategy destructors, `Draw`, its ahead context, the
 *   `Particle`/`ParticleManager` walkers, the draw-time particle copies and ahead-vector builders the billboard
 *   dispatch hands out, and the class's destructor, which closes the range.
 * RANGE. .text 0x800B4AC8-0x800B9A44 (61 functions); extab 0x8000A10C-0x8000A23C, extabindex 0x80023538-0x80023700,
 *   .data 0x805939E8-0x80593E88 (the `__FILE__` string "ef_drawstripestrategy.cpp" first, copies at 0x80593D0C,
 *   0x80593D5C and 0x80593DAC), .sdata2 0x80796120-0x80796150.  Left edge: `ef/ef_resource.cpp`
 *   ends there; right edge: `ef/ef_drawbillboardstrategy.cpp` starts there.
 * FLAGS. `cflags_main`; file-wide `#pragma peephole off`, `#pragma pool_data off` (each string gets its own
 *   `lis`/`addi`), `#pragma fp_contract off` and `#pragma dont_inline on` before the includes (the base destructors
 *   the constructor's unwind entry emits call their bases out of line, as retail's do); inlining comes back on after
 *   `ef_stripe_noop`, is off around `ef_stripe_draw_count` (it calls `GetNumActive` out of line) and goes off again at
 *   the end (the class's own destructor).
 * ORDER. Retail emits every helper as a deferred inline right after the function that first references it, the
 *   pending references popping last-in first-out; only `ef_stripe_draw_ribbon`, the tube builders, `Draw`,
 *   `ef_stripe_setup_gx`, `ef_stripe_first_ahead`, `GetCalcAheadFunc`, the `AheadContext` constructor and the four
 *   ahead builders are real definitions, in that order.  The helpers are `__declspec(noinline) inline`, defined before
 *   their first caller; the weak string pools they carry land in reverse definition order, so the six that own strings
 *   are defined in retail's reverse data order (view_mtx, modify_rotate, get_rotate, draw_segment, scale_mtx,
 *   ribbon_vertex).
 * NAMES. GUESS: `ef_vec3_normalize`, `ef_vec3_transform`, `ef_draw_info_view_mtx`, `ef_particle_get_rotate`,
 *   `ef_pm_modify_rotate`, the `ef_draw_setting_*` accessors and the `ef_stripe_*` builders from their bodies and
 *   from how `Draw`, `GetCalcAheadFunc` and the tube select them; `AheadContext` (0x800B8888) from the base context
 *   it extends.
 *   GUESS (from the body and its callers): `ef_stripe_draw_ribbon`, `ef_stripe_ribbon_vertex`,
 *   GUESS: `ef_stripe_scale_mtx`, `ef_pm_modify_rotate`, `ef_float_epsilon`, `ef_pm_first_alive`,
 *   GUESS: `ef_pm_next_alive`, `ef_pm_list_next`, `ef_pm_list_head`, `ef_stripe_draw_segment`,
 *   GUESS: `ef_stripe_draw_count`, `ef_stripe_tube_ring`, `ef_stripe_tube_strip`, `ef_stripe_tube_open`,
 *   GUESS: `ef_stripe_tube_loop`, `ef_stripe_tube_to_emitter`, `ef_get_unit_z_vec`, `ef_get_unit_x_vec`,
 *   GUESS: `ef_draw_setting_tube_divide`, `ef_stripe_begin_side`, `ef_stripe_setup_gx`, `ef_stripe_draw_tube`,
 *   GUESS: `ef_draw_setting_connect_type`, `ef_stripe_first_ahead`, `ef_ahead_manager_axis_y`,
 *   GUESS: `ef_ahead_emitter_axis_y`, `ef_ahead_from_emitter`, `ef_ahead_move_dir`, `ef_stripe_ahead_type3`,
 *   GUESS: `ef_stripe_ahead_type6`, `ef_stripe_ahead_type6_link1`, `ef_stripe_ahead_type6_link2`,
 *   GUESS: `ef_pm_prev_alive`, `ef_pm_list_prev`, `ef_pm_last_alive`, `ef_pm_list_tail`.
 *   GUESS: `EfDrawParticleList::GetNumActive` (0x800B5B5C, nw4r's `ActivityList::GetNumActive`), the weak copy
 *   the compiler emits after `ef_stripe_draw_count`.
 *   GUESS (from the bodies): `ef_stripe_noop`, `ef_stripe_write_edge_pair`, `ef_gx_write_f32x2`,
 *   GUESS: `ef_gx_write_vec3`, `ef_gx_write_f32x3` (the FIFO writers; the dump's `GXPosition3f32` at 0x800B52BC),
 *   GUESS: `ef_flag_bit0`, `ef_flag_bit3`, `ef_flag_bit4`, `ef_draw_setting_flags_hi2`,
 *   GUESS: `ef_draw_setting_flags_mid3`, `ef_draw_setting_draw_order`, `ef_stripe_param_copy`,
 *   GUESS: `ef_stripe_sample_ctor`; the constants `1.0f`, `0.0f`, `1.1920929e-07f` by value.
 * RESIDUALS. none: 61 rows at 100 %, `.data`, `.sdata2`, `extab` and `extabindex` match the target.  The object also
 *   emits five weak nw4r::math copies (`VEC3`/`MTX34` constructors, `VEC3::operator=`, 0x64 bytes) and their unwind
 *   entry that an earlier link input defines; the link keeps the first definition.
 *   `DrawStripeStrategy` (constructor): the empty body is the whole source - the compiler
 *   emits the base call and the vtable store.
 * SHAPES. `ef_stripe_write_edge_pair` takes the scale before the flags (the caller loads f1 before r5).  `EfStripeParam` (in
 *   `ef_drawstripestrategy.h`, shared with the smooth stripe) is the tube ring (three `VEC3` and the texture
 *   coordinate); its copy is the implicit member-wise assignment (words for the vectors, `lfs`/`stfs` for the
 *   scalar).  The tube builders keep their rings in one local array
 *   (`ring[2]`/`ring[3]`, constructed in index order at ascending offsets), and each vertex position is passed
 *   straight from its expression (`static_cast<const VEC3&>(...)`: a compiler temporary, not a named local).
 * SHAPES. `Draw` picks the ribbon or the tube with a `switch` on the type option (`cmpwi`; an `if` compares
 *   `cmplwi`).
 * SHAPES. `ef_stripe_draw_count` returns `GetNumActive()` as `s32` (both `s32`: the call becomes retail's tail `b`
 *   with no `clrlwi`; a `u16` return adds one to every caller).
 */

#pragma peephole off
#pragma dont_inline on
#pragma pool_data off
#pragma fp_contract off

#include "types.h"
#include "gx.h"
#include "ef.h"
#include "ef/ef_pointer_assert.h" /* EF_VALID_PTR_ASSERT */
#include "ef/ef_drawstrategyimpl.h"
#include "ef/fn_800AEE48.h"
#include "ef/ef_drawstripestrategy.h" /* the ahead-vector builders and EfAheadItem (this unit's own header) */
#include "sys_mem.h"
#include "unsplit/ef.h"
#include "g3d/g3d_anmscn.h" /* vec3_copy_construct, owned by g3d/g3d_anmscn.cpp (rule 2) */
#include "mh3_pad.h" /* VEC3::VEC3() / VEC3::operator= / VEC3(x, y, z) (rule 2) */
#include "ef/ef_particle.h"   /* fn_800AB388 (rule 2) */
#include "RVLGX/GXTransform.h"
#include "RVLGX/GXAttr.h"
#include "RVLGX/GXGeometry.h"
#include "g3d/g3d_calcview.h" /* fn_800710BC (rule 2) */
#include "nw4r/fn_805012C4.h" /* mtx34_rotate_vec3 (rule 2) */
#include "fn_8004CAD8.h"      /* MTX34::MTX34(), fn_80050508, vec3_to_vec (rule 2) */
#include "draw_shape/mtx34_copy.h" /* the matrix copy (rule 2) */
#include "g3d/g3d_draw.h" /* mtx34_set (rule 2) */
#include "g3d/g3d_camera.h" /* sin_cos_deg (rule 2) */
#include "g3d/g3d_anmchr.h"       /* math_reciprocal (rule 2) */
#include "nw4r/mtx34_mult_vec3.h" /* mtx34_mult_vec3 (rule 2) */
#include "g3d/g3d_calcworld.h"    /* VEC3::operator+= (rule 2) */
#include "ef/ef_vec3_normalize_to.h" /* ef_vec3_normalize_to (rule 2) */
#include "ef/ef_get_life_status.h"   /* ef_get_life_status (rule 2) */
#include "g3d/mtx34_inverse.h"       /* mtx34_inverse (rule 2) */



#ifdef __cplusplus
namespace nw4r {
namespace math {
f32 FrSqrt(f32 value);
}  // namespace math
}  // namespace nw4r
extern "C" {
#endif



/* Helpers owned by other units. */
extern void* vec3_to_const_vec(void* arg);
extern void  PSMTXMultVec(const f32* mtx, const void* src, void* dst);
extern void  ef_particle_get_move_dir(void* arg, Vec* v);


/* The per-instance byte offset a node manager carries. */
typedef struct EfNodeManager {
    u8 pad_0x00[0x42]; /* +0x00 */
    u16 field_0x42;    /* +0x42  byte offset from a node base to its link field */
} EfNodeManager; /* size: 0x44 */

/* The unit's own symbols that are used before their definition. */
void ef_stripe_noop(void);
void ef_gx_write_f32x2(f32 x, f32 y);
int  ef_flag_bit0(u32 value);
void ef_gx_write_vec3(Vec* v);
void ef_gx_write_f32x3(f32 x, f32 y, f32 z);
void* ef_pm_list_prev(void* self, void* node);
u32 ef_pm_list_tail(EfParticleState* self);
u32 ef_draw_setting_draw_order(void* ctx, EfParticleState* particle);

/* The ahead-vector builders `GetCalcAheadFunc` hands out (unwritten). */
void ef_stripe_ahead_type3(VEC3* out, nw4r::ef::DrawStrategyImpl::AheadContext* ctx, EfDrawParticle* p);
void ef_stripe_ahead_type6(VEC3* out, nw4r::ef::DrawStrategyImpl::AheadContext* ctx, EfDrawParticle* p);
void ef_stripe_ahead_type6_link1(VEC3* out, nw4r::ef::DrawStrategyImpl::AheadContext* ctx, EfDrawParticle* p);
void ef_stripe_ahead_type6_link2(VEC3* out, nw4r::ef::DrawStrategyImpl::AheadContext* ctx, EfDrawParticle* p);

/* The stripe builders `Draw` hands the particles to (unwritten). */
VEC3 ef_stripe_first_ahead(nw4r::ef::DrawStripeStrategy* self, EfEmitterDrawSetting* ed,
                           nw4r::ef::DrawStripeStrategy::AheadContext* ctx);
s32 ef_draw_setting_flags_mid3(void* ctx, EfParticleState* particle);
void ef_stripe_draw_segment(MTX34* out, nw4r::ef::DrawStripeStrategy* self,
                            nw4r::ef::DrawStripeStrategy::AheadContext* ctx, u32 flags, EfDrawParticle* p,
                            const VEC3* ahead, const VEC3* pos);
void ef_stripe_setup_gx(nw4r::ef::DrawStripeStrategy* self, const EfDrawInfo* info, EfDrawParticleManager* pm);
void ef_stripe_draw_ribbon(nw4r::ef::DrawStripeStrategy* self, nw4r::ef::DrawStripeStrategy::AheadContext* ctx,
                           u32 flags, const VEC3& a, const VEC3& b);
void ef_stripe_ribbon_vertex(nw4r::ef::DrawStripeStrategy* self, nw4r::ef::DrawStripeStrategy::AheadContext* ctx,
                             u32 flags, EfDrawParticle* p, nw4r::ef::DrawStrategyImpl::CalcAheadFunc calc_ahead,
                             const VEC3* pos, const VEC3* a, const VEC3* b, f32 width, f32 t);
void ef_stripe_scale_mtx(MTX34* out, nw4r::ef::DrawStripeStrategy* self, EfDrawParticle* p, f32 width);
void ef_pm_modify_rotate(EfDrawParticleManager* pm, const EfDrawParticle* p, VEC3* rotate);
s32 ef_draw_setting_flags_hi2(void* ctx, EfParticleState* particle);
void ef_stripe_write_edge_pair(Vec* a, Vec* b, f32 scale, u32 flags);
void ef_stripe_draw_tube(nw4r::ef::DrawStripeStrategy* self, nw4r::ef::DrawStripeStrategy::AheadContext* ctx,
                         u32 flags, u32 flush);
void ef_stripe_tube_open(nw4r::ef::DrawStripeStrategy* self, nw4r::ef::DrawStripeStrategy::AheadContext* ctx,
                         u32 flags);
void ef_stripe_tube_loop(nw4r::ef::DrawStripeStrategy* self, nw4r::ef::DrawStripeStrategy::AheadContext* ctx,
                         u32 flags);
void ef_stripe_tube_to_emitter(nw4r::ef::DrawStripeStrategy* self, nw4r::ef::DrawStripeStrategy::AheadContext* ctx,
                               u32 flags);
void ef_stripe_tube_ring(nw4r::ef::DrawStripeStrategy* self, EfStripeParam* out,
                         nw4r::ef::DrawStripeStrategy::AheadContext* ctx, u32 flags, EfDrawParticle* p,
                         nw4r::ef::DrawStrategyImpl::CalcAheadFunc calc_ahead, f32 offset_x, f32 offset_y, f32 t);
void ef_stripe_tube_strip(nw4r::ef::DrawStripeStrategy* self, const EfStripeParam* a, const EfStripeParam* b,
                          u32 flags, s32 divide, const f32* trig);
void ef_stripe_param_copy(EfStripeParam* dst, EfStripeParam* src);
int ef_flag_bit4(u32 value);
s32 ef_draw_setting_tube_divide(void* ctx, EfParticleState* particle);
s32 ef_draw_setting_connect_type(void* ctx, EfParticleState* particle);
void ef_stripe_begin_side(u32 cull_mode, u32 enabled, u32 flags);

#ifdef __cplusplus
}
#endif

namespace nw4r {
namespace ef {

/* 0x800B4AC8 (0x3C): builds the strategy. */
DrawStripeStrategy::DrawStripeStrategy() {}

}  // namespace ef
}  // namespace nw4r

#ifdef __cplusplus
extern "C" {
#endif

/* Nothing to do. */
__declspec(noinline) inline void ef_stripe_noop(void) {}

#pragma dont_inline off

/* Transforms a vector by the matrix the helper pair builds, and returns `dst`. */
__declspec(noinline) inline VEC3* ef_vec3_transform(VEC3* dst, const MTX34* m, const VEC3* v) {
    PSMTXMultVec((const f32*)mtx34_const_ptr((u32)m), vec3_to_const_vec((void*)v), vec3_to_vec(dst));
    return dst;
}

/* 0x800B7DB0 (0x1A8): copies the draw's view matrix into `result` and, for a nonzero depth offset, pushes it along
 * the view direction of the depth origin (perspective) or along view Z (orthographic). */
__declspec(noinline) inline MTX34* ef_draw_info_view_mtx(const EfDrawInfo* info, MTX34* result) {
    if (result == NULL) {
        nw4r::db::Panic("drawinfo.h", 123, "NW4R:Pointer must not be NULL (result)");
    }
    mtx34_copy(result, (MTX34*)&info->view_mtx);
    if (0.0f != info->depth_offset) {
        f32 proj[7];

        GXGetProjectionv(proj);
        switch ((s32)proj[0]) {
        case 0: {
            VEC3 dir;

            ef_vec3_transform(&dir, &info->view_mtx, &info->depth_origin);
            if (ef_vec3_normalize_to(&dir, &dir) != NULL) {
                if (dir.z >= 0.0f) {
                    result->m[0][3] += dir.x * info->depth_offset;
                    result->m[1][3] += dir.y * info->depth_offset;
                    result->m[2][3] += dir.z * info->depth_offset;
                } else {
                    result->m[0][3] -= dir.x * info->depth_offset;
                    result->m[1][3] -= dir.y * info->depth_offset;
                    result->m[2][3] -= dir.z * info->depth_offset;
                }
            } else {
                result->m[2][3] += info->depth_offset;
            }
            break;
        }
        case 1:
            result->m[2][3] += info->depth_offset;
            break;
        }
    }
    return result;
}

/* 0x800B5698 (0x150): adds the manager's rotation offset. */
__declspec(noinline) inline void ef_pm_modify_rotate(EfDrawParticleManager* pm, const EfDrawParticle* p, VEC3* rotate) {
    if (!IsValidPointer((u32)rotate)) {
        nw4r::db::Panic("particlemanager.h", 576, "NW4R:Pointer Error\nbase(=%p) is not valid pointer.", rotate);
    }
    rotate->x += pm->rotate_offset.x;
    rotate->y += pm->rotate_offset.y;
    rotate->z += pm->rotate_offset.z;
}

/* 0x800B54B4 (0x1E4): the particle's rotation plus its per-axis offset steps and its manager's offset. */
__declspec(noinline) inline void ef_particle_get_rotate(const void* src, Vec3* out) {
    const EfDrawParticle* p = (const EfDrawParticle*)src;

    if (!IsValidPointer((u32)out)) {
        nw4r::db::Panic("particle.h", 228, "NW4R:Pointer Error\nrotate(=%p) is not valid pointer.", out);
    }
    *out = p->rotate;
    if (p->rotate_offset[0] != 0) {
        out->x += 0.024543693f * p->rotate_offset[0];
    }
    if (p->rotate_offset[1] != 0) {
        out->y += 0.024543693f * p->rotate_offset[1];
    }
    if (p->rotate_offset[2] != 0) {
        out->z += 0.024543693f * p->rotate_offset[2];
    }
    ef_pm_modify_rotate(p->manager, p, out);
}

/* Writes a pair of f32 to the pipe. */
__declspec(noinline) inline void ef_gx_write_f32x2(f32 x, f32 y) {
    GXWGFifo.f32 = x;
    GXWGFifo.f32 = y;
}

/* Tests the low bit of a status word (the booleanised `(value & 1) != 0`). */
__declspec(noinline) inline int ef_flag_bit0(u32 value) {
    return (value & 1) != 0;
}

/* Writes three f32 to the pipe. */
__declspec(noinline) inline void ef_gx_write_f32x3(f32 x, f32 y, f32 z) {
    GXWGFifo.f32 = x;
    GXWGFifo.f32 = y;
    GXWGFifo.f32 = z;
}

__declspec(noinline) inline void ef_gx_write_vec3(Vec* v) {
    ef_gx_write_f32x3(v->x, v->y, v->z);
}

/* Emits two stripe endpoints, each preceded by its scale when the flag's low bit is set. */
__declspec(noinline) inline void ef_stripe_write_edge_pair(Vec* a, Vec* b, f32 scale, u32 flags) {
    ef_gx_write_vec3(a);
    if (ef_flag_bit0(flags)) {
        ef_gx_write_f32x2(1.0f, scale);
    }
    ef_gx_write_vec3(b);
    if (ef_flag_bit0(flags)) {
        ef_gx_write_f32x2(0.0f, scale);
    }
}

/* A stripe-strategy constant. */
__declspec(noinline) inline f32 ef_float_epsilon(void) {
    return 1.1920929e-07f;
}

/* Resolves the stripe length, returning 0 when it is below the unit's threshold. */
__declspec(noinline) inline int ef_vec3_normalize(void* self) {
    f32 length = vec3_dot((const f32*)self, (const f32*)self);
    if (length < ef_float_epsilon()) {
        return 0;
    }
    vec3_scale_in_place((VEC3*)self, nw4r::math::FrSqrt(length));
    return 1;
}

/* Tests one bit of a status word. */
__declspec(noinline) inline int ef_flag_bit3(u32 value) {
    return (value & 0x8) != 0;
}

/* 0x800B57E8 (0x1FC): the frame of one stripe particle: the side axis is the ahead vector crossed with the previous
 * particle's (or, in view-facing mode, the view's Z axis), the up axis completes the frame and becomes the particle's
 * new ahead vector, and the position is the translation. */
__declspec(noinline) inline void ef_stripe_draw_segment(MTX34* out, nw4r::ef::DrawStripeStrategy* self,
                            nw4r::ef::DrawStripeStrategy::AheadContext* ctx, u32 flags, EfDrawParticle* p,
                            const VEC3* ahead, const VEC3* pos) {
    if (!IsValidPointer((u32)p)) {
        nw4r::db::Panic("ef_drawstripestrategy.cpp", 174, "NW4R:Pointer Error\npp(=%p) is not valid pointer.", p);
    }
    VEC3 side;
    VEC3 up;
    if (ef_flag_bit3(flags) == 0) {
        vec3_cross((f32*)&side, (const f32*)ahead, (const f32*)&p->ahead);
    } else {
        vec3_cross((f32*)&side, (const f32*)ahead, (const f32*)&ctx->view_axis_z);
    }
    if (ef_vec3_normalize(&side) == 0) {
        side = ctx->emitter_axis_x;
    }
    vec3_cross((f32*)&up, (const f32*)&side, (const f32*)ahead);
    ef_vec3_normalize(&up);
    p->ahead = up;
    mtx34_set(out, side.x, ahead->x, up.x, pos->x, side.y, ahead->y, up.y, pos->y, side.z, ahead->z, up.z,
                pos->z);
}

/* 0x800B52D0 (0x1E4): the rotation (about the stripe axis, by the particle's Y rotation) and scale of one ribbon
 * segment, recentred on the strip width. */
__declspec(noinline) inline void ef_stripe_scale_mtx(MTX34* out, nw4r::ef::DrawStripeStrategy* self, EfDrawParticle* p, f32 width) {
    f32 c;
    f32 s;
    f32 size;

    if (!IsValidPointer((u32)p)) {
        nw4r::db::Panic("ef_drawstripestrategy.cpp", 206, "NW4R:Pointer Error\npp(=%p) is not valid pointer.", p);
    }
    VEC3 rotate;
    ef_particle_get_rotate(p, &rotate);
    size = ef_particle_get_scale((EfParticle*)p);
    ef_sin_cos(&s, &c, rotate.y);
    s *= size;
    c *= size;
    mtx34_set(out, c, 0.0f, -s, width - c * width, 0.0f, 1.0f, 0.0f, 0.0f, s, 0.0f, c, -s * width);
}

/* The next list node: the manager's per-instance byte offset into the node.  The offset is a runtime
 * field, so there is no compile-time field name to reach; the byte add is the only spelling. */
__declspec(noinline) inline void* ef_pm_list_next(void* self, void* node) {
    return *(void**)((u8*)node + ((EfNodeManager*)self)->field_0x42);
}

/* Walks the auxiliary list through the per-node offset table until the callback reports 1. */
__declspec(noinline) inline void* ef_pm_next_alive(void* self, void* node) {
    void* next = ef_pm_list_next(self, node);
    while (next != 0 && ef_get_life_status(next) != 1) {
        next = ef_pm_list_next(self, next);
    }
    return next;
}

/* The particle's stored word at +0x3C. */
__declspec(noinline) inline u32 ef_pm_list_head(EfParticleState* self) {
    return self->field_0x3C;
}

/* Walks the particle list at +0x3C until the callback reports 1 (or the end). */
__declspec(noinline) inline void* ef_pm_first_alive(EfDrawList* self) {
    void* node = (void*)ef_pm_list_head((EfParticleState*)self);
    while (node != 0 && ef_get_life_status(node) != 1) {
        node = ef_pm_next_alive(self, node);
    }
    return node;
}

/* 0x800B4FE4 (0x214): the two ribbon vertices of one particle (or, for NULL, of the eldest one at `pos`): its frame
 * times its rotation-and-scale matrix applied to the edge offsets `a`/`b`, with texture coordinate `t`. */
__declspec(noinline) inline void ef_stripe_ribbon_vertex(nw4r::ef::DrawStripeStrategy* self, nw4r::ef::DrawStripeStrategy::AheadContext* ctx,
                             u32 flags, EfDrawParticle* p, nw4r::ef::DrawStrategyImpl::CalcAheadFunc calc_ahead,
                             const VEC3* pos, const VEC3* a, const VEC3* b, f32 width, f32 t) {

    if (!IsValidPointer((u32)ctx)) {
        nw4r::db::Panic("ef_drawstripestrategy.cpp", 236, "NW4R:Pointer Error\naheadContext(=%p) is not valid pointer.", ctx);
    }
    VEC3 ahead;
    MTX34 mtx;
    _MTX34 frame;
    _MTX34 scale;
    if (p == NULL) {
        p = (EfDrawParticle*)ef_pm_first_alive((EfDrawList*)ctx->particle_manager);
    }
    calc_ahead(&ahead, ctx, p);
    ef_stripe_draw_segment((MTX34*)&frame, self, ctx, flags, p, &ahead, pos);
    ef_stripe_scale_mtx((MTX34*)&scale, self, p, width);
    mtx34_concat(&mtx, (MTX34*)&frame, (MTX34*)&scale);
    VEC3 va;
    VEC3 vb;
    mtx34_mult_vec3(&va, &mtx, a);
    mtx34_mult_vec3(&vb, &mtx, b);
    ef_stripe_write_edge_pair((Vec*)&va, (Vec*)&vb, t, flags);
}

/* The particle's packed flags, bits 6-7. */
__declspec(noinline) inline s32 ef_draw_setting_flags_hi2(void* ctx, EfParticleState* particle) {
    return particle->field_0xB2 & 0xC0;
}

#pragma dont_inline on

/* Returns the list's live particle count. */
inline s32 EfDrawParticleList::GetNumActive() const {
    return count;
}

/* 0x800B5B54 (0x8): the number of particles the manager holds. */
__declspec(noinline) inline s32 ef_stripe_draw_count(nw4r::ef::DrawStrategyImpl* self, EfDrawParticleManager* pm) {
    return pm->particles.GetNumActive();
}

#pragma dont_inline off

/* The Z unit vector. */
__declspec(noinline) inline const VEC3* ef_get_unit_z_vec(void) {
    return &ef_unit_z_vec;
}

/* The X unit vector. */
__declspec(noinline) inline const VEC3* ef_get_unit_x_vec(void) {
    return &ef_unit_x_vec;
}

/* Tests one bit of a status word. */
__declspec(noinline) inline int ef_flag_bit4(u32 value) {
    return (value & 0x10) != 0;
}

/* The particle's layer/parameter index byte. */
__declspec(noinline) inline s32 ef_draw_setting_tube_divide(void* ctx, EfParticleState* particle) {
    return particle->field_0xB0;
}

/* Copies a stripe sample. */
__declspec(noinline) inline void ef_stripe_param_copy(EfStripeParam* dst, EfStripeParam* src) {
    *(EfStripeParamImage*)dst = *(const EfStripeParamImage*)src;
}

}  // extern "C"

/* 0x800B6954 (0x40): constructs a tube ring's three vectors. */
__declspec(noinline) inline EfStripeParam::EfStripeParam() {
}

extern "C" {

/* The particle's state bit 0x800. */
__declspec(noinline) inline u32 ef_draw_setting_draw_order(void* ctx, EfParticleState* particle) {
    return particle->flags_0x00 & 0x800;
}

/* Draws eight stripe steps at one scale when the flag is set. */
__declspec(noinline) inline void ef_stripe_begin_side(u32 cull_mode, u32 enabled, u32 flags) {
    GXSetCullMode(cull_mode);
    if (enabled != 0) {
        GXBegin(0x98, 0, 8);
        for (int i = 0; i < 8; i++) {
            ef_gx_write_f32x3(0.0f, 0.0f, 0.0f);
            if (ef_flag_bit0(flags)) {
                ef_gx_write_f32x2(0.0f, 0.0f);
            }
        }
        ef_stripe_noop();
    }
}

/* The particle's packed flags, bits 0-2. */
__declspec(noinline) inline s32 ef_draw_setting_connect_type(void* ctx, EfParticleState* particle) {
    return particle->field_0xB2 & 0x7;
}

/* The particle's packed flags, bits 3-5. */
__declspec(noinline) inline s32 ef_draw_setting_flags_mid3(void* ctx, EfParticleState* particle) {
    return particle->field_0xB2 & 0x38;
}

/* Builds the particle's +0xB0 transform into a local and copies it into `dst`. */
__declspec(noinline) inline void ef_ahead_manager_axis_y(nw4r::math::VEC3* dst, EfParticleState* particle) {
    *dst = nw4r::math::VEC3(*(const nw4r::math::_VEC3*)&particle->field_0xB0);
}

/* Copies the particle's +0x98 block into `dst`. */
__declspec(noinline) inline void ef_ahead_emitter_axis_y(nw4r::math::VEC3* dst, EfParticleState* particle) {
    *dst = *((const nw4r::math::VEC3*)&particle->field_0x98);
}

/* Subtracts the two ahead vectors and copies the reference block when the result is degenerate. */
__declspec(noinline) inline void ef_ahead_from_emitter(Vec* a, EfParticleState* particle, EfAheadItem* item) {
    PSVECSubtract((f32*)a, (const f32*)&item->field_0xAC, (const f32*)&particle->field_0xA4);
    if (ef_vec3_normalize(a) == 0) {
        *(nw4r::math::VEC3*)a = *((const nw4r::math::VEC3*)&particle->field_0x98);
    }
}

/* untyped: opaque handle passed through to ef_particle_get_move_dir */
__declspec(noinline) inline void ef_ahead_move_dir(Vec* a, EfParticleState* particle, void* arg) {
    ef_particle_get_move_dir(arg, a);
    if (ef_vec3_normalize(a) == 0) {
        *(nw4r::math::VEC3*)a = *((const nw4r::math::VEC3*)&particle->field_0x98);
    }
}

__declspec(noinline) inline void* ef_pm_list_prev(void* self, void* node) {
    return *(void**)((u8*)node + ((EfNodeManager*)self)->field_0x42 + 4);
}

/* Walks a node chain through ef_pm_list_prev until the callback reports 1 (or the end). */
__declspec(noinline) inline void* ef_pm_prev_alive(void* self, void* node) {
    void* next = ef_pm_list_prev(self, node);
    while (next != 0 && ef_get_life_status(next) != 1) {
        next = ef_pm_list_prev(self, next);
    }
    return next;
}

/* The particle's stored word at +0x38. */
__declspec(noinline) inline u32 ef_pm_list_tail(EfParticleState* self) {
    return self->field_0x38;
}

/* Walks the +0x38 head through ef_pm_list_prev until the callback reports 1 (or the end). */
__declspec(noinline) inline void* ef_pm_last_alive(EfParticleState* self) {
    void* next = (void*)ef_pm_list_tail(self);
    while (next != 0 && ef_get_life_status(next) != 1) {
        next = ef_pm_list_prev(self, next);
    }
    return next;
}

/* 0x800B4BA4 (0x43C): draws the stripe as one ribbon strip: two vertices per particle (the `a`/`b` edge offsets
 * through each particle's frame), plus a closing particle when the stripe loops and the emitter origin when it
 * connects to the emitter. */
void ef_stripe_draw_ribbon(nw4r::ef::DrawStripeStrategy* self, nw4r::ef::DrawStripeStrategy::AheadContext* ctx,
                           u32 flags, const VEC3& a_ref, const VEC3& b_ref) {
    const VEC3* a = &a_ref;
    const VEC3* b = &b_ref;
    EfDrawParticleManager* pm;
    EfEmitterDrawSetting* ed;
    nw4r::ef::DrawStrategyImpl::CalcAheadFunc calc_ahead;
    u32 order;
    nw4r::ef::DrawStrategyImpl::GetFirstDrawParticleFunc first;
    nw4r::ef::DrawStrategyImpl::GetNextDrawParticleFunc next;
    s32 count;
    f32 width;
    bool loop;
    bool to_emitter;
    f32 step;
    s32 index;
    s32 dir;
    EfDrawParticle* p;

    if (!IsValidPointer((u32)ctx)) {
        nw4r::db::Panic("ef_drawstripestrategy.cpp", 275, "NW4R:Pointer Error\naheadContext(=%p) is not valid pointer.", ctx);
    }
    pm = ctx->particle_manager;
    ed = (EfEmitterDrawSetting*)ef_resource_draw_setting(pm->resource);
    calc_ahead = self->GetCalcAheadFunc(pm);
    order = ed->flags & 0x800;
    first = self->GetGetFirstDrawParticleFunc(order);
    next = self->GetGetNextDrawParticleFunc(order);
    count = ef_stripe_draw_count(self, pm);
    width = 0.01f * ed->scale_a;
    loop = (ed->stripe_connect & 7) == 1;
    to_emitter = (ed->stripe_connect & 7) == 2;
    if (loop || to_emitter) {
        count++;
    }
    GXBegin(0x98, 0, count * 2);
    if (ef_draw_setting_flags_hi2(self, (EfParticleState*)ed) == 0x40) {
        step = 1.0f;
    } else {
        step = math_reciprocal(count - 1);
    }
    if ((ed->flags & 0x800) == 0) {
        index = 0;
        dir = 1;
    } else {
        index = count - 1;
        dir = -1;
    }
    if (to_emitter && order == 0) {
        ef_stripe_ribbon_vertex(self, ctx, flags, NULL, calc_ahead, &ctx->emitter_center, a, b, width, step * index);
        index += dir;
    }
    for (p = first(pm); p != NULL; p = next(pm, p), index += dir) {
        ef_stripe_ribbon_vertex(self, ctx, flags, p, calc_ahead, (const VEC3*)&p->world_pos, a, b, width,
                                step * index);
    }
    if (loop) {
        p = first(pm);
        ef_stripe_ribbon_vertex(self, ctx, flags, p, calc_ahead, (const VEC3*)&p->world_pos, a, b, width,
                                step * index);
    }
    if (to_emitter && order != 0) {
        ef_stripe_ribbon_vertex(self, ctx, flags, NULL, calc_ahead, &ctx->emitter_center, a, b, width, step * index);
    }
    ef_stripe_noop();
}

/* 0x800B5B64 (0x5C8): one ring of a tube: the particle's frame times its rotation-and-scale matrix, kept as a centre
 * and the X/Z axes, with texture coordinate `t`. */
void ef_stripe_tube_ring(nw4r::ef::DrawStripeStrategy* self, EfStripeParam* out,
                         nw4r::ef::DrawStripeStrategy::AheadContext* ctx, u32 flags, EfDrawParticle* p,
                         nw4r::ef::DrawStrategyImpl::CalcAheadFunc calc_ahead, f32 offset_x, f32 offset_y, f32 t) {
    _MTX34 frame;
    _MTX34 local;
    f32 s;
    f32 c;
    f32 scale_x;
    f32 scale_y;

    if (!IsValidPointer((u32)out)) {
        nw4r::db::Panic("ef_drawstripestrategy.cpp", 395, "NW4R:Pointer Error\ndst(=%p) is not valid pointer.", out);
    }
    if (!IsValidPointer((u32)ctx)) {
        nw4r::db::Panic("ef_drawstripestrategy.cpp", 396, "NW4R:Pointer Error\ncontext(=%p) is not valid pointer.", ctx);
    }
    if (!IsValidPointer((u32)p)) {
        nw4r::db::Panic("ef_drawstripestrategy.cpp", 397, "NW4R:Pointer Error\npp(=%p) is not valid pointer.", p);
    }
    if (!IsValidPointer((u32)calc_ahead)) {
        nw4r::db::Panic("ef_drawstripestrategy.cpp", 398, "NW4R:Pointer Error\ncalcAhead(=%p) is not valid pointer.", calc_ahead);
    }
    VEC3 ahead;
    calc_ahead(&ahead, ctx, p);
    ef_stripe_draw_segment((MTX34*)&frame, self, ctx, flags, p, &ahead, (const VEC3*)&p->world_pos);
    VEC3 rotate;
    ef_particle_get_rotate(p, &rotate);
    scale_x = ef_particle_get_scale((EfParticle*)p);
    scale_y = ef_particle_get_scale_y((EfParticle*)p);
    ef_sin_cos(&s, &c, rotate.y);
    mtx34_set((MTX34*)&local, c * scale_x, 0.0f, -s * scale_y,
              (offset_x - offset_x * (c * scale_x)) + offset_y * (s * scale_y), 0.0f, 1.0f,
              0.0f, 0.0f, s * scale_x, 0.0f, c * scale_y,
              (offset_y - offset_x * (s * scale_x)) - offset_y * (c * scale_y));
    MTX34 mtx;
    mtx34_concat(&mtx, (MTX34*)&frame, (MTX34*)&local);
    out->center = VEC3(mtx.m[0][3], mtx.m[1][3], mtx.m[2][3]);
    mtx34_rotate_vec3(&out->side, &mtx, ef_get_unit_x_vec());
    mtx34_rotate_vec3(&out->up, &mtx, ef_get_unit_z_vec());
    out->tex_t = t;
}

/* 0x800B6144 (0x3F0): one tube segment as a strip around the two rings, `divide` sides from the (sin, cos) table;
 * the ring order sets the facing. */
void ef_stripe_tube_strip(nw4r::ef::DrawStripeStrategy* self, const EfStripeParam* a, const EfStripeParam* b,
                          u32 flags, s32 divide, const f32* trig) {
    f32 inv;
    s32 i;

    if (!IsValidPointer((u32)trig)) {
        nw4r::db::Panic("ef_drawstripestrategy.cpp", 441, "NW4R:Pointer Error\ntrigonometric(=%p) is not valid pointer.", trig);
    }
    inv = math_reciprocal(divide);
    GXBegin(0x98, 0, divide * 2 + 2);
    if (ef_flag_bit4(flags)) {
        for (i = 0; i <= divide; trig += 2, i++) {
            f32 s = trig[0];
            f32 c = trig[1];

            ef_gx_write_vec3((Vec*)&static_cast<const VEC3&>(((*(VEC3*)&a->side * c) + (*(VEC3*)&a->up * s)) + *(VEC3*)&a->center));
            if (ef_flag_bit0(flags)) {
                ef_gx_write_f32x2(inv * i, a->tex_t);
            }
            ef_gx_write_vec3((Vec*)&static_cast<const VEC3&>(((*(VEC3*)&b->side * c) + (*(VEC3*)&b->up * s)) + *(VEC3*)&b->center));
            if (ef_flag_bit0(flags)) {
                ef_gx_write_f32x2(inv * i, b->tex_t);
            }
        }
    } else {
        for (i = 0; i <= divide; trig += 2, i++) {
            f32 s = trig[0];
            f32 c = trig[1];

            ef_gx_write_vec3((Vec*)&static_cast<const VEC3&>(((*(VEC3*)&b->side * c) + (*(VEC3*)&b->up * s)) + *(VEC3*)&b->center));
            if (ef_flag_bit0(flags)) {
                ef_gx_write_f32x2(inv * i, b->tex_t);
            }
            ef_gx_write_vec3((Vec*)&static_cast<const VEC3&>(((*(VEC3*)&a->side * c) + (*(VEC3*)&a->up * s)) + *(VEC3*)&a->center));
            if (ef_flag_bit0(flags)) {
                ef_gx_write_f32x2(inv * i, a->tex_t);
            }
        }
    }
    ef_stripe_noop();
}

/* 0x800B6548 (0x3B0): an open tube: one segment between each pair of neighbouring particles. */
void ef_stripe_tube_open(nw4r::ef::DrawStripeStrategy* self, nw4r::ef::DrawStripeStrategy::AheadContext* ctx,
                         u32 flags) {
    EfDrawParticleManager* pm;
    EfEmitterDrawSetting* ed;
    nw4r::ef::DrawStrategyImpl::CalcAheadFunc calc_ahead;
    u32 order;
    nw4r::ef::DrawStrategyImpl::GetFirstDrawParticleFunc first;
    nw4r::ef::DrawStrategyImpl::GetNextDrawParticleFunc next;
    f32 offset_x;
    f32 offset_y;
    s32 count;
    f32 step;
    s32 index;
    s32 dir;
    EfDrawParticle* p;

    if (!IsValidPointer((u32)ctx)) {
        nw4r::db::Panic("ef_drawstripestrategy.cpp", 493, "NW4R:Pointer Error\naheadContext(=%p) is not valid pointer.", ctx);
    }
    pm = ctx->particle_manager;
    ed = (EfEmitterDrawSetting*)ef_resource_draw_setting(pm->resource);
    calc_ahead = self->GetCalcAheadFunc(pm);
    order = ef_draw_setting_draw_order(self, (EfParticleState*)ed);
    first = self->GetGetFirstDrawParticleFunc(order);
    next = self->GetGetNextDrawParticleFunc(order);
    offset_x = 0.01f * ed->scale_a;
    offset_y = 0.01f * ed->scale_b;
    count = ef_stripe_draw_count(self, pm);
    if (ef_draw_setting_flags_hi2(self, (EfParticleState*)ed) == 0x40) {
        step = 1.0f;
    } else {
        step = math_reciprocal(count - 1);
    }
    index = 0;
    dir = 1;
    if (order != 0) {
        index = count - 1;
        dir = -1;
    }
    {
        EfStripeParam ring[2]; /* [0] the previous ring, [1] the current one */

        p = first(pm);
        ef_stripe_tube_ring(self, &ring[1], ctx, flags, p, calc_ahead, offset_x, offset_y, step * index);
        index += dir;
        for (p = next(pm, p); p != NULL; p = next(pm, p), index += dir) {
            ef_stripe_param_copy(&ring[0], &ring[1]);
            ef_stripe_tube_ring(self, &ring[1], ctx, flags, p, calc_ahead, offset_x, offset_y, step * index);
            ef_stripe_tube_strip(self, &ring[0], &ring[1], flags, ef_draw_setting_tube_divide(self, (EfParticleState*)ed),
                                 ctx->trig_table);
        }
    }
}

/* 0x800B69A0 (0x404): a looped tube: the open tube plus a closing segment back to the first particle. */
void ef_stripe_tube_loop(nw4r::ef::DrawStripeStrategy* self, nw4r::ef::DrawStripeStrategy::AheadContext* ctx,
                         u32 flags) {
    EfDrawParticleManager* pm;
    EfEmitterDrawSetting* ed;
    nw4r::ef::DrawStrategyImpl::CalcAheadFunc calc_ahead;
    u32 order;
    nw4r::ef::DrawStrategyImpl::GetFirstDrawParticleFunc first;
    nw4r::ef::DrawStrategyImpl::GetNextDrawParticleFunc next;
    f32 offset_x;
    f32 offset_y;
    s32 count;
    f32 step;
    s32 index;
    s32 dir;
    EfDrawParticle* p;

    if (!IsValidPointer((u32)ctx)) {
        nw4r::db::Panic("ef_drawstripestrategy.cpp", 549, "NW4R:Pointer Error\naheadContext(=%p) is not valid pointer.", ctx);
    }
    pm = ctx->particle_manager;
    ed = (EfEmitterDrawSetting*)ef_resource_draw_setting(pm->resource);
    calc_ahead = self->GetCalcAheadFunc(pm);
    order = ef_draw_setting_draw_order(self, (EfParticleState*)ed);
    first = self->GetGetFirstDrawParticleFunc(order);
    next = self->GetGetNextDrawParticleFunc(order);
    offset_x = 0.01f * ed->scale_a;
    offset_y = 0.01f * ed->scale_b;
    count = ef_stripe_draw_count(self, pm);
    if (ef_draw_setting_flags_hi2(self, (EfParticleState*)ed) == 0x40) {
        step = 1.0f;
    } else {
        step = math_reciprocal(count);
    }
    index = 0;
    dir = 1;
    if (order != 0) {
        index = count;
        dir = -1;
    }
    {
        EfStripeParam ring[3]; /* [0] the first ring, [1] the previous one, [2] the current one */

        p = first(pm);
        ef_stripe_tube_ring(self, &ring[0], ctx, flags, p, calc_ahead, offset_x, offset_y, step * index);
        ef_stripe_param_copy(&ring[2], &ring[0]);
        index += dir;
        for (p = next(pm, p); p != NULL; p = next(pm, p), index += dir) {
            ef_stripe_param_copy(&ring[1], &ring[2]);
            ef_stripe_tube_ring(self, &ring[2], ctx, flags, p, calc_ahead, offset_x, offset_y, step * index);
            ef_stripe_tube_strip(self, &ring[1], &ring[2], flags, ef_draw_setting_tube_divide(self, (EfParticleState*)ed),
                                 ctx->trig_table);
        }
        ring[0].tex_t = step * index;
        ef_stripe_tube_strip(self, &ring[2], &ring[0], flags, ef_draw_setting_tube_divide(self, (EfParticleState*)ed),
                             ctx->trig_table);
    }
}

/* 0x800B6DA4 (0x44C): a tube that also reaches back to the emitter: the first segment runs from the emitter origin
 * (the first ring moved by the youngest particle's offset from it). */
void ef_stripe_tube_to_emitter(nw4r::ef::DrawStripeStrategy* self, nw4r::ef::DrawStripeStrategy::AheadContext* ctx,
                               u32 flags) {
    EfDrawParticleManager* pm;
    EfEmitterDrawSetting* ed;
    nw4r::ef::DrawStrategyImpl::CalcAheadFunc calc_ahead;
    u32 order;
    nw4r::ef::DrawStrategyImpl::GetFirstDrawParticleFunc first;
    nw4r::ef::DrawStrategyImpl::GetNextDrawParticleFunc next;
    f32 offset_x;
    f32 offset_y;
    s32 count;
    f32 step;
    s32 index;
    s32 dir;
    EfDrawParticle* p;

    if (!IsValidPointer((u32)ctx)) {
        nw4r::db::Panic("ef_drawstripestrategy.cpp", 609, "NW4R:Pointer Error\naheadContext(=%p) is not valid pointer.", ctx);
    }
    pm = ctx->particle_manager;
    ed = (EfEmitterDrawSetting*)ef_resource_draw_setting(pm->resource);
    calc_ahead = self->GetCalcAheadFunc(pm);
    order = ef_draw_setting_draw_order(self, (EfParticleState*)ed);
    first = self->GetGetFirstDrawParticleFunc(order);
    next = self->GetGetNextDrawParticleFunc(order);
    offset_x = 0.01f * ed->scale_a;
    offset_y = 0.01f * ed->scale_b;
    count = ef_stripe_draw_count(self, pm);
    if (ef_draw_setting_flags_hi2(self, (EfParticleState*)ed) == 0x40) {
        step = 1.0f;
    } else {
        step = math_reciprocal(count);
    }
    index = 0;
    dir = 1;
    if (order != 0) {
        index = count;
        dir = -1;
    }
    {
        EfStripeParam ring[2]; /* [0] the previous ring, [1] the current one */
        EfDrawParticle* youngest;

        p = first(pm);
        ef_stripe_tube_ring(self, &ring[0], ctx, flags, p, calc_ahead, offset_x, offset_y, step * index);
        index += dir;
        ef_stripe_param_copy(&ring[1], &ring[0]);
        ring[1].tex_t = step * index;
        youngest = (EfDrawParticle*)ef_pm_first_alive((EfDrawList*)pm);
        if (!youngest) {
            nw4r::db::Panic("ef_drawstripestrategy.cpp", 655, "NW4R:Failed assertion youngest");
        }
        VEC3 offset = ctx->emitter_center - (const VEC3&)youngest->world_pos;
        ring[0].center += offset;
        index += dir;
        ef_stripe_tube_strip(self, &ring[0], &ring[1], flags, ef_draw_setting_tube_divide(self, (EfParticleState*)ed),
                             ctx->trig_table);
        for (p = next(pm, p); p != NULL; p = next(pm, p), index += dir) {
            ef_stripe_param_copy(&ring[0], &ring[1]);
            ef_stripe_tube_ring(self, &ring[1], ctx, flags, p, calc_ahead, offset_x, offset_y, step * index);
            ef_stripe_tube_strip(self, &ring[0], &ring[1], flags, ef_draw_setting_tube_divide(self, (EfParticleState*)ed),
                                 ctx->trig_table);
        }
    }
}

/* 0x800B71F0 (0x438): draws the stripe as a tube: builds the (sin, cos) table of its sides on the stack, then draws
 * the back faces and the front faces through the connection type's builder. */
void ef_stripe_draw_tube(nw4r::ef::DrawStripeStrategy* self, nw4r::ef::DrawStripeStrategy::AheadContext* ctx,
                         u32 flags, u32 flush) {
    EfEmitterDrawSetting* ed;
    s32 divide;

    if (!IsValidPointer((u32)ctx)) {
        nw4r::db::Panic("ef_drawstripestrategy.cpp", 679, "NW4R:Pointer Error\naheadContext(=%p) is not valid pointer.", ctx);
    }
    ed = (EfEmitterDrawSetting*)ef_resource_draw_setting(ctx->particle_manager->resource);
    if (!(3 <= ef_draw_setting_tube_divide(self, (EfParticleState*)ed))) {
        nw4r::db::Panic("ef_drawstripestrategy.cpp", 684, "NW4R:Failed assertion 3 <= GetTubeDivide(ed)");
    }
    divide = ef_draw_setting_tube_divide(self, (EfParticleState*)ed);
    if (divide >= 3) {
        f32* trig = (f32*)__alloca((divide + 1) * 8);

        if (!IsValidPointer((u32)trig)) {
            nw4r::db::Panic("ef_drawstripestrategy.cpp", 694, "NW4R:Pointer Error\ntrigonometric(=%p) is not valid pointer.", trig);
        }
        if (trig != NULL) {
            f32 step = 360.0f / divide;
            f32* pair;
            s32 i;

            trig[1] = 1.0f;
            trig[0] = 0.0f;
            for (i = 1, pair = trig + 2; i < divide; pair += 2, i++) {
                sin_cos_deg(&pair[0], &pair[1], step * i);
            }
            trig[divide * 2 + 1] = 1.0f;
            trig[divide * 2] = 0.0f;
            ctx->trig_table = trig;
            switch (ef_draw_setting_connect_type(self, (EfParticleState*)ed)) {
            default:
                ef_stripe_begin_side(1, flush, flags);
                ef_stripe_tube_open(self, ctx, flags);
                ef_stripe_begin_side(2, flush, flags);
                ef_stripe_tube_open(self, ctx, flags);
                break;
            case 1:
                ef_stripe_begin_side(1, flush, flags);
                ef_stripe_tube_loop(self, ctx, flags);
                ef_stripe_begin_side(2, flush, flags);
                ef_stripe_tube_loop(self, ctx, flags);
                break;
            case 2:
                ef_stripe_begin_side(1, flush, flags);
                ef_stripe_tube_to_emitter(self, ctx, flags);
                ef_stripe_begin_side(2, flush, flags);
                ef_stripe_tube_to_emitter(self, ctx, flags);
                break;
            }
        }
    }
}

}  // extern "C"

namespace nw4r {
namespace ef {

/* 0x800B76C4 (0x6EC): draws the manager's particles as one stripe: each particle's ahead vector first, then either
 * one strip per particle (the short connection modes) or the whole stripe, as a ribbon, a cross or a tube. */
void DrawStripeStrategy::Draw(const EfDrawInfo& info, EfDrawParticleManager* pm) {
    EfEmitterDrawSetting* ed;
    s32 count;

    if (!IsValidPointer((u32)pm)) {
        nw4r::db::Panic("ef_drawstripestrategy.cpp", 738, "NW4R:Pointer Error\npm(=%p) is not valid pointer.", pm);
    }
    EF_VALID_PTR_ASSERT("ef_drawstripestrategy.cpp", 740, "NW4R:Pointer Error\npm->mResource(=%p) is not valid pointer.", pm->resource);
    ed = (EfEmitterDrawSetting*)ef_resource_draw_setting(pm->resource);
    if (!IsValidPointer((u32)ed)) {
        nw4r::db::Panic("ef_drawstripestrategy.cpp", 742, "NW4R:Pointer Error\n&ed(=%p) is not valid pointer.", ed);
    }
    count = ef_stripe_draw_count(this, pm);
    if (count != 0) {
        MTX34 view_mtx;
        EfDrawParticle* p;
        u32 flags;

        ef_draw_info_view_mtx(&info, &view_mtx);
        AheadContext ctx(&view_mtx, pm);
        VEC3 ahead = ef_stripe_first_ahead(this, ed, &ctx);
        for (p = (EfDrawParticle*)ef_pm_list_head((EfParticleState*)pm); p != NULL;
             p = (EfDrawParticle*)ef_pm_list_next(pm, p)) {
            if (p->ahead.x <= 1.0f) {
                break;
            }
            p->ahead = ahead;
        }

        flags = (ef_draw_setting_draw_order(this, (EfParticleState*)ed) ? 0x10 : 0) | (ed->type_option == 2 ? 8 : 0);
        MTX34 mtx;
        ef_draw_info_view_mtx(&info, &mtx);
        mtx34_concat(&mtx, &mtx, &ctx.manager_mtx);
        if (ef_flag_bit3(flags)) {
            MTX34 inv;
            if (mtx34_inverse(&inv, &mtx)) {
                ctx.view_axis_z = VEC3(inv.m[0][2], inv.m[1][2], inv.m[2][2]);
            } else {
                ctx.view_axis_z = VEC3(0.0f, 0.0f, 1.0f);
            }
        }

        if (((ed->stripe_connect & 7) == 1 && count < 3) || ((ed->stripe_connect & 7) == 0 && count < 2)) {
            u32 order = ed->flags & 0x800;
            CalcAheadFunc calc_ahead = GetCalcAheadFunc(pm);
            GetFirstDrawParticleFunc first = GetGetFirstDrawParticleFunc(order);
            GetNextDrawParticleFunc next = GetGetNextDrawParticleFunc(order);
            EfDrawParticle* q;

            for (q = first(pm); q != NULL; q = next(pm, q)) {
                VEC3 axis;
                _MTX34 frame;

                calc_ahead(&axis, &ctx, q);
                ef_stripe_draw_segment((MTX34*)&frame, this, &ctx, flags, q, &axis, (VEC3*)&q->world_pos);
            }
            return;
        }

        ef_stripe_setup_gx(this, &info, pm);
        GXLoadPosMtxImm((const f32(*)[4])mtx34_get_ptr(&mtx), 0);
        flags |= (mNumTexmap != 0);
        p = GetGetFirstDrawParticleFunc(ed->flags & 0x800)(pm);
        SetupGP(p, *ed, info, true, false);
        switch (ed->type_option) {
        default: {
            ef_stripe_draw_ribbon(this, &ctx, flags, VEC3(1.0f, 0.0f, 0.0f), VEC3(-1.0f, 0.0f, 0.0f));
            if (ed->type_option == 1) {
                ef_stripe_draw_ribbon(this, &ctx, flags, VEC3(0.0f, 0.0f, -1.0f), VEC3(0.0f, 0.0f, 1.0f));
            }
            break;
        }
        case 3: {
            u32 flush = 0;
            if ((ed->flags & 8) != 0 || p->manager->emitter->effect->system->flush_gp == 0) {
                flush = 1;
            }
            ef_stripe_draw_tube(this, &ctx, flags, flush);
            break;
        }
        }
    }
}

}  // namespace ef
}  // namespace nw4r

extern "C" {

/* 0x800B7FCC (0x1B8): sets the stripe's GX state: the strategy's texture/TEV/channel setup, then a direct position
 * and texcoord vertex format at the current matrix. */
void ef_stripe_setup_gx(nw4r::ef::DrawStripeStrategy* self, const EfDrawInfo* info, EfDrawParticleManager* pm) {
    if (!IsValidPointer((u32)pm)) {
        nw4r::db::Panic("ef_drawstripestrategy.cpp", 849, "NW4R:Pointer Error\npm(=%p) is not valid pointer.", pm);
    }
    self->InitGraphics(pm, *(EfEmitterDrawSetting*)ef_resource_draw_setting(pm->resource), *info);
    GXEnableTexOffsets(0, 1, 1);
    GXClearVtxDesc();
    GXSetVtxDesc(9, 1);
    if (self->mNumTexmap != 0) {
        GXSetVtxDesc(0xD, 1);
    }
    GXSetVtxAttrFmt(0, 9, 1, 4, 0);
    GXSetVtxAttrFmt(0, 0xD, 1, 4, 0);
    GXSetCurrentMtx(0);
}

/* 0x800B8184 (0x23C): the stripe's first ahead vector: for direction type 7 one manager axis (or their sum) by the
 * stripe's axis bits, otherwise the emitter's X or Z axis (or a small diagonal) taken into manager space;
 * normalised, falling back to the emitter's Y axis. */
VEC3 ef_stripe_first_ahead(nw4r::ef::DrawStripeStrategy* self, EfEmitterDrawSetting* ed,
                           nw4r::ef::DrawStripeStrategy::AheadContext* ctx) {
    VEC3 out;

    if (ed->type_direction == 7) {
        switch (ef_draw_setting_flags_mid3(self, (EfParticleState*)ed)) {
        case 8:
            out = VEC3(ctx->manager_mtx_inv.m[0][0], ctx->manager_mtx_inv.m[1][0], ctx->manager_mtx_inv.m[2][0]);
            break;
        default:
            out = VEC3(ctx->manager_mtx_inv.m[0][1], ctx->manager_mtx_inv.m[1][1], ctx->manager_mtx_inv.m[2][1]);
            break;
        case 16:
            out = VEC3(ctx->manager_mtx_inv.m[0][2], ctx->manager_mtx_inv.m[1][2], ctx->manager_mtx_inv.m[2][2]);
            break;
        case 24:
            out = (VEC3(ctx->manager_mtx_inv.m[0][0], ctx->manager_mtx_inv.m[1][0], ctx->manager_mtx_inv.m[2][0]) +
                   VEC3(ctx->manager_mtx_inv.m[0][1], ctx->manager_mtx_inv.m[1][1], ctx->manager_mtx_inv.m[2][1])) +
                  VEC3(ctx->manager_mtx_inv.m[0][2], ctx->manager_mtx_inv.m[1][2], ctx->manager_mtx_inv.m[2][2]);
            break;
        }
    } else {
        switch (ef_draw_setting_flags_mid3(self, (EfParticleState*)ed)) {
        case 8:
            mtx34_rotate_vec3(&out, &ctx->emitter_mtx, ef_get_unit_x_vec());
            mtx34_rotate_vec3(&out, &ctx->manager_mtx_inv, &out);
            break;
        default:
            out = ctx->emitter_axis_y;
            return out;
        case 16:
            mtx34_rotate_vec3(&out, &ctx->emitter_mtx, ef_get_unit_z_vec());
            mtx34_rotate_vec3(&out, &ctx->manager_mtx_inv, &out);
            break;
        case 24:
            VEC3 diagonal(1.0f, 1.0f, 1.0f);
            mtx34_rotate_vec3(&out, &ctx->emitter_mtx, &diagonal);
            mtx34_rotate_vec3(&out, &ctx->manager_mtx_inv, &out);
            break;
        }
    }
    if (ef_vec3_normalize(&out) == 0) {
        out = ctx->emitter_axis_y;
    }
    return out;
}

}  // extern "C"

namespace nw4r {
namespace ef {

/* 0x800B83CC (0x3BC): the ahead-vector builder the draw setting's direction type (and, for type 6, the stripe's
 * connection mode) selects. */
DrawStrategyImpl::CalcAheadFunc DrawStripeStrategy::GetCalcAheadFunc(EfDrawParticleManager* pm) {
    EfEmitterDrawSetting* ed;

    if (!IsValidPointer((u32)pm)) {
        nw4r::db::Panic("ef_drawstripestrategy.cpp", 949, "NW4R:Pointer Error\npm(=%p) is not valid pointer.", pm);
    }
    EF_VALID_PTR_ASSERT("ef_drawstripestrategy.cpp", 950, "NW4R:Pointer Error\npm->mResource(=%p) is not valid pointer.", pm->resource);
    ed = (EfEmitterDrawSetting*)ef_resource_draw_setting(pm->resource);
    if (!IsValidPointer((u32)ed)) {
        nw4r::db::Panic("ef_drawstripestrategy.cpp", 953, "NW4R:Pointer Error\n&ed(=%p) is not valid pointer.", ed);
    }
    switch (ed->type_direction) {
    case 0:
        return (CalcAheadFunc)ef_ahead_move_dir;
    case 1:
        return (CalcAheadFunc)ef_ahead_from_emitter;
    case 2:
        return (CalcAheadFunc)ef_ahead_emitter_axis_y;
    case 3:
        return ef_stripe_ahead_type3;
    case 5:
    case 7:
        return (CalcAheadFunc)ef_ahead_manager_axis_y;
    case 6:
        switch (ed->stripe_connect & 7) {
        case 1:
            return ef_stripe_ahead_type6_link1;
        case 2:
            return ef_stripe_ahead_type6_link2;
        default:
            return ef_stripe_ahead_type6;
        }
    default:
        return (CalcAheadFunc)ef_ahead_move_dir;
    }
}

/* 0x800B8888 (0x164): the base ahead context plus the emitter's X axis in manager space. */
DrawStripeStrategy::AheadContext::AheadContext(const MTX34* view, EfDrawParticleManager* pm)
    : DrawStrategyImpl::AheadContext(view, pm) {
    trig_table = NULL;
    if (!IsValidPointer((u32)pm)) {
        nw4r::db::Panic("ef_drawstripestrategy.cpp", 989, "NW4R:Pointer Error\npm(=%p) is not valid pointer.", pm);
    }
    VEC3 axis(emitter_mtx.m[0][0], emitter_mtx.m[1][0], emitter_mtx.m[2][0]);
    mtx34_rotate_vec3(&emitter_axis_x, &manager_mtx_inv, &axis);
}

}  // namespace ef
}  // namespace nw4r

extern "C" {

/* 0x800B89EC (0x35C): the ahead vector toward the next elder particle (or, for the eldest, from the emitter origin),
 * normalised, falling back to the emitter's Y axis. */
void ef_stripe_ahead_type3(VEC3* out, nw4r::ef::DrawStrategyImpl::AheadContext* ctx, EfDrawParticle* p) {
    EfDrawParticle* next;

    if (!IsValidPointer((u32)out)) {
        nw4r::db::Panic("ef_drawstripestrategy.cpp", 1002, "NW4R:Pointer Error\nyAxis(=%p) is not valid pointer.", out);
    }
    if (!IsValidPointer((u32)ctx)) {
        nw4r::db::Panic("ef_drawstripestrategy.cpp", 1003, "NW4R:Pointer Error\ncontext(=%p) is not valid pointer.", ctx);
    }
    if (!IsValidPointer((u32)p)) {
        nw4r::db::Panic("ef_drawstripestrategy.cpp", 1004, "NW4R:Pointer Error\nparticle(=%p) is not valid pointer.", p);
    }
    next = (EfDrawParticle*)ef_pm_prev_alive(ctx->particle_manager, p);
    if (next != NULL) {
        PSVECSubtract((f32*)out, (const f32*)&p->world_pos, (const f32*)&next->world_pos);
    } else {
        PSVECSubtract((f32*)out, (const f32*)&p->world_pos, (const f32*)&ctx->emitter_center);
    }
    if (ef_vec3_normalize(out) == 0) {
        *out = ctx->emitter_axis_y;
    }
}

/* 0x800B8DC0 (0x400): the ahead vector across the particle's two neighbours (each direction normalised), falling
 * back to the emitter's Y axis. */
void ef_stripe_ahead_type6(VEC3* out, nw4r::ef::DrawStrategyImpl::AheadContext* ctx, EfDrawParticle* p) {
    EfDrawParticle* younger;
    EfDrawParticle* elder;

    if (!IsValidPointer((u32)out)) {
        nw4r::db::Panic("ef_drawstripestrategy.cpp", 1030, "NW4R:Pointer Error\nyAxis(=%p) is not valid pointer.", out);
    }
    if (!IsValidPointer((u32)ctx)) {
        nw4r::db::Panic("ef_drawstripestrategy.cpp", 1031, "NW4R:Pointer Error\ncontext(=%p) is not valid pointer.", ctx);
    }
    if (!IsValidPointer((u32)p)) {
        nw4r::db::Panic("ef_drawstripestrategy.cpp", 1032, "NW4R:Pointer Error\nparticle(=%p) is not valid pointer.", p);
    }
    younger = (EfDrawParticle*)ef_pm_next_alive(ctx->particle_manager, p);
    elder = (EfDrawParticle*)ef_pm_prev_alive(ctx->particle_manager, p);
    VEC3 to_younger(0.0f, 0.0f, 0.0f);
    if (younger != NULL) {
        PSVECSubtract((f32*)&to_younger, (const f32*)&younger->world_pos, (const f32*)&p->world_pos);
        if (ef_vec3_normalize(&to_younger) == 0) {
            to_younger = VEC3(0.0f, 0.0f, 0.0f);
        }
    }
    VEC3 to_elder(0.0f, 0.0f, 0.0f);
    if (elder != NULL) {
        PSVECSubtract((f32*)&to_elder, (const f32*)&elder->world_pos, (const f32*)&p->world_pos);
        if (ef_vec3_normalize(&to_elder) == 0) {
            to_elder = VEC3(0.0f, 0.0f, 0.0f);
        }
    }
    PSVECSubtract((f32*)out, (const f32*)&to_younger, (const f32*)&to_elder);
    if (ef_vec3_normalize(out) == 0) {
        *out = ctx->emitter_axis_y;
    }
}

/* 0x800B91C0 (0x400): as the two-neighbour ahead vector, with the list wrapping around for a looped stripe. */
void ef_stripe_ahead_type6_link1(VEC3* out, nw4r::ef::DrawStrategyImpl::AheadContext* ctx, EfDrawParticle* p) {
    EfDrawParticle* younger;
    EfDrawParticle* elder;

    if (!IsValidPointer((u32)out)) {
        nw4r::db::Panic("ef_drawstripestrategy.cpp", 1069, "NW4R:Pointer Error\nyAxis(=%p) is not valid pointer.", out);
    }
    if (!IsValidPointer((u32)ctx)) {
        nw4r::db::Panic("ef_drawstripestrategy.cpp", 1070, "NW4R:Pointer Error\ncontext(=%p) is not valid pointer.", ctx);
    }
    if (!IsValidPointer((u32)p)) {
        nw4r::db::Panic("ef_drawstripestrategy.cpp", 1071, "NW4R:Pointer Error\nparticle(=%p) is not valid pointer.", p);
    }
    younger = (EfDrawParticle*)ef_pm_next_alive(ctx->particle_manager, p);
    if (younger == NULL) {
        younger = (EfDrawParticle*)ef_pm_first_alive((EfDrawList*)ctx->particle_manager);
    }
    elder = (EfDrawParticle*)ef_pm_prev_alive(ctx->particle_manager, p);
    if (elder == NULL) {
        elder = (EfDrawParticle*)ef_pm_last_alive((EfParticleState*)ctx->particle_manager);
    }
    VEC3 to_younger;
    PSVECSubtract((f32*)&to_younger, (const f32*)&younger->world_pos, (const f32*)&p->world_pos);
    if (ef_vec3_normalize(&to_younger) == 0) {
        to_younger = VEC3(0.0f, 0.0f, 0.0f);
    }
    VEC3 to_elder;
    PSVECSubtract((f32*)&to_elder, (const f32*)&elder->world_pos, (const f32*)&p->world_pos);
    if (ef_vec3_normalize(&to_elder) == 0) {
        to_elder = VEC3(0.0f, 0.0f, 0.0f);
    }
    PSVECSubtract((f32*)out, (const f32*)&to_younger, (const f32*)&to_elder);
    if (ef_vec3_normalize(out) == 0) {
        *out = ctx->emitter_axis_y;
    }
}

/* 0x800B9630 (0x3B8): as the two-neighbour ahead vector, with the eldest particle reaching to the emitter origin. */
void ef_stripe_ahead_type6_link2(VEC3* out, nw4r::ef::DrawStrategyImpl::AheadContext* ctx, EfDrawParticle* p) {
    EfDrawParticle* younger;
    EfDrawParticle* elder;

    if (!IsValidPointer((u32)out)) {
        nw4r::db::Panic("ef_drawstripestrategy.cpp", 1110, "NW4R:Pointer Error\nyAxis(=%p) is not valid pointer.", out);
    }
    if (!IsValidPointer((u32)ctx)) {
        nw4r::db::Panic("ef_drawstripestrategy.cpp", 1111, "NW4R:Pointer Error\ncontext(=%p) is not valid pointer.", ctx);
    }
    if (!IsValidPointer((u32)p)) {
        nw4r::db::Panic("ef_drawstripestrategy.cpp", 1112, "NW4R:Pointer Error\nparticle(=%p) is not valid pointer.", p);
    }
    younger = (EfDrawParticle*)ef_pm_next_alive(ctx->particle_manager, p);
    elder = (EfDrawParticle*)ef_pm_prev_alive(ctx->particle_manager, p);
    VEC3 to_younger(0.0f, 0.0f, 0.0f);
    if (younger != NULL) {
        PSVECSubtract((f32*)&to_younger, (const f32*)&younger->world_pos, (const f32*)&p->world_pos);
        ef_vec3_normalize(&to_younger);
    }
    VEC3 to_elder;
    if (elder != NULL) {
        PSVECSubtract((f32*)&to_elder, (const f32*)&elder->world_pos, (const f32*)&p->world_pos);
    } else {
        PSVECSubtract((f32*)&to_elder, (const f32*)&ctx->emitter_center, (const f32*)&p->world_pos);
    }
    ef_vec3_normalize(&to_elder);
    PSVECSubtract((f32*)out, (const f32*)&to_younger, (const f32*)&to_elder);
    if (ef_vec3_normalize(out) == 0) {
        *out = ctx->emitter_axis_y;
    }
}

}  // extern "C"

/* The destructors are the classes' inline ones, emitted at the end of this TU; retail calls each base destructor
 * out of line from them (`bl` + `extsh` of the delete flag), which a deferred inline body only gets with inlining
 * off at its point of emission. */
#pragma dont_inline on
