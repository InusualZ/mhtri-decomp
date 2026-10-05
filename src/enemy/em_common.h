/* Declarations owned by `src/enemy/em_common.*` (docs/plan.md 6.5 rule 2): a consumer includes this header instead of declaring the symbols itself. */
#ifndef MHTRI_ENEMY_EM_COMMON_H
#define MHTRI_ENEMY_EM_COMMON_H

#include "types.h"
#include "nw4r/math.h"
#include "enemy/fn_8012E968.h"
#include "enemy/fn_801502C8.h"

/* Declarations moved here from `unsplit/enemy.h` (docs/plan.md 6.5 rule 2: the owner declares). */
struct _ENEMY_WORK;

#ifdef __cplusplus
extern "C" {
#endif

void CancelFade(struct _ENEMY_WORK *self);

void em_mot_set(struct _ENEMY_WORK* self, s32 a, s32 b, s32 c);

/* 0x8012F504 - the five-argument motion setter `em_mot_set` tail-calls; moved here from
 * `enemy/fn_801550FC.cpp` on landing (rule 2).  `em_mot_set` narrows its second argument to u16
 * (`clrlwi r4,r4,16`) before the tail call, so the owner's first argument is u16. */
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
 * landed callers pass (`enemy/fn_8014A1BC.c` and `enemy/fn_801D80EC.cpp` both call it
 * `(self, mask)`). */
u32 em_flags836_ck(struct _ENEMY_WORK* self, u32 a);

void em_action_finish(struct _ENEMY_WORK* self);

void em_state_set(struct _ENEMY_WORK* self, u32 a, u32 b);

void em_part_hit_set(struct _ENEMY_WORK* self, u32 a, u32 b);

void em_frame_flag_set(struct _ENEMY_WORK* self);

void em_approach_start(struct _ENEMY_WORK* self, f32 speed, u32 flags);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_EM_COMMON_H */
