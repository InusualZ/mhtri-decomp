/*
 * Declarations for the symbols `src/camera/camera_main.cpp` owns that other units use (docs/plan.md 6.5, rule 2).
 */
#ifndef MHTRI_CAMERA_CAMERA_MAIN_H
#define MHTRI_CAMERA_CAMERA_MAIN_H

#include "types.h"
#include "nw4r/math.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802BE038 */
u32 get_camera_rot_y(void);

/* 0x802BDDB0 - the current camera's target (look-at) position, written through `out`. */
void get_camera_target_pos(nw4r::math::VEC3* out);

/* 0x802BE39C - 1 when the camera work is idle and the current camera's +0x188 mode byte is 1. */
u32 camera_mode1_ck(void);

/* 0x802BE5D0 - posts the camera request `kind` for the enemy `self`. */
void camera_em_shake_req(struct _ENEMY_WORK* self, u32 kind);

/* 0x802B954C */
void camera_flag_0x145_set(void);
/* 0x802B9574 */
void camera_target_slot_clear(u8 reset);

void camera_pl_quake_req(struct _PLW* player, u8 kind);
void camera_mode2_req(void);
void camera_mode3_req(void);
#ifdef __cplusplus
}
#endif

#endif /* MHTRI_CAMERA_CAMERA_MAIN_H */
