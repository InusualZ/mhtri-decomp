/* Leaf header for `lobby/lb_companion_ui.cpp`'s 0x80339038, which broadcasts one sub-0x0D command (a player's
 * serial-table row or act request) to the other players. */
#ifndef MHTRI_LOBBY_LB_SUB0D_SEND_H
#define MHTRI_LOBBY_LB_SUB0D_SEND_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Sends the record's +5/+6 bytes, its +8 halfword and `flag` for player `index` (the owner spells the flag `s8`; the
 * shell pool's call site narrows it with `clrlwi`, a `u8`). */
void lb_sub0d_send(u8 index, void* record /* untyped: a caller-owned record (a serial-table row or a lobby act request) whose +5/+6 bytes and +8 halfword the message carries */, u8 flag);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_SUB0D_SEND_H */
