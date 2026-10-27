/* `lb_title_hunt_rows`, defined by `lobby/lb_npc.cpp` (leaf header, rule 2). */
#ifndef MHTRI_LOBBY_LB_TITLE_HUNT_ROWS_H
#define MHTRI_LOBBY_LB_TITLE_HUNT_ROWS_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The hunt-count title rows, two halfwords each: a monster id and the first of its three titles (0xFFFF ends the table). */
extern u16 lb_title_hunt_rows[38];

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_TITLE_HUNT_ROWS_H */
