/* g3d/g3d_camera_types.h - `nw4r::g3d::Camera`, a one-word `ResCommon<CameraData>` handle whose members
 *   `SetPosition`/`SetPosture`/`SetPerspective`/`GetCameraMtx` `g3d/g3d_camera.cpp` defines. */
#ifndef MHTRI_G3D_G3D_CAMERA_TYPES_H
#define MHTRI_G3D_G3D_CAMERA_TYPES_H

#include "types.h"
#include "nw4r/math.h"

#ifdef __cplusplus

struct CameraData;

namespace nw4r {
namespace g3d {

class Camera {
public:
    struct PostureInfo {
        /* +0x00 */ s32 mType;
        /* +0x04 */ f32 mPosX;
        /* +0x08 */ f32 mPosY;
        /* +0x0C */ f32 mPosZ;
        /* +0x10 */ f32 mTargetX;
        /* +0x14 */ f32 mTargetY;
        /* +0x18 */ f32 mTargetZ;
        /* +0x1C */ f32 mUpX;
        /* +0x20 */ f32 mUpY;
        /* +0x24 */ f32 mUpZ;
        /* +0x28 */ f32 mUnk28;
    }; /* size: 0x2C */

    void SetPosition(const math::VEC3& rPos);
    void SetPosture(const PostureInfo& rInfo);
    void SetPerspective(f32 fovy, f32 aspect, f32 near, f32 far);
    void GetCameraMtx(math::MTX34* pMtx) const;

    /* +0x0 */ CameraData* mpData;
}; /* size: 0x4 */

}  // namespace g3d
}  // namespace nw4r

#endif /* __cplusplus */

#endif /* MHTRI_G3D_G3D_CAMERA_TYPES_H */
