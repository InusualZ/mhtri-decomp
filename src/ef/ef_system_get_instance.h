/*
 * Leaf header (docs/plan.md 6.5 rule 2) for the effect-system singleton accessor, owned by `ef/ef_effectsystem.cpp`.
 * GUESS name `ef_system_get_instance`: the 12-byte body returns the system's one instance.
 */
#ifndef MHTRI_EF_EF_SYSTEM_GET_INSTANCE_H
#define MHTRI_EF_EF_SYSTEM_GET_INSTANCE_H

#include "ef/ef_effectsystem.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800A56A4 (0xC): Returns the effect system's one instance. */
EfSys* ef_system_get_instance(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_EF_SYSTEM_GET_INSTANCE_H */
