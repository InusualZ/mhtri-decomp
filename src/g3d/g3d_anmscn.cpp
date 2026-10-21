/*
 * g3d/g3d_anmscn.cpp - nw4r g3d `g3d_anmscn.cpp`: the scene animation (`AnmScn`, `AnmScnRes`): the light-object
 *   builders, GetLightSetting, Construct and the AnmScnRes interface over a `ResAnmScn` resource with its result
 *   caches, plus the out-of-line copies of the `LightObj`/`LightSetting`/`LightSet`, `ResAnmScn`, `ResLightSet`,
 *   `ResAnmAmbLight`/`ResAnmLight`/`ResAnmFog`/`ResAnmCamera` and `Fog` accessors it calls.
 * RANGE. .text 0x800649CC-0x8006946C; extab 0x800078F4-0x80007BB4, extabindex 0x8001FC98-0x8002004C, .rodata
 *   0x8056F578-0x8056F598, .data 0x8058C288-0x8058CC40, .sdata 0x80791170-0x80791188, .sdata2 0x80795D70-0x80795D90.
 *   The TU is `g3d_anmscn.cpp`: its one-copy `__FILE__` string (0x8058C288) is cited from 0x800649CC through
 *   0x80068FC4; the previous TU's (`g3d_anmobj.cpp`, 0x8058C248) by PlayPolicy_Loop and the next TU's
 *   (`g3d_anmshp.cpp`, 0x8058CC40) from 0x8006946C on.
 * FLAGS. `cflags_g3d`'s `-O3` is measured on fn_800680A8 (docs/g3d.md, playbook 27).
 * NAMES. anmscn_round_up (0x80065ED8, Construct's align-up: nw4r's ut::RoundUp shape) is a GUESS.
 *   The AnmScn/AnmScnRes members are nw4r's (the vtables at 0x8058C808/0x8058C898 fix the slot order, the
 *   assert strings name Attach, Detach, GetParent, TestScnAnmFlag(FLAG_HAS_CACHE) and Construct's `pSize`/`pHeap`/
 *   `buf` checks).  GUESS: the count/limit slot names (GetNum*, Get*MaxRefNumber, from the ResAnmScn reader each
 *   forwards to), MakeSpecularLightObj (MakeDiffuseLightObj's specular twin), vec3_copy_construct,
 *   obj_delete_flagged (the LightSet view's destructor), fog_set_color, res_anm_light_config_test_flag8,
 *   res_anm_light_get_id, res_anm_fog_get_id, res_anm_camera_get_id.  The weak accessor copies keep their map stems
 *   (C linkage); fn_80066C8C and fn_800680A8 keep C++ linkage (`fn_80066C8C__FPv`, `fn_800680A8__FPv`, playbook 50).
 * RESIDUALS. The ambient colours convert through ut::Color's `operator u32` weak copy (`__opUl__Q34nw4r2ut5ColorCFv`,
 *   the map's fn_800651BC); fn_800651BC stays defined as the C stem `g3d/g3d_resanm.cpp` calls.
 *   fn_8006522C: retail's ternary join branch.
 *   Data: the `.data` strings are not in `.text` order in retail (Construct's follow G3dProc's, GetLightSet's follow
 *   GetFog's); the `_ac.h` inline tail (0x8058C928-0x8058CC40, its "ref" copies in `.sdata`) is still extern; the
 *   weak `ut::Color` constructor/destructor copies MWCC emits here are retail's dead copies (linker-folded).
 * SHAPES. GetLightSetting copies the diffuse count into a register local before the light-object calls: retail adds
 *   that copy (r28) to the specular count; the address-taken count itself is reloaded after the virtual call.
 * SHAPES. The round-up helper takes `unsigned int align` (nw4r's ut::RoundUp signature): with a u32 alignment MWCC
 *   folds `~(align - 1)` into `andc` where retail keeps `nor` + `and`.
 * SHAPES. `#pragma peephole off` (retail keeps the unfused `clrlwi` + `cmpwi`, `extsh`, `addi r0` vtable-store and
 *   `mr r3` + `lwz r12,0(r3)` virtual-call forms; playbook idea 106) and `#pragma pool_data off` (each string its
 *   own `lis`/`addi`).  GetLightSetting and GetAmbLightColor compile under `#pragma dont_inline on`, so the
 *   `ut::Color` constructor, conversion and the temporary's destructor are called out of line.  The light results'
 *   colours reach InitLightColor/fog_set_color as `static_cast<const ut::Color&>` (retail's fields are ut::Color:
 *   the by-value GXColor slice takes its address before the copy); GetLightSetting's diffuse loops each declare
 *   their own index and reach the light object through one reference.  GetLightSet takes its LightSet by reference: retail passes a copy made by the out-of-line copy
 *   constructor (fn_800651F0) and destroys it after the call (obj_delete_flagged), which GetLightSetting spells.
 *   The resolver words are passed to the handle copies by reference (retail's return temporaries sit below the
 *   handles); `perspFovy / 2.0f` keeps retail's load order of the ortho height.
 */

#include "types.h"
#include "nw4r/g3d/res_common.h" /* ResHandle (rule 1) */
#include "nw4r/db_assert.h"      /* nw4r::db::Panic / Warning, owner nw4r/db_assert.cpp (rule 2) */
#include "nw4r/ut_Color.h"       /* nw4r::ut::Color */
#include "unsplit/g3d.h"      /* the math types it reads through the band header */
#include "g3d/g3d_light.h" /* LightObj / LightSet / LightSetting and their initialisers (rule 2) */
#include "g3d/g3d_fog.h"   /* nw4r::g3d::Fog (rule 1) */
#include "g3d/g3d_resanmscn.h"      /* the ResAnmScn index and reference-number getters (rule 2) */
#include "g3d/g3d_resanmamblight.h" /* res_anm_amb_light_get_anm_result (rule 2) */
#include "g3d/g3d_resanmcamera.h"   /* res_anm_camera_get_anm_result (rule 2) */
#include "g3d/g3d_resanmlight.h"    /* res_anm_light_get_anm_result (rule 2) */
#include "g3d/g3d_resanmfog.h"      /* res_anm_fog_get_anm_result (rule 2) */
#include "g3d/g3d_anmscn.h"
#include "g3d/light_obj_is_enable.h" /* this unit's light-object, light-setting and fog accessors */
#include "g3d/g3d_anmclr.h"   /* color_to_u32_ref, color_ctor_white, owned by g3d/g3d_anmclr.cpp (rule 2) */
#include "g3d/g3d_anmchr.h"   /* type_obj_set_name, fn_8005DC60, GetAnmPlayPolicy, owned by g3d/g3d_anmchr.cpp (rule 2) */
#include "g3d/g3d_resmat.h"   /* nw4r::g3d::ResMat / ResMatChan (rule 2) */
#include "g3d/g3d_pointer_assert.h" /* G3D_POINTER_ASSERT (rule 1) */
#include "fn_80047398.h" /* color_rgba_copy (rule 2) */
#include "fn_8004CAD8.h" /* subVec3 / vec3_scale / vec3_normalize_into (rule 2) */
#include "mh3_pad.h"        /* VEC3_ctor / copyVec3, owned by mh3_pad.cpp (rule 2) */
#include "MSL_C/alloc.h"    /* __fpclassifyf, owner MSL_C/alloc.cpp (rule 2) */

#pragma peephole off
#pragma pool_data off

using namespace nw4r;
using namespace nw4r::g3d;

/* The run-time type-name records GetTypeObj/GetTypeObjStatic return. */
extern "C" const TypeNameData<sizeof("AnmScn")> anm_typename_AnmScn = {sizeof("AnmScn"), "AnmScn"};
extern "C" const TypeNameData<sizeof("AnmScnRes")> anm_typename_AnmScnRes = {sizeof("AnmScnRes"), "AnmScnRes"};

/* The type-name strings the `_ac.h` getters below return by address (`.data`, read-only). */
extern u8 lbl_8058C928[];
extern u8 lbl_8058CA70[];
extern u8 lbl_8058CB38[];
extern u8 lbl_8058CC04[];
extern u8 lbl_8058CA18[];
extern u8 lbl_8058CA58[];
extern u8 lbl_8058CC10[];
extern u8 lbl_8058CC2C[];
extern u8 lbl_8058C934[];
extern u8 lbl_8058C950[];
extern u8 lbl_8058C968[];
extern u8 lbl_8058C9AC[];
extern u8 lbl_8058C9C4[];
extern u8 lbl_8058CA00[];
extern u8 lbl_8058CA80[];
extern u8 lbl_8058CAA0[];
extern u8 lbl_8058CB44[];
extern u8 lbl_8058CB60[];
extern u8 lbl_8058CB78[];
extern u8 lbl_8058CB98[];
extern u8 lbl_8058CBA8[];
extern u8 lbl_8058CBC8[];
extern u8 lbl_8058CBD8[];
extern u8 lbl_8058CBF8[];
extern u8 lbl_8058CAFC[]; /* "ResAnmFog"     */
extern u8 lbl_8058CAB8[]; /* "ResAnmCamera"  */
extern u8 lbl_8058CB24[];
extern u8 lbl_8058CB08[];
extern u8 lbl_8058CAE4[];
extern u8 lbl_8058CAC8[];

/* The `ResAnmScn` count readers of `g3d/g3d_resanmlight.cpp` (the GetNum* slots forward to them). */
extern "C" u32 fn_80090110(const ResHandle *p);
extern "C" u32 fn_800902E4(const ResHandle *p);
extern "C" u32 fn_800904B8(const ResHandle *p);
extern "C" u32 fn_8009068C(const ResHandle *p);
extern "C" u32 fn_80090860(const ResHandle *p);

/* This unit's own accessor copies, declared ahead of the definitions (which follow retail's address order). */
extern "C" {
void fn_80064BE4(LightObj *p);
void fn_80064BF4(LightObj *p);
void fn_80064C04(LightObj *self, const math::VEC3 *v);
void fn_80064CE0(LightObj *self, const math::VEC3 *v);
u32 *fn_800651E8(const LightSetting *p);
void fn_800651F0(LightSet *dst, const LightSet *src);
void fn_8006522C(LightSet *out, LightSetting *self, s32 index);
u32 fn_8006527C(const LightSetting *p);
LightSet *obj_delete_flagged(LightSet *pSelf, s16 flag);
u32 fn_80065828(const ResHandle *p);
u32 fn_800658B0(u32 *p);
u32 fn_800658B8(void);
s32 fn_800658C4(const ResHandle *p);
u32 fn_800658F0(const ResHandle *p);
u32 fn_8006591C(const ResHandle *p);
u32 fn_80065948(const ResHandle *p);
u32 fn_80065974(const ResHandle *p);
u32 fn_800659A0(const ResHandle *p);
u32 anmscn_round_up(u32 offset, unsigned int align);
ResHandle *fn_80065FFC(ResHandle *dst, const ResHandle *src);
void fn_8006602C(u32 *dst, const u32 *src);
u32 fn_80066038(const ResHandle *p);
s32 fn_8006605C(const ResHandle *p);
u32 fn_80066C68(const ResHandle *p);
u32 fn_80066CF0(u32 *p);
u32 fn_80066CF8(void);
s32 fn_80066D04(const ResHandle *p);
ResHandle *fn_80066D18(ResHandle *dst, const u32 &src);
void fn_80066D48(u32 *dst, const u32 *src);
u32 fn_80066D54(const ResHandle *p);
u32 fn_80066E18(u32 *p);
u32 fn_80066E20(void);
s32 fn_80066E2C(const ResHandle *p);
u32 fn_80066E5C(const ResHandle *p);
u32 fn_80066EE4(u32 *p);
u32 fn_80066EEC(void);
u32 res_anm_light_config_test_flag8(const ResHandle *p);
u32 fn_80066F2C(const ResHandle *p);
s32 fn_80066F50(const ResHandle *p);
ResHandle *fn_80066F64(ResHandle *dst, const u32 &src);
void fn_80066F94(u32 *dst, const u32 *src);
u32 fn_80066FA0(const ResHandle *self, u32 index);
u32 fn_80067060(const ResHandle *p);
s32 fn_80067084(const ResHandle *p);
ResHandle *fn_80067098(ResHandle *dst, const u32 &src);
void fn_800670C8(u32 *dst, const u32 *src);
LightAnmResult *fn_800677D8(LightAnmResult *self);
void fog_set_color(Fog *self, GXColor color);
void fn_80067A5C(Fog *self, f32 startz, f32 endz);
void fn_80067AE4(Fog *self, s32 type);
FogAnmResult *fn_80067B5C(FogAnmResult *self);
CameraAnmResult *fn_80067EB4(CameraAnmResult *self);
u32 res_anm_light_get_id(const ResHandle *p);
u32 res_anm_fog_get_id(const ResHandle *p);
u32 res_anm_camera_get_id(const ResHandle *p);
u32 fn_80068698(void *p);
u32 fn_800686A0(void);
u32 fn_800686AC(const ResHandle *p);
ResHandle *fn_800686C0(ResHandle *self, const u32 &src);
void fn_800686F0(u32 *dst, const u32 *src);
u32 fn_80068A14(void *p);
u32 fn_80068A1C(void);
u32 fn_80068A28(const ResHandle *p);
ResHandle *fn_80068A3C(ResHandle *self, const u32 &src);
void fn_80068A6C(u32 *dst, const u32 *src);
}

/* 0x800680A8: the ambient light's ID (C++ linkage, map row `fn_800680A8__FPv`). */
unsigned int fn_800680A8(void *obj);

/* The `ResAnmAmbLight`/`ResAnmLight`/`ResAnmFog`/`ResAnmCamera` records' ID word. */
typedef struct {
    /* +0x00 */ u8 pad_0x00[0xC];
    /* +0x0C */ u32 id;
} ResAnmIdHead; /* size: 0x10 (approximate - only the +0xC ID is evidenced) */

/* Whether `value` is a finite number (not infinite, not NaN): the frame and rate setters' assertion. */
#define ANMSCN_FLOAT_VALID(value) (__fpclassifyf(value) > 2 && __fpclassifyf(value) != 1)

#define ANMSCN_FILE "g3d_anmscn.cpp"
#define ANMSCN_THIS_ERROR "NW4R:Pointer Error\nthis(=%p) is not valid pointer."
#define ANMSCN_RESULT_ERROR "NW4R:Pointer Error\npResult(=%p) is not valid pointer."

/* --------------------------------------------------------------------------------------------- *
 * The light-object builders (0x800649CC-0x80064CF0).
 * --------------------------------------------------------------------------------------------- */

namespace nw4r {
namespace g3d {
namespace {

/* 0x800649CC (0x208): builds a point, directional or spot light object from a light animation result. */
void MakeDiffuseLightObj(LightObj *pObj, const LightAnmResult *pResult)
{
    s32 type = pResult->flags & LightAnmResult::FLAG_LIGHT_TYPE_MASK;

    pObj->Clear();
    switch (type) {
    case 0:
        pObj->InitLightColor(static_cast<const ut::Color&>(pResult->color));
        fn_80064C14(pObj, &pResult->pos.x);
        pObj->InitLightSpot(0.0f, 0);
        pObj->InitLightDistAttn(pResult->refDistance, pResult->refBrightness, pResult->distFunc);
        fn_8007A5E4(pObj, 0.0f, 0.0f, 0.0f);
        break;
    case 1: {
        math::VEC3 dir;
        math::VEC3 pos;

        subVec3(&dir, &pResult->aim, &pResult->pos);
        vec3_scale(&pos, &dir, -1.0e10f);
        pObj->InitLightColor(static_cast<const ut::Color&>(pResult->color));
        fn_80064C14(pObj, &pos.x);
        pObj->InitLightAttnA(1.0f, 0.0f, 0.0f);
        pObj->InitLightAttnK(1.0f, 0.0f, 0.0f);
        fn_8007A5E4(pObj, 0.0f, 0.0f, 0.0f);
        break;
    }
    case 2: {
        math::VEC3 dir;

        subVec3(&dir, &pResult->aim, &pResult->pos);
        vec3_normalize_into(&dir, &dir);
        pObj->InitLightColor(static_cast<const ut::Color&>(pResult->color));
        fn_80064C14(pObj, &pResult->pos.x);
        pObj->InitLightSpot(pResult->cutoff, pResult->spotFunc);
        pObj->InitLightDistAttn(pResult->refDistance, pResult->refBrightness, pResult->distFunc);
        fn_80064C04(pObj, &dir);
        break;
    }
    default:
        nw4r::db::Panic(ANMSCN_FILE, 105, "NW4R:Fatal Error\nMakeDiffuseLightObj: Unknown LightType=(%d)", type);
        break;
    }
    if (!(pResult->flags & LightAnmResult::FLAG_COLOR_ENABLE)) {
        fn_80064BF4(pObj);
    }
    if (!(pResult->flags & LightAnmResult::FLAG_ALPHA_ENABLE)) {
        fn_80064BE4(pObj);
    }
    light_obj_enable(pObj);
}

/* 0x80064C24 (0xBC): builds the specular light object from a light animation result. */
void MakeSpecularLightObj(LightObj *pObj, const LightAnmResult *pResult)
{
    math::VEC3 dir;

    subVec3(&dir, &pResult->aim, &pResult->pos);
    vec3_normalize_into(&dir, &dir);
    pObj->Clear();
    pObj->InitLightColor(static_cast<const ut::Color&>(pResult->specColor));
    fn_80064CE0(pObj, &dir);
    pObj->InitLightShininess(pResult->shininess);
    if (!(pResult->flags & LightAnmResult::FLAG_COLOR_ENABLE)) {
        fn_80064BF4(pObj);
    }
    if (!(pResult->flags & LightAnmResult::FLAG_ALPHA_ENABLE)) {
        fn_80064BE4(pObj);
    }
    light_obj_enable(pObj);
}

}  // namespace
}  // namespace g3d
}  // namespace nw4r

/* Sets the light object's enable bit. */
extern "C" void light_obj_enable(LightObj *p)
{
    p->mFlag |= 4;
}

extern "C" void fn_80064BE4(LightObj *p)
{
    p->mFlag |= 0x20;
}

extern "C" void fn_80064BF4(LightObj *p)
{
    p->mFlag |= 0x10;
}

/* The three-float setter forwarders. */
extern "C" void fn_80064C04(LightObj *self, const math::VEC3 *v)
{
    fn_8007A5E4(self, v->x, v->y, v->z);
}

extern "C" void fn_80064C14(void *self, const f32 *v)
{
    fn_8007A5A8(self, v[0], v[1], v[2]);
}

extern "C" void fn_80064CE0(LightObj *self, const math::VEC3 *v)
{
    fn_8007A724(self, v->x, v->y, v->z);
}

/* --------------------------------------------------------------------------------------------- *
 * AnmScn::GetLightSetting and the light accessors it calls (0x80064CF0-0x800652C8).
 * --------------------------------------------------------------------------------------------- */

#pragma dont_inline on
/* 0x80064CF0 (0x49C): fills the light setting's light sets, ambient colours and light objects from the scene
 * animation. */
void AnmScn::GetLightSetting(LightSetting *pSetting)
{
    G3D_POINTER_ASSERT(ANMSCN_FILE, this, 182, ANMSCN_THIS_ERROR);
    G3D_POINTER_ASSERT(ANMSCN_FILE, pSetting, 183, "NW4R:Pointer Error\npSetting(=%p) is not valid pointer.");

    u32 numLightSet = GetLightSetMaxRefNumber();
    u32 numAmbLight = GetAmbLightMaxRefNumber();
    u32 numDiffuseLight = GetDiffuseLightMaxRefNumber();

    if (numLightSet != 0) {
        u32 numSettingLightSet = fn_8006527C(pSetting);
        u32 n = *g3d_min_u32(&numLightSet, &numSettingLightSet);

        for (u32 i = 0; i < n; i++) {
            LightSet set;
            LightSet arg;

            fn_8006522C(&set, pSetting, i);
            NW4R_ASSERT(light_set_is_valid(&set), ANMSCN_FILE, 198, "NW4R:Failed assertion lightSet.IsValid()");
            fn_800651F0(&arg, &set);
            GetLightSet(arg, i);
            obj_delete_flagged(&arg, -1);
        }
    }

    if (numAmbLight != 0) {
        u32 *pAmbColor = fn_800651E8(pSetting);
        u32 numLightObj = light_setting_get_num_light_obj(pSetting);
        u32 n = *g3d_min_u32(&numAmbLight, &numLightObj);

        for (u32 i = 0; i < n; i++) {
            pAmbColor[i] = GetAmbLightColor(i);
        }
    }

    if (numDiffuseLight != 0) {
        u32 diffuseCount = numDiffuseLight;
        LightObj *pLightObj = light_setting_get_light_obj_array(pSetting);
        u32 numLightObj = light_setting_get_num_light_obj(pSetting);
        u32 numLight = diffuseCount + GetNumSpecularLight();
        u32 numDiffuse = *g3d_min_u32(&numDiffuseLight, &numLightObj);
        u32 numTotal = *g3d_min_u32(&numLight, &numLightObj);

        for (u32 i = 0; i < numDiffuse; i++) {
            light_obj_disable(&pLightObj[i]);
        }
        for (u32 i = 0; i < numDiffuse; i++) {
            LightObj &rDiff = pLightObj[i];
            LightObj *pSpec = NULL;

            if (!light_obj_is_enable(&rDiff)) {
                if (HasSpecularLight(i)) {
                    u32 specID = GetSpecularLightID(i);
                    if (specID < numTotal) {
                        pSpec = &pLightObj[specID];
                    }
                }
                GetLight(&rDiff, pSpec, i);
            }
        }
    }
}
#pragma dont_inline reset

/* Whether the light object is enabled. */
extern "C" s32 light_obj_is_enable(const LightObj *p)
{
    u32 masked = p->mFlag & 4;
    return masked != 0;
}

/* Clears the light object's enable bit. */
extern "C" void light_obj_disable(LightObj *p)
{
    p->mFlag &= ~0x4u;
}

extern "C" LightObj *light_setting_get_light_obj_array(const LightSetting *p)
{
    return p->mpLightObjArray;
}

/* Reads the +0x0 word through the identity helper (0x80064754), the shape the two readers share. */
extern "C" u32 fn_800651BC(void *self)
{
    return *color_to_u32_ref((u32 *)self);
}

extern "C" u32 light_setting_get_num_light_obj(const LightSetting *p)
{
    return p->mNumLight;
}

extern "C" u32 *fn_800651E8(const LightSetting *p)
{
    return (u32 *)p->mpAmbLightObjArray;
}

/* Copies a light set view. */
extern "C" void fn_800651F0(LightSet *dst, const LightSet *src)
{
    dst->mpSetting = src->mpSetting;
    dst->mpLightSetData = src->mpLightSetData;
}

/* Whether the light set view names a setting and a record. */
extern "C" s32 light_set_is_valid(const LightSet *p)
{
    s32 ret = 0;
    if (p->mpSetting != 0 && p->mpLightSetData != 0)
        ret = 1;
    return ret;
}

/* Out-of-range lookups store a NULL record but always store the setting. */
extern "C" void fn_8006522C(LightSet *out, LightSetting *self, s32 index)
{
    LightSetData *elem;
    if (index < (s32)self->mNumLightSet && index >= 0)
        elem = &self->mpLightSetDataArray[index];
    else
        elem = 0;
    out->mpSetting = self;
    out->mpLightSetData = elem;
}

/* The smaller of two counts (by reference). */
extern "C" const u32 *g3d_min_u32(const u32 *a, const u32 *b)
{
    return (*b < *a) ? b : a;
}

extern "C" u32 fn_8006527C(const LightSetting *p)
{
    return p->mNumLightSet;
}

/* 0x80065284 (0x44): the light set view's destructor (frees it when `flag` is positive). */
extern "C" LightSet *obj_delete_flagged(LightSet *pSelf, s16 flag)
{
    if (pSelf && flag > 0) {
        operator delete(pSelf);
    }
    return pSelf;
}

/* --------------------------------------------------------------------------------------------- *
 * AnmScn's own members (0x800652C8-0x80065808).
 * --------------------------------------------------------------------------------------------- */

/* 0x800652C8 (0x274): a plain scene animation takes no children: warns and returns NULL. */
AnmScn *AnmScn::Attach(int idx, AnmScnRes *pRes)
{
    G3D_POINTER_ASSERT(ANMSCN_FILE, this, 269, ANMSCN_THIS_ERROR);
    G3D_POINTER_ASSERT(ANMSCN_FILE, pRes, 270, "NW4R:Pointer Error\npObj(=%p) is not valid pointer.");
    nw4r::db::Warning(ANMSCN_FILE, 276, "AnmScn::Attach(%d, %p(%s)) called with %s. Maybe uninteded call.", idx,
                      pRes, pRes->GetTypeName(), GetTypeName());
    return NULL;
}

/* 0x8006553C (0x38): returns the type's name. */
const char *AnmScn::GetTypeName() const
{
    return GetTypeObj().GetTypeName();
}

/* 0x80065574 (0x30): returns the AnmScn type. */
const nw4r::g3d::G3dObj::TypeObj AnmScn::GetTypeObj() const
{
    const u8 *pName;
    return *reinterpret_cast<const TypeObj *>(type_obj_set_name(&pName, (const u8 *)&anm_typename_AnmScn));
}

/* 0x800655A4 (0x38): returns the type's name. */
const char *AnmScnRes::GetTypeName() const
{
    return GetTypeObj().GetTypeName();
}

/* 0x800655DC (0x30): returns the AnmScnRes type. */
const nw4r::g3d::G3dObj::TypeObj AnmScnRes::GetTypeObj() const
{
    const TypeObj::TypeName *pName;
    return *reinterpret_cast<const TypeObj *>(fn_8005DC60((void **)&pName, (void *)&anm_typename_AnmScnRes));
}

/* 0x8006560C (0x15C): a plain scene animation has no children: warns and returns NULL. */
AnmScnRes *AnmScn::Detach(int idx)
{
    G3D_POINTER_ASSERT(ANMSCN_FILE, this, 286, ANMSCN_THIS_ERROR);
    nw4r::db::Warning(ANMSCN_FILE, 292, "AnmScn::Detach(%d) called with %s. Maybe uninteded call.", idx,
                      GetTypeName());
    return NULL;
}

/* 0x80065768 (0x5C): destroys the scene animation. */
AnmScn::~AnmScn()
{
}

/* 0x800657C4 (0x40): constructs the scene animation with no parent. */
AnmScn::AnmScn(MEMAllocator *pHeap) : nw4r::g3d::G3dObj(pHeap, NULL)
{
}

/* The placement `operator delete`: nothing to release. */
/* untyped: opaque handle passed through - the block and the placement address */
void operator delete(void *block, void *place)
{
}

/* --------------------------------------------------------------------------------------------- *
 * AnmScnRes's count and limit slots and the `ResAnmScn` readers they forward to (0x80065808-0x800659D8).
 * --------------------------------------------------------------------------------------------- */

u32 AnmScnRes::GetNumLightSet() const
{
    return fn_80090110(&mRes);
}

u32 AnmScnRes::GetNumAmbLight() const
{
    return fn_800902E4(&mRes);
}

u32 AnmScnRes::GetNumDiffuseLight() const
{
    return fn_800904B8(&mRes);
}

u32 AnmScnRes::GetNumSpecularLight() const
{
    return fn_80065828(&mRes);
}

extern "C" u32 fn_80065828(const ResHandle *p)
{
    return fn_8006584C((void *)p)->field_0x36;
}

/* The checked `ResAnmScn` resolver: panics out of `g3d_resanmscn_ac.h` when the handle is empty. */
extern "C" ResAnmScnConfig *fn_8006584C(void *p)
{
    if (fn_800658C4((const ResHandle *)p) == 0)
        nw4r::db::Panic((const char *)lbl_8058CC2C, 0x28, (const char *)lbl_8058CC10,
                        (const char *)fn_800658B8(), "ref");
    return (ResAnmScnConfig *)fn_800658B0((u32 *)p);
}

extern "C" u32 fn_800658B0(u32 *p)
{
    return *p;
}

extern "C" u32 fn_800658B8(void)
{
    return (u32)lbl_8058CC04;
}

extern "C" s32 fn_800658C4(const ResHandle *p)
{
    return p->mpData != 0;
}

u32 AnmScnRes::GetNumFog() const
{
    return fn_8009068C(&mRes);
}

u32 AnmScnRes::GetNumCamera() const
{
    return fn_80090860(&mRes);
}

u32 AnmScnRes::GetLightSetMaxRefNumber() const
{
    return fn_800658F0(&mRes);
}

extern "C" u32 fn_800658F0(const ResHandle *p)
{
    return fn_8006584C((void *)p)->field_0x3C;
}

u32 AnmScnRes::GetAmbLightMaxRefNumber() const
{
    return fn_8006591C(&mRes);
}

extern "C" u32 fn_8006591C(const ResHandle *p)
{
    return fn_8006584C((void *)p)->field_0x3E;
}

u32 AnmScnRes::GetDiffuseLightMaxRefNumber() const
{
    return fn_80065948(&mRes);
}

extern "C" u32 fn_80065948(const ResHandle *p)
{
    return fn_8006584C((void *)p)->field_0x40;
}

u32 AnmScnRes::GetFogMaxRefNumber() const
{
    return fn_80065974(&mRes);
}

extern "C" u32 fn_80065974(const ResHandle *p)
{
    return fn_8006584C((void *)p)->field_0x42;
}

u32 AnmScnRes::GetCameraMaxRefNumber() const
{
    return fn_800659A0(&mRes);
}

extern "C" u32 fn_800659A0(const ResHandle *p)
{
    return fn_8006584C((void *)p)->field_0x44;
}

/* Whether the fog handle names a record. */
extern "C" s32 fog_is_valid(const Fog *p)
{
    return p->mpFogData != 0;
}

/* --------------------------------------------------------------------------------------------- *
 * Construct, the constructor and the `ResAnmScn` readers it calls (0x800659D8-0x800660DC).
 * --------------------------------------------------------------------------------------------- */

/* 0x800659D8 (0x500): sizes the object and its result caches, allocates them from `pHeap` and constructs the
 * scene animation in the block; `pSize` receives the size. */
AnmScnRes *AnmScnRes::Construct(MEMAllocator *pHeap, u32 *pSize, ResHandle resAnmScn, bool bHasCache)
{
    if (!fn_800658C4(&resAnmScn)) {
        return NULL;
    }

    u32 numAmbLight = bHasCache ? fn_800902E4(&resAnmScn) : 0;
    u32 numLight = bHasCache ? fn_800904B8(&resAnmScn) : 0;
    u32 numFog = bHasCache ? fn_8009068C(&resAnmScn) : 0;
    u32 numCamera = bHasCache ? fn_80090860(&resAnmScn) : 0;

    u32 ambLightSize = numAmbLight * sizeof(AmbLightAnmResult);
    u32 lightSize = numLight * sizeof(LightAnmResult);
    u32 fogSize = numFog * sizeof(FogAnmResult);
    u32 cameraSize = numCamera * sizeof(CameraAnmResult);

    u32 ambLightOfs = anmscn_round_up(sizeof(AnmScnRes), 4);
    u32 lightOfs = anmscn_round_up(ambLightOfs + ambLightSize, 4);
    u32 fogOfs = anmscn_round_up(lightOfs + lightSize, 4);
    u32 cameraOfs = anmscn_round_up(fogOfs + fogSize, 4);
    u32 size = anmscn_round_up(cameraOfs + cameraSize, 4);

    if (pSize != NULL) {
        G3D_POINTER_ASSERT(ANMSCN_FILE, pSize, 1113, "NW4R:Pointer Error\npSize(=%p) is not valid pointer.");
        *pSize = size;
    }
    if (pHeap == NULL) {
        return NULL;
    }
    G3D_POINTER_ASSERT(ANMSCN_FILE, pHeap, 1122, "NW4R:Pointer Error\npHeap(=%p) is not valid pointer.");

    u8 *pBuf = (u8 *)Alloc(pHeap, size);
    if (pBuf == NULL) {
        return NULL;
    }
    G3D_POINTER_ASSERT(ANMSCN_FILE, pBuf, 1132, "NW4R:Pointer Error\nbuf(=%p) is not valid pointer.");
    if (((u32)pBuf & 3) != 0) {
        nw4r::db::Panic(ANMSCN_FILE, 1133, "NW4R:Alignment Error(0x%x)\nbuf must be aligned to 4 bytes boundary.",
                        pBuf);
    }

    return new (pBuf) AnmScnRes(pHeap, resAnmScn,
                                ambLightSize != 0 ? (AmbLightAnmResult *)(pBuf + ambLightOfs) : NULL,
                                lightSize != 0 ? (LightAnmResult *)(pBuf + lightOfs) : NULL,
                                fogSize != 0 ? (FogAnmResult *)(pBuf + fogOfs) : NULL,
                                cameraSize != 0 ? (CameraAnmResult *)(pBuf + cameraOfs) : NULL);
}

/* Rounds `offset` up to a multiple of `align` (a power of two). */
extern "C" u32 anmscn_round_up(u32 offset, unsigned int align)
{
    return (align + offset - 1) & ~(align - 1);
}

/* 0x80065EF0 (0x10C): starts the frame control over the resource's frame count and play policy, keeps the
 * resource and the caches, and fills the caches when there are any. */
AnmScnRes::AnmScnRes(MEMAllocator *pHeap, ResHandle resAnmScn, AmbLightAnmResult *pAmbLightCache,
                     LightAnmResult *pLightCache, FogAnmResult *pFogCache, CameraAnmResult *pCameraCache)
    : AnmScn(pHeap),
      FrameCtrl(0.0f, fn_8006605C(&resAnmScn), GetAnmPlayPolicy((AnmPolicy)fn_80066038(&resAnmScn)))
{
    fn_80065FFC(&mRes, &resAnmScn);
    mScnAnmFlags = 0;
    mpAmbLightCache = pAmbLightCache;
    mpLightCache = pLightCache;
    mpFogCache = pFogCache;
    mpCameraCache = pCameraCache;
    if (pAmbLightCache != NULL || pLightCache != NULL || pFogCache != NULL || pCameraCache != NULL) {
        SetScnAnmFlag(FLAG_HAS_CACHE, true);
    }
    if (TestScnAnmFlag(FLAG_HAS_CACHE)) {
        UpdateCache();
    }
}

/* Copies a `ResAnmScn` handle. */
extern "C" ResHandle *fn_80065FFC(ResHandle *dst, const ResHandle *src)
{
    fn_8006602C((u32 *)dst, (const u32 *)src);
    return dst;
}

extern "C" void fn_8006602C(u32 *dst, const u32 *src)
{
    *dst = *src;
}

/* The resource's play policy. */
extern "C" u32 fn_80066038(const ResHandle *p)
{
    return fn_8006584C((void *)p)->field_0x38;
}

/* The resource's frame count. */
extern "C" s32 fn_8006605C(const ResHandle *p)
{
    return fn_8006584C((void *)p)->field_0x34;
}

/* 0x80066080 (0x5C): destroys the scene animation. */
AnmScnRes::~AnmScnRes()
{
}

/* --------------------------------------------------------------------------------------------- *
 * The frame control and G3dProc (0x800660DC-0x80066990).
 * --------------------------------------------------------------------------------------------- */

/* 0x800660DC (0x1A0): sets the frame and refreshes the caches. */
void AnmScnRes::SetFrame(f32 frame)
{
    G3D_POINTER_ASSERT(ANMSCN_FILE, this, 1192, ANMSCN_THIS_ERROR);
    {
        bool valid = false;
        if (ANMSCN_FLOAT_VALID(frame)) {
            valid = true;
        }
        if (!valid) {
            nw4r::db::Panic(ANMSCN_FILE, 1193, "NW4R:Floating Point Value Error(%f)\nframe is infinite or nan.",
                            frame);
        }
    }
    SetFrm(frame);
    if (TestScnAnmFlag(FLAG_HAS_CACHE)) {
        UpdateCache();
    }
}

/* 0x8006627C (0x11C): the current frame. */
f32 AnmScnRes::GetFrame() const
{
    G3D_POINTER_ASSERT(ANMSCN_FILE, this, 1206, ANMSCN_THIS_ERROR);
    return GetFrm();
}

/* 0x80066398 (0x1B8): sets the update rate; a stopped animation refreshes the caches. */
void AnmScnRes::SetUpdateRate(f32 rate)
{
    G3D_POINTER_ASSERT(ANMSCN_FILE, this, 1214, ANMSCN_THIS_ERROR);
    {
        bool valid = false;
        if (ANMSCN_FLOAT_VALID(rate)) {
            valid = true;
        }
        if (!valid) {
            nw4r::db::Panic(ANMSCN_FILE, 1215, "NW4R:Floating Point Value Error(%f)\nrate is infinite or nan.",
                            rate);
        }
    }
    SetRate(rate);
    if (0.0f == rate) {
        if (TestScnAnmFlag(FLAG_HAS_CACHE)) {
            UpdateCache();
        }
    }
}

/* 0x80066550 (0x11C): the update rate. */
f32 AnmScnRes::GetUpdateRate() const
{
    G3D_POINTER_ASSERT(ANMSCN_FILE, this, 1230, ANMSCN_THIS_ERROR);
    return GetRate();
}

/* 0x8006666C (0x14C): advances the frame unless stopped, then refreshes the caches. */
void AnmScnRes::UpdateFrame()
{
    G3D_POINTER_ASSERT(ANMSCN_FILE, this, 1238, ANMSCN_THIS_ERROR);
    if (0.0f != GetRate()) {
        UpdateFrm();
        if (TestScnAnmFlag(FLAG_HAS_CACHE)) {
            UpdateCache();
        }
    }
}

/* 0x800667B8 (0x1D8): advances on the frame notification and tracks the parent on attach and detach. */
/* untyped: caller-owned payload - the notification's argument (the new parent on attach) */
void AnmScnRes::G3dProc(u32 task, u32 param, void *pInfo)
{
    G3D_POINTER_ASSERT(ANMSCN_FILE, this, 1261, ANMSCN_THIS_ERROR);
    switch (task) {
    case G3DPROC_UPDATEFRAME:
        UpdateFrame();
        break;
    case G3DPROC_DETACH_PARENT:
        NW4R_ASSERT(GetParent(), ANMSCN_FILE, 1273, "NW4R:Failed assertion GetParent()");
        SetParent(NULL);
        break;
    case G3DPROC_ATTACH_PARENT:
        NW4R_ASSERT(!GetParent(), ANMSCN_FILE, 1280, "NW4R:Failed assertion !GetParent()");
        SetParent((nw4r::g3d::G3dObj *)pInfo);
        break;
    }
}

/* --------------------------------------------------------------------------------------------- *
 * The light-set, ambient, light, fog and camera queries (0x80066990-0x80067EFC) and the accessor copies
 * between them.
 * --------------------------------------------------------------------------------------------- */

/* 0x80066990 (0x2D8): selects the light objects and the ambient object of the light set `refNumber` names. */
bool AnmScnRes::GetLightSet(LightSet &set, u32 refNumber)
{
    G3D_POINTER_ASSERT(ANMSCN_FILE, this, 1307, ANMSCN_THIS_ERROR);

    ResHandle resLightSet;
    fn_80067098(&resLightSet, res_anm_scn_get_res_light_set_by_ref_number(&mRes, refNumber));
    if (!fn_80067084(&resLightSet)) {
        return false;
    }

    u32 specIdx;
    u32 i;
    u32 numLight = fn_80067060(&resLightSet);
    specIdx = 7;
    for (i = 0; i < numLight; i++) {
        u32 lightID = fn_80066FA0(&resLightSet, i);
        if (lightID != 0xFFFF) {
            ResHandle resLight;
            fn_80066F64(&resLight, res_anm_scn_get_res_anm_light(&mRes, lightID));
            NW4R_ASSERT(fn_80066F50(&resLight), ANMSCN_FILE, 1329, "NW4R:Failed assertion resLight.IsValid()");
            set.SelectLightObj(i, fn_80066F2C(&resLight));
            if (res_anm_light_config_test_flag8(&resLight)) {
                set.SelectLightObj(specIdx--, fn_80066E5C(&resLight));
            }
        } else {
            set.SelectLightObj(i, -1);
        }
    }
    for (; numLight <= specIdx; numLight++) {
        set.SelectLightObj(numLight, -1);
    }

    if (fn_80066E2C(&resLightSet)) {
        ResHandle resAmb;
        fn_80066D18(&resAmb, res_anm_scn_get_res_anm_amb_light(&mRes, fn_80066D54(&resLightSet)));
        NW4R_ASSERT(fn_80066D04(&resAmb), ANMSCN_FILE, 1364, "NW4R:Failed assertion resAmb.IsValid()");
        set.SelectAmbLightObj(fn_80066C68(&resAmb));
    } else {
        set.SelectAmbLightObj(-1);
    }
    return true;
}

/* The ambient light's light-object index. */
extern "C" u32 fn_80066C68(const ResHandle *p)
{
    return ((const ResAnmLightConfig *)fn_80066C8C((void *)p))->field_0x10;
}

/* The checked `ResAnmAmbLight` resolver (C++ linkage). */
void *fn_80066C8C(void *obj)
{
    if (fn_80066D04((const ResHandle *)obj) == 0)
        nw4r::db::Panic((const char *)lbl_8058CAA0, 0x25, (const char *)lbl_8058CA80,
                        (const char *)fn_80066CF8(), "ref");
    return (void *)fn_80066CF0((u32 *)obj);
}

extern "C" u32 fn_80066CF0(u32 *p)
{
    return *p;
}

extern "C" u32 fn_80066CF8(void)
{
    return (u32)lbl_8058CA70;
}

extern "C" s32 fn_80066D04(const ResHandle *p)
{
    return p->mpData != 0;
}

/* Copies a `ResAnmAmbLight` handle. */
extern "C" ResHandle *fn_80066D18(ResHandle *dst, const u32 &src)
{
    fn_80066D48((u32 *)dst, &src);
    return dst;
}

extern "C" void fn_80066D48(u32 *dst, const u32 *src)
{
    *dst = *src;
}

/* The light set's ambient light ID; the invalid ID panics out of `g3d_reslightset_ac.h`. */
extern "C" u32 fn_80066D54(const ResHandle *p)
{
    if (fn_80066DB4((void *)p)->field_0x18 == 0xFFFF)
        nw4r::db::Panic((const char *)lbl_8058CA58, 0x79, (const char *)lbl_8058CA18);
    return fn_80066DB4((void *)p)->field_0x18;
}

/* The checked `ResLightSet` resolver. */
extern "C" ResAnmLightConfig *fn_80066DB4(void *p)
{
    if (fn_80067084((const ResHandle *)p) == 0)
        nw4r::db::Panic((const char *)lbl_8058C950, 0x27, (const char *)lbl_8058C934,
                        (const char *)fn_80066E20(), "ref");
    return (ResAnmLightConfig *)fn_80066E18((u32 *)p);
}

extern "C" u32 fn_80066E18(u32 *p)
{
    return *p;
}

extern "C" u32 fn_80066E20(void)
{
    return (u32)lbl_8058C928;
}

/* Whether the light set has an ambient light. */
extern "C" s32 fn_80066E2C(const ResHandle *p)
{
    return fn_80066DB4((void *)p)->field_0x14 != 0;
}

/* The light's specular light-object index. */
extern "C" u32 fn_80066E5C(const ResHandle *p)
{
    return fn_80066E80((void *)p)->field_0x14;
}

/* The checked `ResAnmLight` resolver. */
extern "C" ResAnmLightConfig *fn_80066E80(void *p)
{
    if (fn_80066F50((const ResHandle *)p) == 0)
        nw4r::db::Panic((const char *)lbl_8058CB60, 0x8f, (const char *)lbl_8058CB44,
                        (const char *)fn_80066EEC(), "ref");
    return (ResAnmLightConfig *)fn_80066EE4((u32 *)p);
}

extern "C" u32 fn_80066EE4(u32 *p)
{
    return *p;
}

extern "C" u32 fn_80066EEC(void)
{
    return (u32)lbl_8058CB38;
}

/* 0x80066EF8 (0x34): whether bit 3 of the light config's channel flags is set. */
extern "C" u32 res_anm_light_config_test_flag8(const ResHandle *p)
{
    return (fn_80066E80((void *)p)->flags & 8) != 0;
}

/* The light's light-object index. */
extern "C" u32 fn_80066F2C(const ResHandle *p)
{
    return fn_80066E80((void *)p)->field_0x10;
}

extern "C" s32 fn_80066F50(const ResHandle *p)
{
    return p->mpData != 0;
}

/* Copies a `ResAnmLight` handle. */
extern "C" ResHandle *fn_80066F64(ResHandle *dst, const u32 &src)
{
    fn_80066F94((u32 *)dst, &src);
    return dst;
}

extern "C" void fn_80066F94(u32 *dst, const u32 *src)
{
    *dst = *src;
}

/* The bounds-checked light ID reader: an index past the end panics, and the invalid ID panics out of
 * `g3d_reslightset_ac.h`. */
typedef struct {
    /* +0x00 */ u8 pad_0x00[0x3C];
    /* +0x3C */ u16 entries_0x3C[1];
} ResAnmLightTable; /* size: 0x3E (approximate - only the +0x3C table is evidenced) */

extern "C" u32 fn_80066FA0(const ResHandle *self, u32 index)
{
    u32 count = fn_80067060(self);
    if (index > count - 1)
        nw4r::db::Panic((const char *)lbl_8058CA00, 0x71, (const char *)lbl_8058C9C4,
                        index, 0, fn_80067060(self) - 1);
    if (((ResAnmLightTable *)fn_80066DB4((void *)self))->entries_0x3C[index] == 0xFFFF)
        nw4r::db::Panic((const char *)lbl_8058C9AC, 0x72, (const char *)lbl_8058C968);
    return ((ResAnmLightTable *)fn_80066DB4((void *)self))->entries_0x3C[index];
}

/* The light set's light count. */
extern "C" u32 fn_80067060(const ResHandle *p)
{
    return fn_80066DB4((void *)p)->field_0x1A;
}

extern "C" s32 fn_80067084(const ResHandle *p)
{
    return p->mpData != 0;
}

/* Copies a `ResLightSet` handle. */
extern "C" ResHandle *fn_80067098(ResHandle *dst, const u32 &src)
{
    fn_800670C8((u32 *)dst, &src);
    return dst;
}

extern "C" void fn_800670C8(u32 *dst, const u32 *src)
{
    *dst = *src;
}

#pragma dont_inline on
/* 0x800670D4 (0x17C): the ambient colour `refNumber` names; a channel the animation does not drive reads as
 * full. */
ut::Color AnmScnRes::GetAmbLightColor(u32 refNumber)
{
    G3D_POINTER_ASSERT(ANMSCN_FILE, this, 1381, ANMSCN_THIS_ERROR);

    AmbLightAnmResult result;
    const AmbLightAnmResult *pResult = GetAmbLightResult(&result, refNumber);
    u32 color = pResult->color;
    if (!(pResult->flags & AmbLightAnmResult::FLAG_COLOR_ENABLE)) {
        color |= 0xFFFFFF00;
    }
    if (!(pResult->flags & AmbLightAnmResult::FLAG_ALPHA_ENABLE)) {
        color |= 0xFF;
    }
    return ut::Color(color);
}
#pragma dont_inline reset

/* 0x80067250 (0x588): builds the diffuse and specular light objects of the light `refNumber` names, or
 * disables them when the light is off. */
void AnmScnRes::GetLight(LightObj *pDiff, LightObj *pSpec, u32 refNumber)
{
    G3D_POINTER_ASSERT(ANMSCN_FILE, this, 1403, ANMSCN_THIS_ERROR);

    LightAnmResult result;
    fn_800677D8(&result);
    const LightAnmResult *pResult = GetLightResult(&result, refNumber);
    if (pResult->flags & LightAnmResult::FLAG_LIGHT_ENABLE) {
        if (pDiff != NULL) {
            G3D_POINTER_ASSERT(ANMSCN_FILE, pDiff, 1414, "NW4R:Pointer Error\ndiff(=%p) is not valid pointer.");
            MakeDiffuseLightObj(pDiff, pResult);
        }
        if (pSpec != NULL) {
            G3D_POINTER_ASSERT(ANMSCN_FILE, pSpec, 1421, "NW4R:Pointer Error\nspec(=%p) is not valid pointer.");
            if (pResult->flags & LightAnmResult::FLAG_SPECULAR_ENABLE) {
                MakeSpecularLightObj(pSpec, pResult);
            } else {
                light_obj_disable(pSpec);
            }
        }
    } else {
        if (pDiff != NULL) {
            G3D_POINTER_ASSERT(ANMSCN_FILE, pDiff, 1439, "NW4R:Pointer Error\ndiff(=%p) is not valid pointer.");
            light_obj_disable(pDiff);
        }
        if (pSpec != NULL) {
            G3D_POINTER_ASSERT(ANMSCN_FILE, pSpec, 1444, "NW4R:Pointer Error\nspec(=%p) is not valid pointer.");
            light_obj_disable(pSpec);
        }
    }
}

/* The light result's constructor: default-constructs its two vectors and two colours. */
extern "C" LightAnmResult *fn_800677D8(LightAnmResult *self)
{
    VEC3_ctor(&self->pos);
    VEC3_ctor(&self->aim);
    color_ctor_white((u32 *)&self->color);
    color_ctor_white((u32 *)&self->specColor);
    return self;
}

/* 0x80067824 (0x1B0): sets the fog's type, range and colour from the fog `refNumber` names. */
void AnmScnRes::GetFog(Fog fog, u32 refNumber)
{
    G3D_POINTER_ASSERT(ANMSCN_FILE, this, 1454, ANMSCN_THIS_ERROR);
    NW4R_ASSERT(fog_is_valid(&fog), ANMSCN_FILE, 1455, "NW4R:Failed assertion fog.IsValid()");

    FogAnmResult result;
    fn_80067B5C(&result);
    const FogAnmResult *pResult = GetFogResult(&result, refNumber);
    fn_80067AE4(&fog, pResult->type);
    fn_80067A5C(&fog, pResult->startz, pResult->endz);
    fog_set_color(&fog, static_cast<const ut::Color&>(pResult->color));
}

/* 0x800679D4 (0x80): copies a colour into the fog record the handle names. */
extern "C" void fog_set_color(Fog *self, GXColor color)
{
    if (fog_is_valid(self) == 0)
        nw4r::db::Panic((const char *)lbl_8058CBF8, 0x63, (const char *)lbl_8058CBD8);
    if (fog_is_valid(self) != 0)
        color_rgba_copy((u8 *)fog_ptr(self) + 20, (const u8 *)&color);
}

extern "C" FogData *fog_ptr(const Fog *p)
{
    return p->mpFogData;
}

/* The fog record's z range. */
typedef struct {
    /* +0x00 */ s32 type;
    /* +0x04 */ f32 startz;
    /* +0x08 */ f32 endz;
} FogDataHead; /* size: 0xC (approximate - the record continues with its colour at +0x14) */

/* Sets the fog's z range. */
extern "C" void fn_80067A5C(Fog *self, f32 startz, f32 endz)
{
    if (fog_is_valid(self) == 0)
        nw4r::db::Panic((const char *)lbl_8058CBC8, 0x4b, (const char *)lbl_8058CBA8);
    if (fog_is_valid(self) != 0) {
        FogDataHead *obj = (FogDataHead *)fog_ptr(self);
        obj->startz = startz;
        obj->endz = endz;
    }
}

/* Sets the fog's type. */
extern "C" void fn_80067AE4(Fog *self, s32 type)
{
    if (fog_is_valid(self) == 0)
        nw4r::db::Panic((const char *)lbl_8058CB98, 0x41, (const char *)lbl_8058CB78);
    if (fog_is_valid(self) != 0)
        ((FogDataHead *)fog_ptr(self))->type = type;
}

/* The fog result's constructor: default-constructs its colour. */
extern "C" FogAnmResult *fn_80067B5C(FogAnmResult *self)
{
    color_ctor_white((u32 *)&self->color);
    return self;
}

/* 0x80067B90 (0x2C4): sets the camera's projection, position and posture from the camera `refNumber` names;
 * false when the animation has no such camera. */
bool AnmScnRes::GetCamera(Camera camera, u32 refNumber)
{
    G3D_POINTER_ASSERT(ANMSCN_FILE, this, 1473, ANMSCN_THIS_ERROR);
    NW4R_ASSERT(fn_80067EE8(&camera), ANMSCN_FILE, 1474, "NW4R:Failed assertion camera.IsValid()");

    CameraAnmResult result;
    fn_80067EB4(&result);
    const CameraAnmResult *pResult = GetCameraResult(&result, refNumber);
    if (!(pResult->flags & CameraAnmResult::FLAG_ANM_EXISTS)) {
        return false;
    }

    switch (pResult->projType) {
    case 0:
        camera.SetPerspective(pResult->perspFovy, pResult->aspect, pResult->near, pResult->far);
        break;
    case 1: {
        f32 height = pResult->perspFovy / 2.0f;
        f32 width = height * pResult->aspect;
        camera.SetOrtho(height, -height, -width, width, pResult->near, pResult->far);
        break;
    }
    default:
        nw4r::db::Panic(ANMSCN_FILE, 1515, "NW4R:Fatal Error\nUnknown GXProjectionType(=%d)", pResult->projType);
        break;
    }

    Camera::PostureInfo posture;
    camera_posture_info_ctor(&posture);
    switch (pResult->flags & CameraAnmResult::FLAG_CAMERA_TYPE_MASK) {
    case 0: {
        math::VEC3 rotate;
        posture.mType = 1;
        copyVec3((math::VEC3 *)&posture.mUpX, (math::VEC3 *)vec3_copy_construct(&rotate, (void *)&pResult->rotate));
        break;
    }
    case 1: {
        math::VEC3 aim;
        posture.mType = 2;
        copyVec3((math::VEC3 *)&posture.mTargetX, (math::VEC3 *)vec3_copy_construct(&aim, (void *)&pResult->rotate));
        posture.mUnk28 = pResult->twist;
        break;
    }
    default:
        nw4r::db::Panic(ANMSCN_FILE, 1542, "NW4R:Fatal Error\nUnknown CameraAnmResult::CameraType(=%d)",
                        pResult->flags & CameraAnmResult::FLAG_CAMERA_TYPE_MASK);
        break;
    }
    camera.SetPosition(pResult->pos);
    camera.SetPosture(posture);
    return true;
}

/* 0x80067E54 (0x1C): copies three floats into `out` and returns it. */
extern "C" void *vec3_copy_construct(void *out, void *in)
{
    math::VEC3 *dst = (math::VEC3 *)out;
    const math::VEC3 *src = (const math::VEC3 *)in;
    dst->x = src->x;
    dst->y = src->y;
    dst->z = src->z;
    return out;
}

extern "C" nw4r::g3d::Camera::PostureInfo *camera_posture_info_ctor(nw4r::g3d::Camera::PostureInfo *self)
{
    VEC3_ctor((nw4r::math::VEC3 *)&self->mPosX);
    VEC3_ctor((nw4r::math::VEC3 *)&self->mTargetX);
    VEC3_ctor((nw4r::math::VEC3 *)&self->mUpX);
    return self;
}

/* The camera result's constructor: default-constructs its position. */
extern "C" CameraAnmResult *fn_80067EB4(CameraAnmResult *self)
{
    VEC3_ctor(&self->pos);
    return self;
}

extern "C" s32 fn_80067EE8(const void *p)
{
    return *(const u32 *)p != 0;
}

/* --------------------------------------------------------------------------------------------- *
 * The four result getters and the ID readers (0x80067EFC-0x80068A78).
 * --------------------------------------------------------------------------------------------- */

/* 0x80067EFC (0x1AC): the ambient light result `refNumber` names: the cached one, or evaluated at the current
 * frame into `pResult`. */
AmbLightAnmResult *AnmScnRes::GetAmbLightResult(AmbLightAnmResult *pResult, u32 refNumber)
{
    G3D_POINTER_ASSERT(ANMSCN_FILE, this, 1556, ANMSCN_THIS_ERROR);

    ResHandle resAmb;
    fn_80066D18(&resAmb, res_anm_scn_get_res_anm_amb_light_by_ref_number(&mRes, refNumber));
    if (!fn_80066D04(&resAmb)) {
        pResult->flags = 0;
        pResult->color = 0xFFFFFFFF;
        return pResult;
    }
    if (mpAmbLightCache != NULL) {
        return &mpAmbLightCache[fn_800680A8(&resAmb)];
    }
    res_anm_amb_light_get_anm_result(&resAmb, pResult, GetFrm());
    return pResult;
}

/* 0x800680A8 (0x24): resolves the ambient-light handle through the checked getter and reads its ID at +0xC. */
unsigned int fn_800680A8(void *obj)
{
    return ((const ResAnmIdHead *)fn_80066C8C((void *)obj))->id;
}

/* 0x800680CC (0x290): the light result `refNumber` names: the cached one, or evaluated at the current frame. */
LightAnmResult *AnmScnRes::GetLightResult(LightAnmResult *pResult, u32 refNumber)
{
    G3D_POINTER_ASSERT(ANMSCN_FILE, this, 1583, ANMSCN_THIS_ERROR);
    G3D_POINTER_ASSERT(ANMSCN_FILE, pResult, 1584, ANMSCN_RESULT_ERROR);

    ResHandle resLight;
    fn_80066F64(&resLight, res_anm_scn_get_res_anm_light_by_ref_number(&mRes, refNumber));
    if (!fn_80066F50(&resLight)) {
        pResult->flags = 0;
        return pResult;
    }
    if (mpLightCache != NULL) {
        return &mpLightCache[res_anm_light_get_id(&resLight)];
    }
    res_anm_light_get_anm_result(&resLight, pResult, GetFrm());
    return pResult;
}

/* 0x8006835C (0x24): the light's ID. */
extern "C" u32 res_anm_light_get_id(const ResHandle *p)
{
    return ((const ResAnmIdHead *)fn_80066E80((void *)p))->id;
}

/* 0x80068380 (0x290): the fog result `refNumber` names: the cached one, or evaluated at the current frame. */
FogAnmResult *AnmScnRes::GetFogResult(FogAnmResult *pResult, u32 refNumber)
{
    G3D_POINTER_ASSERT(ANMSCN_FILE, this, 1611, ANMSCN_THIS_ERROR);
    G3D_POINTER_ASSERT(ANMSCN_FILE, pResult, 1612, ANMSCN_RESULT_ERROR);

    ResHandle resFog;
    fn_800686C0(&resFog, res_anm_scn_get_res_anm_fog_by_ref_number(&mRes, refNumber));
    if (!fn_800686AC(&resFog)) {
        pResult->type = 0;
        return pResult;
    }
    if (mpFogCache != NULL) {
        return &mpFogCache[res_anm_fog_get_id(&resFog)];
    }
    res_anm_fog_get_anm_result(&resFog, pResult, GetFrm());
    return pResult;
}

/* 0x80068610 (0x24): the fog's ID. */
extern "C" u32 res_anm_fog_get_id(const ResHandle *p)
{
    return ((const ResAnmIdHead *)fn_80068634((void *)p))->id;
}

/* The checked `ResAnmFog` resolver. */
extern "C" u32 fn_80068634(void *p)
{
    if (fn_800686AC((const ResHandle *)p) == 0)
        nw4r::db::Panic((const char *)lbl_8058CB24, 62, (const char *)lbl_8058CB08,
                        (const char *)fn_800686A0(), "ref");
    return fn_80068698(p);
}

extern "C" u32 fn_80068698(void *p)
{
    return *(u32 *)p;
}

extern "C" u32 fn_800686A0(void)
{
    return (u32)lbl_8058CAFC;
}

extern "C" u32 fn_800686AC(const ResHandle *p)
{
    return p->mpData != 0;
}

/* Copies a `ResAnmFog` handle. */
extern "C" ResHandle *fn_800686C0(ResHandle *self, const u32 &src)
{
    fn_800686F0((u32 *)self, &src);
    return self;
}

extern "C" void fn_800686F0(u32 *dst, const u32 *src)
{
    *dst = *src;
}

/* 0x800686FC (0x290): the camera result `refNumber` names: the cached one, or evaluated at the current frame. */
CameraAnmResult *AnmScnRes::GetCameraResult(CameraAnmResult *pResult, u32 refNumber)
{
    G3D_POINTER_ASSERT(ANMSCN_FILE, this, 1639, ANMSCN_THIS_ERROR);
    G3D_POINTER_ASSERT(ANMSCN_FILE, pResult, 1640, ANMSCN_RESULT_ERROR);

    ResHandle resCamera;
    fn_80068A3C(&resCamera, res_anm_scn_get_res_anm_camera_by_ref_number(&mRes, refNumber));
    if (!fn_80068A28(&resCamera)) {
        pResult->flags = 0;
        return pResult;
    }
    if (mpCameraCache != NULL) {
        return &mpCameraCache[res_anm_camera_get_id(&resCamera)];
    }
    res_anm_camera_get_anm_result(&resCamera, pResult, GetFrm());
    return pResult;
}

/* 0x8006898C (0x24): the camera's ID. */
extern "C" u32 res_anm_camera_get_id(const ResHandle *p)
{
    return ((const ResAnmIdHead *)fn_800689B0((void *)p))->id;
}

/* The checked `ResAnmCamera` resolver. */
extern "C" u32 fn_800689B0(void *p)
{
    if (fn_80068A28((const ResHandle *)p) == 0)
        nw4r::db::Panic((const char *)lbl_8058CAE4, 125, (const char *)lbl_8058CAC8,
                        (const char *)fn_80068A1C(), "ref");
    return fn_80068A14(p);
}

extern "C" u32 fn_80068A14(void *p)
{
    return *(u32 *)p;
}

extern "C" u32 fn_80068A1C(void)
{
    return (u32)lbl_8058CAB8;
}

extern "C" u32 fn_80068A28(const ResHandle *p)
{
    return p->mpData != 0;
}

/* Copies a `ResAnmCamera` handle. */
extern "C" ResHandle *fn_80068A3C(ResHandle *self, const u32 &src)
{
    fn_80068A6C((u32 *)self, &src);
    return self;
}

extern "C" void fn_80068A6C(u32 *dst, const u32 *src)
{
    *dst = *src;
}

/* --------------------------------------------------------------------------------------------- *
 * The specular queries, the flags and the cache refresh (0x80068A78-0x80069334).
 * --------------------------------------------------------------------------------------------- */

/* 0x80068A78 (0x15C): the specular light-object index of the light `refNumber` names, or -1. */
u32 AnmScnRes::GetSpecularLightID(u32 refNumber) const
{
    G3D_POINTER_ASSERT(ANMSCN_FILE, this, 1669, ANMSCN_THIS_ERROR);

    ResHandle resLight;
    fn_80066F64(&resLight, res_anm_scn_get_res_anm_light_by_ref_number(&mRes, refNumber));
    if (fn_80066F50(&resLight)) {
        return fn_80066E5C(&resLight);
    }
    return (u32)-1;
}

/* 0x80068BD4 (0x168): whether the light `refNumber` names has a specular light. */
bool AnmScnRes::HasSpecularLight(u32 refNumber) const
{
    G3D_POINTER_ASSERT(ANMSCN_FILE, this, 1679, ANMSCN_THIS_ERROR);

    ResHandle resLight;
    fn_80066F64(&resLight, res_anm_scn_get_res_anm_light_by_ref_number(&mRes, refNumber));
    bool has = false;
    if (fn_80066F50(&resLight) && res_anm_light_config_test_flag8(&resLight)) {
        has = true;
    }
    return has;
}

/* 0x80068D3C (0x154): sets or clears a flag. */
void AnmScnRes::SetScnAnmFlag(ScnAnmFlag flag, bool on)
{
    G3D_POINTER_ASSERT(ANMSCN_FILE, this, 1689, ANMSCN_THIS_ERROR);
    if (on) {
        mScnAnmFlags |= flag;
    } else {
        mScnAnmFlags &= ~flag;
    }
}

/* 0x80068E90 (0x134): whether a flag is set. */
bool AnmScnRes::TestScnAnmFlag(ScnAnmFlag flag) const
{
    G3D_POINTER_ASSERT(ANMSCN_FILE, this, 1704, ANMSCN_THIS_ERROR);
    return (mScnAnmFlags & flag) != 0;
}

/* 0x80068FC4 (0x370): evaluates every ambient light, light, fog and camera at the current frame into the
 * caches. */
void AnmScnRes::UpdateCache()
{
    G3D_POINTER_ASSERT(ANMSCN_FILE, this, 1712, ANMSCN_THIS_ERROR);
    NW4R_ASSERT(TestScnAnmFlag(FLAG_HAS_CACHE), ANMSCN_FILE, 1713,
                "NW4R:Failed assertion TestScnAnmFlag(FLAG_HAS_CACHE)");

    u32 numAmbLight = fn_800902E4(&mRes);
    u32 numLight = fn_800904B8(&mRes);
    u32 numFog = fn_8009068C(&mRes);
    u32 numCamera = fn_80090860(&mRes);
    f32 frame = GetFrm();

    for (u32 i = 0; i < numAmbLight; i++) {
        ResHandle resAmb;
        fn_80066D18(&resAmb, res_anm_scn_get_res_anm_amb_light(&mRes, i));
        NW4R_ASSERT(fn_80066D04(&resAmb), ANMSCN_FILE, 1724, "NW4R:Failed assertion resAmb.IsValid()");
        res_anm_amb_light_get_anm_result(&resAmb, &mpAmbLightCache[i], frame);
    }
    for (u32 i = 0; i < numLight; i++) {
        ResHandle resLight;
        fn_80066F64(&resLight, res_anm_scn_get_res_anm_light(&mRes, i));
        NW4R_ASSERT(fn_80066F50(&resLight), ANMSCN_FILE, 1732, "NW4R:Failed assertion resLight.IsValid()");
        res_anm_light_get_anm_result(&resLight, &mpLightCache[i], frame);
    }
    for (u32 i = 0; i < numFog; i++) {
        ResHandle resFog;
        fn_800686C0(&resFog, res_anm_scn_get_res_anm_fog(&mRes, i));
        NW4R_ASSERT(fn_800686AC(&resFog), ANMSCN_FILE, 1740, "NW4R:Failed assertion resFog.IsValid()");
        res_anm_fog_get_anm_result(&resFog, &mpFogCache[i], frame);
    }
    for (u32 i = 0; i < numCamera; i++) {
        ResHandle resCamera;
        fn_80068A3C(&resCamera, res_anm_scn_get_res_anm_camera(&mRes, i));
        NW4R_ASSERT(fn_80068A28(&resCamera), ANMSCN_FILE, 1748, "NW4R:Failed assertion resCamera.IsValid()");
        res_anm_camera_get_anm_result(&resCamera, &mpCameraCache[i], frame);
    }
}

/* --------------------------------------------------------------------------------------------- *
 * The run-time type checks (0x80069334-0x8006946C).
 * --------------------------------------------------------------------------------------------- */

/* 0x80069334 (0x6C): whether the object is an AnmScn or derives from `type`. */
bool AnmScn::IsDerivedFrom(TypeObj type) const
{
    if (type == GetTypeObjStatic()) {
        return true;
    }
    return nw4r::g3d::G3dObj::IsDerivedFrom(type);
}

/* 0x800693A0 (0x30): returns the AnmScn type. */
const nw4r::g3d::G3dObj::TypeObj AnmScn::GetTypeObjStatic()
{
    const u8 *pName;
    return *reinterpret_cast<const TypeObj *>(type_obj_set_name(&pName, (const u8 *)&anm_typename_AnmScn));
}

/* 0x800693D0 (0x6C): whether the object is an AnmScnRes or derives from `type`. */
bool AnmScnRes::IsDerivedFrom(TypeObj type) const
{
    if (type == GetTypeObjStatic()) {
        return true;
    }
    return AnmScn::IsDerivedFrom(type);
}

/* 0x8006943C (0x30): returns the AnmScnRes type. */
const nw4r::g3d::G3dObj::TypeObj AnmScnRes::GetTypeObjStatic()
{
    const TypeObj::TypeName *pName;
    return *reinterpret_cast<const TypeObj *>(fn_8005DC60((void **)&pName, (void *)&anm_typename_AnmScnRes));
}
