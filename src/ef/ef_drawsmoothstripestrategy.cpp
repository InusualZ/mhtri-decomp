/* auto/800BFFD4_fn_800BFFD4.c - nw4r::ef DrawSmoothStripeStrategy + DrawStrategyImpl, 0x800BFFD4..0x800C9540.
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with symedit: every fn_ name this file uses is a bare .text entry in config/RMHE08/symbols.txt)
 *
 * 118 functions / 0x956C bytes of the NintendoWare-for-Revolution effect library (`nw4r::ef`).  The
 * split object's own traces name the original source files - the `__FILE__` strings in the unit's
 * `.data` are `ef_drawsmoothstripestrategy.cpp` (0x805944E0, 0x805947C8, 0x80594820),
 * `ef_drawstrategyimpl.cpp` (0x80594850) and `particle.h` (0x80594A70..0x80594B9C) - so the range is
 * two original translation units kept whole by the attribution pass (`over --max-bytes with no legal
 * cut`, docs/plan.md 12 item 5).  The `.cpp` names make the original language **C++** (docs/plan.md,
 * "The language comes from the symbol"); the `Panic__Q24nw4r2dbFPCciPCce` callee says the same.
 *
 * This file is registered as `.c` (the attribution pass chose the extension without that evidence), so
 * it is written C-compatible and every definition sits inside an `extern "C"` guard: compiled as C the
 * guards vanish, compiled as C++ (the promotion the outbox requests) the names stay unmangled and
 * objdiff still pairs them by the map's `fn_XXXXXXXX` (docs/matching.md row 42).
 *
 * Layout in address order:
 *   0x800BFFD4..0x800C6158  ef_drawsmoothstripestrategy.cpp - DrawSmoothStripeStrategy (Draw,
 *                           GetGetFirstDrawParticleFunc/Next/CalcAhead, the stripe/tube builders) plus
 *                           the out-of-line copies of the `Particle`/`ParticleManager` inline helpers.
 *   0x800C6158..0x800C9540  ef_drawstrategyimpl.cpp - DrawStrategyImpl (InitTexture, the per-draw-order
 *                           particle walkers, the stripe/tube/point/line emitters) and the out-of-line
 *                           copies of the GX FIFO writers it uses.  The two files are why one object
 *                           references two `__FILE__` names: the range is two original translation units
 *                           kept whole by the attribution pass (the seam needs a re-check before the
 *                           extension is trusted - `langcheck.py` says the same).
 *
 * Codegen lever: this unit needs the **peephole pass off** (docs/matching.md row 39).  Retail keeps the
 * unfused `clrlwi`/`extsh`/`extsb` in front of every narrowing FIFO store, which `-O3`'s peephole folds
 * away; `#pragma peephole off` is what the `auto` library's other GX-writer unit (8009AA78) needed too,
 * and with it the whole out-of-line writer family matches.
 *
 * Status (measured with `tools/units/recompile.py`, one pass): 57 of the 118 symbols are byte-identical
 * (the 34 out-of-line GX FIFO writers at 0x800C6F90..0x800C7270, the six `EfDrawInfo` accessors, the
 * small predicates, the `EfVec3x2`/`EfVec3x3` builders and copies, the two walker selectors and
 * `AheadContext`'s initialiser); `fn_800C8954`/`fn_800C89D0` sit at 86.8 %/86.3 % and the rest are still
 * the original bytes.  Residual of the two near-misses: retail materialises the layer-range guard into a
 * register (`li r0,0` / `cmplwi` / `bgt` / `li r0,1` / `cmpwi r0,0` / `bne`) where our `if (!(layer <= 2))`
 * branches on the compare directly; the panic call itself, the `lbl_*` arguments and the bit extraction
 * are identical.  Both are above the 80 % bar.
 *
 * Language: the object is C++ (two `.cpp` `__FILE__` strings, `Panic__Q24nw4r2dbFPCciPCce`,
 * `operator_delete`), but the attribution pass registered it as `.c`, so this file is written
 * C-compatible with `extern "C"` guards and the promotion (extension, `-lang`, module name) is in the
 * outbox.  Until that lands the compile is C and the object cannot emit the target's `extab`/`extabindex`
 * group, which is why the unit can never be `Matching` in this shape.
 *
 * Inventory, addresses and sizes: `python tools/units/ledger.py unit auto/800BFFD4_fn_800BFFD4.c`; the
 * binary dossier (source names, panic line map, pool literals): `python tools/units/dossier.py
 * auto/800BFFD4_fn_800BFFD4`.
 */

#include "ef.h"
#include "gx.h"
#include "fn_8004CAD8/mtx.h" /* the symbols deleted above (rule 2) */

/* `nw4r::db::Panic` - the real declaration; the front-end reproduces the map's
 * `Panic__Q24nw4r2dbFPCciPCce` spelling (tools/units/mangle.py confirms it). Declaring the mangled
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

/* This unit's pooled `__FILE__`/assert strings and constants (the `.data` range 0x805944E0..0x80594BA7 and
 * the `.sdata2` range 0x807961C0..0x80796208).  They are declared, never defined here: the data pass
 * claims the ranges once the source emits them (docs/plan.md 8.4), so a definition would move the pool. */
extern char lbl_805944E0[]; /* "ef_drawsmoothstripestrategy.cpp"                          .data 0x805944E0 */
extern char lbl_80594500[]; /* "NW4R:Pointer Error\ndst(=%p) is not valid pointer."        .data 0x80594500 */
extern char lbl_80594534[]; /* "NW4R:Pointer Error\ncontext(=%p) is not valid pointer."    .data 0x80594534 */
extern char lbl_8059456C[]; /* "NW4R:Pointer Error\npp(=%p) is not valid pointer."         .data 0x8059456C */
extern char lbl_805945A0[]; /* "NW4R:Pointer Error\ntrigonometric(=%p) is not valid..."    .data 0x805945A0 */
extern char lbl_805945DC[]; /* "NW4R:Pointer Error\naheadContext(=%p) is not valid..."    .data 0x805945DC */
extern char lbl_80594618[]; /* "NW4R:Failed assertion youngest"                            .data 0x80594618 */
extern char lbl_80594638[]; /* "NW4R:Failed assertion 3 <= GetTubeDivide(ed)"              .data 0x80594638 */
extern char lbl_80594668[]; /* "NW4R:Pointer Error\npm(=%p) is not valid pointer."           .data 0x80594668 */
extern char lbl_8059469C[]; /* "NW4R:Pointer Error\npm->mResource(=%p) is not valid..."    .data 0x8059469C */
extern char lbl_805946D8[]; /* "NW4R:Pointer Error\n&ed(=%p) is not valid pointer."        .data 0x805946D8 */
extern char lbl_8059470C[]; /* "NW4R:Pointer Error\nyAxis(=%p) is not valid pointer."      .data 0x8059470C */
extern char lbl_80594740[]; /* "NW4R:Pointer Error\nparticle(=%p) is not valid pointer."   .data 0x80594740 */
extern char lbl_80594794[]; /* "NW4R:Pointer Error\npp(=%p) is not valid pointer."         .data 0x80594794 */
extern char lbl_805947C8[]; /* "ef_drawsmoothstripestrategy.cpp"                          .data 0x805947C8 */
extern char lbl_805947E8[]; /* "NW4R:Pointer Error\npp(=%p) is not valid pointer."         .data 0x805947E8 */
extern char lbl_80594820[]; /* "ef_drawsmoothstripestrategy.cpp"                          .data 0x80594820 */
extern char lbl_80594850[]; /* "ef_drawstrategyimpl.cpp"                                  .data 0x80594850 */
extern char lbl_80594868[]; /* "NW4R:Failed assertion mTexmapMap[0] == 0"                 .data 0x80594868 */
extern char lbl_80594894[]; /* "NW4R:Pointer Error\npp(=%p) is not valid pointer."         .data 0x80594894 */
extern char lbl_805948C8[]; /* "...pp->mParameter.mTexture[texIndex](=%p) is not valid..." .data 0x805948C8 */
extern char lbl_80594918[]; /* "NW4R:Failed assertion false"                              .data 0x80594918 */
extern char lbl_80594934[]; /* "NW4R:Pointer Error\npm(=%p) is not valid pointer."         .data 0x80594934 */
extern char lbl_80594968[]; /* "NW4R:Pointer Error\npm->mManagerEM(=%p) is not valid..."  .data 0x80594968 */
extern char lbl_805949A8[]; /* "NW4R:Pointer Error\npm->mResource(=%p) is not valid..."    .data 0x805949A8 */
extern char lbl_805949E4[]; /* "NW4R:Pointer Error\n&ed(=%p) is not valid pointer."        .data 0x805949E4 */
extern char lbl_80594A18[]; /* the DrawSmoothStripeStrategy vtable                        .data 0x80594A18 */
extern char lbl_80594A30[]; /* the base DrawStrategy vtable                               .data 0x80594A30 */
extern char lbl_80594A40[]; /* "NW4R:Failed assertion layer >= 0 && layer < 3"             .data 0x80594A40 */
extern char lbl_80594A70[]; /* "particle.h"                                               .data 0x80594A70 */
extern char lbl_80594A7C[]; /* "NW4R:Failed assertion layer >= 0 && layer < 3"             .data 0x80594A7C */
extern char lbl_80594AAC[]; /* "particle.h"                                               .data 0x80594AAC */
extern char lbl_80594AB8[]; /* "NW4R:Failed assertion layer >= 0 && layer < 3"             .data 0x80594AB8 */
extern char lbl_80594AE8[]; /* "particle.h"                                               .data 0x80594AE8 */
extern char lbl_80594AF4[]; /* "NW4R:Failed assertion layer >= 0 && layer < 3"             .data 0x80594AF4 */
extern char lbl_80594B24[]; /* "particle.h"                                               .data 0x80594B24 */
extern char lbl_80594B30[]; /* "NW4R:Failed assertion layer >= 0 && layer < 3"             .data 0x80594B30 */
extern char lbl_80594B60[]; /* "particle.h"                                               .data 0x80594B60 */
extern char lbl_80594B6C[]; /* "NW4R:Failed assertion layer >= 0 && layer < 3"             .data 0x80594B6C */
extern char lbl_80594B9C[]; /* "particle.h"                                               .data 0x80594B9C */

/* The pool constants this unit's code loads (values from the binary dossier). */
extern f32 lbl_807961C0; /* 0.0f                                                             .sdata2 0x807961C0 */
extern f32 lbl_807961C4; /* 1.0f                                                             .sdata2 0x807961C4 */
extern f32 lbl_807961C8; /* 0.5f                                                             .sdata2 0x807961C8 */
extern f32 lbl_807961CC; /* -1.0f                                                            .sdata2 0x807961CC */
extern f32 lbl_807961D8; /* 0.01f                                                            .sdata2 0x807961D8 */
extern f32 lbl_807961DC; /* 360.0f                                                           .sdata2 0x807961DC */
extern f32 lbl_807961E0; /* 1.0f                                                             .sdata2 0x807961E0 */
extern f32 lbl_807961E4; /* 0.0f                                                             .sdata2 0x807961E4 */
extern f32 lbl_807961E8; /* 100.0f                                                           .sdata2 0x807961E8 */
extern f32 lbl_807961EC; /* -1.0f                                                            .sdata2 0x807961EC */
extern f32 lbl_807961F0; /* 0.5f                                                             .sdata2 0x807961F0 */

/* --------------------------------------------------------------------------------------------- *
 * Small predicates and accessors, in address order.
 * --------------------------------------------------------------------------------------------- */

/* Tests one bit of a draw-order/flag word. */
int fn_800C0770(u32 value) {
    return (value & 0x8) != 0;
}

/* Nothing to do (the original's empty body). */
void fn_800C14F0(void) {}

/* Tests one bit of a draw-order/flag word. */
int fn_800C14F4(u32 value) {
    return (value & 0x10) != 0;
}

/* The draw-time view state `nw4r::ef::DrawInfo`.  The layout is the retail one: the engine's copy has
 * a second light mask at +0x68 and keeps the material/ambient colours behind pointers at +0x98/+0x9C,
 * which the public nw4r release spells as locals inside `DrawStrategyImpl::InitColor`. */
typedef struct EfDrawInfo {
    u8 pad_0x00[0x60];       /* +0x00  the view and projection matrices */
    u8 light_enable;         /* +0x60  `mLightEnable` */
    u32 light_mask;          /* +0x64  `mLightMask`, the GX_COLOR0 light bitmask */
    u32 light_mask1;         /* +0x68  the GX_COLOR1A1 light bitmask */
    u8 is_spot_light;        /* +0x6C  `mIsSpotLight` */
    u8 pad_0x6D[0x2B];       /* +0x6D  fog state */
    GXColor mat_color;       /* +0x98  the material colour handed to GXSetChanMatColor */
    GXColor amb_color;       /* +0x9C  the ambient colour handed to GXSetChanAmbColor */
} EfDrawInfo; /* size: 0xA0 (the record continues past what this unit reads) */

/* The material colour of the draw-time view state. */
GXColor* fn_800C68B8(EfDrawInfo* self) {
    return &self->amb_color;
}

/* The ambient colour of the draw-time view state. */
GXColor* fn_800C68C0(EfDrawInfo* self) {
    return &self->mat_color;
}

/* The GX_COLOR1A1 light bitmask of the draw-time view state. */
u32 fn_800C68C8(EfDrawInfo* self) {
    return self->light_mask1;
}

/* Whether the light is a spot light. */
u8 fn_800C68D0(EfDrawInfo* self) {
    return self->is_spot_light;
}

/* The GX_COLOR0 light bitmask of the draw-time view state. */
u32 fn_800C68D8(EfDrawInfo* self) {
    return self->light_mask;
}

/* Whether lighting is enabled for the draw. */
u8 fn_800C68E0(EfDrawInfo* self) {
    return self->light_enable;
}

/* --------------------------------------------------------------------------------------------- *
 * Vector writers, zeroers and copies, in address order.  The `EfVec3x2`/`EfVec3x3` records are the
 * two shapes this part of the effect pipeline passes around: a pair (or triple) of positions with a
 * trailing scalar, which the original copies as an aggregate (word-by-word, then the scalar).
 * --------------------------------------------------------------------------------------------- */

/* Writes a pair of f32 to the pipe. */
void fn_800C0E14(f32 x, f32 y) {
    GXWGFifo.f32 = x;
    GXWGFifo.f32 = y;
}

/* Tests the low bit of a status word. */
int fn_800C0E24(u32 value) {
    return (value & 1) != 0;
}

/* Writes one vector to the pipe. */
void fn_800C0E48(f32 x, f32 y, f32 z);
void fn_800C0E38(Vec* v) {
    fn_800C0E48(v->x, v->y, v->z);
}
/* Writes three f32 to the pipe. */
void fn_800C0E48(f32 x, f32 y, f32 z) {
    GXWGFifo.f32 = x;
    GXWGFifo.f32 = y;
    GXWGFifo.f32 = z;
}

/* A pair of vectors plus a trailing scalar. */
typedef struct EfVec3x2 {
    Vec a; /* +0x00 */
    Vec b; /* +0x0C */
    f32 w; /* +0x18 */
} EfVec3x2; /* size: 0x1C */

/* Three vectors plus a trailing scalar. */
typedef struct EfVec3x3 {
    Vec a; /* +0x00 */
    Vec b; /* +0x0C */
    Vec c; /* +0x18 */
    f32 w; /* +0x24 */
} EfVec3x3; /* size: 0x28 */

/* Copies a three-vector record. */
void fn_800C1508(EfVec3x3* dst, EfVec3x3* src) {
    dst->a = src->a;
    dst->b = src->b;
    dst->c = src->c;
    dst->w = src->w;
}

/* Zeroes a three-vector record and returns it. */
EfVec3x3* fn_800C16F8(EfVec3x3* self) {
    VEC3_ctor((VEC3*)&self->a);   /* the declaration takes the nw4r vector; same 3-float layout */
    VEC3_ctor((VEC3*)&self->b);
    VEC3_ctor((VEC3*)&self->c);
    return self;
}

/* Zeroes a two-vector record and returns it. */
EfVec3x2* fn_800C1B7C(EfVec3x2* self) {
    VEC3_ctor((VEC3*)&self->a);
    VEC3_ctor((VEC3*)&self->b);
    return self;
}

/* Builds a two-vector record from two source vectors and a scalar, and returns it. */
EfVec3x2* fn_800C26A4(EfVec3x2* self, Vec* a, Vec* b, f32 w) {
    fn_80051490(&self->a, a);
    fn_80051490(&self->b, b);
    self->w = w;
    return self;
}

/* Copies a two-vector record. */
void fn_800C26FC(EfVec3x2* dst, EfVec3x2* src) {
    dst->a = src->a;
    dst->b = src->b;
    dst->w = src->w;
}

/* --------------------------------------------------------------------------------------------- *
 * The ahead-context initialiser, the particle-walker selectors and the tail of the unit.
 * --------------------------------------------------------------------------------------------- */

/* An `nw4r::math::MTX34` (three rows of four floats) comes from `nw4r/math.h`. */

/* The per-draw ahead state `nw4r::ef::DrawStrategyImpl::AheadContext`: the emitter and manager
 * transforms plus the two axes the stripe/tube walkers advance along. */
typedef struct EfAheadContext {
    void* particle_manager;    /* +0x00 */
    const void* view_mtx;      /* +0x04 */
    Mtx34 emitter_mtx;         /* +0x08 */
    Mtx34 manager_mtx;         /* +0x38 */
    Mtx34 manager_mtx_inv;     /* +0x68 */
    Vec emitter_axis_y;        /* +0x98 */
    Vec emitter_center;        /* +0xA4 */
} EfAheadContext; /* size: 0xB0 (the record continues past what this unit reads) */

/* The GX texture-coordinate generator, the matrix initialiser this unit shares with its neighbours, and
 * the four particle walkers this unit defines further down. */
extern void GXSetTexCoordGen2(u32 dst_coord, u32 func, u32 src_param, u32 mtx, u32 normalize,
                              u32 pt_texmtx);
void fn_800C8A80(void);
void fn_800C8B9C(void);
void fn_800C8CB8(void);
void fn_800C8DE4(void);

/* Initialises the ahead context and returns it. */
EfAheadContext* fn_800C9434(EfAheadContext* self) {
    MTX34_ctor(&self->emitter_mtx);
    MTX34_ctor(&self->manager_mtx);
    MTX34_ctor(&self->manager_mtx_inv);
    VEC3_ctor((VEC3*)&self->emitter_axis_y);
    VEC3_ctor((VEC3*)&self->emitter_center);
    return self;
}

/* Builds a three-vector record from three source vectors and a scalar, and returns it. */
EfVec3x3* fn_800C3834(EfVec3x3* self, Vec* a, Vec* b, Vec* c, f32 w) {
    fn_80051490(&self->a, a);
    fn_80051490(&self->b, b);
    fn_80051490(&self->c, c);
    self->w = w;
    return self;
}

/* Picks the walker that visits the younger particles first. */
void (*fn_800C8A48(void* self, int draw_order))(void);
void (*fn_800C8A48(void* self, int draw_order))(void) {
    if (draw_order == 0) {
        return fn_800C8B9C;
    }
    return fn_800C8A80;
}

/* Picks the walker that visits the elder particles first. */
void (*fn_800C8A64(void* self, int draw_order))(void);
void (*fn_800C8A64(void* self, int draw_order))(void) {
    if (draw_order == 0) {
        return fn_800C8DE4;
    }
    return fn_800C8CB8;
}

/* Sets one texture coordinate generator and leaves the projective matrix at identity. */
void fn_800C8674(u32 dst_coord, u32 func, u32 src_param, u32 mtx) {
    GXSetTexCoordGen2(dst_coord, func, src_param, mtx, 0, 125);
}

/* The particle-side record these three accessors read.  Only the two texture-layer bit fields are
 * touched here: +0x94 packs a 2-bit value per texture layer at a stride of 4 bits (the second pair of
 * bits of each layer is read by `fn_800C8954`), +0x96 packs a 2-bit value per layer at a stride of 2. */
typedef struct EfParticleLayers {
    u8 pad_0x00[0x94];     /* +0x00 */
    u16 texture_wrap_bits; /* +0x94  wrap mode per texture layer (bits 0-1, 4-5, 8-9) */
    u8 texture_flag_bits;  /* +0x96  filter/reverse mode per texture layer (bits 0-1, 2-3, 4-5) */
    u8 pad_0x97[0x01];     /* +0x97 */
} EfParticleLayers; /* size: 0x98 */

/* The wrap mode of one texture layer. */
u32 fn_800C8954(EfParticleLayers* self, u32 layer);
u32 fn_800C8954(EfParticleLayers* self, u32 layer) {
    if (!(layer <= 2)) {
        nw4r::db::Panic(lbl_80594AAC, 414, lbl_80594A7C);
    }
    return ((s32)self->texture_wrap_bits >> (layer * 4 + 2)) & 3;
}

/* The second wrap mode of one texture layer. */
u32 fn_800C89D0(EfParticleLayers* self, u32 layer) {
    if (!(layer <= 2)) {
        nw4r::db::Panic(lbl_80594A70, 375, lbl_80594A40);
    }
    return ((s32)self->texture_wrap_bits >> (layer * 4)) & 3;
}

/* --------------------------------------------------------------------------------------------- *
 * The out-of-line GX FIFO writers, 0x800C6F90..0x800C7270.  Retail emits one copy per component count
 * and per store width; the empty one is the SDK's no-op `GXEnd`.
 * --------------------------------------------------------------------------------------------- */

/* Ends the current FIFO command (the SDK's `GXEnd`, which writes nothing). */
void fn_800C6F90(void) {}

/* Writes one u16 to the pipe. */
void fn_800C6F94(u16 value) {
    GXWGFifo.u16 = value;
}

/* Writes one u8 to the pipe. */
void fn_800C6FA4(u8 value) {
    GXWGFifo.u8 = value;
}

/* Writes one f32 to the pipe. */
void fn_800C6FB4(f32 value) {
    GXWGFifo.f32 = value;
}

/* Writes one s16 to the pipe. */
void fn_800C6FC0(s16 value) {
    GXWGFifo.s16 = value;
}

/* Writes one u16 to the pipe. */
void fn_800C6FD0(u16 value) {
    GXWGFifo.u16 = value;
}

/* Writes one s8 to the pipe. */
void fn_800C6FE0(s8 value) {
    GXWGFifo.s8 = value;
}

/* Writes one u8 to the pipe. */
void fn_800C6FF0(u8 value) {
    GXWGFifo.u8 = value;
}

/* Writes a pair of f32 to the pipe. */
void fn_800C7000(f32 x, f32 y) {
    GXWGFifo.f32 = x;
    GXWGFifo.f32 = y;
}

/* Writes a pair of s16 to the pipe. */
void fn_800C7010(s16 x, s16 y) {
    GXWGFifo.s16 = x;
    GXWGFifo.s16 = y;
}

/* Writes a pair of u16 to the pipe. */
void fn_800C7028(u16 x, u16 y) {
    GXWGFifo.u16 = x;
    GXWGFifo.u16 = y;
}

/* Writes a pair of s8 to the pipe. */
void fn_800C7040(s8 x, s8 y) {
    GXWGFifo.s8 = x;
    GXWGFifo.s8 = y;
}

/* Writes a pair of u8 to the pipe. */
void fn_800C7058(u8 x, u8 y) {
    GXWGFifo.u8 = x;
    GXWGFifo.u8 = y;
}

/* Writes one u16 to the pipe. */
void fn_800C7070(u16 value) {
    GXWGFifo.u16 = value;
}

/* Writes one u8 to the pipe. */
void fn_800C7080(u8 value) {
    GXWGFifo.u8 = value;
}

/* Writes four u8 to the pipe (an RGBA colour). */
void fn_800C7090(u8 r, u8 g, u8 b, u8 a) {
    GXWGFifo.u8 = r;
    GXWGFifo.u8 = g;
    GXWGFifo.u8 = b;
    GXWGFifo.u8 = a;
}

/* Writes three u8 to the pipe (an RGB colour). */
void fn_800C70B8(u8 r, u8 g, u8 b) {
    GXWGFifo.u8 = r;
    GXWGFifo.u8 = g;
    GXWGFifo.u8 = b;
}

/* Writes one u16 to the pipe. */
void fn_800C70D8(u16 value) {
    GXWGFifo.u16 = value;
}

/* Writes one u8 to the pipe. */
void fn_800C70E8(u8 value) {
    GXWGFifo.u8 = value;
}

/* Writes three f32 to the pipe. */
void fn_800C70F8(f32 x, f32 y, f32 z) {
    GXWGFifo.f32 = x;
    GXWGFifo.f32 = y;
    GXWGFifo.f32 = z;
}

/* Writes three s16 to the pipe. */
void fn_800C710C(s16 x, s16 y, s16 z) {
    GXWGFifo.s16 = x;
    GXWGFifo.s16 = y;
    GXWGFifo.s16 = z;
}

/* Writes three s8 to the pipe. */
void fn_800C712C(s8 x, s8 y, s8 z) {
    GXWGFifo.s8 = x;
    GXWGFifo.s8 = y;
    GXWGFifo.s8 = z;
}

/* Writes one u16 to the pipe. */
void fn_800C714C(u16 value) {
    GXWGFifo.u16 = value;
}

/* Writes one u8 to the pipe. */
void fn_800C715C(u8 value) {
    GXWGFifo.u8 = value;
}

/* Writes a pair of f32 to the pipe. */
void fn_800C716C(f32 x, f32 y) {
    GXWGFifo.f32 = x;
    GXWGFifo.f32 = y;
}

/* Writes a pair of s16 to the pipe. */
void fn_800C717C(s16 x, s16 y) {
    GXWGFifo.s16 = x;
    GXWGFifo.s16 = y;
}

/* Writes a pair of u16 to the pipe. */
void fn_800C7194(u16 x, u16 y) {
    GXWGFifo.u16 = x;
    GXWGFifo.u16 = y;
}

/* Writes a pair of s8 to the pipe. */
void fn_800C71AC(s8 x, s8 y) {
    GXWGFifo.s8 = x;
    GXWGFifo.s8 = y;
}

/* Writes a pair of u8 to the pipe. */
void fn_800C71C4(u8 x, u8 y) {
    GXWGFifo.u8 = x;
    GXWGFifo.u8 = y;
}

/* Writes three f32 to the pipe. */
void fn_800C71DC(f32 x, f32 y, f32 z) {
    GXWGFifo.f32 = x;
    GXWGFifo.f32 = y;
    GXWGFifo.f32 = z;
}

/* Writes three s16 to the pipe. */
void fn_800C71F0(s16 x, s16 y, s16 z) {
    GXWGFifo.s16 = x;
    GXWGFifo.s16 = y;
    GXWGFifo.s16 = z;
}

/* Writes three u16 to the pipe. */
void fn_800C7210(u16 x, u16 y, u16 z) {
    GXWGFifo.u16 = x;
    GXWGFifo.u16 = y;
    GXWGFifo.u16 = z;
}

/* Writes three s8 to the pipe. */
void fn_800C7230(s8 x, s8 y, s8 z) {
    GXWGFifo.s8 = x;
    GXWGFifo.s8 = y;
    GXWGFifo.s8 = z;
}

/* Writes three u8 to the pipe. */
void fn_800C7250(u8 x, u8 y, u8 z) {
    GXWGFifo.u8 = x;
    GXWGFifo.u8 = y;
    GXWGFifo.u8 = z;
}

#ifdef __cplusplus
}
#endif
