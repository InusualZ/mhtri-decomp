/* Leaf header (docs/plan.md 6.5 rule 2): `g3d/g3d_resanmchr.cpp`'s `anm_chr_result_rot_get` and its siblings the
 * rig's world-matrix callbacks call.  C linkage (the map rows are plain names). */
#ifndef MHTRI_G3D_ANM_CHR_RESULT_ROT_GET_H
#define MHTRI_G3D_ANM_CHR_RESULT_ROT_GET_H

#include "types.h"

struct ResAnmChrObj;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8008C484 - builds `out` as the rotation by `x`, `y`, `z` degrees (GUESS name). */
void mtx34_rot_xyz_deg(f32* out, f32 x, f32 y, f32 z);
/* 0x8008DC4C / 0x8008EB68 - reads / writes an animation result's rotation in degrees (GUESS names). */
s32 anm_chr_result_rot_get(struct ResAnmChrObj* self, f32* out);
void anm_chr_result_rot_set(struct ResAnmChrObj* self, const f32* in);
/* 0x8008E698 / 0x8008F3DC - reads / writes an animation result's rotation matrix (GUESS names). */
void anm_chr_result_mtx_get(struct ResAnmChrObj* self, f32* out);
void anm_chr_result_mtx_set(struct ResAnmChrObj* self, const f32* in);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_G3D_ANM_CHR_RESULT_ROT_GET_H */
