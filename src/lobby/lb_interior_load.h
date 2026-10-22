/* lobby/lb_interior_load.h - leaf header (docs/plan.md 6.5 rule 2): the interior-model resource helpers
 *   `lobby/lb_quest_screen.cpp` defines, for `menu/menu_placeinfo.cpp`.  C linkage (the map rows are plain names);
 *   the signatures are the owner's definitions.  GUESS names (the owner's header). */
#ifndef MHTRI_LOBBY_LB_INTERIOR_LOAD_H
#define MHTRI_LOBBY_LB_INTERIOR_LOAD_H

#include "types.h"
#include "lobby/lobby_work.h"   /* LbResSlot */

#ifdef __cplusplus
extern "C" {
#endif

/* 0x803A79C0 / 0x803A7CD8 / 0x803A7D30 / 0x803A7D48: loads interior `id` into a resource slot, drops one user of
 * it, returns its file path and its kind. */
LbResSlot* lb_interior_load(u8 mode, u8 sub, u8 id);
void lb_interior_release(u8 id);
char* lb_interior_path_get(u8 id);
u8 lb_interior_kind_get(u8 kind);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_INTERIOR_LOAD_H */
