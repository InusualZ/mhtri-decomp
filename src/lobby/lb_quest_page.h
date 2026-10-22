/* lobby/lb_quest_page.h - the record type of `lobby_hunter_cards` (`lobby/lb_companion_ui.cpp`), shared by the owner's
 * full header and its leaf header `lobby/lobby_hunter_cards.h`. */
#ifndef MHTRI_LOBBY_LB_QUEST_PAGE_H
#define MHTRI_LOBBY_LB_QUEST_PAGE_H

#include "types.h"

/* The 0x130-byte hunter record `hud_key_lookup` scans for a six-byte key: a used byte, the hunter's id text (its
 * first six bytes are the key) and the hunter's 0x100-byte card (`updatePeerCardBlock` refreshes it).  size: 0x130 */
typedef struct LbQuestPage {
    /* +0x000 */ u8 active_0x000;
    /* +0x001 */ u8 slot_0x001;     /* the player slot the card belongs to */
    /* +0x002 */ u8 loaded_0x002;   /* set once the card's player work is loaded */
    /* +0x003 */ union {
        u8 key_0x03[6];
        char id_0x03[0xA];          /* the hunter's id text */
    };
    /* +0x00D */ char name_0x0D[0x17]; /* the hunter's name */
    /* +0x024 */ u8 card_0x024[0x100];
    /* +0x124 */ struct _PLW* player_0x124; /* the player work the card is loaded into */
    /* +0x128 */ u8 unused_0x128[0x8];
} LbQuestPage; /* size: 0x130 */

#endif /* MHTRI_LOBBY_LB_QUEST_PAGE_H */
