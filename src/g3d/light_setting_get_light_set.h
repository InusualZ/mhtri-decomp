/* Leaf header (docs/plan.md 6.5 rule 2): `g3d/g3d_anmscn.cpp`'s light-set accessor. */
#ifndef MHTRI_G3D_LIGHT_SETTING_GET_LIGHT_SET_H
#define MHTRI_G3D_LIGHT_SETTING_GET_LIGHT_SET_H

#include "types.h"

#ifdef __cplusplus
namespace nw4r {
namespace g3d {
class LightSetting;
class LightSet;
}  // namespace g3d
}  // namespace nw4r

/* 0x8006522C - writes the light setting's light set `index` into `out` (LightSetting::GetLightSet). */
extern "C" void light_setting_get_light_set(nw4r::g3d::LightSet* out, nw4r::g3d::LightSetting* self, s32 index);
#endif

#endif /* MHTRI_G3D_LIGHT_SETTING_GET_LIGHT_SET_H */
