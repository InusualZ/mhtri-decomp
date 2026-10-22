/* enemy/em009_act.h - `enemy/em009_act.cpp`'s entry points: the `em009_prog_tbl` slots, the action dispatchers and
 * the per-action state steps they select, the effect router and the user-data helper's constructor/release. */
#ifndef MHTRI_ENEMY_EM009_ACT_H
#define MHTRI_ENEMY_EM009_ACT_H

#include "types.h"

struct _ENEMY_WORK;
struct Em009UserData;

#ifdef __cplusplus
extern "C" {
#endif
void em009_work_clear(struct _ENEMY_WORK* self);
void em009_init(struct _ENEMY_WORK* self, u8 mode);
struct Em009UserData* em009_user_data_ctor(struct Em009UserData* self);
void em009_motion_next(struct _ENEMY_WORK* self, u8* state, u8* sub);
void em009_event_notify(struct _ENEMY_WORK* self, u8 kind, u8 sub);
void em009_frame_update(struct _ENEMY_WORK* self);
void em009_act0_0(struct _ENEMY_WORK* self);
void em009_act0_1(struct _ENEMY_WORK* self);
void em009_act0_2(struct _ENEMY_WORK* self);
void em009_act0_7(struct _ENEMY_WORK* self);
void em009_act0_dispatch(struct _ENEMY_WORK* self);
void em009_act1_0(struct _ENEMY_WORK* self);
void em009_act1_1(struct _ENEMY_WORK* self);
void em009_act1_2(struct _ENEMY_WORK* self);
void em009_act1_3(struct _ENEMY_WORK* self);
void em009_act1_4(struct _ENEMY_WORK* self);
void em009_act1_5(struct _ENEMY_WORK* self);
void em009_act1_6(struct _ENEMY_WORK* self);
void em009_act1_7(struct _ENEMY_WORK* self);
void em009_act1_8(struct _ENEMY_WORK* self);
void em009_act1_dispatch(struct _ENEMY_WORK* self);
void em009_act2_0(struct _ENEMY_WORK* self, u8 a, u8 b);
void em009_act2_6(struct _ENEMY_WORK* self, u8 a);
void em009_act2_7(struct _ENEMY_WORK* self);
void em009_act2_9(struct _ENEMY_WORK* self);
void em009_act2_10(struct _ENEMY_WORK* self);
void em009_act2_11(struct _ENEMY_WORK* self, u8 a);
void em009_act2_13(struct _ENEMY_WORK* self);
void em009_act2_dispatch(struct _ENEMY_WORK* self);
void em009_act6_0(struct _ENEMY_WORK* self);
void em009_act6_1(struct _ENEMY_WORK* self);
void em009_act6_2(struct _ENEMY_WORK* self);
void em009_act6_3(struct _ENEMY_WORK* self, u8 a);
void em009_act6_4(struct _ENEMY_WORK* self);
void em009_act6_5(struct _ENEMY_WORK* self);
void em009_act6_10(struct _ENEMY_WORK* self, u8 a);
void em009_act6_dispatch(struct _ENEMY_WORK* self);
void em009_act7_0(struct _ENEMY_WORK* self);
void em009_act7_1(struct _ENEMY_WORK* self, u8 a);
void em009_act7_36(struct _ENEMY_WORK* self);
void em009_act7_2(struct _ENEMY_WORK* self, u8 a);
void em009_act7_3(struct _ENEMY_WORK* self, u8 a);
void em009_act7_5(struct _ENEMY_WORK* self, u8 a, u8 b);
void em009_act7_8(struct _ENEMY_WORK* self);
void em009_act7_9(struct _ENEMY_WORK* self, u8 a);
void em009_act7_dispatch(struct _ENEMY_WORK* self);
void em009_act10_se_play(struct _ENEMY_WORK* self);
void em009_act11_se_play(struct _ENEMY_WORK* self);
void em009_act12_0(struct _ENEMY_WORK* self);
void em009_act12_dispatch(struct _ENEMY_WORK* self);
void em009_act13_0(struct _ENEMY_WORK* self);
void em009_act13_1(struct _ENEMY_WORK* self);
void em009_act13_dispatch(struct _ENEMY_WORK* self);
void em009_action_dispatch(struct _ENEMY_WORK* self);
void em009_alt_mode_effect(struct _ENEMY_WORK* self);
void em009_effect_spawn(struct _ENEMY_WORK* self, u8 a, u8 b, u32 c, s32 d, f32 scale);
void em009_motion_events(struct _ENEMY_WORK* self);
void em009_tev_color_update(void);
u32 em009_state_query(struct _ENEMY_WORK* self, u8 kind);
void em009_death_action_get(struct _ENEMY_WORK* self, u8* action, u8* sub);
void em009_cooldown_clear(struct _ENEMY_WORK* self);
int em009_part_broken_ck(struct _ENEMY_WORK* self);
u32 em009_action_select(struct _ENEMY_WORK* self, u32 arg);
struct Em009UserData* em009_user_data_dtor(struct Em009UserData* self, s32 flags);
#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_EM009_ACT_H */
