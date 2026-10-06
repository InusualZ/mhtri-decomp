/* lobby/lb_quest_page.h - the record type of `lobby_hunter_cards` (`lobby/lb_companion_ui.cpp`), shared by the owner's
 * full header and its leaf header `lobby/lobby_hunter_cards.h`. */
#ifndef MHTRI_LOBBY_LB_QUEST_PAGE_H
#define MHTRI_LOBBY_LB_QUEST_PAGE_H

#include "types.h"

/* The 0x130-byte hunter record `hud_key_lookup` scans for a six-byte key: a used byte, the hunter's id text (its
 * first six bytes are the key) and the hunter's 0x100-byte card (`updatePeerCardBlock` refreshes it).  size: 0x130 */
typedef struct LbQuestPage {
    /* +0x000 */ u8 active_0x000;
    /* +0x001 */ u8 unused_0x001[0x2];
    /* +0x003 */ u8 key_0x03[6];
    /* +0x009 */ u8 unused_0x009[0x1B];
    /* +0x024 */ u8 card_0x024[0x100];
    /* +0x124 */ u8 unused_0x124[0xC];
} LbQuestPage; /* size: 0x130 */

#endif /* MHTRI_LOBBY_LB_QUEST_PAGE_H */
