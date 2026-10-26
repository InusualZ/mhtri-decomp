/* Leaf header (docs/plan.md 6.5 rule 2): `g3d/g3d_anmscn.cpp`'s ambient-light array accessor. */
#ifndef MHTRI_G3D_LIGHT_SETTING_GET_AMB_LIGHT_OBJ_ARRAY_H
#define MHTRI_G3D_LIGHT_SETTING_GET_AMB_LIGHT_OBJ_ARRAY_H

#include "types.h"

#ifdef __cplusplus
namespace nw4r {
namespace g3d {
class LightSetting;
struct AmbLightObj;
}  // namespace g3d
}  // namespace nw4r

/* 0x800651E8 - the light setting's ambient-light array (LightSetting::GetAmbLightObjArray). */
extern "C" nw4r::g3d::AmbLightObj* light_setting_get_amb_light_obj_array(const nw4r::g3d::LightSetting* p);
#endif

#endif /* MHTRI_G3D_LIGHT_SETTING_GET_AMB_LIGHT_OBJ_ARRAY_H */
