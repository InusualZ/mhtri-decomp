/* enemy/em_kind.h - the declarations of `enemy/em_kind.cpp` its consumers call (docs/plan.md 6.5 rule 2). */
#ifndef MHTRI_ENEMY_EM_KIND_H
#define MHTRI_ENEMY_EM_KIND_H

#include "types.h"

struct EnemyData;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80140C00 - the enemy data record of team `group`, entry `index`. */
struct EnemyData* enemy_data_by_team_get(u8 group, u8 index);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_EM_KIND_H */
