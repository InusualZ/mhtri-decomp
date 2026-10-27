/* `lb_smith_open`, defined by `lobby/lb_screen_step.cpp` (leaf header, rule 2). */
#ifndef MHTRI_LOBBY_LB_SMITH_OPEN_H
#define MHTRI_LOBBY_LB_SMITH_OPEN_H

#include "types.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

/* Opens the smithy screen for `plw` and the smith NPC. */
void lb_smith_open(struct _PLW* plw, u8* npc);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_SMITH_OPEN_H */
