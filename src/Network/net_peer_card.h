/* Network/net_peer_card.h - the hunter card record, shared by `Network/net_session_close.h` (`copyPeerProfileCard`
 *   fills it) and `menu/movie.h` (the friend picker keeps eight). */
#ifndef MHTRI_NETWORK_NET_PEER_CARD_H
#define MHTRI_NETWORK_NET_PEER_CARD_H

#include "types.h"

/* The peer card `copyPeerProfileCard` fills from the community peer record (the menu units' hunter card): the name,
 * the id text, the profile's id, rank and level bytes, its nine equipment records in card order, its comment, the
 * three names, the server and settings bytes and two words.  Field names GUESSED from the source fields.
 * size: 0x212 (approximation: the last byte written is +0x211) */
typedef struct NetPeerCard {
    /* +0x000 */ char name_0x000[0x14];
    /* +0x014 */ char id_0x014[0xA];       /* `formatNetId` text */
    /* +0x01E */ u16 id_0x01E;
    /* +0x020 */ u8 level_0x020;
    /* +0x021 */ u8 rank_0x021;
    /* +0x022 */ u8 equip_0x022[9][0xC];  /* `_EQUIP` records */
    /* +0x08E */ u16 equip_ids_0x08E[8];  /* the shown equipment ids (`card_equip_ids_build`) */
    /* +0x09E */ char comment_0x09E[0xAC];
    /* +0x14A */ char names_0x14A[3][0x40];
    /* +0x20A */ u8 server_0x20A;
    /* +0x20B */ u8 settings_0x20B[4];
    /* +0x20F */ s8 value_0x20F;
    /* +0x210 */ s8 value_0x210;
    /* +0x211 */ u8 status_0x211;
} NetPeerCard; /* size: 0x212 */

#endif /* MHTRI_NETWORK_NET_PEER_CARD_H */
