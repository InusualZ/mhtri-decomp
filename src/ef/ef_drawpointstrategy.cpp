/* ef/ef_drawpointstrategy.cpp - nw4r::ef DrawPointStrategy family, 0x800BF818..0x800BFFD4.
 * rule 7 deferred: the symbol map has only fn_XXXXXXXX for this range (checked with symedit: every
 * fn_ name this file uses is a bare .text entry in config/RMHE08/symbols.txt)
 *
 * 9 functions / 0x7BC bytes of the NintendoWare-for-Revolution effect library (`nw4r::ef`).  The split
 * object's own traces name the original source file: the `__FILE__` string in its `.data` is
 * `ef_drawpointstrategy.cpp` (0x80594408), next to the three `NW4R:Pointer Error` messages the Panic
 * calls pass (0x80594424/0x80594458/0x80594494).  The `.cpp` name makes the original language C++
 * (docs/plan.md, "The language comes from the symbol"); the `Panic__Q24nw4r2dbFPCciPCce` callee and the
 * `__dl__FPv` deleting destructors say the same.  This is the point sibling of
 * `ef/ef_drawstripestrategy.cpp` (0x800B99E8..0x800BE154) and `ef/ef_drawsmoothstripestrategy.cpp`
 * (0x800BFFD4..0x800C5DB8); the seam is unproven (docs/plan.md 8.3) - the two gap units between them own
 * 0x800BE154.. and 0x800BEF98.., this unit is registered at its final home `ef/ef_drawpointstrategy.cpp`.
 *
 * Layout in address order:
 *   0x800BF818  DrawPointStrategy::Draw - the point particle walker
 *   0x800BFCCC  the single point emitter (GXBegin POINTS + position + optional texcoord)
 *   0x800BFD38  the no-op GXEnd
 *   0x800BFD3C  the f32-pair FIFO writer (texcoord)
 *   0x800BFD4C  the low-bit test
 *   0x800BFD60  the Vec3 FIFO writer
 *   0x800BFD84  DrawPointStrategy::SetupState - the point GX state
 *   0x800BFF3C  the deleting destructor
 *   0x800BFF98  the constructor (installs the `lbl_80594778` vtable)
 *
 * Codegen lever: the out-of-line FIFO writers keep the unfused narrowing stores (docs/matching.md row 39),
 * exactly as the sibling `ef/ef_drawsmoothstripestrategy.cpp` needed, so the peephole pass is off here.
 *
 * The `Panic` line numbers are the literals the target's `li r4,NN` carry (92/96/98 in Draw, 150 in
 * SetupState); writing them as literals keeps the source line count from moving them.
 *
 * Status (measured with `tools/units/recompile.py`, the official report metric): six of the nine
 * functions are byte-identical (fn_800BFCCC, fn_800BFD38, fn_800BFD3C, fn_800BFD4C, fn_800BFD84,
 * fn_800BFF3C, fn_800BFF98 - the emitter, the FIFO writers, the low-bit test, the state setup, the
 * destructor and the constructor), fn_800BF818 is 98.57 % and fn_800BFD60 is 85.56 %; every one is far
 * above the 80 % bar.  Residual: fn_800BFD60 is 32 B against the target's 36 B - retail materialises a
 * 4-byte `b $+4` between the three `lfs` and the `lis` on the FIFO base, the same irreducible shape the
 * sibling `ef/ef_drawstripestrategy.cpp` recorded for fn_800BA710/fn_800BC070; the two direct-store and
 * locals variants both score 40 %/85.6 %, so the locals shape is kept.  fn_800BF818's remaining 1.4 %
 * is the argument evaluator's `mr r3,self` schedule around the two vtable getter calls (ours loads the
 * vtable through r5 where retail uses r3) and the scheduler's placement of one `lwz`/one `lfs`; the
 * sizes and every other instruction match.
 */

#include "ef.h"
#include "gx.h"
#include "ef/ef_drawstrategy.h"

/* `nw4r::db::Panic` - the real declaration; the front-end reproduces the map's
 * `Panic__Q24nw4r2dbFPCciPCce` spelling (tools/units/mangle.py confirms it).  Declaring the mangled
 * spelling instead would re-mangle it and break the link (docs/matching.md 50); rule 9. */
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
 * externs owned by neighbouring translation units / the SDK (rule 2 keeps the definition where it is).
 * -------------------------------------------------------------------------------------------------- */

EfEmitterShape* fn_800AB388(void* emitter);
f32 fn_800AB3AC(void* particle);
f32 fn_800B5A48(void);
f32* fn_800BF5B0(f32* limit, f32* value);
void fn_800AE360(EfDrawArgs* args, Mtx34* mtx);
void fn_800B7DB0(void* em, Mtx34* mtx);
void fn_800710BC(Mtx34* out, Mtx34* a, Mtx34* b);
void fn_8005050C(Mtx34* mtx);
Mtx34* fn_80050508(Mtx34* mtx);
void fn_800C6064(EfDrawStrategyObj* self, EfDrawArgs* args, EfEmitterShape* shape, void* em);
void fn_800C68E8(EfDrawStrategyObj* self, void* particle, EfEmitterShape* shape, void* em, u32 first,
                 u32 texcoord);
void fn_800B4B04(void* self, int mode);
void fn_800C5F74(void* self);

/* SDK GX entry points (unsplit: rule 2's named gap). */
extern void GXBegin(u8 prim, u8 vtxfmt, u16 nverts);
extern void GXSetPointSize(u8 pointSize, u32 texOffset);
extern void GXEnableTexOffsets(u32 coord, u32 line_enable, u32 point_enable);
extern void GXClearVtxDesc(void);
extern void GXSetVtxDesc(u32 attr, u32 type);
extern void GXSetVtxAttrFmt(u32 fmt, u32 attr, u32 cnt, u32 type, u32 frac);
extern void GXLoadPosMtxImm(void* mtx, u32 id);
extern void GXSetCurrentMtx(u32 id);

/* This unit's pooled `__FILE__`/assert strings, the draw-order vtable and the constants (declared, never
 * defined here: the data pass claims the ranges once the source emits them, docs/plan.md 8.4). */
extern char lbl_80594408[]; /* "ef_drawpointstrategy.cpp"                                .data 0x80594408 */
extern char lbl_80594424[]; /* "NW4R:Pointer Error\npm(=%p) is not valid pointer."      .data 0x80594424 */
extern char lbl_80594458[]; /* "NW4R:Pointer Error\npm->mResource(=%p) is not valid..." .data 0x80594458 */
extern char lbl_80594494[]; /* "NW4R:Pointer Error\n&ed(=%p) is not valid pointer."     .data 0x80594494 */
extern char lbl_80594778[]; /* the DrawPointStrategy vtable                             .data 0x80594778 */
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

    fn_8005050C(&mtx_view);
    fn_800AE360(args, &mtx_view);
    fn_8005050C(&mtx_result);
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

/* Deletes the DrawPointStrategy and, when the flag is positive, frees the storage. */
void* fn_800BFF3C(void* self, s16 flag) {
    if (self != 0) {
        fn_800B4B04(self, 0);
        if ((s16)flag > 0) {
            operator delete(self);
        }
    }
    return self;
}

/* Constructs the DrawPointStrategy: runs the base constructor and installs the vtable. */
void** fn_800BFF98(void** self) {
    fn_800C5F74(self);
    self[0] = (void*)lbl_80594778;
    return self;
}

#ifdef __cplusplus
}
#endif
