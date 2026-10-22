/* Leaf header (docs/plan.md 6.5 rule 2): `lobby/fn_80212810.cpp`'s `lb_speaker_name_get`.  C linkage (the map row is a
 * plain name). */
#ifndef MHTRI_LOBBY_LB_SPEAKER_NAME_GET_H
#define MHTRI_LOBBY_LB_SPEAKER_NAME_GET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8021800C - the display name of lobby speaker `npc` (an NPC kind; the trade-ship captains are 0x14..0x16). */
s8* lb_speaker_name_get(u16 npc);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_SPEAKER_NAME_GET_H */
