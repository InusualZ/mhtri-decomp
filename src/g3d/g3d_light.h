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
#endif

#endif /* MHTRI_G3D_G3D_LIGHT_H */
