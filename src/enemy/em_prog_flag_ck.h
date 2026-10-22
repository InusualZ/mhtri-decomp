/* Leaf header (docs/plan.md 6.5 rule 2): symbols `enemy/em_prog_support.cpp` defines, for `hud/cockpit_quest.cpp`. */
#ifndef MHTRI_ENEMY_EM_PROG_FLAG_CK_H
#define MHTRI_ENEMY_EM_PROG_FLAG_CK_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8038392C: whether the enemy program flag halfword is set. */
u32 em_prog_flag_ck(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_EM_PROG_FLAG_CK_H */
