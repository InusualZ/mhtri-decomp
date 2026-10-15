/*
 * g3d/light_obj_is_enable.h - the light-object, light-setting and fog accessors `g3d/g3d_anmscn.cpp` defines that
 *   `g3d/g3d_state.cpp` and `g3d/g3d_scnroot.cpp` call (a leaf header: the owner's full header does not carry
 *   them yet).
 */
#ifndef MHTRI_G3D_LIGHT_OBJ_IS_ENABLE_H
#define MHTRI_G3D_LIGHT_OBJ_IS_ENABLE_H

#include "types.h"

#ifdef __cplusplus
struct G3dLightObj;
struct G3dLightSetting;
struct ResHandle;
struct FogData;

/* 0x8006518C - whether the light object is enabled. */
extern "C" BOOL light_obj_is_enable(const G3dLightObj* pSelf);
/* 0x800651B4/0x800651E0 - the light setting's light-object array and its length. */
extern "C" G3dLightObj* light_setting_get_light_obj_array(const G3dLightSetting* pSelf);
extern "C" u32 light_setting_get_num_light_obj(const G3dLightSetting* pSelf);
/* 0x800659C4 / 0x80067A54 - whether the fog handle names a record, and that record. */
extern "C" BOOL fog_is_valid(const ResHandle* pSelf);
extern "C" FogData* fog_ptr(ResHandle* pSelf);
/* 0x80065264 - the smaller of the two words (`std::min<u32>`). */
extern "C" const u32* g3d_min_u32(const u32* pA, const u32* pB);
#endif

#endif /* MHTRI_G3D_LIGHT_OBJ_IS_ENABLE_H */
