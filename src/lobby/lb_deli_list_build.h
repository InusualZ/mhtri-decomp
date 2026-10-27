/* Leaf header (docs/plan.md 6.5 rule 2): the delivery-list entry points `lobby/lb_companion_ui.cpp` defines and
 * `lobby/lb_screen_step.cpp` calls.  C linkage (the map rows are plain names). */
#ifndef MHTRI_LOBBY_LB_DELI_LIST_BUILD_H
#define MHTRI_LOBBY_LB_DELI_LIST_BUILD_H

#include "types.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8033B6C0 - builds the delivery list for the player `plw` and the NPC record `npc`; returns the row count. */
s16 lb_deli_list_build(struct _PLW* plw, u8* npc);
/* 0x8033B990 - steps the delivery list: 2 once it is cancelled, 1 once its talk page has closed. */
s32 lb_deli_menu_step(void);
/* 0x8033C1AC - draws the delivery list. */
void lb_deli_menu_draw(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_DELI_LIST_BUILD_H */
