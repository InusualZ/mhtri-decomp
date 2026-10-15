/* g3d/g3d_light.h - the declarations `g3d/g3d_light.cpp` owns (C linkage keeps the map's stems). */
#ifndef MHTRI_G3D_G3D_LIGHT_H
#define MHTRI_G3D_G3D_LIGHT_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8007A5E4/0x8007A5A8/0x8007A724 - the 3-float setters (caller: g3d/g3d_anmscn.cpp).  The object
 * arrives in r3 and the three values in f1-f3. */
void fn_8007A5E4(void* self, f32 x, f32 y, f32 z);
void fn_8007A5A8(void* self, f32 x, f32 y, f32 z);
void fn_8007A724(void* self, f32 x, f32 y, f32 z);

/* 0x8007A8E0 (0x618): builds a light setting over its light-object, light-animation and ambient buffers. */
/* untyped: opaque handle - the LightSetting record */
void* light_setting_ctor(void* pSelf, s32 pLightObjBuf, s32 pLightAnmBuf, u32 numLightObj, s32 pAmbLightObjBuf,
                         u32 numAmbLightObj);

/* 0x8007A7C8/0x8007A7E4 (0x1C): read the light object's position and direction into `pOut` when it is given
 * (`g3d/g3d_draw.cpp`'s specular pass calls them). */
u32 light_obj_get_light_pos(s32 pLightObj, s32 pOut);
u32 light_obj_get_light_dir(s32 pLightObj, s32 pOut);

#ifdef __cplusplus
}

struct G3dLightObj;
struct G3dLightSetting;
struct _GXColor;

/* 0x8007AEF8 - copies the ambient light colour `pSrc` into `pDst`. */
extern "C" void amb_light_obj_copy(_GXColor* pDst, const _GXColor* pSrc);
/* 0x8007AF1C/0x8007AF28 - constructs / destroys a light object. */
extern "C" G3dLightObj* light_obj_ctor(G3dLightObj* pSelf);
extern "C" G3dLightObj* light_obj_dtor(G3dLightObj* pSelf, s16 bDelete);
/* 0x8007AF6C - copies the light objects of `pSrc` into the setting; FALSE when they do not fit. */
extern "C" BOOL light_setting_import(G3dLightSetting* pSelf, const G3dLightSetting* pSrc);
/* 0x8007B0A0 - moves the first `numLightObj` light objects into view space through `pCamera`. */
extern "C" void light_setting_apply_view_mtx(G3dLightSetting* pSelf, const nw4r::math::MTX34* pCamera,
                                             u32 numLightObj);
#endif

#ifdef __cplusplus
#include "gx.h"

namespace nw4r {
namespace g3d {

/* A hardware light: the enable/colour/alpha flag word and the GXLightObj the initialisers fill (the 0x44 stride
 * `g3d/g3d_anmscn.cpp`'s light loops walk).  size: 0x44 */
struct LightObj {
    /* +0x00 */ u32 mFlags;
    /* +0x04 */ u8 mObj[0x40];
};

/* One light set's record: the eight light-object indices and the ambient one (the 0xC stride
 * LightSetting's light-set getter indexes).  size: 0xC */
struct LightSetData {
    /* +0x0 */ s8 idxLight[8];
    /* +0x8 */ s8 idxAmbLight;
    /* +0x9 */ u8 pad_0x09[3];
};

/* The scene's light buffers: the light-object and light-set counts and the three arrays.  size: 0x10 */
struct LightSetting {
    /* +0x00 */ u16 mNumLightObj;
    /* +0x02 */ u16 mNumLightSet;
    /* +0x04 */ LightObj* mpLightObjArray;
    /* +0x08 */ u32* mpAmbLightColorArray;
    /* +0x0C */ LightSetData* mpLightSetDataArray;
};

/* A view of one light set inside a LightSetting.  size: 0x8 */
struct LightSet {
    /* +0x0 */ LightSetting* mpSetting;
    /* +0x4 */ LightSetData* mpLightSetData;
};

}  // namespace g3d
}  // namespace nw4r

extern "C" {

/* The LightObj initialisers and the LightSet selectors `g3d/g3d_anmscn.cpp` calls (nw4r's LightObj::Clear,
 * InitLightColor, InitLightSpot, InitLightAttnA, InitLightDistAttn, InitLightAttnK, InitLightShininess and
 * LightSet::SelectLightObj / SelectAmbLightObj). */
void light_obj_clear(nw4r::g3d::LightObj* pSelf);                                       /* 0x8007A564 */
void light_obj_init_light_color(nw4r::g3d::LightObj* pSelf, GXColor color);            /* 0x8007A578 */
void light_obj_init_light_spot(nw4r::g3d::LightObj* pSelf, f32 cutoff, s32 spotFunc);   /* 0x8007A624 */
void light_obj_init_light_attn_a(nw4r::g3d::LightObj* pSelf, f32 a0, f32 a1, f32 a2);   /* 0x8007A664 */
void light_obj_init_light_dist_attn(nw4r::g3d::LightObj* pSelf, f32 refDistance, f32 refBrightness,
                                    s32 distFunc);                                     /* 0x8007A6A4 */
void light_obj_init_light_attn_k(nw4r::g3d::LightObj* pSelf, f32 k0, f32 k1, f32 k2);   /* 0x8007A6E4 */
void light_obj_init_light_shininess(nw4r::g3d::LightObj* pSelf, f32 shininess);         /* 0x8007A768 */
void light_set_select_light_obj(nw4r::g3d::LightSet* pSelf, u32 lightIdx, s32 lightObjIdx); /* 0x8007B160 */
void light_set_select_amb_light_obj(nw4r::g3d::LightSet* pSelf, s32 lightObjIdx);          /* 0x8007B224 */

}
#endif /* __cplusplus */

#endif /* MHTRI_G3D_G3D_LIGHT_H */
