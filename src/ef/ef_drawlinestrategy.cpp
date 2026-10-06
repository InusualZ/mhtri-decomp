/*
 * ef/ef_drawlinestrategy.cpp - nw4r::ef DrawLineStrategy: the class's `Draw` (projects the particle space, walks
 *   the draw-order particle list through the table's +0x10/+0x14 first/next selectors and emits one GX line per
 *   eligible particle, width `(u8)(6.0f * min(t, 42.5f))`), its per-draw setup, the GX helpers, a deleting
 *   destructor (base `fn_800B4B04`) and the constructor that installs the DrawPointStrategy table `lbl_805944C8`.
 * RANGE. .text 0x800BEF98-0x800BF818 (10 functions); extab 0x8000A344-0x8000A36C, extabindex 0x8002388C-0x800238C8,
 *   .data 0x80594330-0x80594408 (the `__FILE__` string "ef_drawlinestrategy.cpp" first), .sdata
 *   0x80791348-0x80791350, .sdata2 0x807961A8-0x807961B8.
 *   Unproven seam (playbook 80: a TU's tables sit late in its `.data`): this unit's table `lbl_805943F0` is installed
 *   by `fn_800BEF5C` at the tail of `ef/ef_drawfreestrategy.cpp`'s range, and this range ends with
 *   `fn_800BF780`/`fn_800BF7DC`, the constructor installing `ef/ef_drawpointstrategy.cpp`'s table, so each `.text`
 *   seam may be off by that constructor.
 * FLAGS. `cflags_main`.
 * NAMES. The map has only `fn_` stems for the range; the assert names (`pm`, `pm->mResource`, `&ed`) are
 *   NintendoWare's `ef_drawlinestrategy.cpp`'s.  The types are this unit's private views, so they carry
 *   unit-qualified names (`_ParticleManager` here reads +0x24, `ef/ef_line.cpp`'s a table at +0x1C).
 * RESIDUALS. 2 partial rows:
 *  - `fn_800BEF98`: the two first/next-particle virtual calls load the table through a general temp (`lwz r5,
 *    0x0(r28); lwz r12, 0x10(r5)`) where retail's native dispatch reuses r12 off r3 (`lwz r12, 0x0(r3)`); a named
 *    table local or a direct slot fetch keeps ours;
 *  - `fn_800BF58C` (ours 0x20 of 0x24): retail has a `b` to the next instruction between the three `lfs` and the
 *    three FIFO stores and loads into f1-f3 (ours f0-f2); no source shape tried (a block, `if (1)`, `do {} while
 *    (0)`, `for (;;)`, a `switch`, a local array, a forwarder) emits the branch.
 *   flipcheck: `.data`, `.sdata` and `.sdata2` claimed, not emitted; `.text` 0x87C of 0x880.
 */

#include "types.h"
#include "nw4r/math.h"
#include "gx.h"
#include "ef/fn_800AEE48.h"
#include "ef/ef_drawstrategyimpl.h"
#include "ef/ef_particle.h"
#include "ef/ef_particlemanager.h"
#include "unsplit/ef.h"
#include "unsplit/g3d.h"
#include "fn_8004CAD8/mtx.h" /* the matrix helpers (rule 2) */

/* `nw4r::db::Panic`, declared in its namespace so the front-end emits the map's mangling
 * (`Panic__Q24nw4r2dbFPCciPCce`). */
namespace nw4r {
namespace db {
void Panic(const char* file, int line, const char* fmt, ...);
}  // namespace db
}  // namespace nw4r

/* The global `operator delete`; declaring the mangled `__dl__FPv` would be rule 9's violation. */
void operator delete(void* ptr) throw();

#ifdef __cplusplus
extern "C" {
#endif

#pragma peephole off

/* This unit's `__FILE__`/assert strings and constants (its claimed `.data` and `.sdata2`), declared,
 * never defined. */
extern char lbl_80594330[];  /* "ef_drawlinestrategy.cpp"                                 .data */
extern char lbl_80594348[];  /* "NW4R:Pointer Error\npm(=%p) is not valid pointer."        .data */
extern char lbl_8059437C[];  /* "NW4R:Pointer Error\npm->mResource(=%p) is not valid..."  .data */
extern char lbl_805943B8[];  /* "NW4R:Pointer Error\n&ed(=%p) is not valid pointer."      .data */
extern char lbl_805944C8[];  /* the DrawPointStrategy table (end of ef_drawpointstrategy's .data) */
extern f32 lbl_80791348;     /* 42.5f                                                     .sdata */
extern f32 lbl_807961A8;     /* 6.0f                                                      .sdata2 */
extern f32 lbl_807961AC;     /* 0.0f                                                      .sdata2 */
extern f32 lbl_807961B0;     /* 1.0f                                                      .sdata2 */

/* The SDK GX entry points this unit calls; the target object's relocations carry the plain C names. */
extern void GXBegin(u32 primitive, u32 vtxfmt, u16 nverts);
extern void GXEnableTexOffsets(u8 coord, u8 lineOffset, u8 pointOffset);
extern void GXClearVtxDesc(void);
extern void GXSetVtxDesc(u32 attr, u32 type);
extern void GXSetVtxAttrFmt(u32 vtxfmt, u32 attr, u32 cnt, u32 type, u8 frac);
extern void GXSetCurrentMtx(u32 id);
extern void GXSetLineWidth(u8 width, u32 texOffsets);
extern void GXLoadPosMtxImm(void* mtx, u32 id);

/* The nw4r math/effect helpers retail references by their plain `fn_` names; the ef callees come from
 * the headers included above. */
extern MTX34* fn_80050508(MTX34* mtx);                      /* MTX34::Get() */
extern void fn_800513F0(VEC3* v, f32 scale);                /* VEC3::Scale / rotate helper */
extern void fn_800A7F00(void* particle, VEC3* out);         /* particle velocity/axis accessor */

/* `nw4r::math::PSVECSubtract` (the map's `PSVECSubtract`, a C symbol). */
extern void PSVECSubtract(const VEC3* a, const VEC3* b, VEC3* out);

/* forward type declarations (the prototypes below name them; the layouts follow) */
typedef struct DrawLineParticle DrawLineParticle;
typedef struct DrawLineEmitter DrawLineEmitter;
typedef struct DrawLineParticleManager DrawLineParticleManager;
typedef struct DrawLineStrategy DrawLineStrategy;
typedef struct DrawLineStrategyVtbl DrawLineStrategyVtbl;
typedef struct EmResource EmResource;

/* forward declarations of this unit's own functions (the draw path calls the helpers below it) */
void fn_800BF4C8(const VEC3* a, const VEC3* b, u32 flag);
void fn_800BF564(void);
void fn_800BF568(f32 x, f32 y);
u32 fn_800BF578(u32 value);
void fn_800BF58C(const VEC3* v);
f32* fn_800BF5B0(f32* a, f32* b);
void fn_800BF5C8(DrawLineStrategy* self, DrawLineEmitter* em, DrawLineParticleManager* pm);

/* `NW4R_POINTER_ASSERT`'s RVL address-range check (MEM1/MEM2, cached and uncached, plus locked
 * cache), shared verbatim with ef/ef_line.cpp. */
#define NW4R_VALID_PTR(p)                                                                          \
    (((u32)(p) & 0xFF000000) == 0x80000000 || ((u32)(p) & 0xFF800000) == 0x81000000 ||             \
     ((u32)(p) & 0xF8000000) == 0x90000000 || ((u32)(p) & 0xFF000000) == 0xC0000000 ||             \
     ((u32)(p) & 0xFF800000) == 0xC1000000 || ((u32)(p) & 0xF8000000) == 0xD0000000 ||             \
     ((u32)(p) & 0xFFFFC000) == 0xE0000000)

#define NW4R_POINTER_ASSERT(p, line, msg)                                                          \
    (NW4R_VALID_PTR(p) ? (void)0 : nw4r::db::Panic(lbl_80594330, line, msg, (p)))

/* -------------------------------------------------------------------------------------------------
 * types
 * ------------------------------------------------------------------------------------------------- */

/* The particle record's fields this unit reads.  Only the position is touched here (the life/scale
 * accessors are the out-of-line `fn_800AB3AC`/`fn_800AB2DC`); the record continues far past it. */
struct DrawLineParticle {
    u8 pad_0x00[0xAC]; /* +0x00 */
    /* +0xAC */ VEC3 position;
}; /* size: 0xB8 (at least; the record continues past what this unit reads) */

/* The emitter record; opaque here, its only use is as the `em` argument of the matrix helpers. */
struct DrawLineEmitter {
    u8 pad_0x00[0x04]; /* +0x00 */
}; /* size: 0x04 */

/* The draw resource `ed`; the draw path tests one bit of its leading halfword. */
struct EmResource {
    u16 flags; /* +0x00 */
}; /* size: 0x02 (at least; the record continues past what this unit reads) */

/* The particle manager `pm`; only the resource reference at +0x24 is read. */
struct DrawLineParticleManager {
    u8 pad_0x00[0x24]; /* +0x00 */
    /* +0x24 */ void* mResource;
}; /* size: 0x28 (at least; the record continues past what this unit reads) */

/* The draw-order walkers the vtable's slots +0x10/+0x14 return.  `getFirst` takes the manager only,
 * `getNext` the manager and the current particle. */
typedef DrawLineParticle* (*GetFirstFunc)(DrawLineParticleManager* pm);
typedef DrawLineParticle* (*GetNextFunc)(DrawLineParticleManager* pm, DrawLineParticle* particle);

/* The DrawLineStrategy vtable: offset-to-top, typeinfo, then the four slots.  Slots +0x10/+0x14 select
 * the first/next draw-order particle. */
struct DrawLineStrategyVtbl {
    u8 pad_0x00[0x10]; /* +0x00 */
    /* +0x10 */ void* (*getFirst)(DrawLineStrategy* self, u32 flag);
    /* +0x14 */ void* (*getNext)(DrawLineStrategy* self, u32 flag);
}; /* size: 0x18 */

/* The draw strategy object.  Only the vptr and the texture-coordinate flag are read here. */
struct DrawLineStrategy {
    /* +0x00 */ DrawLineStrategyVtbl* vtbl;
    u8 pad_0x04[0xD0 - 0x04]; /* +0x04 */
    /* +0xD0 */ u8 mTexCoordEnable;
}; /* size: 0xD1 (at least; the record continues past what this unit reads) */

/* -------------------------------------------------------------------------------------------------
 * the functions, in address order
 * ------------------------------------------------------------------------------------------------- */

/* DrawLineStrategy::Draw - walk the draw-order particle list and emit one line per eligible particle.
 * `em` is the emitter, `pm` the particle manager. */
void fn_800BEF98(DrawLineStrategy* self, DrawLineEmitter* em, DrawLineParticleManager* pm) {
    NW4R_POINTER_ASSERT(pm, 100, lbl_80594348);
    fn_800BF5C8(self, em, pm);
    NW4R_POINTER_ASSERT(pm->mResource, 104, lbl_8059437C);
    EmResource* ed = (EmResource*)fn_800AB388(pm->mResource);
    NW4R_POINTER_ASSERT(ed, 106, lbl_805943B8);

    MTX34 mtxPm;
    MTX34 mtxEm;
    u32 screenSpace = (self->mTexCoordEnable != 0);

    MTX34_ctor(&mtxPm);
    fn_800AE360(pm, &mtxPm);
    MTX34_ctor(&mtxEm);
    fn_800B7DB0(em, &mtxEm);
    fn_800710BC(&mtxEm, &mtxEm, &mtxPm);
    GXLoadPosMtxImm(fn_80050508(&mtxEm), 0);

    GetFirstFunc getFirst = (GetFirstFunc)self->vtbl->getFirst(self, ed->flags & 0x800);
    GetNextFunc getNext = (GetNextFunc)self->vtbl->getNext(self, ed->flags & 0x800);

    u32 prevWidth = 0;
    u32 first = 1;
    DrawLineParticle* particle = getFirst(pm);
    f32 widthScale = lbl_807961A8; /* 6.0f */

    for (; particle != NULL; particle = getNext(pm, particle)) {
        f32 t;
        f32 g;
        VEC3 pos;
        VEC3 dir;

        t = fn_800AB3AC((struct EfParticle*)particle);
        if (t < fn_800B5A48()) {
            continue;
        }
        g = fn_800AB2DC((struct EfParticle*)particle);
        if (g < fn_800B5A48()) {
            continue;
        }
        assignVec3((Vec*)&pos, (Vec*)&particle->position);
        VEC3_ctor(&dir);
        fn_800A7F00(particle, &dir);
        if (fn_800B59E4(&dir) == 0) {
            continue;
        }
        fn_800513F0(&dir, g);
        PSVECSubtract(&dir, &pos, &dir);
        {
            u32 width = (u32)(s32)(widthScale * *fn_800BF5B0(&lbl_80791348, &t));
            if ((u8)width == 0) {
                continue;
            }
            if ((u8)prevWidth != (u8)width) {
                prevWidth = width;
                GXSetLineWidth((u8)width, 5);
                fn_800C68E8(self, particle, ed, em, first, 1);
            } else {
                fn_800C68E8(self, particle, ed, em, first, 0);
            }
        }
        first = 0;
        fn_800BF4C8(&pos, &dir, screenSpace);
    }
}

/* Draws one GX line between two positions, with the texture coords the target uses. */
void fn_800BF4C8(const VEC3* a, const VEC3* b, u32 flag) {
    GXBegin(0xA8, 0, 2);
    fn_800BF58C(a);
    if (fn_800BF578(flag)) {
        fn_800BF568(lbl_807961AC, lbl_807961AC);
    }
    fn_800BF58C(b);
    if (fn_800BF578(flag)) {
        fn_800BF568(lbl_807961AC, lbl_807961B0);
    }
    fn_800BF564();
}

/* The SDK's no-op `GXEnd`. */
void fn_800BF564(void) {}

/* Writes a pair of f32 to the GX FIFO (a texture coordinate). */
void fn_800BF568(f32 x, f32 y) {
    GXWGFifo.f32 = x;
    GXWGFifo.f32 = y;
}

/* Tests the low bit of a flag word. */
u32 fn_800BF578(u32 value) {
    return (value & 1) != 0;
}

/* Writes a position to the GX FIFO. */
void fn_800BF58C(const VEC3* v) {
    f32 x = v->x;
    f32 y = v->y;
    f32 z = v->z;
    GXWGFifo.f32 = x;
    GXWGFifo.f32 = y;
    GXWGFifo.f32 = z;
}

/* Returns a pointer to the smaller of the two floats. */
f32* fn_800BF5B0(f32* a, f32* b) {
    return (*b < *a) ? b : a;
}

/* DrawLineStrategy per-draw setup: binds the resource and the line vertex format. */
void fn_800BF5C8(DrawLineStrategy* self, DrawLineEmitter* em, DrawLineParticleManager* pm) {
    NW4R_POINTER_ASSERT(pm, 174, lbl_80594348);
    fn_800C6064((EfDrawStrategyImpl*)self, (u32)pm, (u16*)fn_800AB388(pm->mResource), (void*)em);

    GXEnableTexOffsets(0, 1, 1);
    GXClearVtxDesc();
    GXSetVtxDesc(9, 1);
    if (self->mTexCoordEnable != 0) {
        GXSetVtxDesc(0xD, 1);
    }
    GXSetVtxAttrFmt(0, 9, 1, 4, 0);
    GXSetVtxAttrFmt(0, 0xD, 1, 4, 0);
    GXSetCurrentMtx(0);
}

/* A deleting destructor: tears down the base and, for a positive flag, frees. */
void* fn_800BF780(void* self, s16 flag) {
    if (self != NULL) {
        fn_800B4B04(self, 0);
        if (flag > 0) {
            operator delete(self);
        }
    }
    return self;
}

/* Constructs the DrawPointStrategy: chains to the base and installs lbl_805944C8. */
void* fn_800BF7DC(DrawLineStrategy* self) {
    fn_800C5F74((EfParticleLayers*)self);
    self->vtbl = (DrawLineStrategyVtbl*)lbl_805944C8;
    return self;
}

#ifdef __cplusplus
}
#endif
