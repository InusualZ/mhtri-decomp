/* g3d/g3d_light.h - the declarations `g3d/g3d_light.cpp` owns: nw4r g3d's light object, ambient light, light-set
 *   record, light setting and light set, and the C entry points its consumers call by their map names. */
#ifndef MHTRI_G3D_G3D_LIGHT_H
#define MHTRI_G3D_G3D_LIGHT_H

#include "types.h"

#ifdef __cplusplus
#include "gx.h"
#include "nw4r/math.h"

namespace nw4r {
namespace g3d {

/* One GX light and its kind flags (bit 0: directional, bit 1: specular, bit 2: enabled, bit 3: specular direction
 * set).  The constructor and destructor are inline; their kept copies (0x8007AF1C, 0x8007AF28) follow
 * g3d_light.cpp's LightSetting constructor.  size: 0x44 */
class LightObj {
public:
    LightObj() : mFlag(0) {}
    ~LightObj() {}
    LightObj& operator=(const LightObj& rhs);

    void Clear();
    void InitLightColor(GXColor color);
    void InitLightSpot(f32 cutoff, u32 spotFn);
    void InitLightAttnA(f32 a0, f32 a1, f32 a2);
    void InitLightDistAttn(f32 refDist, f32 refBright, u32 distAttnFn);
    void InitLightAttnK(f32 k0, f32 k1, f32 k2);
    void InitLightShininess(f32 shininess);
    void GetLightColor(GXColor* pColor) const;
    void ApplyViewMtx(const math::MTX34& camMtx);

    /* +0x00 */ u32 mFlag;
    /* +0x04 */ GXLightObj mObj;
};

/* An ambient light colour.  size: 0x4 */
struct AmbLightObj {
    /* +0x0 */ u8 r;
    /* +0x1 */ u8 g;
    /* +0x2 */ u8 b;
    /* +0x3 */ u8 a;
};

/* One light set: the light objects (-1 = none) and the ambient light it selects.  size: 0xC */
struct LightSetData {
    /* +0x0 */ s8 idxLight[8];
    /* +0x8 */ s8 idxAmbLight;
    /* +0x9 */ u8 pad_0x9[3];
};

/* The light objects, ambient lights and light sets a scene shares.  The destructor is inline (its kept copy,
 * 0x80083444, follows g3d_scnroot.cpp's ScnRoot constructor).  size: 0x10 */
class LightSetting {
public:
    LightSetting(LightObj* pLightObjArray, AmbLightObj* pAmbLightObjArray, u32 numLight,
                 LightSetData* pLightSetDataArray, u32 numLightSet);
    ~LightSetting() {}
    bool Import(const LightSetting& rhs);
    void ApplyViewMtx(const math::MTX34& camMtx, u32 numLight);

    /* +0x00 */ u16 mNumLight;
    /* +0x02 */ u16 mNumLightSet;
    /* +0x04 */ LightObj* mpLightObjArray;
    /* +0x08 */ AmbLightObj* mpAmbLightObjArray;
    /* +0x0C */ LightSetData* mpLightSetDataArray;
};

/* One light set of a light setting.  size: 0x8 */
class LightSet {
public:
    bool SelectLightObj(u32 lightIdxInSet, int lightObjIdx);
    bool SelectAmbLightObj(int ambLightObjIdx);

    /* +0x00 */ LightSetting* mpSetting;
    /* +0x04 */ LightSetData* mpLightSetData;
};

}  // namespace g3d
}  // namespace nw4r

extern "C" {
/* 0x8007A7C8/0x8007A7E4 (0x1C): read the light object's position and direction into `pOut` when it is given
 * (`g3d/g3d_draw.cpp`'s indirect-matrix set-up calls them). */
void light_obj_get_light_pos(const nw4r::g3d::LightObj* pLightObj, nw4r::math::VEC3* pOut);
void light_obj_get_light_dir(const nw4r::g3d::LightObj* pLightObj, nw4r::math::VEC3* pOut);
}
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8007A5E4/0x8007A5A8/0x8007A724 - the light object's direction, position and specular-direction setters (caller:
 * g3d/g3d_anmscn.cpp).  The object arrives in r3 and the three values in f1-f3. */
void fn_8007A5E4(void* self, f32 x, f32 y, f32 z); /* untyped: opaque handle - the LightObj */
void fn_8007A5A8(void* self, f32 x, f32 y, f32 z); /* untyped: opaque handle - the LightObj */
void fn_8007A724(void* self, f32 x, f32 y, f32 z); /* untyped: opaque handle - the LightObj */

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_G3D_G3D_LIGHT_H */
