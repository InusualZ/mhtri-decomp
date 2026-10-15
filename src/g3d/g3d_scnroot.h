/* g3d/g3d_scnroot.h - `nw4r::g3d::ScnRoot`, the scene-graph root over ScnGroup: its draw collection, the 32
 *   camera and fog records, the light setting and the bound scene animation.  Its virtuals are defined by
 *   `g3d/g3d_scnroot.cpp`, which emits the vtable. */
#ifndef MHTRI_G3D_G3D_SCNROOT_H
#define MHTRI_G3D_G3D_SCNROOT_H

#include "types.h"

#ifdef __cplusplus
#ifdef MHTRI_G3D_G3D_SCNOBJ_H
#include "g3d/g3d_camera_types.h" /* CameraData (rule 1) */
#endif

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
    /* untyped: opaque handle - the draw collection, the light-object buffers */
    ScnRoot(MEMAllocator* pHeap, void* pCollection, ScnObj** ppChildren, u32 maxChildren, u32 numLightObj,
            u32 numAmbLightObj, void* pLightObjBuf, void* pLightAnmBuf, void* pAmbLightObjBuf);
    virtual ~ScnRoot();
    virtual bool IsDerivedFrom(TypeObj type) const;
    virtual void G3dProc(u32 task, u32 param, void* pInfo); /* untyped: caller-owned payload */
    virtual const TypeObj GetTypeObj() const;
    virtual const char* GetTypeName() const;

    static const TypeObj GetTypeObjStatic();
    static ScnRoot* Construct(MEMAllocator* pHeap, u32* pSize, u32 maxChildren, u32 maxScnObj, u32 numLightObj,
                              u32 numLightSet);

    void SetGlbSettings();
    int GetCamera(int index);
    void SetCurrentCamera(int camera);
    int GetFog(int index);

    /* +0xE8 */ void* mpCollection;       /* untyped: opaque handle - the IScnObjGather draw collection */
    /* +0xEC */ u32 mDrawMode;            /* 2 after construction */
    /* +0xF0 */ u32 mScnRootFlags;        /* 0 after construction */
    /* +0xF4 */ u8 mCurrentCameraID;
    /* +0xF5 */ u8 pad_0xF5[3];
    /* +0xF8 */ CameraData mCamera[32];
    /* +0x2278 */ u8 mFog[32][0x30];      /* the FogData records */
    /* +0x2878 */ u8 mLightSetting[0x10]; /* the LightSetting record */
    /* +0x2888 */ AnmScn* mpAnmScn;
};

/* The draw collector ScnRoot::Construct builds beside the root: the opaque and translucent object lists.  The
 * vtable is this unit's (`.data` 0x8058F640); the destructor (0x8007B794) and Clear (0x8007B9AC) are weak copies in
 * `g3d/g3d_scninline.cpp`'s range.  Add is declared first so the vtable is emitted with it.  size: 0x18 */
class ScnObjGather : public IScnObjGather {
public:
    ScnObjGather(ScnObj** ppBufOpa, ScnObj** ppBufXlu, u32 bufSize);

    virtual CullingStatus Add(ScnObj* pObj, bool opa, bool xlu);
    virtual ~ScnObjGather();
    virtual void Clear();
    virtual void ZSort();
    virtual void Sort();
    virtual void Sort(ScnObjLessThanFunc pOpaFunc, ScnObjLessThanFunc pXluFunc);
    virtual void DrawOpa(u32* pDrawMode);
    virtual void DrawXlu(u32* pDrawMode);
    virtual bool IsUserSorted() const;

    /* +0x04 */ ScnObj** mpArrayOpa;
    /* +0x08 */ ScnObj** mpArrayXlu;
    /* +0x0C */ u32 mSizeScnObj;
    /* +0x10 */ u32 mNumScnObjOpa;
    /* +0x14 */ u32 mNumScnObjXlu;
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

/* A fog record (`ScnRoot::mFog`'s elements, the record a fog handle names): the fields the SetGlbSettings fog pass and
 * the Fog accessor copies read and write (nw4r's FogData layout; +0x1A is what fog_set_range_adj_param stores).
 * size: 0x30 */
struct FogData {
    /* +0x00 */ u32 type;          /* GXFogType: 0 is off, bit 3 the orthographic variant */
    /* +0x04 */ f32 startz;
    /* +0x08 */ f32 endz;
    /* +0x0C */ f32 nearz;
    /* +0x10 */ f32 farz;
    /* +0x14 */ u32 color;
    /* +0x18 */ u8 adjEnable;      /* 1 when the range adjustment is on */
    /* +0x19 */ u8 pad_0x19;
    /* +0x1A */ u16 adjCenter;
    /* +0x1C */ u8 adjTable[0x14];
};

#ifdef __cplusplus
extern "C" {
#endif

void VEC2_ctor(void* p); /* 0x800834F0 - constructs one 8-byte sub-object (ef_particle's parameter record) */
#ifdef __cplusplus
void scn_root_apply_anm_scn(nw4r::g3d::ScnRoot* pRoot); /* 0x8008311C - copies the bound scene animation's light, fog and camera settings into the root */
void scn_root_calc_world(nw4r::g3d::ScnRoot* pRoot); /* 0x80083290 - applies the scene animation, then the world pass */
void scn_root_calc_material(nw4r::g3d::ScnRoot* pRoot); /* 0x800832DC - the material pass */
#endif
u16 math_f32_to_u16(f32 value); /* 0x80082F18 - the frame-round helper (callers: g3d_resanm.c, g3d_resanmchr.cpp) */
s32 scn_root_get_current_camera(s32 model); /* 0x80082BCC - the camera handle lookup (callers: eft019.cpp, em_effect_ctrl.cpp) */

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_G3D_G3D_SCNROOT_H */
