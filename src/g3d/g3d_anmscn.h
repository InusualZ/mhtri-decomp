/* g3d/g3d_anmscn.h - the cross-unit declarations of `g3d/g3d_anmscn.cpp` (C linkage, plain map stems), except
 *   `fn_80066C8C`, which keeps C++ linkage outside the `extern "C"` block (map row `fn_80066C8C__FPv`). */
#ifndef MHTRI_G3D_G3D_ANMSCN_H
#define MHTRI_G3D_G3D_ANMSCN_H

#include "types.h"
#include "g3d/g3d_camera_types.h" /* `nw4r::g3d::Camera::PostureInfo`, the object `camera_posture_info_ctor` initialises */

#ifdef __cplusplus
extern "C" {
#endif

/* The cluster's owns with plain `fn_XXXXXXXX` map names, so C linkage. */
/* 0x80064C14 - releases the object the second argument points at (`light/light.cpp`'s fn_802C1AD8 tail-calls it). */
void fn_80064C14(void *self, const f32 *v);
u32 fn_800651BC(void *self);                /* 0x800651BC - reads the word at +0x0 of `self` */
void *vec3_copy_construct(void *out, void *in);     /* 0x80067E54 - copies a VEC3 (three floats), returns `out` */
s32 fn_80067EE8(const void *p);             /* 0x80067EE8 - `*(u32*)p != 0` */
#ifdef __cplusplus
/* 0x80067E70 - the posture record's constructor: default-constructs its position, target and up vectors. */
nw4r::g3d::Camera::PostureInfo *camera_posture_info_ctor(nw4r::g3d::Camera::PostureInfo *self);
#endif

#ifdef __cplusplus
}
#endif

/* The one real C++ free function of the cluster: the front-end mangles this to `fn_80066C8C__FPv`. */
void *fn_80066C8C(void *obj);

/* The resolved-resource records `fn_8006584C`/`fn_80066E80` return, shared with `g3d/g3d_resanmlight.cpp`: the
 * fields both units read; each record continues past its last field, so the sizes are lower bounds. */
typedef struct {
    /* +0x00 */ u8 pad_0x00[0x10];
    /* +0x10 */ u32 field_0x10;
    /* +0x14 */ u8 pad_0x14[0x20];
    /* +0x34 */ u16 field_0x34;
    /* +0x36 */ u16 field_0x36;
    /* +0x38 */ u32 field_0x38;
    /* +0x3C */ u16 field_0x3C;
    /* +0x3E */ u16 field_0x3E;
    /* +0x40 */ u16 field_0x40;
    /* +0x42 */ u16 field_0x42;
    /* +0x44 */ u16 field_0x44;
} ResAnmScnConfig; /* size: 0x46 (approximate - the record continues past +0x44) */

typedef struct {
    /* +0x00 */ u8 pad_0x00[0x04];
    /* +0x04 */ u32 field_0x04;      /* sub-resource offset resolved against the object base */
    /* +0x08 */ u8 pad_0x08[0x08];
    /* +0x10 */ u32 field_0x10;
    /* +0x14 */ u32 field_0x14;      /* the type word copied verbatim into the result */
    /* +0x18 */ u16 field_0x18;
    /* +0x1A */ u8 field_0x1A;
    /* +0x1B */ u8 pad_0x1B;
    /* +0x1C */ u32 flags;           /* per-channel "inline value" bits */
    /* +0x20 */ u32 dataOffset;      /* base of the packed channel array */
    /* +0x24 */ u32 channel_0x24;
    /* +0x28 */ u32 channel_0x28;
    /* +0x2C */ u32 channel_0x2C;
    /* +0x30 */ u32 channel_0x30;
    /* +0x34 */ u32 channel_0x34;
    /* +0x38 */ u32 channel_0x38;
    /* +0x3C */ u32 channel_0x3C;
    /* +0x40 */ u32 field_0x40;
    /* +0x44 */ u32 channel_0x44;
    /* +0x48 */ u32 channel_0x48;
    /* +0x4C */ u32 field_0x4C;
    /* +0x50 */ u32 channel_0x50;
    /* +0x54 */ u32 channel_0x54;
    /* +0x58 */ u32 channel_0x58;
} ResAnmLightConfig; /* size: 0x5C (approximate - the record continues past +0x58) */

#ifdef __cplusplus
extern "C" {
#endif

ResAnmScnConfig *fn_8006584C(void *p);   /* 0x8006584C - the checked `ResAnmScn` config getter */
ResAnmLightConfig *fn_80066DB4(void *p); /* 0x80066DB4 - the checked light-config getter */
ResAnmLightConfig *fn_80066E80(void *p); /* 0x80066E80 - the checked light-config getter */

/* 0x80068634 - the checked `ResAnmFog` resolver: panics when the handle's reference is invalid, then returns the
 * body word at +0x0 (the resolved data pointer; caller: g3d_resanmfog.cpp). */
u32 fn_80068634(void* p);
/* 0x800689B0 - the checked `ResAnmCamera` resolver (caller: g3d_resanmcamera.cpp): returns the resolved data
 * pointer. */
u32 fn_800689B0(void* p);

#ifdef __cplusplus
}

/* 0x80065804 - the placement `operator delete` (a bare `blr`); the landing pad of a placement `new` whose
 * constructor throws calls it. */
/* untyped: opaque handle passed through - the block and the placement address */
void operator delete(void* block, void* place);

#include "gx.h"
#include "nw4r/math.h"
#include "nw4r/g3d/res_common.h" /* ResHandle */
#include "g3d/g3d_obj.h"         /* AnmScn, FrameCtrl */

namespace nw4r {
namespace g3d {

/* The ambient light's evaluated result: which channels it animates and the colour.  size: 0x8 */
struct AmbLightAnmResult {
    enum {
        FLAG_COLOR_ENABLE = (1 << 0),
        FLAG_ALPHA_ENABLE = (1 << 1)
    };

    /* +0x0 */ u32 flags;
    /* +0x4 */ u32 color;
};

/* A light's evaluated result: the type and enable bits, the position and aim, the colour, the distance and spot
 * attenuation, and the specular colour and shininess.  size: 0x40 (the cache stride and Construct's multiplier) */
struct LightAnmResult {
    enum {
        FLAG_LIGHT_TYPE_MASK = 3,
        FLAG_LIGHT_ENABLE = (1 << 2),
        FLAG_SPECULAR_ENABLE = (1 << 3),
        FLAG_COLOR_ENABLE = (1 << 4),
        FLAG_ALPHA_ENABLE = (1 << 5)
    };

    /* +0x00 */ u32 specIdx;
    /* +0x04 */ u32 flags;
    /* +0x08 */ math::VEC3 pos;
    /* +0x14 */ math::VEC3 aim;
    /* +0x20 */ GXColor color;
    /* +0x24 */ s32 distFunc;
    /* +0x28 */ f32 refDistance;
    /* +0x2C */ f32 refBrightness;
    /* +0x30 */ s32 spotFunc;
    /* +0x34 */ f32 cutoff;
    /* +0x38 */ GXColor specColor;
    /* +0x3C */ f32 shininess;
};

/* A fog's evaluated result: the fog type, the z range and the colour.  size: 0x10 */
struct FogAnmResult {
    /* +0x0 */ s32 type;
    /* +0x4 */ f32 startz;
    /* +0x8 */ f32 endz;
    /* +0xC */ GXColor color;
};

/* A camera's evaluated result: the camera-type and existence bits, the projection, the position, the aspect and
 * clip planes, the rotation or aim (the camera type picks which), the twist and the field of view or ortho
 * height.  size: 0x34 */
struct CameraAnmResult {
    enum {
        FLAG_CAMERA_TYPE_MASK = 1,
        FLAG_ANM_EXISTS = (1 << 1)
    };

    /* +0x00 */ u32 flags;
    /* +0x04 */ s32 projType;
    /* +0x08 */ math::VEC3 pos;
    /* +0x14 */ f32 aspect;
    /* +0x18 */ f32 near;
    /* +0x1C */ f32 far;
    /* +0x20 */ math::VEC3 rotate;
    /* +0x2C */ f32 twist;
    /* +0x30 */ f32 perspFovy;
};

/* The scene animation over a ResAnmScn resource, with optional result caches.  size: 0x38 (Construct's
 * `new (pBuf)` size) */
class AnmScnRes : public AnmScn, protected FrameCtrl {
public:
    /* The bits of mScnAnmFlags. */
    enum ScnAnmFlag {
        FLAG_HAS_CACHE = (1 << 0)
    };

    static AnmScnRes* Construct(MEMAllocator* pHeap, u32* pSize, ResHandle resAnmScn, bool bHasCache);
    AnmScnRes(MEMAllocator* pHeap, ResHandle resAnmScn, AmbLightAnmResult* pAmbLightCache,
              LightAnmResult* pLightCache, FogAnmResult* pFogCache, CameraAnmResult* pCameraCache);

    virtual bool IsDerivedFrom(TypeObj type) const;
    virtual void G3dProc(u32 task, u32 param, void* pInfo); /* untyped: caller-owned payload */
    virtual ~AnmScnRes();
    virtual const TypeObj GetTypeObj() const;
    virtual const char* GetTypeName() const;

    virtual void SetFrame(f32 frame);
    virtual f32 GetFrame() const;
    virtual void SetUpdateRate(f32 rate);
    virtual f32 GetUpdateRate() const;
    virtual void UpdateFrame();
    virtual u32 GetNumLightSet() const;
    virtual u32 GetNumAmbLight() const;
    virtual u32 GetNumDiffuseLight() const;
    virtual u32 GetNumSpecularLight() const;
    virtual u32 GetNumFog() const;
    virtual u32 GetNumCamera() const;
    virtual u32 GetLightSetMaxRefNumber() const;
    virtual u32 GetAmbLightMaxRefNumber() const;
    virtual u32 GetDiffuseLightMaxRefNumber() const;
    virtual u32 GetFogMaxRefNumber() const;
    virtual u32 GetCameraMaxRefNumber() const;
    virtual bool GetLightSet(LightSet& set, u32 refNumber);
    virtual ut::Color GetAmbLightColor(u32 refNumber);
    virtual void GetLight(LightObj* pDiff, LightObj* pSpec, u32 refNumber);
    virtual void GetFog(Fog fog, u32 refNumber);
    virtual bool GetCamera(Camera camera, u32 refNumber);
    virtual u32 GetSpecularLightID(u32 refNumber) const;
    virtual bool HasSpecularLight(u32 refNumber) const;
    virtual AmbLightAnmResult* GetAmbLightResult(AmbLightAnmResult* pResult, u32 refNumber);
    virtual LightAnmResult* GetLightResult(LightAnmResult* pResult, u32 refNumber);
    virtual FogAnmResult* GetFogResult(FogAnmResult* pResult, u32 refNumber);
    virtual CameraAnmResult* GetCameraResult(CameraAnmResult* pResult, u32 refNumber);

    void SetScnAnmFlag(ScnAnmFlag flag, bool on);
    bool TestScnAnmFlag(ScnAnmFlag flag) const;
    void UpdateCache();
    static const TypeObj GetTypeObjStatic();

    /* +0x00 vtable, +0x04 G3dObj's parent and heap, +0x0C FrameCtrl */
    /* +0x20 */ ResHandle mRes;
    /* +0x24 */ u32 mScnAnmFlags;
    /* +0x28 */ AmbLightAnmResult* mpAmbLightCache;
    /* +0x2C */ LightAnmResult* mpLightCache;
    /* +0x30 */ FogAnmResult* mpFogCache;
    /* +0x34 */ CameraAnmResult* mpCameraCache;
};

}  // namespace g3d
}  // namespace nw4r
#endif

#endif /* MHTRI_G3D_G3D_ANMSCN_H */

