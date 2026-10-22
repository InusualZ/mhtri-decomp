/* lobby/lb_world_submode_get.h - leaf header (docs/plan.md 6.5 rule 2): `lobby/fn_801E7530.cpp`'s
 *   `lb_world_submode_get`, for `menu/menu_placeinfo.cpp`.  C linkage (the map row is a plain name).  GUESS name
 *   (the owner's header). */
#ifndef MHTRI_LOBBY_LB_WORLD_SUBMODE_GET_H
#define MHTRI_LOBBY_LB_WORLD_SUBMODE_GET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x801E9B68: the lobby world's sub-mode byte (the place info reads it as the farm plot count). */
u8 lb_world_submode_get(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_WORLD_SUBMODE_GET_H */
