/*
 * ef/ef_drawstrategyimpl.cpp - nw4r::ef DrawStrategyImpl: the strategy builder's lazy singletons, the constructors,
 *   the per-draw texture/TEV/channel setup, the per-particle GP setup (alpha compare, TEV colours, texture objects and
 *   matrices, and a dummy primitive that flushes the GP), the out-of-line GX FIFO writers and `EfDrawInfo`/particle
 *   accessors, the particle-list walkers and their selectors, the ahead context, and the static initializer.
 * RANGE. .text 0x800C5DB8-0x800C9540 (69 functions); extab 0x8000A49C-0x8000A554, extabindex 0x80023A90-0x80023BA4,
 *   .ctors 0x8056F2E4-0x8056F2E8, .data 0x80594850-0x80594BA8 (the `__FILE__` string "ef_drawstrategyimpl.cpp"
 *   first), .bss 0x806945E8-0x80694C68, .sbss 0x80794930-0x80794938, .sdata2 0x807961E0-0x80796208.
 *   Seam: `DrawStrategyBuilder::Create` (0x800C5DB8), its seven statics (.bss 0x806945E8-0x80694C08), their guards
 *   (.sbss 0x80794930-0x80794937) and its vtable (.data 0x80594840, between this unit's and
 *   `ef/ef_drawsmoothstripestrategy.cpp`'s claims) are one TU of their own in nw4r (`ef_drawstrategybuilder.cpp`): no
 *   assert of this file's is cited before 0x800C5F74.
 * FLAGS. `cflags_main` plus `-pool off` (configure.py: each `.bss` object of `Create` and the static initializer gets
 *   its own `lis`/`addi`, playbook 43); `#pragma peephole off` before the includes (the unfused `clrlwi`/`extsh`/
 *   `extsb` of the GX FIFO writers and guard tests, playbook 39) and `#pragma fp_contract off` (the texture matrix
 *   keeps `fmuls` + `fadds`).
 * NAMES. GUESS: the members are named after NintendoWare's `DrawStrategyImpl` (`InitTexture`/`InitTev`/`InitColor`,
 *   `SetupGP`, `GetGetFirstDrawParticleFunc`) from what they do and the assert text (`mTexmapMap[0] == 0`,
 *   `pp->mParameter.mTexture[texIndex]`, `pm->mManagerEM`); `InitGraphics`, `SetupGPAlpha`/`Color`/`Texture`,
 *   `PrevTexture`, `AheadContext`'s parameters, the `ef_particle_*` texture-layer accessors and the `ef_unit_*_vec`/
 *   `ef_zero_vec`/`ef_identity_mtx` globals are descriptive.
 *   GUESS: the FIFO writers carry the SDK's GX vertex names, read off the vertex-format switch in `SetupGP`
 *   GUESS: (attribute, component count and type pick each one; the dump agrees at `GXPosition3f32`):
 *   GUESS: `GXEnd`, `GXTexCoord1x16`, `GXTexCoord1x8`, `GXTexCoord1f32`, `GXTexCoord1s16`, `GXTexCoord1u16`,
 *   GUESS: `GXTexCoord1s8`, `GXTexCoord1u8`, `GXTexCoord2f32`, `GXTexCoord2s16`, `GXTexCoord2u16`, `GXTexCoord2s8`,
 *   GUESS: `GXTexCoord2u8`, `GXColor1x16`, `GXColor1x8`, `GXColor4u8`, `GXColor3u8`, `GXNormal1x16`, `GXNormal1x8`,
 *   GUESS: `GXNormal3f32`, `GXNormal3s16`, `GXNormal3s8`, `GXPosition1x16`, `GXPosition1x8`, `GXPosition2f32`,
 *   GUESS: `GXPosition2s16`, `GXPosition2u16`, `GXPosition2s8`, `GXPosition2u8`, `GXPosition3f32`,
 *   GUESS: `GXPosition3s16`, `GXPosition3u16`, `GXPosition3s8`, `GXPosition3u8`.
 *   GUESS (from the bodies): `ef_draw_info_get_fog`, `ef_draw_info_amb_color`, `ef_draw_info_mat_color`,
 *   GUESS: `ef_draw_info_light_mask1`, `ef_draw_info_is_spot_light`, `ef_draw_info_light_mask`,
 *   GUESS: `ef_draw_info_light_enable`, `ef_set_tex_coord_gen`, `ef_pm_first_eldest`, `ef_pm_first_youngest`,
 *   GUESS: `ef_pm_next_eldest`, `ef_pm_next_youngest`, `ef_ahead_context_members_ctor`, `ef_strategy_init_basis`
 *   GUESS: (the unit's static initializer, the `.ctors` entry), and the `ef_strategy_impl_*_str` strings by their text.
 *   GUESS (from the body and its callers): `ef_particle_tex_offset_t`, `ef_particle_tex_offset_s`,
 *   GUESS: `ef_particle_tex_scale_t`, `ef_particle_tex_scale_s`, `ef_particle_wrap_t`, `ef_particle_wrap_s`.
 * RESIDUALS. 4 partial rows:
 *  - `SetupGP` (0x800C68E8): retail keeps an explicit `cmpwi 0` for the GX_NONE case of the position, normal and
 *    texcoord switches (ours folds it into the default; an explicit `default: break;` does not keep it);
 *  - `SetupGPAlpha` (0x800C7270): retail follows the reference compare with a redundant `bne`/`beq` pair (swapped
 *    compare operands, a reference local, `!(==)`, `(!=) != false` and an early return do not produce it);
 *  - `SetupGPTexture` (0x800C7CE0): the texture pointer lives in r30 (ours r23) and the loop's saved registers
 *    number one higher;
 *  - `AheadContext::AheadContext` (0x800C8F10): the two `(0, 1, 0)` temporaries sit in swapped stack slots.
 *   flipcheck: `.ctors` and `.data` claimed, not emitted (the strings are declared by their map names).
 *   `DrawStrategyImpl` (constructor): the empty body is the whole source - the compiler
 *     emits the base call and the vtable store.
 *   `DrawStrategy` (constructor): the empty body is the whole source - the compiler
 *     emits the base call and the vtable store.
 * SHAPES. `SetupGP` returns early on one `||` of the per-emitter and the system flush flags (retail's `bne`/`b`).
 * SHAPES. `IsValidPointer` field asserts pass the pointer as the fourth `Panic` argument (retail loads it into r6);
 *   `InitTev`'s `GXSetZMode` arguments are ternaries (ours evaluates them in retail's order only that way); the
 *   attenuation selector is an enum; the accessors take `int layer` (the `layer >= 0 && layer < 3` assert folds into
 *   one unsigned compare) and the reverse flags are `u8` (the `case 2/3` range check wraps through `addi 254`).
 */

#pragma peephole off

#include "ef.h"
#include "ef/ef_pointer_assert.h" /* EF_VALID_PTR_ASSERT */
#include "gx.h"
#include "ef/ef_drawstrategyimpl.h"
#include "ef/ef_drawbillboardstrategy.h" /* DrawBillboardStrategy/DrawDirectionalStrategy (rule 2) */
#include "ef/ef_drawfreestrategy.h"      /* DrawFreeStrategy (rule 2) */
#include "ef/ef_drawlinestrategy.h"      /* DrawLineStrategy (rule 2) */
#include "ef/ef_drawpointstrategy.h"     /* DrawPointStrategy (rule 2) */
#include "ef/ef_drawsmoothstripestrategy.h" /* DrawSmoothStripeStrategy (rule 2) */
#include "ef/ef_drawstripestrategy.h"    /* DrawStripeStrategy, fn_800B7F58 (rule 2) */
#include "ef/fn_800AEE48.h"              /* the stripe unit's walkers and ef_vec3_normalize (rule 2) */
#include "ef/ef_particle.h"              /* fn_800AB388 (rule 2) */
#include "ef/ef_particlemanager.h"       /* fn_800AE360 (rule 2) */
#include "RVLGX/GXAttr.h"
#include "RVLGX/GXGeometry.h"
#include "RVLGX/GXLight.h"
#include "RVLGX/GXBump.h"
#include "RVLGX/GXPixel.h"
#include "RVLGX/GXTev.h"
#include "RVLGX/GXTexture.h"
#include "RVLGX/GXTransform.h"
#include "g3d/g3d_scnroot.h"             /* VEC2_ctor (rule 2) */
#include "g3d/fn_80075DCC.h"             /* fn_80077DF0 (rule 2) */
#include "g3d/g3d_calcview.h"            /* fn_800710BC (rule 2) */
#include "g3d/g3d_state.h"               /* mtx34_inverse (rule 2) */
#include "nw4r/fn_805012C4.h"            /* mtx34_rotate_vec3 (rule 2) */
#include "fn_8004CAD8.h"                 /* MTX34_ctor, mtx34_identity, fn_80050508 (rule 2) */
#include "fn_80047398.h"                 /* color_rgba_copy (rule 2) */
#include "mh3_pad/vec3.h"                /* copyVec3 (rule 2) */
#include "ef/ef_particle_get_color.h"   /* the particle colour getters (rule 2) */
#include "ef/ef_pm_modulate_color.h"    /* the manager colour helpers (rule 2) */
#include "ef/ef_emitter_tex_flags.h"    /* the emitter accessors (rule 2) */
#include "ef/ef_draw_info_projection.h" /* ef_draw_info_projection (rule 2) */
#include "mh3_pad/vec3_assign.h"        /* vec3_assign (rule 2) */



extern "C" {

#pragma fp_contract off

/* This unit's `__FILE__`/assert strings and the `particle.h` assert pairs (its claimed `.data`),
 * declared, never defined. */
extern char ef_strategy_impl_file_str[]; /* "ef_drawstrategyimpl.cpp"                                    .data 0x80594850 */
extern char ef_strategy_impl_texmap_assert_str[]; /* "NW4R:Failed assertion mTexmapMap[0] == 0"                  .data 0x80594868 */
extern char ef_strategy_impl_pp_assert_str[]; /* "NW4R:Pointer Error\npp(=%p) is not valid pointer."         .data 0x80594894 */
extern char ef_strategy_impl_texture_assert_str[]; /* "...pp->mParameter.mTexture[texIndex](=%p) is not valid..." .data 0x805948C8 */
extern char ef_strategy_impl_false_assert_str[]; /* "NW4R:Failed assertion false"                              .data 0x80594918 */
extern char ef_strategy_impl_pm_assert_str[]; /* "NW4R:Pointer Error\npm(=%p) is not valid pointer."         .data 0x80594934 */
extern char ef_strategy_impl_manager_em_assert_str[]; /* "NW4R:Pointer Error\npm->mManagerEM(=%p) is not valid..."  .data 0x80594968 */
extern char ef_strategy_impl_resource_assert_str[]; /* "NW4R:Pointer Error\npm->mResource(=%p) is not valid..."    .data 0x805949A8 */
extern char ef_strategy_impl_ed_assert_str[]; /* "NW4R:Pointer Error\n&ed(=%p) is not valid pointer."        .data 0x805949E4 */
extern char ef_strategy_impl_layer_assert_str0[]; /* "NW4R:Failed assertion layer >= 0 && layer < 3"             .data 0x80594A40 */
extern char ef_strategy_impl_particle_h_str0[]; /* "particle.h"                                               .data 0x80594A70 */
extern char ef_strategy_impl_layer_assert_str1[]; /* "NW4R:Failed assertion layer >= 0 && layer < 3"             .data 0x80594A7C */
extern char ef_strategy_impl_particle_h_str1[]; /* "particle.h"                                               .data 0x80594AAC */
extern char ef_strategy_impl_layer_assert_str2[]; /* "NW4R:Failed assertion layer >= 0 && layer < 3"             .data 0x80594AB8 */
extern char ef_strategy_impl_particle_h_str2[]; /* "particle.h"                                               .data 0x80594AE8 */
extern char ef_strategy_impl_layer_assert_str3[]; /* "NW4R:Failed assertion layer >= 0 && layer < 3"             .data 0x80594AF4 */
extern char ef_strategy_impl_particle_h_str3[]; /* "particle.h"                                               .data 0x80594B24 */
extern char ef_strategy_impl_layer_assert_str4[]; /* "NW4R:Failed assertion layer >= 0 && layer < 3"             .data 0x80594B30 */
extern char ef_strategy_impl_particle_h_str4[]; /* "particle.h"                                               .data 0x80594B60 */
extern char ef_strategy_impl_layer_assert_str5[]; /* "NW4R:Failed assertion layer >= 0 && layer < 3"             .data 0x80594B6C */
extern char ef_strategy_impl_particle_h_str5[]; /* "particle.h"                                               .data 0x80594B9C */

EfAheadContext* ef_ahead_context_members_ctor(EfAheadContext* self);

}  // extern "C"

namespace nw4r {
namespace ef {

/* 0x800C5DB8 (0x1BC): returns the strategy singleton of a draw type, building it on first use. */
DrawStrategy* DrawStrategyBuilder::Create(u32 type) {
    static DrawBillboardStrategy billboard;
    static DrawDirectionalStrategy directional;
    static DrawFreeStrategy free;
    static DrawLineStrategy line;
    static DrawPointStrategy point;
    static DrawStripeStrategy stripe;
    static DrawSmoothStripeStrategy smooth_stripe;

    switch (type) {
    case 3:
        return &billboard;
    case 4:
        return &directional;
    case 2:
        return &free;
    case 1:
        return &line;
    case 0:
        return &point;
    case 5:
        return &stripe;
    case 6:
        return &smooth_stripe;
    }
    return &billboard;
}

}  // namespace ef
}  // namespace nw4r

/* The unit vectors, the zero vector and the identity matrix the static initializer builds. */
VEC3 ef_unit_x_vec;
VEC3 ef_unit_y_vec;
VEC3 ef_unit_z_vec;
VEC3 ef_zero_vec;
MTX34 ef_identity_mtx;

namespace nw4r {
namespace ef {

/* 0x800C5F74 (0x68): builds the strategy and its three texture-state caches. */
DrawStrategyImpl::DrawStrategyImpl() {}

/* 0x800C5FDC (0x78): one texture layer's cache, at the identity transform with no texture. */
DrawStrategyImpl::PrevTexture::PrevTexture() {
    f32 one;
    f32 zero;

    VEC2_ctor(&scale);
    VEC2_ctor(&translate);
    texture = NULL;
    one = 1.0f;
    scale_s = one;
    scale_t = one;
    offset_s = one;
    offset_t = one;
    wrap_s = 0;
    wrap_t = 0;
    scale.x = one;
    scale.y = one;
    zero = 0.0f;
    rotate = zero;
    translate.x = zero;
    translate.y = zero;
}

/* 0x800C6054 (0x10): the interface's constructor. */
DrawStrategy::DrawStrategy() {}

/* 0x800C6064 (0x78): sets the GX texture coordinates, TEV and colour channels up for one particle manager. */
void DrawStrategyImpl::InitGraphics(EfDrawParticleManager* pm, const EfEmitterDrawSetting& setting,
                                    const EfDrawInfo& info) {
    InitTexture(setting);
    InitTev(setting, info);
    InitColor(pm, setting, info);
}

/* 0x800C60DC (0x7C): gives each enabled texture layer the next texture coordinate generator. */
void DrawStrategyImpl::InitTexture(const EfEmitterDrawSetting& setting) {
    mNumTexmap = 0;
    mTexmapMap[0] = -1;
    mTexmapMap[1] = -1;
    mTexmapMap[2] = -1;
    if ((setting.flags & 0x10) != 0) {
        mTexmapMap[0] = 0;
        mNumTexmap = 1;
    }
    if ((setting.flags & 0x20) != 0) {
        mTexmapMap[1] = mNumTexmap;
        mNumTexmap++;
    }
    if ((setting.flags & 0x40) != 0) {
        mTexmapMap[2] = mNumTexmap;
        mNumTexmap++;
    }
    GXSetNumTexGens(mNumTexmap);
}

}  // namespace ef
}  // namespace nw4r

extern "C" {

/* 0x800C64AC (0x38): reads the fog record out of the view state. */
void ef_draw_info_get_fog(const EfDrawInfo* self, s32* type, f32* start_z, f32* end_z, f32* near_z, f32* far_z,
                 GXColor* color) {
    *type = self->fog_type;
    *start_z = self->fog_start_z;
    *end_z = self->fog_end_z;
    *near_z = self->fog_near_z;
    *far_z = self->fog_far_z;
    color_rgba_copy((u8*)color, (const u8*)&self->fog_color);
}

}  // extern "C"

namespace nw4r {
namespace ef {

/* 0x800C6158 (0x354): sets the TEV stages, the indirect stage, the pixel pipeline and the fog of the draw. */
void DrawStrategyImpl::InitTev(const EfEmitterDrawSetting& setting, const EfDrawInfo& info) {
    int i;

    GXSetClipMode((setting.flags & 8) != 0);
    GXSetNumTevStages(setting.num_tev_stages);
    GXSetTevSwapModeTable(0, 0, 1, 2, 3);
    for (i = 0; i < setting.num_tev_stages; i++) {
        GXSetTevDirect(i);
        GXSetTevColorIn(i, setting.tev_color_in[i][0], setting.tev_color_in[i][1], setting.tev_color_in[i][2],
                        setting.tev_color_in[i][3]);
        GXSetTevAlphaIn(i, setting.tev_alpha_in[i][0], setting.tev_alpha_in[i][1], setting.tev_alpha_in[i][2],
                        setting.tev_alpha_in[i][3]);
        GXSetTevColorOp(i, setting.tev_color_op[i][0], setting.tev_color_op[i][1], setting.tev_color_op[i][2],
                        setting.tev_color_op[i][3], setting.tev_color_op[i][4]);
        GXSetTevAlphaOp(i, setting.tev_alpha_op[i][0], setting.tev_alpha_op[i][1], setting.tev_alpha_op[i][2],
                        setting.tev_alpha_op[i][3], setting.tev_alpha_op[i][4]);
        GXSetTevKColorSel(i, setting.tev_kcolor_sel[i]);
        GXSetTevKAlphaSel(i, setting.tev_kalpha_sel[i]);
        GXSetTevSwapMode(i, 0, 0);
        if (setting.tev_texture[i] == 0) {
            if (!(mTexmapMap[0] == 0)) {
                nw4r::db::Panic(ef_strategy_impl_file_str, 134, ef_strategy_impl_texmap_assert_str);
            }
            GXSetTevOrder(i, 0, 0, 4);
        } else if (setting.tev_texture[i] == 1) {
            GXSetTevOrder(i, mTexmapMap[1], mTexmapMap[1], 4);
        } else {
            GXSetTevOrder(i, 0xFF, 0xFF, 4);
        }
    }

    if ((setting.flags & 0x40) != 0) {
        u8 stage_bit;
        int stage;

        GXSetNumIndStages(1);
        GXSetIndTexOrder(0, mTexmapMap[2], mTexmapMap[2]);
        GXSetIndTexCoordScale(0, 0, 0);
        GXSetIndTexMtx(1, setting.ind_tex_mtx, setting.ind_tex_scale_exp);
        stage_bit = 1;
        for (stage = 0; stage < setting.num_tev_stages; stage++, stage_bit <<= 1) {
            if ((setting.ind_target_stages & stage_bit) != 0) {
                GXSetTevIndirect(stage, 0, 0, 7, 1, 0, 0, 0, 0, 0);
            }
        }
    } else {
        GXSetNumIndStages(0);
    }

    GXSetZCompLoc((setting.flags & 4) != 0);
    GXSetCullMode(0);
    GXSetCoPlanar(0);
    GXSetBlendMode(setting.blend_type, setting.blend_src, setting.blend_dst, setting.blend_op);
    GXSetZMode((setting.flags & 1) ? 1 : 0, setting.z_compare_func, (setting.flags & 2) ? 1 : 0);

    if ((setting.flags & 0x1000) != 0) {
        s32 type;
        f32 start_z;
        f32 end_z;
        f32 near_z;
        f32 far_z;
        GXColor color;

        ef_draw_info_get_fog(&info, &type, &start_z, &end_z, &near_z, &far_z, &color);
        GXSetFog(type, start_z, end_z, near_z, far_z, color);
    } else {
        GXColor color = {0, 0, 0, 0};
        GXSetFog(0, 0.0f, 100.0f, 0.0f, 100.0f, color);
    }
}

/* The SDK's light attenuation selector. */
enum GXAttnFn { GX_AF_SPEC, GX_AF_SPOT, GX_AF_NONE };

/* Marks the particle colour a TEV register reads as used. */
inline void MarkColorUse(u8 source, u8* use) {
    switch (source) {
    case 1:
        use[0] = true;
        break;
    case 2:
        use[1] = true;
        break;
    case 5:
        use[0] = true;
        use[1] = true;
        break;
    case 3:
        use[2] = true;
        break;
    case 4:
        use[3] = true;
        break;
    case 6:
        use[2] = true;
        use[3] = true;
        break;
    }
}

/* 0x800C64E4 (0x3D4): sets the colour channels from the light state and records which particle colours and alphas
 * the TEV registers read. */
void DrawStrategyImpl::InitColor(EfDrawParticleManager* pm, const EfEmitterDrawSetting& setting,
                                 const EfDrawInfo& info) {
    int i;

    mPrevAlphaRef0 = -1;
    mPrevAlphaRef1 = -1;
    GXSetNumChans(1);
    if (!ef_draw_info_light_enable(&info)) {
        GXSetChanCtrl(4, 0, 0, 0, 0, 0, 2);
    } else {
        if (setting.color_ras == 1) {
            GXAttnFn attn = ef_draw_info_is_spot_light(&info) ? GX_AF_SPOT : GX_AF_NONE;
            GXSetChanCtrl(0, 1, 0, 0, ef_draw_info_light_mask(&info), 0, attn);
        } else {
            GXSetChanCtrl(0, 0, 0, 0, 0, 0, 2);
        }
        if (setting.alpha_ras == 1) {
            GXAttnFn attn = ef_draw_info_is_spot_light(&info) ? GX_AF_SPOT : GX_AF_NONE;
            GXSetChanCtrl(2, 1, 0, 0, ef_draw_info_light_mask1(&info), 0, attn);
        } else {
            GXSetChanCtrl(2, 0, 0, 0, 0, 0, 2);
        }
    }
    GXSetChanCtrl(5, 0, 0, 0, 0, 0, 2);
    GXSetChanMatColor(4, *ef_draw_info_mat_color(&info));
    GXSetChanAmbColor(4, *ef_draw_info_amb_color(&info));

    mUseColor[0][0] = false;
    mUseColor[0][1] = false;
    mUseColor[1][0] = false;
    mUseColor[1][1] = false;
    mUseAlpha[0][0] = false;
    mUseAlpha[0][1] = false;
    mUseAlpha[1][0] = false;
    mUseAlpha[1][1] = false;
    for (i = 0; i < 3; i++) {
        MarkColorUse(setting.color_tev[i], mUseColor[0]);
        MarkColorUse(setting.alpha_tev[i], mUseAlpha[0]);
    }
    for (i = 0; i < 4; i++) {
        MarkColorUse(setting.color_tev_k[i], mUseColor[0]);
        MarkColorUse(setting.alpha_tev_k[i], mUseAlpha[0]);
    }
}

}  // namespace ef
}  // namespace nw4r

extern "C" {

/* 0x800C68B8 (0x8): the view state's `amb_color` field. */
const GXColor* ef_draw_info_amb_color(const EfDrawInfo* self) {
    return &self->amb_color;
}

/* 0x800C68C0 (0x8): the view state's `mat_color` field. */
const GXColor* ef_draw_info_mat_color(const EfDrawInfo* self) {
    return &self->mat_color;
}

/* 0x800C68C8 (0x8): the GX_COLOR1A1 light bitmask of the view state. */
u32 ef_draw_info_light_mask1(const EfDrawInfo* self) {
    return self->light_mask1;
}

/* 0x800C68D0 (0x8): whether the light is a spot light. */
bool ef_draw_info_is_spot_light(const EfDrawInfo* self) {
    return self->is_spot_light;
}

/* 0x800C68D8 (0x8): the GX_COLOR0 light bitmask of the view state. */
u32 ef_draw_info_light_mask(const EfDrawInfo* self) {
    return self->light_mask;
}

/* 0x800C68E0 (0x8): whether lighting is on for the draw. */
bool ef_draw_info_light_enable(const EfDrawInfo* self) {
    return self->light_enable;
}

}  // extern "C"

/* The out-of-line GX FIFO writers, 0x800C6F90..0x800C7250: one copy per component count and store width.  The
 * dummy primitive below calls them. */
extern "C" {
void GXEnd(void);
void GXTexCoord1x16(u16 value);
void GXTexCoord1x8(u8 value);
void GXTexCoord1f32(f32 value);
void GXTexCoord1s16(s16 value);
void GXTexCoord1u16(u16 value);
void GXTexCoord1s8(s8 value);
void GXTexCoord1u8(u8 value);
void GXTexCoord2f32(f32 x, f32 y);
void GXTexCoord2s16(s16 x, s16 y);
void GXTexCoord2u16(u16 x, u16 y);
void GXTexCoord2s8(s8 x, s8 y);
void GXTexCoord2u8(u8 x, u8 y);
void GXColor1x16(u16 value);
void GXColor1x8(u8 value);
void GXColor4u8(u8 r, u8 g, u8 b, u8 a);
void GXColor3u8(u8 r, u8 g, u8 b);
void GXNormal1x16(u16 value);
void GXNormal1x8(u8 value);
void GXNormal3f32(f32 x, f32 y, f32 z);
void GXNormal3s16(s16 x, s16 y, s16 z);
void GXNormal3s8(s8 x, s8 y, s8 z);
void GXPosition1x16(u16 value);
void GXPosition1x8(u8 value);
void GXPosition2f32(f32 x, f32 y);
void GXPosition2s16(s16 x, s16 y);
void GXPosition2u16(u16 x, u16 y);
void GXPosition2s8(s8 x, s8 y);
void GXPosition2u8(u8 x, u8 y);
void GXPosition3f32(f32 x, f32 y, f32 z);
void GXPosition3s16(s16 x, s16 y, s16 z);
void GXPosition3u16(u16 x, u16 y, u16 z);
void GXPosition3s8(s8 x, s8 y, s8 z);
void GXPosition3u8(u8 x, u8 y, u8 z);
}

namespace nw4r {
namespace ef {

/* The vertex description `GXGetVtxDesc`/`GXGetVtxAttrFmt` report for one attribute. */
struct VtxAttrState {
    /* +0x00 */ s32 type;  /* GXAttrType: none, direct, 8- or 16-bit index */
    /* +0x04 */ s32 cnt;   /* GXCompCnt */
    /* +0x08 */ s32 comp;  /* GXCompType */
}; /* size: 0x0C */

/* 0x800C68E8 (0x6A8): loads the particle's alpha compare, TEV colours and textures; when GX state changed but no
 * texture was reloaded and the effect system asks for it, draws an eight-vertex dummy strip in the current vertex
 * format so the GP takes the new state. */
void DrawStrategyImpl::SetupGP(EfDrawParticle* pp, const EfEmitterDrawSetting& setting, const EfDrawInfo& info,
                               bool first, bool xf_dirty) {
    bool alpha_dirty;
    bool color_dirty;
    bool texture_loaded;
    bool flush;

    if (!IsValidPointer((u32)pp)) {
        nw4r::db::Panic(ef_strategy_impl_file_str, 368, ef_strategy_impl_pp_assert_str, pp);
    }

    alpha_dirty = SetupGPAlpha(pp, setting, first);
    alpha_dirty = xf_dirty || alpha_dirty;
    color_dirty = SetupGPColor(pp, setting, first);
    color_dirty = alpha_dirty || color_dirty;
    texture_loaded = SetupGPTexture(pp, setting, info, first);
    flush = color_dirty && !texture_loaded;

    if ((setting.flags & 8) != 0 || pp->manager->emitter->effect->system->flush_gp == 0) {
        return;
    }
    if (flush) {
        u8 frac;
        VtxAttrState pos;
        VtxAttrState nrm;
        VtxAttrState tex0;
        VtxAttrState clr[2];
        int i;
        int c;

        GXGetVtxDesc(9, &pos.type);
        GXGetVtxDesc(10, &nrm.type);
        GXGetVtxDesc(11, &clr[0].type);
        GXGetVtxDesc(12, &clr[1].type);
        GXGetVtxDesc(13, &tex0.type);
        GXGetVtxAttrFmt(0, 9, &pos.cnt, &pos.comp, &frac);
        GXGetVtxAttrFmt(0, 10, &nrm.cnt, &nrm.comp, &frac);
        GXGetVtxAttrFmt(0, 11, &clr[0].cnt, &clr[0].comp, &frac);
        GXGetVtxAttrFmt(0, 12, &clr[1].cnt, &clr[1].comp, &frac);
        GXGetVtxAttrFmt(0, 13, &tex0.cnt, &tex0.comp, &frac);
        GXBegin(0x98, 0, 8);
        for (i = 0; i < 8; i++) {
            switch (pos.type) {
            case 0:
                break;
            case 1:
                switch (pos.cnt) {
                case 1:
                    switch (pos.comp) {
                    case 0:
                        GXPosition3u8(0, 0, 0);
                        break;
                    case 1:
                        GXPosition3s8(0, 0, 0);
                        break;
                    case 2:
                        GXPosition3u16(0, 0, 0);
                        break;
                    case 3:
                        GXPosition3s16(0, 0, 0);
                        break;
                    case 4:
                        GXPosition3f32(0.0f, 0.0f, 0.0f);
                        break;
                    }
                    break;
                case 0:
                    switch (pos.comp) {
                    case 0:
                        GXPosition2u8(0, 0);
                        break;
                    case 1:
                        GXPosition2s8(0, 0);
                        break;
                    case 2:
                        GXPosition2u16(0, 0);
                        break;
                    case 3:
                        GXPosition2s16(0, 0);
                        break;
                    case 4:
                        GXPosition2f32(0.0f, 0.0f);
                        break;
                    }
                    break;
                }
                break;
            case 2:
                GXPosition1x8(0);
                break;
            case 3:
                GXPosition1x16(0);
                break;
            }

            switch (nrm.type) {
            case 0:
                break;
            case 1:
                switch (nrm.cnt) {
                case 0:
                    switch (nrm.comp) {
                    case 1:
                        GXNormal3s8(0, 0, 0);
                        break;
                    case 3:
                        GXNormal3s16(0, 0, 0);
                        break;
                    case 4:
                        GXNormal3f32(0.0f, 0.0f, 0.0f);
                        break;
                    }
                    break;
                }
                break;
            case 2:
                GXNormal1x8(0);
                break;
            case 3:
                GXNormal1x16(0);
                break;
            }

            for (c = 0; c < 2; c++) {
                switch (clr[c].type) {
                case 1:
                    switch (clr[c].cnt) {
                    case 0:
                        GXColor3u8(0, 0, 0);
                        break;
                    case 1:
                        GXColor4u8(0, 0, 0, 0);
                        break;
                    }
                    break;
                case 2:
                    GXColor1x8(0);
                    break;
                case 3:
                    GXColor1x16(0);
                    break;
                }
            }

            switch (tex0.type) {
            case 0:
                break;
            case 1:
                switch (tex0.cnt) {
                case 1:
                    switch (tex0.comp) {
                    case 0:
                        GXTexCoord2u8(0, 0);
                        break;
                    case 1:
                        GXTexCoord2s8(0, 0);
                        break;
                    case 2:
                        GXTexCoord2u16(0, 0);
                        break;
                    case 3:
                        GXTexCoord2s16(0, 0);
                        break;
                    case 4:
                        GXTexCoord2f32(0.0f, 0.0f);
                        break;
                    }
                    break;
                case 0:
                    switch (tex0.comp) {
                    case 0:
                        GXTexCoord1u8(0);
                        break;
                    case 1:
                        GXTexCoord1s8(0);
                        break;
                    case 2:
                        GXTexCoord1u16(0);
                        break;
                    case 3:
                        GXTexCoord1s16(0);
                        break;
                    case 4:
                        GXTexCoord1f32(0.0f);
                        break;
                    }
                    break;
                }
                break;
            case 2:
                GXTexCoord1x8(0);
                break;
            case 3:
                GXTexCoord1x16(0);
                break;
            }
        }
        GXEnd();
    }
}

}  // namespace ef
}  // namespace nw4r

/* --------------------------------------------------------------------------------------------- *
 * The out-of-line GX FIFO writers, 0x800C6F90..0x800C7250.  Retail emits one copy per component count
 * and per store width; the empty one is the SDK's no-op `GXEnd`.
 * --------------------------------------------------------------------------------------------- */

extern "C" {

/* Ends the current FIFO command (the SDK's `GXEnd`, which writes nothing). */
void GXEnd(void) {}

/* Writes one u16 to the pipe. */
void GXTexCoord1x16(u16 value) {
    GXWGFifo.u16 = value;
}

/* Writes one u8 to the pipe. */
void GXTexCoord1x8(u8 value) {
    GXWGFifo.u8 = value;
}

/* Writes one f32 to the pipe. */
void GXTexCoord1f32(f32 value) {
    GXWGFifo.f32 = value;
}

/* Writes one s16 to the pipe. */
void GXTexCoord1s16(s16 value) {
    GXWGFifo.s16 = value;
}

/* Writes one u16 to the pipe. */
void GXTexCoord1u16(u16 value) {
    GXWGFifo.u16 = value;
}

/* Writes one s8 to the pipe. */
void GXTexCoord1s8(s8 value) {
    GXWGFifo.s8 = value;
}

/* Writes one u8 to the pipe. */
void GXTexCoord1u8(u8 value) {
    GXWGFifo.u8 = value;
}

/* Writes a pair of f32 to the pipe. */
void GXTexCoord2f32(f32 x, f32 y) {
    GXWGFifo.f32 = x;
    GXWGFifo.f32 = y;
}

/* Writes a pair of s16 to the pipe. */
void GXTexCoord2s16(s16 x, s16 y) {
    GXWGFifo.s16 = x;
    GXWGFifo.s16 = y;
}

/* Writes a pair of u16 to the pipe. */
void GXTexCoord2u16(u16 x, u16 y) {
    GXWGFifo.u16 = x;
    GXWGFifo.u16 = y;
}

/* Writes a pair of s8 to the pipe. */
void GXTexCoord2s8(s8 x, s8 y) {
    GXWGFifo.s8 = x;
    GXWGFifo.s8 = y;
}

/* Writes a pair of u8 to the pipe. */
void GXTexCoord2u8(u8 x, u8 y) {
    GXWGFifo.u8 = x;
    GXWGFifo.u8 = y;
}

/* Writes one u16 to the pipe. */
void GXColor1x16(u16 value) {
    GXWGFifo.u16 = value;
}

/* Writes one u8 to the pipe. */
void GXColor1x8(u8 value) {
    GXWGFifo.u8 = value;
}

/* Writes four u8 to the pipe (an RGBA colour). */
void GXColor4u8(u8 r, u8 g, u8 b, u8 a) {
    GXWGFifo.u8 = r;
    GXWGFifo.u8 = g;
    GXWGFifo.u8 = b;
    GXWGFifo.u8 = a;
}

/* Writes three u8 to the pipe (an RGB colour). */
void GXColor3u8(u8 r, u8 g, u8 b) {
    GXWGFifo.u8 = r;
    GXWGFifo.u8 = g;
    GXWGFifo.u8 = b;
}

/* Writes one u16 to the pipe. */
void GXNormal1x16(u16 value) {
    GXWGFifo.u16 = value;
}

/* Writes one u8 to the pipe. */
void GXNormal1x8(u8 value) {
    GXWGFifo.u8 = value;
}

/* Writes three f32 to the pipe. */
void GXNormal3f32(f32 x, f32 y, f32 z) {
    GXWGFifo.f32 = x;
    GXWGFifo.f32 = y;
    GXWGFifo.f32 = z;
}

/* Writes three s16 to the pipe. */
void GXNormal3s16(s16 x, s16 y, s16 z) {
    GXWGFifo.s16 = x;
    GXWGFifo.s16 = y;
    GXWGFifo.s16 = z;
}

/* Writes three s8 to the pipe. */
void GXNormal3s8(s8 x, s8 y, s8 z) {
    GXWGFifo.s8 = x;
    GXWGFifo.s8 = y;
    GXWGFifo.s8 = z;
}

/* Writes one u16 to the pipe. */
void GXPosition1x16(u16 value) {
    GXWGFifo.u16 = value;
}

/* Writes one u8 to the pipe. */
void GXPosition1x8(u8 value) {
    GXWGFifo.u8 = value;
}

/* Writes a pair of f32 to the pipe. */
void GXPosition2f32(f32 x, f32 y) {
    GXWGFifo.f32 = x;
    GXWGFifo.f32 = y;
}

/* Writes a pair of s16 to the pipe. */
void GXPosition2s16(s16 x, s16 y) {
    GXWGFifo.s16 = x;
    GXWGFifo.s16 = y;
}

/* Writes a pair of u16 to the pipe. */
void GXPosition2u16(u16 x, u16 y) {
    GXWGFifo.u16 = x;
    GXWGFifo.u16 = y;
}

/* Writes a pair of s8 to the pipe. */
void GXPosition2s8(s8 x, s8 y) {
    GXWGFifo.s8 = x;
    GXWGFifo.s8 = y;
}

/* Writes a pair of u8 to the pipe. */
void GXPosition2u8(u8 x, u8 y) {
    GXWGFifo.u8 = x;
    GXWGFifo.u8 = y;
}

/* Writes three f32 to the pipe. */
void GXPosition3f32(f32 x, f32 y, f32 z) {
    GXWGFifo.f32 = x;
    GXWGFifo.f32 = y;
    GXWGFifo.f32 = z;
}

/* Writes three s16 to the pipe. */
void GXPosition3s16(s16 x, s16 y, s16 z) {
    GXWGFifo.s16 = x;
    GXWGFifo.s16 = y;
    GXWGFifo.s16 = z;
}

/* Writes three u16 to the pipe. */
void GXPosition3u16(u16 x, u16 y, u16 z) {
    GXWGFifo.u16 = x;
    GXWGFifo.u16 = y;
    GXWGFifo.u16 = z;
}

/* Writes three s8 to the pipe. */
void GXPosition3s8(s8 x, s8 y, s8 z) {
    GXWGFifo.s8 = x;
    GXWGFifo.s8 = y;
    GXWGFifo.s8 = z;
}

/* Writes three u8 to the pipe. */
void GXPosition3u8(u8 x, u8 y, u8 z) {
    GXWGFifo.u8 = x;
    GXWGFifo.u8 = y;
    GXWGFifo.u8 = z;
}

}  // extern "C"

namespace nw4r {
namespace ef {

/* 0x800C7270 (0x188): loads the particle's alpha-compare references unless they are already current. */
bool DrawStrategyImpl::SetupGPAlpha(EfDrawParticle* pp, const EfEmitterDrawSetting& setting, bool force) {
    if (!IsValidPointer((u32)pp)) {
        nw4r::db::Panic(ef_strategy_impl_file_str, 591, ef_strategy_impl_pp_assert_str, pp);
    }
    if (force || pp->alpha_ref0 != mPrevAlphaRef0) {
        mPrevAlphaRef0 = pp->alpha_ref0;
        mPrevAlphaRef1 = pp->alpha_ref0;
        GXSetAlphaCompare(setting.alpha_comp0, pp->alpha_ref0, setting.alpha_op, setting.alpha_comp1,
                          pp->alpha_ref1);
        return true;
    }
    return false;
}

/* Multiplies two 8-bit colour components, rounding. */
inline u8 MulColorComponent(u8 a, u8 b) {
    return (u8)((a * b + 0x80) >> 8);
}

/* Multiplies the RGB of two colours into `dst`, leaving its alpha. */
inline void MulColorRGB(GXColor* dst, const GXColor* a, const GXColor* b) {
    u8 g = MulColorComponent(a->g, b->g);
    u8 blue = MulColorComponent(a->b, b->b);
    dst->r = MulColorComponent(a->r, b->r);
    dst->g = g;
    dst->b = blue;
}

/* 0x800C73F8 (0x8E8): fetches the particle colours the TEV registers read, scales their alphas by the flicker and
 * the emitter colour, and loads every TEV colour and konstant colour that changed. */
bool DrawStrategyImpl::SetupGPColor(EfDrawParticle* pp, const EfEmitterDrawSetting& setting, bool force) {
    bool dirty;
    u8 alpha_scale;
    int i;

    if (!IsValidPointer((u32)pp)) {
        nw4r::db::Panic(ef_strategy_impl_file_str, 610, ef_strategy_impl_pp_assert_str, pp);
    }
    dirty = false;

    if (mUseColor[0][0]) {
        ef_particle_get_color((EfParticle*)pp, 0, 0, (u8*)&mColor[0][0]);
    }
    if (mUseColor[0][1]) {
        ef_particle_get_color((EfParticle*)pp, 0, 1, (u8*)&mColor[0][1]);
    }
    if (mUseColor[1][0]) {
        ef_particle_get_color((EfParticle*)pp, 1, 0, (u8*)&mColor[1][0]);
    }
    if (mUseColor[1][1]) {
        ef_particle_get_color((EfParticle*)pp, 1, 1, (u8*)&mColor[1][1]);
    }

    alpha_scale = 0xFF;
    if ((mUseAlpha[0][0] || mUseAlpha[0][1] || mUseAlpha[1][0] || mUseAlpha[1][1]) &&
        setting.alpha_flick_type != 0) {
        alpha_scale = ef_particle_flick_alpha((EfParticle*)pp);
    }
    if (mUseAlpha[0][0]) {
        mColor[0][0].a = ef_particle_get_alpha((EfParticle*)pp, 0, 0);
        if (alpha_scale != 0xFF) {
            mColor[0][0].a = MulColorComponent(mColor[0][0].a, alpha_scale);
        }
    }
    if (mUseAlpha[0][1]) {
        mColor[0][1].a = ef_particle_get_alpha((EfParticle*)pp, 0, 1);
        if (alpha_scale != 0xFF) {
            mColor[0][1].a = MulColorComponent(mColor[0][1].a, alpha_scale);
        }
    }
    if (mUseAlpha[1][0]) {
        mColor[1][0].a = ef_particle_get_alpha((EfParticle*)pp, 1, 0);
        if (alpha_scale != 0xFF) {
            mColor[1][0].a = MulColorComponent(mColor[1][0].a, alpha_scale);
        }
    }
    if (mUseAlpha[1][1]) {
        mColor[1][1].a = ef_particle_get_alpha((EfParticle*)pp, 1, 1);
        if (alpha_scale != 0xFF) {
            mColor[1][1].a = MulColorComponent(mColor[1][1].a, alpha_scale);
        }
    }

    if (pp->manager->inherit_emitter_color != 0) {
        if (mUseColor[0][0] || mUseColor[0][1] || mUseAlpha[0][0] || mUseAlpha[0][1]) {
            ef_pm_modulate_color(pp->manager, pp, &mColor[0][0], &mColor[0][1]);
        }
        if (mUseColor[1][0] || mUseColor[1][1] || mUseAlpha[1][0] || mUseAlpha[1][1]) {
            ef_pm_modulate_color(pp->manager, pp, &mColor[1][0], &mColor[1][1]);
        }
    }

    for (i = 0; i < 3; i++) {
        GXColor color = {0, 0, 0, 0};

        if (setting.color_tev[i] != 0 || setting.alpha_tev[i] != 0) {
            switch (setting.color_tev[i]) {
            case 1:
                color.r = mColor[0][0].r;
                color.g = mColor[0][0].g;
                color.b = mColor[0][0].b;
                break;
            case 2:
                color.r = mColor[0][1].r;
                color.g = mColor[0][1].g;
                color.b = mColor[0][1].b;
                break;
            case 5:
                MulColorRGB(&color, &mColor[0][0], &mColor[0][1]);
                break;
            case 3:
                color.r = mColor[1][0].r;
                color.g = mColor[1][0].g;
                color.b = mColor[1][0].b;
                break;
            case 4:
                color.r = mColor[1][1].r;
                color.g = mColor[1][1].g;
                color.b = mColor[1][1].b;
                break;
            case 6:
                MulColorRGB(&color, &mColor[1][0], &mColor[1][1]);
                break;
            }
            switch (setting.alpha_tev[i]) {
            case 1:
                color.a = mColor[0][0].a;
                break;
            case 2:
                color.a = mColor[0][1].a;
                break;
            case 5:
                color.a = MulColorComponent(mColor[0][0].a, mColor[0][1].a);
                break;
            case 3:
                color.a = mColor[1][0].a;
                break;
            case 4:
                color.a = mColor[1][1].a;
                break;
            case 6:
                color.a = MulColorComponent(mColor[1][0].a, mColor[1][1].a);
                break;
            }
            if (force || color.r != mPrevTevColor[i].r || color.g != mPrevTevColor[i].g ||
                color.b != mPrevTevColor[i].b || color.a != mPrevTevColor[i].a) {
                GXSetTevColor(i + 1, color);
                color_rgba_copy((u8*)&mPrevTevColor[i], (const u8*)&color);
                dirty = true;
            }
        }
    }

    for (i = 0; i < 4; i++) {
        GXColor color = {0, 0, 0, 0};

        if (setting.color_tev_k[i] != 0 || setting.alpha_tev_k[i] != 0) {
            switch (setting.color_tev_k[i]) {
            case 1:
                color.r = mColor[0][0].r;
                color.g = mColor[0][0].g;
                color.b = mColor[0][0].b;
                break;
            case 2:
                color.r = mColor[0][1].r;
                color.g = mColor[0][1].g;
                color.b = mColor[0][1].b;
                break;
            case 5:
                MulColorRGB(&color, &mColor[0][0], &mColor[0][1]);
                break;
            case 3:
                color.r = mColor[1][0].r;
                color.g = mColor[1][0].g;
                color.b = mColor[1][0].b;
                break;
            case 4:
                color.r = mColor[1][1].r;
                color.g = mColor[1][1].g;
                color.b = mColor[1][1].b;
                break;
            case 6:
                MulColorRGB(&color, &mColor[1][0], &mColor[1][1]);
                break;
            }
            switch (setting.alpha_tev_k[i]) {
            case 1:
                color.a = mColor[0][0].a;
                break;
            case 2:
                color.a = mColor[0][1].a;
                break;
            case 5:
                color.a = MulColorComponent(mColor[0][0].a, mColor[0][1].a);
                break;
            case 3:
                color.a = mColor[1][0].a;
                break;
            case 4:
                color.a = mColor[1][1].a;
                break;
            case 6:
                color.a = MulColorComponent(mColor[1][0].a, mColor[1][1].a);
                break;
            }
            if (force || color.r != mPrevTevKColor[i].r || color.g != mPrevTevKColor[i].g ||
                color.b != mPrevTevKColor[i].b || color.a != mPrevTevKColor[i].a) {
                GXSetTevKColor(i, color);
                color_rgba_copy((u8*)&mPrevTevKColor[i], (const u8*)&color);
                dirty = true;
            }
        }
    }
    return dirty;
}

/* 0x800C7CE0 (0x994): loads each enabled layer's texture object when the texture or its wrap changed, and its
 * texture matrix (scale, rotation, translation, mirroring, and the projection for a projected layer) when the
 * transform changed. */
bool DrawStrategyImpl::SetupGPTexture(EfDrawParticle* pp, const EfEmitterDrawSetting& setting,
                                      const EfDrawInfo& info, bool force) {
    bool loaded;
    int layer;

    if (!IsValidPointer((u32)pp)) {
        nw4r::db::Panic(ef_strategy_impl_file_str, 881, ef_strategy_impl_pp_assert_str, pp);
    }
    loaded = false;
    for (layer = 0; layer < 3; layer++) {
        if (mTexmapMap[layer] >= 0) {
            EfTextureData* tex;

            EF_VALID_PTR_ASSERT(ef_strategy_impl_file_str, 890, ef_strategy_impl_texture_assert_str, pp->texture[layer]);
            tex = pp->texture[layer];
            if (tex != NULL) {
                s32 wrap_s = ef_particle_wrap_s(pp, layer);
                s32 wrap_t = ef_particle_wrap_t(pp, layer);
                f32 scale_s;
                f32 scale_t;
                f32 offset_s;
                f32 offset_t;

                if (force || tex != mPrevTexture[layer].texture || wrap_s != mPrevTexture[layer].wrap_s || wrap_t != mPrevTexture[layer].wrap_t) {
                    GXTexObj tex_obj;
                    u8 format;

                    loaded = true;
                    mPrevTexture[layer].texture = tex;
                    mPrevTexture[layer].wrap_s = wrap_s;
                    mPrevTexture[layer].wrap_t = wrap_t;
                    format = tex->format;
                    switch (format) {
                    case 8:
                    case 9:
                    case 10: {
                        GXTlutObj tlut_obj;

                        GXInitTlutObj(&tlut_obj, tex->tlut, tex->tlut_format, tex->tlut_entries);
                        GXLoadTlut(&tlut_obj, mTexmapMap[layer]);
                        GXInitTexObjCI(&tex_obj, tex->image, tex->width, tex->height, format, wrap_s, wrap_t,
                                       tex->mipmap_count > 1, mTexmapMap[layer]);
                        break;
                    }
                    case 0:
                    case 1:
                    case 2:
                    case 3:
                    case 4:
                    case 5:
                    case 6:
                    case 14:
                        GXInitTexObj(&tex_obj, tex->image, tex->width, tex->height, format, wrap_s, wrap_t,
                                     tex->mipmap_count > 1);
                        break;
                    default:
                        nw4r::db::Panic(ef_strategy_impl_file_str, 954, ef_strategy_impl_false_assert_str);
                        break;
                    }
                    GXInitTexObjLOD(&tex_obj, tex->min_filter, tex->mag_filter, 0.0f,
                                    -1.0f + tex->mipmap_count, tex->lod_bias, 0, 0, 0);
                    GXLoadTexObj(&tex_obj, mTexmapMap[layer]);
                }

                scale_s = ef_particle_tex_scale_s(pp, layer);
                scale_t = ef_particle_tex_scale_t(pp, layer);
                offset_s = ef_particle_tex_offset_s(pp, layer);
                offset_t = ef_particle_tex_offset_t(pp, layer);
                if (force || scale_s != mPrevTexture[layer].scale_s || scale_t != mPrevTexture[layer].scale_t || offset_s != mPrevTexture[layer].offset_s ||
                    offset_t != mPrevTexture[layer].offset_t || pp->tex_scale[layer].x != mPrevTexture[layer].scale.x ||
                    pp->tex_scale[layer].y != mPrevTexture[layer].scale.y || pp->tex_rotate[layer] != mPrevTexture[layer].rotate ||
                    pp->tex_translate[layer].x != mPrevTexture[layer].translate.x ||
                    pp->tex_translate[layer].y != mPrevTexture[layer].translate.y) {
                    MTX34 mtx;
                    u8* flags;

                    loaded = true;
                    mPrevTexture[layer].scale_s = scale_s;
                    mPrevTexture[layer].scale_t = scale_t;
                    mPrevTexture[layer].offset_s = offset_s;
                    mPrevTexture[layer].offset_t = offset_t;
                    mPrevTexture[layer].scale.x = pp->tex_scale[layer].x;
                    mPrevTexture[layer].scale.y = pp->tex_scale[layer].y;
                    mPrevTexture[layer].rotate = pp->tex_rotate[layer];
                    mPrevTexture[layer].translate.x = pp->tex_translate[layer].x;
                    mPrevTexture[layer].translate.y = pp->tex_translate[layer].y;

                    MTX34_ctor(&mtx);
                    mtx34_identity(&mtx);
                    mtx.m[0][3] = pp->tex_translate[layer].x;
                    mtx.m[1][3] = pp->tex_translate[layer].y;
                    if (0.0f != pp->tex_rotate[layer]) {
                        f32 sin;
                        f32 cos;
                        f32 du = mtx.m[0][3] - 0.5f;
                        f32 dv = mtx.m[1][3] - 0.5f;

                        ef_sin_cos(&sin, &cos, pp->tex_rotate[layer]);
                        mtx.m[1][1] = cos;
                        mtx.m[0][0] = cos;
                        mtx.m[0][1] = -sin;
                        mtx.m[1][0] = sin;
                        mtx.m[0][3] = 0.5f + cos * du - sin * dv;
                        mtx.m[1][3] = 0.5f + sin * du + cos * dv;
                    }
                    if (1.0f != scale_s || 1.0f != scale_t ||
                        1.0f != pp->tex_scale[layer].x || 1.0f != pp->tex_scale[layer].y ||
                        0.0f != offset_s || 0.0f != offset_t) {
                        f32 sx = pp->tex_scale[layer].x;
                        f32 ss = scale_s * sx;
                        f32 sy = pp->tex_scale[layer].y;
                        f32 st = scale_t * sy;

                        mtx.m[0][0] *= ss;
                        mtx.m[0][1] *= ss;
                        mtx.m[0][3] = offset_s + scale_s * (0.5f + (mtx.m[0][3] - 0.5f) * sx);
                        mtx.m[1][0] *= st;
                        mtx.m[1][1] *= st;
                        mtx.m[1][3] = offset_t + scale_t * (0.5f + (mtx.m[1][3] - 0.5f) * sy);
                    }

                    {
                        EfDrawParticleManager* handle;

                        ef_pm_handle(&handle, pp->manager);
                        flags = ef_emitter_tex_flags(&handle);
                    }
                    if (layer == 0) {
                        if ((flags[2] & 4) != 0 && (pp->tex_flags & 1) != 0) {
                            mtx.m[0][0] *= -1.0f;
                            mtx.m[0][3] += pp->tex_scale[layer].x;
                            mtx.m[1][1] *= -1.0f;
                            mtx.m[1][3] += pp->tex_scale[layer].y;
                        } else {
                            if ((flags[2] & 8) != 0) {
                                mtx.m[0][0] *= -1.0f;
                                mtx.m[0][3] += pp->tex_scale[layer].x;
                            }
                            if ((flags[2] & 0x10) != 0) {
                                mtx.m[1][1] *= -1.0f;
                                mtx.m[1][3] += pp->tex_scale[layer].y;
                            }
                        }
                    }
                    if (layer == 1) {
                        if ((flags[2] & 0x20) != 0 && (pp->tex_flags & 1) != 0) {
                            mtx.m[0][0] *= -1.0f;
                            mtx.m[0][3] += pp->tex_scale[layer].x;
                            mtx.m[1][1] *= -1.0f;
                            mtx.m[1][3] += pp->tex_scale[layer].y;
                        } else {
                            if ((flags[1] & 0x40) != 0) {
                                mtx.m[0][0] *= -1.0f;
                                mtx.m[0][3] += pp->tex_scale[layer].x;
                            }
                            if ((flags[1] & 0x80) != 0) {
                                mtx.m[1][1] *= -1.0f;
                                mtx.m[1][3] += pp->tex_scale[layer].y;
                            }
                        }
                    }

                    if (((setting.flags >> (layer + 7)) & 1) == 0) {
                        GXLoadTexMtxImm((const f32(*)[4])mtx34_get_ptr(&mtx), mTexmapMap[layer] * 3 + 30, 1);
                        if (force) {
                            ef_set_tex_coord_gen(mTexmapMap[layer], 1, 4, mTexmapMap[layer] * 3 + 30);
                        }
                    } else {
                        mtx34_concat(&mtx, ef_draw_info_projection(&info), &mtx);
                        GXLoadTexMtxImm((const f32(*)[4])mtx34_get_ptr(&mtx), mTexmapMap[layer] * 3 + 64, 0);
                        if (force) {
                            GXSetTexCoordGen2(mTexmapMap[layer], 0, 0, 0, 0, mTexmapMap[layer] * 3 + 64);
                        }
                    }
                }
            }
        }
    }
    return loaded;
}

}  // namespace ef
}  // namespace nw4r

/* --------------------------------------------------------------------------------------------- *
 * The texture-coordinate wrapper and the particle's texture-layer accessors (`particle.h`), in address order.
 * --------------------------------------------------------------------------------------------- */

extern "C" {

/* 0x800C8674 (0xC): writes one texture-coordinate generator and leaves the projective matrix at identity. */
void ef_set_tex_coord_gen(u32 dst_coord, u32 func, u32 src_param, u32 mtx) {
    GXSetTexCoordGen2(dst_coord, func, src_param, mtx, 0, 125);
}

/* 0x800C8680 (0xB4): the T translation of a layer: 1 when reversed, doubled for a mirrored repeat. */
s32 ef_particle_tex_offset_t(EfDrawParticle* self, int layer) {
    int ok;
    u8 flags;
    u32 value;

    ok = (layer >= 0 && layer < 3);
    if (!ok) {
        nw4r::db::Panic(ef_strategy_impl_particle_h_str5, 541, ef_strategy_impl_layer_assert_str5);
    }
    value = 0;
    flags = ((s32)self->texture_reverse >> (layer * 2)) & 3;
    if (flags == 2 || flags == 3) {
        value = 1;
    }
    if (ef_particle_wrap_t(self, layer) == 2) {
        value <<= 1;
    }
    return value;
}

/* 0x800C8734 (0xB4): the S translation of a layer: 1 when reversed, doubled for a mirrored repeat. */
s32 ef_particle_tex_offset_s(EfDrawParticle* self, int layer) {
    int ok;
    u32 flags;
    u32 value;

    ok = (layer >= 0 && layer < 3);
    if (!ok) {
        nw4r::db::Panic(ef_strategy_impl_particle_h_str4, 512, ef_strategy_impl_layer_assert_str4);
    }
    value = 0;
    flags = ((s32)self->texture_reverse >> (layer * 2)) & 3;
    if (flags == 1 || flags == 3) {
        value = 1;
    }
    if (ef_particle_wrap_s(self, layer) == 2) {
        value <<= 1;
    }
    return value;
}

/* 0x800C87E8 (0xB4): the signed T repeat of a layer: 2 for a mirrored repeat, negative when reversed. */
s32 ef_particle_tex_scale_t(EfDrawParticle* self, int layer) {
    int ok;
    u8 flags;
    s32 value;

    ok = (layer >= 0 && layer < 3);
    if (!ok) {
        nw4r::db::Panic(ef_strategy_impl_particle_h_str3, 483, ef_strategy_impl_layer_assert_str3);
    }
    value = 1;
    if (ef_particle_wrap_t(self, layer) == 2) {
        value = 2;
    }
    flags = ((s32)self->texture_reverse >> (layer * 2)) & 3;
    if (flags == 2 || flags == 3) {
        value = -value;
    }
    return value;
}

/* 0x800C889C (0xB8): the signed S repeat of a layer: 2 for a mirrored repeat, negative when reversed. */
s32 ef_particle_tex_scale_s(EfDrawParticle* self, int layer) {
    int ok;
    u32 flags;
    s32 value;

    ok = (layer >= 0 && layer < 3);
    if (!ok) {
        nw4r::db::Panic(ef_strategy_impl_particle_h_str2, 454, ef_strategy_impl_layer_assert_str2);
    }
    value = 1;
    if (ef_particle_wrap_s(self, layer) == 2) {
        value = 2;
    }
    flags = ((s32)self->texture_reverse >> (layer * 2)) & 3;
    if (flags == 1 || flags == 3) {
        value -= value << 1;
    }
    return value;
}

/* 0x800C8954 (0x7C): the T wrap mode of one texture layer. */
s32 ef_particle_wrap_t(EfDrawParticle* self, int layer) {
    int ok;

    ok = (layer >= 0 && layer < 3);
    if (!ok) {
        nw4r::db::Panic(ef_strategy_impl_particle_h_str1, 414, ef_strategy_impl_layer_assert_str1);
    }
    return ((s32)self->texture_wrap >> (layer * 4 + 2)) & 3;
}

/* 0x800C89D0 (0x78): the S wrap mode of one texture layer. */
s32 ef_particle_wrap_s(EfDrawParticle* self, int layer) {
    int ok;

    ok = (layer >= 0 && layer < 3);
    if (!ok) {
        nw4r::db::Panic(ef_strategy_impl_particle_h_str0, 375, ef_strategy_impl_layer_assert_str0);
    }
    return ((s32)self->texture_wrap >> (layer * 4)) & 3;
}

}  // extern "C"

namespace nw4r {
namespace ef {

/* 0x800C8A48 (0x1C): the walker that starts a draw: the younger particles first for draw order 0. */
DrawStrategyImpl::GetFirstDrawParticleFunc DrawStrategyImpl::GetGetFirstDrawParticleFunc(int draw_order) {
    if (draw_order == 0) {
        return ef_pm_first_youngest;
    }
    return ef_pm_first_eldest;
}

/* 0x800C8A64 (0x1C): the walker that continues a draw in the same order. */
DrawStrategyImpl::GetNextDrawParticleFunc DrawStrategyImpl::GetGetNextDrawParticleFunc(int draw_order) {
    if (draw_order == 0) {
        return ef_pm_next_youngest;
    }
    return ef_pm_next_eldest;
}

}  // namespace ef
}  // namespace nw4r

extern "C" {

/* 0x800C8A80 (0x11C): the first particle of the manager's list, eldest first. */
EfDrawParticle* ef_pm_first_eldest(EfDrawParticleManager* pm) {
    if (!IsValidPointer((u32)pm)) {
        nw4r::db::Panic(ef_strategy_impl_file_str, 1190, ef_strategy_impl_pm_assert_str, pm);
    }
    return (EfDrawParticle*)ef_pm_last_alive((EfParticleState*)pm);
}

/* 0x800C8B9C (0x11C): the first particle of the manager's list, youngest first. */
EfDrawParticle* ef_pm_first_youngest(EfDrawParticleManager* pm) {
    if (!IsValidPointer((u32)pm)) {
        nw4r::db::Panic(ef_strategy_impl_file_str, 1197, ef_strategy_impl_pm_assert_str, pm);
    }
    return (EfDrawParticle*)ef_pm_first_alive((EfDrawList*)pm);
}

/* 0x800C8CB8 (0x12C): the particle after `p`, eldest first. */
EfDrawParticle* ef_pm_next_eldest(EfDrawParticleManager* pm, EfDrawParticle* p) {
    if (!IsValidPointer((u32)pm)) {
        nw4r::db::Panic(ef_strategy_impl_file_str, 1204, ef_strategy_impl_pm_assert_str, pm);
    }
    return (EfDrawParticle*)ef_pm_prev_alive(pm, p);
}

/* 0x800C8DE4 (0x12C): the particle after `p`, youngest first. */
EfDrawParticle* ef_pm_next_youngest(EfDrawParticleManager* pm, EfDrawParticle* p) {
    if (!IsValidPointer((u32)pm)) {
        nw4r::db::Panic(ef_strategy_impl_file_str, 1211, ef_strategy_impl_pm_assert_str, pm);
    }
    return (EfDrawParticle*)ef_pm_next_alive(pm, p);
}

}  // extern "C"

namespace nw4r {
namespace ef {

/* 0x800C8F10 (0x524): takes the emitter and manager transforms of a draw and the emitter's Y axis and origin in
 * manager space (and, for draw types 5 and 7, the manager's own Y axis). */
DrawStrategyImpl::AheadContext::AheadContext(const MTX34* view, EfDrawParticleManager* pm) {
    EfEmitterDrawSetting* ed;
    VEC3 axis_y;
    VEC3 center;

    ef_ahead_context_members_ctor(this);
    if (!IsValidPointer((u32)pm)) {
        nw4r::db::Panic(ef_strategy_impl_file_str, 1219, ef_strategy_impl_pm_assert_str, pm);
    }
    view_mtx = view;
    particle_manager = pm;
    EF_VALID_PTR_ASSERT(ef_strategy_impl_file_str, 1224, ef_strategy_impl_manager_em_assert_str, pm->emitter);
    ef_emitter_get_mtx(pm->emitter, &emitter_mtx);
    ef_pm_get_mtx(pm, &manager_mtx);
    mtx34_inverse(&manager_mtx_inv, &manager_mtx);

    setVec3(&axis_y, emitter_mtx.m[0][1], emitter_mtx.m[1][1], emitter_mtx.m[2][1]);
    mtx34_rotate_vec3(&axis_y, &manager_mtx_inv, &axis_y);
    if (ef_vec3_normalize(&axis_y) == 0) {
        VEC3 up;
        copyVec3(&axis_y, setVec3(&up, 0.0f, 1.0f, 0.0f));
    }
    copyVec3(&emitter_axis_y, &axis_y);

    setVec3(&center, emitter_mtx.m[0][3], emitter_mtx.m[1][3], emitter_mtx.m[2][3]);
    ef_vec3_transform(&center, &manager_mtx_inv, &center);
    copyVec3(&emitter_center, &center);

    EF_VALID_PTR_ASSERT(ef_strategy_impl_file_str, 1250, ef_strategy_impl_resource_assert_str, pm->resource);
    ed = (EfEmitterDrawSetting*)ef_resource_draw_setting(pm->resource);
    if (!IsValidPointer((u32)ed)) {
        nw4r::db::Panic(ef_strategy_impl_file_str, 1253, ef_strategy_impl_ed_assert_str, ed);
    }
    if ((s32)ed->type_direction == 5 || (s32)ed->type_direction == 7) {
        VEC3 manager_y;

        setVec3(&manager_y, manager_mtx_inv.m[0][1], manager_mtx_inv.m[1][1], manager_mtx_inv.m[2][1]);
        if (ef_vec3_normalize(&manager_y) == 0) {
            VEC3 up;
            copyVec3(&manager_y, setVec3(&up, 0.0f, 1.0f, 0.0f));
        }
        vec3_assign(&manager_axis_y, &manager_y);
    }
}

}  // namespace ef
}  // namespace nw4r

extern "C" {

/* 0x800C9434 (0x54): the ahead context's member constructors (three matrices, two vectors). */
EfAheadContext* ef_ahead_context_members_ctor(EfAheadContext* self) {
    MTX34_ctor(&self->emitter_mtx);
    MTX34_ctor(&self->manager_mtx);
    MTX34_ctor(&self->manager_mtx_inv);
    VEC3_ctor(&self->emitter_axis_y);
    VEC3_ctor(&self->emitter_center);
    return self;
}

/* 0x800C9488 (0xB8): builds the four basis vectors and the identity matrix the strategies start from. */
void ef_strategy_init_basis(void) {
    setVec3(&ef_unit_x_vec, 1.0f, 0.0f, 0.0f);
    setVec3(&ef_unit_y_vec, 0.0f, 1.0f, 0.0f);
    setVec3(&ef_unit_z_vec, 0.0f, 0.0f, 1.0f);
    setVec3(&ef_zero_vec, 0.0f, 0.0f, 0.0f);
    mtx34_set(&ef_identity_mtx, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);
}

}  // extern "C"
