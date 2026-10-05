/* The lobby equipment-page unit `lobby/lb_equip_page.cpp`: the page helpers and `.data` tables the NPC control band
 * and the player action unit (`Pl/pl_act.cpp`) reach.
 */
#ifndef MHTRI_LOBBY_LB_EQUIP_PAGE_H
#define MHTRI_LOBBY_LB_EQUIP_PAGE_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

void fn_80220114(void);
void fn_80223258(struct _PLW* self, u8 value);

/* 0x805BAA90 / 0x805BAC98 - the per-act effect-id and joint-offset tables the player action unit indexes by
 * its act number (`_PLW::field_0x002`). */
extern u8 lbl_805BAA90[];
extern u8 lbl_805BAC98[];

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_EQUIP_PAGE_H */
