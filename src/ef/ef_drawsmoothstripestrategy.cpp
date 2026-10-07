/*
 * ef/ef_drawsmoothstripestrategy.cpp - nw4r::ef DrawSmoothStripeStrategy: `Draw`, `GetCalcAheadFunc`, its ahead
 *   context, and the TU's own copies of the stripe helpers: the quadratic B-spline ribbon and tube builders (each
 *   particle samples a ribbon edge pair or a tube ring, and every three neighbouring samples are blended into
 *   `curve steps` sub-segments), the GX writers and setup, the first-ahead and ahead-vector builders.
 * RANGE. .text 0x800BFFD4-0x800C5DB8 (49 functions); extab 0x8000A394-0x8000A49C, extabindex 0x80023904-0x80023A90,
 *   .data 0x805944E0-0x80594840 (the `__FILE__` string "ef_drawsmoothstripestrategy.cpp" first), .bss
 *   0x806945B8-0x806945E8, .sbss 0x80794928-0x80794930, .sdata2 0x807961C0-0x807961E0.
 *   Seam (playbook 80, measured): every strategy TU runs [constructor, ..., inline destructor]; this class's
 *   constructor (0x800BFF98) is the tail of `ef/ef_drawpointstrategy.cpp`'s range and is defined there.
 * FLAGS. `cflags_main`; `#pragma peephole off`, `#pragma fp_contract off` and `#pragma pool_data off` before the
 *   includes (retail keeps the unfused `fmuls`/`fsubs`, and reaches each `.bss` static by its own `lis`/`addi`
 *   where the pooled form hoists one base: `Draw` 95.09 -> 97.78), `#pragma dont_inline on` at the end (the inline
 *   destructor calls its base out of line).
 * NAMES. GUESS: every `ef_smooth_*` helper, from its body and its callers, in the scheme of the stripe unit's
 *   `ef_stripe_*` copies; the `.bss` statics `ef_smooth_axis_*` by their values.
 *   GUESS (from the body and its callers): `ef_smooth_ribbon_sample`, `ef_smooth_scale_mtx`,
 *   GUESS: `ef_smooth_draw_segment`, `ef_smooth_flag_facing`, `ef_smooth_tube_ring`, `ef_smooth_spline_weight`,
 *   GUESS: `ef_smooth_ribbon_curve`, `ef_smooth_ribbon_pair`, `ef_smooth_gx_texcoord`, `ef_smooth_flag_tex`,
 *   GUESS: `ef_smooth_gx_position`, `ef_smooth_gx_position3`, `ef_smooth_ribbon_blend`, `ef_smooth_ribbon_end`,
 *   GUESS: `ef_smooth_tube_curve`, `ef_smooth_gx_end`, `ef_smooth_flag_reverse`, `ef_smooth_tube_param_copy`,
 *   GUESS: `ef_smooth_tube_blend`, `ef_smooth_tube_param_ctor`, `ef_smooth_ribbon_open`,
 *   GUESS: `ef_smooth_ribbon_param_ctor`, `ef_smooth_tex_mode`, `ef_smooth_draw_order`, `ef_smooth_ribbon_loop`,
 *   GUESS: `ef_smooth_ribbon_to_emitter`, `ef_smooth_ribbon_param_set`, `ef_smooth_ribbon_param_copy`,
 *   GUESS: `ef_smooth_draw_ribbon`, `ef_smooth_connect_type`, `ef_smooth_tube_open`, `ef_smooth_tube_divide`,
 *   GUESS: `ef_smooth_tube_loop`, `ef_smooth_tube_to_emitter`, `ef_smooth_tube_param_set`, `ef_smooth_draw_tube`,
 *   GUESS: `ef_smooth_begin_side`, `ef_smooth_curve_steps`, `ef_smooth_setup_gx`, `ef_smooth_first_ahead`,
 *   GUESS: `ef_smooth_axis_mode`, `ef_smooth_ahead_type3`, `ef_smooth_ahead_type6`, `ef_smooth_ahead_type6_link1`,
 *   GUESS: `ef_smooth_ahead_type6_link2`.
 * SHAPES. `Draw` clamps the curve steps before the axis statics and picks the ribbon or the tube with a `switch`
 *   on the type option (`cmpwi`; an `if` compares `cmplwi`).
 * SHAPES. The `.bss` edge offsets are function-local statics constructed through `setVec3` behind `.sbss` guards;
 *   the ribbon pair writer takes the scale before the flags (the caller loads f1 before r5); a spline weight's `t`
 *   is computed into a local before the call; the connection dispatches list `default` first.
 * RESIDUALS. Every row is written.  8 partial rows:
 *  - `Draw` (0x800C3D88): retail's `copyVec3(.., setVec3(&tmp, ..))` temporaries sit below the loop locals (the
 *    out-of-line VEC3 constructor's temporaries), ours above, and `ed`/`count` take r31/r29 where ours has r30/r31;
 *  - `ef_smooth_ribbon_open`/`_loop`/`_to_emitter`, `ef_smooth_tube_open`/`_loop`/`_to_emitter`: the register
 *    allocator gives the parameters r20-r25 and the loop state the high registers in retail, the reverse in ours;
 *  - `ef_smooth_tube_curve`: the second side loop's counter and table pointer swap r20/r21.
 *   Data: `.bss` (the four statics) is emitted and matches; `.sbss` emits the four guards (0x4 of the claimed
 *   0x8); `.data` holds only our vtable - the strings stay `extern` because the range is two TUs by its
 *   emission order (`datagap.py`); the weak `DrawStrategyImpl`/`DrawStrategy` destructors add 0xA0 of `.text`.
 *   Relocation names that differ from retail (pool constants, save helpers, statics): `ef_smooth_zero`,
 *     `ef_smooth_one`, `ef_smooth_half`, `ef_smooth_minus_one`, `ef_smooth_hundredth`, `ef_smooth_full_circle`,
 *     `ef_smooth_axis_x`, `ef_smooth_axis_neg_x`, `ef_smooth_axis_z`, `ef_smooth_axis_neg_z`.
 */

#pragma peephole off
#pragma fp_contract off
#pragma pool_data off

#include "types.h"
#include "gx.h"
#include "ef.h"
#include "ef/ef_pointer_assert.h" /* EF_VALID_PTR_ASSERT */
#include "ef/ef_drawstrategyimpl.h"
#include "ef/fn_800AEE48.h"            /* ef_vec3_normalize / ef_pm_*_alive (rule 2) */
#include "ef/ef_drawstripestrategy.h"  /* EfStripeParam, ef_stripe_draw_count, the shared ahead builders (rule 2) */
#include "ef/ef_drawsmoothstripestrategy.h"
#include "sys_mem.h"
#include "mh3_pad.h"                   /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "ef/ef_particle.h"            /* ef_particle_get_scale / ef_particle_get_scale_y (rule 2) */
#include "RVLGX/GXSetTevOrder.h"       /* GXLoadPosMtxImm (rule 2) */
#include "EXI/GXBegin.h"               /* the GX vertex-format entry points (rule 2) */
#include "g3d/g3d_calcview.h"          /* mtx34_concat (rule 2) */
#include "nw4r/fn_805012C4.h"          /* mtx34_rotate_vec3 (rule 2) */
#include "fn_8004CAD8.h"               /* MTX34_ctor, vec3_cross, addVec3, subVec3 (rule 2) */
#include "g3d/fn_80075DCC.h"           /* mtx34_set, sin_cos_deg (rule 2) */
#include "g3d/g3d_anmchr.h"            /* math_reciprocal (rule 2) */
#include "nw4r/mtx34_mult_vec3.h"      /* mtx34_mult_vec3 (rule 2) */
#include "g3d/g3d_calcworld.h"         /* addVec3To (rule 2) */
#include "g3d/mtx34_inverse.h"         /* mtx34_inverse (rule 2) */

/* `nw4r::db::Panic`, declared in its namespace so the front-end emits the map's mangling
 * (`Panic__Q24nw4r2dbFPCciPCce`). */
namespace nw4r {
namespace db {
void Panic(const char* file, int line, const char* fmt, ...);
}  // namespace db
}  // namespace nw4r

extern "C" {

/* The `__FILE__`/assert strings of this unit's `.data`, declared, never defined. */
extern char ef_smooth_file_str[];            /* "ef_drawsmoothstripestrategy.cpp"                 .data 0x805944E0 */
extern char ef_smooth_dst_assert_str[];      /* "NW4R:Pointer Error\ndst(=%p) ..."                 .data 0x80594500 */
extern char ef_smooth_context_assert_str[];  /* "NW4R:Pointer Error\ncontext(=%p) ..."             .data 0x80594534 */
extern char ef_smooth_pp_assert_str[];       /* "NW4R:Pointer Error\npp(=%p) ..."                  .data 0x8059456C */
extern char ef_smooth_trig_assert_str[];     /* "NW4R:Pointer Error\ntrigonometric(=%p) ..."       .data 0x805945A0 */
extern char ef_smooth_ahead_assert_str[];    /* "NW4R:Pointer Error\naheadContext(=%p) ..."        .data 0x805945DC */
extern char ef_smooth_youngest_assert_str[]; /* "NW4R:Failed assertion youngest"                   .data 0x80594618 */
extern char ef_smooth_tube_divide_assert_str[]; /* "NW4R:Failed assertion 3 <= GetTubeDivide(ed)"  .data 0x80594638 */
extern char ef_smooth_pm_assert_str[];       /* "NW4R:Pointer Error\npm(=%p) ..."                  .data 0x80594668 */
extern char ef_smooth_resource_assert_str[]; /* "NW4R:Pointer Error\npm->mResource(=%p) ..."       .data 0x8059469C */
extern char ef_smooth_ed_assert_str[];       /* "NW4R:Pointer Error\n&ed(=%p) ..."                 .data 0x805946D8 */
extern char ef_smooth_yaxis_assert_str[];    /* "NW4R:Pointer Error\nyAxis(=%p) ..."               .data 0x8059470C */
extern char ef_smooth_particle_assert_str[]; /* "NW4R:Pointer Error\nparticle(=%p) ..."            .data 0x80594740 */
extern char ef_smooth_scale_pp_str[];        /* "NW4R:Pointer Error\npp(=%p) ...", an inline copy  .data 0x80594794 */
extern char ef_smooth_scale_file_str[];      /* "ef_drawsmoothstripestrategy.cpp", an inline copy .data 0x805947C8 */
extern char ef_smooth_inline_pp_str[];       /* "NW4R:Pointer Error\npp(=%p) ...", an inline copy  .data 0x805947E8 */
extern char ef_smooth_inline_file_str[];     /* "ef_drawsmoothstripestrategy.cpp", an inline copy .data 0x80594820 */

/* A ribbon sample: the two edge points and the texture coordinate along the stripe. */
typedef struct EfRibbonParam {
    /* +0x00 */ VEC3 a;
    /* +0x0C */ VEC3 b;
    /* +0x18 */ f32 tex_t;
} EfRibbonParam; /* size: 0x1C */

/* Declarations of this unit's helpers that are used before their definition. */
void ef_smooth_draw_segment(MTX34* out, nw4r::ef::DrawSmoothStripeStrategy* self,
                            nw4r::ef::DrawSmoothStripeStrategy::AheadContext* ctx, u32 flags, EfDrawParticle* p,
                            const VEC3* ahead, const VEC3* pos);
void ef_smooth_scale_mtx(MTX34* out, nw4r::ef::DrawSmoothStripeStrategy* self, EfDrawParticle* p, f32 width);
int ef_smooth_flag_facing(u32 value);
void ef_smooth_ribbon_pair(Vec* a, Vec* b, f32 scale, u32 flags);
void ef_smooth_gx_texcoord(f32 x, f32 y);
int ef_smooth_flag_tex(u32 value);
void ef_smooth_gx_position(Vec* v);
void ef_smooth_gx_position3(f32 x, f32 y, f32 z);
EfRibbonParam* ef_smooth_ribbon_blend(EfRibbonParam* out, const EfRibbonParam* a, const EfRibbonParam* b,
                                      const EfRibbonParam* c, const VEC3* w);
void ef_smooth_spline_weight(VEC3* out, nw4r::ef::DrawSmoothStripeStrategy* self, f32 t);
void ef_smooth_gx_end(void);
int ef_smooth_flag_reverse(u32 value);
void ef_smooth_tube_param_copy(EfStripeParam* dst, const EfStripeParam* src);
EfStripeParam* ef_smooth_tube_blend(EfStripeParam* out, const EfStripeParam* a, const EfStripeParam* b,
                                    const EfStripeParam* c, const VEC3* w);
EfStripeParam* ef_smooth_tube_param_ctor(EfStripeParam* self);
EfRibbonParam* ef_smooth_ribbon_param_ctor(EfRibbonParam* self);
s32 ef_smooth_tex_mode(nw4r::ef::DrawSmoothStripeStrategy* self, const EfEmitterDrawSetting* ed);
u32 ef_smooth_draw_order(nw4r::ef::DrawSmoothStripeStrategy* self, const EfEmitterDrawSetting* ed);
EfRibbonParam* ef_smooth_ribbon_param_set(EfRibbonParam* self, const VEC3* a, const VEC3* b, f32 t);
void ef_smooth_ribbon_param_copy(EfRibbonParam* dst, const EfRibbonParam* src);
s32 ef_smooth_connect_type(nw4r::ef::DrawSmoothStripeStrategy* self, const EfEmitterDrawSetting* ed);
s32 ef_smooth_tube_divide(nw4r::ef::DrawSmoothStripeStrategy* self, const EfEmitterDrawSetting* ed);
EfStripeParam* ef_smooth_tube_param_set(EfStripeParam* self, const VEC3* center, const VEC3* side, const VEC3* up,
                                        f32 t);
void ef_smooth_begin_side(u32 cull_mode, u32 enabled, u32 flags);
s32 ef_smooth_curve_steps(nw4r::ef::DrawSmoothStripeStrategy* self, const EfEmitterDrawSetting* ed);
void ef_smooth_setup_gx(nw4r::ef::DrawSmoothStripeStrategy* self, const EfDrawInfo* info, EfDrawParticleManager* pm);
void ef_smooth_first_ahead(VEC3* out, nw4r::ef::DrawSmoothStripeStrategy* self, EfEmitterDrawSetting* ed,
                           nw4r::ef::DrawSmoothStripeStrategy::AheadContext* ctx);
s32 ef_smooth_axis_mode(nw4r::ef::DrawSmoothStripeStrategy* self, const EfEmitterDrawSetting* ed);
void ef_smooth_ahead_type3(VEC3* out, nw4r::ef::DrawStrategyImpl::AheadContext* ctx, EfDrawParticle* p);
void ef_smooth_ahead_type6(VEC3* out, nw4r::ef::DrawStrategyImpl::AheadContext* ctx, EfDrawParticle* p);
void ef_smooth_ahead_type6_link1(VEC3* out, nw4r::ef::DrawStrategyImpl::AheadContext* ctx, EfDrawParticle* p);
void ef_smooth_ahead_type6_link2(VEC3* out, nw4r::ef::DrawStrategyImpl::AheadContext* ctx, EfDrawParticle* p);

/* 0x800BFFD4 (0x3BC): one ribbon sample: the particle's frame times its rotation-and-scale matrix applied to the edge
 * offsets `a`/`b`, with texture coordinate `t`. */
void ef_smooth_ribbon_sample(nw4r::ef::DrawSmoothStripeStrategy* self, EfRibbonParam* out,
                             nw4r::ef::DrawSmoothStripeStrategy::AheadContext* ctx, u32 flags, EfDrawParticle* p,
                             nw4r::ef::DrawStrategyImpl::CalcAheadFunc calc_ahead, const VEC3* pos, const VEC3* a,
                             const VEC3* b, f32 width, f32 t) {
    VEC3 ahead;
    MTX34 mtx;
    MTX34 frame;
    MTX34 scale;

    if (!IsValidPointer((u32)out)) {
        nw4r::db::Panic(ef_smooth_file_str, 208, ef_smooth_dst_assert_str, out);
    }
    if (!IsValidPointer((u32)ctx)) {
        nw4r::db::Panic(ef_smooth_file_str, 209, ef_smooth_context_assert_str, ctx);
    }
    if (!IsValidPointer((u32)p)) {
        nw4r::db::Panic(ef_smooth_file_str, 210, ef_smooth_pp_assert_str, p);
    }
    VEC3_ctor(&ahead);
    MTX34_ctor(&mtx);
    calc_ahead(&ahead, ctx, p);
    ef_smooth_draw_segment(&frame, self, ctx, flags, p, &ahead, pos);
    ef_smooth_scale_mtx(&scale, self, p, width);
    mtx34_concat(&mtx, &frame, &scale);
    mtx34_mult_vec3(&out->a, &mtx, a);
    mtx34_mult_vec3(&out->b, &mtx, b);
    out->tex_t = t;
}

/* 0x800C0390 (0x1E4): the rotation (about the stripe axis, by the particle's Y rotation) and scale of one ribbon
 * segment, recentred on the strip width. */
void ef_smooth_scale_mtx(MTX34* out, nw4r::ef::DrawSmoothStripeStrategy* self, EfDrawParticle* p, f32 width) {
    VEC3 rotate;
    f32 c;
    f32 s;
    f32 size;

    if (!IsValidPointer((u32)p)) {
        nw4r::db::Panic(ef_smooth_scale_file_str, 177, ef_smooth_scale_pp_str, p);
    }
    VEC3_ctor(&rotate);
    ef_particle_get_rotate(p, &rotate);
    size = ef_particle_get_scale((EfParticle*)p);
    ef_sin_cos(&s, &c, rotate.y);
    c *= size;
    s *= size;
    mtx34_set(out, c, 0.0f, -s, width - c * width, 0.0f, 1.0f, 0.0f, 0.0f, s, 0.0f, c, -s * width);
}

/* 0x800C0574 (0x1FC): the frame of one stripe particle: the side axis is the ahead vector crossed with the previous
 * particle's (or, in view-facing mode, the view's Z axis), the up axis completes the frame and becomes the particle's
 * new ahead vector, and the position is the translation. */
void ef_smooth_draw_segment(MTX34* out, nw4r::ef::DrawSmoothStripeStrategy* self,
                            nw4r::ef::DrawSmoothStripeStrategy::AheadContext* ctx, u32 flags, EfDrawParticle* p,
                            const VEC3* ahead, const VEC3* pos) {
    VEC3 side;
    VEC3 up;

    if (!IsValidPointer((u32)p)) {
        nw4r::db::Panic(ef_smooth_inline_file_str, 140, ef_smooth_inline_pp_str, p);
    }
    VEC3_ctor(&side);
    VEC3_ctor(&up);
    if (ef_smooth_flag_facing(flags) == 0) {
        vec3_cross((f32*)&side, (const f32*)ahead, (const f32*)&p->ahead);
    } else {
        vec3_cross((f32*)&side, (const f32*)ahead, (const f32*)&ctx->view_axis_z);
    }
    if (ef_vec3_normalize(&side) == 0) {
        copyVec3(&side, &ctx->emitter_axis_x);
    }
    vec3_cross((f32*)&up, (const f32*)&side, (const f32*)ahead);
    ef_vec3_normalize(&up);
    copyVec3(&p->ahead, &up);
    mtx34_set(out, side.x, ahead->x, up.x, pos->x, side.y, ahead->y, up.y, pos->y, side.z, ahead->z, up.z, pos->z);
}

/* Tests the view-facing bit of the draw flags. */
int ef_smooth_flag_facing(u32 value) {
    return (value & 0x8) != 0;
}

/* 0x800C0784 (0x4DC): one tube ring: the particle's frame times its rotation-and-scale matrix, kept as a centre and
 * the X/Z axes, with texture coordinate `t`. */
void ef_smooth_tube_ring(nw4r::ef::DrawSmoothStripeStrategy* self, EfStripeParam* out,
                         nw4r::ef::DrawSmoothStripeStrategy::AheadContext* ctx, u32 flags, EfDrawParticle* p,
                         nw4r::ef::DrawStrategyImpl::CalcAheadFunc calc_ahead, f32 offset_x, f32 offset_y, f32 t) {
    MTX34 frame;
    MTX34 local;
    MTX34 mtx;
    VEC3 ahead;
    VEC3 rotate;
    VEC3 center;
    f32 s;
    f32 c;
    f32 scale_x;
    f32 scale_y;

    if (!IsValidPointer((u32)out)) {
        nw4r::db::Panic(ef_smooth_file_str, 246, ef_smooth_dst_assert_str, out);
    }
    if (!IsValidPointer((u32)ctx)) {
        nw4r::db::Panic(ef_smooth_file_str, 247, ef_smooth_context_assert_str, ctx);
    }
    if (!IsValidPointer((u32)p)) {
        nw4r::db::Panic(ef_smooth_file_str, 248, ef_smooth_pp_assert_str, p);
    }
    VEC3_ctor(&ahead);
    calc_ahead(&ahead, ctx, p);
    ef_smooth_draw_segment(&frame, self, ctx, flags, p, &ahead, (const VEC3*)&p->world_pos);
    VEC3_ctor(&rotate);
    ef_particle_get_rotate(p, &rotate);
    scale_x = ef_particle_get_scale((EfParticle*)p);
    scale_y = ef_particle_get_scale_y((EfParticle*)p);
    ef_sin_cos(&s, &c, rotate.y);
    mtx34_set(&local, c * scale_x, 0.0f, -s * scale_y,
              (offset_x - offset_x * (c * scale_x)) + offset_y * (s * scale_y), 0.0f, 1.0f,
              0.0f, 0.0f, s * scale_x, 0.0f, c * scale_y,
              (offset_y - offset_x * (s * scale_x)) - offset_y * (c * scale_y));
    MTX34_ctor(&mtx);
    mtx34_concat(&mtx, &frame, &local);
    copyVec3(&out->center, setVec3(&center, mtx.m[0][3], mtx.m[1][3], mtx.m[2][3]));
    mtx34_rotate_vec3(&out->side, &mtx, ef_get_unit_x_vec());
    mtx34_rotate_vec3(&out->up, &mtx, ef_get_unit_z_vec());
    out->tex_t = t;
}

/* 0x800C0C60 (0x2C): the quadratic B-spline weights of the three neighbouring samples at `t` in [0, 1]. */
void ef_smooth_spline_weight(VEC3* out, nw4r::ef::DrawSmoothStripeStrategy* self, f32 t) {
    f32 t2 = t * t;
    f32 half_t2 = 0.5f * t2;

    setVec3(out, 0.5f + (half_t2 - t), 0.5f + (-1.0f * t2 + t), half_t2);
}

/* 0x800C0C8C (0xF8): the `steps` ribbon vertex pairs of the curve through three neighbouring samples (the last
 * one, at t = 1, is the next curve's first). */
void ef_smooth_ribbon_curve(nw4r::ef::DrawSmoothStripeStrategy* self, s32 steps, const EfRibbonParam* a,
                            const EfRibbonParam* b, const EfRibbonParam* c, u32 flags) {
    f32 inv = math_reciprocal(steps);
    s32 i;

    for (i = 0; i < steps; i++) {
        EfRibbonParam point;
        VEC3 w;
        f32 t = i * inv;

        ef_smooth_spline_weight(&w, self, t);
        ef_smooth_ribbon_blend(&point, a, b, c, &w);
        ef_smooth_ribbon_pair((Vec*)&point.a, (Vec*)&point.b, point.tex_t, flags);
    }
}

/* 0x800C0D84 (0x90): writes one ribbon vertex pair, each followed by its texture coordinate when the flag asks. */
void ef_smooth_ribbon_pair(Vec* a, Vec* b, f32 scale, u32 flags) {
    ef_smooth_gx_position(a);
    if (ef_smooth_flag_tex(flags)) {
        ef_smooth_gx_texcoord(1.0f, scale);
    }
    ef_smooth_gx_position(b);
    if (ef_smooth_flag_tex(flags)) {
        ef_smooth_gx_texcoord(0.0f, scale);
    }
}

/* Writes a pair of f32 to the pipe. */
void ef_smooth_gx_texcoord(f32 x, f32 y) {
    GXWGFifo.f32 = x;
    GXWGFifo.f32 = y;
}

/* Tests the texture-coordinate bit of the draw flags. */
int ef_smooth_flag_tex(u32 value) {
    return (value & 1) != 0;
}

/* Writes one vector to the pipe. */
void ef_smooth_gx_position(Vec* v) {
    ef_smooth_gx_position3(v->x, v->y, v->z);
}

/* Writes three f32 to the pipe. */
void ef_smooth_gx_position3(f32 x, f32 y, f32 z) {
    GXWGFifo.f32 = x;
    GXWGFifo.f32 = y;
    GXWGFifo.f32 = z;
}

/* 0x800C0E5C (0x138): the weighted sum of three ribbon samples. */
EfRibbonParam* ef_smooth_ribbon_blend(EfRibbonParam* out, const EfRibbonParam* a, const EfRibbonParam* b,
                                      const EfRibbonParam* c, const VEC3* w) {
    VEC3 a_sum2;
    VEC3 a_sum1;
    VEC3 a_wa;
    VEC3 a_wb;
    VEC3 a_wc;
    VEC3 b_sum2;
    VEC3 b_sum1;
    VEC3 b_wa;
    VEC3 b_wb;
    VEC3 b_wc;

    VEC3_ctor(&out->a);
    VEC3_ctor(&out->b);
    vec3_scale(&a_wc, (VEC3*)&c->a, w->z);
    vec3_scale(&a_wb, (VEC3*)&b->a, w->y);
    vec3_scale(&a_wa, (VEC3*)&a->a, w->x);
    addVec3(&a_sum1, &a_wa, &a_wb);
    addVec3(&a_sum2, &a_sum1, &a_wc);
    copyVec3(&out->a, &a_sum2);
    vec3_scale(&b_wc, (VEC3*)&c->b, w->z);
    vec3_scale(&b_wb, (VEC3*)&b->b, w->y);
    vec3_scale(&b_wa, (VEC3*)&a->b, w->x);
    addVec3(&b_sum1, &b_wa, &b_wb);
    addVec3(&b_sum2, &b_sum1, &b_wc);
    copyVec3(&out->b, &b_sum2);
    out->tex_t = c->tex_t * w->z + (a->tex_t * w->x + b->tex_t * w->y);
    return out;
}

/* 0x800C0F94 (0x8C): the ribbon vertex pair at the end (t = 1) of the curve through three samples. */
void ef_smooth_ribbon_end(nw4r::ef::DrawSmoothStripeStrategy* self, const EfRibbonParam* a, const EfRibbonParam* b,
                          const EfRibbonParam* c, u32 flags) {
    EfRibbonParam point;
    VEC3 w;

    ef_smooth_spline_weight(&w, self, 1.0f);
    ef_smooth_ribbon_blend(&point, a, b, c, &w);
    ef_smooth_ribbon_pair((Vec*)&point.a, (Vec*)&point.b, point.tex_t, flags);
}

/* 0x800C1020 (0x4D0): the tube strips of the curve through three neighbouring rings: `steps` blended rings, one strip
 * of `divide` sides between each pair (from the cos/sin table; the ring order sets the facing). */
void ef_smooth_tube_curve(nw4r::ef::DrawSmoothStripeStrategy* self, s32 steps, s32 divide, const EfStripeParam* a,
                          const EfStripeParam* b, const EfStripeParam* c, u32 flags, const f32* trig) {
    f32 inv_steps;
    f32 inv_divide;
    VEC3 w;
    EfStripeParam prev;
    EfStripeParam cur;
    s32 i;

    if (!IsValidPointer((u32)trig)) {
        nw4r::db::Panic(ef_smooth_file_str, 334, ef_smooth_trig_assert_str, trig);
    }
    inv_steps = math_reciprocal(steps);
    inv_divide = math_reciprocal(divide);
    ef_smooth_spline_weight(&w, self, 0.0f);
    ef_smooth_tube_param_ctor(&prev);
    ef_smooth_tube_blend(&cur, a, b, c, &w);
    for (i = 1; i <= steps; i++) {
        VEC3 next_w;
        EfStripeParam next;
        s32 j;
        const f32* pair;
        f32 t;

        ef_smooth_tube_param_copy(&prev, &cur);
        t = i * inv_steps;
        ef_smooth_spline_weight(&next_w, self, t);
        copyVec3(&w, &next_w);
        ef_smooth_tube_param_copy(&cur, ef_smooth_tube_blend(&next, a, b, c, &w));
        GXBegin(0x98, 0, divide * 2 + 2);
        if (ef_smooth_flag_reverse(flags)) {
            for (j = 0, pair = trig; j <= divide; pair += 2, j++) {
                f32 s = pair[1];
                f32 co = pair[0];
                VEC3 p_pos;
                VEC3 p_sum;
                VEC3 p_side;
                VEC3 p_up;
                VEC3 c_pos;
                VEC3 c_sum;
                VEC3 c_side;
                VEC3 c_up;

                vec3_scale(&p_up, &prev.up, s);
                vec3_scale(&p_side, &prev.side, co);
                addVec3(&p_sum, &p_side, &p_up);
                addVec3(&p_pos, &p_sum, &prev.center);
                ef_smooth_gx_position((Vec*)&p_pos);
                if (ef_smooth_flag_tex(flags)) {
                    ef_smooth_gx_texcoord(inv_divide * j, prev.tex_t);
                }
                vec3_scale(&c_up, &cur.up, s);
                vec3_scale(&c_side, &cur.side, co);
                addVec3(&c_sum, &c_side, &c_up);
                addVec3(&c_pos, &c_sum, &cur.center);
                ef_smooth_gx_position((Vec*)&c_pos);
                if (ef_smooth_flag_tex(flags)) {
                    ef_smooth_gx_texcoord(inv_divide * j, cur.tex_t);
                }
            }
        } else {
            for (j = 0, pair = trig; j <= divide; pair += 2, j++) {
                f32 s = pair[1];
                f32 co = pair[0];
                VEC3 c_pos;
                VEC3 c_sum;
                VEC3 c_side;
                VEC3 c_up;
                VEC3 p_pos;
                VEC3 p_sum;
                VEC3 p_side;
                VEC3 p_up;

                vec3_scale(&c_up, &cur.up, s);
                vec3_scale(&c_side, &cur.side, co);
                addVec3(&c_sum, &c_side, &c_up);
                addVec3(&c_pos, &c_sum, &cur.center);
                ef_smooth_gx_position((Vec*)&c_pos);
                if (ef_smooth_flag_tex(flags)) {
                    ef_smooth_gx_texcoord(inv_divide * j, cur.tex_t);
                }
                vec3_scale(&p_up, &prev.up, s);
                vec3_scale(&p_side, &prev.side, co);
                addVec3(&p_sum, &p_side, &p_up);
                addVec3(&p_pos, &p_sum, &prev.center);
                ef_smooth_gx_position((Vec*)&p_pos);
                if (ef_smooth_flag_tex(flags)) {
                    ef_smooth_gx_texcoord(inv_divide * j, prev.tex_t);
                }
            }
        }
        ef_smooth_gx_end();
    }
}

/* Ends the current FIFO command (the SDK's `GXEnd`, which writes nothing). */
void ef_smooth_gx_end(void) {}

/* Tests the reversed-facing bit of the draw flags. */
int ef_smooth_flag_reverse(u32 value) {
    return (value & 0x10) != 0;
}

/* Copies a tube ring. */
void ef_smooth_tube_param_copy(EfStripeParam* dst, const EfStripeParam* src) {
    *dst = *src;
}

/* 0x800C155C (0x19C): the weighted sum of three tube rings. */
EfStripeParam* ef_smooth_tube_blend(EfStripeParam* out, const EfStripeParam* a, const EfStripeParam* b,
                                    const EfStripeParam* c, const VEC3* w) {
    VEC3 c_sum2;
    VEC3 c_sum1;
    VEC3 c_wa;
    VEC3 c_wb;
    VEC3 c_wc;
    VEC3 s_sum2;
    VEC3 s_sum1;
    VEC3 s_wa;
    VEC3 s_wb;
    VEC3 s_wc;
    VEC3 u_sum2;
    VEC3 u_sum1;
    VEC3 u_wa;
    VEC3 u_wb;
    VEC3 u_wc;

    VEC3_ctor(&out->center);
    VEC3_ctor(&out->side);
    VEC3_ctor(&out->up);
    vec3_scale(&c_wc, (VEC3*)&c->center, w->z);
    vec3_scale(&c_wb, (VEC3*)&b->center, w->y);
    vec3_scale(&c_wa, (VEC3*)&a->center, w->x);
    addVec3(&c_sum1, &c_wa, &c_wb);
    addVec3(&c_sum2, &c_sum1, &c_wc);
    copyVec3(&out->center, &c_sum2);
    vec3_scale(&s_wc, (VEC3*)&c->side, w->z);
    vec3_scale(&s_wb, (VEC3*)&b->side, w->y);
    vec3_scale(&s_wa, (VEC3*)&a->side, w->x);
    addVec3(&s_sum1, &s_wa, &s_wb);
    addVec3(&s_sum2, &s_sum1, &s_wc);
    copyVec3(&out->side, &s_sum2);
    vec3_scale(&u_wc, (VEC3*)&c->up, w->z);
    vec3_scale(&u_wb, (VEC3*)&b->up, w->y);
    vec3_scale(&u_wa, (VEC3*)&a->up, w->x);
    addVec3(&u_sum1, &u_wa, &u_wb);
    addVec3(&u_sum2, &u_sum1, &u_wc);
    copyVec3(&out->up, &u_sum2);
    out->tex_t = c->tex_t * w->z + (a->tex_t * w->x + b->tex_t * w->y);
    return out;
}

/* Zeroes the three vectors of a tube ring and returns it. */
EfStripeParam* ef_smooth_tube_param_ctor(EfStripeParam* self) {
    VEC3_ctor(&self->center);
    VEC3_ctor(&self->side);
    VEC3_ctor(&self->up);
    return self;
}

/* 0x800C1738 (0x444): an open smooth ribbon: one sample per particle, a curve through every three neighbouring
 * samples and the closing curve and end pair at the last one. */
void ef_smooth_ribbon_open(nw4r::ef::DrawSmoothStripeStrategy* self,
                           nw4r::ef::DrawSmoothStripeStrategy::AheadContext* ctx, s32 steps, u32 flags,
                           const VEC3* a, const VEC3* b) {
    EfDrawParticleManager* pm;
    EfEmitterDrawSetting* ed;
    nw4r::ef::DrawStrategyImpl::CalcAheadFunc calc_ahead;
    u32 order;
    nw4r::ef::DrawStrategyImpl::GetFirstDrawParticleFunc first;
    nw4r::ef::DrawStrategyImpl::GetNextDrawParticleFunc next;
    s32 count;
    f32 width;
    f32 step;
    s32 index;
    s32 dir;
    EfRibbonParam samples[3];
    s32 prev;
    s32 cur;
    EfDrawParticle* p;

    if (!IsValidPointer((u32)ctx)) {
        nw4r::db::Panic(ef_smooth_file_str, 403, ef_smooth_ahead_assert_str, ctx);
    }
    pm = ctx->particle_manager;
    ed = (EfEmitterDrawSetting*)ef_resource_draw_setting(pm->resource);
    calc_ahead = self->GetCalcAheadFunc(pm);
    order = ef_smooth_draw_order(self, ed);
    first = self->GetGetFirstDrawParticleFunc(order);
    next = self->GetGetNextDrawParticleFunc(order);
    count = ef_stripe_draw_count(self, pm);
    width = 0.01f * ed->scale_a;
    GXBegin(0x98, 0, (count * steps + 1) * 2);
    if (ef_smooth_tex_mode(self, ed) == 0x40) {
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
    ef_smooth_ribbon_param_ctor(&samples[0]);
    ef_smooth_ribbon_param_ctor(&samples[1]);
    ef_smooth_ribbon_param_ctor(&samples[2]);
    prev = 0;
    cur = 0;
    p = first(pm);
    ef_smooth_ribbon_sample(self, &samples[0], ctx, flags, p, calc_ahead, (const VEC3*)&p->world_pos, a, b, width,
                            step * index);
    index += dir;
    for (p = next(pm, p); p != NULL; p = next(pm, p), index += dir) {
        s32 old = prev;
        EfRibbonParam* sample;

        prev = cur;
        cur = (cur + 1) % 3;
        sample = &samples[cur];
        ef_smooth_ribbon_sample(self, sample, ctx, flags, p, calc_ahead, (const VEC3*)&p->world_pos, a, b, width,
                                step * index);
        ef_smooth_ribbon_curve(self, steps, &samples[old], &samples[prev], sample, flags);
    }
    {
        EfRibbonParam* last = &samples[cur];
        EfRibbonParam* before = &samples[prev];

        ef_smooth_ribbon_curve(self, steps, before, last, last, flags);
        ef_smooth_ribbon_end(self, before, last, last, flags);
    }
    ef_smooth_gx_end();
}

/* Zeroes the two vectors of a ribbon sample and returns it. */
EfRibbonParam* ef_smooth_ribbon_param_ctor(EfRibbonParam* self) {
    VEC3_ctor(&self->a);
    VEC3_ctor(&self->b);
    return self;
}

/* The texture-mapping bits (6-7) of the draw setting's stripe flags. */
s32 ef_smooth_tex_mode(nw4r::ef::DrawSmoothStripeStrategy* self, const EfEmitterDrawSetting* ed) {
    return ed->stripe_connect & 0xC0;
}

/* The draw-order bit (0x800) of the draw setting's flags. */
u32 ef_smooth_draw_order(nw4r::ef::DrawSmoothStripeStrategy* self, const EfEmitterDrawSetting* ed) {
    return ed->flags & 0x800;
}

/* 0x800C1BCC (0x518): a looped smooth ribbon: the open ribbon plus the curves that close it through the first two
 * samples (sampled first, at half steps). */
void ef_smooth_ribbon_loop(nw4r::ef::DrawSmoothStripeStrategy* self,
                           nw4r::ef::DrawSmoothStripeStrategy::AheadContext* ctx, s32 steps, u32 flags,
                           const VEC3* a, const VEC3* b) {
    EfDrawParticleManager* pm;
    EfEmitterDrawSetting* ed;
    nw4r::ef::DrawStrategyImpl::CalcAheadFunc calc_ahead;
    u32 order;
    nw4r::ef::DrawStrategyImpl::GetFirstDrawParticleFunc first;
    nw4r::ef::DrawStrategyImpl::GetNextDrawParticleFunc next;
    s32 count;
    f32 width;
    f32 step;
    s32 index;
    s32 dir;
    EfRibbonParam samples[5];
    VEC3 unused_a;
    VEC3 unused_b;
    s32 prev;
    s32 cur;
    EfDrawParticle* p;

    if (!IsValidPointer((u32)ctx)) {
        nw4r::db::Panic(ef_smooth_file_str, 475, ef_smooth_ahead_assert_str, ctx);
    }
    pm = ctx->particle_manager;
    ed = (EfEmitterDrawSetting*)ef_resource_draw_setting(pm->resource);
    calc_ahead = self->GetCalcAheadFunc(pm);
    order = ef_smooth_draw_order(self, ed);
    first = self->GetGetFirstDrawParticleFunc(order);
    next = self->GetGetNextDrawParticleFunc(order);
    count = ef_stripe_draw_count(self, pm);
    width = 0.01f * ed->scale_a;
    GXBegin(0x98, 0, (count * steps + 1) * 2);
    if (ef_smooth_tex_mode(self, ed) == 0x40) {
        step = 0.5f;
    } else {
        step = 0.5f / count;
    }
    index = -1;
    dir = 2;
    if (order != 0) {
        index = count * 2 + 1;
        dir = -2;
    }
    ef_smooth_ribbon_param_ctor(&samples[0]);
    ef_smooth_ribbon_param_ctor(&samples[1]);
    ef_smooth_ribbon_param_ctor(&samples[2]);
    ef_smooth_ribbon_param_ctor(&samples[3]);
    ef_smooth_ribbon_param_ctor(&samples[4]);
    VEC3_ctor(&unused_a);
    VEC3_ctor(&unused_b);
    p = first(pm);
    ef_smooth_ribbon_sample(self, &samples[3], ctx, flags, p, calc_ahead, (const VEC3*)&p->world_pos, a, b, width,
                            step * index);
    p = next(pm, p);
    index += dir;
    ef_smooth_ribbon_sample(self, &samples[4], ctx, flags, p, calc_ahead, (const VEC3*)&p->world_pos, a, b, width,
                            step * index);
    p = next(pm, p);
    index += dir;
    prev = 3;
    cur = 4;
    for (; p != NULL; p = next(pm, p), index += dir) {
        s32 old = prev;
        EfRibbonParam* sample;

        prev = cur;
        cur = (cur + 1) % 3;
        sample = &samples[cur];
        ef_smooth_ribbon_sample(self, sample, ctx, flags, p, calc_ahead, (const VEC3*)&p->world_pos, a, b, width,
                                step * index);
        ef_smooth_ribbon_curve(self, steps, &samples[old], &samples[prev], sample, flags);
    }
    samples[3].tex_t = step * index;
    {
        EfRibbonParam* last = &samples[cur];

        ef_smooth_ribbon_curve(self, steps, &samples[prev], last, &samples[3], flags);
        samples[4].tex_t = step * (index + dir);
        ef_smooth_ribbon_curve(self, steps, last, &samples[3], &samples[4], flags);
        ef_smooth_ribbon_end(self, last, &samples[3], &samples[4], flags);
    }
    ef_smooth_gx_end();
}

/* 0x800C20E4 (0x5C0): a smooth ribbon that also reaches back to the emitter: the eldest (or youngest) end is moved
 * by the youngest particle's offset from the emitter origin. */
void ef_smooth_ribbon_to_emitter(nw4r::ef::DrawSmoothStripeStrategy* self,
                                 nw4r::ef::DrawSmoothStripeStrategy::AheadContext* ctx, s32 steps, u32 flags,
                                 const VEC3* a, const VEC3* b) {
    EfDrawParticleManager* pm;
    EfEmitterDrawSetting* ed;
    nw4r::ef::DrawStrategyImpl::CalcAheadFunc calc_ahead;
    u32 order;
    nw4r::ef::DrawStrategyImpl::GetFirstDrawParticleFunc first;
    nw4r::ef::DrawStrategyImpl::GetNextDrawParticleFunc next;
    f32 width;
    EfDrawParticle* youngest;
    VEC3 offset;
    s32 count;
    f32 step;
    s32 index;
    s32 dir;
    EfRibbonParam samples[3];
    VEC3 unused_a;
    VEC3 unused_b;
    s32 prev;
    s32 cur;
    EfDrawParticle* p;

    if (!IsValidPointer((u32)ctx)) {
        nw4r::db::Panic(ef_smooth_file_str, 566, ef_smooth_ahead_assert_str, ctx);
    }
    pm = ctx->particle_manager;
    ed = (EfEmitterDrawSetting*)ef_resource_draw_setting(pm->resource);
    calc_ahead = self->GetCalcAheadFunc(pm);
    order = ef_smooth_draw_order(self, ed);
    first = self->GetGetFirstDrawParticleFunc(order);
    next = self->GetGetNextDrawParticleFunc(order);
    width = 0.01f * ed->scale_a;
    youngest = (EfDrawParticle*)ef_pm_first_alive((EfDrawList*)ctx->particle_manager);
    if (!youngest) {
        nw4r::db::Panic(ef_smooth_file_str, 579, ef_smooth_youngest_assert_str);
    }
    subVec3(&offset, &ctx->emitter_center, &youngest->world_pos);
    count = ef_stripe_draw_count(self, pm);
    GXBegin(0x98, 0, ((count + 1) * steps + 1) * 2);
    if (ef_smooth_tex_mode(self, ed) == 0x40) {
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
    ef_smooth_ribbon_param_ctor(&samples[0]);
    ef_smooth_ribbon_param_ctor(&samples[1]);
    ef_smooth_ribbon_param_ctor(&samples[2]);
    prev = 0;
    cur = 0;
    VEC3_ctor(&unused_a);
    VEC3_ctor(&unused_b);
    p = first(pm);
    ef_smooth_ribbon_sample(self, &samples[0], ctx, flags, p, calc_ahead, (const VEC3*)&p->world_pos, a, b, width,
                            step * index);
    index += dir;
    if (order == 0) {
        cur = 1;
        ef_smooth_ribbon_param_copy(&samples[1], &samples[0]);
        samples[1].tex_t = step * index;
        addVec3To(&samples[0].a, &offset);
        addVec3To(&samples[0].b, &offset);
        index += dir;
        ef_smooth_ribbon_curve(self, steps, &samples[0], &samples[0], &samples[1], flags);
    }
    for (p = next(pm, p); p != NULL; p = next(pm, p), index += dir) {
        s32 old = prev;
        EfRibbonParam* sample;

        prev = cur;
        cur = (cur + 1) % 3;
        sample = &samples[cur];
        ef_smooth_ribbon_sample(self, sample, ctx, flags, p, calc_ahead, (const VEC3*)&p->world_pos, a, b, width,
                                step * index);
        ef_smooth_ribbon_curve(self, steps, &samples[old], &samples[prev], sample, flags);
    }
    if (order != 0) {
        s32 old = prev;
        EfRibbonParam* last;
        EfRibbonParam* sample;
        EfRibbonParam moved;
        VEC3 moved_a;
        VEC3 moved_b;

        prev = cur;
        cur = (cur + 1) % 3;
        last = &samples[prev];
        sample = &samples[cur];
        addVec3(&moved_b, &last->b, &offset);
        addVec3(&moved_a, &last->a, &offset);
        ef_smooth_ribbon_param_copy(sample, ef_smooth_ribbon_param_set(&moved, &moved_a, &moved_b, step * index));
        ef_smooth_ribbon_curve(self, steps, &samples[old], last, sample, flags);
    }
    {
        EfRibbonParam* last = &samples[cur];
        EfRibbonParam* before = &samples[prev];

        ef_smooth_ribbon_curve(self, steps, before, last, last, flags);
        ef_smooth_ribbon_end(self, before, last, last, flags);
    }
    ef_smooth_gx_end();
}

/* Builds a ribbon sample from two edge points and a texture coordinate, and returns it. */
EfRibbonParam* ef_smooth_ribbon_param_set(EfRibbonParam* self, const VEC3* a, const VEC3* b, f32 t) {
    assignVec3((Vec*)&self->a, (Vec*)a);
    assignVec3((Vec*)&self->b, (Vec*)b);
    self->tex_t = t;
    return self;
}

/* Copies a ribbon sample. */
void ef_smooth_ribbon_param_copy(EfRibbonParam* dst, const EfRibbonParam* src) {
    *dst = *src;
}

/* 0x800C2738 (0x1B4): draws the smooth ribbon through the builder the stripe's connection type selects. */
void ef_smooth_draw_ribbon(nw4r::ef::DrawSmoothStripeStrategy* self,
                           nw4r::ef::DrawSmoothStripeStrategy::AheadContext* ctx, s32 steps, u32 flags,
                           const VEC3* a, const VEC3* b) {
    if (!IsValidPointer((u32)ctx)) {
        nw4r::db::Panic(ef_smooth_file_str, 670, ef_smooth_ahead_assert_str, ctx);
    }
    switch (ef_smooth_connect_type(
        self, (EfEmitterDrawSetting*)ef_resource_draw_setting(ctx->particle_manager->resource))) {
    default:
        ef_smooth_ribbon_open(self, ctx, steps, flags, a, b);
        break;
    case 1:
        ef_smooth_ribbon_loop(self, ctx, steps, flags, a, b);
        break;
    case 2:
        ef_smooth_ribbon_to_emitter(self, ctx, steps, flags, a, b);
        break;
    }
}

/* The connection-type bits (0-2) of the draw setting's stripe flags. */
s32 ef_smooth_connect_type(nw4r::ef::DrawSmoothStripeStrategy* self, const EfEmitterDrawSetting* ed) {
    return ed->stripe_connect & 0x7;
}

/* 0x800C28F8 (0x448): an open smooth tube: one ring per particle, the strips of the curve through every three
 * neighbouring rings and the closing curve at the last one. */
void ef_smooth_tube_open(nw4r::ef::DrawSmoothStripeStrategy* self,
                         nw4r::ef::DrawSmoothStripeStrategy::AheadContext* ctx, s32 steps, u32 flags) {
    EfDrawParticleManager* pm;
    EfEmitterDrawSetting* ed;
    nw4r::ef::DrawStrategyImpl::CalcAheadFunc calc_ahead;
    u32 order;
    nw4r::ef::DrawStrategyImpl::GetFirstDrawParticleFunc first;
    nw4r::ef::DrawStrategyImpl::GetNextDrawParticleFunc next;
    s32 count;
    f32 offset_x;
    f32 offset_y;
    f32 step;
    s32 index;
    s32 dir;
    EfStripeParam rings[3];
    s32 prev;
    s32 cur;
    EfDrawParticle* p;

    if (!IsValidPointer((u32)ctx)) {
        nw4r::db::Panic(ef_smooth_file_str, 698, ef_smooth_ahead_assert_str, ctx);
    }
    pm = ctx->particle_manager;
    ed = (EfEmitterDrawSetting*)ef_resource_draw_setting(pm->resource);
    calc_ahead = self->GetCalcAheadFunc(pm);
    order = ef_smooth_draw_order(self, ed);
    first = self->GetGetFirstDrawParticleFunc(order);
    next = self->GetGetNextDrawParticleFunc(order);
    count = ef_stripe_draw_count(self, pm);
    offset_x = 0.01f * ed->scale_a;
    offset_y = 0.01f * ed->scale_b;
    if (ef_smooth_tex_mode(self, ed) == 0x40) {
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
    ef_smooth_tube_param_ctor(&rings[0]);
    ef_smooth_tube_param_ctor(&rings[1]);
    ef_smooth_tube_param_ctor(&rings[2]);
    prev = 0;
    cur = 0;
    p = first(pm);
    ef_smooth_tube_ring(self, &rings[0], ctx, flags, p, calc_ahead, offset_x, offset_y, step * index);
    index += dir;
    for (p = next(pm, p); p != NULL; p = next(pm, p), index += dir) {
        s32 old = prev;
        EfStripeParam* ring;

        prev = cur;
        cur = (cur + 1) % 3;
        ring = &rings[cur];
        ef_smooth_tube_ring(self, ring, ctx, flags, p, calc_ahead, offset_x, offset_y, step * index);
        ef_smooth_tube_curve(self, steps, ef_smooth_tube_divide(self, ed), &rings[old], &rings[prev], ring, flags,
                             ctx->trig_table);
    }
    {
        EfStripeParam* last = &rings[cur];

        ef_smooth_tube_curve(self, steps, ef_smooth_tube_divide(self, ed), &rings[prev], last, last, flags,
                             ctx->trig_table);
    }
}

/* The tube side count of the draw setting. */
s32 ef_smooth_tube_divide(nw4r::ef::DrawSmoothStripeStrategy* self, const EfEmitterDrawSetting* ed) {
    return ed->type_option2;
}

/* 0x800C2D48 (0x51C): a looped smooth tube: the open tube plus the curves that close it through the first two rings
 * (sampled first, at half steps). */
void ef_smooth_tube_loop(nw4r::ef::DrawSmoothStripeStrategy* self,
                         nw4r::ef::DrawSmoothStripeStrategy::AheadContext* ctx, s32 steps, u32 flags) {
    EfDrawParticleManager* pm;
    EfEmitterDrawSetting* ed;
    nw4r::ef::DrawStrategyImpl::CalcAheadFunc calc_ahead;
    u32 order;
    nw4r::ef::DrawStrategyImpl::GetFirstDrawParticleFunc first;
    nw4r::ef::DrawStrategyImpl::GetNextDrawParticleFunc next;
    s32 count;
    f32 offset_x;
    f32 offset_y;
    f32 step;
    s32 index;
    s32 dir;
    EfStripeParam rings[5];
    s32 prev;
    s32 cur;
    EfDrawParticle* p;

    if (!IsValidPointer((u32)ctx)) {
        nw4r::db::Panic(ef_smooth_file_str, 764, ef_smooth_ahead_assert_str, ctx);
    }
    pm = ctx->particle_manager;
    ed = (EfEmitterDrawSetting*)ef_resource_draw_setting(pm->resource);
    calc_ahead = self->GetCalcAheadFunc(pm);
    order = ef_smooth_draw_order(self, ed);
    first = self->GetGetFirstDrawParticleFunc(order);
    next = self->GetGetNextDrawParticleFunc(order);
    count = ef_stripe_draw_count(self, pm);
    offset_x = 0.01f * ed->scale_a;
    offset_y = 0.01f * ed->scale_b;
    if (ef_smooth_tex_mode(self, ed) == 0x40) {
        step = 0.5f;
    } else {
        step = 0.5f / count;
    }
    index = -1;
    dir = 2;
    if (order != 0) {
        index = count * 2 + 1;
        dir = -2;
    }
    ef_smooth_tube_param_ctor(&rings[0]);
    ef_smooth_tube_param_ctor(&rings[1]);
    ef_smooth_tube_param_ctor(&rings[2]);
    ef_smooth_tube_param_ctor(&rings[3]);
    ef_smooth_tube_param_ctor(&rings[4]);
    p = first(pm);
    ef_smooth_tube_ring(self, &rings[3], ctx, flags, p, calc_ahead, offset_x, offset_y, step * index);
    p = next(pm, p);
    index += dir;
    ef_smooth_tube_ring(self, &rings[4], ctx, flags, p, calc_ahead, offset_x, offset_y, step * index);
    p = next(pm, p);
    index += dir;
    prev = 3;
    cur = 4;
    for (; p != NULL; p = next(pm, p), index += dir) {
        s32 old = prev;
        EfStripeParam* ring;

        prev = cur;
        cur = (cur + 1) % 3;
        ring = &rings[cur];
        ef_smooth_tube_ring(self, ring, ctx, flags, p, calc_ahead, offset_x, offset_y, step * index);
        ef_smooth_tube_curve(self, steps, ef_smooth_tube_divide(self, ed), &rings[old], &rings[prev], ring, flags,
                             ctx->trig_table);
    }
    rings[3].tex_t = step * index;
    {
        EfStripeParam* last = &rings[cur];

        ef_smooth_tube_curve(self, steps, ef_smooth_tube_divide(self, ed), &rings[prev], last, &rings[3], flags,
                             ctx->trig_table);
        rings[4].tex_t = step * (index + dir);
        ef_smooth_tube_curve(self, steps, ef_smooth_tube_divide(self, ed), last, &rings[3], &rings[4], flags,
                             ctx->trig_table);
    }
}

/* 0x800C3264 (0x5D0): a smooth tube that also reaches back to the emitter: the eldest (or youngest) end ring is
 * moved by the youngest particle's offset from the emitter origin. */
void ef_smooth_tube_to_emitter(nw4r::ef::DrawSmoothStripeStrategy* self,
                               nw4r::ef::DrawSmoothStripeStrategy::AheadContext* ctx, s32 steps, u32 flags) {
    EfDrawParticleManager* pm;
    EfEmitterDrawSetting* ed;
    nw4r::ef::DrawStrategyImpl::CalcAheadFunc calc_ahead;
    u32 order;
    nw4r::ef::DrawStrategyImpl::GetFirstDrawParticleFunc first;
    nw4r::ef::DrawStrategyImpl::GetNextDrawParticleFunc next;
    f32 offset_x;
    f32 offset_y;
    EfDrawParticle* youngest;
    VEC3 offset;
    s32 count;
    f32 step;
    s32 index;
    s32 dir;
    EfStripeParam rings[3];
    s32 prev;
    s32 cur;
    EfDrawParticle* p;

    if (!IsValidPointer((u32)ctx)) {
        nw4r::db::Panic(ef_smooth_file_str, 845, ef_smooth_ahead_assert_str, ctx);
    }
    pm = ctx->particle_manager;
    ed = (EfEmitterDrawSetting*)ef_resource_draw_setting(pm->resource);
    calc_ahead = self->GetCalcAheadFunc(pm);
    order = ef_smooth_draw_order(self, ed);
    first = self->GetGetFirstDrawParticleFunc(order);
    next = self->GetGetNextDrawParticleFunc(order);
    offset_x = 0.01f * ed->scale_a;
    offset_y = 0.01f * ed->scale_b;
    youngest = (EfDrawParticle*)ef_pm_first_alive((EfDrawList*)ctx->particle_manager);
    if (!youngest) {
        nw4r::db::Panic(ef_smooth_file_str, 859, ef_smooth_youngest_assert_str);
    }
    subVec3(&offset, &ctx->emitter_center, &youngest->world_pos);
    count = ef_stripe_draw_count(self, pm);
    if (ef_smooth_tex_mode(self, ed) == 0x40) {
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
    ef_smooth_tube_param_ctor(&rings[0]);
    ef_smooth_tube_param_ctor(&rings[1]);
    ef_smooth_tube_param_ctor(&rings[2]);
    prev = 0;
    cur = 0;
    p = first(pm);
    ef_smooth_tube_ring(self, &rings[0], ctx, flags, p, calc_ahead, offset_x, offset_y, step * index);
    index += dir;
    if (order == 0) {
        EfStripeParam moved;

        cur = 1;

        ef_smooth_tube_param_copy(&rings[1], ef_smooth_tube_param_set(&moved, &rings[0].center, &rings[0].side,
                                                                      &rings[0].up, step * index));
        addVec3To(&rings[0].center, &offset);
        index += dir;
        ef_smooth_tube_curve(self, steps, ef_smooth_tube_divide(self, ed), &rings[0], &rings[0], &rings[1], flags,
                             ctx->trig_table);
    }
    for (p = next(pm, p); p != NULL; p = next(pm, p), index += dir) {
        s32 old = prev;
        EfStripeParam* ring;

        prev = cur;
        cur = (cur + 1) % 3;
        ring = &rings[cur];
        ef_smooth_tube_ring(self, ring, ctx, flags, p, calc_ahead, offset_x, offset_y, step * index);
        ef_smooth_tube_curve(self, steps, ef_smooth_tube_divide(self, ed), &rings[old], &rings[prev], ring, flags,
                             ctx->trig_table);
    }
    if (order != 0) {
        s32 old = prev;
        EfStripeParam* last;
        EfStripeParam* ring;
        EfStripeParam moved;
        VEC3 moved_center;

        prev = cur;
        cur = (cur + 1) % 3;
        last = &rings[prev];
        ring = &rings[cur];
        addVec3(&moved_center, &last->center, &offset);
        ef_smooth_tube_param_copy(ring, ef_smooth_tube_param_set(&moved, &moved_center, &last->side, &last->up,
                                                                 step * index));
        ef_smooth_tube_curve(self, steps, ef_smooth_tube_divide(self, ed), &rings[old], last, ring, flags,
                             ctx->trig_table);
    }
    {
        EfStripeParam* last = &rings[cur];

        ef_smooth_tube_curve(self, steps, ef_smooth_tube_divide(self, ed), &rings[prev], last, last, flags,
                             ctx->trig_table);
    }
}

/* Builds a tube ring from its three vectors and a texture coordinate, and returns it. */
EfStripeParam* ef_smooth_tube_param_set(EfStripeParam* self, const VEC3* center, const VEC3* side, const VEC3* up,
                                        f32 t) {
    assignVec3((Vec*)&self->center, (Vec*)center);
    assignVec3((Vec*)&self->side, (Vec*)side);
    assignVec3((Vec*)&self->up, (Vec*)up);
    self->tex_t = t;
    return self;
}

/* 0x800C38A4 (0x454): draws the smooth stripe as a tube: builds the (cos, sin) table of its sides on the stack, then
 * draws the back faces and the front faces through the connection type's builder. */
void ef_smooth_draw_tube(nw4r::ef::DrawSmoothStripeStrategy* self,
                         nw4r::ef::DrawSmoothStripeStrategy::AheadContext* ctx, s32 steps, u32 flags, u32 flush) {
    EfEmitterDrawSetting* ed;
    s32 divide;

    if (!IsValidPointer((u32)ctx)) {
        nw4r::db::Panic(ef_smooth_file_str, 944, ef_smooth_ahead_assert_str, ctx);
    }
    ed = (EfEmitterDrawSetting*)ef_resource_draw_setting(ctx->particle_manager->resource);
    if (!(3 <= ef_smooth_tube_divide(self, ed))) {
        nw4r::db::Panic(ef_smooth_file_str, 949, ef_smooth_tube_divide_assert_str);
    }
    divide = ef_smooth_tube_divide(self, ed);
    if (divide >= 3) {
        f32* trig = (f32*)__alloca((divide + 1) * 8);

        if (!IsValidPointer((u32)trig)) {
            nw4r::db::Panic(ef_smooth_file_str, 959, ef_smooth_trig_assert_str, trig);
        }
        if (trig != NULL) {
            f32 step = 360.0f / divide;
            f32* pair;
            s32 i;

            trig[0] = 1.0f;
            trig[1] = 0.0f;
            for (i = 1, pair = trig + 2; i < divide; pair += 2, i++) {
                sin_cos_deg(&pair[1], &pair[0], step * i);
            }
            trig[divide * 2] = 1.0f;
            trig[divide * 2 + 1] = 0.0f;
            ctx->trig_table = trig;
            switch (ef_smooth_connect_type(self, ed)) {
            default:
                ef_smooth_begin_side(1, flush, flags);
                ef_smooth_tube_open(self, ctx, steps, flags);
                ef_smooth_begin_side(2, flush, flags);
                ef_smooth_tube_open(self, ctx, steps, flags);
                break;
            case 1:
                ef_smooth_begin_side(1, flush, flags);
                ef_smooth_tube_loop(self, ctx, steps, flags);
                ef_smooth_begin_side(2, flush, flags);
                ef_smooth_tube_loop(self, ctx, steps, flags);
                break;
            case 2:
                ef_smooth_begin_side(1, flush, flags);
                ef_smooth_tube_to_emitter(self, ctx, steps, flags);
                ef_smooth_begin_side(2, flush, flags);
                ef_smooth_tube_to_emitter(self, ctx, steps, flags);
                break;
            }
        }
    }
}

/* Draws an empty eight-vertex strip (the GP flush) with the cull mode set, when `enabled`. */
void ef_smooth_begin_side(u32 cull_mode, u32 enabled, u32 flags) {
    GXSetCullMode(cull_mode);
    if (enabled != 0) {
        GXBegin(0x98, 0, 8);
        for (int i = 0; i < 8; i++) {
            ef_smooth_gx_position3(0.0f, 0.0f, 0.0f);
            if (ef_smooth_flag_tex(flags)) {
                ef_smooth_gx_texcoord(0.0f, 0.0f);
            }
        }
        ef_smooth_gx_end();
    }
}

}  // extern "C"

/* A function-local static edge offset: constructed through `setVec3` on first use. */
struct EfSmoothAxis : public VEC3 {
    EfSmoothAxis(f32 x, f32 y, f32 z) { setVec3(this, x, y, z); }
}; /* size: 0xC */

namespace nw4r {
namespace ef {

/* 0x800C3D88 (0x7A0): draws the manager's particles as one smooth stripe: each particle's ahead vector first, then
 * either one strip per particle (the short connection modes) or the whole stripe, as a ribbon, a cross or a tube. */
void DrawSmoothStripeStrategy::Draw(const EfDrawInfo& info, EfDrawParticleManager* pm) {
    EfEmitterDrawSetting* ed;
    s32 count;

    if (!IsValidPointer((u32)pm)) {
        nw4r::db::Panic(ef_smooth_file_str, 1003, ef_smooth_pm_assert_str, pm);
    }
    EF_VALID_PTR_ASSERT(ef_smooth_file_str, 1005, ef_smooth_resource_assert_str, pm->resource);
    ed = (EfEmitterDrawSetting*)ef_resource_draw_setting(pm->resource);
    if (!IsValidPointer((u32)ed)) {
        nw4r::db::Panic(ef_smooth_file_str, 1007, ef_smooth_ed_assert_str, ed);
    }
    count = ef_stripe_draw_count(this, pm);
    if (count != 0) {
        MTX34 view_mtx;
        EfDrawParticle* p;
        u32 flags;
        u32 order;
        MTX34 mtx;
        VEC3 ahead;

        MTX34_ctor(&view_mtx);
        ef_draw_info_view_mtx(&info, &view_mtx);
        AheadContext ctx(&view_mtx, pm);
        ef_smooth_first_ahead(&ahead, this, ed, &ctx);
        for (p = (EfDrawParticle*)ef_pm_list_head((EfParticleState*)pm); p != NULL;
             p = (EfDrawParticle*)ef_pm_list_next(pm, p)) {
            if (p->ahead.x <= 1.0f) {
                break;
            }
            copyVec3(&p->ahead, &ahead);
        }

        flags = (ef_smooth_draw_order(this, ed) ? 0x10 : 0) | (ed->type_option == 2 ? 8 : 0);
        order = ef_smooth_draw_order(this, ed);
        MTX34_ctor(&mtx);
        ef_draw_info_view_mtx(&info, &mtx);
        mtx34_concat(&mtx, &mtx, &ctx.manager_mtx);
        if (ef_smooth_flag_facing(flags)) {
            MTX34 inv;
            MTX34_ctor(&inv);
            if (mtx34_inverse(&inv, &mtx)) {
                VEC3 z;
                copyVec3(&ctx.view_axis_z, setVec3(&z, inv.m[0][2], inv.m[1][2], inv.m[2][2]));
            } else {
                VEC3 z;
                copyVec3(&ctx.view_axis_z, setVec3(&z, 0.0f, 0.0f, 1.0f));
            }
        }

        {
            s32 connect = ef_smooth_connect_type(this, ed);

            if ((connect == 1 && count < 3) || (connect == 0 && count < 2)) {
                CalcAheadFunc calc_ahead = GetCalcAheadFunc(pm);
                GetFirstDrawParticleFunc first = GetGetFirstDrawParticleFunc(order);
                GetNextDrawParticleFunc next = GetGetNextDrawParticleFunc(order);

                for (p = first(pm); p != NULL; p = next(pm, p)) {
                    VEC3 axis;
                    MTX34 frame;

                    VEC3_ctor(&axis);
                    calc_ahead(&axis, &ctx, p);
                    ef_smooth_draw_segment(&frame, this, &ctx, flags, p, &axis, (VEC3*)&p->world_pos);
                }
                return;
            }
        }

        ef_smooth_setup_gx(this, &info, pm);
        GXLoadPosMtxImm((const f32(*)[4])mtx34_get_ptr(&mtx), 0);
        flags |= (mNumTexmap != 0);
        p = GetGetFirstDrawParticleFunc(ef_smooth_draw_order(this, ed))(pm);
        SetupGP(p, *ed, info, true, false);
        {
            s32 steps = ef_smooth_curve_steps(this, ed);
            if (steps == 0) {
                steps = 1;
            }
            static EfSmoothAxis ef_smooth_axis_x(1.0f, 0.0f, 0.0f);
            static EfSmoothAxis ef_smooth_axis_neg_x(-1.0f, 0.0f, 0.0f);
            static EfSmoothAxis ef_smooth_axis_z(0.0f, 0.0f, 1.0f);
            static EfSmoothAxis ef_smooth_axis_neg_z(0.0f, 0.0f, -1.0f);

            switch (ed->type_option) {
            default: {
                ef_smooth_draw_ribbon(this, &ctx, steps, flags, &ef_smooth_axis_x, &ef_smooth_axis_neg_x);
                if (ed->type_option == 1) {
                    ef_smooth_draw_ribbon(this, &ctx, steps, flags, &ef_smooth_axis_z, &ef_smooth_axis_neg_z);
                }
                break;
            }
            case 3: {
                u32 flush = 0;
                if ((ed->flags & 8) != 0 || p->manager->emitter->effect->system->flush_gp == 0) {
                    flush = 1;
                }
                ef_smooth_draw_tube(this, &ctx, steps, flags, flush);
                break;
            }
            }
        }
    }
}

}  // namespace ef
}  // namespace nw4r

extern "C" {

/* The curve subdivision count of the draw setting. */
s32 ef_smooth_curve_steps(nw4r::ef::DrawSmoothStripeStrategy* self, const EfEmitterDrawSetting* ed) {
    return ed->type_option3;
}

/* 0x800C4530 (0x1B8): sets the stripe's GX state: the strategy's texture/TEV/channel setup, then a direct position
 * and texcoord vertex format at the current matrix. */
void ef_smooth_setup_gx(nw4r::ef::DrawSmoothStripeStrategy* self, const EfDrawInfo* info, EfDrawParticleManager* pm) {
    if (!IsValidPointer((u32)pm)) {
        nw4r::db::Panic(ef_smooth_file_str, 1126, ef_smooth_pm_assert_str, pm);
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

/* 0x800C46E8 (0x23C): the stripe's first ahead vector: for direction type 7 one manager axis (or their sum) by the
 * stripe's axis bits, otherwise the emitter's X or Z axis (or a small diagonal) taken into manager space;
 * normalised, falling back to the emitter's Y axis. */
void ef_smooth_first_ahead(VEC3* out, nw4r::ef::DrawSmoothStripeStrategy* self, EfEmitterDrawSetting* ed,
                           nw4r::ef::DrawSmoothStripeStrategy::AheadContext* ctx) {
    VEC3 diagonal;
    VEC3 axis_x;
    VEC3 axis_y;
    VEC3 axis_z;
    VEC3 sum_xyz;
    VEC3 sum_xy;
    VEC3 x;
    VEC3 y;
    VEC3 z;

    VEC3_ctor(out);
    if (ed->type_direction == 7) {
        switch (ef_smooth_axis_mode(self, ed)) {
        case 8:
            copyVec3(out, setVec3(&axis_x, ctx->manager_mtx_inv.m[0][0], ctx->manager_mtx_inv.m[1][0],
                                  ctx->manager_mtx_inv.m[2][0]));
            break;
        default:
            copyVec3(out, setVec3(&axis_y, ctx->manager_mtx_inv.m[0][1], ctx->manager_mtx_inv.m[1][1],
                                  ctx->manager_mtx_inv.m[2][1]));
            break;
        case 16:
            copyVec3(out, setVec3(&axis_z, ctx->manager_mtx_inv.m[0][2], ctx->manager_mtx_inv.m[1][2],
                                  ctx->manager_mtx_inv.m[2][2]));
            break;
        case 24: {
            VEC3* py;
            VEC3* pz;

            pz = setVec3(&z, ctx->manager_mtx_inv.m[0][2], ctx->manager_mtx_inv.m[1][2],
                               ctx->manager_mtx_inv.m[2][2]);
            py = setVec3(&y, ctx->manager_mtx_inv.m[0][1], ctx->manager_mtx_inv.m[1][1],
                               ctx->manager_mtx_inv.m[2][1]);
            addVec3(&sum_xy, setVec3(&x, ctx->manager_mtx_inv.m[0][0], ctx->manager_mtx_inv.m[1][0],
                                     ctx->manager_mtx_inv.m[2][0]), py);
            addVec3(&sum_xyz, &sum_xy, pz);
            copyVec3(out, &sum_xyz);
            break;
        }
        }
    } else {
        switch (ef_smooth_axis_mode(self, ed)) {
        case 8:
            mtx34_rotate_vec3(out, &ctx->emitter_mtx, ef_get_unit_x_vec());
            mtx34_rotate_vec3(out, &ctx->manager_mtx_inv, out);
            break;
        default:
            copyVec3(out, &ctx->emitter_axis_y);
            return;
        case 16:
            mtx34_rotate_vec3(out, &ctx->emitter_mtx, ef_get_unit_z_vec());
            mtx34_rotate_vec3(out, &ctx->manager_mtx_inv, out);
            break;
        case 24:
            setVec3(&diagonal, 1.0f, 1.0f, 1.0f);
            mtx34_rotate_vec3(out, &ctx->emitter_mtx, &diagonal);
            mtx34_rotate_vec3(out, &ctx->manager_mtx_inv, out);
            break;
        }
    }
    if (ef_vec3_normalize(out) == 0) {
        copyVec3(out, &ctx->emitter_axis_y);
    }
}

/* The first-ahead axis bits (3-5) of the draw setting's stripe flags. */
s32 ef_smooth_axis_mode(nw4r::ef::DrawSmoothStripeStrategy* self, const EfEmitterDrawSetting* ed) {
    return ed->stripe_connect & 0x38;
}

}  // extern "C"

namespace nw4r {
namespace ef {

/* 0x800C4930 (0x3BC): the ahead-vector builder the draw setting's direction type (and, for type 6, the stripe's
 * connection mode) selects. */
DrawStrategyImpl::CalcAheadFunc DrawSmoothStripeStrategy::GetCalcAheadFunc(EfDrawParticleManager* pm) {
    EfEmitterDrawSetting* ed;

    if (!IsValidPointer((u32)pm)) {
        nw4r::db::Panic(ef_smooth_file_str, 1226, ef_smooth_pm_assert_str, pm);
    }
    EF_VALID_PTR_ASSERT(ef_smooth_file_str, 1227, ef_smooth_resource_assert_str, pm->resource);
    ed = (EfEmitterDrawSetting*)ef_resource_draw_setting(pm->resource);
    if (!IsValidPointer((u32)ed)) {
        nw4r::db::Panic(ef_smooth_file_str, 1230, ef_smooth_ed_assert_str, ed);
    }
    switch (ed->type_direction) {
    case 0:
        return (CalcAheadFunc)ef_ahead_move_dir;
    case 1:
        return (CalcAheadFunc)ef_ahead_from_emitter;
    case 2:
        return (CalcAheadFunc)ef_ahead_emitter_axis_y;
    case 3:
        return ef_smooth_ahead_type3;
    case 5:
    case 7:
        return (CalcAheadFunc)ef_ahead_manager_axis_y;
    case 6:
        switch (ed->stripe_connect & 7) {
        case 1:
            return ef_smooth_ahead_type6_link1;
        case 2:
            return ef_smooth_ahead_type6_link2;
        default:
            return ef_smooth_ahead_type6;
        }
    default:
        return (CalcAheadFunc)ef_ahead_move_dir;
    }
}

/* 0x800C4CEC (0x15C): the base ahead context plus the emitter's X axis in manager space. */
DrawSmoothStripeStrategy::AheadContext::AheadContext(const MTX34* view, EfDrawParticleManager* pm)
    : DrawStrategyImpl::AheadContext(view, pm) {
    VEC3 axis;

    VEC3_ctor(&emitter_axis_x);
    VEC3_ctor(&view_axis_z);
    if (!IsValidPointer((u32)pm)) {
        nw4r::db::Panic(ef_smooth_file_str, 1265, ef_smooth_pm_assert_str, pm);
    }
    setVec3(&axis, emitter_mtx.m[0][0], emitter_mtx.m[1][0], emitter_mtx.m[2][0]);
    mtx34_rotate_vec3(&emitter_axis_x, &manager_mtx_inv, &axis);
}

}  // namespace ef
}  // namespace nw4r

extern "C" {

/* 0x800C4E48 (0x35C): the ahead vector toward the next elder particle (or, for the eldest, from the emitter origin),
 * normalised, falling back to the emitter's Y axis. */
void ef_smooth_ahead_type3(VEC3* out, nw4r::ef::DrawStrategyImpl::AheadContext* ctx, EfDrawParticle* p) {
    EfDrawParticle* next;

    if (!IsValidPointer((u32)out)) {
        nw4r::db::Panic(ef_smooth_file_str, 1278, ef_smooth_yaxis_assert_str, out);
    }
    if (!IsValidPointer((u32)ctx)) {
        nw4r::db::Panic(ef_smooth_file_str, 1279, ef_smooth_context_assert_str, ctx);
    }
    if (!IsValidPointer((u32)p)) {
        nw4r::db::Panic(ef_smooth_file_str, 1280, ef_smooth_particle_assert_str, p);
    }
    next = (EfDrawParticle*)ef_pm_prev_alive(ctx->particle_manager, p);
    if (next != NULL) {
        PSVECSubtract((f32*)out, (const f32*)&p->world_pos, (const f32*)&next->world_pos);
    } else {
        PSVECSubtract((f32*)out, (const f32*)&p->world_pos, (const f32*)&ctx->emitter_center);
    }
    if (ef_vec3_normalize(out) == 0) {
        copyVec3(out, &ctx->emitter_axis_y);
    }
}

/* 0x800C51A4 (0x400): the ahead vector across the particle's two neighbours (each direction normalised), falling
 * back to the emitter's Y axis. */
void ef_smooth_ahead_type6(VEC3* out, nw4r::ef::DrawStrategyImpl::AheadContext* ctx, EfDrawParticle* p) {
    EfDrawParticle* younger;
    EfDrawParticle* elder;
    VEC3 to_younger;
    VEC3 to_elder;
    VEC3 zero_a;
    VEC3 zero_b;

    if (!IsValidPointer((u32)out)) {
        nw4r::db::Panic(ef_smooth_file_str, 1305, ef_smooth_yaxis_assert_str, out);
    }
    if (!IsValidPointer((u32)ctx)) {
        nw4r::db::Panic(ef_smooth_file_str, 1306, ef_smooth_context_assert_str, ctx);
    }
    if (!IsValidPointer((u32)p)) {
        nw4r::db::Panic(ef_smooth_file_str, 1307, ef_smooth_particle_assert_str, p);
    }
    younger = (EfDrawParticle*)ef_pm_next_alive(ctx->particle_manager, p);
    elder = (EfDrawParticle*)ef_pm_prev_alive(ctx->particle_manager, p);
    setVec3(&to_younger, 0.0f, 0.0f, 0.0f);
    if (younger != NULL) {
        PSVECSubtract((f32*)&to_younger, (const f32*)&younger->world_pos, (const f32*)&p->world_pos);
        if (ef_vec3_normalize(&to_younger) == 0) {
            copyVec3(&to_younger, setVec3(&zero_a, 0.0f, 0.0f, 0.0f));
        }
    }
    setVec3(&to_elder, 0.0f, 0.0f, 0.0f);
    if (elder != NULL) {
        PSVECSubtract((f32*)&to_elder, (const f32*)&elder->world_pos, (const f32*)&p->world_pos);
        if (ef_vec3_normalize(&to_elder) == 0) {
            copyVec3(&to_elder, setVec3(&zero_b, 0.0f, 0.0f, 0.0f));
        }
    }
    PSVECSubtract((f32*)out, (const f32*)&to_younger, (const f32*)&to_elder);
    if (ef_vec3_normalize(out) == 0) {
        copyVec3(out, &ctx->emitter_axis_y);
    }
}

/* 0x800C55A4 (0x400): as the two-neighbour ahead vector, with the list wrapping around for a looped stripe. */
void ef_smooth_ahead_type6_link1(VEC3* out, nw4r::ef::DrawStrategyImpl::AheadContext* ctx, EfDrawParticle* p) {
    EfDrawParticle* younger;
    EfDrawParticle* elder;
    VEC3 to_younger;
    VEC3 to_elder;
    VEC3 zero_a;
    VEC3 zero_b;

    if (!IsValidPointer((u32)out)) {
        nw4r::db::Panic(ef_smooth_file_str, 1345, ef_smooth_yaxis_assert_str, out);
    }
    if (!IsValidPointer((u32)ctx)) {
        nw4r::db::Panic(ef_smooth_file_str, 1346, ef_smooth_context_assert_str, ctx);
    }
    if (!IsValidPointer((u32)p)) {
        nw4r::db::Panic(ef_smooth_file_str, 1347, ef_smooth_particle_assert_str, p);
    }
    younger = (EfDrawParticle*)ef_pm_next_alive(ctx->particle_manager, p);
    if (younger == NULL) {
        younger = (EfDrawParticle*)ef_pm_first_alive((EfDrawList*)ctx->particle_manager);
    }
    elder = (EfDrawParticle*)ef_pm_prev_alive(ctx->particle_manager, p);
    if (elder == NULL) {
        elder = (EfDrawParticle*)ef_pm_last_alive((EfParticleState*)ctx->particle_manager);
    }
    VEC3_ctor(&to_younger);
    PSVECSubtract((f32*)&to_younger, (const f32*)&younger->world_pos, (const f32*)&p->world_pos);
    if (ef_vec3_normalize(&to_younger) == 0) {
        copyVec3(&to_younger, setVec3(&zero_a, 0.0f, 0.0f, 0.0f));
    }
    VEC3_ctor(&to_elder);
    PSVECSubtract((f32*)&to_elder, (const f32*)&elder->world_pos, (const f32*)&p->world_pos);
    if (ef_vec3_normalize(&to_elder) == 0) {
        copyVec3(&to_elder, setVec3(&zero_b, 0.0f, 0.0f, 0.0f));
    }
    PSVECSubtract((f32*)out, (const f32*)&to_younger, (const f32*)&to_elder);
    if (ef_vec3_normalize(out) == 0) {
        copyVec3(out, &ctx->emitter_axis_y);
    }
}

/* 0x800C59A4 (0x3B8): as the two-neighbour ahead vector, with the eldest particle reaching to the emitter origin. */
void ef_smooth_ahead_type6_link2(VEC3* out, nw4r::ef::DrawStrategyImpl::AheadContext* ctx, EfDrawParticle* p) {
    EfDrawParticle* younger;
    EfDrawParticle* elder;
    VEC3 to_younger;
    VEC3 to_elder;

    if (!IsValidPointer((u32)out)) {
        nw4r::db::Panic(ef_smooth_file_str, 1386, ef_smooth_yaxis_assert_str, out);
    }
    if (!IsValidPointer((u32)ctx)) {
        nw4r::db::Panic(ef_smooth_file_str, 1387, ef_smooth_context_assert_str, ctx);
    }
    if (!IsValidPointer((u32)p)) {
        nw4r::db::Panic(ef_smooth_file_str, 1388, ef_smooth_particle_assert_str, p);
    }
    younger = (EfDrawParticle*)ef_pm_next_alive(ctx->particle_manager, p);
    elder = (EfDrawParticle*)ef_pm_prev_alive(ctx->particle_manager, p);
    setVec3(&to_younger, 0.0f, 0.0f, 0.0f);
    if (younger != NULL) {
        PSVECSubtract((f32*)&to_younger, (const f32*)&younger->world_pos, (const f32*)&p->world_pos);
        ef_vec3_normalize(&to_younger);
    }
    VEC3_ctor(&to_elder);
    if (elder != NULL) {
        PSVECSubtract((f32*)&to_elder, (const f32*)&elder->world_pos, (const f32*)&p->world_pos);
    } else {
        PSVECSubtract((f32*)&to_elder, (const f32*)&ctx->emitter_center, (const f32*)&p->world_pos);
    }
    ef_vec3_normalize(&to_elder);
    PSVECSubtract((f32*)out, (const f32*)&to_younger, (const f32*)&to_elder);
    if (ef_vec3_normalize(out) == 0) {
        copyVec3(out, &ctx->emitter_axis_y);
    }
}

}  // extern "C"

/* The destructor is the class's inline one, emitted at the end of this TU; retail calls the base destructor out of
 * line from it (`bl` + `extsh` of the delete flag), which a deferred inline body only gets with inlining off at its
 * point of emission. */
#pragma dont_inline on
