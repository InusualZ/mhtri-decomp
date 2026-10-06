/*
 * ef/ef_drawstripestrategy.cpp - nw4r::ef DrawStripeStrategy and the inline helpers its TU emits first: the class's
 *   constructor, the out-of-line DrawStrategyImpl/DrawStrategy destructors, `Draw`, its ahead context, the
 *   `Particle`/`ParticleManager` walkers, the draw-time particle copies and ahead-vector builders the billboard
 *   dispatch hands out, and the class's destructor, which closes the range.
 * RANGE. .text 0x800B4AC8-0x800B9A44 (60 functions); extab 0x8000A10C-0x8000A23C, extabindex 0x80023538-0x80023700,
 *   .data 0x805939E8-0x80593E88 (the `__FILE__` string "ef_drawstripestrategy.cpp" first, copies at 0x80593D0C,
 *   0x80593D5C and 0x80593DAC), .sdata2 0x80796120-0x80796150.  Left edge: `ef/ef_resource.cpp`
 *   ends there; right edge: `ef/ef_drawbillboardstrategy.cpp` starts there.
 * FLAGS. `cflags_main`; file-wide `#pragma peephole off`, `#pragma fp_contract off` and `#pragma dont_inline on`
 *   before the includes (the base destructors the constructor's unwind entry emits call their bases out of line,
 *   as retail's do); inlining comes back on after `fn_800B4FE0` and goes off again at the end (the class's own
 *   destructor).
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
 * RESIDUALS. 5 partial rows:
 *  - `Draw` (0x800B76C4): ours allocates 0x20 fewer bytes of stack temporaries (frame 0x210 against 0x230), so
 *    every local sits at another offset, and the asserted resource is loaded before the assert temporaries;
 *  - `GetCalcAheadFunc` (0x800B83CC): the same asserted-resource load placement;
 *  - `ef_stripe_draw_count` (ours 0xC of 0x10): retail keeps an inlined call's `b` to the next instruction;
 *  - `ef_draw_info_view_mtx`: retail reloads the depth offset for every product (ours keeps it in f2);
 *  - `ef_vec3_transform`: two saved values swap r30/r31.
 *   flipcheck: `.data` and `.sdata2` claimed, not emitted (declared by their map names, playbook 29).
 *   Relocation names that differ from retail (pool constants, save helpers, statics): `_savegpr_25`,
 *     `_savegpr_26`, `_restgpr_25`, `_restgpr_26`.
 *   `DrawStripeStrategy` (constructor): the empty body is the whole source - the compiler
 *     emits the base call and the vtable store.
 * SHAPES. `fn_800B51F8` takes the scale before the flags (the caller loads f1 before r5).  `EfStripeParam` (in
 *   `ef_drawstripestrategy.h`, shared with the smooth stripe) is the tube ring (three `VEC3` and the texture
 *   coordinate); its copy is the implicit member-wise assignment (words for the vectors, `lfs`/`stfs` for the
 *   scalar).  Stack-slot order follows the
 *   declaration order (first declared, highest offset), which the tube builders' locals rely on.
 */

#pragma peephole off
#pragma dont_inline on
#pragma fp_contract off

#include "types.h"
#include "gx.h"
#include "ef.h"
#include "ef/ef_drawstrategyimpl.h"
#include "ef/fn_800AEE48.h"
#include "ef/ef_drawstripestrategy.h" /* the ahead-vector builders and EfAheadItem (this unit's own header) */
#include "sys_mem.h"
#include "unsplit/ef.h"
#include "g3d/fn_80063888.h" /* fn_80067E54, owned by g3d/fn_80063888.cpp (rule 2) */
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "ef/ef_particle.h"   /* fn_800AB388 (rule 2) */
#include "RVLGX/GXSetTevOrder.h" /* GXLoadPosMtxImm (rule 2) */
#include "EXI/GXBegin.h"      /* the GX vertex-format entry points (rule 2) */
#include "g3d/g3d_calcview.h" /* fn_800710BC (rule 2) */
#include "nw4r/fn_805012C4.h" /* mtx34_rotate_vec3 (rule 2) */
#include "fn_8004CAD8.h"      /* MTX34_ctor, fn_80050508 (rule 2) */
#include "draw_shape/mtx34_copy.h" /* the matrix copy (rule 2) */
#include "g3d/fn_80075DCC.h"      /* fn_80077DF0 (rule 2) */
#include "g3d/g3d_anmchr.h"       /* math_reciprocal (rule 2) */
#include "nw4r/mtx34_mult_vec3.h" /* mtx34_mult_vec3 (rule 2) */
#include "g3d/g3d_calcworld.h"    /* addVec3To (rule 2) */
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


extern f32 lbl_80796140;    /* a stripe-strategy constant                                 .sdata2 0x80796140 */
extern f32 lbl_80796124; /* a scale factor  .sdata2 0x80796124 */
extern f32 lbl_80796130; /* a scale factor  .sdata2 0x80796130 */

/* This unit's `__FILE__` and pointer-assert strings (its claimed `.data`), declared, never defined. */
extern char ef_stripe_file_str[]; /* "ef_drawstripestrategy.cpp"                                  .data 0x805939E8 */
extern char ef_stripe_pm_assert_str[]; /* "NW4R:Pointer Error\npm(=%p) is not valid pointer."         .data 0x80593BA4 */
extern char ef_stripe_resource_assert_str[]; /* "NW4R:Pointer Error\npm->mResource(=%p) is not valid..."   .data 0x80593BD8 */
extern char ef_drawinfo_null_result_str[]; /* "NW4R:Pointer must not be NULL (result)"                    .data 0x80593E54 */
extern char ef_drawinfo_file_str[]; /* "drawinfo.h"                                                .data 0x80593E7C */
extern char ef_stripe_ahead_assert_str[];     /* "NW4R:Pointer Error\naheadContext(=%p) ..."  .data 0x80593A04 */
extern char ef_stripe_vertex_ahead_str[];     /* the same, an inline copy                       .data 0x80593CD0 */
extern char ef_stripe_vertex_file_str[];      /* "ef_drawstripestrategy.cpp", an inline copy    .data 0x80593D0C */
extern char ef_stripe_scale_pp_str[];         /* "NW4R:Pointer Error\npp(=%p) ...", an inline copy .data 0x80593D28 */
extern char ef_stripe_scale_file_str[];       /* "ef_drawstripestrategy.cpp", an inline copy    .data 0x80593D5C */
extern char ef_particle_rotate_assert_str[];  /* "NW4R:Pointer Error\nrotate(=%p) ..."         .data 0x80593DC8 */
extern char ef_particle_h_file_str[];         /* "particle.h"                                   .data 0x80593E00 */
extern char ef_pm_base_assert_str[];          /* "NW4R:Pointer Error\nbase(=%p) ..."           .data 0x80593E0C */
extern char ef_particlemanager_h_file_str[];  /* "particlemanager.h"                            .data 0x80593E40 */
extern f32 ef_stripe_percent; /* 0.01f           .sdata2 0x80796120 */
extern f32 ef_stripe_rotate_step; /* 2pi / 256       .sdata2 0x80796134 */
extern char ef_stripe_trig_assert_str[];        /* "NW4R:Pointer Error\ntrigonometric(=%p) ..." .data 0x80593B18 */
extern char ef_stripe_tube_divide_assert_str[]; /* "NW4R:Failed assertion 3 <= GetTubeDivide(ed)" .data 0x80593B74 */
extern f32 ef_stripe_full_circle; /* 360.0f          .sdata2 0x80796144 */
extern char ef_stripe_dst_assert_str[];        /* "NW4R:Pointer Error\ndst(=%p) ..."        .data 0x80593A40 */
extern char ef_stripe_context_assert_str[];    /* "NW4R:Pointer Error\ncontext(=%p) ..."    .data 0x80593A74 */
extern char ef_stripe_pp_assert_str[];         /* "NW4R:Pointer Error\npp(=%p) ..."         .data 0x80593AAC */
extern char ef_stripe_calc_ahead_assert_str[]; /* "NW4R:Pointer Error\ncalcAhead(=%p) ..."  .data 0x80593AE0 */
extern char ef_stripe_youngest_assert_str[];   /* "NW4R:Failed assertion youngest"          .data 0x80593B54 */
extern char ef_stripe_yaxis_assert_str[];    /* "NW4R:Pointer Error\nyAxis(=%p) ..."     .data 0x80593C48 */
extern char ef_stripe_particle_assert_str[]; /* "NW4R:Pointer Error\nparticle(=%p) ..."  .data 0x80593C7C */
extern char ef_stripe_inline2_pp_str[];   /* "NW4R:Pointer Error\npp(=%p) is not valid pointer."  .data 0x80593D78 */
extern char ef_stripe_inline3_file_str[]; /* "ef_drawstripestrategy.cpp" (an inline copy)        .data 0x80593DAC */
extern char ef_stripe_ed_assert_str[]; /* "NW4R:Pointer Error\n&ed(=%p) is not valid pointer."       .data 0x80593C14 */

/* Helpers owned by other units. */
extern void* fn_800508A8(void* arg);
extern void* fn_800508AC(void* arg);
extern void  PSMTXMultVec(const f32* mtx, const void* src, void* dst);
extern void  ef_particle_get_move_dir(void* arg, Vec* v);

/* Three positions the sample zeroer walks; `fn_800B6954` zeroes them one `Vec` at a time. */
typedef struct EfStripeSample {
    Vec a; /* +0x00 */
    Vec b; /* +0x0C */
    Vec c; /* +0x18 */
} EfStripeSample; /* size: 0x24 */

/* The per-instance byte offset a node manager carries. */
typedef struct EfNodeManager {
    u8 pad_0x00[0x42]; /* +0x00 */
    u16 field_0x42;    /* +0x42  byte offset from a node base to its link field */
} EfNodeManager; /* size: 0x44 */

/* The unit's own symbols that are used before their definition. */
void fn_800B4FE0(void);
void fn_800B5288(f32 x, f32 y);
int  fn_800B5298(u32 value);
void fn_800B52AC(Vec* v);
void fn_800B52BC(f32 x, f32 y, f32 z);
void* ef_pm_list_prev(void* self, void* node);
u32 ef_pm_list_tail(EfParticleState* self);
u32 fn_800B6994(void* ctx, EfParticleState* particle);

/* The ahead-vector builders `GetCalcAheadFunc` hands out (unwritten). */
void ef_stripe_ahead_type3(VEC3* out, nw4r::ef::DrawStrategyImpl::AheadContext* ctx, EfDrawParticle* p);
void ef_stripe_ahead_type6(VEC3* out, nw4r::ef::DrawStrategyImpl::AheadContext* ctx, EfDrawParticle* p);
void ef_stripe_ahead_type6_link1(VEC3* out, nw4r::ef::DrawStrategyImpl::AheadContext* ctx, EfDrawParticle* p);
void ef_stripe_ahead_type6_link2(VEC3* out, nw4r::ef::DrawStrategyImpl::AheadContext* ctx, EfDrawParticle* p);

/* The stripe builders `Draw` hands the particles to (unwritten). */
void ef_stripe_first_ahead(VEC3* out, nw4r::ef::DrawStripeStrategy* self, EfEmitterDrawSetting* ed,
                           nw4r::ef::DrawStripeStrategy::AheadContext* ctx);
s32 fn_800B83C0(void* ctx, EfParticleState* particle);
void ef_stripe_draw_segment(MTX34* out, nw4r::ef::DrawStripeStrategy* self,
                            nw4r::ef::DrawStripeStrategy::AheadContext* ctx, u32 flags, EfDrawParticle* p,
                            const VEC3* ahead, const VEC3* pos);
void ef_stripe_setup_gx(nw4r::ef::DrawStripeStrategy* self, const EfDrawInfo* info, EfDrawParticleManager* pm);
void ef_stripe_draw_ribbon(nw4r::ef::DrawStripeStrategy* self, nw4r::ef::DrawStripeStrategy::AheadContext* ctx,
                           u32 flags, const VEC3* a, const VEC3* b);
void ef_stripe_ribbon_vertex(nw4r::ef::DrawStripeStrategy* self, nw4r::ef::DrawStripeStrategy::AheadContext* ctx,
                             u32 flags, EfDrawParticle* p, nw4r::ef::DrawStrategyImpl::CalcAheadFunc calc_ahead,
                             const VEC3* pos, const VEC3* a, const VEC3* b, f32 width, f32 t);
void ef_stripe_scale_mtx(MTX34* out, nw4r::ef::DrawStripeStrategy* self, EfDrawParticle* p, f32 width);
void ef_pm_modify_rotate(EfDrawParticleManager* pm, const EfDrawParticle* p, VEC3* rotate);
s32 fn_800B5B48(void* ctx, EfParticleState* particle);
void fn_800B51F8(Vec* a, Vec* b, f32 scale, u32 flags);
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
EfStripeSample* fn_800B6954(EfStripeSample* self);
void fn_800B6900(EfStripeParam* dst, EfStripeParam* src);
int fn_800B6534(u32 value);
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
void fn_800B4FE0(void) {}
#pragma dont_inline off

/* 0x800B4BA4 (0x43C): draws the stripe as one ribbon strip: two vertices per particle (the `a`/`b` edge offsets
 * through each particle's frame), plus a closing particle when the stripe loops and the emitter origin when it
 * connects to the emitter. */
void ef_stripe_draw_ribbon(nw4r::ef::DrawStripeStrategy* self, nw4r::ef::DrawStripeStrategy::AheadContext* ctx,
                           u32 flags, const VEC3* a, const VEC3* b) {
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
        nw4r::db::Panic(ef_stripe_file_str, 275, ef_stripe_ahead_assert_str, ctx);
    }
    pm = ctx->particle_manager;
    ed = (EfEmitterDrawSetting*)ef_resource_draw_setting(pm->resource);
    calc_ahead = self->GetCalcAheadFunc(pm);
    order = ed->flags & 0x800;
    first = self->GetGetFirstDrawParticleFunc(order);
    next = self->GetGetNextDrawParticleFunc(order);
    count = ef_stripe_draw_count(self, pm);
    width = ef_stripe_percent * ed->scale_a;
    loop = (ed->stripe_connect & 7) == 1;
    to_emitter = (ed->stripe_connect & 7) == 2;
    if (loop || to_emitter) {
        count++;
    }
    GXBegin(0x98, 0, count * 2);
    if (fn_800B5B48(self, (EfParticleState*)ed) == 0x40) {
        step = lbl_80796124;
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
    fn_800B4FE0();
}

/* 0x800B4FE4 (0x214): the two ribbon vertices of one particle (or, for NULL, of the eldest one at `pos`): its frame
 * times its rotation-and-scale matrix applied to the edge offsets `a`/`b`, with texture coordinate `t`. */
void ef_stripe_ribbon_vertex(nw4r::ef::DrawStripeStrategy* self, nw4r::ef::DrawStripeStrategy::AheadContext* ctx,
                             u32 flags, EfDrawParticle* p, nw4r::ef::DrawStrategyImpl::CalcAheadFunc calc_ahead,
                             const VEC3* pos, const VEC3* a, const VEC3* b, f32 width, f32 t) {
    VEC3 ahead;
    MTX34 mtx;
    MTX34 frame;
    MTX34 scale;
    VEC3 va;
    VEC3 vb;

    if (!IsValidPointer((u32)ctx)) {
        nw4r::db::Panic(ef_stripe_vertex_file_str, 236, ef_stripe_vertex_ahead_str, ctx);
    }
    VEC3_ctor(&ahead);
    MTX34_ctor(&mtx);
    if (p == NULL) {
        p = (EfDrawParticle*)ef_pm_first_alive((EfDrawList*)ctx->particle_manager);
    }
    calc_ahead(&ahead, ctx, p);
    ef_stripe_draw_segment(&frame, self, ctx, flags, p, &ahead, pos);
    ef_stripe_scale_mtx(&scale, self, p, width);
    mtx34_concat(&mtx, &frame, &scale);
    VEC3_ctor(&va);
    VEC3_ctor(&vb);
    mtx34_mult_vec3(&va, &mtx, a);
    mtx34_mult_vec3(&vb, &mtx, b);
    fn_800B51F8((Vec*)&va, (Vec*)&vb, t, flags);
}


/* Emits two stripe endpoints, each preceded by its scale when the flag's low bit is set. */
void fn_800B51F8(Vec* a, Vec* b, f32 scale, u32 flags) {
    fn_800B52AC(a);
    if (fn_800B5298(flags)) {
        fn_800B5288(lbl_80796124, scale);
    }
    fn_800B52AC(b);
    if (fn_800B5298(flags)) {
        fn_800B5288(lbl_80796130, scale);
    }
}

/* Writes a pair of f32 to the pipe. */
void fn_800B5288(f32 x, f32 y) {
    GXWGFifo.f32 = x;
    GXWGFifo.f32 = y;
}

/* Tests the low bit of a status word (the booleanised `(value & 1) != 0`). */
int fn_800B5298(u32 value) {
    return (value & 1) != 0;
}

void fn_800B52AC(Vec* v) {
    fn_800B52BC(v->x, v->y, v->z);
}

/* Writes three f32 to the pipe. */
void fn_800B52BC(f32 x, f32 y, f32 z) {
    GXWGFifo.f32 = x;
    GXWGFifo.f32 = y;
    GXWGFifo.f32 = z;
}

/* 0x800B52D0 (0x1E4): the rotation (about the stripe axis, by the particle's Y rotation) and scale of one ribbon
 * segment, recentred on the strip width. */
void ef_stripe_scale_mtx(MTX34* out, nw4r::ef::DrawStripeStrategy* self, EfDrawParticle* p, f32 width) {
    VEC3 rotate;
    f32 c;
    f32 s;
    f32 size;

    if (!IsValidPointer((u32)p)) {
        nw4r::db::Panic(ef_stripe_scale_file_str, 206, ef_stripe_scale_pp_str, p);
    }
    VEC3_ctor(&rotate);
    ef_particle_get_rotate(p, &rotate);
    size = ef_particle_get_scale((EfParticle*)p);
    ef_sin_cos(&s, &c, rotate.y);
    s *= size;
    c *= size;
    mtx34_set(out, c, 0.0f, -s, width - c * width, 0.0f, 1.0f, 0.0f, 0.0f, s, 0.0f, c, -s * width);
}

/* 0x800B54B4 (0x1E4): the particle's rotation plus its per-axis offset steps and its manager's offset. */
void ef_particle_get_rotate(const void* src, Vec3* out) {
    const EfDrawParticle* p = (const EfDrawParticle*)src;

    if (!IsValidPointer((u32)out)) {
        nw4r::db::Panic(ef_particle_h_file_str, 228, ef_particle_rotate_assert_str, out);
    }
    copyVec3(out, &p->rotate);
    if (p->rotate_offset[0] != 0) {
        out->x += ef_stripe_rotate_step * p->rotate_offset[0];
    }
    if (p->rotate_offset[1] != 0) {
        out->y += ef_stripe_rotate_step * p->rotate_offset[1];
    }
    if (p->rotate_offset[2] != 0) {
        out->z += ef_stripe_rotate_step * p->rotate_offset[2];
    }
    ef_pm_modify_rotate(p->manager, p, out);
}

/* 0x800B5698 (0x150): adds the manager's rotation offset. */
void ef_pm_modify_rotate(EfDrawParticleManager* pm, const EfDrawParticle* p, VEC3* rotate) {
    if (!IsValidPointer((u32)rotate)) {
        nw4r::db::Panic(ef_particlemanager_h_file_str, 576, ef_pm_base_assert_str, rotate);
    }
    rotate->x += pm->rotate_offset.x;
    rotate->y += pm->rotate_offset.y;
    rotate->z += pm->rotate_offset.z;
}

/* Resolves the stripe length, returning 0 when it is below the unit's threshold. */
int ef_vec3_normalize(void* self) {
    f32 length = vec3_dot((const f32*)self, (const f32*)self);
    if (length < ef_float_epsilon()) {
        return 0;
    }
    fn_800513F0((VEC3*)self, nw4r::math::FrSqrt(length));
    return 1;
}

/* A stripe-strategy constant. */
f32 ef_float_epsilon(void) {
    return lbl_80796140;
}

/* Tests one bit of a status word. */
int fn_800B5A50(u32 value) {
    return (value & 0x8) != 0;
}

/* Walks the particle list at +0x3C until the callback reports 1 (or the end). */
void* ef_pm_first_alive(EfDrawList* self) {
    void* node = (void*)ef_pm_list_head((EfParticleState*)self);
    while (node != 0 && ef_get_life_status(node) != 1) {
        node = ef_pm_next_alive(self, node);
    }
    return node;
}

/* Walks the auxiliary list through the per-node offset table until the callback reports 1. */
void* ef_pm_next_alive(void* self, void* node) {
    void* next = ef_pm_list_next(self, node);
    while (next != 0 && ef_get_life_status(next) != 1) {
        next = ef_pm_list_next(self, next);
    }
    return next;
}

/* The next list node: the manager's per-instance byte offset into the node.  The offset is a runtime
 * field, so there is no compile-time field name to reach; the byte add is the only spelling. */
void* ef_pm_list_next(void* self, void* node) {
    return *(void**)((u8*)node + ((EfNodeManager*)self)->field_0x42);
}

/* The particle's stored word at +0x3C. */
u32 ef_pm_list_head(EfParticleState* self) {
    return self->field_0x3C;
}

/* The particle's packed flags, bits 6-7. */
s32 fn_800B5B48(void* ctx, EfParticleState* particle) {
    return particle->field_0xB2 & 0xC0;
}

/* 0x800B57E8 (0x1FC): the frame of one stripe particle: the side axis is the ahead vector crossed with the previous
 * particle's (or, in view-facing mode, the view's Z axis), the up axis completes the frame and becomes the particle's
 * new ahead vector, and the position is the translation. */
void ef_stripe_draw_segment(MTX34* out, nw4r::ef::DrawStripeStrategy* self,
                            nw4r::ef::DrawStripeStrategy::AheadContext* ctx, u32 flags, EfDrawParticle* p,
                            const VEC3* ahead, const VEC3* pos) {
    VEC3 side;
    VEC3 up;

    if (!IsValidPointer((u32)p)) {
        nw4r::db::Panic(ef_stripe_inline3_file_str, 174, ef_stripe_inline2_pp_str, p);
    }
    VEC3_ctor(&side);
    VEC3_ctor(&up);
    if (fn_800B5A50(flags) == 0) {
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
    mtx34_set(out, side.x, ahead->x, up.x, pos->x, side.y, ahead->y, up.y, pos->y, side.z, ahead->z, up.z,
                pos->z);
}

/* 0x800B5B54 (0x10): the number of particles the manager holds. */
s32 ef_stripe_draw_count(nw4r::ef::DrawStrategyImpl* self, EfDrawParticleManager* pm) {
    return pm->particles.count;
}

/* 0x800B5B64 (0x5C8): one ring of a tube: the particle's frame times its rotation-and-scale matrix, kept as a centre
 * and the X/Z axes, with texture coordinate `t`. */
void ef_stripe_tube_ring(nw4r::ef::DrawStripeStrategy* self, EfStripeParam* out,
                         nw4r::ef::DrawStripeStrategy::AheadContext* ctx, u32 flags, EfDrawParticle* p,
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
        nw4r::db::Panic(ef_stripe_file_str, 395, ef_stripe_dst_assert_str, out);
    }
    if (!IsValidPointer((u32)ctx)) {
        nw4r::db::Panic(ef_stripe_file_str, 396, ef_stripe_context_assert_str, ctx);
    }
    if (!IsValidPointer((u32)p)) {
        nw4r::db::Panic(ef_stripe_file_str, 397, ef_stripe_pp_assert_str, p);
    }
    if (!IsValidPointer((u32)calc_ahead)) {
        nw4r::db::Panic(ef_stripe_file_str, 398, ef_stripe_calc_ahead_assert_str, calc_ahead);
    }
    VEC3_ctor(&ahead);
    calc_ahead(&ahead, ctx, p);
    ef_stripe_draw_segment(&frame, self, ctx, flags, p, &ahead, (const VEC3*)&p->world_pos);
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

/* 0x800B6144 (0x3F0): one tube segment as a strip around the two rings, `divide` sides from the (sin, cos) table;
 * the ring order sets the facing. */
void ef_stripe_tube_strip(nw4r::ef::DrawStripeStrategy* self, const EfStripeParam* a, const EfStripeParam* b,
                          u32 flags, s32 divide, const f32* trig) {
    f32 inv;
    s32 i;

    if (!IsValidPointer((u32)trig)) {
        nw4r::db::Panic(ef_stripe_file_str, 441, ef_stripe_trig_assert_str, trig);
    }
    inv = math_reciprocal(divide);
    GXBegin(0x98, 0, divide * 2 + 2);
    if (fn_800B6534(flags)) {
        for (i = 0; i <= divide; trig += 2, i++) {
            f32 s = trig[0];
            f32 c = trig[1];
            VEC3 a_pos;
            VEC3 a_sum;
            VEC3 a_side;
            VEC3 a_up;
            VEC3 b_pos;
            VEC3 b_sum;
            VEC3 b_side;
            VEC3 b_up;

            vec3_scale(&a_up, (VEC3*)&a->up, s);
            vec3_scale(&a_side, (VEC3*)&a->side, c);
            addVec3(&a_sum, &a_side, &a_up);
            addVec3(&a_pos, &a_sum, (VEC3*)&a->center);
            fn_800B52AC((Vec*)&a_pos);
            if (fn_800B5298(flags)) {
                fn_800B5288(inv * i, a->tex_t);
            }
            vec3_scale(&b_up, (VEC3*)&b->up, s);
            vec3_scale(&b_side, (VEC3*)&b->side, c);
            addVec3(&b_sum, &b_side, &b_up);
            addVec3(&b_pos, &b_sum, (VEC3*)&b->center);
            fn_800B52AC((Vec*)&b_pos);
            if (fn_800B5298(flags)) {
                fn_800B5288(inv * i, b->tex_t);
            }
        }
    } else {
        for (i = 0; i <= divide; trig += 2, i++) {
            f32 s = trig[0];
            f32 c = trig[1];
            VEC3 b_pos;
            VEC3 b_sum;
            VEC3 b_side;
            VEC3 b_up;
            VEC3 a_pos;
            VEC3 a_sum;
            VEC3 a_side;
            VEC3 a_up;

            vec3_scale(&b_up, (VEC3*)&b->up, s);
            vec3_scale(&b_side, (VEC3*)&b->side, c);
            addVec3(&b_sum, &b_side, &b_up);
            addVec3(&b_pos, &b_sum, (VEC3*)&b->center);
            fn_800B52AC((Vec*)&b_pos);
            if (fn_800B5298(flags)) {
                fn_800B5288(inv * i, b->tex_t);
            }
            vec3_scale(&a_up, (VEC3*)&a->up, s);
            vec3_scale(&a_side, (VEC3*)&a->side, c);
            addVec3(&a_sum, &a_side, &a_up);
            addVec3(&a_pos, &a_sum, (VEC3*)&a->center);
            fn_800B52AC((Vec*)&a_pos);
            if (fn_800B5298(flags)) {
                fn_800B5288(inv * i, a->tex_t);
            }
        }
    }
    fn_800B4FE0();
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
        nw4r::db::Panic(ef_stripe_file_str, 493, ef_stripe_ahead_assert_str, ctx);
    }
    pm = ctx->particle_manager;
    ed = (EfEmitterDrawSetting*)ef_resource_draw_setting(pm->resource);
    calc_ahead = self->GetCalcAheadFunc(pm);
    order = fn_800B6994(self, (EfParticleState*)ed);
    first = self->GetGetFirstDrawParticleFunc(order);
    next = self->GetGetNextDrawParticleFunc(order);
    offset_x = ef_stripe_percent * ed->scale_a;
    offset_y = ef_stripe_percent * ed->scale_b;
    count = ef_stripe_draw_count(self, pm);
    if (fn_800B5B48(self, (EfParticleState*)ed) == 0x40) {
        step = lbl_80796124;
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
        EfStripeParam cur;
        EfStripeParam prev;

        fn_800B6954((EfStripeSample*)&prev);
        fn_800B6954((EfStripeSample*)&cur);
        p = first(pm);
        ef_stripe_tube_ring(self, &cur, ctx, flags, p, calc_ahead, offset_x, offset_y, step * index);
        index += dir;
        for (p = next(pm, p); p != NULL; p = next(pm, p), index += dir) {
            fn_800B6900(&prev, &cur);
            ef_stripe_tube_ring(self, &cur, ctx, flags, p, calc_ahead, offset_x, offset_y, step * index);
            ef_stripe_tube_strip(self, &prev, &cur, flags, ef_draw_setting_tube_divide(self, (EfParticleState*)ed),
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
        nw4r::db::Panic(ef_stripe_file_str, 549, ef_stripe_ahead_assert_str, ctx);
    }
    pm = ctx->particle_manager;
    ed = (EfEmitterDrawSetting*)ef_resource_draw_setting(pm->resource);
    calc_ahead = self->GetCalcAheadFunc(pm);
    order = fn_800B6994(self, (EfParticleState*)ed);
    first = self->GetGetFirstDrawParticleFunc(order);
    next = self->GetGetNextDrawParticleFunc(order);
    offset_x = ef_stripe_percent * ed->scale_a;
    offset_y = ef_stripe_percent * ed->scale_b;
    count = ef_stripe_draw_count(self, pm);
    if (fn_800B5B48(self, (EfParticleState*)ed) == 0x40) {
        step = lbl_80796124;
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
        EfStripeParam cur;
        EfStripeParam prev;
        EfStripeParam head;

        fn_800B6954((EfStripeSample*)&head);
        fn_800B6954((EfStripeSample*)&prev);
        fn_800B6954((EfStripeSample*)&cur);
        p = first(pm);
        ef_stripe_tube_ring(self, &head, ctx, flags, p, calc_ahead, offset_x, offset_y, step * index);
        fn_800B6900(&cur, &head);
        index += dir;
        for (p = next(pm, p); p != NULL; p = next(pm, p), index += dir) {
            fn_800B6900(&prev, &cur);
            ef_stripe_tube_ring(self, &cur, ctx, flags, p, calc_ahead, offset_x, offset_y, step * index);
            ef_stripe_tube_strip(self, &prev, &cur, flags, ef_draw_setting_tube_divide(self, (EfParticleState*)ed),
                                 ctx->trig_table);
        }
        head.tex_t = step * index;
        ef_stripe_tube_strip(self, &cur, &head, flags, ef_draw_setting_tube_divide(self, (EfParticleState*)ed),
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
        nw4r::db::Panic(ef_stripe_file_str, 609, ef_stripe_ahead_assert_str, ctx);
    }
    pm = ctx->particle_manager;
    ed = (EfEmitterDrawSetting*)ef_resource_draw_setting(pm->resource);
    calc_ahead = self->GetCalcAheadFunc(pm);
    order = fn_800B6994(self, (EfParticleState*)ed);
    first = self->GetGetFirstDrawParticleFunc(order);
    next = self->GetGetNextDrawParticleFunc(order);
    offset_x = ef_stripe_percent * ed->scale_a;
    offset_y = ef_stripe_percent * ed->scale_b;
    count = ef_stripe_draw_count(self, pm);
    if (fn_800B5B48(self, (EfParticleState*)ed) == 0x40) {
        step = lbl_80796124;
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
        EfStripeParam cur;
        EfStripeParam prev;
        EfDrawParticle* youngest;
        VEC3 offset;

        fn_800B6954((EfStripeSample*)&prev);
        fn_800B6954((EfStripeSample*)&cur);
        p = first(pm);
        ef_stripe_tube_ring(self, &prev, ctx, flags, p, calc_ahead, offset_x, offset_y, step * index);
        index += dir;
        fn_800B6900(&cur, &prev);
        cur.tex_t = step * index;
        youngest = (EfDrawParticle*)ef_pm_first_alive((EfDrawList*)pm);
        if (!youngest) {
            nw4r::db::Panic(ef_stripe_file_str, 655, ef_stripe_youngest_assert_str);
        }
        subVec3(&offset, &ctx->emitter_center, &youngest->world_pos);
        addVec3To(&prev.center, &offset);
        index += dir;
        ef_stripe_tube_strip(self, &prev, &cur, flags, ef_draw_setting_tube_divide(self, (EfParticleState*)ed),
                             ctx->trig_table);
        for (p = next(pm, p); p != NULL; p = next(pm, p), index += dir) {
            fn_800B6900(&prev, &cur);
            ef_stripe_tube_ring(self, &cur, ctx, flags, p, calc_ahead, offset_x, offset_y, step * index);
            ef_stripe_tube_strip(self, &prev, &cur, flags, ef_draw_setting_tube_divide(self, (EfParticleState*)ed),
                                 ctx->trig_table);
        }
    }
}

/* The Z unit vector. */
const VEC3* ef_get_unit_z_vec(void) {
    return &ef_unit_z_vec;
}

/* The X unit vector. */
const VEC3* ef_get_unit_x_vec(void) {
    return &ef_unit_x_vec;
}

/* Tests one bit of a status word. */
int fn_800B6534(u32 value) {
    return (value & 0x10) != 0;
}

/* The particle's layer/parameter index byte. */
s32 ef_draw_setting_tube_divide(void* ctx, EfParticleState* particle) {
    return particle->field_0xB0;
}

/* Copies a stripe sample. */
void fn_800B6900(EfStripeParam* dst, EfStripeParam* src) {
    *dst = *src;
}

/* Zeroes the three positions of a stripe sample and returns it. */
EfStripeSample* fn_800B6954(EfStripeSample* self) {
    VEC3_ctor((VEC3*)&self->a);
    VEC3_ctor((VEC3*)&self->b);
    VEC3_ctor((VEC3*)&self->c);
    return self;
}

/* The particle's state bit 0x800. */
u32 fn_800B6994(void* ctx, EfParticleState* particle) {
    return particle->flags_0x00 & 0x800;
}

/* Draws eight stripe steps at one scale when the flag is set. */
void ef_stripe_begin_side(u32 cull_mode, u32 enabled, u32 flags) {
    GXSetCullMode(cull_mode);
    if (enabled != 0) {
        GXBegin(0x98, 0, 8);
        for (int i = 0; i < 8; i++) {
            fn_800B52BC(lbl_80796130, lbl_80796130, lbl_80796130);
            if (fn_800B5298(flags)) {
                fn_800B5288(lbl_80796130, lbl_80796130);
            }
        }
        fn_800B4FE0();
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
        nw4r::db::Panic(ef_stripe_file_str, 738, ef_stripe_pm_assert_str, pm);
    }
    if (!IsValidPointer((u32)pm->resource)) {
        nw4r::db::Panic(ef_stripe_file_str, 740, ef_stripe_resource_assert_str, pm->resource);
    }
    ed = (EfEmitterDrawSetting*)ef_resource_draw_setting(pm->resource);
    if (!IsValidPointer((u32)ed)) {
        nw4r::db::Panic(ef_stripe_file_str, 742, ef_stripe_ed_assert_str, ed);
    }
    count = ef_stripe_draw_count(this, pm);
    if (count != 0) {
        MTX34 view_mtx;
        EfDrawParticle* p;
        u32 flags;
        MTX34 mtx;
        VEC3 ahead;

        MTX34_ctor(&view_mtx);
        ef_draw_info_view_mtx(&info, &view_mtx);
        AheadContext ctx(&view_mtx, pm);
        ef_stripe_first_ahead(&ahead, this, ed, &ctx);
        for (p = (EfDrawParticle*)ef_pm_list_head((EfParticleState*)pm); p != NULL;
             p = (EfDrawParticle*)ef_pm_list_next(pm, p)) {
            if (p->ahead.x <= 1.0f) {
                break;
            }
            copyVec3(&p->ahead, &ahead);
        }

        flags = (fn_800B6994(this, (EfParticleState*)ed) ? 0x10 : 0) | (ed->type_option == 2 ? 8 : 0);
        MTX34_ctor(&mtx);
        ef_draw_info_view_mtx(&info, &mtx);
        mtx34_concat(&mtx, &mtx, &ctx.manager_mtx);
        if (fn_800B5A50(flags)) {
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

        if (((ed->stripe_connect & 7) == 1 && count < 3) || ((ed->stripe_connect & 7) == 0 && count < 2)) {
            u32 order = ed->flags & 0x800;
            CalcAheadFunc calc_ahead = GetCalcAheadFunc(pm);
            GetFirstDrawParticleFunc first = GetGetFirstDrawParticleFunc(order);
            GetNextDrawParticleFunc next = GetGetNextDrawParticleFunc(order);

            for (p = first(pm); p != NULL; p = next(pm, p)) {
                VEC3 axis;
                MTX34 frame;

                VEC3_ctor(&axis);
                calc_ahead(&axis, &ctx, p);
                ef_stripe_draw_segment(&frame, this, &ctx, flags, p, &axis, (VEC3*)&p->world_pos);
            }
            return;
        }

        ef_stripe_setup_gx(this, &info, pm);
        GXLoadPosMtxImm((const f32(*)[4])mtx34_get_ptr(&mtx), 0);
        flags |= (mNumTexmap != 0);
        p = GetGetFirstDrawParticleFunc(ed->flags & 0x800)(pm);
        SetupGP(p, *ed, info, true, false);
        if (ed->type_option != 3) {
            VEC3 a;
            VEC3 b;
            const VEC3* pb = setVec3(&b, -1.0f, 0.0f, 0.0f);
            ef_stripe_draw_ribbon(this, &ctx, flags, setVec3(&a, 1.0f, 0.0f, 0.0f), pb);
            if (ed->type_option == 1) {
                VEC3 c;
                VEC3 d;
                const VEC3* pd = setVec3(&d, 0.0f, 0.0f, 1.0f);
                ef_stripe_draw_ribbon(this, &ctx, flags, setVec3(&c, 0.0f, 0.0f, -1.0f), pd);
            }
        } else {
            u32 flush = 0;
            if ((ed->flags & 8) != 0 || p->manager->emitter->effect->system->flush_gp == 0) {
                flush = 1;
            }
            ef_stripe_draw_tube(this, &ctx, flags, flush);
        }
    }
}

/* 0x800B83CC (0x3BC): the ahead-vector builder the draw setting's direction type (and, for type 6, the stripe's
 * connection mode) selects. */
DrawStrategyImpl::CalcAheadFunc DrawStripeStrategy::GetCalcAheadFunc(EfDrawParticleManager* pm) {
    EfEmitterDrawSetting* ed;

    if (!IsValidPointer((u32)pm)) {
        nw4r::db::Panic(ef_stripe_file_str, 949, ef_stripe_pm_assert_str, pm);
    }
    if (!IsValidPointer((u32)pm->resource)) {
        nw4r::db::Panic(ef_stripe_file_str, 950, ef_stripe_resource_assert_str, pm->resource);
    }
    ed = (EfEmitterDrawSetting*)ef_resource_draw_setting(pm->resource);
    if (!IsValidPointer((u32)ed)) {
        nw4r::db::Panic(ef_stripe_file_str, 953, ef_stripe_ed_assert_str, ed);
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
    VEC3 axis;

    VEC3_ctor(&emitter_axis_x);
    VEC3_ctor(&view_axis_z);
    trig_table = NULL;
    if (!IsValidPointer((u32)pm)) {
        nw4r::db::Panic(ef_stripe_file_str, 989, ef_stripe_pm_assert_str, pm);
    }
    setVec3(&axis, emitter_mtx.m[0][0], emitter_mtx.m[1][0], emitter_mtx.m[2][0]);
    mtx34_rotate_vec3(&emitter_axis_x, &manager_mtx_inv, &axis);
}

}  // namespace ef
}  // namespace nw4r

extern "C" {

/* 0x800B7FCC (0x1B8): sets the stripe's GX state: the strategy's texture/TEV/channel setup, then a direct position
 * and texcoord vertex format at the current matrix. */
void ef_stripe_setup_gx(nw4r::ef::DrawStripeStrategy* self, const EfDrawInfo* info, EfDrawParticleManager* pm) {
    if (!IsValidPointer((u32)pm)) {
        nw4r::db::Panic(ef_stripe_file_str, 849, ef_stripe_pm_assert_str, pm);
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

/* 0x800B7DB0 (0x1A8): copies the draw's view matrix into `result` and, for a nonzero depth offset, pushes it along
 * the view direction of the depth origin (perspective) or along view Z (orthographic). */
MTX34* ef_draw_info_view_mtx(const EfDrawInfo* info, MTX34* result) {
    if (result == NULL) {
        nw4r::db::Panic(ef_drawinfo_file_str, 123, ef_drawinfo_null_result_str);
    }
    mtx34_copy(result, (MTX34*)&info->view_mtx);
    if (0.0f != info->depth_offset) {
        f32 proj[7];

        GXGetProjectionv(proj);
        switch ((s32)proj[0]) {
        case 0: {
            VEC3 dir;

            VEC3_ctor(&dir);
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

/* 0x800B71F0 (0x438): draws the stripe as a tube: builds the (sin, cos) table of its sides on the stack, then draws
 * the back faces and the front faces through the connection type's builder. */
void ef_stripe_draw_tube(nw4r::ef::DrawStripeStrategy* self, nw4r::ef::DrawStripeStrategy::AheadContext* ctx,
                         u32 flags, u32 flush) {
    EfEmitterDrawSetting* ed;
    s32 divide;

    if (!IsValidPointer((u32)ctx)) {
        nw4r::db::Panic(ef_stripe_file_str, 679, ef_stripe_ahead_assert_str, ctx);
    }
    ed = (EfEmitterDrawSetting*)ef_resource_draw_setting(ctx->particle_manager->resource);
    if (!(3 <= ef_draw_setting_tube_divide(self, (EfParticleState*)ed))) {
        nw4r::db::Panic(ef_stripe_file_str, 684, ef_stripe_tube_divide_assert_str);
    }
    divide = ef_draw_setting_tube_divide(self, (EfParticleState*)ed);
    if (divide >= 3) {
        f32* trig = (f32*)__alloca((divide + 1) * 8);

        if (!IsValidPointer((u32)trig)) {
            nw4r::db::Panic(ef_stripe_file_str, 694, ef_stripe_trig_assert_str, trig);
        }
        if (trig != NULL) {
            f32 step = ef_stripe_full_circle / divide;
            f32* pair;
            s32 i;

            trig[1] = lbl_80796124;
            trig[0] = lbl_80796130;
            for (i = 1, pair = trig + 2; i < divide; pair += 2, i++) {
                sin_cos_deg(&pair[0], &pair[1], step * i);
            }
            trig[divide * 2 + 1] = lbl_80796124;
            trig[divide * 2] = lbl_80796130;
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

/* The particle's packed flags, bits 0-2. */
s32 ef_draw_setting_connect_type(void* ctx, EfParticleState* particle) {
    return particle->field_0xB2 & 0x7;
}

/* Transforms a vector by the matrix the helper pair builds, and returns `dst`. */
VEC3* ef_vec3_transform(VEC3* dst, const MTX34* m, const VEC3* v) {
    void* x = fn_800508A8(dst);
    void* y = fn_800508AC((void*)v);
    PSMTXMultVec((const f32*)mtx34_const_ptr((u32)m), y, x);
    return dst;
}

/* 0x800B8184 (0x23C): the stripe's first ahead vector: for direction type 7 one manager axis (or their sum) by the
 * stripe's axis bits, otherwise the emitter's X or Z axis (or a small diagonal) taken into manager space;
 * normalised, falling back to the emitter's Y axis. */
void ef_stripe_first_ahead(VEC3* out, nw4r::ef::DrawStripeStrategy* self, EfEmitterDrawSetting* ed,
                           nw4r::ef::DrawStripeStrategy::AheadContext* ctx) {
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
        switch (fn_800B83C0(self, (EfParticleState*)ed)) {
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
        switch (fn_800B83C0(self, (EfParticleState*)ed)) {
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
            setVec3(&diagonal, lbl_80796124, lbl_80796124, lbl_80796124);
            mtx34_rotate_vec3(out, &ctx->emitter_mtx, &diagonal);
            mtx34_rotate_vec3(out, &ctx->manager_mtx_inv, out);
            break;
        }
    }
    if (ef_vec3_normalize(out) == 0) {
        copyVec3(out, &ctx->emitter_axis_y);
    }
}

/* The particle's packed flags, bits 3-5. */
s32 fn_800B83C0(void* ctx, EfParticleState* particle) {
    return particle->field_0xB2 & 0x38;
}

/* Builds the particle's +0xB0 transform into a local and copies it into `dst`. */
void ef_ahead_manager_axis_y(nw4r::math::VEC3* dst, EfParticleState* particle) {
    u8 tmp[0x10];
    copyVec3(dst, (const nw4r::math::VEC3*)vec3_copy_construct(tmp, &particle->field_0xB0));
}

/* Copies the particle's +0x98 block into `dst`. */
void ef_ahead_emitter_axis_y(nw4r::math::VEC3* dst, EfParticleState* particle) {
    copyVec3(dst, (const nw4r::math::VEC3*)&particle->field_0x98);
}

/* Subtracts the two ahead vectors and copies the reference block when the result is degenerate. */
void ef_ahead_from_emitter(Vec* a, EfParticleState* particle, EfAheadItem* item) {
    PSVECSubtract((f32*)a, (const f32*)&item->field_0xAC, (const f32*)&particle->field_0xA4);
    if (ef_vec3_normalize(a) == 0) {
        copyVec3((nw4r::math::VEC3*)a, (const nw4r::math::VEC3*)&particle->field_0x98);
    }
}

/* Builds the ahead vector and copies the reference block when the result is degenerate. */
/* untyped: opaque handle passed through to ef_particle_get_move_dir */
void ef_ahead_move_dir(Vec* a, EfParticleState* particle, void* arg) {
    ef_particle_get_move_dir(arg, a);
    if (ef_vec3_normalize(a) == 0) {
        copyVec3((nw4r::math::VEC3*)a, (const nw4r::math::VEC3*)&particle->field_0x98);
    }
}

/* 0x800B89EC (0x35C): the ahead vector toward the next elder particle (or, for the eldest, from the emitter origin),
 * normalised, falling back to the emitter's Y axis. */
void ef_stripe_ahead_type3(VEC3* out, nw4r::ef::DrawStrategyImpl::AheadContext* ctx, EfDrawParticle* p) {
    EfDrawParticle* next;

    if (!IsValidPointer((u32)out)) {
        nw4r::db::Panic(ef_stripe_file_str, 1002, ef_stripe_yaxis_assert_str, out);
    }
    if (!IsValidPointer((u32)ctx)) {
        nw4r::db::Panic(ef_stripe_file_str, 1003, ef_stripe_context_assert_str, ctx);
    }
    if (!IsValidPointer((u32)p)) {
        nw4r::db::Panic(ef_stripe_file_str, 1004, ef_stripe_particle_assert_str, p);
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

/* 0x800B8DC0 (0x400): the ahead vector across the particle's two neighbours (each direction normalised), falling
 * back to the emitter's Y axis. */
void ef_stripe_ahead_type6(VEC3* out, nw4r::ef::DrawStrategyImpl::AheadContext* ctx, EfDrawParticle* p) {
    EfDrawParticle* younger;
    EfDrawParticle* elder;
    VEC3 to_younger;
    VEC3 to_elder;
    VEC3 zero_a;
    VEC3 zero_b;

    if (!IsValidPointer((u32)out)) {
        nw4r::db::Panic(ef_stripe_file_str, 1030, ef_stripe_yaxis_assert_str, out);
    }
    if (!IsValidPointer((u32)ctx)) {
        nw4r::db::Panic(ef_stripe_file_str, 1031, ef_stripe_context_assert_str, ctx);
    }
    if (!IsValidPointer((u32)p)) {
        nw4r::db::Panic(ef_stripe_file_str, 1032, ef_stripe_particle_assert_str, p);
    }
    younger = (EfDrawParticle*)ef_pm_next_alive(ctx->particle_manager, p);
    elder = (EfDrawParticle*)ef_pm_prev_alive(ctx->particle_manager, p);
    setVec3(&to_younger, lbl_80796130, lbl_80796130, lbl_80796130);
    if (younger != NULL) {
        PSVECSubtract((f32*)&to_younger, (const f32*)&younger->world_pos, (const f32*)&p->world_pos);
        if (ef_vec3_normalize(&to_younger) == 0) {
            copyVec3(&to_younger, setVec3(&zero_a, lbl_80796130, lbl_80796130, lbl_80796130));
        }
    }
    setVec3(&to_elder, lbl_80796130, lbl_80796130, lbl_80796130);
    if (elder != NULL) {
        PSVECSubtract((f32*)&to_elder, (const f32*)&elder->world_pos, (const f32*)&p->world_pos);
        if (ef_vec3_normalize(&to_elder) == 0) {
            copyVec3(&to_elder, setVec3(&zero_b, lbl_80796130, lbl_80796130, lbl_80796130));
        }
    }
    PSVECSubtract((f32*)out, (const f32*)&to_younger, (const f32*)&to_elder);
    if (ef_vec3_normalize(out) == 0) {
        copyVec3(out, &ctx->emitter_axis_y);
    }
}

/* 0x800B91C0 (0x400): as the two-neighbour ahead vector, with the list wrapping around for a looped stripe. */
void ef_stripe_ahead_type6_link1(VEC3* out, nw4r::ef::DrawStrategyImpl::AheadContext* ctx, EfDrawParticle* p) {
    EfDrawParticle* younger;
    EfDrawParticle* elder;
    VEC3 to_younger;
    VEC3 to_elder;
    VEC3 zero_a;
    VEC3 zero_b;

    if (!IsValidPointer((u32)out)) {
        nw4r::db::Panic(ef_stripe_file_str, 1069, ef_stripe_yaxis_assert_str, out);
    }
    if (!IsValidPointer((u32)ctx)) {
        nw4r::db::Panic(ef_stripe_file_str, 1070, ef_stripe_context_assert_str, ctx);
    }
    if (!IsValidPointer((u32)p)) {
        nw4r::db::Panic(ef_stripe_file_str, 1071, ef_stripe_particle_assert_str, p);
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
        copyVec3(&to_younger, setVec3(&zero_a, lbl_80796130, lbl_80796130, lbl_80796130));
    }
    VEC3_ctor(&to_elder);
    PSVECSubtract((f32*)&to_elder, (const f32*)&elder->world_pos, (const f32*)&p->world_pos);
    if (ef_vec3_normalize(&to_elder) == 0) {
        copyVec3(&to_elder, setVec3(&zero_b, lbl_80796130, lbl_80796130, lbl_80796130));
    }
    PSVECSubtract((f32*)out, (const f32*)&to_younger, (const f32*)&to_elder);
    if (ef_vec3_normalize(out) == 0) {
        copyVec3(out, &ctx->emitter_axis_y);
    }
}

/* 0x800B9630 (0x3B8): as the two-neighbour ahead vector, with the eldest particle reaching to the emitter origin. */
void ef_stripe_ahead_type6_link2(VEC3* out, nw4r::ef::DrawStrategyImpl::AheadContext* ctx, EfDrawParticle* p) {
    EfDrawParticle* younger;
    EfDrawParticle* elder;
    VEC3 to_younger;
    VEC3 to_elder;

    if (!IsValidPointer((u32)out)) {
        nw4r::db::Panic(ef_stripe_file_str, 1110, ef_stripe_yaxis_assert_str, out);
    }
    if (!IsValidPointer((u32)ctx)) {
        nw4r::db::Panic(ef_stripe_file_str, 1111, ef_stripe_context_assert_str, ctx);
    }
    if (!IsValidPointer((u32)p)) {
        nw4r::db::Panic(ef_stripe_file_str, 1112, ef_stripe_particle_assert_str, p);
    }
    younger = (EfDrawParticle*)ef_pm_next_alive(ctx->particle_manager, p);
    elder = (EfDrawParticle*)ef_pm_prev_alive(ctx->particle_manager, p);
    setVec3(&to_younger, lbl_80796130, lbl_80796130, lbl_80796130);
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

/* Walks a node chain through ef_pm_list_prev until the callback reports 1 (or the end). */
void* ef_pm_prev_alive(void* self, void* node) {
    void* next = ef_pm_list_prev(self, node);
    while (next != 0 && ef_get_life_status(next) != 1) {
        next = ef_pm_list_prev(self, next);
    }
    return next;
}

void* ef_pm_list_prev(void* self, void* node) {
    return *(void**)((u8*)node + ((EfNodeManager*)self)->field_0x42 + 4);
}

/* Walks the +0x38 head through ef_pm_list_prev until the callback reports 1 (or the end). */
void* ef_pm_last_alive(EfParticleState* self) {
    void* next = (void*)ef_pm_list_tail(self);
    while (next != 0 && ef_get_life_status(next) != 1) {
        next = ef_pm_list_prev(self, next);
    }
    return next;
}

/* The particle's stored word at +0x38. */
u32 ef_pm_list_tail(EfParticleState* self) {
    return self->field_0x38;
}

#ifdef __cplusplus
}
#endif

namespace nw4r {
namespace ef {

}  // namespace ef
}  // namespace nw4r

/* The destructors are the classes' inline ones, emitted at the end of this TU; retail calls each base destructor
 * out of line from them (`bl` + `extsh` of the delete flag), which a deferred inline body only gets with inlining
 * off at its point of emission. */
#pragma dont_inline on
