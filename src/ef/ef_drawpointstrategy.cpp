/*
 * ef/ef_drawpointstrategy.cpp - nw4r::ef DrawPointStrategy: `Draw` (the point particle walker), the single point
 *   emitter (`GXBegin` POINTS, position, optional texcoord), the out-of-line GX FIFO writers, the point GX state, the
 *   class's destructor and the DrawSmoothStripeStrategy constructor.
 * RANGE. .text 0x800BF818-0x800BFFD4 (9 functions); extab 0x8000A36C-0x8000A394, extabindex 0x800238C8-0x80023904,
 *   .data 0x80594408-0x805944E0 (the `__FILE__` string "ef_drawpointstrategy.cpp" and the three pointer-error
 *   messages first), .sdata 0x80791350-0x80791358, .sdata2 0x807961B8-0x807961C0.
 *   Seam (playbook 80, measured): every strategy TU runs [constructor, ..., inline destructor], and each constructor
 *   installs the vtable at the end of its own TU's `.data`; so this class's constructor is the tail of `ef/ef_drawlinestrategy.cpp`'s range
 *   (0x800BF7DC) and the DrawSmoothStripeStrategy constructor at this range's tail (0x800BFF98) is the next TU's.
 * FLAGS. `cflags_main`; `#pragma peephole off` before the includes (the FIFO writers keep the unfused narrowing stores,
 *   playbook 39), `#pragma dont_inline on` at the end (the inline destructor calls its base out of line).
 * NAMES. The map has only `fn_` stems for the helpers; `fn_800BFD84` is a GUESS for NintendoWare's setup (its assert
 *   on line 150 names `pm`).
 * RESIDUALS. The source defines the FIFO writers and the emitter before `Draw`, so our `.text` (and the extab and
 *   extabindex records) run in a different order from retail's address order.
 *   2 partial rows:
 *  - `Draw` (0x800BF818): one `lwz r6, 0x24(pm)` (the asserted resource) is scheduled before the six assert
 *    temporaries where retail loads it after them, and one `lfs` is placed differently;
 *  - `fn_800BFD60` (ours 0x20 of 0x24): retail has a `b` to the next instruction between the three `lfs` and the
 *    `lis` of the FIFO base (the `ef/ef_drawlinestrategy.cpp` row `fn_800BF58C` is the same):
 *    the row is two functions, a tail call into a 0x14 static FIFO writer at +0x10 that the map folds into it
 *    (reproduced byte for byte with a `dont_inline` static helper; the map split is request nw4r-l3#21).
 *   flipcheck: `.data` claimed, not emitted; `.sdata` (the size clamp) is emitted with its bytes equal (0x4 short of
 *   the claim, the trailing pad); `.sdata2` (the literals' pool) holds 0.0f before 6.0f where retail has 6.0f first.
 *   `DrawSmoothStripeStrategy` (constructor): the empty body is the whole source - the compiler
 *     emits the base call and the vtable store.
 * SHAPES. The `Panic` line numbers are literals (92/96/98 in `Draw`, 150 in the state setup), so the source's
 *   line count does not move them.
 */

#pragma peephole off

#include "ef.h"
#include "gx.h"
#include "ef/ef_drawpointstrategy.h"
#include "ef/ef_drawstripestrategy.h" /* ef_draw_info_view_mtx (rule 2) */
#include "ef/ef_drawsmoothstripestrategy.h" /* DrawSmoothStripeStrategy, whose constructor closes this range */
#include "fn_8004CAD8/mtx.h" /* the matrix helpers (rule 2) */

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
 * externs owned by neighbouring translation units and the SDK.
 * -------------------------------------------------------------------------------------------------- */

EfEmitterShape* ef_resource_draw_setting(void* emitter);
f32 ef_particle_get_scale(void* particle);
f32 ef_float_epsilon(void);
f32* ef_min_float(f32* limit, f32* value);
void ef_pm_get_mtx(EfDrawArgs* args, Mtx34* mtx);
void mtx34_concat(Mtx34* out, Mtx34* a, Mtx34* b);
Mtx34* mtx34_get_ptr(Mtx34* mtx);

/* SDK GX entry points, declared locally. */
extern void GXBegin(u8 prim, u8 vtxfmt, u16 nverts);
extern void GXSetPointSize(u8 pointSize, u32 texOffset);
extern void GXEnableTexOffsets(u32 coord, u32 line_enable, u32 point_enable);
extern void GXClearVtxDesc(void);
extern void GXSetVtxDesc(u32 attr, u32 type);
extern void GXSetVtxAttrFmt(u32 fmt, u32 attr, u32 cnt, u32 type, u32 frac);
extern void GXLoadPosMtxImm(void* mtx, u32 id);
extern void GXSetCurrentMtx(u32 id);

/* This unit's `__FILE__`/assert strings, the draw-order table and the constants, declared, never
 * defined. */
extern char lbl_80594408[]; /* "ef_drawpointstrategy.cpp"                                .data 0x80594408 */
extern char lbl_80594424[]; /* "NW4R:Pointer Error\npm(=%p) is not valid pointer."      .data 0x80594424 */
extern char lbl_80594458[]; /* "NW4R:Pointer Error\npm->mResource(=%p) is not valid..." .data 0x80594458 */
extern char lbl_80594494[]; /* "NW4R:Pointer Error\n&ed(=%p) is not valid pointer."     .data 0x80594494 */
/* The largest point the size clamp allows (`.sdata`). */
static f32 ef_point_max_size = 42.5f;

/* --------------------------------------------------------------------------------------------------
 * The out-of-line GX FIFO writers, in address order.
 * -------------------------------------------------------------------------------------------------- */

/* Ends the current FIFO command (the SDK's no-op `GXEnd`). */
void fn_800BFD38(void) {}

/* Writes one f32 pair to the pipe (the point's texture coordinate). */
void fn_800BFD3C(f32 x, f32 y) {
    GXWGFifo.f32 = x;
    GXWGFifo.f32 = y;
}

/* Tests the low bit of a status word. */
int fn_800BFD4C(u32 value) {
    return (value & 1) != 0;
}

/* Writes a vector to the pipe. */
void fn_800BFD60(Vec* v) {
    f32 x = v->x;
    f32 y = v->y;
    f32 z = v->z;
    GXWGFifo.f32 = x;
    GXWGFifo.f32 = y;
    GXWGFifo.f32 = z;
}

/* --------------------------------------------------------------------------------------------------
 * The single point emitter and the point GX state.
 * -------------------------------------------------------------------------------------------------- */

/* Emits one point at `pos`: GXBegin(POINTS, 0, 1), the position, and the (0, 0) texcoord when `flag`'s
 * low bit asks for one. */
void fn_800BFCCC(Vec* pos, u32 flag) {
    GXBegin(0xB8, 0, 1);
    fn_800BFD60(pos);
    if (fn_800BFD4C(flag)) {
        fn_800BFD3C(0.0f, 0.0f);
    }
    fn_800BFD38();
}

/* Sets the point GX state (texcoord offsets, vertex format, current matrix). */
void fn_800BFD84(nw4r::ef::DrawPointStrategy* self, const EfDrawInfo* info, EfDrawParticleManager* args) {
    if (!IsValidPointer((u32)args)) {
        nw4r::db::Panic(lbl_80594408, 150, lbl_80594424, args);
    }
    self->InitGraphics(args, *(EfEmitterDrawSetting*)ef_resource_draw_setting(args->resource), *info);
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

#ifdef __cplusplus
}
#endif

namespace nw4r {
namespace ef {

/* 0x800BF818 (0x4B4): draws each live particle of the manager as a GX point, resizing the point only when the size
 * changes. */
void DrawPointStrategy::Draw(const EfDrawInfo& info, EfDrawParticleManager* pm) {
    EfEmitterDrawSetting* ed;
    u32 flag;
    GetFirstDrawParticleFunc first_fn;
    GetNextDrawParticleFunc next_fn;
    u32 last_size;
    bool first;
    EfDrawParticle* particle;
    Mtx34 mtx_view;
    Mtx34 mtx_result;
    f32 progress;
    f32 scale;
    u32 size;

    if (!IsValidPointer((u32)pm)) {
        nw4r::db::Panic(lbl_80594408, 92, lbl_80594424, pm);
    }
    fn_800BFD84(this, &info, pm);
    if (!IsValidPointer((u32)pm->resource)) {
        nw4r::db::Panic(lbl_80594408, 96, lbl_80594458, pm->resource);
    }
    ed = (EfEmitterDrawSetting*)ef_resource_draw_setting(pm->resource);
    if (!IsValidPointer((u32)ed)) {
        nw4r::db::Panic(lbl_80594408, 98, lbl_80594494, ed);
    }

    flag = (mNumTexmap != 0);

    MTX34_ctor(&mtx_view);
    ef_pm_get_mtx(pm, &mtx_view);
    MTX34_ctor(&mtx_result);
    ef_draw_info_view_mtx(&info, &mtx_result);
    mtx34_concat(&mtx_result, &mtx_result, &mtx_view);
    GXLoadPosMtxImm(mtx34_get_ptr(&mtx_result), 0);

    first_fn = GetGetFirstDrawParticleFunc(ed->flags & 0x800);
    next_fn = GetGetNextDrawParticleFunc(ed->flags & 0x800);

    last_size = 0;
    first = true;
    scale = 6.0f;
    for (particle = first_fn(pm); particle != 0; particle = next_fn(pm, particle)) {
        progress = ef_particle_get_scale(particle);
        if (progress < ef_float_epsilon()) {
            continue;
        }
        size = (u32)(s32)(scale * *ef_min_float(&ef_point_max_size, &progress));
        if ((u8)size != 0) {
            if ((u8)last_size != (u8)size) {
                last_size = size;
                GXSetPointSize((u8)size, 5);
                SetupGP(particle, *ed, info, first, true);
            } else {
                SetupGP(particle, *ed, info, first, false);
            }
            first = false;
            fn_800BFCCC(&particle->world_pos, flag);
        }
    }
}

/* 0x800BFF98 (0x3C): the DrawSmoothStripeStrategy constructor (the next unit's class; see the unit header). */
DrawSmoothStripeStrategy::DrawSmoothStripeStrategy() {}

}  // namespace ef
}  // namespace nw4r

/* The destructors are the classes' inline ones, emitted at the end of this TU; retail calls each base destructor
 * out of line from them (`bl` + `extsh` of the delete flag), which a deferred inline body only gets with inlining
 * off at its point of emission. */
#pragma dont_inline on
