/* Leaf header (docs/plan.md 6.5 rule 2) for `parts_mdl_release_all` (0x80268770), the `Pl/player_control.cpp`
 * release of every equipment part model a player work holds (`player_control.cpp` has no header of its own). */
#ifndef MHTRI_PL_PARTS_MDL_RELEASE_ALL_H
#define MHTRI_PL_PARTS_MDL_RELEASE_ALL_H

#include "types.h"

#ifdef __cplusplus
struct _PLW;
/* 0x80268770 - releases every part model of `plw`. */
void parts_mdl_release_all(_PLW* plw);
#endif

#endif /* MHTRI_PL_PARTS_MDL_RELEASE_ALL_H */
