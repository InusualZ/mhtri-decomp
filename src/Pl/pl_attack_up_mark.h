/* Pl/pl_attack_up_mark.h - leaf header: `pl_attack_up_mark` (0x8027D698), owned by `Pl/pl_act.cpp`. C linkage. */
#ifndef MHTRI_PL_PL_ATTACK_UP_MARK_H
#define MHTRI_PL_PL_ATTACK_UP_MARK_H

#include "types.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif
/* Records the attack-up level and its frames on the player (+0x452, +0x458). */
void pl_attack_up_mark(struct _PLW* self, s8 level, s16 frames);
#ifdef __cplusplus
}
#endif

#endif
