/* ef/ef_drawstrategyimpl.cpp - the nw4r::ef DrawStrategyImpl translation unit,
 * .text 0x800C5DB8..0x800C9540 (69 functions / 0x3788 bytes).
 *
 * rule 7 deferred: the symbol map has only fn_XXXXXXXX for this range (checked with
 * tools/units/dossier.py proposal/800C5DB8_fn_800C5DB8: every .text row of the split object is a bare
 * fn_ name).
 *
 * The original source file is named by evidence class 1: the unit's own `.data` pool carries the bare
 * `__FILE__` string "ef_drawstrategyimpl.cpp" (`lbl_80594850`, confirmed by the binary dossier), which
 * is the file argument of every nw4r::db::Panic call below. The `.cpp` suffix makes the language C++
 * (langcheck.py agrees: the `Panic__Q24nw4r2dbFPCciPCce` callee and the `__dl__FPv` delete are C++),
 * so the unit is registered at `ef/ef_drawstrategyimpl.cpp`.
 *
 * Layout in address order:
 *   0x800C5DB8..0x800C6158  the lazy singletons, the three-layer texture-set constructor and its layers.
 *   0x800C6158..0x800C9540  DrawStrategyImpl's per-draw material/walker setup, the layer accessors and
 *                           the out-of-line copies of the GX FIFO writers they use.
 *
 * Status (measured with `tools/units/recompile.py ef/ef_drawstrategyimpl --measure <symbol> --main
 * <worktree>`, the objdiff report metric): 62 of the 69 symbols are reconstructed and every one is at or
 * above the 80 % bar, 52 of them byte-identical (unit 28.34 %, 2416/14216 code bytes).  The 7 symbols
 * still missing are the range's large bodies - fn_800C5DB8, fn_800C6158, fn_800C64E4, fn_800C68E8,
 * fn_800C73F8, fn_800C7CE0, fn_800C8F10 (10028 bytes).  Six reconstructed accessors sit at 86-90 %: the
 * retail `layer >= 0 && layer < 3` assert materialises into a register while every source shape tried
 * here branches on the compare directly (residual recorded).
 *
 * Codegen lever: this unit needs the **peephole pass off** (docs/matching.md row 39). Retail keeps the
 * unfused `clrlwi`/`extsh`/`extsb` in front of every narrowing GX FIFO store, which `-O3`'s peephole
 * folds away; `#pragma peephole off` is what the sibling `ef/ef_drawsmoothstripestrategy.cpp` needed
 * for the same out-of-line writer family.
 */

#include "ef.h"
#include "ef/ef_drawstrategyimpl.h"
#include "ef/fn_800AEE48.h"
#include "gx.h"
#include "unsplit/ef.h"
#include "unsplit/g3d.h"
#include "g3d/fn_80075DCC.h" /* fn_80077DF0, owned by g3d/fn_80075DCC.cpp (rule 2) */

/* `nw4r::db::Panic` - the real declaration; the front end reproduces the map's
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

/* This unit's pooled `__FILE__`/assert strings and the `particle.h` assert pair (the `.data` range
 * 0x80594850..0x80594BA7).  They are declared, never defined here: the data pass claims the ranges
 * once the source emits them (docs/plan.md 8.4), so a definition would move the pool. */
extern char lbl_80594850[]; /* "ef_drawstrategyimpl.cpp"                                    .data 0x80594850 */
extern char lbl_80594868[]; /* "NW4R:Failed assertion mTexmapMap[0] == 0"                  .data 0x80594868 */
extern char lbl_80594894[]; /* "NW4R:Pointer Error\npp(=%p) is not valid pointer."         .data 0x80594894 */
extern char lbl_805948C8[]; /* "...pp->mParameter.mTexture[texIndex](=%p) is not valid..." .data 0x805948C8 */
extern char lbl_80594918[]; /* "NW4R:Failed assertion false"                              .data 0x80594918 */
extern char lbl_80594934[]; /* "NW4R:Pointer Error\npm(=%p) is not valid pointer."         .data 0x80594934 */
extern char lbl_80594968[]; /* "NW4R:Pointer Error\npm->mManagerEM(=%p) is not valid..."  .data 0x80594968 */
extern char lbl_805949A8[]; /* "NW4R:Pointer Error\npm->mResource(=%p) is not valid..."    .data 0x805949A8 */
extern char lbl_805949E4[]; /* "NW4R:Pointer Error\n&ed(=%p) is not valid pointer."        .data 0x805949E4 */
extern char lbl_80594A18[]; /* the three-layer texture set's vtable (fn_800C8A48/8A64)     .data 0x80594A18 */
extern char lbl_80594A30[]; /* the base texture-set vtable                                 .data 0x80594A30 */
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
extern f32 lbl_807961E0; /* 1.0f                                                             .sdata2 0x807961E0 */
extern f32 lbl_807961E4; /* 0.0f                                                             .sdata2 0x807961E4 */
extern f32 lbl_807961E8; /* 100.0f                                                           .sdata2 0x807961E8 */
extern f32 lbl_807961EC; /* -1.0f                                                            .sdata2 0x807961EC */
extern f32 lbl_807961F0; /* 0.5f                                                             .sdata2 0x807961F0 */
extern f32 lbl_807961F8; /* 4.5036e+15 (0x4330000000000000)                                  .sdata2 0x807961F8 */
extern f32 lbl_80796200; /* 4.5036e+15 (0x4330000080000000)                                  .sdata2 0x80796200 */

/* The GX texture-coordinate generator (an SDK symbol) and the shared matrix initialiser. */
extern void GXSetTexCoordGen2(u32 dst_coord, u32 func, u32 src_param, u32 mtx, u32 normalize,
                              u32 pt_texmtx);
extern void fn_8005050C(Mtx34* mtx);

/* --------------------------------------------------------------------------------------------- *
 * The three-layer texture set's constructor, 0x800C5F74..0x800C6054.
 * --------------------------------------------------------------------------------------------- */

EfParticleLayers* fn_800C5F74(EfParticleLayers* self);
EfTextureLayer* fn_800C5FDC(EfTextureLayer* self);
void fn_800C6054(EfParticleLayers* self);

/* Builds the three-layer texture set, installs its vtable and returns it. */
EfParticleLayers* fn_800C5F74(EfParticleLayers* self) {
    EfTextureLayer* p;
    EfTextureLayer* end;

    fn_800C6054(self);
    self->vtable = (u32)lbl_80594A18;
    p = &self->layers[0];
    end = &self->layers[3];
    do {
        fn_800C5FDC(p);
        p++;
    } while (p < end);
    return self;
}

/* Builds one texture layer. */
EfTextureLayer* fn_800C5FDC(EfTextureLayer* self) {
    f32 one;
    f32 zero;

    fn_800834F0(&self->field_0x1C);
    fn_800834F0(&self->field_0x28);
    self->field_0x00 = 0;
    one = lbl_807961E0;
    self->field_0x04 = one;
    self->field_0x08 = one;
    self->field_0x0C = one;
    self->field_0x10 = one;
    self->field_0x14 = 0;
    self->field_0x18 = 0;
    self->field_0x1C = one;
    self->field_0x20 = one;
    zero = lbl_807961E4;
    self->field_0x24 = zero;
    self->field_0x28 = zero;
    self->field_0x2C = zero;
    return self;
}

/* Installs the base vtable (the derived constructor overwrites it). */
void fn_800C6054(EfParticleLayers* self) {
    self->vtable = (u32)lbl_80594A30;
}

/* The GX calls and the shared fog helper the per-draw setup below uses. */
extern void GXSetNumTexGens(u8 count);
extern void GXSetAlphaCompare(u32 ref0, u32 func, u32 ref1, u32 op, u32 ref2);
extern void fn_8004C4F0(void* dst, const void* src);

void fn_800C6064(EfDrawStrategyImpl* self, u32 a, u16* params, void* state);
void fn_800C60DC(EfDrawStrategyImpl* self, u16* params);
void fn_800C6158(EfDrawStrategyImpl* self, void* params, void* state);
void fn_800C64E4(EfDrawStrategyImpl* self, u32 a, void* params, void* state);

/* Per-draw texture-coordinate setup: picks the GX tex-coord generators from the material flags. */
void fn_800C60DC(EfDrawStrategyImpl* self, u16* params) {
    self->tex_coord_count = 0;
    self->tex_coord_0 = -1;
    self->tex_coord_1 = -1;
    self->tex_coord_2 = -1;
    if ((*params & 0x10) != 0) {
        self->tex_coord_0 = 0;
        self->tex_coord_count = 1;
    }
    if ((*params & 0x20) != 0) {
        self->tex_coord_1 = self->tex_coord_count;
        self->tex_coord_count = self->tex_coord_count + 1;
    }
    if ((*params & 0x40) != 0) {
        self->tex_coord_2 = self->tex_coord_count;
        self->tex_coord_count = self->tex_coord_count + 1;
    }
    GXSetNumTexGens(self->tex_coord_count);
}

/* Sets one draw strategy up from a material: texture coords, then the layer state and figures. */
void fn_800C6064(EfDrawStrategyImpl* self, u32 a, u16* params, void* state) {
    fn_800C60DC(self, params);
    fn_800C6158(self, params, state);
    fn_800C64E4(self, a, params, state);
}

/* Reads the fog record out of the material for GXSetFog. */
void fn_800C64AC(EfDrawStrategyImpl* self, s32* color, f32* near, f32* far, f32* density, f32* out_scale,
                 void* fog_out) {
    *color = self->fog_color;
    *near = self->fog_near;
    *far = self->fog_far;
    *density = self->fog_density;
    *out_scale = self->fog_scale;
    fn_8004C4F0(fog_out, &self->pad_0x84);
}

/* --------------------------------------------------------------------------------------------- *
 * The draw-time view state's six accessors, 0x800C68B8..0x800C68E0 (`EfDrawInfo` is defined once in
 * ef/ef_drawstrategyimpl.h).
 * --------------------------------------------------------------------------------------------- */

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
 * The ahead-context initialiser, the particle-walker selectors and the texture-coord wrapper,
 * in address order.
 * --------------------------------------------------------------------------------------------- */

/* Writes one texture-coordinate generator and leaves the projective matrix at identity. */
void fn_800C8674(u32 dst_coord, u32 func, u32 src_param, u32 mtx) {
    GXSetTexCoordGen2(dst_coord, func, src_param, mtx, 0, 125);
}

/* The texture-layer accessors, in address order (`EfParticleLayers` is defined once in
 * ef/ef_drawstrategyimpl.h). */
u32 fn_800C8954(EfParticleLayers* self, u32 layer);
u32 fn_800C89D0(EfParticleLayers* self, u32 layer);

/* Whether the texture wraps for a layer, combined with the filter mode of that layer. */
u32 fn_800C8680(EfParticleLayers* self, u32 layer) {
    int ok;
    u32 flags;
    u32 value;

    ok = (layer < 3);
    if (!ok) {
        nw4r::db::Panic(lbl_80594B9C, 541, lbl_80594B6C);
    }
    value = 0;
    flags = ((s32)self->texture_flag_bits >> (layer * 2)) & 3;
    if ((u8)(flags - 2) <= 1) {
        value = 1;
    }
    if (fn_800C8954(self, layer) == 2) {
        value <<= 1;
    }
    return value;
}

/* Whether the texture filter is 1 or 3 for a layer, combined with the wrap mode of that layer. */
u32 fn_800C8734(EfParticleLayers* self, u32 layer) {
    int ok;
    u32 flags;
    u32 value;

    ok = (layer < 3);
    if (!ok) {
        nw4r::db::Panic(lbl_80594B60, 512, lbl_80594B30);
    }
    value = 0;
    flags = ((s32)self->texture_flag_bits >> (layer * 2)) & 3;
    if (flags == 1 || flags == 3) {
        value = 1;
    }
    if (fn_800C89D0(self, layer) == 2) {
        value <<= 1;
    }
    return value;
}

/* The signed layer scale (wrap mode first) used by the stripe walker. */
s32 fn_800C87E8(EfParticleLayers* self, u32 layer) {
    int ok;
    u32 flags;
    s32 value;

    ok = (layer < 3);
    if (!ok) {
        nw4r::db::Panic(lbl_80594B24, 483, lbl_80594AF4);
    }
    value = 1;
    if (fn_800C8954(self, layer) == 2) {
        value = 2;
    }
    flags = ((s32)self->texture_flag_bits >> (layer * 2)) & 3;
    if ((u8)(flags - 2) <= 1) {
        value = -value;
    }
    return value;
}

/* The signed layer scale (filter mode first) used by the stripe walker. */
s32 fn_800C889C(EfParticleLayers* self, u32 layer) {
    int ok;
    u32 flags;
    s32 value;

    ok = (layer < 3);
    if (!ok) {
        nw4r::db::Panic(lbl_80594AE8, 454, lbl_80594AB8);
    }
    value = 1;
    if (fn_800C89D0(self, layer) == 2) {
        value = 2;
    }
    flags = ((s32)self->texture_flag_bits >> (layer * 2)) & 3;
    if (flags == 1 || flags == 3) {
        value -= value << 1;
    }
    return value;
}

/* The wrap mode of one texture layer. */
u32 fn_800C8954(EfParticleLayers* self, u32 layer) {
    int ok;

    ok = (layer < 3);
    if (!ok) {
        nw4r::db::Panic(lbl_80594AAC, 414, lbl_80594A7C);
    }
    return ((s32)self->texture_wrap_bits >> (layer * 4 + 2)) & 3;
}

/* The second wrap mode of one texture layer. */
u32 fn_800C89D0(EfParticleLayers* self, u32 layer) {
    int ok;

    ok = (layer < 3);
    if (!ok) {
        nw4r::db::Panic(lbl_80594A70, 375, lbl_80594A40);
    }
    return ((s32)self->texture_wrap_bits >> (layer * 4)) & 3;
}

/* The four particle walkers this unit defines further down, and the single-particle walkers they
 * hand the manager to (owned by the sibling ef unit; the map leaves them `fn_*`). */
void fn_800C8A80(void* pm);
void fn_800C8B9C(void* pm);
void fn_800C8CB8(void* pm, void* em);
void fn_800C8DE4(void* pm, void* em);

/* Picks the walker that visits the younger particles first. */
void (*fn_800C8A48(void* self, int draw_order))(void*);
void (*fn_800C8A48(void* self, int draw_order))(void*) {
    if (draw_order == 0) {
        return fn_800C8B9C;
    }
    return fn_800C8A80;
}

/* Picks the walker that visits the elder particles first. */
void (*fn_800C8A64(void* self, int draw_order))(void*, void*);
void (*fn_800C8A64(void* self, int draw_order))(void*, void*) {
    if (draw_order == 0) {
        return fn_800C8DE4;
    }
    return fn_800C8CB8;
}

/* Walks the particle manager's youngest-first list and hands each particle to the emitter. */
void fn_800C8A80(void* pm) {
    if (!IsValidPointer((u32)pm)) {
        nw4r::db::Panic(lbl_80594850, 1190, lbl_80594934, pm);
    }
    fn_800B95C0((EfParticleState*)pm);
}

/* Walks the particle manager's youngest-first list the other way. */
void fn_800C8B9C(void* pm) {
    if (!IsValidPointer((u32)pm)) {
        nw4r::db::Panic(lbl_80594850, 1197, lbl_80594934, pm);
    }
    fn_800B5A64((EfDrawList*)pm);
}

/* Walks the particle manager's eldest-first list and hands each particle to the emitter. */
void fn_800C8CB8(void* pm, void* em) {
    if (!IsValidPointer((u32)pm)) {
        nw4r::db::Panic(lbl_80594850, 1204, lbl_80594934, pm);
    }
    fn_800B8D48(pm, em);
}

/* Walks the particle manager's eldest-first list the other way. */
void fn_800C8DE4(void* pm, void* em) {
    if (!IsValidPointer((u32)pm)) {
        nw4r::db::Panic(lbl_80594850, 1211, lbl_80594934, pm);
    }
    fn_800B5ACC(pm, em);
}

/* The per-draw ahead-context initialiser (`EfAheadContext` is defined once in
 * ef/ef_drawstrategyimpl.h). */

/* Initialises the ahead context and returns it. */
EfAheadContext* fn_800C9434(EfAheadContext* self) {
    fn_8005050C(&self->emitter_mtx);
    fn_8005050C(&self->manager_mtx);
    fn_8005050C(&self->manager_mtx_inv);
    fn_80043EA8((VEC3*)&self->emitter_axis_y);
    fn_80043EA8((VEC3*)&self->emitter_center);
    return self;
}

/* --------------------------------------------------------------------------------------------- *
 * The out-of-line GX FIFO writers, 0x800C6F90..0x800C7250.  Retail emits one copy per component count
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

/* The shared draw-strategy singletons this unit's setup writes (owned by the effect state; the map
 * leaves them `lbl_`).  Declared, never defined here. */
extern Vec lbl_80694C08;  /* .bss 0x80694C08 - the four basis vectors */
extern Vec lbl_80694C14;  /* .bss 0x80694C14 */
extern Vec lbl_80694C20;  /* .bss 0x80694C20 */
extern Vec lbl_80694C2C;  /* .bss 0x80694C2C */
extern Mtx34 lbl_80694C38; /* .bss 0x80694C38 - the identity matrix */

/* Sets the alpha compare from a particle's alpha thresholds, skipping the GX call when nothing moved. */
u32 fn_800C7270(EfDrawStrategyImpl* self, EfParticleLayers* particle, u8* params, u32 force) {
    if (!IsValidPointer((u32)particle)) {
        nw4r::db::Panic(lbl_80594850, 591, lbl_80594894, particle);
    }
    if (force == 0 && self->alpha_ref == particle->alpha_threshold) {
        return 0;
    }
    self->alpha_ref = particle->alpha_threshold;
    self->alpha_ref2 = particle->alpha_threshold;
    GXSetAlphaCompare(params[2], particle->alpha_threshold, params[4], params[3],
                      particle->alpha_threshold2);
    return 1;
}

/* Initialises the four basis vectors and the identity matrix the draw strategy starts from. */
void fn_800C9488(void) {
    f32 m20;
    f32 m21;
    f32 m22;
    f32 m23;

    fn_80041E8C(&lbl_80694C08, lbl_807961E0, lbl_807961E4, lbl_807961E4);
    fn_80041E8C(&lbl_80694C14, lbl_807961E4, lbl_807961E0, lbl_807961E4);
    fn_80041E8C(&lbl_80694C20, lbl_807961E4, lbl_807961E4, lbl_807961E0);
    fn_80041E8C(&lbl_80694C2C, lbl_807961E4, lbl_807961E4, lbl_807961E4);
    m20 = lbl_807961E4;
    m21 = lbl_807961E4;
    m22 = lbl_807961E0;
    m23 = lbl_807961E4;
    fn_80077DF0(&lbl_80694C38, lbl_807961E0, lbl_807961E4, lbl_807961E4, lbl_807961E4, lbl_807961E4,
                lbl_807961E0, lbl_807961E4, lbl_807961E4, m20, m21, m22, m23);
}

#ifdef __cplusplus
}
#endif
