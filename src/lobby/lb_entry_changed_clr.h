/* lobby/lb_entry_changed_clr.h - leaf header (docs/plan.md 6.5 rule 2): `lobby/lb_companion_ui.cpp`'s
 *   `lb_entry_changed_clr`, for `lobby/fn_802FA9A0.cpp`.  C linkage (the map row is a plain name). */
#ifndef MHTRI_LOBBY_LB_ENTRY_CHANGED_CLR_H
#define MHTRI_LOBBY_LB_ENTRY_CHANGED_CLR_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8033C384: clears the user block's "entry changed" bit and republishes the entry byte. */
void lb_entry_changed_clr(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_ENTRY_CHANGED_CLR_H */
