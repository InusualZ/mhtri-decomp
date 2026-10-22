/* enemy/em025_prog.h - the declarations of `enemy/em025_prog.cpp` its consumers call (docs/plan.md 6.5 rule 2). */
#ifndef MHTRI_ENEMY_EM025_PROG_H
#define MHTRI_ENEMY_EM025_PROG_H

#include "types.h"

struct _ENEMY_WORK;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8019E70C - the shore action's sub-phase of enemy record `self` (255 when it is not that action). */
u32 em025_shore_phase_get(struct _ENEMY_WORK* self);
/* 0x8019E840 - 1 while the shore action's byte stack is in one of its two late windows. */
u32 em025_shore_window_ck(struct _ENEMY_WORK* self);
/* 0x8019E960 / 0x8019E9AC - set and test bit `id` of the action bit set at +0x354. */
void em025_act_bit_set(struct _ENEMY_WORK* self, s32 id);
s32 em025_act_bit_ck(struct _ENEMY_WORK* self, s32 id);
/* 0x8019EA04 - releases the four roster slots and frees their area-table states. */
void em025_slots_release(struct _ENEMY_WORK* self);
/* 0x8019EC38 - shakes the camera at joint `joint` (-1: the work position) at the ground height. */
void em025_joint_quake(struct _ENEMY_WORK* self, s32 joint, s32 kind, s32 level);
/* 0x801A9748 / 0x801A98F8 - the sway curve step and the head tracking the motion events run each frame. */
void em025_sway_update(struct _ENEMY_WORK* self);
void em025_head_track(struct _ENEMY_WORK* self);
/* 0x801A9DF4 - releases the joint-effect slots and re-arms the two the record owns (`enemy/fn_8011D448.cpp` calls it). */
void em025_effect_slots_reset(struct _ENEMY_WORK* self);
/* 0x801A9E60 - the shore action's target test (`menu/get_pop_dat_ptr.cpp` calls it): 1, 2 or 0. */
u32 em025_shore_target_ck(struct _ENEMY_WORK* self);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_EM025_PROG_H */
