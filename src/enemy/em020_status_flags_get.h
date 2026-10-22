/* Leaf header (docs/plan.md 6.5 rule 2): `enemy/em020_prog.cpp`'s `em020_status_flags_get`, the one declaration
 * `ai/ai_npc.cpp` needs. */
#ifndef MHTRI_ENEMY_EM020_STATUS_FLAGS_GET_H
#define MHTRI_ENEMY_EM020_STATUS_FLAGS_GET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80375628 (0x1B8): the em020 status bits the HUD and network read (0x80 aim target, 0x40 cooldown, 0x08 low HP,
 * 0x04/0x02/0x01 the head break stage, 0x10/0x20 the interpreter's progress). */
u8 em020_status_flags_get(struct _ENEMY_WORK* self);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_EM020_STATUS_FLAGS_GET_H */
