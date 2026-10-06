/* `lobby_hunter_cards` (`.bss` 0x806BE340, ten 0x130-byte hunter id/card records, GUESS name), owned by
 * `lobby/lb_companion_ui.cpp`: its leaf header, for consumers that cannot include the owner's full header (it clashes with
 * the enemy band's and the network band's headers). */
#ifndef MHTRI_LOBBY_LOBBY_HUNTER_CARDS_H
#define MHTRI_LOBBY_LOBBY_HUNTER_CARDS_H

#include "types.h"
#include "lobby/lb_quest_page.h"   /* LbQuestPage */

extern LbQuestPage lobby_hunter_cards[10]; /* .bss 0x806BE340 */

#endif /* MHTRI_LOBBY_LOBBY_HUNTER_CARDS_H */
