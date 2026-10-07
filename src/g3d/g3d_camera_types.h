/* g3d/g3d_camera_types.h - `nw4r::g3d::Camera`, a one-word `ResCommon<CameraData>` handle whose members
 *   `SetPosition`/`SetPosture`/`SetPerspective`/`GetCameraMtx` `g3d/g3d_camera.cpp` defines. */
#ifndef MHTRI_G3D_G3D_CAMERA_TYPES_H
#define MHTRI_G3D_G3D_CAMERA_TYPES_H

#include "types.h"
#include "nw4r/math.h"

#ifdef __cplusplus

/* The camera payload `Camera::mpData` points at (`g3d_camera.cpp`'s `CameraData`). The 0x70 flag word
 * records which posture/projection branch is current. */
struct CameraData {
    /* +0x00 */ f32 mViewMtx[3][4];
    /* +0x30 */ f32 mProjMtx[3][4];
    /* +0x60 */ u32 mUnk60[4];
    /* +0x70 */ u32 mFlags;
    /* +0x74 */ f32 mPosX;
    /* +0x78 */ f32 mPosY;
    /* +0x7C */ f32 mPosZ;
    /* +0x80 */ f32 mTargetX;
    /* +0x84 */ f32 mTargetY;
    /* +0x88 */ f32 mTargetZ;
    /* +0x8C */ f32 mUpX;
    /* +0x90 */ f32 mUpY;
    /* +0x94 */ f32 mUpZ;
    /* +0x98 */ f32 mUnk98;
    /* +0x9C */ f32 mUnk9C;
    /* +0xA0 */ f32 mUnkA0;
    /* +0xA4 */ f32 mUnkA4;
    /* +0xA8 */ s32 mProjType;
    /* +0xAC */ f32 mProjA;
    /* +0xB0 */ f32 mProjB;
    /* +0xB4 */ f32 mProjC;
    /* +0xB8 */ f32 mProjD;
    /* +0xBC */ f32 mUnkBC;
    /* +0xC0 */ f32 mUnkC0;
    /* +0xC4 */ f32 mUnkC4;
    /* +0xC8 */ f32 mUnkC8;
    /* +0xCC */ f32 mUnkCC;
    /* +0xD0 */ f32 mUnkD0;
    /* +0xD4 */ f32 mUnkD4;
    /* +0xD8 */ f32 mUnkD8;
    /* +0xDC */ f32 mViewportX;
    /* +0xE0 */ f32 mViewportY;
    /* +0xE4 */ f32 mViewportW;
    /* +0xE8 */ f32 mViewportH;
    /* +0xEC */ f32 mViewportNear;
    /* +0xF0 */ f32 mViewportFar;
    /* +0xF4 */ s32 mScissorX;
    /* +0xF8 */ s32 mScissorY;
    /* +0xFC */ s32 mScissorW;
    /* +0x100 */ s32 mScissorH;
    /* +0x104 */ s32 mScissorOffsetX;
    /* +0x108 */ s32 mScissorOffsetY;
}; /* size: 0x10C */

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
