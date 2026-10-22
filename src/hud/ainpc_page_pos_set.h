/* Leaf header (docs/plan.md 6.5 rule 2): the `hud/cockpit` unit's symbol `menu/menu_item.cpp` calls, for a
 * consumer that cannot include the owner's full header.  C linkage (the map row is a plain name). */
#ifndef MHTRI_HUD_AINPC_PAGE_POS_SET_H
#define MHTRI_HUD_AINPC_PAGE_POS_SET_H

#include "types.h"
#include "hud/layout_types.h"   /* _mh_ivec2_, passed by value */

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802DE24C - copies the caller's position pair into ainpc_page_state +0x32 (a uv_pair_copy tail call) */
void ainpc_page_pos_set(_mh_ivec2_ pos);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_HUD_AINPC_PAGE_POS_SET_H */
