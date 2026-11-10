/* g3d/g3d_camera_types.h - `nw4r::g3d::Camera`, a one-word `ResCommon<CameraData>` handle whose members
 *   `SetPosition`/`SetPosture`/`SetPerspective`/`GetCameraMtx` `g3d/g3d_camera.cpp` defines. */
#ifndef MHTRI_G3D_G3D_CAMERA_TYPES_H
#define MHTRI_G3D_G3D_CAMERA_TYPES_H

#include "types.h"
#include "nw4r/math.h"

#ifdef __cplusplus

/* The camera payload `Camera::mpData` points at (`g3d_camera.cpp`'s `CameraData`). The 0x70 flag word
 * records which posture/projection branch is current.  The constructor (`g3d/g3d_scnroot.cpp`, inline) runs the
 * matrix and vector members' constructors. */
struct CameraData {
    CameraData();

    /* +0x00 */ nw4r::math::MTX34 mViewMtx;
    /* +0x30 */ nw4r::math::MTX44 mProjMtx;
    /* +0x70 */ u32 mFlags;
    /* +0x74 */ nw4r::math::VEC3 mPos;
    /* +0x80 */ nw4r::math::VEC3 mTarget;
    /* +0x8C */ nw4r::math::VEC3 mUp;
    /* +0x98 */ nw4r::math::VEC3 mUnk98;
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
    /* +0xDC */ nw4r::math::VEC2 mViewportPos;  /* x, y */
    /* +0xE4 */ nw4r::math::VEC2 mViewportSize; /* width, height */
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
        /* +0x04 */ nw4r::math::VEC3 mPos;
        /* +0x10 */ nw4r::math::VEC3 mTarget;
        /* +0x1C */ nw4r::math::VEC3 mUp;
        /* +0x28 */ f32 mUnk28;

        /* 0x80067E70 (0x44): retail's out-of-line constructor (its three vectors). */
        PostureInfo() __attribute__((never_inline)) {}
    }; /* size: 0x2C */

    void SetPosition(const math::VEC3& rPos);
    void SetPosture(const PostureInfo& rInfo);
    void SetPerspective(f32 fovy, f32 aspect, f32 near, f32 far);
    /* 0x80074F24 - the orthographic projection (`g3d/g3d_anmscn.cpp`'s GetCamera calls it). */
    void SetOrtho(f32 top, f32 bottom, f32 left, f32 right, f32 near, f32 far);
    void GetCameraMtx(math::MTX34* pMtx) const;

    /* +0x0 */ CameraData* mpData;
}; /* size: 0x4 */

}  // namespace g3d
}  // namespace nw4r

#endif /* __cplusplus */

#endif /* MHTRI_G3D_G3D_CAMERA_TYPES_H */
