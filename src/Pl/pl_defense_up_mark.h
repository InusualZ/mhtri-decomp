/* Pl/pl_defense_up_mark.h - leaf header: `pl_defense_up_mark` (0x8027D6A4), owned by `Pl/pl_act.cpp`. C linkage. */
#ifndef MHTRI_PL_PL_DEFENSE_UP_MARK_H
#define MHTRI_PL_PL_DEFENSE_UP_MARK_H

#include "types.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif
/* Records the defence-up level on the player (+0x450) and raises its frames (+0x454) to `frames`. */
void pl_defense_up_mark(struct _PLW* self, u8 level, s16 frames);
#ifdef __cplusplus
}
#endif

#endif
