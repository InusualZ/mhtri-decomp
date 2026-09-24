/* ef_drawlinestrategy.cpp - nw4r::ef DrawLineStrategy, .text 0x800BEF98..0x800BF818.
 * rule 7 deferred: the symbol map has only fn_XXXXXXXX for this range (checked with
 * `python tools/symbols/dumpmap.py lookup 0x800BEF98` - the map carries `fn_800BEF98` and the shared
 * runtime dump carries only `zz_00bef98_`, so no better name exists for the ten functions).
 *
 * The unit's own `.data` pool starts with the `__FILE__` string "ef_drawlinestrategy.cpp"
 * (`lbl_80594330`, confirmed by the shared dump's symbol map as `s_ef_drawlinestrategy.cpp_80594330`,
 * 24 bytes = the string + NUL) which is the file argument of every `nw4r::db::Panic` call below.  The
 * three panic messages beside it are "pm(=%p) is not valid pointer." (0x80594348), "pm->mResource(=%p)
 * is not valid pointer." (0x8059437C) and "&ed(=%p) is not valid pointer." (0x805943B8) - the exact
 * `NW4R_POINTER_ASSERT` names of the NintendoWare 2009-04-03 `ef_drawlinestrategy.cpp`.  The runtime
 * dump string is not a `zz_` placeholder, so it is evidence class 1: module `ef`, file
 * `ef_drawlinestrategy.cpp`, language C++ (the `.cpp` suffix; langcheck.py agrees - the file's
 * `nw4r::db::Panic` callee is the `Panic__Q24nw4r2dbFPCciPCce` spelling the C++ front end emits).
 *
 * The class: `self` carries a vptr at +0x00 (the vtable is the map's `lbl_805944C8`, 24 bytes =
 * offset-to-top, typeinfo and four slots) and the one byte `self+0xD0` the draw path reads.  The
 * constructor fn_800BF7DC chains to the base constructor fn_800C5F74 and installs the vtable; the
 * destructor fn_800BF780 chains to fn_800B4B04 (the base vtable's slot +0x08) and, for a positive
 * flag, `operator delete`s.  fn_800BEF98 is the class's Draw(): it chains / projects the particle
 * space, then walks the draw-order particle list (the vtable's +0x10/+0x14 "first/next particle"
 * selectors) and emits one GX line per eligible particle, with the line width quantised to
 * `(u8)(6.0f * min(t, 42.5f))`.
 *
 * Sections: the code unit owns `.text` plus the `extab`/`extabindex` fragments that travel with it
 * (five exception entries, one per non-leaf function) - the target object emits no `.data`/`.sdata2`
 * of its own, so the pool strings and floats below are declared, never defined.
 *
 * Shared types and externs (docs/plan.md 6.5): the ef-band callees the owning unit does not exist for
 * yet live in `include/unsplit/ef.h` (fn_800C5F74, fn_800C6064, fn_800C68E8) and
 * `include/unsplit/g3d.h` (fn_800710BC); the types below are this unit's private views (the layouts
 * differ from the same-named copies other ef units carry - `_ParticleManager` here reads +0x24 while
 * ef_line.cpp's reads a vtable at +0x1C), so they carry unit-qualified names rather than duplicating a
 * `src/` type (rule 1).
 *
 * Status (official `report generate` metric from `recompile.py ef/ef_drawlinestrategy.cpp
 * --measure <symbol>`; unit mean 99.71 % over 2176 bytes):
 *   fn_800BEF98  99.90964 %  (1328 B; 4 of 332 instruction rows differ - see the residual below)
 *   fn_800BF58C  85.55556 %  (target 36 B / ours 32 B - see the residual below)
 *   fn_800BF4C8 100.00 %   fn_800BF564 100.00 %   fn_800BF568 100.00 %   fn_800BF578 100.00 %
 *   fn_800BF5B0 100.00 %   fn_800BF5C8 100.00 %   fn_800BF780 100.00 %   fn_800BF7DC 100.00 %
 *
 * Residual fn_800BEF98 (99.91 %): the two virtual-call sequences that fetch the draw-order
 * first/next-particle walkers load the vtable base from the `this` register into `r12`
 * (`lwz r12, 0x0(r3); lwz r12, 0x10(r12)`), which is MWCC's native virtual-dispatch shape; the
 * struct-member call this C-compatible source uses emits a general temp instead
 * (`lwz r5, 0x0(r28); lwz r12, 0x10(r5)`).  Everything else - both pointer asserts' register webs,
 * the matrix chain, the loop, the width quantisation `(u8)(s32)(6.0f * min(t, 42.5f))` and the
 * `min` call - is byte-identical.  Tried and rejected: naming the vtable pointer in a local, and
 * fetching the slot through the expression directly; both keep the `r5`/`r28` shape.
 *
 * Residual fn_800BF58C (85.56 %): retail is 9 instructions - `lfs f1/f2/f3` of the three position
 * components, then a `b` to the very next instruction, then the three FIFO stores; ours is 8 with
 * `f0/f1/f2` and no `b`.  The loads, the store order and the `lis r3, 0xcc01` base all match.  The
 * `b +4` is a branch to the instruction after it, so no source-shape branch is observable; tried
 * and rejected (all below the 85.56 % recorded): the three stores straight from `v->x/y/z` (40 %),
 * a `switch` (40 %), the three stores inside `{}`, `if (1)`, `do { } while (0)`, `while (1)`,
 * `for (;;)`, an empty `if` (all 85.56 %), a local `f32 xyz[3]` (45.6 %), a `GXPosition3f32`
 * `do-while(0)` macro (40 %), and a `static inline` forwarder (85.33 %, wrong load order).
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

/* The real `nw4r::db::Panic`; the map already carries its C++ mangling
 * (`Panic__Q24nw4r2dbFPCciPCce`) and the C++ front end reproduces it (tools/units/mangle.py agrees).
 * Rule 9: declare the owner, never the mangled spelling. */
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

/* This unit's pooled `__FILE__`/assert strings and constants (`.data` 0x80594330..0x805944E0 and
 * `.sdata2` 0x807961A8..0x807961B4).  Declared, never defined: the split does not own them, and the
 * target object emits no `.data`/`.sdata2` of its own. */
extern char lbl_80594330[];  /* "ef_drawlinestrategy.cpp"                                 .data */
extern char lbl_80594348[];  /* "NW4R:Pointer Error\npm(=%p) is not valid pointer."        .data */
extern char lbl_8059437C[];  /* "NW4R:Pointer Error\npm->mResource(=%p) is not valid..."  .data */
extern char lbl_805943B8[];  /* "NW4R:Pointer Error\n&ed(=%p) is not valid pointer."      .data */
extern char lbl_805944C8[];  /* the DrawLineStrategy vtable                               .data */
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

/* The nw4r math/effect helpers the target object references as plain `fn_` names.  The ef-band
 * callees whose owner units are registered live in those units' headers (rule 2):
 * fn_800B5A48/fn_800B59E4/fn_800B4B04/fn_800B7DB0 -> `ef/fn_800AEE48.h`,
 * fn_800AB388/fn_800AB3AC/fn_800AB2DC -> `ef/ef_particle.h`, fn_800AE360 ->
 * `ef/ef_particlemanager.h`.  The still-unsplit ones live in `unsplit/ef.h` (fn_800C5F74,
 * fn_800C6064, fn_800C68E8) and `unsplit/g3d.h` (fn_800710BC). */
extern void fn_8005050C(MTX34* mtx);                        /* MTX34::MTX34() (identity) */
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

    fn_8005050C(&mtxPm);
    fn_800AE360(pm, &mtxPm);
    fn_8005050C(&mtxEm);
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
        fn_80051490((Vec*)&pos, (Vec*)&particle->position);
        fn_80043EA8(&dir);
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

/* Draw one GX line between two positions, with the texture coords the target uses. */
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

/* Writes a position to the GX FIFO.  Residual (85.56 %): see the file header - retail carries one
 * extra `b +4` between the three loads and the three stores, and numbers the loads f1/f2/f3 where
 * ours uses f0/f1/f2; the loads, the store order and the FIFO base all match. */
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

/* DrawLineStrategy per-draw setup: bind the resource and the line vertex format. */
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

/* DrawLineStrategy destructor: tear down the base and, for a positive flag, free. */
void* fn_800BF780(void* self, s16 flag) {
    if (self != NULL) {
        fn_800B4B04(self, 0);
        if (flag > 0) {
            operator delete(self);
        }
    }
    return self;
}

/* DrawLineStrategy constructor: chain to the base and install the vtable. */
void* fn_800BF7DC(DrawLineStrategy* self) {
    fn_800C5F74((EfParticleLayers*)self);
    self->vtbl = (DrawLineStrategyVtbl*)lbl_805944C8;
    return self;
}

#ifdef __cplusplus
}
#endif
