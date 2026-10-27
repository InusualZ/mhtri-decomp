/* ef/ef_inst_spawn_joint.h - the declaration of `ef_inst_spawn_joint`, which `ef/eft019.cpp` owns (docs/plan.md 6.5
 * rule 2), with the scale `enemy/em_common.cpp` passes in f1.  GUESS name (the owner's body). */
#ifndef MHTRI_EF_EF_INST_SPAWN_JOINT_H
#define MHTRI_EF_EF_INST_SPAWN_JOINT_H

#include "types.h"

struct _ENEMY_WORK;

#ifdef __cplusplus
extern "C" {
#endif
/* 0x80114C6C: spawns an eft020 record for effect `key` on the actor's joint `joint`, at scale `scale`. */
void ef_inst_spawn_joint(struct _ENEMY_WORK* actor, u8 key, s32 joint, f32 scale);
#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_EF_INST_SPAWN_JOINT_H */
