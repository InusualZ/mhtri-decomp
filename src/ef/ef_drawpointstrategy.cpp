/*
 * ef/ef_drawpointstrategy.cpp - nw4r::ef DrawPointStrategy: `Draw` (the point particle walker), the single point
 *   emitter (`GXBegin` POINTS, position, optional texcoord), the out-of-line GX FIFO writers, the point GX state,
 *   a deleting destructor and the constructor that installs the DrawSmoothStripeStrategy table `lbl_80594778`.
 * RANGE. .text 0x800BF818-0x800BFFD4 (9 functions); extab 0x8000A36C-0x8000A394, extabindex 0x800238C8-0x80023904,
 *   .data 0x80594408-0x805944E0 (the `__FILE__` string "ef_drawpointstrategy.cpp" and the three pointer-error
 *   messages first), .sdata 0x80791350-0x80791358, .sdata2 0x807961B8-0x807961C0.
 *   Unproven seam (playbook 80: a TU's tables sit late in its `.data`): this unit's table `lbl_805944C8` is installed
 *   by `fn_800BF7DC` at the tail of `ef/ef_drawlinestrategy.cpp`'s range, and this range ends with
 *   `fn_800BFF3C`/`fn_800BFF98`, the constructor installing `ef/ef_drawsmoothstripestrategy.cpp`'s table, so each
 *   `.text` seam may be off by that constructor.
 * FLAGS. `cflags_main`; `#pragma peephole off` (the FIFO writers keep the unfused narrowing stores, playbook 39).
 * NAMES. The map has only `fn_` stems for the range and the dump only `zz_` names; `fn_800BF818`/`fn_800BFD84` are
 *   a GUESS for NintendoWare's `DrawPointStrategy::Draw`/`SetupState` (their asserts on lines 92/96/98 and 150
 *   name `pm`, `pm->mResource` and `&ed`).
 * RESIDUALS. The source defines the FIFO writers and the emitter before `Draw`, so our `.text` (and the extab and
 *   extabindex records) run in a different order from retail's address order.
 *   2 partial rows:
 *  - `fn_800BF818`: the two table getter calls load the table through r5 where retail uses r3 (the `mr r3,self`
 *    schedule), and one `lwz` and one `lfs` are placed differently;
 *  - `fn_800BFD60` (ours 0x20 of 0x24): retail has a `b` to the next instruction between the three `lfs` and the
 *    `lis` of the FIFO base (the `ef/ef_drawlinestrategy.cpp` row `fn_800BF58C` is the same).
 *   flipcheck: `.data`, `.sdata` and `.sdata2` claimed, not emitted; `.text` 0x7B8 of 0x7BC.
 * SHAPES. The `Panic` line numbers are literals (92/96/98 in `Draw`, 150 in the state setup), so the source's
 *   line count does not move them.
 */

#include "ef.h"
#include "gx.h"
#include "ef/ef_drawstrategy.h"
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

#pragma peephole off

/* --------------------------------------------------------------------------------------------------
 * externs owned by neighbouring translation units and the SDK.
 * -------------------------------------------------------------------------------------------------- */

EfEmitterShape* fn_800AB388(void* emitter);
f32 fn_800AB3AC(void* particle);
f32 fn_800B5A48(void);
f32* fn_800BF5B0(f32* limit, f32* value);
void fn_800AE360(EfDrawArgs* args, Mtx34* mtx);
void fn_800B7DB0(void* em, Mtx34* mtx);
void fn_800710BC(Mtx34* out, Mtx34* a, Mtx34* b);
Mtx34* fn_80050508(Mtx34* mtx);
void fn_800C6064(EfDrawStrategyObj* self, EfDrawArgs* args, EfEmitterShape* shape, void* em);
void fn_800C68E8(EfDrawStrategyObj* self, void* particle, EfEmitterShape* shape, void* em, u32 first,
                 u32 texcoord);
void fn_800B4B04(void* self, int mode);
void fn_800C5F74(void* self);

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
extern char lbl_80594778[]; /* the DrawSmoothStripeStrategy table                       .data 0x80594778 */
extern f32 lbl_80791350;    /* 42.5f - the point-size clamp                            .sdata  0x80791350 */
extern f32 lbl_807961B8;    /* 6.0f  - the point-size scale                           .sdata2 0x807961B8 */
extern f32 lbl_807961BC;    /* 0.0f  - the point texcoord                             .sdata2 0x807961BC */

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
        fn_800BFD3C(lbl_807961BC, lbl_807961BC);
    }
    fn_800BFD38();
}

/* Sets the point GX state (texcoord offsets, vertex format, current matrix). */
void fn_800BFD84(EfDrawStrategyObj* self, void* em, EfDrawArgs* args) {
    if (!IsValidPointer((u32)args)) {
        nw4r::db::Panic(lbl_80594408, 150, lbl_80594424, args);
    }
    fn_800C6064(self, args, fn_800AB388(args->emitter), em);
    GXEnableTexOffsets(0, 1, 1);
    GXClearVtxDesc();
    GXSetVtxDesc(9, 1);
    if (self->flag_0xD0 != 0) {
        GXSetVtxDesc(0xD, 1);
    }
    GXSetVtxAttrFmt(0, 9, 1, 4, 0);
    GXSetVtxAttrFmt(0, 0xD, 1, 4, 0);
    GXSetCurrentMtx(0);
}

/* --------------------------------------------------------------------------------------------------
 * DrawPointStrategy::Draw - the point particle walker.
 * -------------------------------------------------------------------------------------------------- */

void fn_800BF818(EfDrawStrategyObj* self, void* em, EfDrawArgs* args) {
    EfEmitterShape* shape;
    u32 flag;
    EfPointWalkerFn first_fn;
    EfPointWalkerFn next_fn;
    u32 last_size;
    u32 first;
    EfParticleRecord* particle;
    Mtx34 mtx_view;
    Mtx34 mtx_result;
    f32 progress;
    f32 scale;
    u32 size;

    if (!IsValidPointer((u32)args)) {
        nw4r::db::Panic(lbl_80594408, 92, lbl_80594424, args);
    }
    fn_800BFD84(self, em, args);
    if (!IsValidPointer((u32)args->emitter)) {
        nw4r::db::Panic(lbl_80594408, 96, lbl_80594458, args->emitter);
    }
    shape = fn_800AB388(args->emitter);
    if (!IsValidPointer((u32)shape)) {
        nw4r::db::Panic(lbl_80594408, 98, lbl_80594494, shape);
    }

    flag = (self->flag_0xD0 != 0);

    MTX34_ctor(&mtx_view);
    fn_800AE360(args, &mtx_view);
    MTX34_ctor(&mtx_result);
    fn_800B7DB0(em, &mtx_result);
    fn_800710BC(&mtx_result, &mtx_result, &mtx_view);
    GXLoadPosMtxImm(fn_80050508(&mtx_result), 0);

    first_fn = self->vtable->get_first(self, shape->flags_0x00 & 0x800);
    next_fn = self->vtable->get_next(self, shape->flags_0x00 & 0x800);

    last_size = 0;
    first = 1;
    scale = lbl_807961B8;
    for (particle = (EfParticleRecord*)((void* (*)(void*))first_fn)(args); particle != 0;
         particle = (EfParticleRecord*)((void* (*)(void*, void*))next_fn)(args, particle)) {
        progress = fn_800AB3AC(particle);
        if (progress < fn_800B5A48()) {
            continue;
        }
        size = (u32)(s32)(scale * *fn_800BF5B0(&lbl_80791350, &progress));
        if ((u8)size != 0) {
            if ((u8)last_size != (u8)size) {
                last_size = size;
                GXSetPointSize((u8)size, 5);
                fn_800C68E8(self, particle, shape, em, first, 1);
            } else {
                fn_800C68E8(self, particle, shape, em, first, 0);
            }
            first = 0;
            fn_800BFCCC(&particle->world_pos, flag);
        }
    }
}

/* --------------------------------------------------------------------------------------------------
 * The deleting destructor and the constructor.
 * -------------------------------------------------------------------------------------------------- */

/* A deleting destructor: tears down the base and, when the flag is positive, frees the storage. */
void* fn_800BFF3C(void* self, s16 flag) {
    if (self != 0) {
        fn_800B4B04(self, 0);
        if ((s16)flag > 0) {
            operator delete(self);
        }
    }
    return self;
}

/* Constructs the DrawSmoothStripeStrategy: runs the base constructor and installs lbl_80594778. */
void** fn_800BFF98(void** self) {
    fn_800C5F74(self);
    self[0] = (void*)lbl_80594778;
    return self;
}

#ifdef __cplusplus
}
#endif
