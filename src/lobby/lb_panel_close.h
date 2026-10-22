/* lobby/lb_panel_close.h - leaf header (docs/plan.md 6.5 rule 2): `lobby/fn_80212810.cpp`'s `lb_panel_close`, for a
 *   consumer that cannot include the lobby band.  C linkage (the map row is a plain name). */
#ifndef MHTRI_LOBBY_LB_PANEL_CLOSE_H
#define MHTRI_LOBBY_LB_PANEL_CLOSE_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80217934: closes the open lobby panel. */
s32 lb_panel_close(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_PANEL_CLOSE_H */
