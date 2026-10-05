/* Leaf header for `lb_sub0d_send` (0x80339038), owned by `lobby/lb_companion_ui.cpp` whose full header
 * carries the lobby's own record views.  It broadcasts one sub-0x0D command (a player's serial-table row or act request)
 * to the other players.
 */
#ifndef MHTRI_LOBBY_LB_SUB0D_SEND_H
#define MHTRI_LOBBY_LB_SUB0D_SEND_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Sends the sub-0x0D command for player `index`: the record's bytes at +5 and +6, its halfword at +8 and
 * `flag` go into the message (the owner's own prototype spells the flag `s8`; the shell pool's call
 * site narrows it with `clrlwi`, i.e. passes a `u8`). */
void lb_sub0d_send(u8 index, void* record /* untyped: a caller-owned record (a serial-table row or a lobby act request) whose +5/+6 bytes and +8 halfword the message carries */, u8 flag);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_SUB0D_SEND_H */
