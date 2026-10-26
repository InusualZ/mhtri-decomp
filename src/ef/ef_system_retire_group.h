/*
 * Leaf header (docs/plan.md 6.5 rule 2) for the effect system's group retire, owned by `ef/ef_effectsystem.cpp`.
 * GUESS name `ef_system_retire_group`: the owner documents the body as retiring every effect on one group's active
 * list.
 */
#ifndef MHTRI_EF_EF_SYSTEM_RETIRE_GROUP_H
#define MHTRI_EF_EF_SYSTEM_RETIRE_GROUP_H

#include "ef/ef_effectsystem.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800A5D8C (0xE0): Retires every effect on the group's active list. */
u32 ef_system_retire_group(EfSys* self, u32 groupID);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_EF_SYSTEM_RETIRE_GROUP_H */
