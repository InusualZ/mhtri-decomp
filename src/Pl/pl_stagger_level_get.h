/* `pl_stagger_level_get`, defined by `Pl/pl_act.cpp` (leaf header, rule 2). */
#ifndef MHTRI_PL_PL_STAGGER_LEVEL_GET_H
#define MHTRI_PL_PL_STAGGER_LEVEL_GET_H

#include "types.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

/* The player's stagger level (0 when not staggered). */
s32 pl_stagger_level_get(struct _PLW* plw);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_PL_STAGGER_LEVEL_GET_H */
