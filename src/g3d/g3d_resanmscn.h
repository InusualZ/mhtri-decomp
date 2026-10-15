/* g3d/g3d_resanmscn.h - the five `ResAnmTexPat` helpers of `g3d/g3d_resanmscn.cpp` that `g3d/g3d_resanmtexsrt.cpp`
 *   calls through its `ResFile`/`ResDic` chain (C linkage).  The owner does not include this header, so no unit
 *   sees two declarations of one C symbol. */
#ifndef MHTRI_G3D_G3D_RESANMSCN_H
#define MHTRI_G3D_G3D_RESANMSCN_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8009125C - the byte array that follows the resolved data head (the next unit's comparator walks
 * it as a `{s32 length; char name[];}` record). */
char* fn_8009125C(void* self);
/* 0x800912D4 - the word at +0x0 of the resolved data pointer. */
u32 fn_800912D4(void* self);
/* 0x800913A0 - `ResAnmTexPat::Bind(ResFile)`: resolve every texture/palette name against `file` and
 * store the handles; returns whether every entry resolved. */
u32 fn_800913A0(void* self, void* file);
/* 0x80091628 - `ResAnmTexPat::Release()`: reset every texture/palette handle in the two arrays. */
void fn_80091628(void* self);

/* `ResAnmScn`'s reference-number getters `g3d/g3d_anmscn.cpp` calls: each resolves the channel record a reference
 * number names and returns its handle word (zero when there is none). */
struct ResHandle;
u32 res_anm_scn_get_res_light_set_by_ref_number(const struct ResHandle* pSelf, u32 refNumber);     /* 0x800908FC */
u32 res_anm_scn_get_res_anm_amb_light_by_ref_number(const struct ResHandle* pSelf, u32 refNumber); /* 0x80090ADC */
u32 res_anm_scn_get_res_anm_light_by_ref_number(const struct ResHandle* pSelf, u32 refNumber);     /* 0x80090CBC */
u32 res_anm_scn_get_res_anm_fog_by_ref_number(const struct ResHandle* pSelf, u32 refNumber);       /* 0x80090E9C */
u32 res_anm_scn_get_res_anm_camera_by_ref_number(const struct ResHandle* pSelf, u32 refNumber);    /* 0x8009107C */
/* 0x800902E0/0x800904B4/0x80090688/0x8009085C - the `ResAnmScn` sub-resource of each kind at index `idx`. */
u32 res_anm_scn_get_res_anm_amb_light(const struct ResHandle* pSelf, u32 idx);
u32 res_anm_scn_get_res_anm_light(const struct ResHandle* pSelf, u32 idx);
u32 res_anm_scn_get_res_anm_fog(const struct ResHandle* pSelf, u32 idx);
u32 res_anm_scn_get_res_anm_camera(const struct ResHandle* pSelf, u32 idx);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_G3D_G3D_RESANMSCN_H */
