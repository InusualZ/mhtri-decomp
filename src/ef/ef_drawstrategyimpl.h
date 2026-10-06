/* ef/ef_drawstrategyimpl.h - the records `ef/ef_drawstrategyimpl.cpp` reads: `EfDrawInfo` (nw4r::ef `DrawInfo`),
 * `EfAheadContext` (`DrawStrategyImpl::AheadContext`) and `EfParticleLayers` (the two bit-packed texture-layer
 * fields).  `ef/ef_drawsmoothstripestrategy.cpp` carries private copies of the first and third. */
#ifndef MHTRI_EF_EF_DRAWSTRATEGYIMPL_H
#define MHTRI_EF_EF_DRAWSTRATEGYIMPL_H

#include "types.h"
#include "ef.h"
#include "gx.h"
#include "nw4r/math.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The draw-time view state `nw4r::ef::DrawInfo`.  The layout is the retail one: the engine's copy has
 * a second light mask at +0x68 and keeps the material/ambient colours behind pointers at +0x98/+0x9C,
 * which the public nw4r release spells as locals inside `DrawStrategyImpl::InitColor`. */
typedef struct EfDrawInfo {
    u8 pad_0x00[0x60]; /* +0x00  the view and projection matrices */
    u8 light_enable;   /* +0x60  `mLightEnable` */
    u32 light_mask;    /* +0x64  `mLightMask`, the GX_COLOR0 light bitmask */
    u32 light_mask1;   /* +0x68  the GX_COLOR1A1 light bitmask */
    u8 is_spot_light;  /* +0x6C  `mIsSpotLight` */
    u8 pad_0x6D[0x2B]; /* +0x6D  fog state */
    GXColor mat_color; /* +0x98  the material colour handed to GXSetChanMatColor */
    GXColor amb_color; /* +0x9C  the ambient colour handed to GXSetChanAmbColor */
} EfDrawInfo; /* size: 0xA0 (the record continues past what this unit reads) */

/* One entry of the three-layer texture set the constructor builds.  The two 8-byte sub-objects
 * (`field_0x1C`/`field_0x28`) are built by `fn_800834F0` and then overwritten with the scale pair. */
typedef struct EfTextureLayer {
    u32 field_0x00; /* +0x00  zeroed by the constructor */
    f32 field_0x04; /* +0x04  1.0f */
    f32 field_0x08; /* +0x08  1.0f */
    f32 field_0x0C; /* +0x0C  1.0f */
    f32 field_0x10; /* +0x10  1.0f */
    u32 field_0x14; /* +0x14  zeroed */
    u32 field_0x18; /* +0x18  zeroed */
    f32 field_0x1C; /* +0x1C  1.0f */
    f32 field_0x20; /* +0x20  1.0f */
    f32 field_0x24; /* +0x24  0.0f */
    f32 field_0x28; /* +0x28  0.0f */
    f32 field_0x2C; /* +0x2C  0.0f */
} EfTextureLayer; /* size: 0x30 */

/* The three-layer texture set the constructor builds (vtable + three 0x30-byte layers + the two
 * bit-packed texture-layer fields the accessors read). */
typedef struct EfParticleLayers {
    u32 vtable;               /* +0x00 */
    EfTextureLayer layers[3]; /* +0x04  stride 0x30, ends at +0x94 */
    u16 texture_wrap_bits;    /* +0x94  wrap mode per texture layer (bits 0-1, 4-5, 8-9) */
    u8 texture_flag_bits;     /* +0x96  filter/reverse mode per texture layer (bits 0-1, 2-3, 4-5) */
    u8 alpha_threshold;       /* +0x97  the particle's alpha-compare reference */
    u8 alpha_threshold2;      /* +0x98  the particle's second alpha-compare reference */
} EfParticleLayers; /* size: 0x9C (the record continues past what this unit reads) */

/* The draw-strategy object `nw4r::ef::DrawStrategyImpl`.  Only the fields these functions touch are
 * named; the record is much larger than the 0xE0 reached here.  The fog record at +0x70..+0x84 and
 * the material's alpha/tex-coord state at +0xB0/+0xD0 are read by the per-draw setup. */
typedef struct EfDrawStrategyImpl {
    u8 pad_0x00[0x70];   /* +0x00 */
    u32 fog_color;       /* +0x70  fog colour handed to GXSetFog */
    f32 fog_near;        /* +0x74 */
    f32 fog_far;         /* +0x78 */
    f32 fog_density;     /* +0x7C */
    f32 fog_scale;       /* +0x80 */
    u8 pad_0x84[0x2C];   /* +0x84 */
    u32 alpha_ref;       /* +0xB0  the last alpha-compare reference applied */
    u32 alpha_ref2;      /* +0xB4 */
    u8 pad_0xB8[0x18];   /* +0xB8 */
    u8 tex_coord_count;  /* +0xD0  the number of texture coordinate generators */
    u8 pad_0xD1[0x03];   /* +0xD1 */
    s32 tex_coord_0;     /* +0xD4  the GX tex-coord id of layer 0, or -1 */
    s32 tex_coord_1;     /* +0xD8 */
    s32 tex_coord_2;     /* +0xDC */
} EfDrawStrategyImpl; /* size: 0xE0 (lower bound, the record continues past +0xDC) */

/* The per-draw ahead state `nw4r::ef::DrawStrategyImpl::AheadContext`: the emitter and manager
 * transforms plus the two axes the stripe/tube walkers advance along. */
typedef struct EfAheadContext {
    void* particle_manager; /* +0x00 */
    const void* view_mtx;   /* +0x04 */
    Mtx34 emitter_mtx;      /* +0x08 */
    Mtx34 manager_mtx;      /* +0x38 */
    Mtx34 manager_mtx_inv;  /* +0x68 */
    Vec emitter_axis_y;     /* +0x98 */
    Vec emitter_center;     /* +0xA4 */
} EfAheadContext; /* size: 0xB0 (the record continues past what this unit reads) */


/* The texture-set constructor and the per-draw setup this unit owns; the callers (`ef/fn_800AEE48.cpp`,
 * `ef/ef_drawfreestrategy.cpp`, `ef/ef_drawlinestrategy.cpp`) cast to these types. */
EfParticleLayers* fn_800C5F74(EfParticleLayers* self);
void fn_800C6064(EfDrawStrategyImpl* self, u32 a, u16* params, void* state);
#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_EF_DRAWSTRATEGYIMPL_H */
