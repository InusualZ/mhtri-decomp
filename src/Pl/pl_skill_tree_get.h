/* Pl/pl_skill_tree_get.h - leaf header (docs/plan.md 6.5 rule 2): `Pl/pl_act.cpp`'s skill tree lookup, for `menu/movie.cpp`.  GUESS name (the registry carries it). */
#ifndef MHTRI_PL_PL_SKILL_TREE_GET_H
#define MHTRI_PL_PL_SKILL_TREE_GET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802718C8 - the skill tree that holds active skill `skill` (0 for none). */
u8 pl_skill_tree_get(u16 skill);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_PL_SKILL_TREE_GET_H */
