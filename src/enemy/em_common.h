/* Declarations `enemy/em_common.cpp` owns, for the units that call them. */
#ifndef MHTRI_ENEMY_EM_COMMON_H
#define MHTRI_ENEMY_EM_COMMON_H

#include "types.h"
#include "nw4r/math.h"
#include "enemy/fn_8012E968.h"
#include "enemy/fn_801502C8.h"

/* `enemy/em_common.cpp`'s own declarations. */
struct _ENEMY_WORK;

#ifdef __cplusplus
extern "C" {
#endif

void CancelFade(struct _ENEMY_WORK *self);

void em_mot_set(struct _ENEMY_WORK* self, s32 a, s32 b, s32 c);

/* 0x8012F504 - the five-argument motion setter `em_mot_set` tail-calls; `em_mot_set` narrows its second argument to
 * u16 (`clrlwi r4,r4,16`) first, so the owner's first argument is u16. */
void em_mot_set_blend(struct _ENEMY_WORK* self, u32 a, u32 b, u32 c, u32 d);

void em_mot_set_ck(struct _ENEMY_WORK *self, u32 a, u32 b, u32 c);

f32 get_em_base_scale(struct _ENEMY_WORK *self);

f32 em_fall_height_get(struct _ENEMY_WORK* self);

void em_move_mode_set(struct _ENEMY_WORK *self, u32 a);

void em_fall_start(struct _ENEMY_WORK *self);

u32 em_status_set(struct _ENEMY_WORK* work, s32 kind);

void em_alt_mode_set(struct _ENEMY_WORK* enemy, s32 value);

void em_busy_timer_reset(struct _ENEMY_WORK* work);

u32 em_status_ck(struct _ENEMY_WORK* enemy, s32 value);

void em_state_refresh(struct _ENEMY_WORK* work);

u32 em_turn_to_target(struct _ENEMY_WORK *self, u32 a);

void em_turn_in_window(struct _ENEMY_WORK* self, f32 lo, f32 hi, s32 angle);

u32 em_approach_step(struct _ENEMY_WORK* self, s32 a, s32 b);

void em_lift_start(struct _ENEMY_WORK *self);

void em_lift_step(struct _ENEMY_WORK *self);

void em_dive_start(struct _ENEMY_WORK *self);

void em_dive_step(struct _ENEMY_WORK *self);

void em_move_vec_clr(struct _ENEMY_WORK *self);

void em_move_vec2_clr(struct _ENEMY_WORK *self);

void em_move_offset_apply(struct _ENEMY_WORK *self);

/* 0x80135748 - the part-mask probe: r3 (`self`) and r4, which it narrows to u16 (`clrlwi r4,r4,16`)
 * before ANDing it against the record's `flags_0x836`; the body's `neg`/`or`/`srwi 31` returns 1
 * when any masked bit is set, so the result is a u32 0/1 and every call site compares it with
 * `cmplwi`.  The old-style `s32 em_flags836_ck()` declaration could not carry the two arguments the
 * landed callers pass (`enemy/em001_prog.cpp` and `enemy/em007_act.cpp` both call it
 * `(self, mask)`). */
u32 em_flags836_ck(struct _ENEMY_WORK* self, u32 a);

void em_action_finish(struct _ENEMY_WORK* self);

void em_state_set(struct _ENEMY_WORK* self, u32 a, u32 b);

void em_part_hit_set(struct _ENEMY_WORK* self, u32 a, u32 b);

void em_frame_flag_set(struct _ENEMY_WORK* self);

void em_approach_start(struct _ENEMY_WORK* self, f32 speed, u32 flags);

/* 0x8012E644 / 0x8012E654 / 0x8012E884 / 0x8012E8C0 - the record's hit test, the mini record's, whether the enemy
 * was captured, and the mini record's end kind (2 when it left without a kill).  GUESS names. */
s32 em_record_hit_ck(struct _ENEMY_WORK* record);
s32 em_mini_hit_ck(struct _ENEMY_WORK* record);
u8 em_captured_ck(struct _ENEMY_WORK* enemy);
/* 0x80126AD0 - whether area entry `entry` lies in area `sub` or one area `index` reaches from it (GUESS name). */
s32 em_area_entry_near_ck(struct EmAreaEntry* entry, u8 index, u8 sub);
s32 em_mini_kill_kind_get(struct _ENEMY_WORK* enemy);
/* 0x8013763C - sets the record's +0x43F latch the roar step arms (GUESS name). */
void em_roar_latch_set(struct _ENEMY_WORK* self);
/* 0x8013023C - the stage byte of the enemy record `work` (`ef/eft_slot.cpp` reads it for its bound enemy). */
u8 em_phase_get(struct _ENEMY_WORK* work);
/* 0x8012D878 - whether the enemy record `work` is in its enraged state (`ef/eft_slot.cpp`'s tracking reads it). */
s32 em_action_mid_ck(struct _ENEMY_WORK* work);
/* 0x801271BC - the area of `data`'s map table nearest `pos` from area `area` (`ef/eft_slot.cpp` flees to it). */
struct EnemyData;
u8 em_nearest_area_get(struct EnemyData* data, nw4r::math::VEC3* pos, u8 area);

/* 0x80136D38 */
u32 em_eft_hide_ck(_ENEMY_WORK* enemy);

/* 0x8012F74C */
void em_mot_layer_set(struct _ENEMY_WORK* self, u32 layer, u16 motion, u32 a, u32 b);
/* 0x80130088 */
s32 em_surface_reach_ck(struct _ENEMY_WORK* self, f32 depth);
/* 0x80135468 */
u32 em_move_offset_fall_update(struct _ENEMY_WORK* self, f32 limit);

/* 0x8012FEB4 */
u32 em_ground_snap_offset_ck(struct _ENEMY_WORK* self, f32 offset);
/* 0x801303A0 */
void em_fall_speed_set(struct _ENEMY_WORK* self, f32 speed);
/* 0x8013063C */
void em_joint_drop_start(struct _ENEMY_WORK* self, u32 joint);
/* 0x80131D90 */
void em_hide_set(struct _ENEMY_WORK* self);

/* 0x8012F6A0 */
void em_mot_layer_blend_set(struct _ENEMY_WORK* self, u32 layer, u16 motion, u32 frames, s32 start, u32 mode);
/* 0x8013216C */
void em_rest_flag_set(struct _ENEMY_WORK* self);

/* 0x8012813C */
void em_action_finish_sub2(struct _ENEMY_WORK* self);
/* 0x80128194 */
void em_action_finish_dive(struct _ENEMY_WORK* self);
/* 0x80130308 */
f32 em_ceiling_depth_get(struct _ENEMY_WORK* self);
/* 0x801306C0 */
void em_move_sub_mode_set(struct _ENEMY_WORK* self, u8 sub);
/* 0x80130CE8 */
void em_stamina_regen(struct _ENEMY_WORK* self, f32 rate, s16 delta, s16 period);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_EM_COMMON_H */
