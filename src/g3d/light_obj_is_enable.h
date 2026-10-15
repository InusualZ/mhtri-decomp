/*
 * g3d/light_obj_is_enable.h - the light-object, light-setting and fog accessors `g3d/g3d_anmscn.cpp` defines that
 *   `g3d/g3d_state.cpp` calls (a leaf header: the owner's full header does not carry them yet).
 */
#ifndef MHTRI_G3D_LIGHT_OBJ_IS_ENABLE_H
#define MHTRI_G3D_LIGHT_OBJ_IS_ENABLE_H

#include "types.h"

#ifdef __cplusplus
struct G3dLightObj;
struct G3dLightSetting;
struct G3dFog;

/* 0x8006518C - whether the light object is enabled. */
extern "C" BOOL light_obj_is_enable(const G3dLightObj* pSelf);
/* 0x800651B4/0x800651E0 - the light setting's light-object array and its length. */
extern "C" G3dLightObj* light_setting_get_light_obj_array(const G3dLightSetting* pSelf);
extern "C" u32 light_setting_get_num_light_obj(const G3dLightSetting* pSelf);
/* 0x800659C4 - whether the fog handle names a record. */
extern "C" BOOL fog_is_valid(const G3dFog* pSelf);
#endif

#endif /* MHTRI_G3D_LIGHT_OBJ_IS_ENABLE_H */
