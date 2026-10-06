/*
 * Declarations of `Pl/pl_act.cpp`'s part/motion section (0x802693C4-0x8026BA1C) and its equipment-type reader; the
 * signatures are the callers' or the definition's, whichever the caller settled.  The six mangled symbols
 * (`set_com_motion_type__FUc`, `Get_motion_no__FP4_PLW`, `Pl_frame_check__FP4_PLWUlff`, `Pl_chr_setX__FP4_PLWUsll`,
 * `Pl_chr_set_attr__FP4_PLWUsllUl`, `pl_get_joint_wpos__FP4_PLWUlPQ34nw4r4math4VEC3`) are declared with their real
 * C++ signatures (rule 9); the map's unmangled `fn_` stems stay `extern "C"`.
 */
#ifndef MHTRI_PL_FN_802693C4_H
#define MHTRI_PL_FN_802693C4_H

#include "types.h"
#include "nw4r/math.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

u32 fn_802693C4(s32 player, u8 part, s32 value);
u32 fn_80269474(s32 player, u8 part, s32 value);
s32 fn_80269508(struct _PLW* self, s32 part);
s32 fn_8026954C(s32 player, u8 part, s32 value);
void fn_802695A4(u8 player, void* equipA, void* equipB);
u8 fn_802699AC(void);
s16 fn_8026A00C(u16 id);
s16 fn_8026A068(struct _PLW* self, u16 id);
void Pl_chr_set_attr_default(struct _PLW* self, u16 motion, s32 a, s32 b);
void fn_8026A230(struct _PLW* self, s32 a, u16 motion, s32 b, s32 c);
void fn_8026A23C(struct _PLW* self, s32 index, f32 value);
void fn_8026A2BC(struct _PLW* self);
void fn_8026A2D0(struct _PLW* self, u8 value);
void fn_8026A2DC(struct _PLW* self);
void pl_model_set_state(struct _PLW* self, u8 value);
void fn_8026A2F8(struct _PLW* self);
u32 fn_8026A328(struct _PLW* self, u16 frame, f32 a, f32 b);
u32 Pl_motion_end_ck(struct _PLW* self);
f32 fn_8026A34C(struct _PLW* self);
f32 pl_rig_get_float_a4(struct _PLW* self);
u32 fn_8026A364(struct _PLW* self);
void fn_8026A394(struct _PLW* self, s32 joint, nw4r::math::MTX34* out);
u8 fn_8026A3A0(struct _PLW* self);
void pl_act_set_cam_ang(struct _PLW* self);
void fn_8026A4E4(struct _PLW* self);
void fn_8026A518(struct _PLW* self);
void fn_8026A570(struct _PLW* self);
void fn_8026A590(struct _PLW* self);
void fn_8026A618(struct _PLW* self, s32 id);
/* The id-flag tests return `u32`: `Pl/pl_act_step.cpp` compares `pl_part_flag_ck(...) == 1` and retail performs a
 * `cmplwi r3,1` there (`cmpwi` is the signed form). */
u32 pl_part_flag_ck(struct _PLW* self, s32 id);
void fn_8026A678(struct _PLW* self, s32 id);
s32 fn_8026A6A4(struct _PLW* self, s32 id);
void fn_8026A6D8(struct _PLW* self, s32 id);
s32 fn_8026A6F4(struct _PLW* self, s32 id);
void fn_8026A718(struct _PLW* self, s32 id);
u32 fn_8026A178(struct _PLW* self, s32 a, u16 motion, s32 b, s32 c, u32 e);
u32 fn_8026B934(struct _PLW* self);
u32 fn_8026B99C(struct _PLW* self);
u32 fn_8026BA04(struct _PLW* self);
void fn_8026AF08(struct _PLW* self, u32 value);

#ifdef __cplusplus
}

/* The mangled half (rule 9).  `Pl_chr_set_attr` takes the fifth `u32` argument `Pl_chr_set_attr_default` fills in
 * with 0; `Pl_frame_check`/`fn_8026A328` ignore their two floats and pass a constant flag instead. */
void set_com_motion_type(u8 type);
u16 Get_motion_no(struct _PLW* self);
u32 Pl_frame_check(struct _PLW* self, u32 mask, f32 a, f32 b);
void Pl_chr_setX(struct _PLW* self, u16 motion, s32 a, s32 b);
u32 Pl_chr_set_attr(struct _PLW* self, u16 motion, s32 a, s32 b, u32 d);
void pl_get_joint_wpos(struct _PLW* self, u32 joint, nw4r::math::VEC3* out);

/* 0x8027EED0 - the equipment-type reader `fn_802695A4` calls (`Pl/pl_act.cpp`'s equipment section); the map spells
 * it `Get_pl_type__FP6_EQUIPP6_EQUIP`, so the declaration is the real C++ signature (rule 9). */
u8 Get_pl_type(struct _EQUIP* equipA, struct _EQUIP* equipB);
#endif

#endif /* MHTRI_PL_FN_802693C4_H */
