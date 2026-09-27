/* ef/ef_drawstripestrategy.cpp - nw4r::ef draw-strategy family, 0x800B99E8..0x800BE154.
 * rule 7 deferred: the symbol map has only fn_XXXXXXXX for this range (checked with symedit: every
 * fn_ name this file uses is a bare .text entry in config/RMHE08/symbols.txt)
 *
 * 37 functions / 0x4770 bytes of the NintendoWare-for-Revolution effect library (`nw4r::ef`).  The
 * split region is a **maximal unclaimed run across three original translation units** - the
 * `__FILE__` strings in the object's `.data` are `ef_drawstripestrategy.cpp` (0x805939E8, 0x80593D0C,
 * 0x80593D5C, 0x80593DAC - four copies), `ef_drawbillboardstrategy.cpp` (0x80593E88) and
 * `ef_drawdirectionalstrategy.cpp` (0x80594070).  The run opens on the DrawStripeStrategy deleting
 * destructor (its vtable `lbl_80593CB4` sits beside the `ef_drawstripestrategy.cpp` string), so the
 * dominant, first-owned name is `ef_drawstripestrategy.cpp`: that is this unit's final home.  The
 * seam is unproven (docs/plan.md 8.3) - `ef_drawbillboardstrategy.cpp` owns 0x800B9A44/0x800B9A80/
 * 0x800BBD90 and `ef_drawdirectionalstrategy.cpp` owns 0x800BBDEC/0x800BC1B4/0x800BE0BC, both kept
 * whole by the attribution pass.
 *
 * The `.cpp` spelling makes the original language C++ (docs/plan.md, "The language comes from the
 * symbol"); the `Panic__Q24nw4r2dbFPCciPCce` callee and the `__dl__FPv` deleting destructors say the
 * same.  This file is registered as `.cpp` and every definition sits inside an `extern "C"` guard so
 * the front-end keeps the map's `fn_XXXXXXXX` spelling (docs/matching.md row 42) instead of mangling
 * it.
 *
 * Layout in address order (37 symbols):
 *   0x800B99E8  the DrawStripeStrategy deleting destructor
 *   0x800B9A44  the DrawBillboardStrategy constructor
 *   0x800B9A80  the per-particle draw-strategy dispatch (three `nw4r::ef::Emitter*` shape branches)
 *   0x800B9DF8/0x800BA854/0x800BAFBC  the point/stripe/tube particle walkers
 *   0x800BA1E0  the four-vertex stripe writer
 *   0x800BA6E8..0x800BA710  the out-of-line GX FIFO writers (empty/u8/bit/vec3)
 *   0x800BA734  the indexed four-vertex stripe writer
 *   0x800BAC30  the tube writer
 *   0x800BAFB0  the two-f32 accessor
 *   0x800BB488  the four-vertex tube writer
 *   0x800BB748  the stripe GX state setup
 *   0x800BB93C  the DrawBillboardStrategy emitter-shape dispatch
 *   0x800BBBBC/0x800BBCF8  the two ahead-context position resolvers
 *   0x800BBD90  the DrawBillboardStrategy deleting destructor
 *   0x800BBDEC  the DrawDirectionalStrategy constructor
 *   0x800BBE28  the four-vertex matrix writer
 *   0x800BC048..0x800BC070  the out-of-line GX FIFO writers (second copy)
 *   0x800BC094  the indexed four-vertex writer (second copy)
 *   0x800BC1B4  the DrawDirectionalStrategy emitters
 *   0x800BC41C  the particle flag accessor
 *   0x800BC428/0x800BD234  the large per-particle emitters
 *   0x800BCD14/0x800BCE98/0x800BDB60/0x800BDD34  the layer/tube emitters
 *   0x800BE0BC  the DrawDirectionalStrategy deleting destructor
 *   0x800BE118  the next constructor
 *
 * Codegen lever: this unit needs the peephole pass off - the GX FIFO writers keep the unfused
 * `clrlwi`/`extsh`/`extsb` in front of every narrowing store, exactly as the sibling
 * `ef/ef_drawsmoothstripestrategy.cpp` and `gx/fn_8009AA78.c` did.
 *
 * Status (measured against the retired auto_fn_<addr> target objects with the official report metric):
 * 27 of the 37 symbols are written, every one of them at or above 80 % - 18 byte-identical (the
 * constructors/destructors, the GX FIFO writers, the indexed four-vertex writers, the matrix writer
 * and the ahead resolver fn_800BBCF8).  Residual: fn_800BA710/fn_800BC070 (85.6 %) miss only the
 * 4-byte `b` retail materialises between the three vector loads and the FIFO base load;
 * fn_800BB748/fn_800BDB60 (94.9/95.9 %) differ in the GXSetArray sda21 access; fn_800BBBBC (94.9 %)
 * in the setVec3/copyVec3 pairing; fn_800B9A80/fn_800BB93C/fn_800BC1B4/fn_800BDD34 (98-99 %)
 * in the inlined IsValidPointer short-circuit on one guard.  The eleven unwritten emitters
 * (fn_800B9DF8, fn_800BA1E0, fn_800BA854, fn_800BAC30, fn_800BAFBC, fn_800BB488, fn_800BC428,
 * fn_800BCD14, fn_800BCE98, fn_800BD234) are large nw4r paired-single (AltiVec) particle walkers that
 * m2c cannot recover - they are the recorded residual, not a finished translation.
 */

#include "ef.h"
#include "gx.h"
#include "sys_mem.h"
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "fn_8004CAD8/mtx.h" /* the symbols deleted above (rule 2) */

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
 * externs owned by the neighbouring ef translation units (declared here, never defined: rule 2 keeps
 * the definition in the TU that owns the symbol).
 * -------------------------------------------------------------------------------------------------- */

void fn_800B4B04(void* self, int mode);
void fn_800C5F74(void* self);

/* The walker family this unit's dispatch selects. */
void fn_800B9DF8(void* self, void* em, void* args);
void fn_800BA854(void* self, void* em, void* args);
void fn_800BAFBC(void* self, void* em, void* args);

/* The per-particle draw request and the emitter shape record it points at.  Only the fields this
 * unit reads are named. */
typedef struct EfDrawArgs {
    u8 pad_0x00[0x24]; /* +0x00 */
    void* emitter;     /* +0x24  the emitter/effect record the shape comes from */
} EfDrawArgs; /* size: 0x28 (lower bound, the record continues past what this unit reads) */

typedef struct EfEmitterShape {
    u8 pad_0x00[0xAD]; /* +0x00 */
    u8 shape_0xAD;     /* +0xAD  0/3 = point, 1 = stripe, 2 = tube */
    u8 shape_0xAE;     /* +0xAE  the walker selector */
} EfEmitterShape; /* size: 0xAF (lower bound, the record continues past what this unit reads) */

extern EfEmitterShape* fn_800AB388(void* emitter);

/* The draw-strategy object header: the draw order and a flag word this unit reads at +0xD0. */
typedef struct EfDrawStrategyObj {
    u8 pad_0x00[0xD0]; /* +0x00 */
    u8 flag_0xD0;      /* +0xD0  non-zero when the draw order is set */
} EfDrawStrategyObj; /* size: 0xD1 (lower bound, the record continues past what this unit reads) */

/* The ahead-context argument block the walkers read: the particle handle and two cached positions. */
typedef struct EfAheadArgs {
    void* particle;    /* +0x00 */
    u8 pad_0x04[0x94]; /* +0x04 */
    Vec prev_pos;      /* +0x98  the previous resolved position */
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
void fn_800B87C8(void);
void fn_800B87D0(void);
void fn_800B882C(void);
void fn_800B8788(void);
void fn_800BBCF8(Vec* out, EfAheadArgs* args, EfWalkerObj* em);
void fn_800BBBBC(Vec* out, EfAheadArgs* args, EfWalkerObj* em);
typedef void (*EfWalkerFn)(void);

/* The large per-particle emitters this unit dispatches to (defined further down). */
void fn_800BC428(void* self, void* em, EfDrawArgs* args);
void fn_800BD234(void* self, void* em, EfDrawArgs* args);
void fn_800BDB60(EfDrawStrategyObj* self, void* em, EfDrawArgs* args);
u32 fn_800BC41C(void* unused, EfParticleFlags* self);

/* SDK GX state setters and the mtx helpers (unsplit / SDK: rule 2's named gap). */
extern void fn_800504D4(Mtx34* mtx);
extern void fn_80050508(Mtx34* mtx);
void fn_800C6064(void* self, EfDrawArgs* args, EfEmitterShape* shape, void* em);
extern void GXEnableTexOffsets(u32 coord, u32 line_enable, u32 point_enable);
extern void GXSetArray(u32 attr, const void* base, u8 stride);
extern void GXClearVtxDesc(void);
extern void GXSetVtxDesc(u32 attr, u32 type);
extern void GXSetVtxAttrFmt(u32 fmt, u32 attr, u32 cnt, u32 type, u32 frac);
extern void GXLoadPosMtxImm(void* mtx, u32 id);
extern void GXSetCurrentMtx(u32 id);
extern char lbl_80791300[]; /* the GX position/normal array descriptor (.sdata) */
extern char lbl_80791320[]; /* the GX texcoord array descriptor (.sdata) */

/* The ahead-context walkers: the particle/emitter lookup helpers and the small vector helpers they
 * use (all still `fn_*`/SDK, defined outside this range). */
extern void PSVECSubtract(Vec* out, Vec* a, Vec* b);
extern int fn_800B59E4(Vec* v);
extern EfWalkerObj* fn_800B5ACC(void* particle, EfWalkerObj* em);
extern EfWalkerObj* fn_800B8D48(void* particle, EfWalkerObj* em);
extern f32 lbl_80796154; /* the zero/one constant the walkers initialise with (.sdata2) */

/* This unit's pooled `__FILE__`/assert strings and the vtables the constructors write.  They are
 * declared, never defined here: the data pass claims the ranges once the source emits them
 * (docs/plan.md 8.4), so a definition would move the pool. */
extern char lbl_80593E88[]; /* "ef_drawbillboardstrategy.cpp"                                 .data 0x80593E88 */
extern char lbl_80593EA8[]; /* "NW4R:Pointer Error\npm(=%p) is not valid pointer."           .data 0x80593EA8 */
extern char lbl_80593EDC[]; /* "NW4R:Pointer Error\npm->mResource(=%p) is not valid..."     .data 0x80593EDC */
extern char lbl_80593F18[]; /* "NW4R:Pointer Error\n&ed(=%p) is not valid pointer."         .data 0x80593F18 */
extern char lbl_80593F4C[]; /* the DrawBillboardStrategy vtable                             .data 0x80593F4C */
extern char lbl_80594070[]; /* "ef_drawdirectionalstrategy.cpp"                             .data 0x80594070 */
extern char lbl_80594090[]; /* "NW4R:Pointer Error\np(=%p) is not valid pointer."           .data 0x80594090 */
extern char lbl_805941D8[]; /* the DrawDirectionalStrategy vtable                           .data 0x805941D8 */
extern char lbl_80594318[]; /* the next strategy vtable                                     .data 0x80594318 */
extern char lbl_805940C0[]; /* "NW4R:Pointer Error\npm(=%p) is not valid pointer."           .data 0x805940C0 */
extern char lbl_805940F4[]; /* "NW4R:Pointer Error\npm->mResource(=%p) is not valid..."     .data 0x805940F4 */
extern char lbl_80594130[]; /* "NW4R:Pointer Error\n&ed(=%p) is not valid pointer."         .data 0x80594130 */

/* --------------------------------------------------------------------------------------------------
 * The DrawStripeStrategy constructors and deleting destructors.
 * -------------------------------------------------------------------------------------------------- */

/* Deletes the DrawStripeStrategy and, when the flag is positive, frees the storage. */
void* fn_800B99E8(void* self, s16 flag) {
    if (self != 0) {
        fn_800B4B04(self, 0);
        if ((s16)flag > 0) {
            operator delete(self);
        }
    }
    return self;
}

/* Constructs the DrawBillboardStrategy: runs the base constructor and installs the vtable. */
void** fn_800B9A44(void** self) {
    fn_800C5F74(self);
    self[0] = (void*)lbl_80593F4C;
    return self;
}

/* Dispatches one draw request to the point/stripe/tube walker its emitter shape selects. */
void fn_800B9A80(void* self, void* em, EfDrawArgs* args) {
    EfEmitterShape* shape;

    if (!IsValidPointer((u32)args)) {
        nw4r::db::Panic(lbl_80593E88, 502, lbl_80593EA8, args);
    }
    if (!IsValidPointer((u32)args->emitter)) {
        nw4r::db::Panic(lbl_80593E88, 503, lbl_80593EDC, args->emitter);
    }
    shape = fn_800AB388(args->emitter);
    if (!IsValidPointer((u32)shape)) {
        nw4r::db::Panic(lbl_80593E88, 506, lbl_80593F18, shape);
    }
    switch (shape->shape_0xAD) {
    case 0:
    case 3:
        fn_800B9DF8(self, em, args);
        break;
    case 1:
        fn_800BA854(self, em, args);
        break;
    case 2:
        fn_800BAFBC(self, em, args);
        break;
    }
}

/* The base deleting destructor (DrawStrategyImpl). */
void* fn_800BBD90(void* self, s16 flag) {
    if (self != 0) {
        fn_800B4B04(self, 0);
        if ((s16)flag > 0) {
            operator delete(self);
        }
    }
    return self;
}

/* The base constructor (DrawStrategyImpl). */
void** fn_800BBDEC(void** self) {
    fn_800C5F74(self);
    self[0] = (void*)lbl_805941D8;
    return self;
}

/* The second deleting destructor. */
void* fn_800BE0BC(void* self, s16 flag) {
    if (self != 0) {
        fn_800B4B04(self, 0);
        if ((s16)flag > 0) {
            operator delete(self);
        }
    }
    return self;
}

/* The second constructor. */
void** fn_800BE118(void** self) {
    fn_800C5F74(self);
    self[0] = (void*)lbl_80594318;
    return self;
}

/* --------------------------------------------------------------------------------------------------
 * The out-of-line GX FIFO writers, in address order (the same family the sibling
 * ef_drawsmoothstripestrategy.cpp carries at 0x800C6F90.. and ef_drawstrategyimpl.cpp at 0x800BC...).
 * -------------------------------------------------------------------------------------------------- */

/* Ends the current FIFO command (the SDK's `GXEnd`, which writes nothing). */
void fn_800BA6E8(void) {}

/* Writes one u8 to the pipe. */
void fn_800BA6EC(u8 value) {
    GXWGFifo.u8 = value;
}

/* Tests the low bit of a status word. */
int fn_800BA6FC(u32 value) {
    return (value & 1) != 0;
}

/* Writes a vector to the pipe. */
void fn_800BA710(Vec* v) {
    f32 x = v->x;
    f32 y = v->y;
    f32 z = v->z;
    GXWGFifo.f32 = x;
    GXWGFifo.f32 = y;
    GXWGFifo.f32 = z;
}

/* Writes a pair of f32 to the pipe. */
void fn_800BAFB0(f32* dst, f32 x, f32 y) {
    dst[0] = x;
    dst[1] = y;
}

/* Ends the current FIFO command (second copy). */
void fn_800BC048(void) {}

/* Writes one u8 to the pipe (second copy). */
void fn_800BC04C(u8 value) {
    GXWGFifo.u8 = value;
}

/* Tests the low bit of a status word (second copy). */
int fn_800BC05C(u32 value) {
    return (value & 1) != 0;
}

/* Writes a vector to the pipe (second copy). */
void fn_800BC070(Vec* v) {
    f32 x = v->x;
    f32 y = v->y;
    f32 z = v->z;
    GXWGFifo.f32 = x;
    GXWGFifo.f32 = y;
    GXWGFifo.f32 = z;
}

/* --------------------------------------------------------------------------------------------------
 * The indexed four-vertex stripe writers.  They expand a (matrix, two positions) pair into four
 * vertices through `fn_80050CA0`/`fn_80051378` and emit them to the pipe, tagging every other vertex
 * with its index when the draw record asks for it.
 * -------------------------------------------------------------------------------------------------- */

/* GX is an unsplit SDK band (rule 2's named gap): declared here. */
extern void GXBegin(u8 prim, u8 vtxfmt, u16 nverts);
extern void fn_80050CA0(Vec* out, void* mtx, Vec* in);
extern void fn_80051378(Vec* out, void* mtx, Vec* in);
extern void fn_800514FC(Vec* out, void* mtx, Vec* in);

/* The stripe writer (first copy): four vertices from the matrix and two positions. */
void fn_800BA734(void* unused, void* mtx, Vec* a, Vec* b, u32 flags) {
    Vec v0;
    Vec v1;
    Vec v2;
    Vec v3;

    GXBegin(0x80, 0, 4);
    fn_80050CA0(&v0, mtx, a);
    fn_800BA710(&v0);
    if (fn_800BA6FC(flags) != 0) {
        fn_800BA6EC(0);
    }
    fn_80050CA0(&v1, mtx, b);
    fn_800BA710(&v1);
    if (fn_800BA6FC(flags) != 0) {
        fn_800BA6EC(1);
    }
    fn_80051378(&v2, mtx, a);
    fn_800BA710(&v2);
    if (fn_800BA6FC(flags) != 0) {
        fn_800BA6EC(2);
    }
    fn_80051378(&v3, mtx, b);
    fn_800BA710(&v3);
    if (fn_800BA6FC(flags) != 0) {
        fn_800BA6EC(3);
    }
    fn_800BA6E8();
}

/* The stripe writer (second copy): four vertices from the matrix and two positions. */
void fn_800BC094(void* mtx, Vec* a, Vec* b, u32 flags) {
    Vec v0;
    Vec v1;
    Vec v2;
    Vec v3;

    GXBegin(0x80, 0, 4);
    fn_80050CA0(&v0, mtx, a);
    fn_800BC070(&v0);
    if (fn_800BC05C(flags) != 0) {
        fn_800BC04C(0);
    }
    fn_80050CA0(&v1, mtx, b);
    fn_800BC070(&v1);
    if (fn_800BC05C(flags) != 0) {
        fn_800BC04C(1);
    }
    fn_80051378(&v2, mtx, a);
    fn_800BC070(&v2);
    if (fn_800BC05C(flags) != 0) {
        fn_800BC04C(2);
    }
    fn_80051378(&v3, mtx, b);
    fn_800BC070(&v3);
    if (fn_800BC05C(flags) != 0) {
        fn_800BC04C(3);
    }
    fn_800BC048();
}

/* Selects the ahead-context position resolver for one draw request's emitter shape. */
EfWalkerFn fn_800BB93C(void* unused, EfDrawArgs* args) {
    EfEmitterShape* shape;

    if (!IsValidPointer((u32)args->emitter)) {
        nw4r::db::Panic(lbl_80593E88, 784, lbl_80593EDC, args->emitter);
    }
    shape = fn_800AB388(args->emitter);
    if (!IsValidPointer((u32)shape)) {
        nw4r::db::Panic(lbl_80593E88, 786, lbl_80593F18, shape);
    }
    switch (shape->shape_0xAE) {
    case 0:
        return (EfWalkerFn)fn_800B882C;
    case 1:
        return (EfWalkerFn)fn_800B87D0;
    case 2:
        return (EfWalkerFn)fn_800B87C8;
    case 3:
        return (EfWalkerFn)fn_800BBCF8;
    case 4:
        return (EfWalkerFn)fn_800BBBBC;
    default:
        return (EfWalkerFn)fn_800B882C;
    }
}

/* Resolves the ahead-context position of one particle against its emitter and neighbour. */
void fn_800BBCF8(Vec* out, EfAheadArgs* args, EfWalkerObj* em) {
    EfWalkerObj* particle = fn_800B5ACC(args->particle, em);

    if (particle != 0) {
        PSVECSubtract(out, &particle->world_pos, &em->world_pos);
    } else {
        PSVECSubtract(out, &em->world_pos, &args->pos);
    }
    if (fn_800B59E4(out) == 0) {
        copyVec3(out, &args->prev_pos);
    }
}

/* Resolves the ahead-context position of one particle from its two neighbour particles. */
void fn_800BBBBC(Vec* out, EfAheadArgs* args, EfWalkerObj* em) {
    Vec a;
    Vec b;
    Vec c;
    Vec d;
    EfWalkerObj* first = fn_800B5ACC(args->particle, em);
    EfWalkerObj* second = fn_800B8D48(args->particle, em);

    setVec3(&a, lbl_80796154, lbl_80796154, lbl_80796154);
    if (first != 0) {
        PSVECSubtract(&a, &first->world_pos, &em->world_pos);
        if (fn_800B59E4(&a) == 0) {
            setVec3(&c, lbl_80796154, lbl_80796154, lbl_80796154);
            copyVec3(&a, &c);
        }
    }
    setVec3(&b, lbl_80796154, lbl_80796154, lbl_80796154);
    if (second != 0) {
        PSVECSubtract(&b, &second->world_pos, &em->world_pos);
        if (fn_800B59E4(&b) == 0) {
            setVec3(&d, lbl_80796154, lbl_80796154, lbl_80796154);
            copyVec3(&b, &d);
        }
    }
    PSVECSubtract(out, &a, &b);
    if (fn_800B59E4(out) == 0) {
        copyVec3(out, &args->prev_pos);
    }
}

/* Sets the stripe GX state (texcoord array, vertex format and position matrix). */
void fn_800BB748(EfDrawStrategyObj* self, void* em, EfDrawArgs* args) {
    Mtx34 mtx;

    if (!IsValidPointer((u32)args)) {
        nw4r::db::Panic(lbl_80593E88, 746, lbl_80593EA8, args);
    }
    fn_800C6064(self, args, fn_800AB388(args->emitter), em);
    GXEnableTexOffsets(0, 1, 1);
    GXSetArray(0xD, lbl_80791300, 2);
    GXClearVtxDesc();
    GXSetVtxDesc(9, 1);
    if (self->flag_0xD0 != 0) {
        GXSetVtxDesc(0xD, 2);
    }
    GXSetVtxAttrFmt(0, 9, 1, 4, 0);
    GXSetVtxAttrFmt(0, 0xD, 1, 0, 0);
    MTX34_ctor(&mtx);
    fn_800504D4(&mtx);
    fn_80050508(&mtx);
    GXLoadPosMtxImm(&mtx, 0);
    GXSetCurrentMtx(0);
}

/* Sets the directional stripe GX state (texcoord array and vertex format). */
void fn_800BDB60(EfDrawStrategyObj* self, void* em, EfDrawArgs* args) {
    if (!IsValidPointer((u32)args)) {
        nw4r::db::Panic(lbl_80594070, 652, lbl_805940C0, args);
    }
    fn_800C6064(self, args, fn_800AB388(args->emitter), em);
    GXEnableTexOffsets(0, 1, 1);
    GXSetArray(0xD, lbl_80791320, 2);
    GXClearVtxDesc();
    GXSetVtxDesc(9, 1);
    if (self->flag_0xD0 != 0) {
        GXSetVtxDesc(0xD, 2);
    }
    GXSetVtxAttrFmt(0, 9, 1, 4, 0);
    GXSetVtxAttrFmt(0, 0xD, 1, 0, 0);
    GXSetCurrentMtx(0);
}

/* Dispatches the directional emitter to the point/tube path its particle flags select. */
void fn_800BC1B4(EfDrawStrategyObj* self, void* em, EfDrawArgs* args) {
    if (!IsValidPointer((u32)args)) {
        nw4r::db::Panic(lbl_80594070, 342, lbl_805940C0, args);
    }
    fn_800BDB60(self, em, args);
    if (!IsValidPointer((u32)args->emitter)) {
        nw4r::db::Panic(lbl_80594070, 346, lbl_805940F4, args->emitter);
    }
    if (fn_800BC41C(self, (EfParticleFlags*)fn_800AB388(args->emitter)) != 1) {
        fn_800BC428(self, em, args);
        return;
    }
    fn_800BD234(self, em, args);
}

/* The four-vertex matrix writer: transforms four positions by a matrix and emits them. */
void fn_800BBE28(void* mtx, Vec* src, u32 flags) {
    Vec v0;
    Vec v1;
    Vec v2;
    Vec v3;

    if (!IsValidPointer((u32)src)) {
        nw4r::db::Panic(lbl_80594070, 92, lbl_80594090, src);
    }
    VEC3_ctor((VEC3*)&v0);
    VEC3_ctor((VEC3*)&v1);
    VEC3_ctor((VEC3*)&v2);
    VEC3_ctor((VEC3*)&v3);
    fn_800514FC(&v0, mtx, &src[0]);
    fn_800514FC(&v1, mtx, &src[1]);
    fn_800514FC(&v2, mtx, &src[2]);
    fn_800514FC(&v3, mtx, &src[3]);
    GXBegin(0x80, 0, 4);
    fn_800BC070(&v0);
    if (fn_800BC05C(flags) != 0) {
        fn_800BC04C(0);
    }
    fn_800BC070(&v1);
    if (fn_800BC05C(flags) != 0) {
        fn_800BC04C(1);
    }
    fn_800BC070(&v2);
    if (fn_800BC05C(flags) != 0) {
        fn_800BC04C(2);
    }
    fn_800BC070(&v3);
    if (fn_800BC05C(flags) != 0) {
        fn_800BC04C(3);
    }
    fn_800BC048();
}

/* Selects the directional walker for one draw request's emitter shape. */
EfWalkerFn fn_800BDD34(void* unused, EfDrawArgs* args) {
    EfEmitterShape* shape;

    if (!IsValidPointer((u32)args)) {
        nw4r::db::Panic(lbl_80594070, 687, lbl_805940C0, args);
    }
    if (!IsValidPointer((u32)args->emitter)) {
        nw4r::db::Panic(lbl_80594070, 688, lbl_805940F4, args->emitter);
    }
    shape = fn_800AB388(args->emitter);
    if (!IsValidPointer((u32)shape)) {
        nw4r::db::Panic(lbl_80594070, 691, lbl_80594130, shape);
    }
    switch (shape->shape_0xAE) {
    case 0:
        return (EfWalkerFn)fn_800B882C;
    case 1:
        return (EfWalkerFn)fn_800B87D0;
    case 2:
        return (EfWalkerFn)fn_800B87C8;
    case 3:
        return (EfWalkerFn)fn_800BBCF8;
    case 5:
    case 7:
        return (EfWalkerFn)fn_800B8788;
    case 6:
        return (EfWalkerFn)fn_800BBBBC;
    default:
        return (EfWalkerFn)fn_800B882C;
    }
}

/* The first two draw/rotate flag bits of a particle. */
u32 fn_800BC41C(void* unused, EfParticleFlags* self) {
    return self->flags_0xB2 & 3;
}

#ifdef __cplusplus
}
#endif
