/* Leaf header (docs/plan.md 6.5 rule 2): `Pl/pl_act.cpp`'s `pl_info_text_set`, the one declaration `lobby/lb_npc.cpp` needs.
 * C linkage (the map row is a plain name).  GUESS name. */
#ifndef MHTRI_PL_PL_INFO_TEXT_SET_H
#define MHTRI_PL_PL_INFO_TEXT_SET_H

#include "types.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8027552C - clears the player's info text and copies `info` into it (memset, strcpy) */
void pl_info_text_set(struct _PLW* self, char* info);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_PL_INFO_TEXT_SET_H */
