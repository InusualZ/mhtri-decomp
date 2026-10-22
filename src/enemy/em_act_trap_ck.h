/* enemy/em_act_trap_ck.h - leaf header for the `enemy/em_common.cpp` helpers `enemy/fn_8011D448.cpp` calls (docs/plan.md
 * 6.5 rule 2), in the types the call sites use: retail compares the flag/move checks unsigned (`cmplwi`), so they
 * return `u32` here; kept out of `enemy/em_common.h`, whose owner defines several of them `s32`. */
#ifndef MHTRI_ENEMY_EM_ACT_TRAP_CK_H
#define MHTRI_ENEMY_EM_ACT_TRAP_CK_H

#include "types.h"

struct _ENEMY_WORK;

#ifdef __cplusplus
extern "C" {
#endif

u8* em_rank_rec_get(struct _ENEMY_WORK* self);
u32 em_attacker_rec_get(struct _ENEMY_WORK* self, u32 index);
void em_act_request_now(struct _ENEMY_WORK* self, u32 a, u32 b);
void em_act_request_react(struct _ENEMY_WORK* self, u32 a, u32 b);
void em_hate_pl_add(struct _ENEMY_WORK* self, u32 player, s32 amount);
void em_hate_misc_add(struct _ENEMY_WORK* self, s32 amount);
void em_hate_em_add(struct _ENEMY_WORK* self, u32 enemy, s32 amount);
void em_hit_latch_set(struct _ENEMY_WORK* self);
u32 em_ally_kind_ck(struct _ENEMY_WORK* self, u32 kind);
u32 em_act_flinch_light_ck(struct _ENEMY_WORK* self);
u32 em_act_part_flinch_ck(struct _ENEMY_WORK* self);
u32 em_act_trap_ck(struct _ENEMY_WORK* self);
u32 em_act_paralysis_ck(struct _ENEMY_WORK* self);
u32 em_act_sleep_ck(struct _ENEMY_WORK* self);
u32 em_act_kind23_ck(struct _ENEMY_WORK* self);
u32 em_act_wake_ck(struct _ENEMY_WORK* self);
u32 em_act_stun_ck(struct _ENEMY_WORK* self);
s32 em_act_sleep_side_get(struct _ENEMY_WORK* self);
u32 em_act_kind6_ck(u32 state, u32 sub);
u32 em_act_leap_ck(u32 state, u32 action);
u32 em_act_roar_ck(struct _ENEMY_WORK* self);
u32 em_act_ball_ck(struct _ENEMY_WORK* self, u32 flag);
u32 em_act_kind18_ck(struct _ENEMY_WORK* self);
u32 em_quest_entry_live_ck(struct _ENEMY_WORK* self);
u32 em_damage_area_ok_ck(struct _ENEMY_WORK* self);
u32 em_ailment8_window_ck(struct _ENEMY_WORK* self);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_EM_ACT_TRAP_CK_H */
