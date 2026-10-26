/*
 * Leaf header (docs/plan.md 6.5 rule 2) for the effect system's initialiser, owned by `ef/ef_effectsystem.cpp`.
 * GUESS name `ef_system_initialize`: the owner documents the body as `EffectSystem::Initialize` (it builds the group
 * table for `maxGroupID` groups and binds the borrowed objects).
 */
#ifndef MHTRI_EF_EF_SYSTEM_INITIALIZE_H
#define MHTRI_EF_EF_SYSTEM_INITIALIZE_H

#include "ef/ef_effectsystem.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800A56B0 (0x250): Builds the system's group table for `maxGroupID` groups. */
u32 ef_system_initialize(EfSys* self, u32 maxGroupID);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_EF_SYSTEM_INITIALIZE_H */
