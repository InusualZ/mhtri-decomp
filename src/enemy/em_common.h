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

/* 0x80135AA0 - the enemy data kind byte (`EnemyData::kind_0x0C`) of the record. */
u8 em_data_kind_get(struct _ENEMY_WORK* self);

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
s32 em_area_entry_near_ck(struct _ENEMY_MINI_WORK* entry, u8 index, u8 sub);
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
void em_mot_layer_set(struct _ENEMY_WORK* self, s32 layer, u32 motion, s32 start, s32 frame);
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
/* 0x80133BA4 - sets the +0x762 latch with the timer `frames`. */
void em_latch_0x762_set(struct _ENEMY_WORK* self, s16 frames);
/* 0x80131EF0 - arms the +0x81E timer for 2 frames. */
void em_timer_0x81E_arm(struct _ENEMY_WORK* self);
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
/* 0x80131E80 */
void em_busy_timer_clear(struct _ENEMY_WORK* self);
/* 0x80126BD4 */
u32 em_area_entry_enter_ck(struct _ENEMY_MINI_WORK* entry, u8 map, u8 area);
/* 0x80126E3C */
u8 em_kind_area_next(u8 group, u8 team, u8 from, u8 to, u16 seed);

/* The helpers `enemy/fn_8011D448.cpp` calls, as the owner defines them. */
s32 em_part_slot_get(struct _ENEMY_WORK* self, u8 part);
s16 em_frames_get(struct _ENEMY_WORK* self);
void em_act_request(struct _ENEMY_WORK* self, u8 a, u8 b);
void em_hate_add(struct _ENEMY_WORK* self, u8 player, s32 amount);
s32 em_spawn_rec_live_ck(struct _ENEMY_WORK* self);
u32 em_mot_no_ck(struct _ENEMY_WORK* self, u32 motion);
u32 em_in_water_ck(struct _ENEMY_WORK* self, u32 flag);
void em_sight_player_set(struct _ENEMY_WORK* self, u8 player);
u32 em_ailment_mask_ck(struct _ENEMY_WORK* self, u8 bits);
void em_ailment_mask_set(struct _ENEMY_WORK* self, u8 bits);
u32 em_poison_add(struct _ENEMY_WORK* self, s16 amount);
u32 em_sleep_add(struct _ENEMY_WORK* self, s16 amount);
u32 em_paralysis_add(struct _ENEMY_WORK* self, s16 amount);
u32 em_stun_add(struct _ENEMY_WORK* self, s16 amount);
u32 em_exhaust_add(struct _ENEMY_WORK* self, s16 amount);
u32 em_ailment8_add(struct _ENEMY_WORK* self, s16 amount);
void em_action_finish_swim(struct _ENEMY_WORK* self);

#ifdef __cplusplus
}
#endif

/* Declarations `enemy/em020_prog.cpp` calls (its lobby/quest bands). */
#ifdef __cplusplus
extern "C" {
/* 0x80131DA8 */
void em_water_surface_latch(struct _ENEMY_WORK* self);
}
#endif

/* A joint point set: the joint, the point count and the joint-local points (a list ended by joint 0xFFFFFFFF).
 * size: 0xC */
struct EmJointPoints {
    /* +0x0 */ u32 joint;
    /* +0x4 */ u8 count;
    /* +0x5 */ u8 pad_0x5[3];
    /* +0x8 */ nw4r::math::VEC3* points;
};

/* Declarations `enemy/enemy_control.cpp` calls. */
struct _ENEMY_MINI_WORK;
#ifdef __cplusplus
extern "C" {
#endif
/* One marker callback of an enemy kind; `enemy/enemy_control.cpp` calls it with the area entry. */
typedef void (*EmEntryProc)(struct _ENEMY_MINI_WORK* entry);
/* 0x801265F8 */
EmEntryProc em_entry_proc_get(struct _ENEMY_MINI_WORK* entry, u8 marker);
/* 0x801267DC */
s16 em_kind_frames_get(struct _ENEMY_WORK* self);
/* 0x80126868 */
s16 em_kind_frames2_get(struct _ENEMY_WORK* self);
/* 0x80126B5C */
u32 em_area_enter_ck(struct _ENEMY_WORK* self, u8 map, u8 area);
/* 0x8012E718 */
u32 em_spawn_rec_live_ck2(struct _ENEMY_WORK* self);
/* 0x80135AE8 */
void em_break_parts_reset(struct _ENEMY_WORK* self);
/* 0x80136BD4 */
u32 em_data_word_get(struct _ENEMY_WORK* self);
/* 0x80136C20 */
u8 em_joint_points_get(struct _ENEMY_WORK* self, struct EmJointPoints* set, nw4r::math::VEC3* out);
#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_EM_COMMON_H */
