/* `lb_award_equip_kind_lists`, defined by `lobby/lb_npc.cpp` (leaf header, rule 2). */
#ifndef MHTRI_LOBBY_LB_AWARD_EQUIP_KIND_LISTS_H
#define MHTRI_LOBBY_LB_AWARD_EQUIP_KIND_LISTS_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Per equipment kind, the 0xFFFF-terminated list of pieces the equipment award counts (null for a kind with none). */
extern u16* lb_award_equip_kind_lists[16];

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_AWARD_EQUIP_KIND_LISTS_H */
