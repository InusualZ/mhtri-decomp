/* The 4-byte wire header every net message of the character-state sync starts with (`hud/net_char_sync.cpp`
 * owns `NetMsgHeader::fill`; `lobby/lb_companion_ui.cpp` writes the same header for its command packets, so the
 * type lives in its own header - docs/plan.md 6.5 rule 1).
 */
#ifndef MHTRI_HUD_NETMSGHEADER_H
#define MHTRI_HUD_NETMSGHEADER_H

#include "types.h"

/* The 4-byte header every sender of this unit writes first.  `+0x1` is the sender's own slot index,
 * `+0x2` the destination (the following slot for a player message, the fixed 5 an enemy message uses)
 * and `+0x3` the kind the receiver switches on. size: 0x4 */
typedef struct NetMsgHeader {
    void fill(u8 from, u8 to, u8 kind);   /* writes the sender, destination and kind */
    /* +0x0 */ u8 reserved;   /* always 0 */
    /* +0x1 */ u8 from_slot;
    /* +0x2 */ u8 to_slot;
    /* +0x3 */ u8 kind;
} NetMsgHeader;

#endif /* MHTRI_HUD_NETMSGHEADER_H */
