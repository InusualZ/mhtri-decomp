/*
 * g3d/g3d_light.cpp - nw4r g3d's lights: the LightObj copy, clear, GX light initialisers and view transform, the
 *   LightSetting constructor, import and view transform over its light-object, ambient-light and light-set buffers,
 *   and LightSet's light selection.
 * RANGE. .text 0x8007A518-0x8007B2D4 (25 functions); extab 0x80008418-0x800084A0, extabindex 0x80020C4C-0x80020D18,
 *   .data 0x8058EB80-0x8058ED18 (opens on "g3d_light.cpp", read from the LightSetting constructor and LightSet's
 *   selectors), .sdata2 0x80795E30-0x80795E58 (a 0.0 pooled again after `g3d/g3d_fog.cpp`'s 0x80795E28).
 *   `g3d/g3d_init.cpp` precedes it; `g3d/g3d_obj.cpp`'s vtable (0x8058ED18) follows the last string.
 * FLAGS. cflags_g3d (docs/g3d.md); `#pragma peephole off` (playbook idea 106) and `#pragma dont_inline on` (the
 *   inline members and the compiler-generated copies are called out of line, as retail does).
 * NAMES. nw4r's classes and members (LightObj, AmbLightObj, LightSetData, LightSetting, LightSet; the asserts name
 *   G3DState::NUM_LIGHT and the three buffers); the LightSetting constructor and LightObj's inline constructor and
 *   destructor (their kept copies follow the LightSetting constructor) under their C++ manglings.
 *   light_obj_get_light_pos/light_obj_get_light_dir, fn_8007A5A8, fn_8007A5E4 and fn_8007A724 keep C names because
 *   `g3d/g3d_draw.cpp` and `g3d/g3d_anmscn.cpp` call them by those names (nw4r's GetLightPos, GetLightDir,
 *   InitLightPos, InitLightDir and InitSpecularDir).  GUESS: light_obj_enable, light_obj_disable,
 *   light_obj_is_enable, light_set_is_valid, light_setting_get_num_light_obj (`g3d/g3d_anmscn.cpp`'s kept copies of
 *   LightObj::Enable/Disable/IsEnable, LightSet::IsValid and LightSetting::GetNumLightObj).
 * RESIDUALS. The anmscn copies above are declared here until their owner names them (g3d-draw#7..#11).
 * SHAPES. InitLightShininess divides by 2.0f (`* 0.5f` swaps the fmuls operands); the LightSetting constructor
 *   stores its parameters in its initialiser list and declares `white` before `amb` (the two 0xFFFFFFFF pool words
 *   are created in that order); its four LightObj locals get no destructor calls (an inline empty destructor).
 *   `#pragma peephole off` comes before the includes: an inline function compiles under the pragmas at its
 *   definition, and the header's LightObj destructor keeps retail's `extsh` + `cmpwi` deleting-flag test only so.
 */

#pragma peephole off

#include "types.h"
#include "gx.h"
#include "RVLGX/GXLight.h"         /* the GX light initialisers (rule 2) */
#include "mh3_pad.h"               /* VEC3_ctor (rule 2) */
#include "nw4r/math_types.h"       /* mtx34_rotate_vec3 (rule 2) */
#include "nw4r/mtx34_mult_vec3.h"  /* mtx34_mult_vec3 (rule 2) */
#include "nw4r/db_assert.h"        /* nw4r::db::Panic (rule 2) */
#include "g3d/g3d_cpu.h"           /* Copy32ByteBlocks / ZeroMemory32ByteBlocks (rule 2) */
#include "g3d/g3d_pointer_assert.h"
#include "g3d/g3d_light.h"
#include "g3d/light_obj_is_enable.h" /* light_obj_enable/disable/is_enable, light_set_is_valid, light_setting_get_num_light_obj, owner g3d/g3d_anmscn.cpp (rule 2) */
#include "MSL/algorithm.h"         /* std::max */
#include "sys_mem.h"                /* operator delete (rule 2) */

#pragma pool_data off
#pragma dont_inline on


namespace nw4r {
namespace g3d {

/* 0x8007A518 (0x4C): copies the flags and the GX light. */
LightObj& LightObj::operator=(const LightObj& rhs) {
    if (this != &rhs) {
        mFlag = rhs.mFlag;
        detail::Copy32ByteBlocks(&mObj, &rhs.mObj, sizeof(GXLightObj));
    }
    return *this;
}

/* 0x8007A564 (0x14): clears the flags and the GX light. */
void LightObj::Clear() {
    mFlag = 0;
    detail::ZeroMemory32ByteBlocks(&mObj, sizeof(GXLightObj));
}

/* 0x8007A578 (0x30): sets the light's colour. */
void LightObj::InitLightColor(GXColor color) {
    GXInitLightColor(&mObj, color);
}

}  // namespace g3d
}  // namespace nw4r

extern "C" {

/* 0x8007A5A8 (0x3C): sets the light's position and makes it a non-specular light. */
void fn_8007A5A8(void* self, f32 x, f32 y, f32 z) { /* untyped: opaque handle - the LightObj */
    nw4r::g3d::LightObj* pLightObj = (nw4r::g3d::LightObj*)self;
    GXInitLightPos(&pLightObj->mObj, x, y, z);
    pLightObj->mFlag &= ~2;
}

/* 0x8007A5E4 (0x40): sets the light's direction and makes it a directional light. */
void fn_8007A5E4(void* self, f32 x, f32 y, f32 z) { /* untyped: opaque handle - the LightObj */
    nw4r::g3d::LightObj* pLightObj = (nw4r::g3d::LightObj*)self;
    GXInitLightDir(&pLightObj->mObj, x, y, z);
    pLightObj->mFlag = (pLightObj->mFlag & ~2) | 1;
}

}

namespace nw4r {
namespace g3d {

/* 0x8007A624 (0x40): sets the spot cut-off and makes the light directional. */
void LightObj::InitLightSpot(f32 cutoff, u32 spotFn) {
    GXInitLightSpot(&mObj, cutoff, spotFn);
    mFlag = (mFlag & ~2) | 1;
}

/* 0x8007A664 (0x40): sets the angle attenuation and makes the light directional. */
void LightObj::InitLightAttnA(f32 a0, f32 a1, f32 a2) {
    GXInitLightAttnA(&mObj, a0, a1, a2);
    mFlag = (mFlag & ~2) | 1;
}

/* 0x8007A6A4 (0x40): sets the distance attenuation and makes the light directional. */
void LightObj::InitLightDistAttn(f32 refDist, f32 refBright, u32 distAttnFn) {
    GXInitLightDistAttn(&mObj, refDist, refBright, distAttnFn);
    mFlag = (mFlag & ~2) | 1;
}

/* 0x8007A6E4 (0x40): sets the distance attenuation factors and makes the light directional. */
void LightObj::InitLightAttnK(f32 k0, f32 k1, f32 k2) {
    GXInitLightAttnK(&mObj, k0, k1, k2);
    mFlag = (mFlag & ~2) | 1;
}

}  // namespace g3d
}  // namespace nw4r

extern "C" {

/* 0x8007A724 (0x44): sets the specular direction and makes the light specular. */
void fn_8007A724(void* self, f32 x, f32 y, f32 z) { /* untyped: opaque handle - the LightObj */
    nw4r::g3d::LightObj* pLightObj = (nw4r::g3d::LightObj*)self;
    GXInitLightDir(&pLightObj->mObj, x, y, z);
    pLightObj->mFlag = (pLightObj->mFlag & ~1) | 2;
    pLightObj->mFlag |= 8;
}

}

namespace nw4r {
namespace g3d {

#pragma fp_contract off
/* 0x8007A768 (0x60): sets the specular shininess and makes the light specular. */
void LightObj::InitLightShininess(f32 shininess) {
    GXInitLightAttn(&mObj, 0.0f, 0.0f, 1.0f, shininess / 2.0f, 0.0f, 1.0f - shininess / 2.0f);
    mFlag = (mFlag & ~1) | 2;
}
#pragma fp_contract reset

}  // namespace g3d
}  // namespace nw4r

extern "C" {

/* 0x8007A7C8 (0x1C): reads the light's position into `pOut` when it is given. */
void light_obj_get_light_pos(const nw4r::g3d::LightObj* pLightObj, nw4r::math::VEC3* pOut) {
    if (pOut != NULL) {
        GXGetLightPos(&pLightObj->mObj, &pOut->x, &pOut->y, &pOut->z);
    }
}

/* 0x8007A7E4 (0x1C): reads the light's direction into `pOut` when it is given. */
void light_obj_get_light_dir(const nw4r::g3d::LightObj* pLightObj, nw4r::math::VEC3* pOut) {
    if (pOut != NULL) {
        GXGetLightDir(&pLightObj->mObj, &pOut->x, &pOut->y, &pOut->z);
    }
}

}

namespace nw4r {
namespace g3d {

/* 0x8007A800 (0x14): reads the light's colour into `pColor` when it is given. */
void LightObj::GetLightColor(GXColor* pColor) const {
    if (pColor != NULL) {
        GXGetLightColor(&mObj, pColor);
    }
}

/* 0x8007A814 (0xCC): moves the light into view space: the specular direction alone, or the position and the
 * direction. */
void LightObj::ApplyViewMtx(const math::MTX34& camMtx) {
    math::VEC3 dir;
    VEC3_ctor(&dir);
    light_obj_get_light_dir(this, &dir);
    mtx34_rotate_vec3(&dir, &camMtx, &dir);
    if (mFlag & 8) {
        GXInitSpecularDir(&mObj, dir.x, dir.y, dir.z);
    } else {
        math::VEC3 pos;
        VEC3_ctor(&pos);
        light_obj_get_light_pos(this, &pos);
        mtx34_mult_vec3(&pos, &camMtx, &pos);
        GXInitLightPos(&mObj, pos.x, pos.y, pos.z);
        GXInitLightDir(&mObj, dir.x, dir.y, dir.z);
    }
}

}  // namespace g3d
}  // namespace nw4r

namespace nw4r {
namespace g3d {

/* 0x8007A8E0 (0x618): stores the buffers and counts, clears every light set, selects the first two lights in light
 * set 0, and fills the lights with the defaults: a spot light, a point light, a specular light, then disabled
 * lights, each with a white ambient light. */
LightSetting::LightSetting(LightObj* pLightObjArray, AmbLightObj* pAmbLightObjArray, u32 numLight,
                           LightSetData* pLightSetDataArray, u32 numLightSet)
    : mNumLight(numLight), mNumLightSet(numLightSet), mpLightObjArray(pLightObjArray),
      mpAmbLightObjArray(pAmbLightObjArray), mpLightSetDataArray(pLightSetDataArray) {
    bool countsFit = (u16)numLight <= 128 && (u16)numLightSet <= 128;
    if (!countsFit) {
        nw4r::db::Panic("g3d_light.cpp", 0x134,
                        "NW4R:Failed assertion mNumLight <= G3DState::NUM_LIGHT && mNumLightSet <= "
                        "G3DState::NUM_LIGHT_SET");
    }
    bool countsSet = mNumLight > 0 && mNumLightSet > 0;
    if (!countsSet) {
        nw4r::db::Panic("g3d_light.cpp", 0x135, "NW4R:Failed assertion mNumLight > 0 && mNumLightSet > 0");
    }
    G3D_POINTER_ASSERT("g3d_light.cpp", pLightObjArray, 0x137,
                       "NW4R:Pointer Error\npLightObjArray(=%p) is not valid pointer.");
    G3D_POINTER_ASSERT("g3d_light.cpp", pAmbLightObjArray, 0x138,
                       "NW4R:Pointer Error\npAmbLightObjArray(=%p) is not valid pointer.");
    G3D_POINTER_ASSERT("g3d_light.cpp", pLightSetDataArray, 0x139,
                       "NW4R:Pointer Error\npLightSetDataArray(=%p) is not valid pointer.");

    for (u32 i = 0; i < mNumLightSet; i++) {
        LightSetData& data = mpLightSetDataArray[i];
        data.idxAmbLight = -1;
        data.idxLight[0] = -1;
        data.idxLight[1] = -1;
        data.idxLight[2] = -1;
        data.idxLight[3] = -1;
        data.idxLight[4] = -1;
        data.idxLight[5] = -1;
        data.idxLight[6] = -1;
        data.idxLight[7] = -1;
    }
    mpLightSetDataArray[0].idxLight[0] = 0;
    mpLightSetDataArray[0].idxLight[1] = 2;
    mpLightSetDataArray[0].idxAmbLight = -1;

    GXColor white = {0xFF, 0xFF, 0xFF, 0xFF};
    AmbLightObj amb = {0xFF, 0xFF, 0xFF, 0xFF};
    LightObj spotLight;
    LightObj pointLight;
    LightObj specLight;
    LightObj offLight;
    spotLight.Clear();
    pointLight.Clear();
    specLight.Clear();
    offLight.Clear();

    spotLight.InitLightColor(white);
    fn_8007A5A8(&spotLight, 4000000.0f, 4000000.0f, 4000000.0f);
    fn_8007A5E4(&spotLight, 0.0f, -1.0f, 0.0f);
    spotLight.InitLightSpot(90.0f, 0);
    spotLight.InitLightDistAttn(10.0f, 0.5f, 0);
    light_obj_enable(&spotLight);

    pointLight.InitLightColor(white);
    fn_8007A5A8(&pointLight, 4000000.0f, 4000000.0f, 4000000.0f);
    light_obj_enable(&pointLight);

    specLight.InitLightColor(white);
    fn_8007A5A8(&specLight, 4000000.0f, 4000000.0f, 4000000.0f);
    fn_8007A724(&specLight, 0.0f, -1.0f, 0.0f);
    specLight.InitLightShininess(16.0f);
    light_obj_enable(&specLight);

    light_obj_disable(&offLight);

    if (mNumLight > 0) {
        mpLightObjArray[0] = spotLight;
        mpAmbLightObjArray[0] = amb;
    }
    if (mNumLight > 1) {
        mpLightObjArray[1] = pointLight;
        mpAmbLightObjArray[1] = amb;
    }
    if (mNumLight > 2) {
        mpLightObjArray[2] = specLight;
        mpAmbLightObjArray[2] = amb;
    }
    for (u32 i = 3; i < mNumLight; i++) {
        mpLightObjArray[i] = offLight;
        mpAmbLightObjArray[i] = amb;
    }
}

/* 0x8007AF6C (0x108): copies another setting's lights, ambient lights and light sets when they fit; a disabled
 * light disables this setting's light instead of being copied. */
bool LightSetting::Import(const LightSetting& rhs) {
    if (mNumLight < rhs.mNumLight || mNumLightSet < rhs.mNumLightSet) {
        return false;
    }
    u32 i;
    for (i = 0; i < rhs.mNumLight; i++) {
        if (light_obj_is_enable(&rhs.mpLightObjArray[i])) {
            mpLightObjArray[i] = rhs.mpLightObjArray[i];
        } else {
            light_obj_disable(&mpLightObjArray[i]);
        }
        mpAmbLightObjArray[i] = rhs.mpAmbLightObjArray[i];
    }
    for (i = 0; i < rhs.mNumLightSet; i++) {
        mpLightSetDataArray[i] = rhs.mpLightSetDataArray[i];
    }
    return true;
}

/* 0x8007B0A0 (0xA8): moves the first `numLight` enabled lights into view space. */
void LightSetting::ApplyViewMtx(const math::MTX34& camMtx, u32 numLight) {
    numLight = std::max(numLight, (u32)mNumLight);
    for (u32 i = 0; i < numLight; i++) {
        if (light_obj_is_enable(&mpLightObjArray[i])) {
            mpLightObjArray[i].ApplyViewMtx(camMtx);
        }
    }
}

/* 0x8007B160 (0xC4): selects light object `lightObjIdx` (-1: none) for slot `lightIdxInSet` of the set. */
bool LightSet::SelectLightObj(u32 lightIdxInSet, int lightObjIdx) {
    if (!light_set_is_valid(this)) {
        nw4r::db::Panic("g3d_light.cpp", 0x1DC, "NW4R:Failed assertion IsValid()");
    }
    if (light_set_is_valid(this) && lightIdxInSet < 8) {
        if (lightObjIdx < 0) {
            mpLightSetData->idxLight[lightIdxInSet] = -1;
            return true;
        } else if (lightObjIdx < (int)light_setting_get_num_light_obj(mpSetting)) {
            mpLightSetData->idxLight[lightIdxInSet] = lightObjIdx;
            return true;
        }
    }
    return false;
}

/* 0x8007B224 (0xB0): selects ambient light `ambLightObjIdx` (-1: none) for the set. */
bool LightSet::SelectAmbLightObj(int ambLightObjIdx) {
    if (!light_set_is_valid(this)) {
        nw4r::db::Panic("g3d_light.cpp", 0x1F2, "NW4R:Failed assertion IsValid()");
    }
    if (light_set_is_valid(this)) {
        if (ambLightObjIdx < 0) {
            mpLightSetData->idxAmbLight = -1;
            return true;
        } else if (ambLightObjIdx < (int)light_setting_get_num_light_obj(mpSetting)) {
            mpLightSetData->idxAmbLight = ambLightObjIdx;
            return true;
        }
    }
    return false;
}

}  // namespace g3d
}  // namespace nw4r
