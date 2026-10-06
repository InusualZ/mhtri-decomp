/* Leaf header for the link senders and entry checks of `lobby/lb_companion_ui.cpp` (0x80338AD4, 0x80338B4C, 0x80338C48,
 * 0x80338CE4, 0x80339EA8) the arena task includes; the `lb_sub0*_send` names are GUESSes after the protocol
 * sub-command their bodies build. */
#ifndef MHTRI_LOBBY_LB_SUB0A_SEND_H
#define MHTRI_LOBBY_LB_SUB0A_SEND_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Sends the sub-0x0A / sub-0x0B command with one byte payload for entry `index` (gated on the link). */
void lb_sub0a_send(u8 index, u8 value);
void lb_sub0b_send(u8 index, u8 value);

/* Whether every pad bit recorded for entry `index` belongs to an occupied server slot; whether the
 * entry's handled byte is set. */
s32 lb_seen_pad_ck(u8 index);
s32 lb_handled_ck(u8 index);

/* Sends the sub-0x1D command with two bytes.  Retail passes the flag without an `extsb`: the owner's `s8` spelling
 * matches and a `u8` one does not, so the two prototypes differ and no TU may include both. */
void lb_sub1d_send(u8 value, u8 flag);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_SUB0A_SEND_H */
