/* sound/mhchar_motion_set.h - the declarations of `mhchar_motion_set`, `mhchar_motion_layer_set`, `mhchar_motion_flag_ck`, `mhchar_mat_hide_set`, which `sound/mhchar.cpp` owns (docs/plan.md 6.5 rule 2), with the
 * argument views `enemy/em_common.cpp` calls them through. */
#ifndef MHTRI_SOUND_MHCHAR_MOTION_SET_H
#define MHTRI_SOUND_MHCHAR_MOTION_SET_H

#include "types.h"

struct MHchar;

#ifdef __cplusplus
extern "C" {
#endif
void mhchar_motion_set(struct MHchar* chr, u32 a, u32 b, u16 motion, s32 c, s32 d, s32 e, f32 f, f32 g);
void mhchar_motion_layer_set(struct MHchar* chr, s32 layer, u32 a, u32 b, u16 motion, s32 c, s32 d, s32 e, f32 f, f32 g);
u32 mhchar_motion_flag_ck(struct MHchar* chr, s32 flag);
void mhchar_mat_hide_set(struct MHchar* chr, s32 mat, s32 hide);
#ifdef __cplusplus
}
#endif

#endif /* MHTRI_SOUND_MHCHAR_MOTION_SET_H */
