/*
 * g3d/g3d_resanmfog.h - declarations of the symbols owned by `g3d/g3d_resanmfog.cpp` that other units call.
 */
#ifndef G3D_G3D_RESANMFOG_H
#define G3D_G3D_RESANMFOG_H

#include "types.h"

#ifdef __cplusplus
struct ResHandle;

namespace nw4r {
namespace g3d {
struct FogAnmResult;
}  // namespace g3d
}  // namespace nw4r

/* Evaluates the fog animation at `frame` into `pResult` (caller: g3d_anmscn.cpp). */
extern "C" void res_anm_fog_get_anm_result(const ResHandle* pSelf, nw4r::g3d::FogAnmResult* pResult, f32 frame);
#endif

#endif /* G3D_G3D_RESANMFOG_H */
