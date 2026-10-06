/* ef/ef_drawstrategyimpl.h - nw4r::ef `DrawStrategyImpl`, the GX setup every concrete draw strategy derives from, its
 * per-draw `AheadContext`, and the `DrawStrategyBuilder` that hands out the seven strategy singletons; all of them are
 * `ef/ef_drawstrategyimpl.cpp`'s.  The records they read are `ef/ef_drawstrategy.h`'s. */
#ifndef MHTRI_EF_EF_DRAWSTRATEGYIMPL_H
#define MHTRI_EF_EF_DRAWSTRATEGYIMPL_H

#include "types.h"
#include "ef.h"
#include "gx.h"
#include "nw4r/math.h"
#include "ef/ef_drawstrategy.h"

#ifdef __cplusplus
namespace nw4r {
namespace ef {

/* The common part of the concrete draw strategies: the texture, TEV and channel setup of a draw and the per-particle
 * GP state cache that `SetupGP` compares against before it touches GX. */
class DrawStrategyImpl : public DrawStrategy {
public:
    /* The particle-list walkers the two selectors hand out. */
    typedef EfDrawParticle* (*GetFirstDrawParticleFunc)(EfDrawParticleManager* pm);
    typedef EfDrawParticle* (*GetNextDrawParticleFunc)(EfDrawParticleManager* pm, EfDrawParticle* p);

    struct AheadContext;
    /* The per-particle ahead-vector builder a stripe, billboard or directional strategy hands out. */
    typedef void (*CalcAheadFunc)(VEC3* out, AheadContext* ctx, EfDrawParticle* p);

    /* The texture state last loaded for one texture layer. */
    struct PrevTexture {
        PrevTexture();

        /* +0x00 */ EfTextureData* texture;
        /* +0x04 */ f32 scale_s;           /* the layer's repeat, negative when reversed */
        /* +0x08 */ f32 scale_t;
        /* +0x0C */ f32 offset_s;          /* the matching translation (0, 1 or 2) */
        /* +0x10 */ f32 offset_t;
        /* +0x14 */ s32 wrap_s;            /* GXTexWrapMode */
        /* +0x18 */ s32 wrap_t;
        /* +0x1C */ EfVec2 scale;          /* the particle's texture scale */
        /* +0x24 */ f32 rotate;            /* the particle's texture rotation */
        /* +0x28 */ EfVec2 translate;      /* the particle's texture translation */
    }; /* size: 0x30 */

    /* The per-draw transform state the stripe and billboard walkers advance along. */
    struct AheadContext {
        AheadContext(const MTX34* view_mtx, EfDrawParticleManager* pm);

        /* +0x00 */ EfDrawParticleManager* particle_manager;
        /* +0x04 */ const MTX34* view_mtx;
        /* +0x08 */ MTX34 emitter_mtx;
        /* +0x38 */ MTX34 manager_mtx;
        /* +0x68 */ MTX34 manager_mtx_inv;
        /* +0x98 */ VEC3 emitter_axis_y;   /* the emitter's Y axis in manager space, normalised */
        /* +0xA4 */ VEC3 emitter_center;   /* the emitter's origin in manager space */
        /* +0xB0 */ VEC3 manager_axis_y;   /* the manager's Y axis (draw types 5 and 7 only) */
    }; /* size: 0xBC */

    DrawStrategyImpl();
    virtual ~DrawStrategyImpl() {}
    virtual GetFirstDrawParticleFunc GetGetFirstDrawParticleFunc(int draw_order);
    virtual GetNextDrawParticleFunc GetGetNextDrawParticleFunc(int draw_order);

    void InitGraphics(EfDrawParticleManager* pm, const EfEmitterDrawSetting& setting, const EfDrawInfo& info);
    void InitTexture(const EfEmitterDrawSetting& setting);
    void InitTev(const EfEmitterDrawSetting& setting, const EfDrawInfo& info);
    void InitColor(EfDrawParticleManager* pm, const EfEmitterDrawSetting& setting, const EfDrawInfo& info);
    void SetupGP(EfDrawParticle* pp, const EfEmitterDrawSetting& setting, const EfDrawInfo& info, bool first,
                 bool xf_dirty);
    bool SetupGPAlpha(EfDrawParticle* pp, const EfEmitterDrawSetting& setting, bool force);
    bool SetupGPColor(EfDrawParticle* pp, const EfEmitterDrawSetting& setting, bool force);
    bool SetupGPTexture(EfDrawParticle* pp, const EfEmitterDrawSetting& setting, const EfDrawInfo& info,
                        bool force);

    /* +0x00    the vtable */
    /* +0x04 */ PrevTexture mPrevTexture[3];
    /* +0x94 */ GXColor mPrevTevColor[3];  /* GX_TEVREG0..2 as last loaded */
    /* +0xA0 */ GXColor mPrevTevKColor[4]; /* GX_KCOLOR0..3 as last loaded */
    /* +0xB0 */ s32 mPrevAlphaRef0;        /* the alpha-compare references as last loaded, -1 when unknown */
    /* +0xB4 */ s32 mPrevAlphaRef1;
    /* +0xB8 */ u8 mUseColor[2][2];        /* the particle colours ([layer][index]) some TEV register reads */
    /* +0xBC */ u8 mUseAlpha[2][2];        /* the same for the alphas */
    /* +0xC0 */ GXColor mColor[2][2];      /* the particle colours fetched for this particle */
    /* +0xD0 */ u8 mNumTexmap;             /* the number of texture coordinate generators */
    /* +0xD1 */ u8 pad_0xD1[0x03];
    /* +0xD4 */ s32 mTexmapMap[3];         /* the GX texmap of texture layer 0..2, or -1 */
}; /* size: 0xE0 */

/* Hands out the draw strategy of a draw type, one lazily built singleton per type. */
class DrawStrategyBuilder {
public:
    virtual DrawStrategy* Create(u32 type);
}; /* size: 0x04 */

}  // namespace ef
}  // namespace nw4r

typedef nw4r::ef::DrawStrategyImpl::AheadContext EfAheadContext;
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* The out-of-line copies of the `EfDrawInfo` accessors and of the particle's texture-layer accessors (`particle.h`),
 * the four particle-list walkers and the ahead-context member constructor. */
const GXColor* fn_800C68B8(const EfDrawInfo* self);
const GXColor* fn_800C68C0(const EfDrawInfo* self);
u32 fn_800C68C8(const EfDrawInfo* self);
bool fn_800C68D0(const EfDrawInfo* self);
u32 fn_800C68D8(const EfDrawInfo* self);
bool fn_800C68E0(const EfDrawInfo* self);
void fn_800C8674(u32 dst_coord, u32 func, u32 src_param, u32 mtx);
s32 ef_particle_tex_offset_t(EfDrawParticle* self, int layer);
s32 ef_particle_tex_offset_s(EfDrawParticle* self, int layer);
s32 ef_particle_tex_scale_t(EfDrawParticle* self, int layer);
s32 ef_particle_tex_scale_s(EfDrawParticle* self, int layer);
s32 ef_particle_wrap_t(EfDrawParticle* self, int layer);
s32 ef_particle_wrap_s(EfDrawParticle* self, int layer);
EfDrawParticle* fn_800C8A80(EfDrawParticleManager* pm);
EfDrawParticle* fn_800C8B9C(EfDrawParticleManager* pm);
EfDrawParticle* fn_800C8CB8(EfDrawParticleManager* pm, EfDrawParticle* p);
EfDrawParticle* fn_800C8DE4(EfDrawParticleManager* pm, EfDrawParticle* p);

/* The unit vectors, the zero vector and the identity matrix the unit's static initializer builds. */
extern VEC3 ef_unit_x_vec;
extern VEC3 ef_unit_y_vec;
extern VEC3 ef_unit_z_vec;
extern VEC3 ef_zero_vec;
extern MTX34 ef_identity_mtx;

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_EF_DRAWSTRATEGYIMPL_H */
