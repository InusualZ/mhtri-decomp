/*
 * g3d/g3d_resanmlight.h - declarations of the symbols owned by `g3d/g3d_resanmlight.cpp` that other units call.
 */
#ifndef G3D_G3D_RESANMLIGHT_H
#define G3D_G3D_RESANMLIGHT_H

#include "types.h"

#ifdef __cplusplus
struct ResHandle;

namespace nw4r {
namespace g3d {
struct LightAnmResult;
}  // namespace g3d
}  // namespace nw4r

/* Evaluates the light animation at `frame` into `pResult` (caller: g3d_anmscn.cpp). */
extern "C" void res_anm_light_get_anm_result(const ResHandle* pSelf, nw4r::g3d::LightAnmResult* pResult, f32 frame);
#endif

#endif /* G3D_G3D_RESANMLIGHT_H */
