/* ef/ef_drawstrategy.h - the nw4r::ef draw-strategy base class `DrawStrategy` and the records every draw strategy
 * reads: the per-draw view state `EfDrawInfo`, the particle manager `EfDrawParticleManager` (the asserts' `pm`), the
 * emitter draw setting `EfEmitterDrawSetting` (the asserts' `ed`) and the particle record `EfDrawParticle` (`pp`).
 * Each record is a view: only the offsets the draw strategies read are named, and the size is the lower bound they
 * reach.  The base class's constructor is `ef/ef_drawstrategyimpl.cpp`'s; its destructor is inline (retail keeps the
 * out-of-line copy in `ef/ef_drawstripestrategy.cpp`'s range). */
#ifndef MHTRI_EF_DRAWSTRATEGY_H
#define MHTRI_EF_DRAWSTRATEGY_H

#include "types.h"
#include "ef.h"
#include "gx.h"

/* The draw-time view state `nw4r::ef::DrawInfo`: the view matrix first, the light state, the fog record and the
 * material and ambient colours. */
typedef struct EfDrawInfo {
    /* +0x00 */ MTX34 view_mtx;      /* the camera's view matrix */
    /* +0x30 */ u8 pad_0x30[0x30];
    /* +0x60 */ bool light_enable;   /* lighting on for the draw */
    /* +0x61 */ u8 pad_0x61[0x03];
    /* +0x64 */ u32 light_mask;      /* the GX_COLOR0 light bitmask */
    /* +0x68 */ u32 light_mask1;     /* the GX_COLOR1A1 light bitmask */
    /* +0x6C */ bool is_spot_light;  /* the light is a spot light */
    /* +0x6D */ u8 pad_0x6D[0x03];
    /* +0x70 */ s32 fog_type;        /* the GXSetFog type */
    /* +0x74 */ f32 fog_start_z;
    /* +0x78 */ f32 fog_end_z;
    /* +0x7C */ f32 fog_near_z;
    /* +0x80 */ f32 fog_far_z;
    /* +0x84 */ GXColor fog_color;
    /* +0x88 */ f32 depth_offset;     /* how far the view matrix is pushed toward the camera */
    /* +0x8C */ VEC3 depth_origin;    /* the world point whose view direction the offset follows */
    /* +0x98 */ GXColor mat_color;   /* the material colour handed to GXSetChanMatColor */
    /* +0x9C */ GXColor amb_color;   /* the ambient colour handed to GXSetChanAmbColor */
} EfDrawInfo; /* size: 0xA0 (lower bound) */

/* The emitter draw setting `nw4r::ef::EmitterDrawSetting`: the GX state of one emitter's particles. */
typedef struct EfEmitterDrawSetting {
    /* +0x00 */ u16 flags;                 /* bit 0 z-compare, 1 z-update, 2 z-compare before texturing, 3 clipping,
                                            * 4-6 texture layer 0-2 on (6 also the indirect stage), 7-9 the layer's
                                            * projection mapping, 11 the particle order, 12 fog */
    /* +0x02 */ u8 alpha_comp0;            /* GXSetAlphaCompare's first comparison */
    /* +0x03 */ u8 alpha_comp1;            /* its second comparison */
    /* +0x04 */ u8 alpha_op;               /* the op combining the two */
    /* +0x05 */ u8 num_tev_stages;
    /* +0x06 */ u8 pad_0x06;
    /* +0x07 */ u8 ind_target_stages;      /* bit n: TEV stage n reads the indirect stage */
    /* +0x08 */ u8 tev_texture[4];         /* per stage: 0 texture layer 0, 1 layer 1, other no texture */
    /* +0x0C */ u8 tev_color_in[4][4];     /* GXSetTevColorIn a/b/c/d per stage */
    /* +0x1C */ u8 tev_color_op[4][5];     /* GXSetTevColorOp op/bias/scale/clamp/out per stage */
    /* +0x30 */ u8 tev_alpha_in[4][4];
    /* +0x40 */ u8 tev_alpha_op[4][5];
    /* +0x54 */ u8 tev_kcolor_sel[4];
    /* +0x58 */ u8 tev_kalpha_sel[4];
    /* +0x5C */ u8 blend_type;
    /* +0x5D */ u8 blend_src;
    /* +0x5E */ u8 blend_dst;
    /* +0x5F */ u8 blend_op;
    /* +0x60 */ u8 color_ras;              /* 1: the colour channel is lit */
    /* +0x61 */ u8 color_tev[3];           /* the particle colour each TEV colour register takes (0 none, 1-6) */
    /* +0x64 */ u8 color_tev_k[4];         /* the same for the four konstant colours */
    /* +0x68 */ u8 alpha_ras;              /* 1: the alpha channel is lit */
    /* +0x69 */ u8 alpha_tev[3];
    /* +0x6C */ u8 alpha_tev_k[4];
    /* +0x70 */ u8 z_compare_func;
    /* +0x71 */ u8 alpha_flick_type;       /* nonzero: the particle's flicker scales the alpha */
    /* +0x72 */ u8 pad_0x72[0x1E];
    /* +0x90 */ f32 ind_tex_mtx[2][3];     /* the indirect texture matrix */
    /* +0xA8 */ s8 ind_tex_scale_exp;
    /* +0xA9 */ s8 scale_a;                /* per-100 scale of the particle's x */
    /* +0xAA */ s8 scale_b;                /* per-100 scale of the particle's y */
    /* +0xAB */ u8 pad_0xAB[0x02];
    /* +0xAD */ u8 type_option;            /* the shape selectors the stripe/tube dispatches read */
    /* +0xAE */ u8 type_direction;
    /* +0xAF */ u8 type_axis;
    /* +0xB0 */ u8 type_option2;           /* the stripe tube's side count; the directional quad's stretch-by-speed flag */
    /* +0xB1 */ u8 type_option3;           /* the directional quad's pivot axis (0 Y, 1 Z); the smooth stripe's curve steps */
    /* +0xB2 */ u8 stripe_connect;         /* bits 0-2: how the stripe connects the particles */
} EfEmitterDrawSetting; /* size: 0xB3 (lower bound) */

/* The texture `nw4r::ef::TextureData` a particle layer binds. */
typedef struct EfTextureData {
    /* +0x00 */ u8 pad_0x00[0x04];
    /* +0x04 */ u16 width;
    /* +0x06 */ u16 height;
    /* +0x08 */ u8 pad_0x08[0x04];
    /* +0x0C */ u8 format;                 /* GXTexFmt; 8..10 are the colour-index formats */
    /* +0x0D */ u8 tlut_format;
    /* +0x0E */ u16 tlut_entries;
    /* +0x10 */ u8 pad_0x10[0x04];
    /* +0x14 */ u8 mipmap_count;
    /* +0x15 */ u8 min_filter;
    /* +0x16 */ u8 mag_filter;
    /* +0x17 */ u8 pad_0x17;
    /* +0x18 */ f32 lod_bias;
    /* +0x1C */ void* image;
    /* +0x20 */ void* tlut;
} EfTextureData; /* size: 0x24 */

/* The 8-byte pair `VEC2_ctor` constructs (`nw4r::math::VEC2`). */
typedef EfParticleScale EfVec2;

struct EfDrawParticleManager;

/* The particle record `nw4r::ef::Particle` the walkers yield, the asserts' `pp`. */
typedef struct EfDrawParticle {
    /* +0x00 */ u8 pad_0x00[0x40];
    /* +0x40 */ VEC3 rotate;               /* the particle's rotation (radians) */
    /* +0x4C */ EfVec2 tex_scale[3];       /* per texture layer */
    /* +0x64 */ f32 tex_rotate[3];
    /* +0x70 */ EfVec2 tex_translate[3];
    /* +0x88 */ EfTextureData* texture[3];
    /* +0x94 */ u16 texture_wrap;          /* wrap S/T per layer: bits 0-1/2-3, 4-5/6-7, 8-9/10-11 */
    /* +0x96 */ u8 texture_reverse;        /* reverse S/T per layer: bits 0/1, 2/3, 4/5 */
    /* +0x97 */ u8 alpha_ref0;             /* the alpha-compare references */
    /* +0x98 */ u8 alpha_ref1;
    /* +0x99 */ u8 pad_0x99;
    /* +0x9A */ u8 rotate_offset[3];       /* added to the rotation in 2pi/256 steps */
    /* +0x9D */ u8 collision_state;        /* the post-field shape test's last answer; bit 7 set on a bounce */
    /* +0x9E */ u8 pad_0x9E[0x02];
    /* +0xA0 */ VEC3 velocity;
    /* +0xAC */ Vec world_pos;
    /* +0xB8 */ u8 pad_0xB8[0x0C];
    /* +0xC4 */ f32 step;                  /* the frame's time step the post field advances by */
    /* +0xC8 */ struct EfDrawParticleManager* manager;
    /* +0xCC */ VEC3 ahead;             /* the stripe's ahead vector at this particle */
    /* +0xD8 */ u8 pad_0xD8[0x04];
    /* +0xDC */ u16 age;                   /* the frames the particle has lived */
    /* +0xDE */ u8 pad_0xDE[0x04];
    /* +0xE2 */ u16 life;                  /* the particle's lifetime, handed to the creation queue */
    /* +0xE4 */ u8 pad_0xE4;
    /* +0xE5 */ u8 tex_flags;              /* bit 0: mirror the layers' transform */
} EfDrawParticle; /* size: 0xE6 (lower bound) */

/* The effect system the emitter's effect belongs to: +0xC064 is the "flush the GP state" flag. */
typedef struct EfDrawEffectSystem {
    /* +0x0000 */ u8 pad_0x0000[0x10];
    /* +0x0010 */ u8 creation_queue[0xC054]; /* `ef/ef_creationqueue.cpp`'s queue (its record type is the owner's) */
    /* +0xC064 */ u8 flush_gp;
} EfDrawEffectSystem; /* size: 0xC065 (lower bound) */

/* The effect an emitter belongs to. */
typedef struct EfDrawEffect {
    /* +0x00 */ u8 pad_0x00[0x20];
    /* +0x20 */ EfDrawEffectSystem* system;
} EfDrawEffect; /* size: 0x24 (lower bound) */

/* The emitter that owns a particle manager. */
typedef struct EfDrawEmitter {
    /* +0x00 */ u8 pad_0x00[0xBC];
    /* +0xBC */ EfDrawEffect* effect;
} EfDrawEmitter; /* size: 0xC0 (lower bound) */

/* The particle list a manager keeps (`nw4r::ut::List`-shaped): only its element count is read here. */
typedef struct EfDrawParticleList {
    /* +0x00 */ u8 pad_0x00[0x18];
    /* +0x18 */ u16 count;
} EfDrawParticleList; /* size: 0x1A (lower bound) */

/* The particle manager `nw4r::ef::ParticleManager`, the asserts' `pm`. */
typedef struct EfDrawParticleManager {
    /* +0x00 */ u8 pad_0x00[0x20];
    /* +0x20 */ EfDrawEmitter* emitter;    /* `mManagerEM` */
    /* +0x24 */ void* resource;            /* `mResource`, the object ef_resource_draw_setting turns into the draw setting */
    /* +0x28 */ u8 pad_0x28[0x10];
    /* +0x38 */ EfDrawParticleList particles;
    /* +0x52 */ u8 pad_0x52[0x0E];
    /* +0x60 */ VEC3 rotate_offset;        /* added to every particle's rotation */
    /* +0x6C */ u8 inherit_emitter_color;  /* nonzero: the emitter's colour multiplies the particle colour */
} EfDrawParticleManager; /* size: 0x6D (lower bound) */

/* The legacy views the point strategy still reads; they describe the same records as above. */
typedef EfDrawParticleManager EfDrawArgs;

/* The emitter-shape view of the draw setting: +0x00 the flags, +0xAD/+0xAE the shape selectors. */
typedef struct EfEmitterShape {
    /* +0x00 */ u16 flags_0x00;
    /* +0x02 */ u8 pad_0x02[0xAB];
    /* +0xAD */ u8 shape_0xAD;
    /* +0xAE */ u8 shape_0xAE;
} EfEmitterShape; /* size: 0xAF */

/* The particle record the point walker yields: its world position is at +0xAC. size: 0xB8 (lower bound) */
typedef struct EfParticleRecord {
    /* +0x00 */ u8 pad_0x00[0xAC];
    /* +0xAC */ Vec world_pos;
} EfParticleRecord; /* size: 0xB8 */

#ifdef __cplusplus
namespace nw4r {
namespace ef {

/* The draw strategy interface: one `Draw` per particle manager. */
class DrawStrategy {
public:
    DrawStrategy();
    virtual ~DrawStrategy() {}
    virtual void Draw(const EfDrawInfo& info, EfDrawParticleManager* pm) = 0;
}; /* size: 0x04 */

}  // namespace ef
}  // namespace nw4r
#endif

#endif /* MHTRI_EF_DRAWSTRATEGY_H */
