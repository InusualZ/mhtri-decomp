/* `lb_menu_marker_update`, defined by `lobby/lb_menu_scratch.cpp` (leaf header, rule 2). */
#ifndef MHTRI_LOBBY_LB_MENU_MARKER_UPDATE_H
#define MHTRI_LOBBY_LB_MENU_MARKER_UPDATE_H

#include "types.h"

struct LbMenuScratch;

#ifdef __cplusplus
extern "C" {
#endif

/* Rebuilds the lobby target marker's rows over the scratch block `scratch`. */
void lb_menu_marker_update(struct LbMenuScratch* scratch);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_MENU_MARKER_UPDATE_H */
