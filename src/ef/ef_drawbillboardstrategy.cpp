/*
 * ef/ef_drawbillboardstrategy.cpp - nw4r::ef DrawBillboardStrategy and DrawDirectionalStrategy: their
 *   constructors and deleting destructors, the per-particle draw dispatch and emitter-shape dispatch, the point,
 *   stripe and tube particle walkers and writers, two copies of the out-of-line GX FIFO writers and the indexed
 *   four-vertex writers, the stripe GX state, the ahead-context position resolvers and the directional emitters.
 * RANGE. .text 0x800B9A44-0x800BE154 (36 functions); extab 0x8000A23C-0x8000A30C, extabindex 0x80023700-0x80023838,
 *   .rodata 0x8056F770-0x8056F7D0, .data 0x80593E88-0x805941F8, .sdata 0x80791300-0x80791340, .sdata2
 *   0x80796150-0x80796190.  The range holds two TUs by their `__FILE__` strings: "ef_drawbillboardstrategy.cpp"
 *   (0x80593E88) and "ef_drawdirectionalstrategy.cpp" (0x80594070, its bodies from 0x800BBDEC); the seam between
 *   them is open.  Left edge: the "ef_drawstripestrategy.cpp" strings belong to the code before 0x800B9A44.
 *   Unproven seam (playbook 80: a TU's tables sit late in its `.data`): the range ends with a deleting destructor and
 *   the constructor `fn_800BE118`, which installs `ef/ef_drawfreestrategy.cpp`'s table `lbl_80594318`, so that
 *   constructor may belong to the next unit.
 * FLAGS. `cflags_main`; `#pragma peephole off` (the GX FIFO writers keep the unfused `clrlwi`/`extsh`/`extsb` in
 *   front of every narrowing store).
 * NAMES. The map has only `fn_` stems for the range; every definition is `extern "C"` to keep them (playbook 42).
 * RESIDUALS. 10 rows unwritten (declared, never defined): 0x800B9DF8-0x800BA6E8, 0x800BA854-0x800BAFB0,
 *   0x800BAFBC-0x800BB748, 0x800BC428-0x800BDB60 (`fn_800BA1E0` carries paired-single ops, playbook 85).  The
 *   source order differs from retail's, so `.text` and the extab and extabindex records run in another order.
 *   9 partial rows:
 *  - `fn_800BA710`, `fn_800BC070` (ours 0x20 of 0x24): retail has a `b` to the next instruction between the three
 *    `lfs` and the FIFO base and loads into f1-f3 (`ef/ef_drawlinestrategy.cpp`'s `fn_800BF58C` is the same);
 *  - `fn_800BB748`, `fn_800BDB60`: the two pointer masks share one `clrrwi`, retail's dead `li r0,0; cmpwi r0,0` is
 *    missing, and `lbl_80791300`/`lbl_80791320` are reached with `lis`/`addi` where retail uses `@sda21`;
 *  - `fn_800B9A80`, `fn_800BC1B4`, `fn_800BDD34`: one `lwz r6, 0x24(pm)` is scheduled earlier (`fn_800BC1B4` also
 *    compares `cmplwi` where retail has `cmpwi`);
 *  - `fn_800BB93C`: the `lwz r6, 0x24(r4)` comes first, so the argument registers shift down one;
 *  - `fn_800BBBBC`: the two vector helper calls pass a stack address (`addi r4, r1, ...`) where retail passes the
 *    returned pointer (`mr r4, r3`).
 *   flipcheck: `.data`, `.rodata`, `.sdata` and `.sdata2` claimed, not emitted; `.text` 0x17EC of 0x4710; extab
 *   0x80 of 0xD0; extabindex 0xC0 of 0x138.
 */

#include "ef.h"
#include "gx.h"
#include "sys_mem.h"
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "fn_8004CAD8/mtx.h" /* the matrix helpers (rule 2) */
#include "fn_8004CAD8/PSVECSubtract.h" /* PSVECSubtract, owned by fn_8004CAD8.cpp's range (rule 2) */
#include "ef/fn_800AEE48.h" /* fn_800B4B04 / fn_800B59E4 / fn_800B5ACC / fn_800B8D48, owned by ef_drawstripestrategy.cpp's range (rule 2) */
#include "ef/ef_drawstripestrategy.h" /* the walker family fn_800B8788..fn_800B882C (rule 2) */
#include "ef/ef_drawfreestrategy.h" /* lbl_80594318, the DrawFreeStrategy table (rule 2) */

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
 * Declarations: a neighbouring unit's base constructor and this unit's unwritten walkers.
 * -------------------------------------------------------------------------------------------------- */

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
void fn_800BBCF8(Vec* out, EfAheadArgs* args, EfWalkerObj* em);
void fn_800BBBBC(Vec* out, EfAheadArgs* args, EfWalkerObj* em);
typedef void (*EfWalkerFn)(void);

/* The large per-particle emitters this unit dispatches to (defined further down). */
void fn_800BC428(void* self, void* em, EfDrawArgs* args);
void fn_800BD234(void* self, void* em, EfDrawArgs* args);
void fn_800BDB60(EfDrawStrategyObj* self, void* em, EfDrawArgs* args);
u32 fn_800BC41C(void* unused, EfParticleFlags* self);

/* SDK GX state setters and the matrix helpers, declared locally. */
extern void mtx34_identity(Mtx34* mtx);
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
extern f32 lbl_80796154; /* the zero/one constant the walkers initialise with (.sdata2) */

/* This unit's `__FILE__`/assert strings and the tables the constructors write (its claimed `.data`),
 * declared, never defined. */
extern char lbl_80593E88[]; /* "ef_drawbillboardstrategy.cpp"                                 .data 0x80593E88 */
extern char lbl_80593EA8[]; /* "NW4R:Pointer Error\npm(=%p) is not valid pointer."           .data 0x80593EA8 */
extern char lbl_80593EDC[]; /* "NW4R:Pointer Error\npm->mResource(=%p) is not valid..."     .data 0x80593EDC */
extern char lbl_80593F18[]; /* "NW4R:Pointer Error\n&ed(=%p) is not valid pointer."         .data 0x80593F18 */
extern char lbl_80593F4C[]; /* the DrawBillboardStrategy vtable                             .data 0x80593F4C */
extern char lbl_80594070[]; /* "ef_drawdirectionalstrategy.cpp"                             .data 0x80594070 */
extern char lbl_80594090[]; /* "NW4R:Pointer Error\np(=%p) is not valid pointer."           .data 0x80594090 */
extern char lbl_805941D8[]; /* the DrawDirectionalStrategy vtable                           .data 0x805941D8 */
extern char lbl_805940C0[]; /* "NW4R:Pointer Error\npm(=%p) is not valid pointer."           .data 0x805940C0 */
extern char lbl_805940F4[]; /* "NW4R:Pointer Error\npm->mResource(=%p) is not valid..."     .data 0x805940F4 */
extern char lbl_80594130[]; /* "NW4R:Pointer Error\n&ed(=%p) is not valid pointer."         .data 0x80594130 */

/* --------------------------------------------------------------------------------------------------
 * The constructors and deleting destructors of this range.
 * -------------------------------------------------------------------------------------------------- */


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

/* The deleting destructor ahead of the DrawDirectionalStrategy constructor: chains the base destructor. */
void* fn_800BBD90(void* self, s16 flag) {
    if (self != 0) {
        fn_800B4B04(self, 0);
        if ((s16)flag > 0) {
            operator delete(self);
        }
    }
    return self;
}

/* Constructs the DrawDirectionalStrategy: runs the base constructor and installs lbl_805941D8. */
void** fn_800BBDEC(void** self) {
    fn_800C5F74(self);
    self[0] = (void*)lbl_805941D8;
    return self;
}

/* The deleting destructor ahead of the DrawFreeStrategy constructor: chains the base destructor. */
void* fn_800BE0BC(void* self, s16 flag) {
    if (self != 0) {
        fn_800B4B04(self, 0);
        if ((s16)flag > 0) {
            operator delete(self);
        }
    }
    return self;
}

/* Constructs the DrawFreeStrategy (table lbl_80594318). */
void** fn_800BE118(void** self) {
    fn_800C5F74(self);
    self[0] = (void*)lbl_80594318;
    return self;
}

/* --------------------------------------------------------------------------------------------------
 * The out-of-line GX FIFO writers, in address order.
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
 * vertices through `subVec3`/`addVec3` and emit them to the pipe, tagging every other vertex
 * with its index when the draw record asks for it.
 * -------------------------------------------------------------------------------------------------- */

/* SDK GX entry points, declared locally. */
extern void GXBegin(u8 prim, u8 vtxfmt, u16 nverts);
extern void subVec3(Vec* out, void* mtx, Vec* in);
extern void addVec3(Vec* out, void* mtx, Vec* in);
extern void fn_800514FC(Vec* out, void* mtx, Vec* in);

/* The stripe writer (first copy): four vertices from the matrix and two positions. */
void fn_800BA734(void* unused, void* mtx, Vec* a, Vec* b, u32 flags) {
    Vec v0;
    Vec v1;
    Vec v2;
    Vec v3;

    GXBegin(0x80, 0, 4);
    subVec3(&v0, mtx, a);
    fn_800BA710(&v0);
    if (fn_800BA6FC(flags) != 0) {
        fn_800BA6EC(0);
    }
    subVec3(&v1, mtx, b);
    fn_800BA710(&v1);
    if (fn_800BA6FC(flags) != 0) {
        fn_800BA6EC(1);
    }
    addVec3(&v2, mtx, a);
    fn_800BA710(&v2);
    if (fn_800BA6FC(flags) != 0) {
        fn_800BA6EC(2);
    }
    addVec3(&v3, mtx, b);
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
    subVec3(&v0, mtx, a);
    fn_800BC070(&v0);
    if (fn_800BC05C(flags) != 0) {
        fn_800BC04C(0);
    }
    subVec3(&v1, mtx, b);
    fn_800BC070(&v1);
    if (fn_800BC05C(flags) != 0) {
        fn_800BC04C(1);
    }
    addVec3(&v2, mtx, a);
    fn_800BC070(&v2);
    if (fn_800BC05C(flags) != 0) {
        fn_800BC04C(2);
    }
    addVec3(&v3, mtx, b);
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
    EfWalkerObj* particle = (EfWalkerObj*)fn_800B5ACC(args->particle, em);

    if (particle != 0) {
        PSVECSubtract(out, &particle->world_pos, &em->world_pos);
    } else {
        PSVECSubtract(out, &em->world_pos, &args->pos);
    }
    if (fn_800B59E4(out) == 0) {
        copyVec3((nw4r::math::VEC3*)out, &args->prev_pos);
    }
}

/* Resolves the ahead-context position of one particle from its two neighbour particles. */
void fn_800BBBBC(Vec* out, EfAheadArgs* args, EfWalkerObj* em) {
    Vec a;
    Vec b;
    Vec c;
    Vec d;
    EfWalkerObj* first = (EfWalkerObj*)fn_800B5ACC(args->particle, em);
    EfWalkerObj* second = (EfWalkerObj*)fn_800B8D48(args->particle, em);

    setVec3((nw4r::math::VEC3*)&a, lbl_80796154, lbl_80796154, lbl_80796154);
    if (first != 0) {
        PSVECSubtract(&a, &first->world_pos, &em->world_pos);
        if (fn_800B59E4(&a) == 0) {
            setVec3((nw4r::math::VEC3*)&c, lbl_80796154, lbl_80796154, lbl_80796154);
            copyVec3((nw4r::math::VEC3*)&a, (const nw4r::math::VEC3*)&c);
        }
    }
    setVec3((nw4r::math::VEC3*)&b, lbl_80796154, lbl_80796154, lbl_80796154);
    if (second != 0) {
        PSVECSubtract(&b, &second->world_pos, &em->world_pos);
        if (fn_800B59E4(&b) == 0) {
            setVec3((nw4r::math::VEC3*)&d, lbl_80796154, lbl_80796154, lbl_80796154);
            copyVec3((nw4r::math::VEC3*)&b, (const nw4r::math::VEC3*)&d);
        }
    }
    PSVECSubtract(out, &a, &b);
    if (fn_800B59E4(out) == 0) {
        copyVec3((nw4r::math::VEC3*)out, &args->prev_pos);
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
    mtx34_identity(&mtx);
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
