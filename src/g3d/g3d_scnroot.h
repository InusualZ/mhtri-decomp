/* g3d/g3d_scnroot.h - `nw4r::g3d::ScnRoot`, the scene-graph root over ScnGroup: its draw collection, the 32
 *   camera and fog records, the light setting and the bound scene animation.  Its virtuals are defined by
 *   `g3d/g3d_scnroot.cpp`, which emits the vtable. */
#ifndef MHTRI_G3D_G3D_SCNROOT_H
#define MHTRI_G3D_G3D_SCNROOT_H

#include "types.h"

#ifdef __cplusplus
namespace nw4r {
namespace g3d {

class AnmScn;

/* The scene-graph root.  size: 0x288C (the Construct block's first allocation)
 * The full class (base, virtuals, fields) is visible only where `g3d/g3d_scnobj.h` was included first (the owner
 * does); a consumer that cannot include the scene-object headers sees the non-virtual camera and fog accessors
 * only, which mangle the same. */
#ifdef MHTRI_G3D_G3D_SCNOBJ_H
class ScnRoot : public ScnGroup {
public:
    virtual ~ScnRoot();
    virtual bool IsDerivedFrom(TypeObj type) const;
    virtual void G3dProc(u32 task, u32 param, void* pInfo); /* untyped: caller-owned payload */
    virtual const TypeObj GetTypeObj() const;
    virtual const char* GetTypeName() const;

    static const TypeObj GetTypeObjStatic();

    int GetCamera(int index);
    void SetCurrentCamera(int camera);
    int GetFog(int index);

    /* +0xE8 */ void* mpCollection;       /* untyped: opaque handle - the IScnObjGather draw collection */
    /* +0xEC */ u32 mDrawMode;            /* 2 after construction */
    /* +0xF0 */ u32 mScnRootFlags;        /* 0 after construction */
    /* +0xF4 */ u8 mCurrentCameraID;
    /* +0xF5 */ u8 pad_0xF5[3];
    /* +0xF8 */ u8 mCamera[32][0x10C];    /* the CameraData records */
    /* +0x2278 */ u8 mFog[32][0x30];      /* the FogData records */
    /* +0x2878 */ u8 mLightSetting[0x10]; /* the LightSetting record */
    /* +0x2888 */ AnmScn* mpAnmScn;
};
#else
/* size: 0x288C (this view names only the accessors) */
class ScnRoot {
public:
    int GetCamera(int index);
    void SetCurrentCamera(int camera);
    int GetFog(int index);
};
#endif

}  // namespace g3d
}  // namespace nw4r
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8056F6D0 - the "ScnRoot" type-name record (`.rodata`: a length word, then the NUL-terminated name) the class's
 * run-time type members read. */
extern const char scn_typename_ScnRoot[];
void VEC2_ctor(void* p); /* 0x800834F0 - constructs one 8-byte sub-object (ef_particle's parameter record) */
u16 fn_80082F18(f32 value); /* 0x80082F18 - the frame-round helper (callers: g3d_resanm.c, g3d_resanmchr.cpp) */
s32 scn_root_get_current_camera(s32 model); /* 0x80082BCC - the camera handle lookup (callers: eft019.cpp, em_effect_ctrl.cpp) */

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_G3D_G3D_SCNROOT_H */
