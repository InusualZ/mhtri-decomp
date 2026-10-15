/*
 * g3d/light_obj_is_enable.h - the light-object, light-setting and fog accessors `g3d/g3d_anmscn.cpp` defines and
 *   `g3d/g3d_state.cpp` and `g3d/g3d_scnroot.cpp` call (a leaf header: the owner's full header redefines the scene
 *   types its consumers carry their own views of).
 */
#ifndef MHTRI_G3D_LIGHT_OBJ_IS_ENABLE_H
#define MHTRI_G3D_LIGHT_OBJ_IS_ENABLE_H

#include "types.h"

#ifdef __cplusplus
namespace nw4r {
namespace g3d {
struct LightObj;
struct LightSetting;
struct Fog;
}  // namespace g3d
}  // namespace nw4r

/* 0x8006518C - whether the light object is enabled. */
extern "C" s32 light_obj_is_enable(const nw4r::g3d::LightObj* p);
/* 0x800651B4/0x800651E0 - the light setting's light-object array and its length. */
extern "C" nw4r::g3d::LightObj* light_setting_get_light_obj_array(const nw4r::g3d::LightSetting* p);
extern "C" u32 light_setting_get_num_light_obj(const nw4r::g3d::LightSetting* p);
/* 0x80065264 - the smaller of the two words (`std::min<u32>`). */
extern "C" const u32* g3d_min_u32(const u32* a, const u32* b);
/* 0x800659C4 / 0x80067A54 - whether the fog handle names a record, and that record's address. */
extern "C" s32 fog_is_valid(const nw4r::g3d::Fog* p);
extern "C" u32 fog_ptr(const nw4r::g3d::Fog* p);
#endif

#endif /* MHTRI_G3D_LIGHT_OBJ_IS_ENABLE_H */
